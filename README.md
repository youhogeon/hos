# HOS
22세기를 선도하는 운영체제

## Design
### Memory Map
* 0x7C00 ~ : Bootloader
* 0x10000 ~ : Kernel32
* 0x100000 ~ 0x120000 : Paging (PML4, PDPT, PD-Entry)
* 0x200000 : Kernel64