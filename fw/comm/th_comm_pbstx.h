/**
 * @file       th_comm.h
 * @brief      Communication thread
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

#ifndef TH_COMM_PBSTX_H
#define TH_COMM_PBSTX_H

#include "fw_common.h"

/* public functions */
thread_t *pbstxCreate(void *chn, size_t size, tprio_t prio);
void send_command_response(uint32_t operation, uint32_t response);
/* debug_printf() defined in fw_common.h */

#endif /* TH_COMM_PBSTX_H */