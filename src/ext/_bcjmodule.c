#include "Python.h"
#include "pythread.h"   /* For Python 3.6 */
#include "structmember.h"

#include "Arch.h"
#include "Bra.h"

#ifndef Py_UNREACHABLE
#define Py_UNREACHABLE() assert(0)
#endif


#define ACQUIRE_LOCK(obj) do {                    \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS                    \
        PyThread_acquire_lock((obj)->lock, 1);    \
        Py_END_ALLOW_THREADS                      \
    } } while (0)
#define RELEASE_LOCK(obj) PyThread_release_lock((obj)->lock)
static const char init_twice_msg[] = "__init__ method is called twice.";

enum Method {
    x86,
    arm,
    armt,
    ppc,
    sparc,
    ia64
};

typedef struct buffer_s {
    Byte *buf;
    SizeT size;
} buffer;

typedef struct {
    PyObject_HEAD

    enum Method method;

    UInt32 ip;
    UInt32 state;
    size_t readahead;
    Bool is_encoder;
    size_t size;

    PyThread_type_lock lock;

    /* __init__ has been called, 0 or 1. */
    char inited;

    Byte *input_buffer;
    size_t input_buffer_size;
    size_t in_begin, in_end;
} BCJFilter;

static PyObject *
BCJFilter_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    BCJFilter *self;
    self = (BCJFilter *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    /* Thread lock */
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        goto error;
    }
    return (PyObject *) self;

    error:
    Py_XDECREF(self);
    PyErr_NoMemory();
    return NULL;
}

static void
BCJFilter_dealloc(BCJFilter *self) {
    if (self->lock) {
        PyThread_free_lock(self->lock);
    }
    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject *) self);
    Py_DECREF(tp);
}

static PyObject *
BCJFilter_do_filter(BCJFilter *self, Py_buffer *data) {
    buffer in;

    ACQUIRE_LOCK(self);

    if (self->in_begin == self->in_end) {
        Byte* tmp = PyMem_Malloc(data->len);
        if (tmp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, data->buf, data->len);
        in.buf = tmp;
        in.size = data->len;
    } else if (data->len == 0) {
        in.buf = self->input_buffer + self->in_begin;
        in.size = self->in_end - self->in_begin;
    } else {
        size_t usenow = self->in_end - self->in_begin;
        Byte *tmp;
        tmp = PyMem_Malloc(data->len + usenow);
        if (tmp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, self->input_buffer + self->in_begin, usenow);
        PyMem_Free(self->input_buffer);
        memcpy(tmp + usenow, data->buf, data->len);
        self->input_buffer = tmp;
        in.buf = self->input_buffer;
        in.size = usenow;
    }

    SizeT out_len;
    switch (self->method) {
        case x86:
            out_len = x86_Convert(in.buf, in.size, self->ip, &self->state, self->is_encoder);
            break;
        case arm:
            out_len = ARM_Convert(in.buf, in.size, self->ip, self->is_encoder);
            break;
        case armt:
            out_len = ARMT_Convert(in.buf, in.size, self->ip, self->is_encoder);
            break;
        case ppc:
            out_len = PPC_Convert(in.buf, in.size, self->ip, self->is_encoder);
            break;
        case sparc:
            out_len = SPARC_Convert(in.buf, in.size, self->ip, self->is_encoder);
            break;
        case ia64:
            out_len = IA64_Convert(in.buf, in.size, self->ip, self->is_encoder);
            break;
        default:
            // should not come here.
            goto error;
    }
    self->ip += out_len;
    self->size -= out_len;

    if (self->size <= self->readahead) {
        // processed all the data
        assert(in.size == out_len + self->size);
        // flush all the data
        out_len = in.size;
    }

    PyObject *result = PyBytes_FromStringAndSize(NULL, out_len);
    if (result == NULL) {
        if (self->input_buffer != NULL) {
            PyMem_Free(self->input_buffer);
        }
        PyErr_NoMemory();
        goto error;
    }
    char *posi = PyBytes_AS_STRING(result);
    memcpy(posi, (const char*)in.buf, out_len);

    /* unconsumed input data */
    if (out_len == in.size) {
        // finished; release buffer
        PyMem_Free(self->input_buffer);
        self->in_begin = 0;
        self->in_end = 0;
        self->input_buffer = NULL;
        self->input_buffer_size = 0;
    } else {
        /* keep unconsumed data position */
        self->in_begin += out_len;
    }
    RELEASE_LOCK(self);
    return result;

    error:
    RELEASE_LOCK(self);
    return NULL;

}

static int
BCJEncoder_init(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    /* Only called once */
    if (self->inited) {
        PyErr_SetString(PyExc_RuntimeError, init_twice_msg);
        goto error;
    }
    self->inited = 1;
    self->method = x86;
    self->readahead = 5;
    self->is_encoder = True;
    return 0;

    error:
    return -1;
}

PyDoc_STRVAR(BCJEncoder_encode_doc,
"");

static PyObject *
BCJEncoder_encode(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"data", NULL};
    Py_buffer data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "y*:BCJEncoder.encode", kwlist,
                                     &data)) {
        return NULL;
    }
    PyObject* result = BCJFilter_do_filter(self, &data);
    PyBuffer_Release(&data);
    return result;
}

