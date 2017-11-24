//
//  CriticalSection.mm
//  Playset
//
//  Created by Steven Massey on 9/13/12.
//  Copyright (c) 2012 Jiggydesign. All rights reserved.
//

#import "JdiOSPlatform.h"
#include "JdCore.hpp"

#import <Foundation/Foundation.h>

namespace JdPlatform
{
	template <> IJdPlatform::FileSystem * 		Get () { return Jd::AcquireSingleton <JdiOSPlatform::FileSystem> (); }
	template <> IJdPlatform::Notification * 	Get () { return Jd::AcquireSingleton <JdiOSPlatform::Notification> (); }
}

// FileSystem  ----------------------------------------------------------------------------------------------------------------------------------------------

using namespace IJdPlatform;

JdiOSPlatform::FileSystem::FileSystem ()
{
	/* Create /Library/AppData/ folder. All of /Documents is exposed for iTunes File Sharing, so we need to create a custom folder for app data.
	 iTunes supposedly still backs up stuff in the /Library folder. Though the actual iOS docs warn against putting user data in the /Library folder.
	 OK, whatever.

	 http://stackoverflow.com/questions/17553316/how-can-i-exclude-a-file-from-itunes-file-sharing
	 http://stackoverflow.com/questions/5243917/store-coredata-file-outside-of-documents-directory
	 
	 */
	
	NSString * libraryPath = [NSSearchPathForDirectoriesInDomains (NSLibraryDirectory, NSUserDomainMask, YES) lastObject];
	NSString * appDataPath = [libraryPath stringByAppendingPathComponent: @"AppData"];
	
	BOOL isDirectory;
	if (![[NSFileManager defaultManager] fileExistsAtPath: appDataPath isDirectory: &isDirectory])
	{
		NSError * error = nil;
		
		if (![[NSFileManager defaultManager] createDirectoryAtPath: appDataPath withIntermediateDirectories:YES attributes:nil error:&error])
		{
			d_jdThrow ("couldn't create /Library/AppData/ @", [error localizedDescription]);
		}
	}
	else if (!isDirectory) d_jdThrow ("/Library/AppData/ isn't a folder");
}

std::string JdiOSPlatform::FileSystem::GetSandboxPath (EJdFileSystemLocation i_location)
{
	std::string path;
	
	NSString *home = NSHomeDirectory ();
	
	// A little kludgey but it works for now... XCTests automatically uploads a stored set of app data for testing files, lists, etc., but
	// we need to redirect all access to persistent data to this directory...
	if (NSClassFromString(@"XCTestCase") != nil)
	{
		home = [home stringByAppendingString:@"/tmp/MVP Tests.xctest/MVP_test_data.xcappdata/AppData"];
	}
	
	path = [home UTF8String];
	path += "/";
	
	if		(i_location == c_jdFileSystemLocation::temporary)		path += "tmp/";
	else if (i_location == c_jdFileSystemLocation::appCache)		path += "Library/Caches/";
	else if (i_location == c_jdFileSystemLocation::userDocuments)	path += "Documents/";
	else if (i_location == c_jdFileSystemLocation::appData)			path += "Library/AppData/";
	else d_jdThrow ("unknown c_jdFileSystemLocation");
	
	return path;
}


u64 JdiOSPlatform::FileSystem::GetAvailableBytesInPath	(const std::string & i_path) // FIX: path not actually implemented but not (yet) relevant to iOS with only one storage device
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



u64 JdiOSPlatform::FileSystem::GetUsedBytesInPath	(const std::string & i_path)
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


std::string JdiOSPlatform::FileSystem::ResolveToFullPath (const std::string & i_path)
{
	std::string fullPath;
	
	char path [PATH_MAX+1];
	
	std::string pathL = i_path, pathR;
	
	while (true)
	{
//		cout << pathL << " | " << pathR << endl;
		
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


i32 JdiOSPlatform::FileSystem::RemoveFile (const std::string & i_path)
{
	return remove (i_path.c_str());
}


// Notifications -----------------------------------------------------------------------------------------------------------------------------------------------------------

// FIX: this stuff probably needs thread-safety... Or just never create a timer outside of the JdTimer module interface.


@implementation JdTimerBridgeCallbackWrapper

@end

@implementation JdTimerBridge


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
	JdTimerBridgeCallbackWrapper *info = [i_timer userInfo];
	
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
- (void *) ScheduleTimer: (int) i_milliseconds callback: (IJdCallback *) i_callback
{
	JdTimerBridgeCallbackWrapper * wrapper = [JdTimerBridgeCallbackWrapper alloc];
	wrapper->m_callback = i_callback;
	
	NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval: (float) i_milliseconds / 1000.0f  // seconds
													  target: self
													selector: @selector (timerFired:)
													userInfo: wrapper
													 repeats: YES];
	
	return (__bridge void *) timer;
	//	cout << "inserting timer!\n";
//	[m_timers insertObject: timer atIndex: [m_timers count] ];
	
	return (__bridge void *) timer;
}



JdTimerRef JdiOSPlatform::Notification::ScheduleTimer (int i_periodInMilliseconds, IJdCallback * i_callback, void * i_refData)
{
	if (! m_timerBridge) m_timerBridge = [[ JdTimerBridge alloc ] init];
	
	void * timer = [m_timerBridge ScheduleTimer: i_periodInMilliseconds callback: i_callback ];
	
	JdTimerRef ref = { timer };
	return ref;
}


void JdiOSPlatform::Notification::DestroyTimer (const JdTimerRef &i_timerRef)
{
	d_jdAssert (m_timerBridge, "obj-c timer bridge isn't initialized");
	
	[m_timerBridge StopTimer: (__bridge NSTimer *) i_timerRef.ref];
}

#include "EpigramAppleUtil.h"


void JdiOSPlatform::Notification::PostNotification (cstr_t i_name, JdId i_zone, EpDelivery i_message)
{
	string zonePlusName;
	
	// FIX: consider changing uuid string to NSObject
	
	if (i_zone.IsSet())
	{
		zonePlusName = Jd::ToString (i_zone);
		zonePlusName += ".";
	}
	
	zonePlusName += i_name;
	
	CFDictionaryRef dictionary = nullptr;
	if (i_message)
		dictionary = EpMsgToCFDictionary (i_message);
	
	NSString * name = [NSString stringWithUTF8String: zonePlusName.c_str()];
	NSNotification * notification = [NSNotification notificationWithName: name object: nil userInfo: (__bridge NSDictionary *) dictionary];
	[[NSNotificationCenter defaultCenter] postNotification: notification ];
	
	if (dictionary) CFRelease (dictionary);
}



@end

