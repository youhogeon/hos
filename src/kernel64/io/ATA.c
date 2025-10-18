#include "ATA.h"
#include "../util/assembly.h"
#include "../util/sync.h"
#include "../util/timer.h"

static ATAMANAGER gs_stATAManager; // primary master ATA

/**
 * 하드 디스크의 상태를 반환
 */
static BYTE kReadATAStatus(BOOL bPrimary) {
    if (bPrimary == TRUE) {
        return inb(ATA_PORT_PRIMARYBASE + ATA_PORT_INDEX_STATUS);
    }

    return inb(ATA_PORT_SECONDARYBASE + ATA_PORT_INDEX_STATUS);
}

/**
 * 하드 디스크의 Busy가 해제될 때까지 일정 시간 동안 대기
 */
static BOOL kWaitForATANoBusy(BOOL bPrimary) {
    QWORD qwStartTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크의 Busy가 해제될 때까지 대기
    while ((kGetTickCount() - qwStartTickCount) <= ATA_WAITTIME) {
        BYTE bStatus = kReadATAStatus(bPrimary);

        // Busy 비트(비트 7)이 설정되어 있지 않으면 Busy가 해제된 것이므로 종료
        if ((bStatus & ATA_STATUS_BUSY) != ATA_STATUS_BUSY) {
            return TRUE;
        }

        kSleep(1);
    }

    return FALSE;
}

/**
 * 하드 디스크가 Ready될 때까지 일정 시간 동안 대기
 */
static BOOL kWaitForATAReady(BOOL bPrimary) {
    QWORD qwStartTickCount = kGetTickCount();

    // 일정 시간 동안 하드 디스크가 Ready가 될 때까지 대기
    while ((kGetTickCount() - qwStartTickCount) <= ATA_WAITTIME) {
        BYTE bStatus = kReadATAStatus(bPrimary);

        // Ready 비트(비트 6)이 설정되어 있으면 데이터를 받을 준비가 된 것이므로 종료
        if ((bStatus & ATA_STATUS_READY) == ATA_STATUS_READY) {
            return TRUE;
        }

        kSleep(1);
    }

    return FALSE;
}

/**
 * 인터럽트 발생 여부를 설정
 */
void kSetATAInterruptFlag(BOOL bPrimary, BOOL bFlag) {
    if (bPrimary == TRUE) {
        gs_stATAManager.bPrimaryInterruptOccur = bFlag;
    } else {
        gs_stATAManager.bSecondaryInterruptOccur = bFlag;
    }
}

/**
 * 인터럽트가 발생할 때까지 대기
 */