static int
BCJDecoder_init(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"size", NULL};
    int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "i:BCJDecoder.__init__", kwlist,
                                     &size)) {
        return -1;
    }

    /* Only called once */
    if (self->inited) {
        PyErr_SetString(PyExc_RuntimeError, init_twice_msg);
        goto error;
    }
    self->inited = 1;
    self->method = x86;
    self->readahead = 5;
    self->is_encoder = False;
    self->size = size;
    self->state = 0;
    return 0;

    error:
    return -1;
}

PyDoc_STRVAR(BCJDecoder_decode_doc,
"");

static PyObject *
BCJDecoder_decode(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    static char *kwlist[] = {"data", NULL};
    Py_buffer data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
                                     "y*:BCJDecoder.decode", kwlist,
                                     &data)) {
        return NULL;
    }
    PyObject* result = BCJFilter_do_filter(self, &data);
    PyBuffer_Release(&data);
    return result;
}

PyDoc_STRVAR(reduce_cannot_pickle_doc,
"Intentionally not supporting pickle.");

static PyObject *
reduce_cannot_pickle(PyObject *self) {
    PyErr_Format(PyExc_TypeError,
                 "Cannot pickle %s object.",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

/*  define class and methos */

/* BCJ encoder */
static PyMethodDef BCJEncoder_methods[] = {
        {"encode",     (PyCFunction) BCJEncoder_encode,
                             METH_VARARGS | METH_KEYWORDS, BCJEncoder_encode_doc},
        {"__reduce__", (PyCFunction) reduce_cannot_pickle,
                             METH_NOARGS,                  reduce_cannot_pickle_doc},
        {NULL,         NULL, 0,                            NULL}
};

static PyType_Slot BCJEncoder_slots[] = {
        {Py_tp_new,     BCJFilter_new},
        {Py_tp_dealloc, BCJFilter_dealloc},
        {Py_tp_init,    BCJEncoder_init},
        {Py_tp_methods, BCJEncoder_methods},
        {0,             0}
};

static PyType_Spec BCJEncoder_type_spec = {
        .name = "_bcj.BCJEncoder",
        .basicsize = sizeof(BCJFilter),
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .slots = BCJEncoder_slots,
};

/* BCJDecoder */
static PyMethodDef BCJDecoder_methods[] = {
        {"decode",     (PyCFunction) BCJDecoder_decode,
                             METH_VARARGS | METH_KEYWORDS, BCJDecoder_decode_doc},
        {"__reduce__", (PyCFunction) reduce_cannot_pickle,
                             METH_NOARGS,                  reduce_cannot_pickle_doc},
        {NULL,         NULL, 0,                            NULL}
};

static PyType_Slot BCJDecoder_slots[] = {
        {Py_tp_new,     BCJFilter_new},
        {Py_tp_dealloc, BCJFilter_dealloc},
        {Py_tp_init,    BCJDecoder_init},
        {Py_tp_methods, BCJDecoder_methods},
        {0,             0}
};

static PyType_Spec BCJDecoder_type_spec = {
        .name = "_bcj.BCJDecoder",
        .basicsize = sizeof(BCJFilter),
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
        .slots = BCJDecoder_slots,
};

/* --------------------
     Initialize code
   -------------------- */

static PyMethodDef _bcj_methods[] = {
        {NULL}
};


typedef struct {
    PyTypeObject *BCJEncoder_type;
    PyTypeObject *BCJDecoder_type;
} _bcj_state;

static _bcj_state static_state;

static int
_bcj_traverse(PyObject *module, visitproc visit, void *arg) {
    Py_VISIT(static_state.BCJEncoder_type);
    Py_VISIT(static_state.BCJDecoder_type);
    return 0;
}

static int
_bcj_clear(PyObject *module) {
    Py_CLEAR(static_state.BCJEncoder_type);
    Py_CLEAR(static_state.BCJDecoder_type);
    return 0;
}

static void
_bcj_free(void *module) {
    _bcj_clear((PyObject *) module);
}

static PyModuleDef _bcjmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_bcj",
        .m_size = -1,
        .m_methods = _bcj_methods,
        .m_traverse = _bcj_traverse,
        .m_clear = _bcj_clear,
        .m_free = _bcj_free
};


static inline int
add_type_to_module(PyObject *module, const char *name,
                   PyType_Spec *type_spec, PyTypeObject **dest) {
    PyObject *temp;

    temp = PyType_FromSpec(type_spec);
    if (PyModule_AddObject(module, name, temp) < 0) {
        Py_XDECREF(temp);
        return -1;
    }

    Py_INCREF(temp);
    *dest = (PyTypeObject *) temp;

    return 0;
}

PyMODINIT_FUNC
PyInit__bcj(void) {
    PyObject *module;

    module = PyModule_Create(&_bcjmodule);
    if (!module) {
        goto error;
    }

    if (add_type_to_module(module,
                           "BCJEncoder",
                           &BCJEncoder_type_spec,
                           &static_state.BCJEncoder_type) < 0) {
        goto error;
    }

    if (add_type_to_module(module,
                           "BCJDecoder",
                           &BCJDecoder_type_spec,
                           &static_state.BCJDecoder_type) < 0) {
        goto error;
    }

    return module;

    error:
    _bcj_clear(NULL);
    Py_XDECREF(module);

    return NULL;
}
