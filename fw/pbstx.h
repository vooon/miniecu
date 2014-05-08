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

extern void pbstx_init(void);
extern void pbstx_check_usb(void);
extern msg_t pbstx_receive(uint8_t *msgid, uint8_t *payload, uint8_t *payload_len);
extern msg_t pbstx_send(uint8_t msgid, const uint8_t *payload, uint8_t payload_len);

#endif /* PBSTX_H */
