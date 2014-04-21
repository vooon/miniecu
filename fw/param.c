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

static msg_t _pr_set_bool(const struct param_entry *obj, miniecu_ParamType *value)
{
	if (value->has_u_bool) {
		_pr_set(obj, &value->u_bool);
		return PARAM_OK;
	}
	else if (value->has_u_int32) {
		bool b_val = value->u_int32 != 0;
		_pr_set(obj, &b_val);
		return PARAM_OK;
	}

	return PARAM_ETYPE;
}

static msg_t _pr_set_int32(const struct param_entry *obj, miniecu_ParamType *value)
{
	if (value->has_u_int32) {
		if (obj->min.i > value->u_int32 || value->u_int32 > obj->max.i)
			return PARAM_LIMIT;

		_pr_set(obj, &value->u_int32);
		return PARAM_OK;
	}

	return PARAM_ETYPE;
}

static msg_t _pr_set_float(const struct param_entry *obj, miniecu_ParamType *value)
{
	if (value->has_u_float) {
		if (obj->min.f > value->u_float || value->u_float > obj->max.f)
			return PARAM_LIMIT;

		_pr_set(obj, &value->u_float);
		return PARAM_OK;
	}

	return PARAM_ETYPE;

}

static msg_t _pr_set_string(const struct param_entry *obj, miniecu_ParamType *value)
{
	if (value->has_u_string) {
		_pr_set(obj, &value->u_string);
		return PARAM_OK;
	}

	return PARAM_ETYPE;

}

static const struct param_entry *_prt_find(const char *id, size_t *idx)
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

msg_t param_set(const char *id, miniecu_ParamType *value)
{
	msg_t ret;
	size_t idx;
	const struct param_entry *p = _prt_find(id, &idx);

	if (p == NULL)
		return PARAM_NOTEXIST;

	switch (p->type) {
	case PT_BOOL:
		ret = _pr_set_bool(p, value);
		break;
	case PT_INT32:
		ret = _pr_set_int32(p, value);
		break;
	case PT_FLOAT:
		ret = _pr_set_float(p, value);
		break;
	case PT_STRING:
		ret = _pr_set_string(p, value);
		break;
	}

	if (ret == PARAM_OK && p->change_cb != NULL)
		p->change_cb(p);

	if (ret == PARAM_ETYPE)
		debug_printf(DP_ERROR, "wrong type: %s", p->id);
	else if (ret == PARAM_LIMIT)
		debug_printf(DP_ERROR, "out of range: %s", p->id);

	return ret;
}

msg_t param_get(const char *id, miniecu_ParamType *value, size_t *idx)
{
	const struct param_entry *p = _prt_find(id, idx);

	if (p == NULL)
		return PARAM_NOTEXIST;

	_pr_set_ParamType(value, p);
	return PARAM_OK;
}

msg_t param_get_by_idx(size_t idx, char *id, miniecu_ParamType *value)
{
	if (idx >= ARRAY_SIZE(parameter_table))
		return PARAM_NOTEXIST;

	strncpy(id, parameter_table[idx].id, PT_ID_SIZE);
	_pr_set_ParamType(value, &parameter_table[idx]);

	return PARAM_OK;
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

