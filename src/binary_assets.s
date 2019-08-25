.section .rodata

.global G_BOOK
.global G_BOOK12
.global G_BOOK12_END
.align 4
G_BOOK:   .incbin "book.dat"
G_BOOK12: .incbin "book12.dat"
G_BOOK12_END: 
