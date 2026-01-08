/*
 * i_system.c
 *
 * System support code
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
 * Copyright (C) 2022-2025 National Cheng Kung University, Taiwan.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


/* i_system.c */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_video.h"
#include "i_system.h"
#include "console.h" // 如果編譯報錯找不到 console.h，可以註解掉這行

// [MyCPU Hardware Definition]
#define MYCPU_INPUT_BASE    0x40000000 
volatile uint32_t* const INPUT_PTR = (uint32_t*)MYCPU_INPUT_BASE;

// [原檔保留] 事件結構定義
enum { KEY_EVENT=0, MOUSE_MOTION_EVENT=1, MOUSE_BUTTON_EVENT=2, QUIT_EVENT=3 };
typedef struct { uint32_t keycode; uint8_t state; } key_event_t;
typedef struct { int32_t x, y, xrel, yrel; } mouse_motion_t;
typedef struct { uint8_t button; uint8_t state; } mouse_button_t;
typedef struct { uint32_t type; union { key_event_t key_event; union { mouse_motion_t motion; mouse_button_t button; } mouse; }; } emu_event_t;

// [MyCPU] 記憶體分配 - 使用靜態陣列避免 Heap 爆炸
#define DOOM_HEAP_SIZE (4 * 1024 * 1024)
byte doom_heap[DOOM_HEAP_SIZE];

void I_SetRelativeMode(boolean enabled) {
    // [Modified] 移除 ecall
}

void I_Init(void)
{
    // [Modified] 移除 ecall 與 Queue 初始化
    // MyCPU 不需要建立 Queue 給模擬器填，而是直接讀 MMIO
    I_SetRelativeMode(true);
}

byte *I_ZoneBase(int *size)
{
    // [MyCPU] 改用靜態分配
	*size = DOOM_HEAP_SIZE;
	return doom_heap;
}

// [MyCPU] 時間函式 - 使用 rdcycle
#define CPU_FREQ 50000000
#define TICKS_PER_SEC 35
int I_GetTime(void)
{
    unsigned long long cycles;
    asm volatile ("rdcycle %0" : "=r" (cycles));
	return (int)((cycles * TICKS_PER_SEC) / CPU_FREQ);
}

// [Stub] 暫時不處理輸入事件
static void I_GetRemoteEvent(void)
{
    // TODO: 未來在這裡讀取 *INPUT_PTR
    // 並將硬體訊號轉換為 event_t 結構傳給 D_PostEvent
}

void I_StartFrame(void) {}

void I_StartTic(void)
{
	I_GetRemoteEvent();
}

ticcmd_t *I_BaseTiccmd(void)
{
	static ticcmd_t emptycmd;
	return &emptycmd;
}

void I_Quit(void)
{
	I_ShutdownSound();
	D_QuitNetGame();
	M_SaveDefaults();
	I_ShutdownGraphics();
	while(1); // [MyCPU] 死迴圈取代 exit
}

byte *I_AllocLow(int length)
{
	byte *mem = malloc(length);
	if (!mem) I_Error("Failed to allocate %d bytes", length);
	return mem;
}

void I_Tactile ( int on, int off, int total ) {}

void I_Error(char *error, ...)
{
    // [Modified] 移除 printf，因為不一定有 UART
	va_list argptr;
	va_start (argptr,error);
	// vfprintf (stderr,error,argptr); // 可選：如果 UART 有通
	va_end (argptr);
	I_ShutdownGraphics();
	while(1); // [MyCPU] 死迴圈
}
