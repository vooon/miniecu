/**
 * @file       pbstx.c
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

#include "pbstx.h"
#include "lib_crc16.h"
#include "alert_led.h"

#define PBSTX_STX		0xae

#define SER_TIMEOUT		MS2ST(100)
#define SER_PAYLOAD_TIMEOUT	MS2ST(500)

struct pbstx_header {
	uint8_t seq;
	uint16_t len;
} __attribute__((packed));

/**
 * Initialize PBSTX protocol object
 */
void pbstxObjectInit(PBStxDev *instp, BaseChannel *chp)
{
	osalDbgCheck(instp != NULL);
	osalDbgCheck(chp != NULL);

	instp->chp = chp;
	instp->rx_seq = instp->tx_seq = 0;
	osalMutexObjectInit(&instp->tx_mutex);
}

/**
 * Receive one message
 *
 * @return MSG_OK if message parsed,
 *         MSG_RESET if error occurs
 *         Q_TIMEOUT if timedout, in that case restart receiving with same *msg
 *
 * @todo use rx_seq to calculate missing message count
 * @todo rx/tx statistics counters
 */
msg_t pbstxReceive(PBStxDev *instp, pbstx_message_t *msg)
{
	msg_t ret;

	osalDbgCheck(instp != NULL);
	osalDbgCheck(msg != NULL);

	while (!chThdShouldTerminateX()) {
		struct pbstx_header hdr;

		// 1. wait STX
		ret = chnGetTimeout(instp->chp, SER_TIMEOUT);
		if (ret != PBSTX_STX)
			return ret;

		// 2. read header
		ret = chnReadTimeout(instp->chp, (uint8_t*)&hdr, sizeof(hdr), SER_TIMEOUT);
		if (ret < 0) {
			alert_component(ALS_COMM, AL_FAIL);
			return ret;
		}

		instp->rx_checksum = crc16((uint8_t*)&hdr, sizeof(hdr));
		msg->size = hdr.len;
		msg->seq = instp->rx_seq = hdr.seq;

		if (msg->size > PBSTX_PAYLOAD_BYTES) {
			/* overflow */
			alert_component(ALS_COMM, AL_FAIL);
			return MSG_RESET;
		}

		// 3. read payload
		ret = chnReadTimeout(instp->chp, msg->payload, msg->size, SER_PAYLOAD_TIMEOUT);
		if (ret < 0) {
			alert_component(ALS_COMM, AL_FAIL);
			return ret;
		}

		instp->rx_checksum = crc16part(msg->payload, msg->size, instp->rx_checksum);

		// 4. read crc16
		ret = chnReadTimeout(instp->chp, (uint8_t*)&msg->checksum, sizeof(msg->checksum), SER_TIMEOUT);
		if (ret < 0) {
			alert_component(ALS_COMM, AL_FAIL);
			return ret;
		}

		// 5. check crc && process pkt
		if (instp->rx_checksum == msg->checksum) {
			alert_component(ALS_COMM, AL_NORMAL);
			return MSG_OK;
		}
		else {
			alert_component(ALS_COMM, AL_FAIL);
			return MSG_RESET;
		}
	}

	return MSG_RESET;
}

/**
 * Send pbstx_message_t
 *
 * This function will calculate checksum.
 */
msg_t pbstxSend(PBStxDev *instp, pbstx_message_t *msg)
{
	osalDbgCheck(instp != NULL);
	osalDbgCheck(msg != NULL);
	osalDbgAssert(msg->size <= PBSTX_PAYLOAD_BYTES, "message to long");

	chMtxLock(&instp->tx_mutex);

	msg_t ret;
	uint8_t header[] = { PBSTX_STX, instp->tx_seq++, msg->size & 0xff, msg->size >> 8 };

	msg->checksum = crc16(header + 1, sizeof(header) - 1);
	msg->checksum = crc16part(msg->payload, msg->size, msg->checksum);

	ret = chnWriteTimeout(instp->chp, header, sizeof(header), SER_TIMEOUT);
	if (ret < 0) goto unlock_ret;

	ret = chnWriteTimeout(instp->chp, msg->payload, msg->size, SER_PAYLOAD_TIMEOUT);
	if (ret < 0) goto unlock_ret;

	ret = chnWriteTimeout(instp->chp, (uint8_t*)&msg->checksum, sizeof(msg->checksum), SER_TIMEOUT);

unlock_ret:
	chMtxUnlock(&instp->tx_mutex);
	return ret;
}

