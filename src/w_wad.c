/* w_wad.c - Memory Mapped Version (Fixed) */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

// [Added] Missing Struct Definitions
typedef struct {
    char        identification[4];
    int         numlumps;
    int         infotableofs;
} __attribute__((packed)) wadinfo_t;

typedef struct {
    int         filepos;
    int         size;
    char        name[8];
} __attribute__((packed)) filelump_t;


// [External Symbols from objcopy]
extern const unsigned char _binary_doom1_wad_start[];
extern const unsigned char _binary_doom1_wad_end[];

// Pointer to WAD data in memory
static const unsigned char* wad_data = _binary_doom1_wad_start;

// Globals
lumpinfo_t* lumpinfo;
int         numlumps;
void** lumpcache;

char* strupr (char* str) {
    char *s = str;
    while (*s) { *s = toupper(*s); s++; }
    return str;
}

void ExtractFileBase (char* path, char* dest) {
    char* src = path + strlen(path) - 1;
    while (src != path && *(src-1) != '\\' && *(src-1) != '/') src--;
    memset (dest,0,8);
    int length = 0;
    while (*src && *src != '.') {
        if (++length == 9) I_Error ("Filename base of %s >8 chars",path);
        *dest++ = toupper((int)*src++);
    }
}

void W_AddFile (char *filename) {
    wadinfo_t* header;
    lumpinfo_t* lump_p;
    int i;
    filelump_t* fileinfo;

    // Read header directly from memory
    header = (wadinfo_t*)wad_data;

    if (strncmp(header->identification,"IWAD",4) &&
        strncmp(header->identification,"PWAD",4)) {
        I_Error ("Wad file identification not IWAD or PWAD");
    }

    int numlumps_val = LONG(header->numlumps);
    int infotableofs_val = LONG(header->infotableofs);
    int startlump = numlumps;

    // Calculate directory position
    fileinfo = (filelump_t*)(wad_data + infotableofs_val);

    numlumps += numlumps_val;
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));
    if (!lumpinfo) I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    for (i=0 ; i<numlumps_val ; i++,lump_p++, fileinfo++) {
        lump_p->handle = 0; 
        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
        strncpy (lump_p->name, fileinfo->name, 8);
    }
}

void W_Reload (void) {}

void W_InitMultipleFiles (char** filenames) {
    numlumps = 0;
    lumpinfo = malloc(1);
    W_AddFile (NULL); 
    if (!numlumps) I_Error ("W_InitFiles: no files found");
    
    int size = numlumps * sizeof(*lumpcache);
    lumpcache = malloc (size);
    if (!lumpcache) I_Error ("Couldn't allocate lumpcache");
    memset (lumpcache,0, size);
}

void W_InitFile (char* filename) {
    char* names[2];
    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles (names);
}

int W_NumLumps (void) { return numlumps; }

int W_CheckNumForName (char* name) {
    union { char s[9]; int x[2]; } name8;
    int v1, v2;
    lumpinfo_t* lump_p;

    strncpy (name8.s,name,8);
    name8.s[8] = 0;
    strupr (name8.s);
    v1 = name8.x[0];
    v2 = name8.x[1];

    lump_p = lumpinfo + numlumps;
    while (lump_p-- != lumpinfo) {
        if ( *(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
            return lump_p - lumpinfo;
    }
    return -1;
}

int W_GetNumForName (char* name) {
    int i = W_CheckNumForName (name);
    if (i == -1) I_Error ("W_GetNumForName: %s not found!", name);
    return i;
}

int W_LumpLength (int lump) {
    if (lump >= numlumps) I_Error ("W_LumpLength: %i >= numlumps",lump);
    return lumpinfo[lump].size;
}

void W_ReadLump (int lump, void* dest) {
    lumpinfo_t* l;
    if (lump >= numlumps) I_Error ("W_ReadLump: %i >= numlumps",lump);
    l = lumpinfo+lump;
    memcpy(dest, wad_data + l->position, l->size);
}

void* W_CacheLumpNum (int lump, int tag) {
    if ((unsigned)lump >= numlumps) I_Error ("W_CacheLumpNum: %i >= numlumps",lump);
    if (!lumpcache[lump]) {
        Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
        W_ReadLump (lump, lumpcache[lump]);
    } else {
        Z_ChangeTag (lumpcache[lump],tag);
    }
    return lumpcache[lump];
}

void* W_CacheLumpName (char* name, int tag) {
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}

void W_Profile (void) {}