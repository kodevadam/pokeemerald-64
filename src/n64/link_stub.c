/*
 * src/n64/link_stub.c
 *
 * N64 port — Phase 7: Link cable / RFU stub implementations.
 *
 * The N64 has no GBA link cable or wireless adapter.  All multiplayer
 * features are stubbed out so the single-player game compiles and runs
 * without modification.  Every function returns the "no link present"
 * value so existing callers silently skip multiplayer code paths.
 *
 * Replaces: src/link.c, src/link_rfu_2.c, src/link_rfu_3.c,
 *           src/librfu_rfu.c, src/librfu_intr.c, src/librfu_sio32id.c,
 *           src/librfu_stwi.c
 */

#include <string.h>
#include "global.h"
#include "link.h"
#include "link_rfu.h"
#include "multiboot.h"
#include "task.h"

/* -----------------------------------------------------------------------
 * Global data required by link.h externs
 * --------------------------------------------------------------------- */
struct Link       gLink;
u8                gBlockSendBuffer[BLOCK_BUFFER_SIZE];
u16               gBlockRecvBuffer[MAX_RFU_PLAYERS][BLOCK_BUFFER_SIZE / 2];
u16               gLinkType             = 0;
u32               gLinkStatus           = 0;
struct LinkPlayer gLinkPlayers[MAX_RFU_PLAYERS];
bool8             gReceivedRemoteLinkPlayers = FALSE;
u32               gBerryBlenderKeySendAttempts = 0;
/* gLinkVSyncDisabled is defined in main.c as u8 gLinkVSyncDisabled */
u16               gLinkPartnersHeldKeys[6];
u32               gLinkDebugSeed        = 0;
struct LinkPlayerBlock gLocalLinkPlayerBlock;
bool8             gLinkErrorOccurred    = FALSE;
u32               gLinkDebugFlags       = 0;
bool8             gRemoteLinkPlayersNotReceived[MAX_LINK_PLAYERS];
u16               gLinkHeldKeys         = 0;
bool8             gReadyToCloseLink[MAX_LINK_PLAYERS];
u16               gReadyCloseLinkType   = 0;
u8                gSuppressLinkErrorMessage = 0;
bool8             gSavedLinkPlayerCount = FALSE;
struct LinkTestBGInfo gLinkTestBGInfo;
struct LinkPlayer gLocalLinkPlayer;

/* -----------------------------------------------------------------------
 * Global data required by link_rfu.h externs
 * --------------------------------------------------------------------- */
struct RfuGameData gHostRfuGameData;
u8                 gHostRfuUsername[PLAYER_NAME_LENGTH + 1];
struct RfuManager  gRfu;
u8                 gWirelessStatusIndicatorSpriteId = 0xFF;

/* -----------------------------------------------------------------------
 * link.h function stubs
 * --------------------------------------------------------------------- */
bool8  IsWirelessAdapterConnected(void)                    { return FALSE; }
void   OpenLink(void)                                      {}
void   CloseLink(void)                                     {}
u16    LinkMain2(const u16 *heldKeys)                      { (void)heldKeys; return 0; }
void   ClearLinkCallback(void)                             {}
void   ClearLinkCallback_2(void)                           {}
u8     GetLinkPlayerCount(void)                            { return 1; }
void   OpenLinkTimed(void)                                 {}
u8     GetLinkPlayerDataExchangeStatusTimed(int min, int max) { (void)min; (void)max; return 0; }
bool8  IsLinkPlayerDataExchangeComplete(void)              { return FALSE; }
u32    GetLinkPlayerTrainerId(u8 who)                      { (void)who; return 0; }
void   ResetLinkPlayers(void)                              {}
u8     GetMultiplayerId(void)                              { return 0; }
u8     BitmaskAllOtherLinkPlayers(void)                    { return 0; }
bool8  SendBlock(u8 unused, const void *src, u16 size)     { (void)unused; (void)src; (void)size; return FALSE; }
u8     GetBlockReceivedStatus(void)                        { return 0; }
void   ResetBlockReceivedFlags(void)                       {}
void   ResetBlockReceivedFlag(u8 who)                      { (void)who; }
u8     GetLinkPlayerCount_2(void)                          { return 1; }
bool8  IsLinkMaster(void)                                  { return FALSE; }
void   CB2_LinkError(void)                                 {}
bool8  GetSioMultiSI(void)                                 { return FALSE; }
bool8  IsLinkConnectionEstablished(void)                   { return FALSE; }
bool8  HasLinkErrorOccurred(void)                          { return FALSE; }
void   ResetSerial(void)                                   {}
u32    LinkMain1(u8 *adv, u16 *sendCmd, u16 (*recvCmds)[CMD_LENGTH])
                                                           { (void)adv; (void)sendCmd; (void)recvCmds; return 0; }
void   LinkVSync(void)                                     {}
void   Timer3Intr(void)                                    {}
void   SerialCB(void)                                      {}
bool32 InUnionRoom(void)                                   { return FALSE; }
void   LoadWirelessStatusIndicatorSpriteGfx(void)          {}
bool8  IsLinkTaskFinished(void)                            { return TRUE; }
void   CreateWirelessStatusIndicatorSprite(u8 x, u8 y)    { (void)x; (void)y; }
void   SetLinkStandbyCallback(void)                        {}
void   SetWirelessCommType1(void)                          {}
void   CheckShouldAdvanceLinkState(void)                   {}
void   SetCloseLinkCallback(void)                          {}
bool8  HandleLinkConnection(void)                          { return FALSE; }
void   SetLinkDebugValues(u32 seed, u32 flags)             { (void)seed; (void)flags; }
void   SetBerryBlenderLinkCallback(void)                   {}
void   SetSuppressLinkErrorMessage(bool8 flag)             { gSuppressLinkErrorMessage = flag; }
void   ConvertLinkPlayerName(struct LinkPlayer *player)    { (void)player; }
void   ClearSavedLinkPlayers(void)                         {}
void   SetLinkErrorBuffer(u32 status, u8 lastSend, u8 lastRecv, bool8 dc)
                                                           { (void)status; (void)lastSend; (void)lastRecv; (void)dc; }
