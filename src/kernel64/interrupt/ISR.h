
#ifndef __ISR_H__
#define __ISR_H__

////////////////////////////////////////////////////////////////
// Exception ISR
////////////////////////////////////////////////////////////////
void kISR_DivideError(void);
void kISR_Debug(void);
void kISR_NMI(void);
void kISR_BreakPoint(void);
void kISR_Overflow(void);
void kISR_BoundRangeExceeded(void);
void kISR_InvalidOpcode();
void kISR_DeviceNotAvailable(void);
void kISR_DoubleFault(void);
void kISR_CoprocessorSegmentOverrun(void);
void kISR_InvalidTSS(void);
void kISR_SegmentNotPresent(void);
void kISR_StackSegmentFault(void);
void kISR_GeneralProtection(void);
void kISR_PageFault(void);
void kISR_15(void);
void kISR_FPUError(void);
void kISR_AlignmentCheck(void);
void kISR_MachineCheck(void);
void kISR_SIMDError(void);
void kISR_ETCException(void);

////////////////////////////////////////////////////////////////
// Interrupt ISR
////////////////////////////////////////////////////////////////
void kISR_Timer(void);
void kISR_Keyboard(void);
void kISR_SlavePIC(void);
void kISR_Serial2(void);
void kISR_Serial1(void);
void kISR_Parallel2(void);
void kISR_Floppy(void);
void kISR_Parallel1(void);
void kISR_RTC(void);
void kISR_Reserved(void);
void kISR_NotUsed1(void);
void kISR_NotUsed2(void);
void kISR_Mouse(void);
void kISR_Coprocessor(void);
void kISR_HDD1(void);
void kISR_HDD2(void);
void kISR_ETCInterrupt(void);

#endif /*__ISR_H__*/