/*
 * Copyright (c) 2016, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         Orchestra: a slotframe dedicated to unicast data transmission. Designed primarily
 *         for RPL non-storing mode but would work with any mode-of-operation. Does not require
 *         any knowledge of the children. Works only as received-base, and as follows:
 *           Nodes listen at a timeslot defined as hash(MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *           Nodes transmit at: for any neighbor, hash(nbr.MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "contiki.h"
#include "orchestra.h"
#include "orchestra-grouped-handler.h"
#include "net/ip/uip.h"
#include "net/packetbuf.h"
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define UNICAST_SLOTFRAME_ID 1

static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;

static struct tsch_slotframe *sf_unicast_group_1;

static uint16_t packet_countdown = 10;
static uint16_t last_rx_count = 0 ;


/*---------------------------------------------------------------------------*/
static uint16_t
get_group_offset(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_SLOTFRAME_GROUP_AMOUNT > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_SLOTFRAME_GROUP_AMOUNT;
  } else {
    return 0xffff;
  }
}

static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_GROUPED_UNICAST_PERIOD > 0) {
    uint16_t group_offset;
    group_offset=get_group_offset(addr);
    if(group_offset==0xffff){
      return 0xffff;
    }
    return group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE+group_handler_get_allocate_slot_offset(group_offset);
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static void
add_self_uc_link(uint8_t requested_slots)
{
  struct tsch_link *l;
  uint16_t node_group_offset;
  uint16_t add_first_slot_offset;
  int i;
  node_group_offset=get_group_offset(&linkaddr_node_addr);
  int add_count = requested_slots-group_handler_get_slot_required_slot(node_group_offset);
  add_first_slot_offset=node_group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE+group_handler_get_slot_required_slot(node_group_offset)-1;
  PRINTF("add link: %d %d %d\n",add_count,add_first_slot_offset,requested_slots);  
  /* Add/update link */
    for(i = 0; i < add_count; i++) {
      add_first_slot_offset++;
     l = tsch_schedule_add_link(sf_unicast_group_1,
          LINK_OPTION_SHARED | LINK_OPTION_TX | LINK_OPTION_RX,
          LINK_TYPE_NORMAL, &tsch_broadcast_address,
          add_first_slot_offset, channel_offset);
      PRINTF("add link loop: %d %d %d\n",i,add_first_slot_offset,l==NULL?0:l->link_options);  
      
    }
    group_handler_set_slot_required_slot(node_group_offset,requested_slots);
}
/*---------------------------------------------------------------------------*/
static void
delete_self_uc_link(uint8_t requested_slots)
{

  struct tsch_link *l;
  uint16_t node_group_offset;
  uint16_t delete_slot_offset;
  int i;
  node_group_offset=get_group_offset(&linkaddr_node_addr);
  int delete_count = group_handler_get_slot_required_slot(node_group_offset) - requested_slots;
  delete_slot_offset=node_group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE+ group_handler_get_slot_required_slot(node_group_offset)-1;
  PRINTF("delete link: %d %d\n",delete_count,delete_slot_offset);  
  /* update link */
    for(i = 0; i < delete_count; i++) {

      l =  tsch_schedule_add_link(sf_unicast_group_1,
        LINK_OPTION_SHARED | LINK_OPTION_TX,
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        delete_slot_offset, channel_offset);
        PRINTF("delete link loop: %d %d %d\n",i,delete_slot_offset,l==NULL?0:l->link_options);  
      delete_slot_offset--;
    }
    group_handler_set_slot_required_slot(node_group_offset,requested_slots);
}
/*---------------------------------------------------------------------------*/
static int is_for_this_slotframe(const linkaddr_t *linkaddr){
#if ORCHESTRA_GROUPED_MULTICHANNEL_ENABLE
  int dest_slotframe_id = ((int)ORCHESTRA_LINKADDR_HASH(linkaddr)/ORCHESTRA_SLOTFRAME_GROUP_AMOUNT)%ORCHESTRA_GROUPED_MULTICHANNEL_NUMBER;
  PRINTF("is_for_this_slotframe 1,%d ,%d \n",dest_slotframe_id,UNICAST_SLOTFRAME_ID);
  if(dest_slotframe_id == UNICAST_SLOTFRAME_ID){
    return 1;
  }
  return 0;
 #else
  return 1;
 #endif
}
/*---------------------------------------------------------------------------*/
static int
is_time_source(const linkaddr_t *linkaddr)
{
  //PRINTF("parent null %d\n",linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null));  
  //PRINTF("linkaddr null %d\n",linkaddr_cmp(linkaddr, &linkaddr_null));   
  if(linkaddr != NULL && !linkaddr_cmp(linkaddr, &linkaddr_null)) {
      if(linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr)) {
        //PRINTF("is_time_source 1 \n");
        return 1;
      }  
    }
  //PRINTF("is_time_source 0 \n");  
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
child_added(const linkaddr_t *linkaddr)
{
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot)
{
  /* Select data packets we have a unicast link to */
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  uint16_t dest_group_offset = get_group_offset(dest);
  PRINTF ("select_packet 1 ,%d ,%d , %d\n",is_for_this_slotframe(dest),dest_group_offset,ORCHESTRA_LINKADDR_HASH(dest));
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && !linkaddr_cmp(dest, &linkaddr_null)
     && is_for_this_slotframe(dest)) {
      PRINTF("group slotframe_1 \n");
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      if(is_time_source(dest)){
        *timeslot = get_node_timeslot(dest);
        group_handler_set_allocate_slot_offset(dest_group_offset,(group_handler_get_allocate_slot_offset(dest_group_offset)+1)%group_handler_get_slot_required_slot(dest_group_offset));
      }
      else
      {
        *timeslot = get_node_timeslot(dest)-group_handler_get_allocate_slot_offset(dest_group_offset);
      }
    }
    //PRINTF("PACKETBUF_ATTR_TSCH_SLOTFRAME: %02x,PACKETBUF_ATTR_TSCH_TIMESLOT: %02x\n",*slotframe,*timeslot);
    
   
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
#if TSCH_CALLBACK_NOACK_CONF 
static int
packet_noack(uint16_t *slotframe,uint16_t *timeslot,struct queuebuf *buf)
{
   // PRINTF("orchestra NOACK\n");
  /* Select data packets we have a unicast link to */
  const linkaddr_t *dest = queuebuf_addr(buf,PACKETBUF_ADDR_RECEIVER);
  uint16_t dest_group_offset = get_group_offset(dest);

  if(((((uint8_t *)(queuebuf_dataptr(buf)))[0]) & 7) == FRAME802154_DATAFRAME
     && !linkaddr_cmp(dest, &linkaddr_null)
     && is_for_this_slotframe(dest)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      if(is_time_source(dest)){
        *timeslot = get_node_timeslot(dest);
        //int backoff_slots=(groups[get_group_offset(dest)].required_slot-1) > 0 ? ORCHESTRA_LINKADDR_HASH(&linkaddr_node_addr) % (groups[get_group_offset(dest)].required_slot-1)+1 : 1 ;
        int backoff_slots =  1;
  
        group_handler_set_allocate_slot_offset(dest_group_offset,(group_handler_get_allocate_slot_offset(dest_group_offset)+backoff_slots)%group_handler_get_slot_required_slot(dest_group_offset));
      }
      else
      {
        *timeslot = get_node_timeslot(dest)-group_handler_get_allocate_slot_offset(dest_group_offset);
      }
    }
    //PRINTF("orchestra NOACK true\n");
    return 1;
  }
  //PRINTF("orchestra NOACK false\n");
  return 0;
}
#endif
/*---------------------------------------------------------------------------*/
#if TSCH_CALLBACK_GROUPED_NESS_CONF
static void
slot_allocate_routine()
{
  uint16_t node_group_offset;
  int i;
  node_group_offset=get_group_offset(&linkaddr_node_addr);
  uint8_t requested_slots_from_child = group_handler_get_requested_slots_frome_child();

  if(is_for_this_slotframe(&linkaddr_node_addr)){
  if(requested_slots_from_child<=ORCHESTRA_SLOTFRAME_GROUP_SIZE && requested_slots_from_child > 0){
   if(requested_slots_from_child > group_handler_get_slot_required_slot(node_group_offset)){
      packet_countdown = 10;
      add_self_uc_link(requested_slots_from_child);
    }
    else if(requested_slots_from_child == group_handler_get_slot_required_slot(node_group_offset))
    {
      packet_countdown = 10;
    }
    else
    {
      packet_countdown--;
      if(packet_countdown == 0){
        packet_countdown = 10;
        delete_self_uc_link(requested_slots_from_child);
      }
    }
  }
  PRINTF("groups[node_group_offset].required_slot: %02x , %d ,count down %d\n",group_handler_get_slot_required_slot(node_group_offset),requested_slots_from_child,packet_countdown);
  group_handler_set_requested_slots_frome_child(0);
}
}

