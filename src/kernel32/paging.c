#include "paging.h"
#include "types.h"

void kInitPageTables(void) {
    // PML4 테이블 생성
    PML4TENTRY* pstPML4TEntry = (PML4TENTRY*)PAGE_PML4TADDRESS;

    kSetPageEntryData(&(pstPML4TEntry[0]), PAGE_PDPTADDRESS, PAGE_FLAGS_DEFAULT, 0);
    for (int i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
        kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0);
    }

    // PDPT(페이지 디렉터리 포인터 테이블) 생성
    PDPTENTRY* pstPDPTEntry = (PDPTENTRY*)PAGE_PDPTADDRESS;
    for (int i = 0; i < 64; i++) {
        kSetPageEntryData(&(pstPDPTEntry[i]), PAGE_PDADDRESS + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT, 0);
    }
    for (int i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0);
    }

    // PD(페이지 디렉터리) 생성
    PDENTRY* pstPDEntry = (PDENTRY*)PAGE_PDADDRESS;
    uint64 dwMappingAddress = 0;
    for (int i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++) {
        kSetPageEntryData(&(pstPDEntry[i]), dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

void kSetPageEntryData(PTENTRY* pstEntry, uint64 address, DWORD lowerFlags, DWORD upperFlags) {
    pstEntry->dwAttributeAndLowerBaseAddress = (DWORD)(address & 0xFFFFFFFF) | (lowerFlags & 0xFFFFFFFF);

    pstEntry->dwUpperBaseAddressAndEXB = (DWORD)((address >> 32) & 0xFF) | (upperFlags & 0xFFFFFFFF);
}