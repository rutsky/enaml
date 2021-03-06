/*-----------------------------------------------------------------------------
| Copyright (c) 2013, Nucleic Development Team.
|
| Distributed under the terms of the Modified BSD License.
|
| The full license is in the file COPYING.txt, distributed with this software.
|----------------------------------------------------------------------------*/
#include "inttypes.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include "pythonhelpers.h"

using namespace PythonHelpers;


typedef struct {
    PyObject_HEAD
    PyObject* tkdata;   // Toolkit specific color representation.
    uint32_t argb;      // Stored using Qt's #AARRGGBB byte order.
} Color;


static PyObject*
Color_new( PyTypeObject* type, PyObject* args, PyObject* kwargs )
{
    int32_t r = -1;
    int32_t g = -1;
    int32_t b = -1;
    int32_t a = 255;
    static char* kwlist[] = { "red", "green", "blue", "alpha", 0 };
    if( !PyArg_ParseTupleAndKeywords( args, kwargs, "|iiii", kwlist, &r, &g, &b, &a ) )
        return 0;
    PyObjectPtr colorptr( PyType_GenericNew( type, args, kwargs ) );
    if( !colorptr )
        return 0;
    Color* color = reinterpret_cast<Color*>( colorptr.get() );
    if( r < 0 || g < 0 || b < 0 || a < 0 )
        color->argb = 0;
    else
    {
        r = std::max( 0, std::min( r, 255 ) );
        g = std::max( 0, std::min( g, 255 ) );
        b = std::max( 0, std::min( b, 255 ) );
        a = std::max( 0, std::min( a, 255 ) );
        color->argb = static_cast<uint32_t>( ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b );
    }
    return colorptr.release();
}


static void
Color_dealloc( Color* self )
{
    Py_CLEAR( self->tkdata );
    Py_TYPE(self)->tp_free( reinterpret_cast<PyObject*>( self ) );
}


static PyObject*
Color_repr( Color* self )
{
    uint32_t a = ( self->argb >> 24 ) & 255;
    uint32_t r = ( self->argb >> 16 ) & 255;
    uint32_t g = ( self->argb >> 8 ) & 255;
    uint32_t b = self->argb & 255;
    std::ostringstream ostr;
    ostr << "Color(red=" << r << ", green=" << g << ", blue=" << b << ", alpha=" << a << ")";
    return PyUnicode_FromString(ostr.str().c_str());
}


static PyObject*
Color_get_alpha( Color* self, void* context )
{
    uint32_t a = ( self->argb >> 24 ) & 255;
    return PyLong_FromLong( a );
}


static PyObject*
Color_get_red( Color* self, void* context )
{
    uint32_t r = ( self->argb >> 16 ) & 255;
    return PyLong_FromLong( r );
}


static PyObject*
Color_get_green( Color* self, void* context )
{
    uint32_t g = ( self->argb >> 8 ) & 255;
    return PyLong_FromLong( g );
}


static PyObject*
Color_get_blue( Color* self, void* context )
{
    uint32_t b = self->argb & 255;
    return PyLong_FromLong( b );
}


static PyObject*
Color_get_argb( Color* self, void* context )
{
    return PyLong_FromUnsignedLong( self->argb );
}


static PyObject*
Color_get_tkdata( Color* self, void* context )
{
    if( !self->tkdata )
        Py_RETURN_NONE;
    Py_INCREF( self->tkdata );
    return self->tkdata;
}


static int
Color_set_tkdata( Color* self, PyObject* value, void* context )
{
    // don't let users do something silly which would require GC
    if( reinterpret_cast<PyObject*>( self ) == value )
        return 0;
    PyObject* old = self->tkdata;
    self->tkdata = value;
    Py_XINCREF( value );
    Py_XDECREF( old );
    return 0;
}


static PyGetSetDef
Color_getset[] = {
    { "alpha", ( getter )Color_get_alpha, 0,
      "Get the alpha value for the color." },
    { "red", ( getter )Color_get_red, 0,
      "Get the red value for the color." },
    { "green", ( getter )Color_get_green, 0,
      "Get the green value for the color." },
    { "blue", ( getter )Color_get_blue, 0,
      "Get the blue value for the color." },
    { "argb", ( getter )Color_get_argb, 0,
      "Get the color as an #AARRGGBB unsigned long." },
    { "_tkdata", ( getter )Color_get_tkdata, ( setter )Color_set_tkdata,
      "Get and set the toolkit specific color representation." },
    { 0 } // sentinel
};


PyTypeObject Color_Type = {
    PyVarObject_HEAD_INIT( &PyType_Type, 0 )
    "colorext.Color",                       /* tp_name */
    sizeof( Color ),                        /* tp_basicsize */
    0,                                      /* tp_itemsize */
    (destructor)Color_dealloc,              /* tp_dealloc */
    (printfunc)0,                           /* tp_print */
    (getattrfunc)0,                         /* tp_getattr */
    (setattrfunc)0,                         /* tp_setattr */
    0,                                      /* tp_reserved */
    (reprfunc)Color_repr,                   /* tp_repr */
    (PyNumberMethods*)0,                    /* tp_as_number */
    (PySequenceMethods*)0,                  /* tp_as_sequence */
    (PyMappingMethods*)0,                   /* tp_as_mapping */
    (hashfunc)0,                            /* tp_hash */
    (ternaryfunc)0,                         /* tp_call */
    (reprfunc)0,                            /* tp_str */
    (getattrofunc)0,                        /* tp_getattro */
    (setattrofunc)0,                        /* tp_setattro */
    (PyBufferProcs*)0,                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    0,                                      /* Documentation string */
    (traverseproc)0,                        /* tp_traverse */
    (inquiry)0,                             /* tp_clear */
    (richcmpfunc)0,                         /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    (getiterfunc)0,                         /* tp_iter */
    (iternextfunc)0,                        /* tp_iternext */
    (struct PyMethodDef*)0,                 /* tp_methods */
    (struct PyMemberDef*)0,                 /* tp_members */
    Color_getset,                           /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    (descrgetfunc)0,                        /* tp_descr_get */
    (descrsetfunc)0,                        /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)0,                            /* tp_init */
    (allocfunc)PyType_GenericAlloc,         /* tp_alloc */
    (newfunc)Color_new,                     /* tp_new */
    (freefunc)0,                            /* tp_free */
    (inquiry)0,                             /* tp_is_gc */
    0,                                      /* tp_bases */
    0,                                      /* tp_mro */
    0,                                      /* tp_cache */
    0,                                      /* tp_subclasses */
    0,                                      /* tp_weaklist */
    (destructor)0                           /* tp_del */
};

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyMethodDef
colorext_methods[] = {
    { 0 } // Sentinel
};


#if PY_MAJOR_VERSION >= 3

static int colorext_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int colorext_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "colorext",
        NULL,
        sizeof(struct module_state),
        colorext_methods,
        NULL,
        colorext_traverse,
        colorext_clear,
        NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_colorext(void)

#else
#define INITERROR return

PyMODINIT_FUNC
initcolorext(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *mod = PyModule_Create(&moduledef);
#else
    PyObject* mod = Py_InitModule( "colorext", colorext_methods );
#endif

    if( !mod )
        INITERROR;

    if( PyType_Ready( &Color_Type ) )
        INITERROR;

    Py_INCREF( ( PyObject* )( &Color_Type ) );
    PyModule_AddObject( mod, "Color", ( PyObject* )( &Color_Type ) );

#if PY_MAJOR_VERSION >= 3
    return mod;
#endif
}
