/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 sw=4 noet ai cindent :
 *
 * Copyright (C) 2005  Johan Dahlin <johan@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "bank.h"
#include <pygobject.h>

GArgument
pyg_argument_from_pyobject(PyObject *object, GITypeInfo *type_info)
{
    GArgument arg;
    GITypeTag type_tag;
    GIBaseInfo* interface_info;
    GIInfoType interface_type;

    type_tag = g_type_info_get_tag((GITypeInfo*)type_info);
    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        /* Nothing to do */
        break;
    case GI_TYPE_TAG_UTF8:
        if (object == Py_None)
            arg.v_pointer = NULL;
        else
            arg.v_pointer = PyString_AsString(object);
        break;
    case GI_TYPE_TAG_UINT8:
        arg.v_uint8 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT:
        arg.v_uint = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT16:
        arg.v_uint16 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_UINT32:
        arg.v_uint32 = PyLong_AsLongLong(object);
        break;
    case GI_TYPE_TAG_UINT64:
        if (PyInt_Check(object)) {
            PyObject *long_obj = PyNumber_Long(object);
            arg.v_uint64 = PyLong_AsUnsignedLongLong(long_obj);
            Py_DECREF(long_obj);
        } else
            arg.v_uint64 = PyLong_AsUnsignedLongLong(object);
        break;
    case GI_TYPE_TAG_INT8:
        arg.v_int8 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT:
        arg.v_int = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_LONG:
        arg.v_long = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_ULONG:
        arg.v_ulong = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_BOOLEAN:
        arg.v_boolean = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT16:
        arg.v_int16 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT32:
        arg.v_int32 = PyInt_AsLong(object);
        break;
    case GI_TYPE_TAG_INT64:
        arg.v_int64 = PyLong_AsLongLong(object);
        break;
    case GI_TYPE_TAG_FLOAT:
        arg.v_float = (float)PyFloat_AsDouble(object);
        break;
    case GI_TYPE_TAG_DOUBLE:
        arg.v_double = PyFloat_AsDouble(object);
        break;
    case GI_TYPE_TAG_INTERFACE:
        interface_info = g_type_info_get_interface(type_info);
        interface_type = g_base_info_get_type(interface_info);
        if (interface_type == GI_INFO_TYPE_ENUM) {
            arg.v_int = PyInt_AsLong(object);
        } else if (object == Py_None)
            arg.v_pointer = NULL;
        else
            arg.v_pointer = pygobject_get(object);
        break;
    case GI_TYPE_TAG_ARRAY:
        arg.v_pointer = NULL;
        break;
    case GI_TYPE_TAG_ERROR:
        /* Allow NULL GError, otherwise fall through */
        if (object == Py_None) {
            arg.v_pointer = NULL;
            break;
        }
    default:
        g_print("<PyO->GArg> GITypeTag %s is unhandled\n",
                g_type_tag_to_string(type_tag));
        break;
    }

    return arg;
}

static PyObject *
glist_to_pyobject(GITypeTag list_tag, GITypeInfo *type_info, GList *list, GSList *slist)
{
    PyObject *py_list;
    int i;
    GArgument arg;
    PyObject *child_obj;

    if ((py_list = PyList_New(0)) == NULL) {
        g_list_free(list);
        return NULL;
    }
    i = 0;
    if (list_tag == GI_TYPE_TAG_GLIST) {
        for ( ; list != NULL; list = list->next) {
            arg.v_pointer = list->data;

            child_obj = pyg_argument_to_pyobject(&arg, type_info);

            if (child_obj == NULL) {
                g_list_free(list);
                Py_DECREF(py_list);
                return NULL;
            }
            PyList_Append(py_list, child_obj);
            Py_DECREF(child_obj);

            ++i;
        }
    } else {
        for ( ; slist != NULL; slist = slist->next) {
            arg.v_pointer = slist->data;

            child_obj = pyg_argument_to_pyobject(&arg, type_info);

            if (child_obj == NULL) {
                g_list_free(list);
                Py_DECREF(py_list);
                return NULL;
            }
            PyList_Append(py_list, child_obj);
            Py_DECREF(child_obj);

            ++i;
        }
    }
    g_list_free(list);
    return py_list;
}

