/*
 *  JdLua.hpp
 *  Jigidesign
 *
 *  Created by Steven Massey on 2/8/12.
 *  Copyright 2012 Jigidesign. All rights reserved.
 *
 */

#ifndef JdLua_hpp
#define JdLua_hpp


#include "JdStopwatch.hpp"
#include "lua.hpp"


class JdLua
{
	public:					JdLua						()
	{
		m_lua = luaL_newstate ();
		luaL_openlibs (m_lua);
	}
	
	
	virtual					~JdLua						()
	{
		lua_close (m_lua);
	}
	
	
	
	JdResult				LoadScriptFromFile			(stringRef_t i_path)
	{
		JdResult result;
		
		if (m_lua)
		{
			m_scriptPath = i_path;
			
			int luaResult = luaL_loadfile (m_lua, i_path. c_str ());
			
			if (luaResult)  result = GetErrorMessage ();
			else            lua_pcall (m_lua, 0, 0, 0);
		}
		else result = d_jdError ("null lua state");
		
		return result;
	}
	
	
	JdResult				LoadScript					(stringRef_t i_script)
	{
		JdResult result;
		
		if (m_lua)
		{
			i32 luaResult = luaL_loadstring (m_lua, i_script.c_str ());
			
			if (luaResult)
			{
				result = GetErrorMessage ();
			}
			else
			{
				luaResult = lua_pcall (m_lua, 0, 0, 0);
				if (luaResult)
				{
					result = GetErrorMessage ();
				}
			}
		}
		
		return result;
	}
	
	
	Epigram               CallFunction            (stringRef_t i_functionName, Epigram i_args = Epigram ())
	{
		Epigram result;
		
		//		i_args.dump ();
		
		cstr_t functionName = i_functionName.c_str ();
		
		if (strstr (functionName, ".") or strstr (functionName, ":"))
		{
			EpDelivery result;
			
			string path = i_functionName;
			
			bool isObjectCall = false;
			
			auto pos = path.find (":");
			if (pos != string::npos)
			{
				path [pos] = '.';
				isObjectCall = true;
			}
			
			FindFunctionInTable (0, path.c_str(), i_args, &result, isObjectCall);
			
			return result;
		}
		else return ExecuteFunction (i_functionName, i_args, 0);
	}
	
	
	Epigram					GetGlobalTable			(stringRef_t i_tableName)
	{
		Epigram e;
		
		lua_getglobal (m_lua, i_tableName.c_str());
		
		if (lua_type (m_lua, -1) == LUA_TTABLE)
		{
			ConvertLuaTableToEpigram (lua_gettop (m_lua), e);
		}
		
		lua_pop (m_lua, 1);
		
		return e;
	}
	
	protected:
	
	
	JdResult                    GetErrorMessage         ()
	{
		JdResult error;
		
		if (m_lua)
		{
			if (lua_isstring (m_lua, -1))
			{
				std::string s = lua_tostring (m_lua, -1);
				lua_pop (m_lua, 1);
				
//				cout << s << endl;
				
				size_t p = s.find (":");
				
				std::string location;
				
				u32 lineNum = -1;
				if (p != std::string::npos)
				{
					location = s.substr(0, p);
					
					std::string line = s.substr (p + 1);
					
					s = line.substr (line.find (":") + 2);
					
					line = line.substr (0, line.find (":"));
					
					sscanf (line.c_str(), "%ud", &lineNum);
				}
				
				error = JdResult (s.c_str(), nullptr, lineNum, true);
			}
		}
		else error = d_jdError ("null lua");
		
		return error;
	}
	
