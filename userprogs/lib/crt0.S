.code64
.global _start
.extern main
.extern exit

.section .text
_start:
    xor %rbp, %rbp

    pop %rdi        //Pop top of stack (argc) into RDI
    
    mov %rsp, %rsi  //The stack pointer now points to argv[0], so RSI = argv

    // Calculate envp (Environment variables)
    lea 8(%rsi, %rdi, 8), %rdx
    add $8, %rdx

    // Calling main(argc, argc, envp)
    call main

    //Return through exit
    mov %rax, %rdi
    call exit

1:
    hlt
    jmp 1b