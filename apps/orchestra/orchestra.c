/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 *         Orchestra: an autonomous scheduler for TSCH exploiting RPL state.
 *         See "Orchestra: Robust Mesh Networks Through Autonomously Scheduled TSCH", ACM SenSys'15
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/rpl/rpl-private.h"
#include "net/rime/rime.h" /* Needed for so-called rime-sniffer */

#define DEBUG DEBUG_PRINT
#include "net/ip/uip-debug.h"

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* A net-layer sniffer for packets sent and received */
static void orchestra_packet_received(void);
static void orchestra_packet_sent(int mac_status);
RIME_SNIFFER(orchestra_sniffer, orchestra_packet_received, orchestra_packet_sent);

/* The current RPL preferred parent's link-layer address */
linkaddr_t orchestra_parent_linkaddr;
/* Set to one only after getting an ACK for a DAO sent to our preferred parent */
int orchestra_parent_knows_us = 0;

/* The set of Orchestra rules in use */
const struct orchestra_rule *all_rules[] = ORCHESTRA_RULES;
#define NUM_RULES (sizeof(all_rules) / sizeof(struct orchestra_rule *))

/*---------------------------------------------------------------------------*/
static void
orchestra_packet_received(void)
{
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_sent(int mac_status)
{
  /* Check if our parent just ACKed a DAO */
  if(orchestra_parent_knows_us == 0
     && mac_status == MAC_TX_OK
     && packetbuf_attr(PACKETBUF_ATTR_NETWORK_ID) == UIP_PROTO_ICMP6
     && packetbuf_attr(PACKETBUF_ATTR_CHANNEL) == (ICMP6_RPL << 8 | RPL_CODE_DAO)) {
    if(!linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null)
       && linkaddr_cmp(&orchestra_parent_linkaddr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))) {
      orchestra_parent_knows_us = 1;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_child_added(const linkaddr_t *addr)
{
  /* Notify all Orchestra rules that a child was added */
  int i;
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->child_added != NULL) {
      all_rules[i]->child_added(addr);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_child_removed(const linkaddr_t *addr)
{
  /* Notify all Orchestra rules that a child was removed */
  int i;
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->child_removed != NULL) {
      all_rules[i]->child_removed(addr);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_packet_ready(void)
{
  int i;
  /* By default, use any slotframe, any timeslot */
  uint16_t slotframe = 9;
  uint16_t timeslot = 0xffff;

  /* Loop over all rules until finding one able to handle the packet */
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->select_packet != NULL) {
      if(all_rules[i]->select_packet(&slotframe, &timeslot)) {
        break;
      }
    }
  }

#if TSCH_WITH_LINK_SELECTOR
  packetbuf_set_attr(PACKETBUF_ATTR_TSCH_SLOTFRAME, slotframe);
  packetbuf_set_attr(PACKETBUF_ATTR_TSCH_TIMESLOT, timeslot);
#endif
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  /* Orchestra assumes that the time source is also the RPL parent.
   * This is the case if the following is set:
   * #define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
   * */

  int i;
  if(new != old) {
    orchestra_parent_knows_us = 0;
  }
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->new_time_source != NULL) {
      all_rules[i]->new_time_source(old, new);
    }
  }
}
/*---------------------------------------------------------------------------*/
#if TSCH_CALLBACK_NOACK_CONF 
void
 orchestra_callback_tsch_noack(struct queuebuf *buf)
 {
  int i;
  /* By default, use any slotframe, any timeslot */
  uint16_t slotframe;
  uint16_t timeslot;
  //PRINTF("orchestra_callback_tsch_noack\n");
  /* Loop over all rules until finding one able to handle the packet */
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->packet_noack != NULL) {
      if(all_rules[i]->packet_noack(&slotframe, &timeslot,buf)) {
        //PRINTF("orchestra_callback_tsch_noack excute\n");
        #if TSCH_WITH_LINK_SELECTOR
         queuebuf_set_attr(buf,PACKETBUF_ATTR_TSCH_SLOTFRAME, slotframe);
         queuebuf_set_attr(buf,PACKETBUF_ATTR_TSCH_TIMESLOT, timeslot);
        #endif
        break;
      }
    }
  }

 }
 #endif
/*---------------------------------------------------------------------------*/
 #if TSCH_CALLBACK_GROUPED_NESS_CONF
 PROCESS(orchestra_process, "orchestra_process");


 int
 orchestra_callback_is_slot_for_parent(const struct tsch_link *link)
 {
  
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->is_slot_for_parent != NULL) {
        if(all_rules[i]->is_slot_for_parent(link)){
          return 1;
        }
     }
   }
   return 0;
 }

 int
 orchestra_callback_is_packet_for_parent(struct queuebuf *buf)
 {
  
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->is_packet_for_parent != NULL) {
        if(all_rules[i]->is_packet_for_parent(buf)){
          return 1;
        }
     }
   }
   return 0;
 }
 
 void
 orchestra_callback_request_slot_routine(uint16_t used_slot)
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->request_slot_routine != NULL) {
        all_rules[i]->request_slot_routine(used_slot);
     }
   }
 }

 void
 orchestra_callback_slot_request_acked()
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->slot_request_acked != NULL) {
        all_rules[i]->slot_request_acked();
     }
   }
 }

 uint8_t
 orchestra_callback_get_request_slots_for_root(linkaddr_t *dest)
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->get_request_slots_for_root != NULL) {
        return all_rules[i]->get_request_slots_for_root(dest);
     }
   }
 }

 void
 orchestra_callback_set_requested_slots_from_child(uint16_t requested_slots_frome_child)
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->set_requested_slots_from_child != NULL) {
        all_rules[i]->set_requested_slots_from_child(requested_slots_frome_child);
     }
   }
 }

 void
 orchestra_callback_slot_allocate_routine() 
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->slot_allocate_routine != NULL) {
        all_rules[i]->slot_allocate_routine();
     }
   }
 }

 void
 orchestra_callback_rx_use_count(const struct tsch_link *link,uint8_t packet_receved,int frame_valid) 
 {
   int i;
   for(i = 0; i < NUM_RULES; i++) {
     if(all_rules[i]->rx_use_count != NULL) {
        all_rules[i]->rx_use_count(link,packet_receved,frame_valid);
     }
   }
 }

 
PROCESS_THREAD(orchestra_process, ev, data)
{
  static struct etimer etaa;
  

  PROCESS_BEGIN();
  
  //etimer_set(&etaa, CLOCK_SECOND * 60);
  etimer_set(&etaa, CLOCK_SECOND * ORCHESTRA_LAST_RX_UNUESD_DELETE_THRESHOLD);
  while(1) {
    PROCESS_YIELD_UNTIL(etimer_expired(&etaa));
    etimer_reset(&etaa);
    int i;
    for(i = 0; i < NUM_RULES; i++) {
      if(all_rules[i]->rx_maintain_routine != NULL) {
         all_rules[i]->rx_maintain_routine(link,packet_receved,frame_valid);
      }
    }
  }

  PROCESS_END();
}

 #endif
/*---------------------------------------------------------------------------*/
void
orchestra_init(void)
{
  int i;
  /* Snoop on packet transmission to know if our parent knows about us
   * (i.e. has ACKed at one of our DAOs since we decided to use it as a parent) */
  rime_sniffer_add(&orchestra_sniffer);
  linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
  /* Initialize all Orchestra rules */
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->init != NULL) {
      PRINTF("Orchestra: initializing rule %u\n", i);
      all_rules[i]->init(i);
    }
  }
  PRINTF("Orchestra: initialization done\n");
  process_start(&orchestra_process, NULL);
}