static BOOL kWaitForATAInterrupt(BOOL bPrimary) {
    QWORD qwTickCount = kGetTickCount();

    // 일정 시간 동안  인터럽트가 발생할 때까지 대기
    while (kGetTickCount() - qwTickCount <= ATA_WAITTIME) {
        if ((bPrimary == TRUE) && (gs_stATAManager.bPrimaryInterruptOccur == TRUE)) {
            return TRUE;
        }

        if ((bPrimary == FALSE) && (gs_stATAManager.bSecondaryInterruptOccur == TRUE)) {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * WORD 내의 바이트 순서를 바꿈
 */
static void kSwapByteInWord(WORD* pwData, int iWordCount) {
    for (int i = 0; i < iWordCount; i++) {
        WORD wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}

/**
 * 하드 디스크 디바이스 드라이버를 초기화
 */
BOOL kInitATA(void) {
    kInitMutex(&(gs_stATAManager.stMutex));

    gs_stATAManager.bPrimaryInterruptOccur = FALSE;
    gs_stATAManager.bSecondaryInterruptOccur = FALSE;

    // 첫 번째와 두 번째 ATA 포트의 디지털 출력 레지스터(포트 0x3F6와 0x376)에 0을
    // 출력하여 하드 디스크 컨트롤러의 인터럽트를 활성화
    outb(ATA_PORT_PRIMARYBASE + ATA_PORT_INDEX_DIGITALOUTPUT, 0);
    outb(ATA_PORT_SECONDARYBASE + ATA_PORT_INDEX_DIGITALOUTPUT, 0);

    // primary master 하드 디스크 정보 요청
    if (kReadHDDInformation(TRUE, TRUE, &(gs_stATAManager.stHDDInformation)) == FALSE) {
        gs_stATAManager.bHDDDetected = FALSE;
        gs_stATAManager.bCanWrite = FALSE;

        return FALSE;
    }

    gs_stATAManager.bHDDDetected = TRUE;
    gs_stATAManager.bCanWrite = TRUE;

    return TRUE;
}

/**
 * 하드 디스크의 정보를 읽음
 */
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation) {
    // PATA 포트에 따라서 I/O 포트의 기본 어드레스를 설정
    WORD wPortBase;
    if (bPrimary == TRUE) {
        wPortBase = ATA_PORT_PRIMARYBASE;
    } else {
        wPortBase = ATA_PORT_SECONDARYBASE;
    }
    kLock(&(gs_stATAManager.stMutex));

    // 아직 수행 중인 커맨드가 있다면 일정 시간 동안 끝날 때까지 대기
    if (kWaitForATANoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    // 드라이브와 헤드 데이터 설정
    BYTE bDriveFlag;
    if (bMaster == TRUE) {
        // 마스터이면 LBA 플래그만 설정
        bDriveFlag = ATA_DRIVEANDHEAD_LBA;
    } else {
        // 슬레이브이면 LBA 플래그에 슬레이브 플래그도 같이 설정
        bDriveFlag = ATA_DRIVEANDHEAD_LBA | ATA_DRIVEANDHEAD_SLAVE;
    }

    outb(wPortBase + ATA_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);
    if (kWaitForATAReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    kSetATAInterruptFlag(bPrimary, FALSE);

    // 커맨드 레지스터(포트 0x1F7 또는 0x177)에 드라이브 인식 커맨드(0xEC)를 전송
    outb(wPortBase + ATA_PORT_INDEX_COMMAND, ATA_COMMAND_IDENTIFY);

    BOOL bWaitResult = kWaitForATAInterrupt(bPrimary);
    BYTE bStatus = kReadATAStatus(bPrimary);
    if ((bWaitResult == FALSE) || ((bStatus & ATA_STATUS_ERROR) == ATA_STATUS_ERROR)) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    // 한 섹터를 읽음 (2 * 256 Byte)
    for (int i = 0; i < 256; i++) {
        ((WORD*)pstHDDInformation)[i] = inw(wPortBase + ATA_PORT_INDEX_DATA);
    }

    kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber) / 2);
    kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber) / 2);

    kUnlock(&(gs_stATAManager.stMutex));

    return TRUE;
}

/**
 * 하드 디스크의 섹터를 읽음
 * 최대 256개의 섹터를 읽을 수 있음
 * 실제로 읽은 섹터 수를 반환
 */
int kReadATASector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    if (gs_stATAManager.bHDDDetected == FALSE || iSectorCount <= 0) {
        return FALSE;
    }

    if (iSectorCount > 256) {
        iSectorCount = 256;
    }

    if (iSectorCount >= gs_stATAManager.stHDDInformation.dwTotalSectors - dwLBA) {
        iSectorCount = gs_stATAManager.stHDDInformation.dwTotalSectors - dwLBA;
    }

    WORD wPortBase;
    if (bPrimary == TRUE) {
        wPortBase = ATA_PORT_PRIMARYBASE;
    } else {
        wPortBase = ATA_PORT_SECONDARYBASE;
    }

    BYTE bDriveFlag;
    if (bMaster == TRUE) {
        bDriveFlag = ATA_DRIVEANDHEAD_LBA;
    } else {
        bDriveFlag = ATA_DRIVEANDHEAD_LBA | ATA_DRIVEANDHEAD_SLAVE;
    }

    kLock(&(gs_stATAManager.stMutex));

    if (kWaitForATANoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    // 섹터 수 레지스터에 읽을 섹터 수를 전송
    outb(wPortBase + ATA_PORT_INDEX_SECTORCOUNT, iSectorCount);
    // 섹터 번호 레지스터에 읽을 섹터 위치(LBA 0~7비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터에 읽을 섹터 위치(LBA 8~15비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터에 읽을 섹터 위치(LBA 16~23비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // 드라이브/헤드 레지스터에 읽을 섹터의 위치(LBA 24~27비트)와 설정된 값을 같이 전송
    outb(wPortBase + ATA_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

    // 커맨드 전송 준비
    if (kWaitForATAReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    kSetATAInterruptFlag(bPrimary, FALSE);

    // 커맨드 레지스터에 읽기(0x20)을 전송
    outb(wPortBase + ATA_PORT_INDEX_COMMAND, ATA_COMMAND_READ);

    // 데이터 수신
    int i;
    long lReadCount = 0;
    for (i = 0; i < iSectorCount; i++) {
        BYTE bStatus = kReadATAStatus(bPrimary);
        if ((bStatus & ATA_STATUS_ERROR) == ATA_STATUS_ERROR) {
            kUnlock(&(gs_stATAManager.stMutex));

            return FALSE;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 수신되길 기다림
        if ((bStatus & ATA_STATUS_DATAREQUEST) != ATA_STATUS_DATAREQUEST) {
            BOOL bWaitResult = kWaitForATAInterrupt(bPrimary);
            kSetATAInterruptFlag(bPrimary, FALSE);

            if (bWaitResult == FALSE) {
                kUnlock(&(gs_stATAManager.stMutex));

                return FALSE;
            }
        }

        for (int j = 0; j < 256; j++) {
            ((WORD*)pcBuffer)[lReadCount++] = inw(wPortBase + ATA_PORT_INDEX_DATA);
        }
    }

    kUnlock(&(gs_stATAManager.stMutex));

    return i;
}

/**
 * 하드 디스크에 섹터를 씀
 * 최대 256개의 섹터를 쓸 수 있음
 * 실제로 쓴 섹터 수를 반환
 */
int kWriteATASector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer) {
    if (gs_stATAManager.bHDDDetected == FALSE || iSectorCount <= 0) {
        return FALSE;
    }

    if (iSectorCount > 256) {
        iSectorCount = 256;
    }

    if (iSectorCount >= gs_stATAManager.stHDDInformation.dwTotalSectors - dwLBA) {
        iSectorCount = gs_stATAManager.stHDDInformation.dwTotalSectors - dwLBA;
    }

    WORD wPortBase;
    if (bPrimary == TRUE) {
        wPortBase = ATA_PORT_PRIMARYBASE;
    } else {
        wPortBase = ATA_PORT_SECONDARYBASE;
    }

    BYTE bDriveFlag;
    if (bMaster == TRUE) {
        bDriveFlag = ATA_DRIVEANDHEAD_LBA;
    } else {
        bDriveFlag = ATA_DRIVEANDHEAD_LBA | ATA_DRIVEANDHEAD_SLAVE;
    }

    kLock(&(gs_stATAManager.stMutex));

    if (kWaitForATANoBusy(bPrimary) == FALSE) {
        return FALSE;
    }

    // 섹터 수 레지스터에 쓸 섹터 수를 전송
    outb(wPortBase + ATA_PORT_INDEX_SECTORCOUNT, iSectorCount);
    // 섹터 번호 레지스터에 쓸 섹터 위치(LBA 0~7비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_SECTORNUMBER, dwLBA);
    // 실린더 LSB 레지스터에 쓸 섹터 위치(LBA 8~15비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    // 실린더 MSB 레지스터에 쓸 섹터 위치(LBA 16~23비트)를 전송
    outb(wPortBase + ATA_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    // 드라이브/헤드 레지스터에 쓸 섹터의 위치(LBA 24~27비트)와 설정된 값을 같이 전송
    outb(wPortBase + ATA_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

    // 커맨드 전송 준비
    if (kWaitForATAReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stATAManager.stMutex));

        return FALSE;
    }

    // 커맨드 전송
    outb(wPortBase + ATA_PORT_INDEX_COMMAND, ATA_COMMAND_WRITE);

    // 데이터 송신이 가능할 때까지 대기
    while (1) {
        BYTE bStatus = kReadATAStatus(bPrimary);
        if ((bStatus & ATA_STATUS_ERROR) == ATA_STATUS_ERROR) {
            kUnlock(&(gs_stATAManager.stMutex));

            return FALSE;
        }

        // Data Request 비트가 설정되었다면 데이터 송신 가능
        if ((bStatus & ATA_STATUS_DATAREQUEST) == ATA_STATUS_DATAREQUEST) {
            break;
        }

        kSleep(1);
    }

    // 데이터 송신
    int i;
    long lReadCount = 0;
    for (i = 0; i < iSectorCount; i++) {
        kSetATAInterruptFlag(bPrimary, FALSE);
        for (int j = 0; j < 256; j++) {
            outw(wPortBase + ATA_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lReadCount++]);
        }

        BYTE bStatus = kReadATAStatus(bPrimary);
        if ((bStatus & ATA_STATUS_ERROR) == ATA_STATUS_ERROR) {
            kUnlock(&(gs_stATAManager.stMutex));

            return i;
        }

        // DATAREQUEST 비트가 설정되지 않았으면 데이터가 처리가 완료되길 기다림
        if ((bStatus & ATA_STATUS_DATAREQUEST) != ATA_STATUS_DATAREQUEST) {
            BOOL bWaitResult = kWaitForATAInterrupt(bPrimary);
            kSetATAInterruptFlag(bPrimary, FALSE);

            if (bWaitResult == FALSE) {
                kUnlock(&(gs_stATAManager.stMutex));

                return FALSE;
            }
        }
    }

    kUnlock(&(gs_stATAManager.stMutex));

    return i;
}
