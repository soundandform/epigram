//
//  Epilog_ObjC.h
//  Epilog
//
//  Created by Steven Massey on 1/27/14.
//
//

#include "Epilog.hpp"

#ifdef epilog
#undef epilog
#endif

#ifndef d_epilogObjC
#define d_epilogObjC

#define epilog(a_classification, ...) if (epg_LogDeferred) { char stack[1024]; epg_LogDeferred (c_epilogClassification_##a_classification, Jd::ParseObjectName (self), CreateEpilogEvent (stack, __VA_ARGS__)); }

#endif


#ifdef d_epilogCpp
#undef d_epilogCpp
#endif

