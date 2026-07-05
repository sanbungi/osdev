# Documentation Index

This directory contains local offline references for operating-system development.
It is intended to be read by humans and LLM agents as a high-level map before
opening the full documents.

## How To Use This Directory

- Use the vendor PDFs for authoritative x86/x86-64 architectural behavior,
  instruction encodings, control registers, paging, interrupts, MSRs, and ABI
  rules.
- Use the OSDev Wiki mirror for implementation-oriented explanations,
  tutorials, common pitfalls, and lists of related topics. Treat it as a guide
  to investigate further, not as the final authority when it disagrees with a
  vendor manual.
- Use [download_manifest.json](download_manifest.json) to verify that local
  downloads have not silently changed. [../scripts/download_docs.py](../scripts/download_docs.py)
  validates file type and recorded hashes during future downloads.

## Downloaded PDFs

### [24593_3.44_APM_Vol2.pdf](24593_3.44_APM_Vol2.pdf)

AMD64 Architecture Programmer's Manual, Volume 2: System Programming.
This is the main AMD reference for AMD64 system-level behavior. Use it for
AMD-specific details around protected mode, long mode, paging, segmentation,
exceptions, interrupts, system instructions, control registers, descriptor
tables, task state, virtualization-related system state, and other privileged
CPU mechanisms.

Good queries:

- "How does AMD64 long mode enter or leave compatibility mode?"
- "What exactly is in a GDT, IDT, TSS, or page table entry on AMD64?"
- "Which control-register bits affect paging, protection, or exceptions?"

### [253665-092-sdm-vol-1.pdf](253665-092-sdm-vol-1.pdf)

Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 1:
Basic Architecture. Use this for the conceptual model of Intel x86/x86-64:
data types, registers, memory model, instruction execution environment,
application-level architecture, SIMD/AVX background, and general programming
model.

Good queries:

- "What registers and flags exist in 32-bit or 64-bit mode?"
- "What is the architectural meaning of EFLAGS/RFLAGS bits?"
- "What is the basic Intel memory and execution model?"

### [325383-092-sdm-vol-2abcd.pdf](325383-092-sdm-vol-2abcd.pdf)

Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 2:
Instruction Set Reference, A-Z. This is the authoritative instruction reference.
Use it for opcode encodings, operands, flags affected, exceptions, valid modes,
and instruction-specific edge cases.

Good queries:

- "What does `iretq`, `lgdt`, `lidt`, `mov cr3`, or `syscall` require?"
- "Which flags does an arithmetic or bit operation modify?"
- "What exceptions can a privileged instruction raise?"

### [325384-092-sdm-vol-3abcd.pdf](325384-092-sdm-vol-3abcd.pdf)

Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 3:
System Programming Guide. This is the primary Intel reference for kernel work:
protected mode, long mode, paging, exceptions, interrupts, APIC, memory
ordering, task management, system calls, VMX, debug facilities, and many
privileged features.

Good queries:

- "How do I set up protected mode, long mode, paging, GDT, IDT, or TSS?"
- "What is the exact page-fault error code layout?"
- "How do APIC, x2APIC, exceptions, and interrupt gates work?"

### [335592-092-sdm-vol-4.pdf](335592-092-sdm-vol-4.pdf)

Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 4:
Model-Specific Registers. Use this when code reads or writes MSRs via `rdmsr`
or `wrmsr`, or when investigating feature-specific registers such as APIC,
EFER, PAT, MTRR, syscall/sysret state, performance counters, or CPU family
specific behavior.

Good queries:

- "Which MSR controls syscall/sysret entry points?"
- "What are the bit fields of EFER, APIC base, PAT, or MTRR MSRs?"
- "Is this MSR architectural or model-specific?"

### [specsbbs101.pdf](specsbbs101.pdf)

BIOS Boot Specification Version 1.01. This older PC BIOS specification covers
BIOS boot ordering, IPL devices, BCV priority, Plug and Play expansion headers,
legacy boot devices, INT 13h boot controller ordering, and related POST boot
selection behavior.

Good queries:

- "How did legacy BIOS define boot-device priority?"
- "What are IPL devices, BAIDs, and BCVs?"
- "How should BIOS option ROM boot ordering be interpreted?"

## OSDev Wiki Offline Mirror

[wiki.osdev.org/](wiki.osdev.org/) is an offline HTML mirror of the OSDev Wiki.
The main entry points are [wiki.osdev.org/index.html](wiki.osdev.org/index.html),
[wiki.osdev.org/Main_Page](wiki.osdev.org/Main_Page), and
[wiki.osdev.org/Categorized_Main_Page](wiki.osdev.org/Categorized_Main_Page).
The mirror currently contains thousands of HTML pages plus images and thumbnails.

