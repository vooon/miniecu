/**
 * @file       param.h
 * @brief      parameter storage functions
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

#ifndef PARAM_H
#define PARAM_H

#include "fw_common.h"

#define PT_ID_SIZE	16
#define PT_STRING_SIZE	16

enum param_type {
	PT_BOOL,	// pointer to bool
	PT_INT32,	// pointer to int32
	PT_FLOAT,	// pointer to float
	PT_STRING	// pointer to char[16]
};

union param_minmax {
	int32_t i;
	float f;
};

union param_default {
	bool b;
	int32_t i;
	float f;
	char s[PT_STRING_SIZE];
};

/**
 * Entry for parameter table
 */
struct param_entry {
	//! parameter name (CAPITALS)
	char id[PT_ID_SIZE];
	//! type of storage variable
	enum param_type type;
	//! pointer to storage
	void *variable;
	//! default
	union param_default default_value;
	//! minimum (only for int32 or float)
	union param_minmax min;
	//! maximum (int32 or float)
	union param_minmax max;
	//! optional callback
	void (*change_cb)(const struct param_entry *self);
};

#define PARAM_BOOL(_id, _var, _default, _change_cb)			\
	{ (_id), PT_BOOL, &(_var), {.b=(_default)}, {.i=0}, {.i=1}, (_change_cb) }
#define PARAM_INT32(_id, _var, _default, _min, _max, _change_cb)	\
	{ (_id), PT_INT32, &(_var), {.i=(_default)}, {.i=(_min)}, {.i=(_max)}, (_change_cb) }
#define PARAM_FLOAT(_id, _var, _default, _min, _max, _change_cb)	\
	{ (_id), PT_FLOAT, &(_var), {.f=(_default)}, {.f=(_min)}, {.f=(_min)}, (_change_cb) }
#define PARAM_STRING(_id, _var, _default, _change_cb)			\
	{ (_id), PT_STRING, &(_var), {.s=(_default)}, {.i=0}, {.i=16}, (_change_cb) }

#ifndef _PB_MINIECU_PB_H_
struct _miniecu_ParamType;
typedef struct _miniecu_Paramtype miniecu_ParamType;
#endif /* _PB_MINIECU_PB_H_ */

msg_t param_set(const char *id, const miniecu_ParamType *value);
msg_t param_get(const char *id, miniecu_ParamType *value, size_t *idx);
msg_t param_get_by_idx(size_t idx, char *id, miniecu_ParamType *value);
size_t param_count(void);
void param_init(void);

#endif /* PARAM_H */
