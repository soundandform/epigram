/*
 *  JdLua.cpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/8/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */

#include "JdLua.hpp"



int jdlua_newMetatable (lua_State * L, cstr_t i_name)
{
	int created = luaL_newmetatable (L, i_name);
	
	lua_pushstring (L, i_name);
	lua_setfield (L, -2, "__name");
	
	return created;
}


int  jdlua_checkForSelf  (lua_State * L, cstr_t i_libName)
{
	int index = 1;
	
	if (lua_type (L, 1) == LUA_TTABLE)
	{
		lua_getfield (L, 1, "__name");
		if (std::string_view (lua_tostring (L, -1)) == i_libName)
			index = 2;
		lua_pop (L, 1);
	}
	
	return index;
}


void  jdlua_removeSelf  (lua_State * L, cstr_t i_libName)
{
	if (lua_type (L, 1) == LUA_TTABLE)
	{
		lua_getfield (L, 1, "__name");
		
		bool hasSelfArg = std::string_view (lua_tostring (L, -1)) == i_libName;
		lua_pop (L, 1);
		
		if (hasSelfArg)
			lua_remove (L, 1);
	}
}


void  jdlua_ref  (lua_State * L, int & io_ref)
{														d_jdAssert (io_ref == 0);
	io_ref = luaL_ref (L, LUA_REGISTRYINDEX);
}


void  jdlua_unRef  (lua_State *L, int & io_ref)
{
	luaL_unref (L, LUA_REGISTRYINDEX, io_ref);
	io_ref = 0;
}


cstr_t  jdlua_getType (lua_State * L, int i_index)
{
	cstr_t name = nullptr;
	
	auto type = lua_type (L, i_index);
	
	if (type == LUA_TUSERDATA or type == LUA_TTABLE)
		name = jdlua_getUserdataName (L, i_index);

	if (not name)
		name = lua_typename (L, type);
	
	return name ? name : "";
}


cstr_t  jdlua_getUserdataName  (lua_State * L, int i_index)
{
	cstr_t name = nullptr;
	
	auto t = lua_gettop (L);

	if (lua_getmetatable (L, i_index))	// (returns success code; doesn't push nil to stack)
	{
		lua_pushstring (L, "__name");
		lua_rawget (L, -2);
		
		name = lua_tostring (L, -1);
	}
	
	lua_settop (L, t);

	return name;
}



f64  jdlua_popRealNumber  (lua_State * L, int i_argIndex)
{
	f64 v = lua_tonumber (L, -1);
	lua_pop (L, 1);

	jdlua_testForRealNumber (L, i_argIndex, v);
	
	return v;
}


int JdLua::HandleLuaError (lua_State * L)
{
//	jd::out (lua_gettop (L));
	
	string s = lua_tostring (L, -1);			// jd::out ("@\n-------------------", s);
	
	u32 level = 1;
	lua_Debug ar = {};

	lua_getfield (L, LUA_REGISTRYINDEX, "JdLua");
	auto jdlua = (JdLua *) lua_touserdata (L, -1);
	lua_pop (L, 1);
	
	if (jdlua)
	{
		// search the call stack until a file-based source is found
		while (lua_getstack (L, level, & ar))
		{
			if (lua_getinfo (L, "Sln", & ar))
			{
				if (ar.source)
				{
					// jd::out ("   @ @ @", ar.source, ar.currentline, ar.name ? ar.name : "");
					
					if (ar.source [0] == '@')
					{
						jdlua->m_errorLocation.push_back ({ ar.source, ar.currentline });
					}
				}
			}
			else break;
			
			++level;
		}
	}
	
	return 1;
}



JdLua::Result  JdLua::HashScript  (string_view i_mainName, JdMD5::MD5 & o_hash, string_view i_script, string_view i_scriptName, vector <u8> * o_bytecode)
{
	Result result;
	
	Initialize ();

	if (L)
	{
		i32 luaResult = (i_script [0] == '@') ? luaL_loadfile (L, i_script.substr (1).data ()) : luaL_loadbuffer (L, i_script.data (), i_script.length (), i_scriptName.data ());
		
		if (not luaResult)
		{
			ByteCodeWriter bcw (o_bytecode);
			lua_dumpx (L, & ByteCodeWriter::Handler, & bcw, 0x80000002);//  BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC
			
			o_hash = bcw.Get ();
		}
		else result = ParseErrorMessage (luaResult, i_mainName);
	}
	
	return result;
}


JdLua::Result  JdLua::CompileScript  (stringRef_t i_mainName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode)
{
	Result result;
	
	Initialize ();

	if (L)
	{
		int r;
		
		if (i_script [0] == '@')
			r = luaL_loadfile (L, i_script + 1);
		else
			r = luaL_loadstring (L, i_script);
		
		if (r)
		{
			result = ParseErrorMessage (r, i_mainName);
			if (io_hashCheck)
				io_hashCheck->Clear ();
		}
		else
		{
			if (io_hashCheck or o_bytecode)
			{
				ByteCodeWriter bcw (o_bytecode, io_hashCheck != nullptr);
				lua_dumpx (L, & ByteCodeWriter::Handler, & bcw, 0x80000002);//  BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC

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
		}
	}
	
	return result;
}


JdLua::Result  JdLua::LoadAndCallScript  (stringRef_t i_mainName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode)
{
	Result result = CompileScript (i_mainName, i_script, io_hashCheck, o_bytecode);
	
	if (not result)
	{
		int r = lua_pcall (L, 0, 0, 0);
		if (r)
		{
			result = ParseErrorMessage (r, i_mainName);
			if (io_hashCheck)
				io_hashCheck->Clear ();
		}
	}
	
	/*
	Result result;
	
	Initialize ();

	if (L)
	{
		int r = luaL_loadstring (L, i_script);
		
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
				lua_dumpx (L, & ByteCodeWriter::Handler, & bcw, 0x80000002);//  BCDUMP_F_STRIP + BCDUMP_F_DETERMINISTIC

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
	*/
	return result;
}



Epigram  JdLua::CallObject  (string_view i_functionName, Epigram && i_args, Result * o_result)
{
	Epigram returns;
	
	Result result;
	
	m_errorLocation.clear ();
	
	if (i_functionName.find (".") != string_view::npos or i_functionName.find (":") != string_view::npos)
	{
		string path (i_functionName);
		
		auto pos = path.find (":");
		if (pos != string::npos)
			path [pos] = '.';

		auto top = lua_gettop (L);

		if (FindFunctionInTable (0, path.c_str ()))
		{
			m_functionName = i_functionName;
			result = ExecuteFunction (nullptr, returns, i_args, -1);
			m_functionName.clear ();
		}

		lua_settop (L, top);
	}
//		else return ExecuteFunction (i_functionName, i_args, 0, o_result);
	
	if (o_result)
		* o_result = result;
	
	return returns;
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



void  JdLua::LuaArgsToEpigram  (Epigram & o_args, lua_State * L, i32 const i_startIndex)
{
	i32 index = i_startIndex;
	int type;

	while ((type = lua_type (L, index)) != LUA_TNONE)
	{
		i32 i = index - i_startIndex;
		
			 if (type == LUA_TSTRING)			o_args [i] = lua_tostring (L, index);
		else if (type == LUA_TNUMBER)			o_args [i] = lua_tonumber (L, index);
		else if (type == LUA_TTABLE)			o_args [i] = TableToEpigram (L, index);
			
		++index;
	}
	
//	o_args.dump ();
}



std::ostream & operator << (std::ostream & out, JdLua::Result const & i_result)
{
	out << "(" << i_result.resultCode << ") " << i_result.errorMsg;
	
	return out;
}