The page files are named after wiki titles. For example, the online page
`https://wiki.osdev.org/Global_Descriptor_Table` is stored locally as
[wiki.osdev.org/Global_Descriptor_Table](wiki.osdev.org/Global_Descriptor_Table).

### Getting Started And Toolchain

Use these pages for early project setup and common OSDev conventions:

- [wiki.osdev.org/Getting_Started](wiki.osdev.org/Getting_Started): overall project guidance.
- [wiki.osdev.org/Required_Knowledge](wiki.osdev.org/Required_Knowledge): prerequisite topics.
- [wiki.osdev.org/Beginner_Mistakes](wiki.osdev.org/Beginner_Mistakes): common design and implementation mistakes.
- [wiki.osdev.org/Bare_Bones](wiki.osdev.org/Bare_Bones): minimal kernel tutorial.
- [wiki.osdev.org/GCC_Cross-Compiler](wiki.osdev.org/GCC_Cross-Compiler): cross-compiler setup.
- [wiki.osdev.org/Linker_Scripts](wiki.osdev.org/Linker_Scripts): linker layout for kernels.
- [wiki.osdev.org/Makefile](wiki.osdev.org/Makefile): build-system patterns.
- [wiki.osdev.org/Meaty_Skeleton](wiki.osdev.org/Meaty_Skeleton): larger starter structure.

### Boot And Firmware

Use these pages for BIOS/UEFI boot flow, bootloaders, real mode, and transition
to protected or long mode:

- [wiki.osdev.org/Bootloader](wiki.osdev.org/Bootloader)
- [wiki.osdev.org/Boot_Sequence](wiki.osdev.org/Boot_Sequence)
- [wiki.osdev.org/BIOS](wiki.osdev.org/BIOS)
- [wiki.osdev.org/UEFI](wiki.osdev.org/UEFI)
- [wiki.osdev.org/UEFI_Bare_Bones](wiki.osdev.org/UEFI_Bare_Bones)
- [wiki.osdev.org/Real_Mode](wiki.osdev.org/Real_Mode)
- [wiki.osdev.org/Protected_Mode](wiki.osdev.org/Protected_Mode)
- [wiki.osdev.org/Long_Mode](wiki.osdev.org/Long_Mode)
- [wiki.osdev.org/Entering_Long_Mode_Directly](wiki.osdev.org/Entering_Long_Mode_Directly)
- [wiki.osdev.org/A20_Line](wiki.osdev.org/A20_Line)
- [wiki.osdev.org/Multiboot](wiki.osdev.org/Multiboot)
- [wiki.osdev.org/Limine_Bare_Bones](wiki.osdev.org/Limine_Bare_Bones)
- [wiki.osdev.org/GRUB](wiki.osdev.org/GRUB)

### x86 And x86-64 CPU State

Use these pages when implementing CPU mode setup, descriptor tables, privilege
levels, and low-level assembly glue:

- [wiki.osdev.org/X86](wiki.osdev.org/X86)
- [wiki.osdev.org/X86-64](wiki.osdev.org/X86-64)
- [wiki.osdev.org/AMD64](wiki.osdev.org/AMD64)
- [wiki.osdev.org/IA32_Architecture_Family](wiki.osdev.org/IA32_Architecture_Family)
- [wiki.osdev.org/CPUID](wiki.osdev.org/CPUID)
- [wiki.osdev.org/CPU_Registers_x86](wiki.osdev.org/CPU_Registers_x86)
- [wiki.osdev.org/CPU_Registers_x86-64](wiki.osdev.org/CPU_Registers_x86-64)
- [wiki.osdev.org/Global_Descriptor_Table](wiki.osdev.org/Global_Descriptor_Table)
- [wiki.osdev.org/GDT_Tutorial](wiki.osdev.org/GDT_Tutorial)
- [wiki.osdev.org/Interrupt_Descriptor_Table](wiki.osdev.org/Interrupt_Descriptor_Table)
- [wiki.osdev.org/Task_State_Segment](wiki.osdev.org/Task_State_Segment)
- [wiki.osdev.org/Calling_Conventions](wiki.osdev.org/Calling_Conventions)
- [wiki.osdev.org/System_V_ABI](wiki.osdev.org/System_V_ABI)

### Interrupts, Exceptions, Timers, And Syscalls

Use these pages for interrupt setup, interrupt controllers, timer sources, and
user/kernel transition design:

