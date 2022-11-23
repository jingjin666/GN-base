.section .user,"aw"
.globl user_start, user_end
user_start:
.incbin "app.elf"
user_end: