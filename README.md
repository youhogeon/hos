# HOS
22세기를 선도하는 운영체제

## Design
### Image Map
* ~ 0x1FF : Bootloader
* ~ 0x11FF: Kernel32
* 0x1200 ~ : Kernel64

### Memory Map
* 0x7C00 ~ : Bootloader
* ~ 0xFFFF : Bootloader Stack, Kernel32 Stack
* 0x10000 ~ 0x10FFF : Kernel32
* 0x11000 ~ 0xFFFFF : Kernel64 (will be copied to 0x200000)
* 0x100000 ~ 0x11FFFF : Pages (PML4, PDPT, PD-Entry)
* 0x200000 ~ 0x5FFFFF : Kernel64
* 0x600000 ~ 0x6FFFFF : Kernel64 Stack