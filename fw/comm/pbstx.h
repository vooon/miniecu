/**
 * @file       pbstx.h
 * @brief      PB sterial transfer functions
 * @author     Vladimir Ermakov Copyright (C) 2014.
 * @see        The GNU Public License (GPL) Version 3
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PBSTX_H
#define PBSTX_H

#include "fw_common.h"
#include "miniecu.pb.h"


#define PBSTX_PAYLOAD_BYTES	256

typedef struct PBstxDev {
	BaseChannel *chp;
	mutex_t tx_mutex;
	uint16_t rx_checksum;
	uint8_t rx_seq;
	uint8_t tx_seq;
} PBStxDev;

typedef struct pbstx_message {
	uint8_t seq;
	uint16_t size;
	uint16_t checksum;
	uint8_t payload[PBSTX_PAYLOAD_BYTES];
} pbstx_message_t;


extern void pbstxObjectInit(PBStxDev *instp, BaseChannel *chp);
extern msg_t pbstxReceive(PBStxDev *instp, pbstx_message_t *msg);
extern msg_t pbstxSend(PBStxDev *instp, pbstx_message_t *msg);

#endif /* PBSTX_H */
