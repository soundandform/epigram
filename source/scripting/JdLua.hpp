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

# include <regex>

# include "Epigram.hpp"
# include "JdResult.hpp"
# include "JdStopwatch.hpp"
# include "JdMD5.hpp"
# include "lua.hpp"


using std::string, std::vector, std::unique_ptr;

static void * luaAlloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
  (void)ud;
  (void)osize;
	
  if (nsize == 0) {
	  jd::out ("free: @", ptr);
	free(ptr);
	return NULL;
  } else {
	  jd::out ("alloc: @ @", ptr, nsize);
	return realloc(ptr, nsize);
  }
}


cstr_t  lua_gettype  (lua_State * L, int i_index);


const i32 c_luaHashUnchanged = 123456;


class JdLua
{
	static std::atomic <u64>		s_sequenceNum;
	
	public:					JdLua						(u64 i_sequence = 0)
	:						m_sequence					(i_sequence)
	{ }

							JdLua						(lua_State * i_lua)
							:
							L							(i_lua),
							m_luaStateOwned				(false)
	{ }
	
	virtual					~JdLua						()
	{
		if (L and m_luaStateOwned)
			lua_close (L);
	}
	
	lua_State *				Get							()
	{
		Initialize ();
		return L;
	}
	
	u64						GetSequenceNum				() const
	{
		return m_sequence;
	}
	
//	void					Swap						(JdLua & io_lua)
//	{
//		std::swap (L, io_lua.L);
//	}
	
	struct Result
	{
//		/Users/smassey/Documents/Sluggo/library/scripts/dsp/svf.lua:6: attempt to perform arithmetic on a table value
		
		i32				resultCode			= 0;
		string 			errorMsg;
		string			location;
		string			function;
		i32				lineNum				= 0;
		u64				sequence			= 0;
		
		operator bool () const { return resultCode; }
	};
	
	static int HandleLuaError (lua_State * L);

	Result				LoadScriptFromFile			(stringRef_t i_path)
	{
		Result result;
		
		Initialize ();
		
		if (L)
		{
			auto path = PreserveString (i_path.c_str ());
			
			i32 resultCode = luaL_loadfile (L, path);
			
			if (resultCode)  	result = ParseErrorMessage (resultCode, "");
			else      		 	lua_pcall (L, 0, 0, 0);
		}
		else
		{
			result.resultCode = -993652;
			result.errorMsg = "null lua state";
		}
		
		return result;
	}
	

	Result				HashScript							(stringRef_t i_functionName, JdMD5::MD5 & o_hash, cstr_t i_script, vector <u8> * o_bytecode = nullptr);

	Result				LoadAndCallScript					(stringRef_t i_functionName, stringRef_t i_script, vector <u8> * o_bytecode = nullptr)
	{
		return LoadAndCallScript (i_functionName, i_script.c_str (), nullptr, o_bytecode);
	}

	// If io_hashCheck is set, only loads script if hash differs. returns new hash
	Result				LoadAndCallScript					(stringRef_t i_functionName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode = nullptr);


	struct ByteCodeWriter : JdMD5
	{
		ByteCodeWriter		(vector <u8> * o_bytecode = nullptr, bool i_doHash = true)
		:
		doHash				(i_doHash),
		bytecode			(o_bytecode)
		{ }
		
		static int Handler (lua_State * L,
							const void * p,
							size_t sz,
							void * ud)
		{
			static_cast <ByteCodeWriter *> (ud)->Handle ((const u8 *) p, sz);
			return 0;
		}
		
		void		Handle   (const u8 * i_ptr, size_t i_size)
		{
			if (doHash)
				Add (i_ptr, i_size);

			if (bytecode)
			{
				bytecode->insert (bytecode->end (), i_ptr, i_ptr + i_size);
			}
		}
		
		bool				doHash							= true;
		vector <u8> *		bytecode						= nullptr;
	};
	
	
	
