#include "orchestra.h"
#include "orchestra-grouped-handler.h"


/*Request slots amount for root 4 bits( 1 ~ 15 ) which actually mean 1~15 slots to allocate,reserved 0 for not sendeing to parent.*/
uint8_t orchestra_request_slots_for_root=0;
uint8_t orchestra_requested_slots_from_child=0;

struct group_attribute_s{
    uint16_t required_slot;
    uint16_t allocate_slot_offset;
  }group_attribute_default={1,0};
  
typedef struct group_attribute_s group_attribute;


group_attribute groups[ORCHESTRA_SLOTFRAME_GROUP_AMOUNT];


/*----------------------------------------------------------------------*/
static uint16_t
get_group_offset(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_SLOTFRAME_GROUP_AMOUNT > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_SLOTFRAME_GROUP_AMOUNT;
  } else {
    return 0xffff;
  }
}

/*----------------------------------------------------------------------*/
/*-------------------------EXTERNAL FUNCTIONS--------------------------*/
/*----------------------------------------------------------------------*/
void group_handler_init()
{
    int i;
    /*Initial groups attribute*/
    for (i = 0; i < ORCHESTRA_SLOTFRAME_GROUP_AMOUNT; i++)
    {
        groups[i] = group_attribute_default;
    }
}
/*----------------------------------------------------------------------*/

void group_handler_new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
    if (new != old)
    {
        const linkaddr_t *old_addr = old != NULL ? &old->addr : NULL;
        const linkaddr_t *new_addr = new != NULL ? &new->addr : NULL;
        if (old_addr != NULL)
        {
            groups[get_group_offset(old_addr)] = group_attribute_default;
        }
        if (new_addr != NULL)
        {
            linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
        }
        else
        {
            linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
        }
    }
}
/*----------------------------------------------------------------------*/
int group_handler_is_time_source(const linkaddr_t *linkaddr)
{
    //PRINTF("parent null %d\n",linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null));
    //PRINTF("linkaddr null %d\n",linkaddr_cmp(linkaddr, &linkaddr_null));
    if (linkaddr != NULL && !linkaddr_cmp(linkaddr, &linkaddr_null))
    {
        if (linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr))
        {
            //PRINTF("group_handler_is_time_source 1 \n");
            return 1;
        }
    }
    //PRINTF("group_handler_is_time_source 0 \n");
    return 0;
}
/*----------------------------------------------------------------------*/
int group_handler_is_packet_for_parent(struct queuebuf *buf){
    
    const linkaddr_t *dest = queuebuf_addr(buf,PACKETBUF_ADDR_RECEIVER);
    return group_handler_is_time_source(dest);
}
/*----------------------------------------------------------------------*/

void group_handler_request_slot_routine(uint16_t used_slot){
    uint16_t parent_group_offset;
    parent_group_offset =get_group_offset(&orchestra_parent_linkaddr);
    if(used_slot>ADD_THRESHOLD)
    {
      orchestra_request_slots_for_root = groups[parent_group_offset].required_slot+1 > ORCHESTRA_SLOTFRAME_GROUP_SIZE ? ORCHESTRA_SLOTFRAME_GROUP_SIZE : groups[parent_group_offset].required_slot+1;
      PRINTF("ADD_THRESHOLD %d , used %d\n",orchestra_request_slots_for_root,used_slot);
    }
    else if(used_slot<DELETE_THRESHOLD)
    {
      orchestra_request_slots_for_root = ((groups[parent_group_offset].required_slot-1) < 1) ? 1 : groups[parent_group_offset].required_slot-1;
      PRINTF("DELETE_THRESHOLD %d , used %d\n",orchestra_request_slots_for_root,used_slot);
    }
    PRINTF("request_slot_routine %d , used %d\n",orchestra_request_slots_for_root,used_slot);
}
/*----------------------------------------------------------------------*/
void group_handler_slot_request_acked(){
    if(orchestra_request_slots_for_root!=0){
        uint16_t parent_group_offset;
        parent_group_offset = get_group_offset(&orchestra_parent_linkaddr);
        if( groups[parent_group_offset].required_slot != orchestra_request_slots_for_root){ 
          groups[parent_group_offset].required_slot = orchestra_request_slots_for_root;
        }
       // PRINTF("slot_request_acked %d\n", groups[parent_group_offset].required_slot);
      }
}
/*----------------------------------------------------------------------*/
uint8_t group_handler_get_request_slots_for_root(linkaddr_t *dest){
    if(!linkaddr_cmp(dest, &linkaddr_null) && group_handler_is_time_source(dest)) {
        PRINTF("get_request_slots_for_root is parent %d\n", orchestra_request_slots_for_root);
      return orchestra_request_slots_for_root;
    }
    return 0;
}
/*----------------------------------------------------------------------*/
void group_handler_set_requested_slots_frome_child(uint8_t requested_slots_frome_child){
    orchestra_requested_slots_from_child = requested_slots_frome_child;
}
/*----------------------------------------------------------------------*/
uint8_t group_handler_get_requested_slots_frome_child(){
    return orchestra_requested_slots_from_child;
}
/*----------------------------------------------------------------------*/
uint16_t group_handler_get_slot_required_slot(int group_offset){
    return groups[group_offset].required_slot;
}

uint16_t group_handler_get_allocate_slot_offset(int group_offset){
    return groups[group_offset].allocate_slot_offset;
}

void group_handler_set_slot_required_slot(int group_offset,uint16_t option){
    groups[group_offset].required_slot = option;
}

void group_handler_set_allocate_slot_offset(int group_offset,uint16_t option){
    groups[group_offset].allocate_slot_offset = option;
}
/*----------------------------------------------------------------------*/

