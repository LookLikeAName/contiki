#ifndef __ORCHESTRA_GROUPED_HANDLER_H__
#define __ORCHESTRA_GROUPED_HANDLER_H__


struct group_attribute_s{
  uint16_t required_slot;
  uint16_t allocate_slot_offset;
}group_attribute_default={1,0};

typedef struct group_attribute_s group_attribute;

void group_handler_init();

void group_handler_new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new);
int group_handler_is_time_source(const linkaddr_t *linkaddr);

uint8_t group_handler_get_request_slots_for_root(linkaddr_t *dest);
void group_handler_set_requested_slots_frome_child(uint8_t requested_slots_frome_child);
uint8_t group_handler_get_requested_slots_frome_child();

int group_handler_is_slot_for_parent(const struct tsch_link *link);
int group_handler_is_packet_for_parent(struct queuebuf *buf);

void group_handler_request_slot_routine(uint16_t used_slot);
void group_handler_slot_request_acked();

uint8_t group_handler_get_request_slots_for_root(linkaddr_t *dest);
void group_handler_set_requested_slots_frome_child(uint8_t requested_slots_frome_child);

uint16_t group_handler_get_slot_required_slot(int group_offset);
uint16_t group_handler_get_allocate_slot_offset(int group_offset);

void group_handler_set_slot_required_slot(int group_offset,uint16_t option);
void group_handler_set_allocate_slot_offset(int group_offset,uint16_t option);

#endif /*__ORCHESTRA_GROUPED_HANDLER_H__*/