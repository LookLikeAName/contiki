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
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#ifndef __TSCH_PROJECT_CONF_H__
#define __TSCH_PROJECT_CONF_H__

/*******************************************************/
/********************* Enable TSCH *********************/
/*******************************************************/

/* Netstack layers */
#undef NETSTACK_CONF_MAC
#define NETSTACK_CONF_MAC     tschmac_driver
#undef NETSTACK_CONF_RDC
#define NETSTACK_CONF_RDC     nordc_driver
#undef NETSTACK_CONF_FRAMER
#define NETSTACK_CONF_FRAMER  framer_802154

/* IEEE802.15.4 frame version */
#undef FRAME802154_CONF_VERSION
#define FRAME802154_CONF_VERSION FRAME802154_IEEE802154E_2012


/*******************************************************/
/********* Enable RPL non-storing mode *****************/
/*******************************************************/

#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES 0 /* No need for routes */
#undef RPL_CONF_MOP
#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/

/* TSCH and RPL callbacks */
#define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
#define RPL_CALLBACK_NEW_DIO_INTERVAL tsch_rpl_callback_new_dio_interval
#define TSCH_CALLBACK_JOINING_NETWORK tsch_rpl_callback_joining_network
#define TSCH_CALLBACK_LEAVING_NETWORK tsch_rpl_callback_leaving_network

/* Needed for cc2420 platforms only */
/* Disable DCO calibration (uses timerB) */
#undef DCOSYNCH_CONF_ENABLED
#define DCOSYNCH_CONF_ENABLED            0
/* Enable SFD timestamps (uses timerB) */
#undef CC2420_CONF_SFD_TIMESTAMPS
#define CC2420_CONF_SFD_TIMESTAMPS       1
/* Needed for IoT-LAB M3 nodes */
#undef RF2XX_SOFT_PREPARE
#define RF2XX_SOFT_PREPARE 0
#undef RF2XX_WITH_TSCH
#define RF2XX_WITH_TSCH 1

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* TSCH logging. 0: disabled. 1: basic log. 2: with delayed
 * log messages from interrupt */
#undef TSCH_LOG_CONF_LEVEL
#define TSCH_LOG_CONF_LEVEL 0

// #define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE (uint8_t[]){ 14, 18, 22, 26 }
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE (uint8_t[]){ 14 }

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#undef TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 51

/* IEEE802.15.4 PANID */
#undef IEEE802154_CONF_PANID
#define IEEE802154_CONF_PANID 0xcdef

#undef TSCH_CONF_JOIN_MY_PANID_ONLY
#define TSCH_CONF_JOIN_MY_PANID_ONLY 1

/* Adaptive timesync */
#undef TSCH_CONF_ADAPTIVE_TIMESYNC
#define TSCH_CONF_ADAPTIVE_TIMESYNC 1

/* Keep-alive timeout */
#undef TSCH_CONF_KEEPALIVE_TIMEOUT
#define TSCH_CONF_KEEPALIVE_TIMEOUT (20 * CLOCK_SECOND)
#undef TSCH_CONF_MAX_KEEPALIVE_TIMEOUT
#define TSCH_CONF_MAX_KEEPALIVE_TIMEOUT (60 * CLOCK_SECOND)

/* Enhanced Beacon Period */
#undef TSCH_CONF_EB_PERIOD
#define TSCH_CONF_EB_PERIOD (16 * CLOCK_SECOND)
#undef TSCH_CONF_MAX_EB_PERIOD
#define TSCH_CONF_MAX_EB_PERIOD (50 * CLOCK_SECOND)



/*******************************************************/
/*************** Enable Orchestra **********************/
/*******************************************************/
/* Set to run orchestra */
#ifndef WITH_ORCHESTRA
#define WITH_ORCHESTRA 0
#endif /* WITH_ORCHESTRA */


#if WITH_ORCHESTRA

/* See apps/orchestra/README.md for more Orchestra configuration options */
#define TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL 0 /* No 6TiSCH minimal schedule */
#define TSCH_CONF_WITH_LINK_SELECTOR 1 /* Orchestra requires per-packet link selection */
/* Orchestra callbacks */
#define TSCH_CALLBACK_NEW_TIME_SOURCE orchestra_callback_new_time_source
#define TSCH_CALLBACK_PACKET_READY orchestra_callback_packet_ready
#define NETSTACK_CONF_ROUTING_NEIGHBOR_ADDED_CALLBACK orchestra_callback_child_added
#define NETSTACK_CONF_ROUTING_NEIGHBOR_REMOVED_CALLBACK orchestra_callback_child_removed

#endif /* WITH_ORCHESTRA */


#if CONTIKI_TARGET_Z1
/* Save some space to fit the limited RAM of the z1 */
#undef UIP_CONF_TCP
#define UIP_CONF_TCP 0
#undef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 3
#undef UIP_CONF_MAX_ROUTES
#define UIP_CONF_MAX_ROUTES  6
#undef NBR_TABLE_CONF_MAX_NEIGHBORS
#define NBR_TABLE_CONF_MAX_NEIGHBORS 6
#undef UIP_CONF_ND6_SEND_NA
#define UIP_CONF_ND6_SEND_NA 0
#undef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 0
#undef ENABLE_QOS_WHITE
#define ENABLE_QOS_WHITE 1
#endif /* CONTIKI_TARGET_Z1 */


#if CONTIKI_TARGET_COOJA
#define COOJA_CONF_SIMULATE_TURNAROUND 0
#endif /* CONTIKI_TARGET_COOJA */

/* Magic clock issie */
#define LPM_CONF_ENABLE 0

/* Maybe clock drift issue? */
// #define TSCH_CONF_ADAPTIVE_TIMESYNC 1

/* CC2538 need this to work? */
#if CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_ZOUL || \
  CONTIKI_TARGET_OPENMOTE_CC2538
#define TSCH_CONF_HW_FRAME_FILTERING    0
#endif /* CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_ZOUL \
       || CONTIKI_TARGET_OPENMOTE_CC2538 */


/* Needed for CC2538 platforms only */
/* For TSCH we have to use the more accurate crystal oscillator
 * by default the RC oscillator is activated */
#undef SYS_CTRL_CONF_OSC32K_USE_XTAL
#define SYS_CTRL_CONF_OSC32K_USE_XTAL 1


#endif /* __TSCH_PROJECT_CONF_H__ */