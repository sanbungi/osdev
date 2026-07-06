# osdev

## 動かす

開発環境: Ubuntu 24.04 + QEMU

依存関係のインストール
```
 sudo apt install -y \
    build-essential \
    binutils \
    nasm \
    python3 \
    python3-pip \
    qemu-system-x86 \
    qemu-system-gui \
    gdb-multiarch
```

本体
```
git clone https://github.com/sanbungi/osdev.git
cd osdev

make run
make run-headless (no gui)
```

テスト
```
python3 -m venv venv & venv/bin/python3 -m pip install -r requirements-dev.txt
make test
```

テストはカーネル内での自己テストと、pytestからqemuを実行して行うものを併用している。

## 構造

- boot.asm (1段目
- stage2_entry.asm (2段目
- kernel.c (2段目と同じ並びで動くCカーネル
- memo/ (メモを書く

## 資料

https://wiki.osdev.org/Bare_Bones

https://docs.amd.com/v/u/en-US/24593_3.44_APM_Vol2

https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html

## スクショ置き場

<img width="1513" height="589" alt="image" src="https://github.com/user-attachments/assets/4d10d058-48df-4019-bb6a-060444493a58" />
