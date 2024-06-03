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

		// closing lua because i_script isn't preserved, so shouldn't attempt to keep using this context
		lua_close (L);
		L = nullptr;
	}
	
	return result;
}


JdLua::Result  JdLua::LoadAndCallScript  (cstr_t i_script, JdMD5::MD5 * io_hashCheck)
{
	Result result;
	
	auto script = PreserveString (i_script);
	
	Initialize ();

	if (L)
	{
		int r = luaL_loadstring (L, script);
		
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


vector <string>  JdLua::GetGlobalFunctions  (JdLua::Result & result)
{
	vector <string> names;

	lua_getglobal	(L, "_G");

	int tableIndex = lua_gettop (L);
	
	if (lua_istable (L, tableIndex))
	{
		lua_pushnil (L);
		
		while (lua_next (L, tableIndex))
		{
			int keyType 	= lua_type (L, -2);
			int valueType 	= lua_type (L, -1);
			
			if (keyType == LUA_TSTRING and valueType == LUA_TFUNCTION)
			{
				names.push_back (lua_tostring (L, -2));
			}
			
			lua_pop (L, 1);
		}

		lua_pop (L, 1);
	}
	else result = { -1, "no global table" };

	return names;
}

