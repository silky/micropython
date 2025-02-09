/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
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
#include <assert.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "obj.h"
#include "runtime.h"

typedef struct _mp_obj_reversed_t {
    mp_obj_base_t base;
    mp_obj_t seq;           // sequence object that we are reversing
    mp_uint_t cur_index;    // current index, plus 1; 0=no more, 1=last one (index 0)
} mp_obj_reversed_t;

STATIC mp_obj_t reversed_make_new(mp_obj_t type_in, uint n_args, uint n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    mp_obj_reversed_t *o = m_new_obj(mp_obj_reversed_t);
    o->base.type = &mp_type_reversed;
    o->seq = args[0];
    o->cur_index = mp_obj_get_int(mp_obj_len(args[0])); // start at the end of the sequence

    return o;
}

STATIC mp_obj_t reversed_iternext(mp_obj_t self_in) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_type_reversed));
    mp_obj_reversed_t *self = self_in;

    // "raise" stop iteration if we are at the end (the start) of the sequence
    if (self->cur_index == 0) {
        return MP_OBJ_STOP_ITERATION;
    }

    // pre-decrement and index sequence
    self->cur_index -= 1;
    return mp_obj_subscr(self->seq, MP_OBJ_NEW_SMALL_INT(self->cur_index), MP_OBJ_SENTINEL);
}

const mp_obj_type_t mp_type_reversed = {
    { &mp_type_type },
    .name = MP_QSTR_reversed,
    .make_new = reversed_make_new,
    .getiter = mp_identity,
    .iternext = reversed_iternext,
};
