/*
 * msg_queue.h - the message list (no hardware access, tested on the PC).
 *
 * Messages have one or two lines of up to MSG_LINE_MAX printable ASCII
 * characters and a colour. Each sits in a numbered slot (id = slot + 1, so
 * ids do not change when other messages are deleted); the play order is the
 * order they were added (seq). Storage is done by msg_store.c.
 */
#ifndef APP_MSG_QUEUE_H
#define APP_MSG_QUEUE_H

#include <stdint.h>

#define MSG_LINE_MAX  40
#define MSG_MAX       32        /* slots held in RAM; fewer if the EEPROM is small */

typedef struct {
  uint16_t seq;                 /* add order; 0 = free slot */
  uint8_t  color;               /* MatrixColor of the text */
  char     line1[MSG_LINE_MAX + 1];
  char     line2[MSG_LINE_MAX + 1];   /* "" = one-line message */
} Msg;

typedef enum { MSG_ORDER_SEQ, MSG_ORDER_RANDOM } MsgOrder;

typedef struct {
  Msg     slots[MSG_MAX];
  uint8_t slot_count;           /* usable slots */
  uint16_t next_seq;
  int8_t  current;              /* slot shown last, -1 = none */
} MsgQueue;

void MsgQueue_Init(MsgQueue *q, uint8_t slot_count);
void MsgQueue_Loaded(MsgQueue *q);                 /* after filling slots from storage */
int  MsgQueue_Count(const MsgQueue *q);
int  MsgQueue_TextValid(const char *s, int allow_empty);
/* returns the slot used, -1 = full, -2 = invalid text */
int  MsgQueue_Add(MsgQueue *q, const char *line1, const char *line2, uint8_t color);
int  MsgQueue_Delete(MsgQueue *q, int slot);       /* 0 = ok */
int  MsgQueue_Sorted(const MsgQueue *q, uint8_t *slots);   /* slots in play order, returns count */
/* Next / previous slot to show after 'current' (random: any other one,
 * using rnd); -1 = no messages. Sets current. */
int  MsgQueue_Next(MsgQueue *q, MsgOrder order, uint32_t rnd);
int  MsgQueue_Prev(MsgQueue *q);
/* true when the seq counter must be renumbered (MsgQueue_Renumber) */
int  MsgQueue_NeedsRenumber(const MsgQueue *q);
void MsgQueue_Renumber(MsgQueue *q);               /* seq = 1..n in play order */

#endif
