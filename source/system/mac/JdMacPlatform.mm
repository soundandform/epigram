//
//  File.cpp
//  Jigidesign
//
//  Created by Steven Massey on 5/7/13.
//  Copyright (c) 2013 Jigidesign. All rights reserved.
//

#include "JdMacPlatform.h"
#include "JdCore.hpp"
#include "JdAssert.hpp"

#import <Foundation/Foundation.h>

namespace JdPlatform
{
	template <> IJdPlatform::FileSystem * 		Get () { return Jd::AcquireSingleton <JdMacPlatform::FileSystem> (); }
	template <> IJdPlatform::Notification * 	Get () { return Jd::AcquireSingleton <JdMacPlatform::Notification> (); }
}

// FileSystem -----------------------------------------------------------------------------------------------------------------------------------------------------------

JdMacPlatform::FileSystem::FileSystem::FileSystem ()
{
	// create [home]/Library/Application Support/Epigram
	
	NSString * libraryPath = [NSSearchPathForDirectoriesInDomains (NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject];
	NSString * appDataPath = [libraryPath stringByAppendingPathComponent: @"Epigram"];
	
	BOOL isDirectory;
	if (![[NSFileManager defaultManager] fileExistsAtPath: appDataPath isDirectory: &isDirectory])
	{
		NSError * error = nil;
		
		if (![[NSFileManager defaultManager] createDirectoryAtPath: appDataPath withIntermediateDirectories:YES attributes:nil error:&error])
		{
			d_jdThrow ("couldn't create ~/Library/Application Support/Epigram/ @", [error localizedDescription]);
		}
	}
	else if (!isDirectory) d_jdThrow ("~/Library/Application Support/Epigram/ isn't a folder");
}


std::string JdMacPlatform::FileSystem::GetSandboxPath (EJdFileSystemLocation i_location)
{
//	return "./"; 	// for unit testing
	
	std::string path;
	
	//	NSArray *paths = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
	//	NSString *docsPath = [paths objectAtIndex: 0];
	
//	NSLibraryDirectory
	
	NSString * nsPath;
	
	if (i_location == c_jdFileSystemLocation::temporary)
	{
		nsPath = NSTemporaryDirectory ();
	}
	else if (i_location == c_jdFileSystemLocation::userDocuments)
	{
		NSArray * directories = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory,  NSUserDomainMask, true);
		
		if ([directories count])
		{
			nsPath = [directories objectAtIndex: 0];
		}
	}
	else if (i_location == c_jdFileSystemLocation::appPreferences)
	{
		NSArray * directories = NSSearchPathForDirectoriesInDomains (NSLibraryDirectory,  NSUserDomainMask, true);
		
		if ([directories count])
		{
			nsPath = [directories objectAtIndex: 0];
		}
		
		nsPath = [nsPath stringByAppendingPathComponent: @"Preferences"];
	}
	else if (i_location == c_jdFileSystemLocation::appData)
	{
		NSString * libraryPath = [NSSearchPathForDirectoriesInDomains (NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject];
		nsPath = [libraryPath stringByAppendingPathComponent: @"Epigram"];
	}
	else nsPath = NSHomeDirectory ();
	
	path = [nsPath UTF8String];
	path += "/";

	return path;
}


u64 JdMacPlatform::FileSystem::GetAvailableBytesInPath	(const std::string & i_path) // FIX: path not actually implemented but not (yet) relevant to iOS with only one storage device
{
    u64 totalSpace = 0, totalFreeSpace = 0;
	
    NSError * error = nil;
    NSArray * paths = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
    NSDictionary * dictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath: [paths lastObject] error: &error];
	
	if (dictionary)
	{
		NSNumber *fileSystemSizeInBytes = [dictionary objectForKey: NSFileSystemSize];
		NSNumber *freeFileSystemSizeInBytes = [dictionary objectForKey: NSFileSystemFreeSize];
		totalSpace = [fileSystemSizeInBytes unsignedLongLongValue];
		totalFreeSpace = [freeFileSystemSizeInBytes unsignedLongLongValue];
		//        NSLog(@"Memory Capacity of %llu MiB with %llu MiB Free memory available.", ((totalSpace/1024ll)/1024ll), ((totalFreeSpace/1024ll)/1024ll));
	} else {
		
		//      NSLog(@"Error Obtaining System Memory Info: Domain = %@, Code = %@", [error domain], [error code]);
	}
	
    return totalFreeSpace;
}



