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

// These variables and functions glue the code emitters to the runtime.

typedef enum {
    MP_CODE_UNUSED,
    MP_CODE_RESERVED,
    MP_CODE_BYTECODE,
    MP_CODE_NATIVE_PY,
    MP_CODE_NATIVE_VIPER,
    MP_CODE_NATIVE_ASM,
} mp_raw_code_kind_t;

typedef struct _mp_code_t {
    mp_raw_code_kind_t kind : 3;
    uint scope_flags : 7;
    uint n_pos_args : 11;
    uint n_kwonly_args : 11;
    qstr *arg_names;
    union {
        struct {
            byte *code;
            uint len;
        } u_byte;
        struct {
            void *fun;
            mp_uint_t type_sig; // for viper, compressed as 2-bit types; ret is MSB, then arg0, arg1, etc
        } u_native;
    };
} mp_raw_code_t;

mp_raw_code_t *mp_emit_glue_new_raw_code(void);

void mp_emit_glue_assign_bytecode(mp_raw_code_t *rc, byte *code, uint len, uint n_pos_args, uint n_kwonly_args, qstr *arg_names, uint scope_flags);
void mp_emit_glue_assign_native(mp_raw_code_t *rc, mp_raw_code_kind_t kind, void *f, uint len, int n_args, mp_uint_t type_sig);

mp_obj_t mp_make_function_from_raw_code(mp_raw_code_t *rc, mp_obj_t def_args, mp_obj_t def_kw_args);
mp_obj_t mp_make_closure_from_raw_code(mp_raw_code_t *rc, uint n_closed_over, const mp_obj_t *args);
