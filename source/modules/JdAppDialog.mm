//
//  JdAppDialog.m
//  LuaParser
//
//  Created by Steven Massey on 3/22/17.
//  Copyright © 2017 Massey Plugins Inc. All rights reserved.
//


#include "JdAppDialog.hpp"

#include "JdModule.hpp"
#include "JdThread.hpp"

#import <Foundation/Foundation.h>
#include <AppKit/AppKit.h>



d_jdModule (JdAppDialog, IJdAppDialog)
{
	d_jdMethod	(u8,		DisplayAlert,		string message; string header = "Alert"; string button1 = "OK"; string button2= "Cancel")
	{
		Jd::EnforceMainThread ();
		
		NSAlert * alert = [[NSAlert alloc] init];
		
		NSString * msgOne = [[NSString alloc] initWithUTF8String: i_.button1.c_str ()];
		[alert addButtonWithTitle: msgOne];

		if (i_.button2.size ())
		{
			NSString * msgTwo = [[NSString alloc] initWithUTF8String: i_.button2.c_str ()];
			[alert addButtonWithTitle: msgTwo];
		}

		NSString * header = [[NSString alloc] initWithUTF8String: i_.header.c_str ()];
		[alert setMessageText: header];
		
		NSString * infoText = [[NSString alloc] initWithUTF8String: i_.message.c_str ()];
		
		[alert setInformativeText: infoText];
		
		[alert setAlertStyle: NSWarningAlertStyle];

//		[alert beginSheetModalForWindow: [NSApp mainWindow] completionHandler:<#^(NSModalResponse returnCode)handler#>]
		
		if ([alert runModal] == NSAlertFirstButtonReturn)
		{
			return 1;
		}
		else return 2;
	}
};



//--------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "JdLibrary.h"

d_jdLibrary (AppDialog)
{
	void Definition ()
	{
		Module	<JdAppDialog>		(c_jdCreationPolicy::singleton);
	}
};
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
