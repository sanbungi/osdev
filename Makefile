BOOT=boot.bin
STAGE2=stage2.bin
IMAGE=os.img
STAGE2_SECTORS=32

all: $(IMAGE)

stage2_entry.o: stage2_entry.asm
	nasm -f elf32 stage2_entry.asm -o stage2_entry.o

keyboard_irq.o: keyboard_irq.asm
	nasm -f elf32 keyboard_irq.asm -o keyboard_irq.o

kernel.o: kernel.c
	gcc -m32 \
	  -ffreestanding \
	  -fno-pic \
	  -fno-stack-protector \
	  -nostdlib \
	  -nostdinc \
	  -fno-builtin \
	  -c kernel.c -o kernel.o

debug: $(IMAGE)
	qemu-system-i386 \
		-accel tcg \
		-drive file=$(IMAGE),format=raw \
		-display gtk \
		-serial stdio \
		-s -S \
		-no-reboot \
		-no-shutdown

stage2.elf: stage2_entry.o kernel.o keyboard_irq.o linker.ld
	ld -m elf_i386 -T linker.ld -nostdlib -o stage2.elf stage2_entry.o kernel.o keyboard_irq.o

$(STAGE2): stage2.elf
	objcopy -O binary stage2.elf $(STAGE2)
	python3 -c 'import os,sys; size=os.path.getsize("$(STAGE2)"); limit=$(STAGE2_SECTORS)*512; print("stage2 size:", size, "bytes"); sys.exit(0 if size <= limit else 1)'

$(BOOT): boot.asm $(STAGE2)
	nasm -f bin -DSTAGE2_SECTORS=$(STAGE2_SECTORS) boot.asm -o $(BOOT)

$(IMAGE): $(BOOT) $(STAGE2)
	dd if=/dev/zero of=$(IMAGE) bs=512 count=$$(($(STAGE2_SECTORS)+1))
	dd if=$(BOOT) of=$(IMAGE) bs=512 count=1 conv=notrunc
	dd if=$(STAGE2) of=$(IMAGE) bs=512 seek=1 conv=notrunc

run: $(IMAGE)
	qemu-system-x86_64 \
	  -drive file=$(IMAGE),format=raw \
	  -display gtk \
	  -serial stdio

run-headless: $(IMAGE)
	qemu-system-x86_64 \
	  -drive file=$(IMAGE),format=raw \
	  -display none \
	  -serial stdio

run-tcg: $(IMAGE)
	qemu-system-x86_64 \
	  -accel tcg \
	  -drive file=$(IMAGE),format=raw \
	  -display gtk \
	  -serial stdio

clean:
	rm -f *.o *.elf *.bin $(IMAGE)