	Result			CallStackTop			(stringRef_t i_functionLabel)	// no args, no return
	{
		Result result;

		int errIndex = 0;
//		lua_pushcfunction	(L, HandleLuaError);
//		lua_insert 			(L, errIndex = -2);

		int r = lua_pcall (L, 0, 0, errIndex);
		if (r)
			result = ParseErrorMessage (r, i_functionLabel);
		
//		if (errIndex)
//			lua_pop (L, 1);

		return result;
	}
	
	Result			CallGlobalFunction		(stringRef_t i_functionName)
	{
		Result result;
		
		lua_getglobal (L, i_functionName.c_str ());
		
		if (lua_isfunction (L, -1))
			result = CallStackTop (i_functionName);
		else
		{
			lua_pop (L, 1);
			result = GenerateError (-494, jd::sprintf ("function '@' not found", i_functionName));
		}
			
		return result;
	}
	
	
	// this can push a function to the top of the stack (such as "Render") so it can be repeatedly called without lookup
//	void			PushGlobal				(cstr_t i_functionName)
//	{
//		lua_getglobal (L, i_functionName);
//	}
	
	
//	f64				Call					(f64 i_arg)
//	{
//		f64 r = 0.;
//
//		if (L)
//		{
//			lua_pushvalue (L, -1);			// copy the function to be called
//			lua_pushnumber (L, i_arg);
//			lua_call (L, 1, 1);
//			r = lua_tonumber (L, -1);
//			lua_pop (L, 1);					// pop the result
//		}
//
//		return r;
//	}
	
	Epigram               Call 			           (stringRef_t i_functionName, Epigram i_args = Epigram ())
	{
		Epigram result;
		
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
		
		int top = lua_gettop (L);
		
		lua_getglobal (L, i_tableName.c_str());
		
		if (lua_type (L, -1) == LUA_TTABLE)
		{
			ConvertLuaTableToEpigram (lua_gettop (L), e);
		}
		
		lua_settop (L, top);
		
		return e;
	}
	
	void					SaveStackTop			()
	{
		m_stackTops.push_back (lua_gettop (L));
	}

	void					RestoreStackTop			()
	{
		int top = m_stackTops.back ();
		m_stackTops.pop_back ();
		lua_settop (L, top);
	}

//	protected:
	
	Result					GenerateError			(i32 i_resultCode, stringRef_t i_message, i32 i_lineNum = 0)
	{
		Result error { .resultCode= i_resultCode, .errorMsg= i_message, .sequence = m_sequence + s_sequenceNum++, .lineNum = i_lineNum };
		
		return error;
	}
	

