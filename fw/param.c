/**
 * @file       param.c
 * @brief      parameter functions
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

#include "miniecu.pb.h"
#include "param.h"
#include <string.h>

/* -*- local -*- */

#include "param_table.c"

/* -*- local functions -*- */

static void _pr_set(const struct param_entry *obj, void *val)
{
	switch (obj->type) {
	case PT_BOOL:
		*((bool *)obj->variable) = *((bool *)val);
		break;
	case PT_INT32:
		*((int32_t *)obj->variable) = *((int32_t *)val);
		break;
	case PT_FLOAT:
		*((float *)obj->variable) = *((float *)val);
		break;
	case PT_STRING:
		strncpy(obj->variable, val, PT_STRING_SIZE);
		break;
	};
}

static const struct param_entry *_pt_find(const char *id, size_t *idx)
{
	const struct param_entry *p = parameter_table;
	*idx = 0;

	for (; *idx < ARRAY_SIZE(parameter_table); (*idx)++, p++) {
		if (strncmp(p->id, id, PT_ID_SIZE) == 0)
			return p;
	}

	return NULL;
}

static void _pr_set_ParamType(miniecu_ParamType *value, const struct param_entry *obj)
{
	switch (obj->type) {
	case PT_BOOL:
		value->has_u_bool = true;
		value->has_u_int32 = false;
		value->has_u_float = false;
		value->has_u_string = false;
		value->u_bool = *((bool *)obj->variable);
		break;
	case PT_INT32:
		value->has_u_bool = false;
		value->has_u_int32 = true;
		value->has_u_float = false;
		value->has_u_string = false;
		value->u_int32 = *((int32_t *)obj->variable);
		break;
	case PT_FLOAT:
		value->has_u_bool = false;
		value->has_u_int32 = false;
		value->has_u_float = true;
		value->has_u_string = false;
		value->u_float = *((float *)obj->variable);
		break;
	case PT_STRING:
		value->has_u_bool = false;
		value->has_u_int32 = false;
		value->has_u_float = false;
		value->has_u_string = true;
		strncpy(value->u_string, obj->variable, PT_STRING_SIZE);
		break;
	};
}

/* -*- global -*- */

msg_t param_set(const char *id, const miniecu_ParamType *value)
{
	return MSG_OK;
}

msg_t param_get(const char *id, miniecu_ParamType *value, size_t *idx)
{
	const struct param_entry *p = _pt_find(id, idx);

	if (p == NULL)
		return MSG_RESET;

	_pr_set_ParamType(value, p);
	return MSG_OK;
}

msg_t param_get_by_idx(size_t idx, char *id, miniecu_ParamType *value)
{
	if (idx >= ARRAY_SIZE(parameter_table))
		return MSG_RESET;

	strncpy(id, parameter_table[idx].id, PT_ID_SIZE);
	_pr_set_ParamType(value, &parameter_table[idx]);

	return MSG_OK;
}

size_t param_count(void)
{
	return ARRAY_SIZE(parameter_table);
}

void param_init(void)
{
	size_t i = 0;
	const struct param_entry *p = parameter_table;

	for (; i < ARRAY_SIZE(parameter_table); i++, p++)
		_pr_set(p, (void *)&p->default_value);
}