PyObject *
pyarray_to_pyobject(gpointer array, int length, GITypeInfo *type_info)
{
    PyObject *py_list;
    PyObject *child_obj;
    GITypeInfo *element_type = g_type_info_get_param_type (type_info, 0);
    GITypeTag type_tag = g_type_info_get_tag(element_type);
    gsize size;
    char buf[256];
    int i;

    if (array == NULL)
        return Py_None;

    // FIXME: Doesn't seem right to have this here:
    switch (type_tag) {
    case GI_TYPE_TAG_INT:
        size = sizeof(int);
        break;
    case GI_TYPE_TAG_INTERFACE:
        size = sizeof(gpointer);
        break;
    default:
        snprintf(buf, sizeof(buf), "Unimplemented type: %s\n", g_type_tag_to_string(type_tag));
        PyErr_SetString(PyExc_TypeError, buf);
        return NULL;
    }

    if ((py_list = PyList_New(0)) == NULL) {
        return NULL;
    }

    for( i = 0; i < length; i++ ) {
        gpointer current_element = array + i * size;

        child_obj = pyg_argument_to_pyobject((GArgument *)&current_element, element_type);
        if (child_obj == NULL) {
            Py_DECREF(py_list);
            return NULL;
        }
        PyList_Append(py_list, child_obj);
        Py_DECREF(child_obj);
    }

    return py_list;
}