- [wiki.osdev.org/Interrupts](wiki.osdev.org/Interrupts)
- [wiki.osdev.org/Interrupt_Service_Routines](wiki.osdev.org/Interrupt_Service_Routines)
- [wiki.osdev.org/Exceptions](wiki.osdev.org/Exceptions)
- [wiki.osdev.org/Double_Fault](wiki.osdev.org/Double_Fault)
- [wiki.osdev.org/8259_PIC](wiki.osdev.org/8259_PIC)
- [wiki.osdev.org/APIC](wiki.osdev.org/APIC)
- [wiki.osdev.org/IOAPIC](wiki.osdev.org/IOAPIC)
- [wiki.osdev.org/Message_Signaled_Interrupts](wiki.osdev.org/Message_Signaled_Interrupts)
- [wiki.osdev.org/PIT](wiki.osdev.org/PIT)
- [wiki.osdev.org/HPET](wiki.osdev.org/HPET)
- [wiki.osdev.org/APIC_Timer](wiki.osdev.org/APIC_Timer)
- [wiki.osdev.org/System_Calls](wiki.osdev.org/System_Calls)
- [wiki.osdev.org/SYSENTER](wiki.osdev.org/SYSENTER)

### Memory Management

Use these pages for physical memory discovery, placement, paging, heap and
virtual memory design:

- [wiki.osdev.org/Memory_Map_(x86)](<wiki.osdev.org/Memory_Map_(x86)>)
- [wiki.osdev.org/Detecting_Memory_(x86)](<wiki.osdev.org/Detecting_Memory_(x86)>)
- [wiki.osdev.org/Memory_Allocation](wiki.osdev.org/Memory_Allocation)
- [wiki.osdev.org/Page_Frame_Allocation](wiki.osdev.org/Page_Frame_Allocation)
- [wiki.osdev.org/Paging](wiki.osdev.org/Paging)
- [wiki.osdev.org/Setting_Up_Paging](wiki.osdev.org/Setting_Up_Paging)
- [wiki.osdev.org/Identity_Paging](wiki.osdev.org/Identity_Paging)
- [wiki.osdev.org/Higher_Half_Kernel](wiki.osdev.org/Higher_Half_Kernel)
- [wiki.osdev.org/Category:Virtual_Memory](wiki.osdev.org/Category:Virtual_Memory)
- [wiki.osdev.org/Memory_Management_Unit](wiki.osdev.org/Memory_Management_Unit)
- [wiki.osdev.org/Heap](wiki.osdev.org/Heap)
- [wiki.osdev.org/Brendan's_Memory_Management_Guide](wiki.osdev.org/Brendan's_Memory_Management_Guide)

### Device Discovery, Buses, And Firmware Tables

Use these pages to discover hardware and parse firmware-provided tables:

- [wiki.osdev.org/PCI](wiki.osdev.org/PCI)
- [wiki.osdev.org/PCI_Express](wiki.osdev.org/PCI_Express)
- [wiki.osdev.org/ACPI](wiki.osdev.org/ACPI)
- [wiki.osdev.org/AML](wiki.osdev.org/AML)
- [wiki.osdev.org/DSDT](wiki.osdev.org/DSDT)
- [wiki.osdev.org/SMBIOS](wiki.osdev.org/SMBIOS)
- [wiki.osdev.org/CMOS](wiki.osdev.org/CMOS)
- [wiki.osdev.org/Serial_Ports](wiki.osdev.org/Serial_Ports)
- [wiki.osdev.org/PS/2_Keyboard](wiki.osdev.org/PS/2_Keyboard)
- [wiki.osdev.org/PS/2_Mouse](wiki.osdev.org/PS/2_Mouse)
- [wiki.osdev.org/USB](wiki.osdev.org/USB)

### Storage, Filesystems, And Disk Images

Use these pages for block devices, boot media, disk partitioning, and filesystem
implementations:

- [wiki.osdev.org/ATA](wiki.osdev.org/ATA)
- [wiki.osdev.org/ATA_PIO_Mode](wiki.osdev.org/ATA_PIO_Mode)
- [wiki.osdev.org/ATAPI](wiki.osdev.org/ATAPI)
- [wiki.osdev.org/AHCI](wiki.osdev.org/AHCI)
- [wiki.osdev.org/NVMe](wiki.osdev.org/NVMe)
- [wiki.osdev.org/Floppy_Drive_Controller](wiki.osdev.org/Floppy_Drive_Controller)
- [wiki.osdev.org/Partition_Table](wiki.osdev.org/Partition_Table)
- [wiki.osdev.org/GPT](wiki.osdev.org/GPT)
- [wiki.osdev.org/FAT](wiki.osdev.org/FAT)
- [wiki.osdev.org/FAT32](wiki.osdev.org/FAT32)
- [wiki.osdev.org/Ext2](wiki.osdev.org/Ext2)
- [wiki.osdev.org/ISO_9660](wiki.osdev.org/ISO_9660)
- [wiki.osdev.org/USTAR](wiki.osdev.org/USTAR)
- [wiki.osdev.org/Disk_Images](wiki.osdev.org/Disk_Images)

