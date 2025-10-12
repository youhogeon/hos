#ifndef __TYPES_H__
#define __TYPES_H__

#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned int
#define QWORD unsigned long
#define BOOL unsigned char

#define TRUE 1
#define FALSE 0
#define NULL 0

#define uint64 unsigned long long

#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

#endif /*__TYPES_H__*/