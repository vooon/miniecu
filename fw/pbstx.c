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
#include "pios_crc.h" /* use crc functions from OpenPilot */

/* Local variables */
static mutex_t tx_mutex;

enum rx_state {
	PR_WAIT_START = 0,
	PR_SEQ,
	PR_MSGID,
	PR_LEN,
	PR_PAYLOAD,
	PR_CRC
};

#define HDR_START		0xa5
#define MAX_PAYLOAD		255

#define SER_TIMEOUT		MS2ST(10)
#define SER_PAYLOAD_TIMEOUT	MS2ST(20)


void pbstx_init(void)
{
	osalMutexObjectInit(&tx_mutex);
}

msg_t pbstx_receive(uint8_t *msgid, uint8_t *payload, uint8_t *payload_len)
{
	msg_t ret;
	static ATTR_UNUSED uint8_t seq = 0;
	static uint8_t pkt_crc = 0;
	static enum rx_state rx_state = PR_WAIT_START;

	while (true /*!chThdShouldTerminate()*/) {
		ret = sdGetTimeout(&PBSTX_SD, SER_TIMEOUT);
		if (ret == Q_TIMEOUT || ret == Q_RESET)
			return ret;

		switch (rx_state) {
		case PR_WAIT_START:
			if (ret == HDR_START) {
				rx_state = PR_SEQ;
			}
			break;

		case PR_SEQ:
			seq = ret;
			pkt_crc = PIOS_CRC_updateByte(0, ret);
			rx_state = PR_MSGID;
			break;

		case PR_MSGID:
			*msgid = ret;
			pkt_crc = PIOS_CRC_updateByte(pkt_crc, ret);
			rx_state = PR_LEN;
			break;

		case PR_LEN:
			*payload_len = ret;
			pkt_crc = PIOS_CRC_updateByte(pkt_crc, ret);
			rx_state = PR_PAYLOAD;
			if (*payload_len == 0) {
				rx_state = PR_CRC;
				break;
			}
			/* fall through if payload exists */

		case PR_PAYLOAD:
			ret = sdReadTimeout(&PBSTX_SD, payload, *payload_len,
					SER_PAYLOAD_TIMEOUT);
			if (ret == Q_TIMEOUT || ret == Q_RESET) {
				//ALERT_SET_FAIL(PROTO, protocol_status);
				rx_state = PR_WAIT_START;
				return ret;
			}

			pkt_crc = PIOS_CRC_updateCRC(pkt_crc, payload, *payload_len);
			rx_state = PR_CRC;
			break;

		case PR_CRC:
			rx_state = PR_WAIT_START;
			/* check crc && process pkt */
			if (pkt_crc == ret) {
				return MSG_OK;
			} else {
				//ALERT_SET_FAIL(PROTO, protocol_status);
				return MSG_RESET;
			}
			break;

		default:
			rx_state = PR_WAIT_START;
		}
	}

	return MSG_RESET;
}

msg_t pbstx_send(uint8_t msgid, const uint8_t *payload, uint8_t payload_len)
{
	msg_t ret;
	uint8_t crc;
	static uint8_t tx_seq = 0;
	uint8_t header[] = { HDR_START, tx_seq++, msgid, payload_len };

	osalMutexLock(&tx_mutex);

	crc = PIOS_CRC_updateCRC(0, header + 1, sizeof(header) - 1);
	ret = sdWriteTimeout(&PBSTX_SD, header, sizeof(header), SER_TIMEOUT);
	if (ret == Q_TIMEOUT || ret == Q_RESET)
		goto unlock_ret;

	if (payload_len > 0) {
		crc = PIOS_CRC_updateCRC(crc, payload, payload_len);
		ret = sdWriteTimeout(&PBSTX_SD, payload, payload_len,
				SER_PAYLOAD_TIMEOUT);
		if (ret == Q_TIMEOUT || ret == Q_RESET)
			goto unlock_ret;
	}

	ret = sdPutTimeout(&PBSTX_SD, crc, SER_TIMEOUT);

unlock_ret:
	osalMutexUnlock(&tx_mutex);
	return ret;
}

