#!/usr/bin/env python3
"""Download offline documentation into docs/ and verify the downloaded files."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import stat
import sys
import tempfile
import zipfile
from dataclasses import dataclass
from email.message import Message
from pathlib import Path
from typing import Iterable
from urllib.error import HTTPError, URLError
from urllib.parse import unquote, urlparse
from urllib.request import Request, urlopen


PDF_DOCS = [
    {"url": "https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2", "path": "24593_3.44_APM_Vol2.pdf"},
    {"url": "https://cdrdv2.intel.com/v1/dl/getContent/671436", "path": "253665-092-sdm-vol-1.pdf"},
    {"url": "https://cdrdv2.intel.com/v1/dl/getContent/671110", "path": "325383-092-sdm-vol-2abcd.pdf"},
    {"url": "https://cdrdv2.intel.com/v1/dl/getContent/671447", "path": "325384-092-sdm-vol-3abcd.pdf"},
    {"url": "https://cdrdv2.intel.com/v1/dl/getContent/671098", "path": "335592-092-sdm-vol-4.pdf"},
    {"url": "https://www.scs.stanford.edu/nyu/04fa/lab/specsbbs101.pdf", "path": "specsbbs101.pdf"},
]

ARCHIVE_DOCS = [
    {
        "url": "https://files.osdev.org/osdev_wiki.zip",
        "path": "wiki.osdev.org",
        "marker": "wiki.osdev.org/index.html",
        "name": "OSDev Wiki offline archive",
    },
]

CHUNK_SIZE = 1024 * 1024
MIN_PDF_SIZE = 1024
MIN_ZIP_SIZE = 1024
MANIFEST_VERSION = 1


@dataclass(frozen=True)
class DownloadResult:
    url: str
    path: Path
    status: str
    size: int
    sha256: str | None
    message: str


def parse_content_disposition(headers: Message) -> str | None:
    value = headers.get("Content-Disposition")
    if not value:
        return None

    # Handles the common forms:
    #   attachment; filename="manual.pdf"
    #   attachment; filename*=UTF-8''manual.pdf
    match = re.search(r"filename\*=([^']*)''([^;]+)", value, re.IGNORECASE)
    if match:
        return unquote(match.group(2).strip().strip('"'))

    match = re.search(r'filename="?([^";]+)"?', value, re.IGNORECASE)
    if match:
        return unquote(match.group(1).strip())

    return None


def fallback_filename(url: str, default_extension: str) -> str:
    parsed = urlparse(url)
    path = unquote(parsed.path.rstrip("/"))
    name = Path(path).name

    if name and name != "getContent":
        return ensure_extension(name, default_extension)

    parent = Path(path).parent.name
    if parent.isdigit():
        return f"{parsed.netloc.replace('.', '_')}_{parent}{default_extension}"

    query_tail = parsed.query.rsplit("/", 1)[-1] if parsed.query else ""
    if query_tail.isdigit():
        return f"{parsed.netloc.replace('.', '_')}_{query_tail}{default_extension}"

    digest = hashlib.sha256(url.encode("utf-8")).hexdigest()[:12]
    return f"document_{digest}{default_extension}"


def ensure_extension(name: str, extension: str) -> str:
    return name if name.lower().endswith(extension.lower()) else f"{name}{extension}"


def safe_filename(name: str, default_extension: str) -> str:
    name = Path(name).name
    name = re.sub(r"[^A-Za-z0-9._+-]+", "_", name).strip("._")
    if not name:
        name = f"document{default_extension}"
    return ensure_extension(name, default_extension)


def read_prefix(path: Path) -> bytes:
    with path.open("rb") as handle:
        return handle.read(512).lstrip()


def looks_like_html(path: Path, content_type: str | None) -> tuple[bool, str]:
    normalized_content_type = (content_type or "").lower()
    if "html" in normalized_content_type:
        return True, f"server returned HTML content type: {content_type}"

    lowered = read_prefix(path)[:256].lower()
    if lowered.startswith(b"<!doctype html") or lowered.startswith(b"<html") or b"<html" in lowered:
        return True, "downloaded content looks like HTML"

    return False, ""


def sniff_pdf(path: Path, content_type: str | None) -> tuple[bool, str]:
    size = path.stat().st_size
    prefix = read_prefix(path)
    with path.open("rb") as handle:
        handle.seek(max(0, size - 4096))
        suffix = handle.read()

    if size < MIN_PDF_SIZE:
        return False, f"too small to be a PDF ({size} bytes)"

    html, message = looks_like_html(path, content_type)
    if html:
        return False, message

    if not prefix.startswith(b"%PDF-"):
        return False, "downloaded content does not start with %PDF-"

    if b"%%EOF" not in suffix:
        return False, "downloaded content is missing %%EOF near the end"

    return True, "valid PDF signature and trailer"


def sniff_zip(path: Path, content_type: str | None) -> tuple[bool, str]:
    size = path.stat().st_size
    if size < MIN_ZIP_SIZE:
        return False, f"too small to be a ZIP archive ({size} bytes)"

    html, message = looks_like_html(path, content_type)
    if html:
        return False, message

    if not zipfile.is_zipfile(path):
        return False, "downloaded content is not a valid ZIP archive"

    return True, "valid ZIP archive"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(CHUNK_SIZE), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tree_sha256(root: Path) -> tuple[str, int]:
    digest = hashlib.sha256()
    file_count = 0
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        rel_path = path.relative_to(root).as_posix()
        file_digest = sha256_file(path)
        size = path.stat().st_size
        digest.update(rel_path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(str(size).encode("ascii"))
        digest.update(b"\0")
        digest.update(file_digest.encode("ascii"))
        digest.update(b"\n")
        file_count += 1
    return digest.hexdigest(), file_count


def load_manifest(path: Path) -> dict[str, object]:
    if not path.exists():
        return {"version": MANIFEST_VERSION, "artifacts": {}}

    with path.open("r", encoding="utf-8") as handle:
        manifest = json.load(handle)

    if not isinstance(manifest, dict):
        raise ValueError(f"manifest is not a JSON object: {path}")
    manifest.setdefault("version", MANIFEST_VERSION)
    manifest.setdefault("artifacts", {})
    if not isinstance(manifest["artifacts"], dict):
        raise ValueError(f"manifest artifacts is not a JSON object: {path}")
    return manifest


def save_manifest(path: Path, manifest: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2, sort_keys=True)
        handle.write("\n")


def manifest_entry(manifest: dict[str, object], url: str) -> dict[str, object] | None:
    artifacts = manifest.get("artifacts", {})
    if not isinstance(artifacts, dict):
        return None
    entry = artifacts.get(url)
    return entry if isinstance(entry, dict) else None


def set_manifest_entry(manifest: dict[str, object], url: str, entry: dict[str, object]) -> bool:
    artifacts = manifest.setdefault("artifacts", {})
    if not isinstance(artifacts, dict):
        raise ValueError("manifest artifacts is not a JSON object")
    if artifacts.get(url) == entry:
        return False
    artifacts[url] = entry
    return True


def pdf_manifest_entry(url: str, path: Path, docs_dir: Path) -> dict[str, object]:
    return {
        "kind": "pdf",
        "path": path.relative_to(docs_dir).as_posix(),
        "size": path.stat().st_size,
        "sha256": sha256_file(path),
    }


def archive_manifest_entry(url: str, path: Path, docs_dir: Path) -> dict[str, object]:
    digest, file_count = tree_sha256(path)
    return {
        "kind": "archive_tree",
        "path": path.relative_to(docs_dir).as_posix(),
        "file_count": file_count,
        "tree_sha256": digest,
    }


def expected_digest(entry: dict[str, object] | None) -> str | None:
    if not entry:
        return None
    value = entry.get("sha256") or entry.get("tree_sha256")
    return value if isinstance(value, str) else None


def prompt_hash_change(label: str, expected: str, actual: str, accept_hash_changes: bool) -> bool:
    if expected == actual:
        return True
    if accept_hash_changes:
        return True

    print(f"hash mismatch: {label}", file=sys.stderr)
    print(f"  expected: {expected}", file=sys.stderr)
    print(f"  actual:   {actual}", file=sys.stderr)
    if not sys.stdin.isatty():
        print("refusing to update because stdin is not interactive; rerun with --accept-hash-changes to allow it", file=sys.stderr)
        return False

    answer = input("Downloaded content differs from the recorded hash. Accept and update manifest? [y/N] ")
    return answer.strip().lower() in {"y", "yes"}


def verify_existing_hash(
    manifest: dict[str, object],
    url: str,
    actual: str,
    label: str,
) -> tuple[bool, str]:
    expected = expected_digest(manifest_entry(manifest, url))
    if expected is None:
        return True, "no recorded hash yet"
    if expected == actual:
        return True, "matches recorded hash"
    return False, f"existing content differs from recorded hash for {label}"


def download_to_temp(url: str, docs_dir: Path, filename: str, accept: str) -> tuple[Path, Message, str]:
    request = Request(url, headers={"Accept": accept, "User-Agent": "osdev-doc-fetcher/1.0"})
    docs_dir.mkdir(parents=True, exist_ok=True)

    with urlopen(request, timeout=60) as response:
        headers = response.headers
        final_url = response.geturl()
        with tempfile.NamedTemporaryFile(
            mode="wb",
            prefix=f".{filename}.",
            suffix=".part",
            dir=docs_dir,
            delete=False,
        ) as tmp:
            tmp_path = Path(tmp.name)
            while True:
                chunk = response.read(CHUNK_SIZE)
                if not chunk:
                    break
                tmp.write(chunk)

    return tmp_path, headers, final_url


def resolve_pdf_doc(pdf_doc: dict[str, str] | str) -> tuple[str, str | None]:
    if isinstance(pdf_doc, str):
        return pdf_doc, None
    return pdf_doc["url"], pdf_doc.get("path")


def download_pdf(
    pdf_doc: dict[str, str] | str,
    docs_dir: Path,
    manifest: dict[str, object],
    force: bool,
    accept_hash_changes: bool,
) -> tuple[DownloadResult, bool]:
    url, configured_path = resolve_pdf_doc(pdf_doc)
    tmp_path: Path | None = None
    manifest_changed = False
    final_path = docs_dir / (configured_path or fallback_filename(url, ".pdf"))

    try:
        if configured_path and final_path.exists() and not force:
            ok, message = sniff_pdf(final_path, content_type=None)
            if not ok:
                return DownloadResult(url, final_path, "failed", 0, None, message), manifest_changed
            actual = sha256_file(final_path)
            ok, hash_message = verify_existing_hash(manifest, url, actual, final_path.name)
            if not ok:
                return DownloadResult(url, final_path, "failed", final_path.stat().st_size, actual, hash_message), manifest_changed
            if expected_digest(manifest_entry(manifest, url)) is None:
                manifest_changed = set_manifest_entry(manifest, url, pdf_manifest_entry(url, final_path, docs_dir))
            return DownloadResult(
                url=url,
                path=final_path,
                status="skipped",
                size=final_path.stat().st_size,
                sha256=actual,
                message=f"already exists and is a valid PDF; {hash_message}",
            ), manifest_changed

        request = Request(
            url,
            headers={
                "Accept": "application/pdf,application/octet-stream;q=0.9,*/*;q=0.1",
                "User-Agent": "osdev-doc-fetcher/1.0",
            },
        )
        with urlopen(request, timeout=60) as response:
            headers = response.headers
            if configured_path:
                filename = safe_filename(configured_path, ".pdf")
            else:
                filename = safe_filename(parse_content_disposition(headers) or fallback_filename(response.geturl(), ".pdf"), ".pdf")
            final_path = docs_dir / filename
            content_type = headers.get("Content-Type")

            if final_path.exists() and not force:
                ok, message = sniff_pdf(final_path, content_type=None)
                if ok:
                    actual = sha256_file(final_path)
                    ok_hash, hash_message = verify_existing_hash(manifest, url, actual, final_path.name)
                    if not ok_hash:
                        return DownloadResult(url, final_path, "failed", final_path.stat().st_size, actual, hash_message), manifest_changed
                    if expected_digest(manifest_entry(manifest, url)) is None:
                        manifest_changed = set_manifest_entry(manifest, url, pdf_manifest_entry(url, final_path, docs_dir))
                    return DownloadResult(
                        url=url,
                        path=final_path,
                        status="skipped",
                        size=final_path.stat().st_size,
                        sha256=actual,
                        message=f"already exists and is a valid PDF; {hash_message}",
                    ), manifest_changed

            docs_dir.mkdir(parents=True, exist_ok=True)
            with tempfile.NamedTemporaryFile(
                mode="wb",
                prefix=f".{filename}.",
                suffix=".part",
                dir=docs_dir,
                delete=False,
            ) as tmp:
                tmp_path = Path(tmp.name)
                while True:
                    chunk = response.read(CHUNK_SIZE)
                    if not chunk:
                        break
                    tmp.write(chunk)

        ok, message = sniff_pdf(tmp_path, content_type)
        if not ok:
            return DownloadResult(url, final_path, "failed", 0, None, message), manifest_changed

        actual = sha256_file(tmp_path)
        expected = expected_digest(manifest_entry(manifest, url))
        if expected and not prompt_hash_change(final_path.name, expected, actual, accept_hash_changes):
            return DownloadResult(url, final_path, "failed", tmp_path.stat().st_size, actual, "downloaded PDF differs from recorded hash"), manifest_changed

        shutil.move(str(tmp_path), final_path)
        tmp_path = None
        manifest_changed = set_manifest_entry(manifest, url, pdf_manifest_entry(url, final_path, docs_dir))
        return DownloadResult(
            url=url,
            path=final_path,
            status="downloaded",
            size=final_path.stat().st_size,
            sha256=actual,
            message=message if not expected or expected == actual else "accepted hash change; manifest updated",
        ), manifest_changed
    except HTTPError as exc:
        return DownloadResult(url, final_path, "failed", 0, None, f"HTTP {exc.code}: {exc.reason}"), manifest_changed
    except URLError as exc:
        return DownloadResult(url, final_path, "failed", 0, None, f"URL error: {exc.reason}"), manifest_changed
    except OSError as exc:
        return DownloadResult(url, final_path, "failed", 0, None, f"I/O error: {exc}"), manifest_changed
    finally:
        if tmp_path is not None:
            tmp_path.unlink(missing_ok=True)


def check_zip_member(info: zipfile.ZipInfo, destination: Path) -> Path:
    if info.filename.startswith(("/", "\\")):
        raise ValueError(f"absolute path in ZIP member: {info.filename}")

    parts = Path(info.filename).parts
    if not parts or any(part in ("", ".", "..") for part in parts):
        raise ValueError(f"unsafe path in ZIP member: {info.filename}")

    mode = info.external_attr >> 16
    if stat.S_IFMT(mode) == stat.S_IFLNK:
        raise ValueError(f"symlink in ZIP member: {info.filename}")

    target = (destination / info.filename).resolve()
    destination_resolved = destination.resolve()
    if os.path.commonpath([destination_resolved, target]) != str(destination_resolved):
        raise ValueError(f"ZIP member escapes destination: {info.filename}")

    return target


def extract_zip_safely(zip_path: Path, destination: Path) -> int:
    extracted_files = 0
    with zipfile.ZipFile(zip_path) as archive:
        for info in archive.infolist():
            target = check_zip_member(info, destination)
            if info.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue

            target.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(info) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output, length=CHUNK_SIZE)
            extracted_files += 1

    return extracted_files


def replace_tree(source: Path, destination: Path) -> None:
    if not destination.exists():
        shutil.move(str(source), destination)
        return

    backup_parent = Path(tempfile.mkdtemp(prefix=f".{destination.name}.backup.", dir=destination.parent))
    backup_path = backup_parent / destination.name
    shutil.move(str(destination), backup_path)
    try:
        shutil.move(str(source), destination)
    except Exception:
        if destination.exists():
            shutil.rmtree(destination)
        shutil.move(str(backup_path), destination)
        raise
    finally:
        shutil.rmtree(backup_parent, ignore_errors=True)


def count_files(path: Path) -> int:
    if not path.exists():
        return 0
    return sum(1 for item in path.rglob("*") if item.is_file())


def download_archive(
    archive_doc: dict[str, str],
    docs_dir: Path,
    manifest: dict[str, object],
    force: bool,
    accept_hash_changes: bool,
) -> tuple[DownloadResult, bool]:
    url = archive_doc["url"]
    marker = Path(archive_doc["marker"])
    archive_path = Path(archive_doc["path"])
    name = archive_doc["name"]
    marker_path = docs_dir / marker
    target_path = docs_dir / archive_path
    tmp_path: Path | None = None
    manifest_changed = False

    if marker_path.exists() and not force:
        actual, file_count = tree_sha256(target_path)
        ok, hash_message = verify_existing_hash(manifest, url, actual, name)
        if not ok:
            return DownloadResult(url, target_path, "failed", 0, actual, hash_message), manifest_changed
        if expected_digest(manifest_entry(manifest, url)) is None:
            manifest_changed = set_manifest_entry(manifest, url, archive_manifest_entry(url, target_path, docs_dir))
        return DownloadResult(
            url=url,
            path=target_path,
            status="skipped",
            size=0,
            sha256=actual,
            message=f"{name} already expanded ({file_count} files); {hash_message}",
        ), manifest_changed

    try:
        filename = safe_filename(fallback_filename(url, ".zip"), ".zip")
        tmp_path, headers, _ = download_to_temp(
            url,
            docs_dir,
            filename,
            "application/zip,application/octet-stream;q=0.9,*/*;q=0.1",
        )
        content_type = headers.get("Content-Type")

        ok, message = sniff_zip(tmp_path, content_type)
        if not ok:
            return DownloadResult(url, target_path, "failed", 0, None, message), manifest_changed

        zip_digest = sha256_file(tmp_path)
        with tempfile.TemporaryDirectory(prefix=f".{archive_path.name}.", dir=docs_dir) as temp_dir:
            temp_root = Path(temp_dir)
            extracted_files = extract_zip_safely(tmp_path, temp_root)
            temp_target = temp_root / archive_path
            temp_marker = temp_root / marker
            if not temp_marker.exists():
                return DownloadResult(url, target_path, "failed", 0, zip_digest, f"extracted ZIP but marker is missing: {marker}"), manifest_changed

            actual_tree_hash, _ = tree_sha256(temp_target)
            expected = expected_digest(manifest_entry(manifest, url))
            if expected and not prompt_hash_change(name, expected, actual_tree_hash, accept_hash_changes):
                return DownloadResult(url, target_path, "failed", tmp_path.stat().st_size, actual_tree_hash, "extracted archive differs from recorded tree hash"), manifest_changed

            replace_tree(temp_target, target_path)

        manifest_changed = set_manifest_entry(manifest, url, archive_manifest_entry(url, target_path, docs_dir))
        return DownloadResult(
            url=url,
            path=target_path,
            status="extracted",
            size=tmp_path.stat().st_size,
            sha256=zip_digest,
            message=message if not expected or expected == actual_tree_hash else f"accepted tree hash change; extracted {extracted_files} files; manifest updated",
        ), manifest_changed
    except HTTPError as exc:
        return DownloadResult(url, target_path, "failed", 0, None, f"HTTP {exc.code}: {exc.reason}"), manifest_changed
    except URLError as exc:
        return DownloadResult(url, target_path, "failed", 0, None, f"URL error: {exc.reason}"), manifest_changed
    except (OSError, zipfile.BadZipFile, ValueError) as exc:
        return DownloadResult(url, target_path, "failed", 0, None, f"archive error: {exc}"), manifest_changed
    finally:
        if tmp_path is not None:
            tmp_path.unlink(missing_ok=True)


def write_current_manifest(docs_dir: Path, manifest_path: Path) -> list[DownloadResult]:
    manifest: dict[str, object] = {"version": MANIFEST_VERSION, "artifacts": {}}
    results: list[DownloadResult] = []

    for pdf_doc in PDF_DOCS:
        url = pdf_doc["url"]
        path = docs_dir / pdf_doc["path"]
        if not path.exists():
            results.append(DownloadResult(url, path, "missing", 0, None, "file is not present"))
            continue
        ok, message = sniff_pdf(path, content_type=None)
        if not ok:
            results.append(DownloadResult(url, path, "failed", 0, None, message))
            continue
        entry = pdf_manifest_entry(url, path, docs_dir)
        set_manifest_entry(manifest, url, entry)
        results.append(DownloadResult(url, path, "recorded", path.stat().st_size, str(entry["sha256"]), "recorded current PDF hash"))

    for archive_doc in ARCHIVE_DOCS:
        url = archive_doc["url"]
        path = docs_dir / archive_doc["path"]
        marker = docs_dir / archive_doc["marker"]
        if not marker.exists():
            results.append(DownloadResult(url, path, "missing", 0, None, f"marker is not present: {archive_doc['marker']}"))
            continue
        entry = archive_manifest_entry(url, path, docs_dir)
        set_manifest_entry(manifest, url, entry)
        results.append(DownloadResult(url, path, "recorded", 0, str(entry["tree_sha256"]), f"recorded current tree hash ({entry['file_count']} files)"))

    if not any(result.status == "failed" for result in results):
        save_manifest(manifest_path, manifest)

    return results


def format_size(size: int) -> str:
    units = ["B", "KiB", "MiB", "GiB"]
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            return f"{value:.1f} {unit}" if unit != "B" else f"{size} B"
        value /= 1024
    return f"{size} B"


def parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--docs-dir", default="docs", type=Path, help="directory where docs are stored")
    parser.add_argument("--manifest", default=Path("docs/download_manifest.json"), type=Path, help="hash manifest path")
    parser.add_argument("--write-manifest", action="store_true", help="record hashes for the currently present built-in docs without downloading")
    parser.add_argument("--force", action="store_true", help="redownload existing PDFs and re-extract archives")
    parser.add_argument("--accept-hash-changes", action="store_true", help="accept changed hashes without prompting and update the manifest")
    parser.add_argument("urls", nargs="*", help="override the built-in PDF URL list")
    return parser.parse_args(list(argv))


def print_result(result: DownloadResult, repo_root: Path) -> None:
    digest = f" sha256={result.sha256[:16]}..." if result.sha256 else ""
    size = f" {format_size(result.size)}" if result.size else ""
    rel_path = os.path.relpath(result.path, repo_root)
    print(f"{result.status:10} {rel_path}{size}{digest} - {result.message}")


def main(argv: Iterable[str] = sys.argv[1:]) -> int:
    args = parse_args(argv)
    repo_root = Path(__file__).resolve().parent.parent
    docs_dir = args.docs_dir if args.docs_dir.is_absolute() else repo_root / args.docs_dir
    manifest_path = args.manifest if args.manifest.is_absolute() else repo_root / args.manifest

    if args.write_manifest:
        results = write_current_manifest(docs_dir, manifest_path)
        for result in results:
            print_result(result, repo_root)
        print(f"manifest  {os.path.relpath(manifest_path, repo_root)}")
        return 1 if any(result.status in {"failed", "missing"} for result in results) else 0

    try:
        manifest = load_manifest(manifest_path)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"failed to read manifest: {exc}", file=sys.stderr)
        return 1

    manifest_changed = False
    if args.urls:
        work_items: list[dict[str, str] | str] = list(args.urls)
        result_pairs = [download_pdf(item, docs_dir, manifest, args.force, args.accept_hash_changes) for item in work_items]
    else:
        result_pairs = [download_pdf(doc, docs_dir, manifest, args.force, args.accept_hash_changes) for doc in PDF_DOCS]
        result_pairs.extend(download_archive(doc, docs_dir, manifest, args.force, args.accept_hash_changes) for doc in ARCHIVE_DOCS)

    results: list[DownloadResult] = []
    for result, changed in result_pairs:
        results.append(result)
        manifest_changed = manifest_changed or changed

    for result in results:
        print_result(result, repo_root)

    if manifest_changed:
        save_manifest(manifest_path, manifest)
        print(f"manifest  {os.path.relpath(manifest_path, repo_root)} updated")

    return 1 if any(result.status == "failed" for result in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
