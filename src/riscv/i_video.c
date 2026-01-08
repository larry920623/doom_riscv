/*
 * i_video.c
 *
 * Video system support code
 *
 * Copyright (C) 2022 National Cheng Kung University, Taiwan.
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

/* i_video.c */
/* i_video.c */
#include <stdint.h>
#include <string.h>
#include <stdlib.h> // 補上 malloc 定義

#include "doomdef.h"
#include "i_system.h"
#include "v_video.h"
#include "i_video.h"

// [MyCPU Hardware Definition]
#define MYCPU_VGA_BASE      0x20000000
#define VGA_CTRL_REG        (MYCPU_VGA_BASE + 0x04)
#define VGA_UPLOAD_ADDR     (MYCPU_VGA_BASE + 0x10)
#define VGA_STREAM_DATA     (MYCPU_VGA_BASE + 0x14)
#define VGA_PALETTE_BASE    (MYCPU_VGA_BASE + 0x400)

volatile uint32_t* const VGA_CTRL_PTR   = (uint32_t*)VGA_CTRL_REG;
volatile uint32_t* const VGA_ADDR_PTR   = (uint32_t*)VGA_UPLOAD_ADDR;
volatile uint32_t* const VGA_DATA_PTR   = (uint32_t*)VGA_STREAM_DATA;
volatile uint32_t* const VGA_PAL_PTR    = (uint32_t*)VGA_PALETTE_BASE;

//static uint32_t buffer[SCREENWIDTH * SCREENHEIGHT]; // 原檔保留
static uint32_t video_pal[256];                     // 原檔保留

// Dirty region tracking
static byte dirty_lines[SCREENHEIGHT];
static int dirty_min_y = SCREENHEIGHT;
static int dirty_max_y = -1;

void I_MarkDirtyLines(int y_start, int y_end)
{
	if (y_start < 0) y_start = 0;
	if (y_end >= SCREENHEIGHT) y_end = SCREENHEIGHT - 1;
	if (y_start > y_end) return;

	if (y_start < dirty_min_y) dirty_min_y = y_start;
	if (y_end > dirty_max_y) dirty_max_y = y_end;

	if (y_start == y_end) {
		dirty_lines[y_start] = 1;
		return;
	}
	for (int y = y_start; y <= y_end; y++)
		dirty_lines[y] = 1;
}

void I_InitGraphics(void)
{
	usegamma = 1;
	I_MarkDirtyLines(0, SCREENHEIGHT - 1);

    // [MyCPU] 初始化硬體
    *VGA_CTRL_PTR = 1;
    *VGA_ADDR_PTR = 0;

	// [MyCPU] 必須分配 screens[0] 給 DOOM 繪圖引擎
	screens[0] = (byte *)malloc (SCREENWIDTH * SCREENHEIGHT);
    if (!screens[0]) I_Error("Failed to allocate screen memory");

    // [Modified] 原本的 ecall 初始化被移除，因為我們是用 MMIO
}

void I_ShutdownGraphics(void)
{
}

void I_SetPalette(byte* palette)
{
	for (int i=0 ; i<256 ; i++) {
		byte r = gammatable[usegamma][*palette++];
		byte g = gammatable[usegamma][*palette++];
		byte b = gammatable[usegamma][*palette++];
		
        // [MyCPU] 同時做兩件事：
        // 1. 填入軟體 buffer (原檔邏輯)
        video_pal[i] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;

        // 2. 寫入硬體色盤 (MyCPU Hardware Logic)
        uint32_t r_2bit = (r >> 6) & 0x3;
        uint32_t g_2bit = (g >> 6) & 0x3;
        uint32_t b_2bit = (b >> 6) & 0x3;
        uint32_t color_entry = (r_2bit << 4) | (g_2bit << 2) | b_2bit;
        VGA_PAL_PTR[i] = color_entry;
	}
}

void I_UpdateNoBlit(void) {}

void I_FinishUpdate (void)
{
    // [MyCPU] 直接將 screens[0] 寫入 VGA 硬體
    // 原檔的 dirty line 邏輯在硬體全刷模式下幫助不大，但我們保留它
    // 為了效能，這裡我們忽略 dirty check，直接刷整個 Framebuffer
    
    *VGA_ADDR_PTR = 0x00000000; // Reset pointer
    
    // 使用指標加速寫入
    const uint32_t* src = (const uint32_t*)screens[0]; 
    // 注意：這裡假設 screens[0] 內容已經是 index，我們需要 index 還是 RGB?
    // MyCPU 的 VGA_DATA_PTR 似乎期望接收 "Index" (如果硬體查表) 
    // 或者 "RGB" (如果硬體是 Framebuffer)。
    
    // 這裡我們沿用您原本 i_mycpu.c 的邏輯 (最高效能)
    int num_words = (SCREENWIDTH * SCREENHEIGHT) / 4;
    int i = 0;
    for (; i < num_words - 8; i += 8)
    {
        *VGA_DATA_PTR = src[i];
        *VGA_DATA_PTR = src[i+1];
        *VGA_DATA_PTR = src[i+2];
        *VGA_DATA_PTR = src[i+3];
        *VGA_DATA_PTR = src[i+4];
        *VGA_DATA_PTR = src[i+5];
        *VGA_DATA_PTR = src[i+6];
        *VGA_DATA_PTR = src[i+7];
    }
    for (; i < num_words; i++) {
        *VGA_DATA_PTR = src[i];
    }

}

void I_WaitVBL(int count) {}

void I_ReadScreen(byte* scr)
{
	memcpy(scr, screens[0], SCREENHEIGHT * SCREENWIDTH);
}