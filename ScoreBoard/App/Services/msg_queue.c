/*
 * msg_queue.c - the message list. See msg_queue.h.
 */
#include "msg_queue.h"
#include <string.h>

void MsgQueue_Init(MsgQueue *q, uint8_t slot_count)
{
  memset(q, 0, sizeof *q);
  q->slot_count = (slot_count > MSG_MAX) ? MSG_MAX : slot_count;
  q->next_seq = 1;
  q->current = -1;
}

void MsgQueue_Loaded(MsgQueue *q)
{
  q->next_seq = 1;
  for (int i = 0; i < q->slot_count; i++)
  {
    if (q->slots[i].seq >= q->next_seq) q->next_seq = (uint16_t)(q->slots[i].seq + 1u);
  }
  q->current = -1;
}

int MsgQueue_Count(const MsgQueue *q)
{
  int n = 0;
  for (int i = 0; i < q->slot_count; i++)
  {
    if (q->slots[i].seq != 0u) n++;
  }
  return n;
}

int MsgQueue_TextValid(const char *s, int allow_empty)
{
  size_t n = strlen(s);
  if (n > MSG_LINE_MAX || (n == 0u && !allow_empty)) return 0;
  for (; *s; s++)
  {
    if (*s < 32 || *s > 126) return 0;
  }
  return 1;
}

int MsgQueue_Add(MsgQueue *q, const char *line1, const char *line2, uint8_t color)
{
  if (line2 == NULL) line2 = "";
  if (!MsgQueue_TextValid(line1, 0) || !MsgQueue_TextValid(line2, 1)) return -2;
  for (int i = 0; i < q->slot_count; i++)
  {
    Msg *m = &q->slots[i];
    if (m->seq == 0u)
    {
      memset(m, 0, sizeof *m);
      m->seq = q->next_seq++;
      m->color = color;
      strcpy(m->line1, line1);
      strcpy(m->line2, line2);
      return i;
    }
  }
  return -1;
}

int MsgQueue_Delete(MsgQueue *q, int slot)
{
  if (slot < 0 || slot >= q->slot_count || q->slots[slot].seq == 0u) return -1;
  memset(&q->slots[slot], 0, sizeof q->slots[slot]);
  if (q->current == slot) q->current = -1;
  return 0;
}

static int P_SortedIn(const MsgQueue *q, uint8_t *slots, uint32_t mask)
{
  int n = 0;
  for (int i = 0; i < q->slot_count; i++)
  {
    if (q->slots[i].seq == 0u || (mask & (1uL << i)) == 0u) continue;
    int j = n++;
    while (j > 0 && q->slots[slots[j - 1]].seq > q->slots[i].seq)   /* insertion sort */
    {
      slots[j] = slots[j - 1];
      j--;
    }
    slots[j] = (uint8_t)i;
  }
  return n;
}

static int P_Pos(const uint8_t *order, int n, int slot)
{
  for (int i = 0; i < n; i++)
  {
    if (order[i] == slot) return i;
  }
  return -1;
}

int MsgQueue_Sorted(const MsgQueue *q, uint8_t *slots)
{
  return P_SortedIn(q, slots, 0xFFFFFFFFu);
}

int MsgQueue_Next(MsgQueue *q, MsgOrder order, uint32_t rnd)
{
  return MsgQueue_NextIn(q, order, rnd, 0xFFFFFFFFu);
}

int MsgQueue_NextIn(MsgQueue *q, MsgOrder order, uint32_t rnd, uint32_t mask)
{
  uint8_t list[MSG_MAX];
  int n = P_SortedIn(q, list, mask);
  if (n == 0) return q->current = -1;
  int pos = P_Pos(list, n, q->current);
  if (order == MSG_ORDER_RANDOM && n > 1)
  {
    int pick = (int)(rnd % (uint32_t)(pos >= 0 ? n - 1 : n));
    if (pos >= 0 && pick >= pos) pick++;          /* never the same one twice in a row */
    q->current = (int8_t)list[pick];
  }
  else
  {
    q->current = (int8_t)list[(pos + 1) % n];     /* pos -1 -> first */
  }
  return q->current;
}

int MsgQueue_Prev(MsgQueue *q)
{
  uint8_t list[MSG_MAX];
  int n = MsgQueue_Sorted(q, list);
  if (n == 0) return q->current = -1;
  int pos = P_Pos(list, n, q->current);
  q->current = (int8_t)list[(pos <= 0) ? n - 1 : pos - 1];
  return q->current;
}

int MsgQueue_NeedsRenumber(const MsgQueue *q)
{
  return q->next_seq >= 0xFF00u;
}

void MsgQueue_Renumber(MsgQueue *q)
{
  uint8_t list[MSG_MAX];
  int n = MsgQueue_Sorted(q, list);
  for (int i = 0; i < n; i++)
  {
    q->slots[list[i]].seq = (uint16_t)(i + 1);
  }
  q->next_seq = (uint16_t)(n + 1);
}