PyObject *
pyg_argument_to_pyobject(GArgument *arg, GITypeInfo *type_info)
{
    GITypeTag type_tag;
    PyObject *obj;
    GIBaseInfo* interface_info;
    GIInfoType interface_type;
    GITypeInfo *param_info;

    g_return_val_if_fail(type_info != NULL, NULL);
    type_tag = g_type_info_get_tag(type_info);

    switch (type_tag) {
    case GI_TYPE_TAG_VOID:
        // TODO: Should we take this as a buffer?
        g_warning("pybank doesn't know what to do with void types");
        obj = Py_None;
        break;
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
        param_info = g_type_info_get_param_type(type_info, 0);
        g_assert(param_info != NULL);
        obj = glist_to_pyobject(type_tag,
                                param_info,
                                type_tag == GI_TYPE_TAG_GLIST ? arg->v_pointer : NULL,
                                type_tag == GI_TYPE_TAG_GSLIST ? arg->v_pointer : NULL);
        break;
    case GI_TYPE_TAG_BOOLEAN:
        obj = PyBool_FromLong(arg->v_boolean);
        break;
    case GI_TYPE_TAG_UINT8:
        obj = PyInt_FromLong(arg->v_uint8);
        break;
    case GI_TYPE_TAG_UINT:
        obj = PyInt_FromLong(arg->v_uint);
        break;
    case GI_TYPE_TAG_UINT16:
        obj = PyInt_FromLong(arg->v_uint16);
        break;
    case GI_TYPE_TAG_UINT32:
        obj = PyLong_FromLongLong(arg->v_uint32);
        break;
    case GI_TYPE_TAG_UINT64:
        obj = PyLong_FromUnsignedLongLong(arg->v_uint64);
        break;
    case GI_TYPE_TAG_INT:
        obj = PyInt_FromLong(arg->v_int);
        break;
    case GI_TYPE_TAG_LONG:
        obj = PyInt_FromLong(arg->v_long);
        break;
    case GI_TYPE_TAG_ULONG:
        obj = PyInt_FromLong(arg->v_ulong);
        break;
    case GI_TYPE_TAG_INT8:
        obj = PyInt_FromLong(arg->v_int8);
        break;
    case GI_TYPE_TAG_INT16:
        obj = PyInt_FromLong(arg->v_int16);
        break;
    case GI_TYPE_TAG_INT32:
        obj = PyInt_FromLong(arg->v_int32);
        break;
    case GI_TYPE_TAG_INT64:
        obj = PyLong_FromLongLong(arg->v_int64);
        break;
    case GI_TYPE_TAG_FLOAT:
        obj = PyFloat_FromDouble(arg->v_float);
        break;
    case GI_TYPE_TAG_DOUBLE:
        obj = PyFloat_FromDouble(arg->v_double);
        break;
    case GI_TYPE_TAG_UTF8:
        if (arg->v_string == NULL)
            obj = Py_None;
        else
            obj = PyString_FromString(arg->v_string);
        break;
    case GI_TYPE_TAG_INTERFACE:
        interface_info = g_type_info_get_interface(type_info);
        interface_type = g_base_info_get_type(interface_info);

        if (interface_type == GI_INFO_TYPE_STRUCT || interface_type == GI_INFO_TYPE_BOXED) {
            // Create new struct based on arg->v_pointer
            const gchar *module_name = g_base_info_get_namespace(interface_info);
            const gchar *type_name = g_base_info_get_name(interface_info);
            PyObject *module = PyImport_ImportModule(module_name);
            PyObject *tp = PyObject_GetAttrString(module, type_name);
            gsize size;
            PyObject *buffer;
            PyObject **dict;

            if (tp == NULL) {
                char buf[256];
                snprintf(buf, sizeof(buf), "Type %s.%s not defined", module_name, type_name);
                PyErr_SetString(PyExc_TypeError, buf);
                return NULL;
            }

            obj = PyObject_GC_New(PyObject, (PyTypeObject *) tp);
            if (obj == NULL)
                return NULL;

            // FIXME: Any better way to initialize the dict pointer?
            dict = (PyObject **) ((char *)obj + ((PyTypeObject *) tp)->tp_dictoffset);
            *dict = NULL;

            size = g_struct_info_get_size ((GIStructInfo*)interface_info);
            buffer = PyBuffer_FromReadWriteMemory(arg->v_pointer, size);
            if (buffer == NULL)
                return NULL;

            PyObject_SetAttrString(obj, "__buffer__", buffer);

        } else if (interface_type == GI_INFO_TYPE_ENUM) {
	    obj = PyInt_FromLong(arg->v_int);
        } else if ( arg->v_pointer == NULL ) {
            obj = Py_None;
        } else {
            GValue value;
            GObject* gobj = arg->v_pointer;
            GType gtype = G_OBJECT_TYPE(gobj);
            GIRepository *repo = g_irepository_get_default();
            GIBaseInfo *object_info = g_irepository_find_by_gtype(repo, gtype);
            const gchar *module_name;
            const gchar *type_name;

            if (object_info != NULL) {
                // It's a pybank class, we should make sure it is initialized.

                module_name = g_base_info_get_namespace(object_info);
                type_name = g_base_info_get_name(object_info);

                // This will make sure the wrapper class is registered.
                char buf[250];
                snprintf(buf, sizeof(buf), "%s.%s", module_name, type_name);
                PyRun_SimpleString(buf);
            }

            value.g_type = gtype;
            value.data[0].v_pointer = gobj;
            obj = pyg_value_as_pyobject(&value, FALSE);
        }
        break;
    case GI_TYPE_TAG_ARRAY:
        g_warning("pyg_argument_to_pyobject: use pyarray_to_pyobject instead for arrays");
        obj = Py_None;
        break;
    default:
        g_print("<GArg->PyO> GITypeTag %s is unhandled\n",
                g_type_tag_to_string(type_tag));
        obj = PyString_FromString("<unhandled return value!>"); /*  */
        break;
    }

    if (obj != NULL)
        Py_INCREF(obj);

    return obj;
}


