/*
 * msg_store.c - messages in the EEPROM. See msg_store.h.
 */
#include "msg_store.h"
#include "ee_record.h"
#include "eeprom_24lc.h"
#include "app_hal.h"
#include "log.h"
#include <string.h>

#define SLOT_SIZE      96u
#define AREA1_START    0x0600u
#define AREA1_END      0x0E00u
#define AREA2_START    0x1000u
#define AREA1_SLOTS    ((AREA1_END - AREA1_START) / SLOT_SIZE)

static MsgQueue queue;
static MsgConfig cfg = { MSG_ORDER_SEQ, 5, 1, 20 };
static uint32_t eeprom_size;
static uint32_t changes;
static uint32_t playlist = MSG_PLAYLIST_ALL;

static uint16_t P_SlotAddr(int slot)
{
  if (slot < (int)AREA1_SLOTS) return (uint16_t)(AREA1_START + (uint32_t)slot * SLOT_SIZE);
  return (uint16_t)(AREA2_START + (uint32_t)(slot - (int)AREA1_SLOTS) * SLOT_SIZE);
}

static void P_SaveSlot(int slot)
{
  if (EeRecord_Save(P_SlotAddr(slot), &queue.slots[slot], sizeof(Msg)) != 0)
  {
    LOG_E("Messages: EEPROM write failed (slot %d)", slot + 1);
  }
}

void MsgStore_Load(void)
{
  eeprom_size = Eeprom24_Size(EE24_ADDR_0);
  uint32_t slots = AREA1_SLOTS;
  if (eeprom_size > AREA2_START) slots += (eeprom_size - AREA2_START) / SLOT_SIZE;
  if (eeprom_size == 0u) slots = 0;               /* no EEPROM */
  MsgQueue_Init(&queue, (uint8_t)(slots > MSG_MAX ? MSG_MAX : slots));

  for (int i = 0; i < queue.slot_count; i++)
  {
    Msg *m = &queue.slots[i];
    if (EeRecord_Load(P_SlotAddr(i), m, sizeof *m) != 0 || m->seq == 0u ||
        !MsgQueue_TextValid((m->line1[MSG_LINE_MAX] = '\0', m->line1), 0) ||
        !MsgQueue_TextValid((m->line2[MSG_LINE_MAX] = '\0', m->line2), 1))
    {
      memset(m, 0, sizeof *m);
    }
  }
  MsgQueue_Loaded(&queue);

  MsgConfig c;
  if (EeRecord_Load(EE_RECORD_MSGCFG_ADDR, &c, sizeof c) == 0 && c.order <= MSG_ORDER_RANDOM &&
      c.show_s >= 1u && c.show_s <= 60u && c.gap_s >= 1u && c.gap_s <= 250u)
  {
    cfg = c;
  }
  LOG_I("EEPROM %lu KB, messages %d/%d", (unsigned long)(eeprom_size / 1024u),
        MsgQueue_Count(&queue), queue.slot_count);
}

const MsgConfig *MsgStore_Config(void) { return &cfg; }

void MsgStore_SetConfig(const MsgConfig *c)
{
  cfg = *c;
  if (EeRecord_Save(EE_RECORD_MSGCFG_ADDR, &cfg, sizeof cfg) != 0)
  {
    LOG_E("Messages: EEPROM write failed (settings)");
  }
  changes++;
}

int MsgStore_Count(void)          { return MsgQueue_Count(&queue); }
int MsgStore_Capacity(void)       { return queue.slot_count; }
uint32_t MsgStore_EepromSize(void){ return eeprom_size; }
uint32_t MsgStore_Changes(void)   { return changes; }

const Msg *MsgStore_Get(int id)
{
  int slot = id - 1;
  if (slot < 0 || slot >= queue.slot_count || queue.slots[slot].seq == 0u) return NULL;
  return &queue.slots[slot];
}

int MsgStore_Add(const char *line1, const char *line2, uint8_t color)
{
  if (MsgQueue_NeedsRenumber(&queue))
  {
    MsgQueue_Renumber(&queue);
    for (int i = 0; i < queue.slot_count; i++)
    {
      if (queue.slots[i].seq != 0u) P_SaveSlot(i);
    }
  }
  int slot = MsgQueue_Add(&queue, line1, line2, color);
  if (slot < 0) return slot;
  if (EeRecord_Save(P_SlotAddr(slot), &queue.slots[slot], sizeof(Msg)) != 0)
  {
    MsgQueue_Delete(&queue, slot);
    return -3;
  }
  changes++;
  return slot + 1;
}

int MsgStore_Delete(int id)
{
  if (MsgQueue_Delete(&queue, id - 1) != 0) return -1;
  EeRecord_Erase(P_SlotAddr(id - 1));
  changes++;
  return 0;
}

void MsgStore_Clear(void)
{
  for (int i = 0; i < queue.slot_count; i++)
  {
    if (queue.slots[i].seq != 0u)
    {
      MsgQueue_Delete(&queue, i);
      EeRecord_Erase(P_SlotAddr(i));
    }
  }
  changes++;
}

int MsgStore_Sorted(uint8_t *ids)
{
  int n = MsgQueue_Sorted(&queue, ids);
  for (int i = 0; i < n; i++) ids[i]++;
  return n;
}

int MsgStore_Next(void)
{
  uint32_t rnd = 0;
  if (cfg.order == MSG_ORDER_RANDOM) HAL_RNG_GenerateRandomNumber(&hrng, &rnd);
  return MsgQueue_NextIn(&queue, (MsgOrder)cfg.order, rnd, playlist) + 1;
}

int MsgStore_Prev(void)
{
  return MsgQueue_Prev(&queue) + 1;
}

void MsgStore_SetPlaylist(uint32_t mask)
{
  playlist = mask;
}

void MsgStore_SetCurrent(int id)
{
  if (MsgStore_Get(id) != NULL) queue.current = (int8_t)(id - 1);
}
