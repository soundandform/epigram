/*
 *  JdLua.cpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/8/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */

#include "JdLua.hpp"


std::atomic <u64> JdLua::s_sequenceNum;

int JdLua::HandleLuaError (lua_State * L)
{
	lua_Debug ar = {};
	
	string s = lua_tostring (L, -1);

	
	int r = lua_getstack (L, 1, & ar);	// 0 = this C function; +1 to get down into Lua
	
	if (r == 1)	// 1=ok. OK...
	{
		if (lua_getinfo (L, "S", & ar))
		{
			if (ar.source)
			{
				if (ar.source [0] == '@')	// if file, don't use short_src; otherwise filename can be truncated
				{
					string s = lua_tostring (L, -1);
					lua_pop (L, 1);
					s = s.substr (s.find (":"));
					
					lua_pushfstring (L, "%s%s", ar.source, s.c_str ());
				}
			}
		}
	}

	return 1;
}




JdLua::Result  JdLua::HashScript  (stringRef_t i_functionName, JdMD5::MD5 & o_hash, cstr_t i_script, vector <u8> * o_bytecode)
{
	Result result;
	
	Initialize ();

	if (L)
	{
		i32 luaResult = luaL_loadstring (L, i_script);
		
		if (not luaResult)
		{
			ByteCodeWriter bcw (o_bytecode);
			lua_dump (L, & ByteCodeWriter::Handler, & bcw); 	// IMPORTANTE: Need to add BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC flags to this function internally

			o_hash = bcw.Get ();
		}
		else result = ParseErrorMessage (luaResult, i_functionName);

//		lua_close (L);
//		L = nullptr;
	}
	
	return result;
}


JdLua::Result  JdLua::LoadAndCallScript  (stringRef_t i_functionName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode)
{
	Result result;
	
	auto script = PreserveString (i_script);
	
	Initialize ();

//	lua_pushcfunction (L, JdLua::HandleLuaError);

	if (L)
	{
		int r = luaL_loadstring (L, script);
		
		if (r)
		{
			result = ParseErrorMessage (r, i_functionName);
			if (io_hashCheck)
				io_hashCheck->Clear ();
		}
		else
		{
			if (io_hashCheck or o_bytecode)
			{														//JdStopwatch _ ("hash");
				ByteCodeWriter bcw (o_bytecode, io_hashCheck != nullptr);
				lua_dump (L, & ByteCodeWriter::Handler, & bcw); 	// IMPORTANTE: Need to add BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC flags to this function internally
				
				auto newHash = bcw.Get ();
				
				if (io_hashCheck)
				{
					if (memcmp (io_hashCheck, & newHash, sizeof (JdMD5::MD5)))
					{
						* io_hashCheck = newHash;
					}
					else return { c_luaHashUnchanged };
				}
			}
			
			int r = lua_pcall (L, 0, 0, 0);
			if (r)
			{
				result = ParseErrorMessage (r, i_functionName);
				if (io_hashCheck)
					io_hashCheck->Clear ();
			}
		}
	}
	
	return result;
}


vector <string>  JdLua::GetGlobalsOfType  (JdLua::Result & o_result, i32 i_luaType)
{
	vector <string> names;

	lua_getglobal	(L, "_G");

	int tableIndex = lua_gettop (L);
	
	if (lua_istable (L, tableIndex))
	{
		lua_pushnil (L);
		
		while (lua_next (L, tableIndex))
		{
			int keyType 	= lua_type (L, -2),
				valueType 	= lua_type (L, -1);
			
			if (keyType == LUA_TSTRING and valueType == i_luaType)
			{
				names.push_back (lua_tostring (L, -2));
			}
			
			lua_pop (L, 1);
		}

		lua_pop (L, 1);
	}
	else o_result = GenerateError (-1, "no global table _G found");

	return names;
}



vector <string>  JdLua::GetGlobalFunctions  (JdLua::Result & o_result)
{
	return GetGlobalsOfType (o_result, LUA_TFUNCTION);
}

