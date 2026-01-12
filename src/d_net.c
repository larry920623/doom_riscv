// d_net.c - Simplified for Bare-metal RISC-V (Single Player Only)

#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t *cmd);

// ------------------------------------------------------------------
// 全域變數
// ------------------------------------------------------------------
doomcom_t* doomcom;
doomdata_t* netbuffer;

// 單人遊戲不需要複雜的網路 Buffer，只需要本地的指令暫存
ticcmd_t        localcmds[BACKUPTICS];

// 為了避免 malloc，我們直接靜態宣告 doomcom 結構
static doomcom_t  doomcom_static;

int         maketic;
int         ticdup;
ticcmd_t    netcmds[MAXPLAYERS][BACKUPTICS];

// ------------------------------------------------------------------
// D_CheckNetGame
// 這是最重要的函式。原本它會去檢查網路參數。
// 我們改寫它，強制設定為「單人模式」。
// ------------------------------------------------------------------
void D_CheckNetGame (void)
{
    // 強制設定為單人
    netgame = false;
    
    // 初始化 doomcom (使用靜態記憶體，避免 Heap 問題)
    doomcom = &doomcom_static;
    memset(doomcom, 0, sizeof(doomcom_t));
    
    doomcom->id = DOOMCOM_ID;
    doomcom->numplayers = doomcom->numnodes = 1;
    doomcom->deathmatch = false;
    doomcom->consoleplayer = 0;
    doomcom->ticdup = 1;
    doomcom->extratics = 0;

    // 讓 netbuffer 指向 doomcom 內部的 data 區塊
    netbuffer = &doomcom->data;

    // 設定玩家狀態
    playeringame[0] = true;
    consoleplayer = 0; // 我們是玩家 1

    // 確保其他玩家被標記為不存在
    for (int i = 1; i < MAXPLAYERS; i++)
        playeringame[i] = false;
}

// ------------------------------------------------------------------
// D_QuitNetGame
// 裸機不需要通知其他電腦離線，直接留空即可。
// ------------------------------------------------------------------
void D_QuitNetGame (void)
{
    // Do nothing
}

// ------------------------------------------------------------------
// NetUpdate
// 負責讀取輸入並建立 "Tic Command" (移動、開槍等指令)
// ------------------------------------------------------------------
int gametime = 0;

void NetUpdate (void)
{
    int nowtime;
    int newtics;
    int i;

    // 1. 取得現在時間 (依賴 i_system.c 的 I_GetTime)
    nowtime = I_GetTime () / doomcom->ticdup;
    newtics = nowtime - gametime;
    gametime = nowtime;

    if (newtics <= 0) 
        return; // 沒有新的時間流逝，不需要更新

    // 2. 處理輸入並建立指令
    for (i = 0; i < newtics; i++)
    {
        I_StartTic ();          // 處理硬體輸入 (鍵盤/滑鼠)
        D_ProcessEvents ();     // 處理事件佇列
        
        // 將輸入轉換為遊戲指令 (TicCmd)
        G_BuildTiccmd (&localcmds[maketic % BACKUPTICS]);
        maketic++;
    }
}

// ------------------------------------------------------------------
// TryRunTics
// 這是遊戲的主迴圈心臟。
// 它負責比較「現在時間」跟「遊戲時間」，如果時間到了就跑一格遊戲邏輯。
// ------------------------------------------------------------------
void TryRunTics (void)
{
    int i;
    int entertic;
    int realtics;
    int counts;

    // 計算經過了多少時間
    entertic = I_GetTime () / doomcom->ticdup;
    realtics = entertic - gametime; // 這裡簡化計算，直接追趕時間

    // 呼叫 NetUpdate 讀取玩家輸入
    NetUpdate ();

    // 如果落後太多，稍微限制一下，避免死結
    if (realtics < 1) counts = 0;
    else if (realtics > 10) counts = 10; // 避免一次跑太多 lag
    else counts = realtics;

    if (counts < 1) return; // 不需要跑

    // 執行遊戲邏輯 (Game Ticker)
    while (counts--)
    {
        // 執行遊戲邏輯
        G_Ticker ();
        gametic++;
        
        // 處理 ticdup (裸機通常 ticdup=1，這段只是為了相容性)
        for (i = 0; i < doomcom->ticdup; i++)
        {
             if (i > 0) gametic++; // 只有 ticdup > 1 才需要
        }
    }
}

// ------------------------------------------------------------------
// 以下是為了滿足 Linker 的 Dummy 函式
// 避免編譯時出現 "undefined reference"
// ------------------------------------------------------------------
void HSendPacket (int node, int flags) {}
boolean HGetPacket (void) { return false; }
void GetPackets (void) {}
void D_ArbitrateNetStart (void) {} // 絕對不能有 while(1)