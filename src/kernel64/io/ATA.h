
#ifndef __ATA_H__
#define __ATA_H__

#include "../types.h"
#include "../util/sync.h"

#define ATA_PORT_PRIMARYBASE 0x1F0
#define ATA_PORT_SECONDARYBASE 0x170

#define ATA_PORT_INDEX_DATA 0x00
#define ATA_PORT_INDEX_SECTORCOUNT 0x02
#define ATA_PORT_INDEX_SECTORNUMBER 0x03
#define ATA_PORT_INDEX_CYLINDERLSB 0x04
#define ATA_PORT_INDEX_CYLINDERMSB 0x05
#define ATA_PORT_INDEX_DRIVEANDHEAD 0x06
#define ATA_PORT_INDEX_STATUS 0x07
#define ATA_PORT_INDEX_COMMAND 0x07
#define ATA_PORT_INDEX_DIGITALOUTPUT 0x206

#define ATA_COMMAND_READ 0x20
#define ATA_COMMAND_WRITE 0x30
#define ATA_COMMAND_IDENTIFY 0xEC

#define ATA_STATUS_ERROR 0x01
#define ATA_STATUS_INDEX 0x02
#define ATA_STATUS_CORRECTEDDATA 0x04
#define ATA_STATUS_DATAREQUEST 0x08
#define ATA_STATUS_SEEKCOMPLETE 0x10
#define ATA_STATUS_WRITEFAULT 0x20
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_BUSY 0x80

#define ATA_DRIVEANDHEAD_LBA 0xE0
#define ATA_DRIVEANDHEAD_SLAVE 0x10

#define ATA_DIGITALOUTPUT_RESET 0x04
#define ATA_DIGITALOUTPUT_DISABLEINTERRUPT 0x01

#define ATA_WAITTIME 500
#define ATA_MAXBULKSECTORCOUNT 256

#pragma pack(push, 1)

typedef struct kHDDInformationStruct {
    // 설정값
    WORD wConfiguation;

    // 실린더 수
    WORD wNumberOfCylinder;
    WORD wReserved1;

    // 헤드 수
    WORD wNumberOfHead;
    WORD wUnformattedBytesPerTrack;
    WORD wUnformattedBytesPerSector;

    // 실린더당 섹터 수
    WORD wNumberOfSectorPerCylinder;
    WORD wInterSectorGap;
    WORD wBytesInPhaseLock;
    WORD wNumberOfVendorUniqueStatusWord;

    // 하드 디스크의 시리얼 넘버
    WORD vwSerialNumber[10];
    WORD wControllerType;
    WORD wBufferSize;
    WORD wNumberOfECCBytes;
    WORD vwFirmwareRevision[4];

    // 하드 디스크의 모델 번호
    WORD vwModelNumber[20];
    WORD vwReserved2[13];

    // 디스크의 총 섹터 수
    DWORD dwTotalSectors;
    WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

typedef struct kATAManagerStruct {
    BOOL bHDDDetected;
    BOOL bCanWrite;

    // 인터럽트 발생 여부와 동기화 객체
    volatile BOOL bPrimaryInterruptOccur;
    volatile BOOL bSecondaryInterruptOccur;
    MUTEX stMutex;

    // HDD 정보
    HDDINFORMATION stHDDInformation;
} ATAMANAGER;

BOOL kInitATA(void);
BOOL kReadATAInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadATASector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteATASector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
void kSetATAInterruptFlag(BOOL bPrimary, BOOL bFlag);

#endif /*__ATA_H__*/
