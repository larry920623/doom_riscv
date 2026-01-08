/*
 * i_sound.c
 *
 * Sound system support code
 *
 * Copyright (C) 2023 Chin Yik Ming
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

/* i_sound.c */
#include "i_sound.h"
#include <stdint.h>
#include <stdio.h>

void I_InitSound() {}
void I_UpdateSound(void) {}
void I_SubmitSound(void) {}
void I_ShutdownSound(void) {}
void I_SetChannels(void) {}

int I_GetSfxLumpNum(sfxinfo_t* sfxinfo) { return 0; }

// [FIXED] 配合 header: 接受 void* 和 int vol
void I_StartSound(void *data, int vol) 
{ 
}

void I_StopSound(int handle) {}
int I_SoundIsPlaying(int handle) { return 0; }

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch) {}

void I_InitMusic(void) {}
void I_ShutdownMusic(void) {}

void I_SetSfxVolume(int volume) { snd_SfxVolume = volume; }
void I_SetMusicVolume(int volume) { snd_MusicVolume = volume; }

void I_PauseSong(int handle) {}
void I_ResumeSong(int handle) {}
int I_RegisterSong(void *data) { return 0; }

// [FIXED] 配合 header: 這裡需要 3 個參數 (data, looping, volume)
void I_PlaySong(void *data, int looping, int volume) 
{
}

void I_StopSong(void) {} 
void I_UnRegisterSong(int handle) {}