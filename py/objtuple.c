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

#include <string.h>
#include <assert.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "obj.h"
#include "runtime0.h"
#include "runtime.h"
#include "objtuple.h"

STATIC mp_obj_t mp_obj_new_tuple_iterator(mp_obj_tuple_t *tuple, int cur);

/******************************************************************************/
/* tuple                                                                      */

void mp_obj_tuple_print(void (*print)(void *env, const char *fmt, ...), void *env, mp_obj_t o_in, mp_print_kind_t kind) {
    mp_obj_tuple_t *o = o_in;
    print(env, "(");
    for (int i = 0; i < o->len; i++) {
        if (i > 0) {
            print(env, ", ");
        }
        mp_obj_print_helper(print, env, o->items[i], PRINT_REPR);
    }
    if (o->len == 1) {
        print(env, ",");
    }
    print(env, ")");
}

STATIC mp_obj_t mp_obj_tuple_make_new(mp_obj_t type_in, uint n_args, uint n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    switch (n_args) {
        case 0:
            // return a empty tuple
            return mp_const_empty_tuple;

        case 1:
        default: {
            // 1 argument, an iterable from which we make a new tuple
            if (MP_OBJ_IS_TYPE(args[0], &mp_type_tuple)) {
                return args[0];
            }

            // TODO optimise for cases where we know the length of the iterator

            uint alloc = 4;
            uint len = 0;
            mp_obj_t *items = m_new(mp_obj_t, alloc);

            mp_obj_t iterable = mp_getiter(args[0]);
            mp_obj_t item;
            while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
                if (len >= alloc) {
                    items = m_renew(mp_obj_t, items, alloc, alloc * 2);
                    alloc *= 2;
                }
                items[len++] = item;
            }

            mp_obj_t tuple = mp_obj_new_tuple(len, items);
            m_free(items, alloc);

            return tuple;
        }
    }
}

// Don't pass MP_BINARY_OP_NOT_EQUAL here
STATIC bool tuple_cmp_helper(int op, mp_obj_t self_in, mp_obj_t another_in) {
    mp_obj_type_t *self_type = mp_obj_get_type(self_in);
    if (self_type->getiter != mp_obj_tuple_getiter) {
        assert(0);
    }
    mp_obj_type_t *another_type = mp_obj_get_type(another_in);
    mp_obj_tuple_t *self = self_in;
    mp_obj_tuple_t *another = another_in;
    if (another_type->getiter != mp_obj_tuple_getiter) {
        // Slow path for user subclasses
        another = mp_instance_cast_to_native_base(another, &mp_type_tuple);
        if (another == MP_OBJ_NULL) {
            return false;
        }
    }

    return mp_seq_cmp_objs(op, self->items, self->len, another->items, another->len);
}

mp_obj_t mp_obj_tuple_unary_op(int op, mp_obj_t self_in) {
    mp_obj_tuple_t *self = self_in;
    switch (op) {
        case MP_UNARY_OP_BOOL: return MP_BOOL(self->len != 0);
        case MP_UNARY_OP_LEN: return MP_OBJ_NEW_SMALL_INT(self->len);
        default: return MP_OBJ_NULL; // op not supported
    }
}

mp_obj_t mp_obj_tuple_binary_op(int op, mp_obj_t lhs, mp_obj_t rhs) {
    mp_obj_tuple_t *o = lhs;
    switch (op) {
        case MP_BINARY_OP_ADD: {
            if (!mp_obj_is_subclass_fast(mp_obj_get_type(rhs), (mp_obj_t)&mp_type_tuple)) {
                return MP_OBJ_NULL; // op not supported
            }
            mp_obj_tuple_t *p = rhs;
            mp_obj_tuple_t *s = mp_obj_new_tuple(o->len + p->len, NULL);
            mp_seq_cat(s->items, o->items, o->len, p->items, p->len, mp_obj_t);
            return s;
        }
        case MP_BINARY_OP_MULTIPLY: {
            mp_int_t n;
            if (!mp_obj_get_int_maybe(rhs, &n)) {
                return MP_OBJ_NULL; // op not supported
            }
            if (n <= 0) {
                return mp_const_empty_tuple;
            }
            mp_obj_tuple_t *s = mp_obj_new_tuple(o->len * n, NULL);
            mp_seq_multiply(o->items, sizeof(*o->items), o->len, n, s->items);
            return s;
        }
        case MP_BINARY_OP_EQUAL:
        case MP_BINARY_OP_LESS:
        case MP_BINARY_OP_LESS_EQUAL:
        case MP_BINARY_OP_MORE:
        case MP_BINARY_OP_MORE_EQUAL:
            return MP_BOOL(tuple_cmp_helper(op, lhs, rhs));

        default:
            return MP_OBJ_NULL; // op not supported
    }
}

