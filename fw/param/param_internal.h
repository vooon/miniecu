/**
 * @file       param_internal.h
 * @brief      parameter storage internal
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

#ifndef PARAM_INTERNAL_H
#define PARAM_INTERNAL_H

#include "param.h"


#define PARAM_BOOL(_id, _var, _default, _flags, _change_cb)			\
	{ (_id), PT_BOOL, &(_var), {.b=(_default)}, {.i=0}, {.i=1}, (_flags), (_change_cb) }
#define PARAM_INT32(_id, _var, _default, _min, _max, _flags, _change_cb)	\
	{ (_id), PT_INT32, &(_var), {.i=(_default)}, {.i=(_min)}, {.i=(_max)}, (_flags), (_change_cb) }
#define PARAM_FLOAT(_id, _var, _default, _min, _max, _flags, _change_cb)	\
	{ (_id), PT_FLOAT, &(_var), {.f=(_default)}, {.f=(_min)}, {.f=(_max)}, (_flags), (_change_cb) }
#define PARAM_STRING(_id, _var, _default, _flags, _change_cb)			\
	{ (_id), PT_STRING, &(_var), {.s=(_default)}, {.i=0}, {.i=PT_STRING_SIZE}, (_flags), (_change_cb) }

#endif /* PARAM_INTERNAL_H */
