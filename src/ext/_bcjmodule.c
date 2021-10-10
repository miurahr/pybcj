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

typedef struct {
    PyObject_HEAD

    /* Which variation used */
    enum Method method;

    /* Encoder/decoder configuration and status */
    UInt32 ip;
    UInt32 state;
    size_t readAhead;
    Bool isEncoder;

    /* thread lock */
    PyThread_type_lock lock;

    /* __init__ has been called, 0 or 1. */
    char inited;

    /* remaining data size when decode*/
    size_t remiaining;

    /* working buffer */
    Byte *buffer;
    SizeT bufSize;
    SizeT bufPos;
} BCJFilter;

/*
 * Common functions to initialize and dealloc.
 */
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

/*
 * Shared methods to process and flush.
 */
static SizeT
BCJFilter_do_method(BCJFilter *self) {
    SizeT outLen;

    if (self->bufSize == self->bufPos) {
        return 0;
    }

    Byte* buf = self->buffer + self->bufPos;
    SizeT size = self->bufSize - self->bufPos;

    switch (self->method) {
        case x86:
            outLen = x86_Convert(buf, size, self->ip, &self->state, self->isEncoder);
            break;
        case arm:
            outLen = ARM_Convert(buf, size, self->ip, self->isEncoder);
            break;
        case armt:
            outLen = ARMT_Convert(buf, size, self->ip, self->isEncoder);
            break;
        case ppc:
            outLen = PPC_Convert(buf, size, self->ip, self->isEncoder);
            break;
        case sparc:
            outLen = SPARC_Convert(buf, size, self->ip, self->isEncoder);
            break;
        case ia64:
            outLen = IA64_Convert(buf, size, self->ip, self->isEncoder);
            break;
        default:
            // should not come here.
            return 0;
    }
    self->ip += outLen;
    self->remiaining -= outLen;
    return outLen;
}

static PyObject *
BCJFilter_do_filter(BCJFilter *self, Py_buffer *data) {
    ACQUIRE_LOCK(self);

    if (self->bufPos == self->bufSize) {
        Byte* tmp = PyMem_Malloc(data->len);
        if (tmp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, data->buf, data->len);
        self->buffer = tmp;
        self->bufSize = data->len;
        self->bufPos = 0;
    } else if (data->len > 0) {
        size_t size = self->bufSize - self->bufPos;
        Byte *tmp;
        tmp = PyMem_Malloc(data->len + size);
        if (tmp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(tmp, self->buffer + self->bufPos, size);
        PyMem_Free(self->buffer);
        memcpy(tmp + size, data->buf, data->len);
        self->buffer = tmp;
        self->bufSize = size;
        self->bufPos = 0;
    }

    SizeT outLen = BCJFilter_do_method(self);
    if (self->remiaining <= self->readAhead) {
        // flush all the data
        outLen = self->bufSize;
    }

    PyObject *result = PyBytes_FromStringAndSize(NULL, outLen);
    if (result == NULL) {
        if (self->buffer != NULL) {
            PyMem_Free(self->buffer);
        }
        PyErr_NoMemory();
        goto error;
    }
    char *posi = PyBytes_AS_STRING(result);
    memcpy(posi, (const char*)(self->buffer + self->bufPos), outLen);

    /* unconsumed input data */
    if (outLen == self->bufSize - self->bufPos) {
        // finished; release buffer
        PyMem_Free(self->buffer);
        self->bufPos = 0;
        self->bufSize = 0;
    } else {
        /* keep unconsumed data position */
        self->bufPos += outLen;
    }
    RELEASE_LOCK(self);
    return result;

    error:
    RELEASE_LOCK(self);
    return NULL;

}

static PyObject *
BCJFilter_do_flush(BCJFilter *self) {
    PyObject *result;

    ACQUIRE_LOCK(self);
    if (self->bufPos == self->bufSize) {
        result = PyBytes_FromStringAndSize(NULL, 0);
    } else {
        SizeT out_len = BCJFilter_do_method(self);
        // override with all remaining data
        out_len = self->bufSize - self->bufPos;
        result = PyBytes_FromStringAndSize(NULL, out_len);
        if (result == NULL) {
            if (self->buffer != NULL) {
                PyMem_Free(self->buffer);
            }
            PyErr_NoMemory();
            goto error;
        }
        char *posi = PyBytes_AS_STRING(result);
        memcpy(posi, (const char *) self->buffer + self->bufPos, out_len);
    }
    RELEASE_LOCK(self);
    return result;

    error:
    RELEASE_LOCK(self);
    return NULL;
}

/*
 * BCJ(X86) Encoder.
 */
static int
BCJEncoder_init(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    /* Only called once */
    if (self->inited) {
        PyErr_SetString(PyExc_RuntimeError, init_twice_msg);
        goto error;
    }
    self->inited = 1;
    self->method = x86;
    self->readAhead = 5;
    self->isEncoder = True;
    self->remiaining = INT_MAX;
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

PyDoc_STRVAR(BCJEncoder_flush_doc,
"");

static PyObject *
BCJEncoder_flush(BCJFilter *self, PyObject *args, PyObject *kwargs) {
    PyObject* result = BCJFilter_do_flush(self);
    return result;
}

/*
 * BCJ(X86) Decoder.
 */
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
    self->readAhead = 5;
    self->isEncoder = False;
    self->remiaining = size;
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

/*
 * common function for python object.
 */

PyDoc_STRVAR(reduce_cannot_pickle_doc,
"Intentionally not supporting pickle.");

static PyObject *
reduce_cannot_pickle(PyObject *self) {
    PyErr_Format(PyExc_TypeError,
                 "Cannot pickle %s object.",
                 Py_TYPE(self)->tp_name);
    return NULL;
}

/*  define class and methods */

/* BCJ encoder */
static PyMethodDef BCJEncoder_methods[] = {
        {"encode",     (PyCFunction) BCJEncoder_encode,
                             METH_VARARGS | METH_KEYWORDS, BCJEncoder_encode_doc},
        {"flush",     (PyCFunction) BCJEncoder_flush,
                METH_VARARGS | METH_KEYWORDS, BCJEncoder_flush_doc},
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