mp_obj_t mp_obj_tuple_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value) {
    if (value == MP_OBJ_SENTINEL) {
        // load
        mp_obj_tuple_t *self = self_in;
#if MICROPY_PY_BUILTINS_SLICE
        if (MP_OBJ_IS_TYPE(index, &mp_type_slice)) {
            mp_bound_slice_t slice;
            if (!mp_seq_get_fast_slice_indexes(self->len, index, &slice)) {
                nlr_raise(mp_obj_new_exception_msg(&mp_type_NotImplementedError,
                    "only slices with step=1 (aka None) are supported"));
            }
            mp_obj_tuple_t *res = mp_obj_new_tuple(slice.stop - slice.start, NULL);
            mp_seq_copy(res->items, self->items + slice.start, res->len, mp_obj_t);
            return res;
        }
#endif
        uint index_value = mp_get_index(self->base.type, self->len, index, false);
        return self->items[index_value];
    } else {
        return MP_OBJ_NULL; // op not supported
    }
}

mp_obj_t mp_obj_tuple_getiter(mp_obj_t o_in) {
    return mp_obj_new_tuple_iterator(o_in, 0);
}

STATIC mp_obj_t tuple_count(mp_obj_t self_in, mp_obj_t value) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_type_tuple));
    mp_obj_tuple_t *self = self_in;
    return mp_seq_count_obj(self->items, self->len, value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(tuple_count_obj, tuple_count);

STATIC mp_obj_t tuple_index(uint n_args, const mp_obj_t *args) {
    assert(MP_OBJ_IS_TYPE(args[0], &mp_type_tuple));
    mp_obj_tuple_t *self = args[0];
    return mp_seq_index_obj(self->items, self->len, n_args, args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tuple_index_obj, 2, 4, tuple_index);

STATIC const mp_map_elem_t tuple_locals_dict_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR_count), (mp_obj_t)&tuple_count_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_index), (mp_obj_t)&tuple_index_obj },
};

STATIC MP_DEFINE_CONST_DICT(tuple_locals_dict, tuple_locals_dict_table);

const mp_obj_type_t mp_type_tuple = {
    { &mp_type_type },
    .name = MP_QSTR_tuple,
    .print = mp_obj_tuple_print,
    .make_new = mp_obj_tuple_make_new,
    .unary_op = mp_obj_tuple_unary_op,
    .binary_op = mp_obj_tuple_binary_op,
    .subscr = mp_obj_tuple_subscr,
    .getiter = mp_obj_tuple_getiter,
    .locals_dict = (mp_obj_t)&tuple_locals_dict,
};

// the zero-length tuple
const mp_obj_tuple_t mp_const_empty_tuple_obj = {{&mp_type_tuple}, 0};

mp_obj_t mp_obj_new_tuple(uint n, const mp_obj_t *items) {
    if (n == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = m_new_obj_var(mp_obj_tuple_t, mp_obj_t, n);
    o->base.type = &mp_type_tuple;
    o->len = n;
    if (items) {
        for (int i = 0; i < n; i++) {
            o->items[i] = items[i];
        }
    }
    return o;
}

void mp_obj_tuple_get(mp_obj_t self_in, uint *len, mp_obj_t **items) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_type_tuple));
    mp_obj_tuple_t *self = self_in;
    if (len) {
        *len = self->len;
    }
    if (items) {
        *items = &self->items[0];
    }
}

void mp_obj_tuple_del(mp_obj_t self_in) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_type_tuple));
    mp_obj_tuple_t *self = self_in;
    m_del_var(mp_obj_tuple_t, mp_obj_t, self->len, self);
}

mp_int_t mp_obj_tuple_hash(mp_obj_t self_in) {
    assert(MP_OBJ_IS_TYPE(self_in, &mp_type_tuple));
    mp_obj_tuple_t *self = self_in;
    // start hash with pointer to empty tuple, to make it fairly unique
    mp_int_t hash = (mp_int_t)mp_const_empty_tuple;
    for (uint i = 0; i < self->len; i++) {
        hash += mp_obj_hash(self->items[i]);
    }
    return hash;
}

/******************************************************************************/
/* tuple iterator                                                             */

typedef struct _mp_obj_tuple_it_t {
    mp_obj_base_t base;
    mp_obj_tuple_t *tuple;
    mp_uint_t cur;
} mp_obj_tuple_it_t;

STATIC mp_obj_t tuple_it_iternext(mp_obj_t self_in) {
    mp_obj_tuple_it_t *self = self_in;
    if (self->cur < self->tuple->len) {
        mp_obj_t o_out = self->tuple->items[self->cur];
        self->cur += 1;
        return o_out;
    } else {
        return MP_OBJ_STOP_ITERATION;
    }
}

STATIC const mp_obj_type_t mp_type_tuple_it = {
    { &mp_type_type },
    .name = MP_QSTR_iterator,
    .getiter = mp_identity,
    .iternext = tuple_it_iternext,
};

STATIC mp_obj_t mp_obj_new_tuple_iterator(mp_obj_tuple_t *tuple, int cur) {
    mp_obj_tuple_it_t *o = m_new_obj(mp_obj_tuple_it_t);
    o->base.type = &mp_type_tuple_it;
    o->tuple = tuple;
    o->cur = cur;
    return o;
}