### Executable Formats, Linking, And Runtime

Use these pages for kernel binary formats, loader contracts, C/C++ runtime
support, and ABI boundaries:

- [wiki.osdev.org/ELF](wiki.osdev.org/ELF)
- [wiki.osdev.org/PE](wiki.osdev.org/PE)
- [wiki.osdev.org/A.out](wiki.osdev.org/A.out)
- [wiki.osdev.org/COFF](wiki.osdev.org/COFF)
- [wiki.osdev.org/Linker_Scripts](wiki.osdev.org/Linker_Scripts)
- [wiki.osdev.org/C](wiki.osdev.org/C)
- [wiki.osdev.org/C_Library](wiki.osdev.org/C_Library)
- [wiki.osdev.org/Libgcc](wiki.osdev.org/Libgcc)
- [wiki.osdev.org/Calling_Global_Constructors](wiki.osdev.org/Calling_Global_Constructors)
- [wiki.osdev.org/C++_Bare_Bones](wiki.osdev.org/C++_Bare_Bones)
- [wiki.osdev.org/C++_Exception_Support](wiki.osdev.org/C++_Exception_Support)

### Scheduling, Processes, And Kernel Design

Use these pages for high-level kernel architecture and core OS services:

- [wiki.osdev.org/Kernel](wiki.osdev.org/Kernel)
- [wiki.osdev.org/Monolithic_Kernel](wiki.osdev.org/Monolithic_Kernel)
- [wiki.osdev.org/Microkernel](wiki.osdev.org/Microkernel)
- [wiki.osdev.org/Scheduling_Algorithms](wiki.osdev.org/Scheduling_Algorithms)
- [wiki.osdev.org/Context_Switching](wiki.osdev.org/Context_Switching)
- [wiki.osdev.org/Processes_and_Threads](wiki.osdev.org/Processes_and_Threads)
- [wiki.osdev.org/Multitasking_Systems](wiki.osdev.org/Multitasking_Systems)
- [wiki.osdev.org/Synchronization_Primitives](wiki.osdev.org/Synchronization_Primitives)
- [wiki.osdev.org/Category:IPC](wiki.osdev.org/Category:IPC)
- [wiki.osdev.org/VFS](wiki.osdev.org/VFS)

### Debugging And Emulation

Use these pages while building and debugging the local kernel:

- [wiki.osdev.org/QEMU](wiki.osdev.org/QEMU)
- [wiki.osdev.org/Bochs](wiki.osdev.org/Bochs)
- [wiki.osdev.org/GDB](wiki.osdev.org/GDB)
- [wiki.osdev.org/Kernel_Debugging](wiki.osdev.org/Kernel_Debugging)
- [wiki.osdev.org/How_Do_I_Use_A_Debugger_With_My_OS](wiki.osdev.org/How_Do_I_Use_A_Debugger_With_My_OS)
- [wiki.osdev.org/Serial_Ports](wiki.osdev.org/Serial_Ports)
- [wiki.osdev.org/Stack_Trace](wiki.osdev.org/Stack_Trace)
- [wiki.osdev.org/Triple_Fault](wiki.osdev.org/Triple_Fault)

### ARM And Other Architectures

Most local project code appears x86-focused, but the mirror also includes ARM
and other architecture material:

- [wiki.osdev.org/ARM](wiki.osdev.org/ARM)
- [wiki.osdev.org/AArch32](wiki.osdev.org/AArch32)
- [wiki.osdev.org/AArch64](wiki.osdev.org/AArch64)
- [wiki.osdev.org/ARM_Paging](wiki.osdev.org/ARM_Paging)
- [wiki.osdev.org/ARM_Generic_Timer](wiki.osdev.org/ARM_Generic_Timer)
- [wiki.osdev.org/RISC-V](wiki.osdev.org/RISC-V)
- [wiki.osdev.org/PowerPC](wiki.osdev.org/PowerPC)

## Suggested Lookup Strategy For LLM Agents

1. Start with this index to pick a likely source.
2. For implementation shape, open the relevant OSDev Wiki page and nearby
   linked pages.
3. For exact CPU behavior, confirm details in the AMD or Intel PDF. Prefer
   Intel Volume 3 / AMD Volume 2 for privileged behavior, Intel Volume 2 for
   instruction semantics, and Intel Volume 4 for MSRs.
4. When a wiki page says "see Intel manual" or discusses disputed architecture
   behavior, treat the vendor PDF as the tie-breaker.
5. When changing local downloads, run `python3 scripts/download_docs.py`; hash
   mismatches are intentionally interactive unless `--accept-hash-changes` is
   provided.
