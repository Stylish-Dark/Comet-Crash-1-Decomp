#pragma once
#include <stdint.h>
typedef int32_t s32; typedef uint32_t u32; typedef uint16_t u16; typedef uint8_t u8;
typedef struct CellPadData { s32 len; u16 button[64]; } CellPadData;
#ifdef __cplusplus
extern "C" {
#endif
s32 cellPadGetData(u32,CellPadData*);
#ifdef __cplusplus
}
#endif
