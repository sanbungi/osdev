import re
import subprocess

import pytest


QEMU_EXIT_SUCCESS = 33  # isa-debug-exit: (0x10 << 1) | 1
QEMU_EXIT_FAILURE = 35  # isa-debug-exit: (0x11 << 1) | 1
QEMU_EXIT_TIMEOUT = 124


def run_qemu_kernel_test(name):
    try:
        return subprocess.run(
            ["make", "qemu-test", f"TEST_NAME={name}"],
            check=False,
            capture_output=True,
            text=True,
            timeout=10,
        )
    except subprocess.TimeoutExpired as exc:
        output = (exc.stdout or "") + (exc.stderr or "")
        raise AssertionError(
            f"qemu kernel test timed out: {name}\n\n{output}"
        ) from exc


def qemu_exit_status(output):
    match = re.search(r"^QEMU_EXIT_STATUS=(\d+)$", output, re.MULTILINE)
    assert match is not None, "QEMU_EXIT_STATUS was not reported\n\n" + output
    return int(match.group(1))


def assert_ktest_failed(name, expected_status=None, expected_output=None):
    result = run_qemu_kernel_test(name)
    output = result.stdout + result.stderr
    qemu_status = qemu_exit_status(output)

    if expected_status is not None:
        assert qemu_status == expected_status, (
            f"unexpected qemu exit status for failing test {name}: "
            f"{qemu_status}\n\n{output}"
        )
    else:
        assert qemu_status != QEMU_EXIT_SUCCESS, (
            f"kernel test unexpectedly passed: {name}\n\n{output}"
        )
    assert result.returncode != 0, (
        f"qemu command unexpectedly succeeded for failing test: {name}\n\n"
        f"{output}"
    )
    if expected_output is not None:
        assert expected_output in output, output

    pytest.xfail(f"kernel test failed as expected: {name}")


def assert_ktest_passed(name, expected_pass):
    result = run_qemu_kernel_test(name)
    output = result.stdout + result.stderr
    qemu_status = qemu_exit_status(output)

    if "KTEST event=fail" in output:
        assert qemu_status == QEMU_EXIT_FAILURE, (
            f"kernel reported KTEST fail but qemu did not use "
            f"qemu_exit_failure(): {qemu_status}\n\n{output}"
        )
        raise AssertionError(f"kernel reported KTEST fail: {name}\n\n{output}")

    if qemu_status == QEMU_EXIT_TIMEOUT:
        raise AssertionError(f"qemu kernel test timed out: {name}\n\n{output}")

    assert qemu_status == QEMU_EXIT_SUCCESS, (
        f"unexpected qemu exit status for {name}: {qemu_status}\n\n{output}"
    )
    assert result.returncode == 0, (
        f"qemu kernel test failed: {name}, exit={result.returncode}\n\n"
        f"{output}"
    )
    assert "KTEST event=fail" not in output, output
    assert expected_pass in output, output


def test_divide_by_zero_passes():
    assert_ktest_passed(
        "divide_by_zero",
        "KTEST event=pass name=divide_by_zero vector=0x00",
    )


def test_debug_exception_passes():
    assert_ktest_failed("debug_exception", expected_status=QEMU_EXIT_TIMEOUT)


def test_memory_write_passes():
    assert_ktest_passed(
        "memory_write",
        "KTEST event=pass name=memory_write",
    )
