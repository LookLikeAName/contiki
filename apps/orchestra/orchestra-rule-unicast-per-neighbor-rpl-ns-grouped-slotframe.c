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
#include "net/ip/uip.h"
#include "net/packetbuf.h"
#include <stdio.h>

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;
static struct tsch_slotframe *sf_unicast;
static uint16_t packet_countdown = 10;

extern uint8_t orchestra_request_slots_for_root;
extern uint8_t orchestra_requested_slots_frome_child;

struct group_attribute_s{
  uint16_t required_slot;
  uint16_t allocate_slot_offset;
}group_attribute_default={3,0};

typedef struct group_attribute_s group_attribute;

group_attribute groups[ORCHESTRA_SLOTFRAME_GROUP_AMOUNT];

/*---------------------------------------------------------------------------*/
uint16_t
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
    return group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE+groups[group_offset].allocate_slot_offset;
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static void
slot_allocate_routine(const linkaddr_t *linkaddr)
{
  //PRINTF("Rule ns grouped slotframe requested slots: %02x \n",orchestra_requested_slots_frome_child);
  uint16_t node_group_offset;
  int i;
  node_group_offset=get_group_offset(&linkaddr_node_addr);
  if(orchestra_requested_slots_frome_child<ORCHESTRA_SLOTFRAME_GROUP_SIZE && orchestra_requested_slots_frome_child > 0){
   if(orchestra_requested_slots_frome_child>=groups[node_group_offset].required_slot){
      packet_countdown = 10;
      groups[node_group_offset].required_slot+=(orchestra_requested_slots_frome_child-groups[node_group_offset].required_slot);
    }
    else
    {
      packet_countdown--;
      if(packet_countdown == 0){
        packet_countdown = 10;
        groups[node_group_offset].required_slot-=(groups[node_group_offset].required_slot-orchestra_requested_slots_frome_child);
      }
    }
  }
 // PRINTF("Rule ns grouped slotframe request slots: %02x \n",orchestra_request_slots_for_root);
  
}
/*---------------------------------------------------------------------------*/
static int
is_time_source(const linkaddr_t *linkaddr)
{
     if(linkaddr != NULL && !linkaddr_cmp(linkaddr, &linkaddr_null)) {
      PRINTF("parent null %d\n",linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null));
      if(linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr)) {
        PRINTF("is_time_source 1 \n");
        return 1;
      }  
    }
  PRINTF("is_time_source 0 \n");  
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
  slot_allocate_routine(dest);
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && !linkaddr_cmp(dest, &linkaddr_null)) {
      PRINTF("group slotframe \n");
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      if(is_time_source(dest)){
        *timeslot = get_node_timeslot(dest);
        groups[get_group_offset(dest)].allocate_slot_offset=(groups[get_group_offset(dest)].allocate_slot_offset+1)%groups[get_group_offset(dest)].required_slot;
      }
      else
      {
        *timeslot = get_node_timeslot(dest)-groups[get_group_offset(dest)].allocate_slot_offset;
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
    PRINTF("orchestra NOACK\n");
  /* Select data packets we have a unicast link to */
  const linkaddr_t *dest = queuebuf_addr(buf,PACKETBUF_ADDR_RECEIVER);
  if(((((uint8_t *)(queuebuf_dataptr(buf)))[0]) & 7) == FRAME802154_DATAFRAME
     && !linkaddr_cmp(dest, &linkaddr_null)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      if(is_time_source(dest)){
        *timeslot = get_node_timeslot(dest);
        groups[get_group_offset(dest)].allocate_slot_offset=(groups[get_group_offset(dest)].allocate_slot_offset+1)%groups[get_group_offset(dest)].required_slot;
      }
      else
      {
        *timeslot = get_node_timeslot(dest)-groups[get_group_offset(dest)].allocate_slot_offset;
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
int is_slot_for_parent(const struct tsch_link *link){
  uint16_t parent_slot_offset_start;
  uint16_t parent_group_offset;

  parent_group_offset =get_group_offset(&orchestra_parent_linkaddr);
  parent_slot_offset_start = parent_group_offset*ORCHESTRA_SLOTFRAME_GROUP_SIZE;
  
  if(link->slotframe_handle == slotframe_handle && parent_group_offset != 0){ 
    
      if(parent_slot_offset_start <= link->timeslot &&
         link->timeslot <  parent_slot_offset_start+groups[parent_group_offset].required_slot)
        {
         // PRINTF("link for parent :%d %d %d %d\n",link->slotframe_handle,link->timeslot,parent_slot_offset_start,groups[group_offset].required_slot);
          return 1;
        }
  }
  return 0;
}

int is_packet_for_parent(struct queuebuf *buf){
  PRINTF("is_packet_for_parent \n");
  const linkaddr_t *dest = queuebuf_addr(buf,PACKETBUF_ADDR_RECEIVER);
  return is_time_source(dest);
}

void request_slot_routine(uint16_t used_slot){
  uint16_t parent_group_offset;
  parent_group_offset =get_group_offset(&orchestra_parent_linkaddr);
  if(used_slot>ADD_THRESHOLD)
  {
    orchestra_request_slots_for_root = groups[parent_group_offset].required_slot+1 > ORCHESTRA_SLOTFRAME_GROUP_SIZE ? 0 : groups[parent_group_offset].required_slot+1;
  }else if(used_slot<DELETE_THRESHOLD)
  {
    orchestra_request_slots_for_root = groups[parent_group_offset].required_slot-1 < 1 ? 0 : groups[parent_group_offset].required_slot-1;
  }
  else
  {
    orchestra_request_slots_for_root = 0;
  }
  PRINTF("request_slot_routine %d\n",orchestra_request_slots_for_root);
}

void slot_request_acked(){
  if(orchestra_request_slots_for_root!=0){
    uint16_t parent_group_offset;
    parent_group_offset = get_group_offset(&orchestra_parent_linkaddr);
    groups[parent_group_offset].required_slot = orchestra_request_slots_for_root;
    orchestra_request_slots_for_root = 0;
    PRINTF("slot_request_acked %d\n", groups[parent_group_offset].required_slot);
  }
}
#endif
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  if(new != old) {
    const linkaddr_t *old_addr = old != NULL ? &old->addr : NULL;
    const linkaddr_t *new_addr = new != NULL ? &new->addr : NULL;
    if(old_addr!= NULL){
      groups[get_group_offset(old_addr)]=group_attribute_default;
    }
    if(new_addr != NULL) {
      linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
    } else {
      linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  PRINTF("init orchestra non-storing-grouped-slotframe\n");
  int i;
  uint16_t rx_timeslot;
  slotframe_handle = sf_handle;
  channel_offset = sf_handle;
  // Debug testing field
  orchestra_request_slots_for_root = get_group_offset(&linkaddr_node_addr);

  /*Initial groups attribute*/
  for(i=0;i<ORCHESTRA_SLOTFRAME_GROUP_AMOUNT;i++){
    groups[i]=group_attribute_default;
  }
  /* Slotframe for unicast transmissions */
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_GROUPED_UNICAST_PERIOD);
  rx_timeslot = get_node_timeslot(&linkaddr_node_addr);
  /* Add a Tx link at each available timeslot. Make the link Rx at our own timeslot. */
  for(i = 0; i < ORCHESTRA_GROUPED_UNICAST_PERIOD; i++) {
    tsch_schedule_add_link(sf_unicast,
        LINK_OPTION_SHARED | LINK_OPTION_TX | (((i >= rx_timeslot) && (i < rx_timeslot+ORCHESTRA_SLOTFRAME_GROUP_SIZE)) ? LINK_OPTION_RX : 0 ),
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        i, channel_offset);
        /*if(i == rx_timeslot){
          i+=(ORCHESTRA_SLOTFRAME_GROUP_SIZE-1);
        }*/
  }
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_rpl_ns_grouped_slotframe = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  packet_noack,
  is_slot_for_parent,
  is_packet_for_parent,
};
