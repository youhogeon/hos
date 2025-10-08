#ifndef __PAGING_H__
#define __PAGING_H__

#include "types.h"

#define PAGE_FLAGS_P 0x00000001
#define PAGE_FLAGS_RW 0x00000002
#define PAGE_FLAGS_US 0x00000004
#define PAGE_FLAGS_PWT 0x00000008
#define PAGE_FLAGS_PCD 0x00000010
#define PAGE_FLAGS_A 0x00000020
#define PAGE_FLAGS_D 0x00000040
#define PAGE_FLAGS_PS 0x00000080
#define PAGE_FLAGS_G 0x00000100
#define PAGE_FLAGS_PAT 0x00001000
#define PAGE_FLAGS_EXB 0x80000000
#define PAGE_FLAGS_DEFAULT (PAGE_FLAGS_P | PAGE_FLAGS_RW)

#define PAGE_PML4TADDRESS 0x100000
#define PAGE_MAXENTRYCOUNT 512
#define PAGE_TABLESIZE (PAGE_MAXENTRYCOUNT * 8)
#define PAGE_PDPTADDRESS (PAGE_PML4TADDRESS + PAGE_TABLESIZE) // 0x101000
#define PAGE_PDADDRESS (PAGE_PDPTADDRESS + PAGE_TABLESIZE)    // 0x102000
#define PAGE_DEFAULTSIZE 0x200000                             // 2MB

#pragma pack(push, 1)

typedef struct pageTableEntryStruct {
    /**
     * PML4TENTRY / PDPTENTRY
     * 0: Present
     * 1: Read/Write
     * 2: User/Supervisor
     * 3: Page-Level Write-Through
     * 4: Page-Level Cache Disable
     * 5: Accessed
     * 6-8: Reserved
     * 9-11: Available to Software
     * 12-39:
     *     PML4TENTRY: Physical Address of PDPT (Page Directory Pointer Table)
     * (shift right 12) PDPTENTRY: Physical Address of PD (Page Directory)
     * (shift right 12) 40-47: Reserved 48-62: Available to Software 63: EXB
     *
     *
     * PDENTRY / PTENTRY
     * 0: Present
     * 1: Read/Write
     * 2: User/Supervisor
     * 3: Page-Level Write-Through
     * 4: Page-Level Cache Disable
     * 5: Accessed
     * 6: Dirty
     * 7: Page Size (0: 4KB, 1: 2MB (PDENTRY only))
     * 8: Global
     * 9-11: Available to Software
     * 12-39:
     *     PDENTRY: Physical Address of PT (Page Table) (shift right 12)
     *     PTENTRY: Physical Address of 4KB Page (shift right 12)
     * 40-47: Reserved
     * 48-62: Available to Software
     * 63: EXB
     */
    DWORD dwAttributeAndLowerBaseAddress;
    DWORD dwUpperBaseAddressAndEXB;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack(pop)

void kInitPageTables(void);
void kSetPageEntryData(PTENTRY* pstEntry, uint64 address, DWORD lowerFlags, DWORD upperFlags);

#endif /*__PAGING_H__*/