	Result                    ParseErrorMessage         (i32 i_resultCode, stringRef_t i_functionName)
	{
		Result error { .resultCode= i_resultCode };
		
		if (L)
		{
			if (lua_isstring (L, -1))
			{
				std::string s = lua_tostring (L, -1);						//	 jd::out (s);
				lua_pop (L, 1);
				
				// there's a specific error message from the package library / require (...). Example:
				//
				// error loading module 'spirit.dsp.lowpass' from file '/Users/smassey/Documents/Sluggo/library/spirit/dsp/lowpass.lua':
				// .../smassey/Documents/Sluggo/library/spirit/dsp/lowpass.lua:15: '=' expected near 'function'
				
				if (s.find ("error loading module") != std::string::npos)
				{
					size_t p = s.find (":\n\t");
					
					if (p != std::string::npos)
					{
						string filename = s.substr (0, p);
						s = s.substr (p + 3);
						
						p = filename.find ("' from file '");
						if (p != std::string::npos)
						{
							filename = filename.substr (p + 13);
							
							p = filename.rfind ("'");
							if (p != std::string::npos)
							{
								filename = filename.substr (0, p);
								error.location = "@" + filename;
							}
						}
					}
				}
				
//				string wtf = R"(:(\d+):)";
//
//				jd::out (wtf);
//
////				s = "fjdskfjxdsafdsf";
//				s = "fdjskfjdskfjs:75:kfgkfk";
				
//				s = "/Users/smassey/Documents/Sluggo/sluggo.lua:57: '=' expected near 'howdy'";
				
				std::smatch m;
				bool matched = (std::regex_search (s, m, std::regex (R"(:(\d+):)")));
				jd::out (matched);
				
				if (matched)
				{
					size_t p = s.find (m[0].str ());
					error.errorMsg = s.substr (p + m[0].str().size ());
					string location = s.substr (0, p);
					
					if (error.location.empty () and s.size () < 1083)
						error.location = location;

					sscanf (m [1].str ().c_str (), "%d", & error.lineNum);
//					jd::out (m[0].str ());
				}
/*
				size_t p = s.rfind (":");
				
				if (p != std::string::npos)
				{
					error.errorMsg = s.substr (p + 1);
					s = s.substr (0, p);
					
					p = s.rfind (":");
															d_jdAssert (p != std::string::npos);
					string line = s.substr (p + 1);
					s = s.substr (0, p);

					if (error.location.empty () and s.size () < 1083)
						error.location = s;
					
					sscanf (line.c_str (), "%d", & error.lineNum);
				}
				*/
				error.sequence = m_sequence + s_sequenceNum++;
				
				if (i_functionName.size ())
					error.function = i_functionName;
			}
		}
		
		return error;
	}
	
	
	bool                        HasGlobal               (cstr_t i_name, i32 i_luaType)
	{
		bool exists = false;
		if (L)
		{
			lua_getglobal (L, i_name);
			
			exists = (lua_type (L, -1) == i_luaType);
			lua_pop (L, 1);
		}
		
		return exists;
	}
	
	
	vector <string>				GetGlobalsOfType			(JdLua::Result & o_result, i32 i_luaType);
	vector <string>				GetGlobalFunctions			(JdLua::Result & result);

	
	
