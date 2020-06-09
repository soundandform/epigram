//
//  EpigramAppleUtil.cpp
//  Jigidesign
//
//  Created by Steven Massey on 11/4/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#include "EpigramAppleUtil.h"

CFDictionaryRef EpigramToCFDictionary (EpDelivery i_msg)
{
	Epigram msg = i_msg;
	
	const u32 c_maxKeyValuePairs = 512;
	
	void * keys [c_maxKeyValuePairs];
	void * values [c_maxKeyValuePairs];
	
	u32 count = 0;
	
	for (auto & item : msg)
	{
		CFStringRef key = CFStringCreateWithCString (NULL, item.GetKeyString (), kCFStringEncodingASCII);
		keys [count] = (void *) key;
		
		if (item.Is <f64> ())
		{
			f64 f = item;
			CFNumberRef value = CFNumberCreate (NULL, kCFNumberDoubleType, &f);
			values [count] = (void *) value;
		}
		else if (item.Is <i32> ())
		{
			i32 i = item;
			CFNumberRef value = CFNumberCreate (NULL, kCFNumberSInt32Type, &i);
			values [count] = (void *) value;
		}
		else if (item.Is <u32> ())
		{
			u32 i = item;
			CFNumberRef value = CFNumberCreate (NULL, kCFNumberLongType, &i);
			values [count] = (void *) value;
		}
		else if (item.Is <string> ())
		{
			cstr_t string = item;
			CFStringRef value = CFStringCreateWithCString (NULL, string, kCFStringEncodingUTF8);
			values [count] = (void *) value;
		}
		else if (item.Is <JdUUID> ())
		{
			JdUUID uuid = item;
			
			CFUUIDBytes bytes;
			memcpy (&bytes, uuid.GetBytes(), sizeof (bytes));
			
			CFUUIDRef value = CFUUIDCreateFromUUIDBytes (NULL, bytes);
			
			values [count] = (void *) value;
		}
		else
		{
			values [count] = nullptr;
			d_jdThrow ("implement me!");
		}
		
		if (++count == c_maxKeyValuePairs)
			d_jdThrow ("EpigramToCFDictionary overflow");
	}
	
	CFDictionaryRef dictionary = CFDictionaryCreate (NULL, (const void **) keys, (const void **) values, count, & kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	
	for (u32 i = 0; i < count; ++i)
	{
		CFRelease (values [i]);
		CFRelease (keys [i]);
	}
	
	return dictionary;
}

