//
//  Epilog_Cpp.hpp
//  Epilog
//
//  Created by Steven Massey on 1/27/14.
//
//

#include "Epilog.hpp"


#ifdef epilog
#undef epilog
#endif

#ifdef d_epilogObjC
#undef d_epilogObjC
#endif


#ifndef d_epilogCpp
#define d_epilogCpp

#define epilog(a_classification, ...) if (epg_LogDeferred) { char stack[1024]; epg_LogDeferred (c_epilogClassification_##a_classification, Jd::ParseObjectName (this), CreateEpilogEvent (stack, __VA_ARGS__)); }

#endif

