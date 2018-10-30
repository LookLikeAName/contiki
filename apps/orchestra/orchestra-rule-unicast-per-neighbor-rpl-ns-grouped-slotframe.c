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

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;
static struct tsch_slotframe *sf_unicast;

struct group_attribute_s{
  uint16_t required_slot;
  uint16_t allocate_slot_offset;
}group_attribute_default={2,0};

typedef struct group_attribute_s group_attribute;

group_attribute groups[ORCHESTRA_CONF_SLOTFRAME_GROUP_AMOUNT];

/*---------------------------------------------------------------------------*/
uint16_t
get_group_offset(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_CONF_SLOTFRAME_GROUP_AMOUNT > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_CONF_SLOTFRAME_GROUP_AMOUNT;
  } else {
    return 0xffff;
  }
}

static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
    uint16_t group_offset;
    group_offset=get_group_offset(addr);
    if(group_offset==0xffff){
      return 0xffff;
    }
    return group_offset*ORCHESTRA_CONF_SLOTFRAME_GROUP_SIZE+groups[group_offset].allocate_slot_offset;
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static void
add_uc_rx_link(const linkaddr_t *linkaddr)
{

}
/*---------------------------------------------------------------------------*/
static void
remove_uc_rx_link(const linkaddr_t *linkaddr)
{

}
/*---------------------------------------------------------------------------*/
static void
slot_allocate_routine(const linkaddr_t *linkaddr)
{
  PRINTF("Rule ns grouped slotframe requested slots: %02x \n",orchestra_requested_slots_frome_child);
  uint16_t node_group_offset;
  node_group_offset=get_group_offset(&linkaddr_node_addr);
  if(orchestra_requested_slots_frome_child>=groups[node_group_offset].required_slot){

  }
  else
  {

  }
  PRINTF("Rule ns grouped slotframe request slots: %02x \n",orchestra_request_slots_for_root);
  
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
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && !linkaddr_cmp(dest, &linkaddr_null)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      *timeslot = get_node_timeslot(dest);
      groups[get_group_offset(dest)].allocate_slot_offset=(groups[get_group_offset(dest)].allocate_slot_offset+1)%groups[get_group_offset(dest)].required_slot;
    }
    PRINTF("PACKETBUF_ATTR_TSCH_SLOTFRAME: %02x,PACKETBUF_ATTR_TSCH_TIMESLOT: %02x\n",*slotframe,*timeslot);
    slot_allocate_routine(dest);
   
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
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
  for(i=0;i<ORCHESTRA_CONF_SLOTFRAME_GROUP_AMOUNT;i++){
    groups[i]=group_attribute_default;
  }
  /* Slotframe for unicast transmissions */
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
  rx_timeslot = get_node_timeslot(&linkaddr_node_addr);
  /* Add a Tx link at each available timeslot. Make the link Rx at our own timeslot. */
  for(i = 0; i < ORCHESTRA_UNICAST_PERIOD; i++) {
    tsch_schedule_add_link(sf_unicast,
        LINK_OPTION_SHARED | LINK_OPTION_TX | (((i >= rx_timeslot) && (i < rx_timeslot+ORCHESTRA_CONF_SLOTFRAME_GROUP_SIZE)) ? LINK_OPTION_RX : 0 ),
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        i, channel_offset);
        /*if(i == rx_timeslot){
          i+=2;
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
};
