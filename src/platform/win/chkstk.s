# __chkstk: touch the stack pages of a large frame in order (rax = frame size), as the MSVC runtime does.
.intel_syntax noprefix
.text
.globl __chkstk
__chkstk:
    push rcx
    push rax
    cmp rax, 0x1000
    lea rcx, [rsp + 24]
    jb 2f
1:  sub rcx, 0x1000
    test [rcx], rcx
    sub rax, 0x1000
    cmp rax, 0x1000
    ja 1b
2:  sub rcx, rax
    test [rcx], rcx
    pop rax
    pop rcx
    ret
