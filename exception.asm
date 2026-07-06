bits 32

  global exception0_stub
  global exception6_stub

  extern exception0_handler
  extern exception6_handler

  exception0_stub:
      pusha
      cld

      push esp
      call exception0_handler
      jmp exception_halt

  exception6_stub:
      pusha
      cld

      push esp
      call exception6_handler
      jmp exception_halt

  exception_halt:
      cli
      hlt
      jmp exception_halt

  section .note.GNU-stack noalloc noexec nowrite progbits
