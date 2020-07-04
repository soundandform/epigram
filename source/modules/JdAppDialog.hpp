//
//  JdAppDialog.h
//  LuaParser
//
//  Created by Steven Massey on 3/22/17.
//  Copyright © 2017 Massey Plugins Inc. All rights reserved.
//

#ifndef JdAppDialog_h
#define JdAppDialog_h

#include "JdInterface.h"


d_jdModuleInterface (IJdAppDialog, 1.0)
{
	d_jdMethodDecl	(u8,		DisplayAlert,		string message; string header = "Alert"; string button1 = "OK"; string button2= "Cancel")
};

d_jdLinkDef (AppDialog)

#endif /* JdAppDialog_h */
