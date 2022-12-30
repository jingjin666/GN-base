.section .vm,"aw"
.globl vm_start, vm_end
vm_start:
.incbin "vm.bin"
vm_end: