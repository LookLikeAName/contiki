#ifndef __ORCHESTRA_GROUPED_HANDLER_H__
#define __ORCHESTRA_GROUPED_HANDLER_H__

/*Request slots amount for root 4 bits( 1 ~ 15 ) which actually mean 1~15 slots to allocate,reserved 0 for not sendeing to parent.*/
uint8_t orchestra_request_slots_for_root=0;
uint8_t orchestra_requested_slots_from_child=0;

static uint16_t packet_countdown = 10;


struct group_attribute_s{
  uint16_t required_slot;
  uint16_t allocate_slot_offset;
}group_attribute_default={1,0};

typedef struct group_attribute_s group_attribute;

group_attribute groups[ORCHESTRA_SLOTFRAME_GROUP_AMOUNT];

void group_handler_init();

static void new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new);
static int is_time_source(const linkaddr_t *linkaddr);

uint8_t get_request_slots_for_root(linkaddr_t *dest);
void set_requested_slots_frome_child(uint8_t requested_slots_frome_child);

static int is_slot_for_parent(const struct tsch_link *link);
int is_packet_for_parent(struct queuebuf *buf);

void request_slot_routine(uint16_t used_slot);
void slot_request_acked();

uint8_t get_request_slots_for_root(linkaddr_t *dest);
void set_requested_slots_frome_child(uint8_t requested_slots_frome_child);

uint16_t get_slot_required_slot(int group_offset);
uint16_t get_allocate_slot_offset(int group_offset);

void set_slot_required_slot(int group_offset,uint16_t option);
void set_allocate_slot_offset(int group_offset,uint16_t option);

#endif /*__ORCHESTRA_GROUPED_HANDLER_H__*/