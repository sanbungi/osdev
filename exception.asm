bits 32

  global exception6_stub
  extern exception6_handler

  exception6_stub:
      pusha
      cld

      push esp
      call exception6_handler

  .halt:
      cli
      hlt
      jmp .halt

  section .note.GNU-stack noalloc noexec nowrite progbits