/*----------------------------------------------------------------------*/
static int is_slot_for_parent(const struct tsch_link *link){
  uint16_t parent_slot_offset_start;
  uint16_t parent_group_offset;

  parent_group_offset = get_group_offset(&orchestra_parent_linkaddr);
  parent_slot_offset_start = parent_group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE;
  
  if(link->slotframe_handle == slotframe_handle){ 
    
      if(parent_slot_offset_start <= link->timeslot &&
         link->timeslot <  parent_slot_offset_start+group_handler_get_slot_required_slot(parent_group_offset))
        {
         // PRINTF("link for parent :%d %d %d %d\n",link->slotframe_handle,link->timeslot,parent_slot_offset_start,groups[group_offset].required_slot);
          return 1;
        }
  }
  return 0;
}

static void rx_use_count(const struct tsch_link *link,uint8_t packet_receved,int frame_valid){
  uint16_t node_slot_offset_start;
  uint16_t node_group_offset;

  node_group_offset =get_group_offset(&linkaddr_node_addr);
  node_slot_offset_start = node_group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE;
  
  if(link->slotframe_handle == slotframe_handle 
    && link->timeslot == node_slot_offset_start+(group_handler_get_slot_required_slot(node_group_offset)-1))
    { 
   
   if(packet_receved && frame_valid)
   {
    last_rx_count ++;
   }
  // PRINTF("self_rx_maintain: %d , %d ,%d b\n",last_rx_count,packet_receved,frame_valid);
  }
}