	bool                        HasGlobal               (cstr_t i_name, i32 i_luaType)
	{
		bool exists = false;
		if (m_lua)
		{
			lua_getglobal (m_lua, i_name);
			
			exists = (lua_type (m_lua, -1) == i_luaType);
			lua_pop (m_lua, 1);
		}
		
		return exists;
	}
	
	
	bool                        FindFunctionInTable         (i32 i_tableIndex, cstr_t i_path, EpDelivery i_args, EpDelivery *o_result,
															 bool i_isObjectCall, i32 i_depth = 0)
	{
		if (! m_lua) return false;
		
		bool found = false;
		
		cstr_t dot = strstr (i_path, ".");
		
		string searchFor;
		string subPath;
		
		if (dot)
		{
			string path = i_path;
			searchFor = path.substr (0, dot - i_path);
			subPath = dot + 1;
			
			//			cout << "searchFor: " << searchFor << endl;
			//			cout << "subPath: " << subPath << endl;
			
			if (i_depth == 0)
				
			{
				i32 top = lua_gettop (m_lua);
				//                cout << "top: " << top << endl;
				
				//                cout << "root table: " << searchFor << endl;
				lua_getglobal (m_lua, searchFor.c_str());
				
				found = FindFunctionInTable (lua_gettop (m_lua), subPath.c_str(), i_args, o_result, i_isObjectCall, i_depth + 1);
				//                d_mpAssert (lua_gettop (m_lua) == top, "stack whacked");
				
				lua_pop (m_lua, 1);
				
				top = lua_gettop (m_lua);
				//                cout << "end-top: " << top << endl;
				
				return found;
			}
		}
		else searchFor = i_path;
		
		//        cout << "find: " << searchFor << " in " << i_tableIndex << endl;
		//		cout << "tableindex: " << i_tableIndex << endl;
		
		if (lua_istable (m_lua, i_tableIndex))
		{
			lua_pushnil (m_lua);
			
			while (lua_next (m_lua, i_tableIndex))
			{
				int keyType = lua_type (m_lua, -2);
				int valueType = lua_type (m_lua, -1);
				
				if (keyType == LUA_TSTRING)
				{
					string key = lua_tostring (m_lua, -2);
					
					//					cout << "searching for " << searchFor << " : " << key << "....\n";
					
					if (key == searchFor)
					{
						//                        cout << "found\n";
						
						if (valueType == LUA_TTABLE)
						{
							found = FindFunctionInTable (lua_gettop (m_lua), subPath.c_str (), i_args, o_result, i_isObjectCall, i_depth + 1);
						}
						else if (valueType == LUA_TFUNCTION)
						{
							d_jdAssert (subPath.size() == 0, "found a function when looking for a table");
							
							i32 objectIndex = 0;
							if (i_isObjectCall)
								objectIndex = i_tableIndex;
							
							if (o_result) *o_result = ExecuteFunction (nullptr, i_args, objectIndex);
							
							//                            cout << "function\n";
							found = true;
						}
						
						lua_pop (m_lua, 2);
						break;
					}
				}
				
				lua_pop (m_lua, 1); // pop value
			}
			
			if (! found)	// look through metatable
			{
				if (lua_getmetatable (m_lua, i_tableIndex))
				{
					i32 metatable = lua_gettop (m_lua);
					
					//					cout << "searching metatable...\n";
					lua_pushnil (m_lua);
					
					while (lua_next (m_lua, metatable))
					{
						int keyType = lua_type (m_lua, -2);
						int valueType = lua_type (m_lua, -1);
						
						if (keyType == LUA_TSTRING)
						{
							string key = lua_tostring (m_lua, -2);
							
							//							cout << "searching  metatable for " << searchFor << " : " << key << "....\n";
							
							if (key == "__index")
							{
								if (valueType == LUA_TTABLE)
								{
									found = FindFunctionInTable (lua_gettop (m_lua), searchFor.c_str(), i_args, o_result, i_isObjectCall, i_depth + 1);
								}
								
								//								else if (valueType == LUA_TFUNCTION)
								//								{
								//									d_mpAssert (subPath.size() == 0, "found a function when looking for a table");
								//
								//									if (o_result) *o_result = ExecuteFunction (nullptr, i_args);
								//
								//									//                            cout << "function\n";
								//									found = true;
								//								}
								//
								lua_pop (m_lua, 2);
								break;
							}
						}
						
						lua_pop (m_lua, 1); // pop value
					}
					
					lua_pop (m_lua, 1); // pop metatable
				}
			}
		}
		
		return found;
	}
	
