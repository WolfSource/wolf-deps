/*
    Project          : Wolf Engine. Copyright(c) Pooya Eimandar (http://PooyaEimandar.com) . All rights reserved.
    Source           : Please direct any bug to https://github.com/PooyaEimandar/Wolf.Engine/issues
    Website          : http://WolfSource.io
    Name             : py_rectangle.h
    Description      : The python exporter for w_rectangle structs
    Comment          :
 */

#ifdef __PYTHON__

#ifndef __PY_RECTANGLE_H__
#define __PY_RECTANGLE_H__

namespace pyWolf
{
    static void py_rectangle_export()
    {
        using namespace boost::python;
        using namespace wolf::system;
        
        class_<w_rectangle>("w_rectangle", init<>())
            .def_readwrite("left", &w_rectangle::left, "left")
            .def_readwrite("top", &w_rectangle::top, "top")
            .def_readwrite("right", &w_rectangle::right, "right")
            .def_readwrite("bottom", &w_rectangle::bottom, "bottom")
            ;
    }
}

#endif//__W_RECTANGLE_PY_H__

#endif//__PYTHON__