void   LocalLinkPlayerToBlock(void)                        {}
void   LinkPlayerFromBlock(u32 who)                        { (void)who; }
bool32 Link_AnyPartnersPlayingFRLG_JP(void)                { return FALSE; }
void   ResetLinkPlayerCount(void)                          {}
void   SaveLinkPlayers(u8 playerCount)                     { (void)playerCount; }
void   SetWirelessCommType0(void)                          {}
bool32 IsLinkRecvQueueAtOverworldMax(void)                 { return FALSE; }
bool32 Link_AnyPartnersPlayingRubyOrSapphire(void)         { return FALSE; }
u32    LinkDummy_Return2(void)                             { return 2; }
void   SetLocalLinkPlayerId(u8 playerId)                   { (void)playerId; }
u8     GetSavedPlayerCount(void)                           { return 1; }
bool8  SendBlockRequest(u8 blockReqType)                   { (void)blockReqType; return FALSE; }
u8     GetLinkPlayerCountAsBitFlags(void)                  { return 1; }
u8     GetSavedLinkPlayerCountAsBitFlags(void)             { return 1; }
void   SetCloseLinkCallbackHandleJP(void)                  {}

/* Task_DestroySelf is declared in link.h but lives in task.c on GBA.
 * On N64 task.c is compiled normally, so we don't re-define it here. */

/* -----------------------------------------------------------------------
 * link_rfu.h function stubs
 * --------------------------------------------------------------------- */
void   WipeTrainerNameRecords(void)                        {}
void   InitRFUAPI(void)                                    {}
void   LinkRfu_Shutdown(void)                              {}
void   Rfu_SetBlockReceivedFlag(u8 id)                     { (void)id; }
void   Rfu_ResetBlockReceivedFlag(u8 id)                   { (void)id; }
bool32 IsSendingKeysToRfu(void)                            { return FALSE; }
void   StartSendingKeysToRfu(void)                         {}
void   Rfu_SetBerryBlenderLinkCallback(void)               {}
u8     Rfu_GetBlockReceivedStatus(void)                    { return 0; }
bool32 Rfu_InitBlockSend(const u8 *src, size_t size)       { (void)src; (void)size; return FALSE; }
void   ClearLinkRfuCallback(void)                          {}
u8     Rfu_GetLinkPlayerCount(void)                        { return 1; }
u8     Rfu_GetMultiplayerId(void)                          { return 0; }
bool8  Rfu_SendBlockRequest(u8 type)                       { (void)type; return FALSE; }
bool8  IsLinkRfuTaskFinished(void)                         { return TRUE; }
bool8  Rfu_IsMaster(void)                                  { return FALSE; }
void   Rfu_SetCloseLinkCallback(void)                      {}
void   Rfu_SetLinkStandbyCallback(void)                    {}
void   ResetLinkRfuGFLayer(void)                           {}
void   UpdateWirelessStatusIndicatorSprite(void)           {}
void   InitRFU(void)                                       {}
bool32 RfuMain1(void)                                      { return FALSE; }
bool32 RfuMain2(void)                                      { return FALSE; }
bool32 RfuHasErrored(void)                                 { return FALSE; }
bool32 IsRfuRecvQueueEmpty(void)                           { return TRUE; }
u32    GetRfuRecvQueueLength(void)                         { return 0; }
void   RfuVSync(void)                                      {}
void   RfuSetIgnoreError(bool32 enable)                    { (void)enable; }
u8     RfuGetStatus(void)                                  { return 0; }
void   UpdateGameData_GroupLockedIn(bool8 start)           { (void)start; }
void   RfuSetErrorParams(u32 info)                         { (void)info; }
void   RfuSetStatus(u8 status, u16 errInfo)                { (void)status; (void)errInfo; }
u8     Rfu_SetLinkRecovery(bool32 enable)                  { (void)enable; return 0; }
void   CopyHostRfuGameDataAndUsername(struct RfuGameData *gd, u8 *name)
                                                           { (void)gd; (void)name; }
void   SetHostRfuGameData(u8 act, u32 info, bool32 started){ (void)act; (void)info; (void)started; }
void   InitializeRfuLinkManager_LinkLeader(u32 max)        { (void)max; }
bool32 IsRfuCommunicatingWithAllChildren(void)             { return FALSE; }
void   LinkRfu_StopManagerAndFinalizeSlots(void)           {}
bool32 RfuTryDisconnectLeavingChildren(void)               { return FALSE; }
bool32 HasTrainerLeftPartnersList(u16 id, const u8 *name)  { (void)id; (void)name; return FALSE; }

/* -----------------------------------------------------------------------
 * multiboot.h function stubs (multiboot.c excluded — ARM-only assembly)
 * --------------------------------------------------------------------- */
void MultiBootInit(struct MultiBootParam *mp)                                         { (void)mp; }
int  MultiBootMain(struct MultiBootParam *mp)                                         { (void)mp; return 0; }
void MultiBootStartProbe(struct MultiBootParam *mp)                                   { (void)mp; }
void MultiBootStartMaster(struct MultiBootParam *mp, const u8 *srcp, int length,
                          u8 palette_color, s8 palette_speed)
     { (void)mp; (void)srcp; (void)length; (void)palette_color; (void)palette_speed; }
int  MultiBootCheckComplete(struct MultiBootParam *mp)                                { (void)mp; return 1; }