u64 JdMacPlatform::FileSystem::GetUsedBytesInPath	(const std::string & i_path)
{
	NSString *path = [NSString stringWithUTF8String: i_path.c_str()];
	
	NSArray *filesArray = [[NSFileManager defaultManager] subpathsOfDirectoryAtPath: path error: nil];
    NSEnumerator *filesEnumerator = [filesArray objectEnumerator];
    NSString *fileName;
    u64 fileSize = 0;
	
	while (fileName = [filesEnumerator nextObject])
	{
		NSDictionary *fileDictionary = [[NSFileManager defaultManager] attributesOfItemAtPath: [path stringByAppendingPathComponent: fileName] error: nil];
		fileSize += [fileDictionary fileSize];
	}
	
    return fileSize;
}


std::string JdMacPlatform::FileSystem::ResolveToFullPath (const std::string & i_path)
{
	std::string pathL = i_path, pathR, fullPath;
	
	while (true)
	{
		char path [PATH_MAX+1];
		const char *real = realpath (pathL.c_str(), path);
		if (real)
		{
			fullPath = real;
			fullPath += pathR;
			break;
		}
		
		size_t p = pathL.rfind ("/");
		if (p == std::string::npos) break;
		
		pathR = pathL.substr(p) + pathR;
		pathL = pathL.substr(0,p);
	}
	
	return fullPath;
}


i32 JdMacPlatform::FileSystem::RemoveFile (const std::string & i_path)
{
	return remove (i_path.c_str());
}


// Notifications -----------------------------------------------------------------------------------------------------------------------------------------------------------

// FIX: this stuff probably needs thread-safety... Or just never create a timer outside of the JdTimer module interface.


@implementation d_objC (JdTimerBridgeCallbackWrapper)

@end


@implementation d_objC (JdTimerBridge)


- (id) init
{
	self = [super init];
	if (self)
	{
		m_timers = [ NSMutableArray array ];
	}
	
	return self;
}


- (void) timerFired: (NSTimer *) i_timer
{
	d_objC (JdTimerBridgeCallbackWrapper) *info = [i_timer userInfo];
	
	bool repeat = info->m_callback->CallbackFired (0);
	
	if (! repeat)
	{
		[i_timer invalidate];
	}
}


- (void) StopTimer: (NSTimer *) i_timer
{
	//	[m_timers ]
	
	[i_timer invalidate];
}


// FIX: this needs a dataRef arg
- (void *) ScheduleTimer: (f64) i_seconds callback: (IJdCallback *) i_callback
{
	auto wrapper = [d_objC (JdTimerBridgeCallbackWrapper) alloc];
	wrapper->m_callback = i_callback;
	
	NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval: i_seconds
													  target: self
													selector: @selector (timerFired:)
													userInfo: wrapper
													 repeats: YES];
	
	return (__bridge void *) timer;
	//	cout << "inserting timer!\n";
	//	[m_timers insertObject: timer atIndex: [m_timers count] ];
	
	return (__bridge void *) timer;
}



JdTimerRef JdMacPlatform::Notification::ScheduleTimer (f64 i_periodInSeconds, IJdCallback * i_callback, void * i_refData)
{
	if (! m_timerBridge) m_timerBridge = [[ d_objC (JdTimerBridge) alloc ] init];
	
	void * timer = [m_timerBridge ScheduleTimer: i_periodInSeconds callback: i_callback ];
	
	JdTimerRef ref = { timer };
	return ref;
}


void JdMacPlatform::Notification::DestroyTimer (const JdTimerRef &i_timerRef)
{
	d_jdAssert (m_timerBridge, "obj-c timer bridge isn't initialized");
	
//	cout << "stopping: " << i_timerRef.ref << endl;
	[m_timerBridge StopTimer: (__bridge NSTimer *) i_timerRef.ref];
}


#include "EpigramAppleUtil.h"


void JdMacPlatform::Notification::PostNotification (cstr_t i_name, JdId i_zone, EpDelivery i_message)
{
	string zonePlusName;
	
	if (i_zone.IsSet())
	{
		zonePlusName = Jd::ToString (i_zone);
		zonePlusName += ".";
	}
	
	zonePlusName += i_name;
	
	CFDictionaryRef dictionary = nullptr;
	if (i_message)
		dictionary = EpigramToCFDictionary (i_message);
	
	NSString * name = [NSString stringWithUTF8String: zonePlusName.c_str()];
	NSNotification * notification = [NSNotification notificationWithName: name object: nil userInfo: (__bridge NSDictionary *) dictionary];
	[[NSNotificationCenter defaultCenter] postNotification: notification ];
	
	if (dictionary) CFRelease (dictionary);
}



@end

