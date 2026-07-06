BOOT=boot.bin
STAGE2=stage2.bin
IMAGE=os.img
STAGE2_SECTORS=32
TEST_NAME?=divide_by_zero
QEMU_TEST_TIMEOUT=5
TEST_BOOT=boot-test-$(TEST_NAME).bin
TEST_STAGE2=stage2-test-$(TEST_NAME).bin
TEST_IMAGE=os-test-$(TEST_NAME).img
TEST_RUNNER=tests/runner.$(TEST_NAME).o

.PHONY: all debug run run-headless run-tcg test qemu-test clean

all: $(IMAGE)

stage2_entry.o: stage2_entry.asm
	nasm -f elf32 stage2_entry.asm -o stage2_entry.o

keyboard_irq.o: keyboard_irq.asm
	nasm -f elf32 keyboard_irq.asm -o keyboard_irq.o

exception.o: exception.asm
	nasm -f elf32 exception.asm -o exception.o

kernel.o: kernel.c lib.h
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -c kernel.c -o kernel.o

lib.o: lib.c lib.h
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -c lib.c -o lib.o

kernel.test.o: kernel.c lib.h
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -DKERNEL_TEST \
	  -c kernel.c -o kernel.test.o

$(TEST_RUNNER): tests/runner.c lib.h
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -DKERNEL_TEST \
	  -DKTEST_NAME=\"$(TEST_NAME)\" \
	  -c tests/runner.c -o $(TEST_RUNNER)

tests/kernel-test.o: tests/kernel-test.c lib.h
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -DKERNEL_TEST \
	  -c tests/kernel-test.c -o tests/kernel-test.o

debug: $(IMAGE)
	qemu-system-i386 \
		-accel tcg \
		-drive file=$(IMAGE),format=raw \
		-display gtk \
		-serial stdio \
		-s -S \
		-no-reboot \
		-no-shutdown

stage2.elf: stage2_entry.o kernel.o lib.o keyboard_irq.o exception.o linker.ld
	ld -m elf_i386 -T linker.ld -nostdlib -o stage2.elf stage2_entry.o kernel.o lib.o keyboard_irq.o exception.o

stage2-test-$(TEST_NAME).elf: stage2_entry.o kernel.test.o lib.o keyboard_irq.o exception.o $(TEST_RUNNER) tests/kernel-test.o linker.ld
	ld -m elf_i386 -T linker.ld -nostdlib -o stage2-test-$(TEST_NAME).elf stage2_entry.o kernel.test.o lib.o keyboard_irq.o exception.o $(TEST_RUNNER) tests/kernel-test.o

$(STAGE2): stage2.elf
	objcopy -O binary stage2.elf $(STAGE2)
	python3 -c 'import os,sys; size=os.path.getsize("$(STAGE2)"); limit=$(STAGE2_SECTORS)*512; print("stage2 size:", size, "bytes"); sys.exit(0 if size <= limit else 1)'

$(TEST_STAGE2): stage2-test-$(TEST_NAME).elf
	objcopy -O binary stage2-test-$(TEST_NAME).elf $(TEST_STAGE2)
	python3 -c 'import os,sys; size=os.path.getsize("$(TEST_STAGE2)"); limit=$(STAGE2_SECTORS)*512; print("test stage2 size:", size, "bytes"); sys.exit(0 if size <= limit else 1)'

$(BOOT): boot.asm $(STAGE2)
	nasm -f bin -DSTAGE2_SECTORS=$(STAGE2_SECTORS) boot.asm -o $(BOOT)

$(TEST_BOOT): boot.asm $(TEST_STAGE2)
	nasm -f bin -DSTAGE2_SECTORS=$(STAGE2_SECTORS) boot.asm -o $(TEST_BOOT)

$(IMAGE): $(BOOT) $(STAGE2)
	dd if=/dev/zero of=$(IMAGE) bs=512 count=$$(($(STAGE2_SECTORS)+1))
	dd if=$(BOOT) of=$(IMAGE) bs=512 count=1 conv=notrunc
	dd if=$(STAGE2) of=$(IMAGE) bs=512 seek=1 conv=notrunc

$(TEST_IMAGE): $(TEST_BOOT) $(TEST_STAGE2)
	dd if=/dev/zero of=$(TEST_IMAGE) bs=512 count=$$(($(STAGE2_SECTORS)+1))
	dd if=$(TEST_BOOT) of=$(TEST_IMAGE) bs=512 count=1 conv=notrunc
	dd if=$(TEST_STAGE2) of=$(TEST_IMAGE) bs=512 seek=1 conv=notrunc

run: $(IMAGE)
	qemu-system-x86_64 \
	  -drive file=$(IMAGE),format=raw \
	  -display gtk \
	  -serial stdio \
	  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	  -no-reboot \
	  -no-shutdown

run-headless: $(IMAGE)
	qemu-system-x86_64 \
	  -drive file=$(IMAGE),format=raw \
	  -display none \
	  -serial stdio \
	  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	  -no-reboot \
	  -no-shutdown

test:
	venv/bin/python3 -m pytest -q

qemu-test: $(TEST_IMAGE)
	@set +e; \
	timeout $(QEMU_TEST_TIMEOUT)s qemu-system-x86_64 \
	  -accel tcg \
	  -drive file=$(TEST_IMAGE),format=raw \
	  -display none \
	  -serial stdio \
	  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	  -no-reboot \
	  -no-shutdown; \
	status=$$?; \
	echo "QEMU_EXIT_STATUS=$$status"; \
	if [ $$status -eq 33 ]; then \
	  exit 0; \
	fi; \
	echo "qemu test failed: exit $$status"; \
	exit $$status

run-tcg: $(IMAGE)
	qemu-system-x86_64 \
	  -accel tcg \
	  -drive file=$(IMAGE),format=raw \
	  -display gtk \
	  -serial stdio

clean:
	rm -f *.o *.elf *.bin tests/*.o $(IMAGE) os-test-*.img
