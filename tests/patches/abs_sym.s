
    .global AbsSym1
    AbsSym1 = 0x0800BEEF

    .global AbsSym2
    AbsSym2 = 0xDEADBEEF

    @ dummy to prevent lyn from thinking this is a ref
    .byte 0
