//
//  JdNucleus.hpp
//  Jigidesign
//
//  Created by Steven Massey on 2/28/15.
//  Copyright (c) 2015 Jigidesign. All rights reserved.
//

#ifndef Jigidesign_JdNucleus_h
#define Jigidesign_JdNucleus_h

#include <inttypes.h>
#include <string>

using std::pair;

typedef uint32_t u32; typedef int32_t i32;
typedef uint64_t u64; typedef int64_t i64;
typedef uint16_t u16; typedef int16_t i16;
typedef uint8_t  u8;  typedef int8_t  i8;

typedef float f32;

# if d_jdNoDoubleFP
	typedef float f64;
# else
	typedef double f64;
# endif

typedef const char * cstr_t;
typedef const char * utf8_t;
typedef const void * voidptr_t;
typedef const std::string & stringRef_t;


#define d_jdRef(CLASS) typedef const CLASS & CLASS##Ref;
#define d_jdStruct(STRUCT) struct STRUCT; typedef const STRUCT & STRUCT##Ref; struct STRUCT
#define d_jdClass(CLASS) class CLASS; typedef const CLASS & CLASS##Ref; class CLASS

#endif