	int                         PushTableFromEpigram      (EpigramRef i_msg)
	{
		if (m_lua)
		{
			lua_newtable (m_lua);
			
			//        for (i32 i = 0; i_msg [i].IsSet(); ++i)
			for (auto & i : i_msg)
			{
				//				cout << Jd::TypeIdToChar (i.GetKeyTypeId ());
				
				if (Jd::IsIntegerType (i.GetKeyTypeId ()))
				{
					d_jdThrow ("fix");
//					i64 key = i.Key <i64> ();
//					lua_pushnumber (m_lua, key);
					
				}
				else if (i.HasKeyType (c_jdTypeId::string))
				{
					cstr_t name = i.GetKeyString ();
					lua_pushstring (m_lua, name);
				}
				else continue;
				
				if (i.Is <string> ())		lua_pushstring (m_lua, i.To <cstr_t>());
				else						lua_pushnumber (m_lua, i);
				
				lua_settable (m_lua, -3);
			}
		}
		
		return 0;
	}
	
	
private: //--------------------------------------------------------------------------------------------------------------------------------
	
	Epigram               ExecuteFunction         (stringRef_t i_functionName, EpigramRef i_args, i32 i_selfIndex)
	{
		Epigram result;
		
		if (m_lua)
		{
			
			int top = lua_gettop (m_lua);
			
			if (i_functionName.size ())
			{
				//            cout << "CALL: [" << i_functionName << "] " << endl;
				lua_getglobal (m_lua, i_functionName.c_str ());
			}
			
			if (lua_isfunction (m_lua, -1))
			{
				
				i32 numCallingArgs = 0;
				
				if (i_selfIndex)
				{
					++numCallingArgs;
					if (lua_istable (m_lua, i_selfIndex))
					{
						lua_pushvalue (m_lua, i_selfIndex);
					}
				}
				
				if (i_args.HasElements ())
				{
					++numCallingArgs;  // 1 lua table
					
					PushTableFromEpigram (i_args);
				}
				
				int luaResult = lua_pcall (m_lua, numCallingArgs, 1, 0);
				if (not luaResult)
				{
					int t = lua_gettop (m_lua);
					
					int type = lua_type (m_lua, t);
					
					//				cout << "type: " << type << endl;
					
					if		(type == LUA_TTABLE)	ConvertLuaTableToEpigram (t, result);
					else if (type == LUA_TSTRING)   result (lua_tostring (m_lua, -1));
					else if (type == LUA_TBOOLEAN)  result ((bool) lua_toboolean (m_lua, -1));
					else if (type == LUA_TNUMBER)   result (lua_tonumber (m_lua, -1));
					else if (type == LUA_TNIL or
							 type == LUA_TLIGHTUSERDATA or
							 type == LUA_TFUNCTION or
							 type == LUA_TUSERDATA or
							 type == LUA_TTHREAD)	result ("warning", "unimplemented Lua return type");
					else d_jdThrow ("unknown return type");
				}
				else
				{
					//cout << "result: " << result << endl;
					JdResult msg = GetErrorMessage ();
					result ("error", msg.Message());
				}
				
				lua_settop (m_lua, top);
			}
			else result (d_jdError ("no lua function to call"));
		}
		else result (d_jdError ("lua state is null"));
		
		return result;
	}
	
	
	void                        ConvertLuaTableToEpigram (i32 i_tableIndex, Epigram &o_msg, u32 depth = 0)
	{
		if (!m_lua)	return;
		
		if (lua_istable (m_lua, i_tableIndex))
		{
			lua_pushnil (m_lua);
			
			// map <i32, string> strings;
			// map <i32, f64> doubles;
			
			vector <string> strings;
			vector <f64> doubles;
			
			Epigram tempMsg;
			
			
			while (lua_next (m_lua, i_tableIndex))
			{
				//                for (int i = 0; i < depth * 4; ++i) cout << " ";
				
				int keyType = lua_type (m_lua, -2);
				int valueType = lua_type (m_lua, -1);
				
				if (keyType == LUA_TNUMBER)
				{
					auto index64 = lua_tointeger (m_lua, -2);
					
					d_jdAssert (index64 >= std::numeric_limits <i32>::lowest () and index64 <= std::numeric_limits <i32>::max (), "lua index out of 32-bit range");
					
					i32 index = (i32) index64;
					
					// this assumes that indicies are sorted
					
					if (valueType == LUA_TSTRING)
					{
						string s = lua_tostring (m_lua, -1);
						strings.push_back (s);
						
						tempMsg (index, s);
					}
					else if (valueType == LUA_TNUMBER)
					{
						f64 f = lua_tonumber (m_lua, -1);
						doubles.push_back (f);
						tempMsg (index, f);
					}
					else if (valueType == LUA_TTABLE)
					{
						Epigram archive;
						ConvertLuaTableToEpigram (lua_gettop (m_lua), archive, depth+1);
						o_msg (archive);
					}
				}
				else if (keyType == LUA_TSTRING)
				{
					cstr_t key = lua_tostring (m_lua, -2);
					if (key)
					{
						if (valueType == LUA_TSTRING)            o_msg (key, lua_tostring (m_lua, -1));
						else if (valueType == LUA_TBOOLEAN)      o_msg (key, (bool) lua_toboolean (m_lua, -1));
						else if (valueType == LUA_TNUMBER)       o_msg (key, lua_tonumber (m_lua, -1));
						else if (valueType == LUA_TTABLE)
						{
							Epigram msg;
							ConvertLuaTableToEpigram (lua_gettop (m_lua), msg, depth+1);
							
							bool isArray = false;
							if (msg.Count() == 1)
							{
								auto & i = * msg.begin();
								
								//							   auto & item = msg.Index (0);
								if (i.IsType (c_jdTypeId::none /*default*/))
								{
									if (i.Is <f64>())
									{
										vector <f64> f = i;
										o_msg (key, f);
										isArray = true;
									}
									else if (i.Is <string>())
									{
										vector <string> s = i;
										o_msg (key, s);
										isArray = true;
									}
								}
							}
							
							if (! isArray) o_msg (key, msg);
						}
					}
				}
				
				lua_pop (m_lua, 1);
			}
			

			if (doubles.size () and strings.size ())
			{
				o_msg = tempMsg;
			}
			else
			{
				if (doubles.size())         o_msg (doubles);
				else if (strings.size())    o_msg (strings);
			}
		}
	}
	
	
	void                        ConvertLuaTableToArray (i32 i_tableIndex, vector <string> &o_strings, vector <f64> &o_doubles, int depth = 0)
	{
		if (!m_lua)	return;
		if (lua_istable (m_lua, i_tableIndex))
		{
			lua_pushnil (m_lua);
			
			while (lua_next (m_lua, i_tableIndex))
			{
				//                for (int i = 0; i < depth * 4; ++i) cout << " ";
				
				if (lua_isnumber (m_lua, -2))
				{
					i32 index = (i32) lua_tointeger (m_lua, -2);
					//                    cout << "index: " << index;
					
					if (lua_isnumber (m_lua, -1))
					{
						f64 d = lua_tonumber (m_lua, -1);
						o_doubles.push_back (d);
					}
					else if (lua_isstring (m_lua, -1))
					{
						cstr_t value = lua_tostring (m_lua, -1);
						o_strings.push_back (value);
					}
				}
				
				lua_pop (m_lua, 1);
			}
		}
	}
	
	
	
	string						m_scriptPath;
	lua_State *                 m_lua				= nullptr;
};


#endif
