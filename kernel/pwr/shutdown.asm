


shutdown:
    cli                     ; Disable interrupts
    hlt                     ; Halt the CPU
    jmp shutdown            ; Infinite loop to ensure shutdown
    ; NOT IMPLEMENTED YET