/*
 *  JdLua.cpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/8/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */

#include "JdLua.hpp"


JdLua::Result  JdLua::HashScript  (JdMD5::MD5 & o_hash, cstr_t i_script)
{
	Result result;
	
	Initialize ();

	if (L)
	{
		i32 luaResult = luaL_loadstring (L, i_script);
		
		if (not luaResult)
		{
			ByteCodeWriter bcw;
			lua_dump (L, & ByteCodeWriter::Handler, & bcw); 	// IMPORTANTE: Need to add BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC flags to this function internally
//			jd::out ("hash: @", bcw.GetString ());
			
			o_hash = bcw.Get ();
		}
		else result = ParseErrorMessage (luaResult);
	}
	
	return result;
}


JdLua::Result  JdLua::LoadScript  (cstr_t i_script, JdMD5::MD5 * io_hashCheck)
{
	Result result;
	
	Initialize ();

	if (L)
	{
		int r = luaL_loadstring (L, i_script);
		
		if (r)
		{
			result = ParseErrorMessage (r);
			if (io_hashCheck)
				io_hashCheck->Clear ();
		}
		else
		{
			if (io_hashCheck)
			{
				ByteCodeWriter bcw;
				lua_dump (L, & ByteCodeWriter::Handler, & bcw); 	// IMPORTANTE: Need to add BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC flags to this function internally
				
				auto newHash = bcw.Get ();
				
				if (memcmp (io_hashCheck, & newHash, sizeof (JdMD5::MD5)))
				{
					* io_hashCheck = newHash;
				}
				else return { 123456 };
			}
			
			int r = lua_pcall (L, 0, 0, 0);
			if (r)
			{
				result = ParseErrorMessage (r);
				if (io_hashCheck)
					io_hashCheck->Clear ();
			}
		}
	}
	
	return result;
}