static void rx_maintain_routine(){
  
  uint16_t node_group_offset;

  node_group_offset =get_group_offset(&linkaddr_node_addr);
 // PRINTF("rx_maintain_routine: %d \n",last_rx_count);
  if((group_handler_get_slot_required_slot(node_group_offset) -1)>0 && last_rx_count == 0 ){
    delete_self_uc_link(group_handler_get_slot_required_slot(node_group_offset) -1 );
  }
  last_rx_count = 0;
}
#endif
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{

}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  PRINTF("init orchestra non-storing-grouped-slotframe %d\n",last_rx_count);
  int i;
  uint16_t rx_timeslot;
  slotframe_handle = sf_handle;
  channel_offset = sf_handle;

  /* Slotframe for unicast transmissions */
  sf_unicast_group_1 = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_GROUPED_UNICAST_PERIOD);
  rx_timeslot = get_node_timeslot(&linkaddr_node_addr);
  /* Add a Tx link at each available timeslot. Make the link Rx at our own timeslot. */
  for(i = 0; i < ORCHESTRA_GROUPED_UNICAST_PERIOD; i++) {
    tsch_schedule_add_link(sf_unicast_group_1,
        LINK_OPTION_SHARED | LINK_OPTION_TX | (((i >= rx_timeslot) && (i < rx_timeslot+1) && is_for_this_slotframe(&linkaddr_node_addr)) ? LINK_OPTION_RX : 0 ),
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        i, channel_offset);
  }
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_rpl_ns_grouped_slotframe_1 = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  packet_noack,
  slot_allocate_routine,
  is_slot_for_parent,
  rx_use_count,
  rx_maintain_routine,
};