	bool                        FindFunctionInTable         (i32 i_tableIndex, cstr_t i_path, EpDelivery i_args, EpDelivery *o_result,
															 bool i_isObjectCall, i32 i_depth = 0)
	{
		if (! L) return false;
		
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
				i32 top = lua_gettop (L);
				//                cout << "top: " << top << endl;
				
				//                cout << "root table: " << searchFor << endl;
				lua_getglobal (L, searchFor.c_str());
				
				found = FindFunctionInTable (lua_gettop (L), subPath.c_str(), i_args, o_result, i_isObjectCall, i_depth + 1);
				//                d_mpAssert (lua_gettop (m_lua) == top, "stack whacked");
				
				lua_pop (L, 1);
				
				top = lua_gettop (L);
				//                cout << "end-top: " << top << endl;
				
				return found;
			}
		}
		else searchFor = i_path;
		
		//        cout << "find: " << searchFor << " in " << i_tableIndex << endl;
		//		cout << "tableindex: " << i_tableIndex << endl;
		
		if (lua_istable (L, i_tableIndex))
		{
			lua_pushnil (L);
			
			while (lua_next (L, i_tableIndex))
			{
				int keyType = lua_type (L, -2);
				int valueType = lua_type (L, -1);
				
				if (keyType == LUA_TSTRING)
				{
					string key = lua_tostring (L, -2);
					
					//					cout << "searching for " << searchFor << " : " << key << "....\n";
					
					if (key == searchFor)
					{
						//                        cout << "found\n";
						
						if (valueType == LUA_TTABLE)
						{
							found = FindFunctionInTable (lua_gettop (L), subPath.c_str (), i_args, o_result, i_isObjectCall, i_depth + 1);
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
						
						lua_pop (L, 2);
						break;
					}
				}
				
				lua_pop (L, 1); // pop value
			}
			
			if (! found)	// look through metatable
			{
				if (lua_getmetatable (L, i_tableIndex))
				{
					i32 metatable = lua_gettop (L);
					
					//					cout << "searching metatable...\n";
					lua_pushnil (L);
					
					while (lua_next (L, metatable))
					{
						int keyType = lua_type (L, -2);
						int valueType = lua_type (L, -1);
						
						if (keyType == LUA_TSTRING)
						{
							string key = lua_tostring (L, -2);
							
							//							cout << "searching  metatable for " << searchFor << " : " << key << "....\n";
							
							if (key == "__index")
							{
								if (valueType == LUA_TTABLE)
								{
									found = FindFunctionInTable (lua_gettop (L), searchFor.c_str(), i_args, o_result, i_isObjectCall, i_depth + 1);
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
								lua_pop (L, 2);
								break;
							}
						}
						
						lua_pop (L, 1); // pop value
					}
					
					lua_pop (L, 1); // pop metatable
				}
			}
		}
		
		return found;
	}
	
	int                         PushTableFromEpigram      (EpigramRef i_msg)
	{
		if (L)
		{
			lua_newtable (L);
			
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
					lua_pushstring (L, name);
				}
				else continue;
				
				if (i.Is <string> ())		lua_pushstring (L, i.To <cstr_t>());
				else						lua_pushnumber (L, i);
				
				lua_settable (L, -3);
			}
		}
		
		return 0;
	}
	
	
	template <typename T>
	void				Bind					(cstr_t i_functionName, lua_CFunction i_cFunction, T & i_arg)
	{
		lua_pushlightuserdata	(L, & i_arg);
		lua_pushcclosure		(L, i_cFunction, 1);
		lua_setglobal			(L, i_functionName);
	}
	
	Epigram						ConvertLuaTableToEpigram (i32 i_tableIndex)
	{
		Epigram e;
		ConvertLuaTableToEpigram (i_tableIndex, e);
		return e;
	}

	
	
	private: //--------------------------------------------------------------------------------------------------------------------------------
	
	Epigram               ExecuteFunction         (stringRef_t i_functionName, EpigramRef i_args, i32 i_selfIndex)
	{
		Epigram result;
		
		if (L)
		{
			
			int top = lua_gettop (L);
			
			if (i_functionName.size ())
			{
				//            cout << "CALL: [" << i_functionName << "] " << endl;
				lua_getglobal (L, i_functionName.c_str ());
			}
			
			if (lua_isfunction (L, -1))
			{
				
				i32 numCallingArgs = 0;
				
				if (i_selfIndex)
				{
					++numCallingArgs;
					if (lua_istable (L, i_selfIndex))
					{
						lua_pushvalue (L, i_selfIndex);
					}
				}
				
				if (i_args.HasElements ())
				{
					++numCallingArgs;  // 1 lua table
					
					PushTableFromEpigram (i_args);
				}
				
				int luaResult = lua_pcall (L, numCallingArgs, 1, 0);
				
				if (not luaResult)
				{
					int t = lua_gettop (L);
					
					if (t)
					{
						int type = lua_type (L, t);						//				cout << "type: " << type << endl;
						
						if		(type == LUA_TTABLE)	ConvertLuaTableToEpigram (t, result);
						else if (type == LUA_TSTRING)   result (lua_tostring (L, -1));
						else if (type == LUA_TBOOLEAN)  result ((bool) lua_toboolean (L, -1));
						else if (type == LUA_TNUMBER)   result (lua_tonumber (L, -1));
						else if (type == LUA_TNIL or
								 type == LUA_TLIGHTUSERDATA or
								 type == LUA_TFUNCTION or
								 type == LUA_TUSERDATA or
								 type == LUA_TTHREAD)	result ("warning", "unimplemented Lua return type");
						else d_jdThrow ("unknown return type");
					}
				}
				else
				{
					//cout << "result: " << result << endl;
					Result r = ParseErrorMessage (luaResult, i_functionName);
					result ("error", r.errorMsg);
				}
				
				lua_settop (L, top);
			}
			else result (d_jdError ("no lua function to call"));
		}
		else result (d_jdError ("lua state is null"));
		
		return result;
	}
	
	
	
	void                        ConvertLuaTableToEpigram (i32 i_tableIndex, Epigram & o_msg, u32 depth = 0)
	{
		if (not L)	return;
		
		if (lua_istable (L, i_tableIndex))
		{
			lua_pushnil (L);
			
			while (lua_next (L, i_tableIndex))
			{
				//                for (int i = 0; i < depth * 4; ++i) cout << " ";
				
				int keyType = lua_type (L, -2);
				int valueType = lua_type (L, -1);
				
				if (keyType == LUA_TNUMBER)
				{
					auto index = lua_tointeger (L, -2);
					
					// this assumes that indicies are sorted
					
					if (valueType == LUA_TSTRING)
					{
						cstr_t s = lua_tostring (L, -1);
						o_msg (index, s);
					}
					else if (valueType == LUA_TNUMBER)
					{
						f64 f = lua_tonumber (L, -1);
						o_msg (index, f);
					}
					else if (valueType == LUA_TTABLE)
					{
						Epigram archive;
						ConvertLuaTableToEpigram (lua_gettop (L), archive, depth+1);
						o_msg (archive);
					}
				}
				else if (keyType == LUA_TSTRING)
				{
					cstr_t key = lua_tostring (L, -2);
					if (key)
					{
						if		(valueType == LUA_TSTRING)		o_msg (key, lua_tostring (L, -1));
						else if (valueType == LUA_TBOOLEAN)   	o_msg (key, (bool) lua_toboolean (L, -1));
						else if (valueType == LUA_TNUMBER)		o_msg (key, lua_tonumber (L, -1));
						else if (valueType == LUA_TTABLE)
						{
							Epigram msg;
							ConvertLuaTableToEpigram (lua_gettop (L), msg, depth+1);
							
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
							
							if (not isArray) o_msg (key, msg);
						}
					}
				}
				
				lua_pop (L, 1);
			}
		}
	}
	
	
	void                        ConvertLuaTableToArray (i32 i_tableIndex, vector <string> &o_strings, vector <f64> &o_doubles, int depth = 0)
	{
		if (!L)	return;
		if (lua_istable (L, i_tableIndex))
		{
			lua_pushnil (L);
			
			while (lua_next (L, i_tableIndex))
			{
				//                for (int i = 0; i < depth * 4; ++i) cout << " ";
				
				if (lua_isnumber (L, -2))
				{
					i32 index = (i32) lua_tointeger (L, -2);
					//                    cout << "index: " << index;
					
					if (lua_isnumber (L, -1))
					{
						f64 d = lua_tonumber (L, -1);
						o_doubles.push_back (d);
					}
					else if (lua_isstring (L, -1))
					{
						cstr_t value = lua_tostring (L, -1);
						o_strings.push_back (value);
					}
				}
				
				lua_pop (L, 1);
			}
		}
	}

	void		Initialize			()
	{
		if (not L)
		{
			
//			L = lua_newstate (luaAlloc, nullptr);

			L = luaL_newstate ();
			luaL_openlibs (L);
		}
	}
	
	// docs don't mention it, but LuaJIT doesn't seem to take ownership of/copy the script strings
	// OR it does!?  TODO: investigate
	cstr_t						PreserveString			(cstr_t i_string)
	{
		return i_string;
//		m_strings.push_back (i_string);
//		return m_strings.back ().c_str ();
	}
	
	lua_State *						L				= nullptr;
	bool							m_luaStateOwned	= true;

	vector <int>					m_stackTops;
	u64								m_sequence		= 0;
	
	vector <string>					m_functionName;
	
	struct FunctionNamePusher
	{
		FunctionNamePusher (JdLua & i_lua, stringRef_t i_function)
		:
		lua (i_lua)
		{
			lua.m_functionName.push_back (i_function);
		}
		~FunctionNamePusher ()
		{
			lua.m_functionName.pop_back ();
		}
		
		JdLua & lua;
	};
};


#endif
