/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "obj.h"
#include "runtime0.h"
#include "runtime.h"

/******************************************************************************/
/* range iterator                                                             */

typedef struct _mp_obj_range_it_t {
    mp_obj_base_t base;
    // TODO make these values generic objects or something
    mp_int_t cur;
    mp_int_t stop;
    mp_int_t step;
} mp_obj_range_it_t;

STATIC mp_obj_t range_it_iternext(mp_obj_t o_in) {
    mp_obj_range_it_t *o = o_in;
    if ((o->step > 0 && o->cur < o->stop) || (o->step < 0 && o->cur > o->stop)) {
        mp_obj_t o_out = MP_OBJ_NEW_SMALL_INT(o->cur);
        o->cur += o->step;
        return o_out;
    } else {
        return MP_OBJ_STOP_ITERATION;
    }
}

STATIC const mp_obj_type_t range_it_type = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .getiter = mp_identity,
    .iternext = range_it_iternext,
};

mp_obj_t mp_obj_new_range_iterator(int cur, int stop, int step) {
    mp_obj_range_it_t *o = m_new_obj(mp_obj_range_it_t);
    o->base.type = &range_it_type;
    o->cur = cur;
    o->stop = stop;
    o->step = step;
    return o;
}

/******************************************************************************/
/* range                                                                      */

typedef struct _mp_obj_range_t {
    mp_obj_base_t base;
    // TODO make these values generic objects or something
    mp_int_t start;
    mp_int_t stop;
    mp_int_t step;
} mp_obj_range_t;

STATIC void range_print(void (*print)(void *env, const char *fmt, ...), void *env, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_obj_range_t *self = self_in;
    print(env, "range(%d, %d", self->start, self->stop);
    if (self->step == 1) {
        print(env, ")");
    } else {
        print(env, ", %d)", self->step);
    }
}

STATIC mp_obj_t range_make_new(mp_obj_t type_in, uint n_args, uint n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 3, false);

    mp_obj_range_t *o = m_new_obj(mp_obj_range_t);
    o->base.type = &mp_type_range;
    o->start = 0;
    o->step = 1;

    if (n_args == 1) {
        o->stop = mp_obj_get_int(args[0]);
    } else {
        o->start = mp_obj_get_int(args[0]);
        o->stop = mp_obj_get_int(args[1]);
        if (n_args == 3) {
            // TODO check step is non-zero
            o->step = mp_obj_get_int(args[2]);
        }
    }

    return o;
}

STATIC mp_int_t range_len(mp_obj_range_t *self) {
    // When computing length, need to take into account step!=1 and step<0.
    mp_int_t len = self->stop - self->start + self->step;
    if (self->step > 0) {
        len -= 1;
    } else {
        len += 1;
    }
    len = len / self->step;
    if (len < 0) {
        len = 0;
    }
    return len;
}

STATIC mp_obj_t range_unary_op(int op, mp_obj_t self_in) {
    mp_obj_range_t *self = self_in;
    mp_int_t len = range_len(self);
    switch (op) {
        case MP_UNARY_OP_BOOL: return MP_BOOL(len > 0);
        case MP_UNARY_OP_LEN: return MP_OBJ_NEW_SMALL_INT(len);
        default: return MP_OBJ_NULL; // op not supported
    }
}

STATIC mp_obj_t range_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {
    if (value == MP_OBJ_SENTINEL) {
        // load
        mp_obj_range_t *self = self_in;
        mp_int_t len = range_len(self);
#if MICROPY_PY_BUILTINS_SLICE
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            mp_seq_get_fast_slice_indexes(len, index, &slice);
            mp_obj_range_t *o = m_new_obj(mp_obj_range_t);
            o->base.type = &mp_type_range;
            o->start = slice.start;
            o->stop = slice.stop;
            o->step = slice.step;
            return o;
        }
#endif
        uint index_val = mp_get_index(self->base.type, len, index, false);
        return MP_OBJ_NEW_SMALL_INT(self->start + index_val * self->step);
    } else {
        return MP_OBJ_NULL; // op not supported
    }
}

STATIC mp_obj_t range_getiter(mp_obj_t o_in) {
    mp_obj_range_t *o = o_in;
    return mp_obj_new_range_iterator(o->start, o->stop, o->step);
}

const mp_obj_type_t mp_type_range = {
    { &mp_type_type },
    .name = MP_QSTR_range,
    .print = range_print,
    .make_new = range_make_new,
    .unary_op = range_unary_op,
    .subscr = range_subscr,
    .getiter = range_getiter,
};
