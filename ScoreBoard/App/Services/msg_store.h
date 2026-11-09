/*
 * msg_store.h - the message list kept in the EEPROM, and how it is played.
 *
 * Slots of 96 bytes at 0x0600..0x0DFF (21 messages on a 4 KB EEPROM) and
 * from 0x1000 on larger EEPROMs (at most MSG_MAX). Each slot is an EEPROM
 * record (length + CRC), so a message lost in a power cut is just dropped.
 * Ids are 1-based slot numbers.
 */
#ifndef APP_MSG_STORE_H
#define APP_MSG_STORE_H

#include "msg_queue.h"

typedef struct {
  uint8_t order;           /* MsgOrder */
  uint8_t show_s;          /* seconds a message that fits is shown (1..60) */
  uint8_t ticker;          /* 1 = also shown in the clock / countdown text line */
  uint8_t gap_s;           /* ticker: seconds the normal text (date, days) stays between messages, 1..250 */
} MsgConfig;

void MsgStore_Load(void);                          /* at boot */
const MsgConfig *MsgStore_Config(void);
void MsgStore_SetConfig(const MsgConfig *c);       /* saved at once */

int  MsgStore_Count(void);
int  MsgStore_Capacity(void);                      /* slots available */
uint32_t MsgStore_EepromSize(void);
const Msg *MsgStore_Get(int id);                   /* NULL if free */
int  MsgStore_Add(const char *line1, const char *line2, uint8_t color);   /* id, -1 full, -2 bad text, -3 EEPROM */
int  MsgStore_Delete(int id);                      /* 0 = ok */
void MsgStore_Clear(void);
int  MsgStore_Sorted(uint8_t *ids);                /* ids in play order, returns count */
int  MsgStore_Next(void);                          /* id to show next (order setting), 0 = none */
int  MsgStore_Prev(void);
void MsgStore_SetCurrent(int id);                  /* e.g. "msg show" */
/* Limit MsgStore_Next to some messages (bit id-1 set): each screen has its
 * own set (screen_layout.c); MSG_PLAYLIST_ALL = every message, 0 = none. */
#define MSG_PLAYLIST_ALL 0xFFFFFFFFu
void MsgStore_SetPlaylist(uint32_t mask);
int  MsgStore_PlaylistCount(void);                 /* messages that can be shown now */
uint32_t MsgStore_Changes(void);                   /* increments on every change */

#endif
