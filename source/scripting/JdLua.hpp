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


using std::string, std::vector, std::unique_ptr, std::deque, std::string_view;


/*static void * luaAlloc (void *ud, void *ptr, size_t osize, size_t nsize)
{
	static u32 used = 0;
	
	used += nsize;
	used -= osize;
	
//	jd::out ("alloc: @ bytes", used);
	
	if (nsize == 0)
	{
//		jd::out ("free: @ @", ptr, osize);
		free (ptr);
		return NULL;
	}
	else
	{
//		jd::out ("alloc: @ @->@", ptr, osize, nsize);
		return realloc (ptr, nsize);
	}
}*/


// jdlua_checkForSelf requires __name be assigned to the library table; maybe should go in a metatable for consistency &
// compatibility with jdlua_getType

int /* index */ 	jdlua_checkForSelf		(lua_State * L, cstr_t i_libName);	// returns 1 if no self obj provided in args stack; otherwise 2
void				jdlua_removeSelf		(lua_State * L, cstr_t i_libName);	// deletes the stack entry if self obj is provided

void				jdlua_ref				(lua_State * L, int & io_ref);
void				jdlua_unRef				(lua_State * L, int & io_ref);

cstr_t  			jdlua_getType			(lua_State * L, int i_index);
int					jdlua_newMetatable		(lua_State * L, cstr_t i_name);
cstr_t				jdlua_getUserdataName	(lua_State * L, int i_index);


const i32 c_luaHashUnchanged = 123456789;


class JdLua
{
	public:
	
	static void Hook (lua_State * L, lua_Debug * ar)
	{
//		jd::out ("hook!");
		luaL_error (L, "infinite loop");
	}
	
	template <typename T>
	struct Caster
	{
		static void  Cast  (lua_State * L, i32 i_index, string & o_string)
		{
			if (lua_isstring (L, -1))
				o_string = lua_tostring (L, -1);
		}

		static void  Cast  (lua_State * L, i32 i_index, f64 & o_value)
		{
			if (lua_isnumber (L, -1))
				o_value = lua_tonumber (L, -1);
		}

		static void  Cast  (lua_State * L, i32 i_index, f32 & o_value)
		{
			if (lua_isnumber (L, -1))
				o_value = (f32) lua_tonumber (L, -1);
		}
	};
	
	template <typename T>
	static vector <T>   TableToVector  (lua_State * L, i32 i_tableIndex)
	{
		vector <T> array;
		
		if (lua_istable (L, i_tableIndex))
		{
			lua_pushnil (L);
			
			while (lua_next (L, i_tableIndex))
			{
				if (lua_isnumber (L, -2))
				{
					auto index = lua_tointeger (L, -2);
					
					if (index >= 1)
					{
						size_t s = std::max (array.size (), (size_t) index);
						array.resize (s);

						Caster <T>::Cast (L, -1, array [index - 1]);
					}
				}
				
				lua_pop (L, 1);
			}
		}
		
		return array;
	}
	
	

	public:					JdLua						(u64 i_instanceId = 0)
							:
							m_instanceId				(i_instanceId)
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
	
	
	// returns all paths of .lua scripts accessed via a 'require' call
	// requires modified Lua library (lj_cf_package_loader_lua () in LuaJIT)
	std::set <std::string>		GetDependencies			()
	{
		std::set <std::string> required;
		
		if (L)
		{
			lua_getfield (L, LUA_REGISTRYINDEX, "required");
			if (lua_istable (L, -1))
			{
				Epigram e = ConvertTableToEpigram (-1);
				
				for (auto & kv : e)
					required.insert (kv.GetKeyString ());
			}
			lua_pop (L, 1);
		}
	
		return required;
	}
	
	
	void					SetAllocator				(lua_Alloc i_allocator, void * i_object)
	{
		m_allocator = i_allocator;
		m_allocatorObj = i_object;
	}
	
	u32						GetAllocatedBytes			()
	{
		u32 bytes = 0;
		if (L)
		{
			bytes += lua_gc (L, LUA_GCCOUNT, 0) * 1024;
			bytes += lua_gc (L, LUA_GCCOUNTB, 0);
		}
		return bytes;
	}
	
	
	lua_State *				Get							()
	{
		Initialize ();
		return L;
	}
	
	operator				lua_State *					()
	{
		return Get ();
	}
	
	void					SetSearchPath				(stringRef_t i_path)
	{
		Initialize ();
		
		lua_getglobal	(L, "package");
		lua_pushstring	(L, i_path.c_str ());
		lua_setfield	(L, -2, "path");
		lua_pop 		(L, 1);
	}
	
	u64						GetInstanceId				() const
	{
		return m_instanceId;
	}
	
	u64						IncrementExecSequence		()
	{
		return m_exectionSequence++;
	}
	

	struct Result
	{
//		/Users/smassey/Documents/Sluggo/library/scripts/dsp/svf.lua:6: attempt to perform arithmetic on a table value
		
		i32					resultCode			= 0;
		u32					execSequence		= 0;
		u64					sequence			= 0;
		string 				errorMsg;
		JdString64			function;			// this is the pcall () entry point

		struct Location
		{
			string		file;
			i32			lineNum				= 0;
			
			bool operator == (Location const & i_other)
			{
				return tie (file, lineNum) == tie (i_other.file, i_other.lineNum);
			}
		};
		
		deque <Location>	location;

		void   AddLocation  (stringRef_t i_file, i32 i_lineNum)
		{
			location.push_back ({ i_file, i_lineNum });
		}
		
		explicit operator bool () const { return resultCode; }
	};
	
	static int HandleLuaError (lua_State * L);

	Result				LoadScriptFromFile			(stringRef_t i_path)
	{
		Result result;
		
		Initialize ();
		
		if (L)
		{
			i32 resultCode = luaL_loadfile (L, i_path.c_str ());
			
			if (not resultCode)
			{
				resultCode = lua_pcall (L, 0, 0, 0);
				if (resultCode)
					result = ParseErrorMessage (resultCode, "main");
			}
			else result = ParseErrorMessage (resultCode, "");
		}
		else
		{
			result.resultCode = -993652;
			result.errorMsg = "null lua state";
		}
		
		return result;
	}
	

	// i_mainName is simply an identifier that's pushed into the Result struct in the case of an error.
	Result				HashScript							(string_view i_mainName, JdMD5::MD5 & o_hash, string_view i_script, string_view i_scriptName, vector <u8> * o_bytecode = nullptr);

	// if i_script [0] = '@', then i_script is a filename
	Result				LoadAndCallScript					(stringRef_t i_mainName, stringRef_t i_script, vector <u8> * o_bytecode = nullptr)
	{
		return LoadAndCallScript (i_mainName, i_script.c_str (), nullptr, o_bytecode);
	}

	Result				CompileScript						(stringRef_t i_mainName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode = nullptr);
	
	// If io_hashCheck is set, only loads script if hash differs. returns new hash
	Result				LoadAndCallScript					(stringRef_t i_mainName, cstr_t i_script, JdMD5::MD5 * io_hashCheck, vector <u8> * o_bytecode = nullptr);

	

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
	
	
	
	Result			CallStackTop			(string_view i_functionLabel, u32 i_numReturns = 0)	// no args
	{
		Result result;

		m_errorLocation.clear ();
		
		int r = lua_pcall (L, 0, i_numReturns, 1);
		if (r)
			result = ParseErrorMessage (r, i_functionLabel);
		
		return result;
	}
	
	Result			CallGlobalFunction		(string_view i_functionName)
	{
		Result result;
		
		if (i_functionName.find (":") != string::npos)
		{
		
		}
		
		lua_getglobal (L, i_functionName.data ());
		
		if (lua_isfunction (L, -1))
			result = CallStackTop (i_functionName);
		else
		{
//			lua_pop (L, 1);
			result = GenerateError (-494, jd::sprintf ("function '@' not found", i_functionName));
		}
			
		return result;
	}
	
	
	// this can push a function to the top of the stack (such as "Render") so it can be repeatedly called without lookup
//	void			PushGlobal				(cstr_t i_functionName)
//	{
//		lua_getglobal (L, i_functionName);
//	}

	
	Epigram					CallObject				(string_view i_functionName, Epigram && i_args = Epigram (), Result * o_result = nullptr);
	// i_functionName can have the form: "someTable.someNestedTable:function"

//	Epigram					CallObject				(int i_luaTableRef, string_view i_functionName, Epigram && i_args = Epigram (), Result * o_result = nullptr);
	
	
	template <typename T>
	void  PushArg  (T const & i_arg)
	{
		constexpr u8 typeId = Jd::TypeId <T> ();
		
		if constexpr (Jd::IsNumberType (typeId))
		{
			lua_pushnumber (L, i_arg);
		}
		else if constexpr (std::is_same <T, string>::value)
		{
			lua_pushstring (L, i_arg.cstr ());
		}
	}
	
	template <typename... Args>
	Result					CallObject				(int i_tableRef, string_view i_functionName, tuple <Args...> const & i_args)
	{
		Result result;
		
		int top = lua_gettop (L);
		
		lua_rawgeti (L, LUA_REGISTRYINDEX, i_tableRef);
		
		if (lua_istable (L, -1))
		{
			lua_getfield (L, -1, i_functionName.data ());
			
			if (lua_isfunction (L, -1))
			{
				lua_pushvalue (L, -2);		// need to copy the table to the self arg: [table][func][self=table]
				
				std::apply ([this] (auto && ... arg)
				{
					(PushArg (arg), ...);
				},
				i_args);
				
				int luaResult = lua_pcall (L, lua_gettop (L) - top - 2, 0, 1);

				result = ParseErrorMessage (luaResult, i_functionName.data ());
			}
		}
		
		lua_settop (L, top);

		return result;
	}
	
	Epigram					GetGlobalTable			(string_view i_tableName)
	{
		Epigram e;
		
		int top = lua_gettop (L);
		
		lua_getglobal (L, i_tableName.data ());
		
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
	

	Result					GenerateError			(i32 i_resultCode, string_view i_message)
	{
		Result error { .resultCode= i_resultCode, .errorMsg= i_message.data (), .sequence = m_instanceId, .execSequence= m_exectionSequence, .function = m_functionName };
		
		return error;
	}
	

	Result                    ParseErrorMessage         (i32 i_resultCode, string_view i_functionName)
	{
		Result error { .resultCode= i_resultCode, .sequence = m_instanceId, .execSequence= m_exectionSequence, .function = m_functionName };
		
		if (i_functionName.size ())
			error.function = i_functionName;

		if (L)
		{
			if (lua_isstring (L, -1))
			{
				Result::Location location;

				// TODO: reimplement require () to add error handler
				// can just wrap require. with pcall!?
				
				std::string s = lua_tostring (L, -1);					//		jd::out (s);
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
								location.file = "@" + filename;
							}
						}
					}
				}
				
				std::smatch m;
				bool matched = (std::regex_search (s, m, std::regex (R"(:(\d+):)")));
				
				if (matched)
				{
					size_t p = s.find (m [0].str ());
					error.errorMsg = s.substr (p + m [0].str().size ());
					string file = s.substr (0, p);
					
					if (location.file.empty () and s.size () < 1083)
					{
						if (file.substr (0, 7) != "[string")
							location.file = "@";

						location.file += file;
					}

					sscanf (m [1].str ().c_str (), "%d", & location.lineNum);
				}
				else error.errorMsg = s;
				
				while (error.errorMsg.c_str () [0] == ' ')
					error.errorMsg = error.errorMsg.substr (1);
				
				error.location = m_errorLocation;
				
				if (not location.file.empty ())
					error.location.push_front (location);		// FIX: refactor. all that parsing for nothing if m_errorLocation
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

	
	bool						FindFunctionInTable			(i32 i_tableIndex, cstr_t i_path)
	{
		bool found = false;

		if (L)
		{
			auto top = lua_gettop (L);
		
			lua_pushnil (L); // function
			lua_pushnil (L); // table (self)

			found = FindFunctionInTable (i_tableIndex, i_path, top + 2, 0);
			
			if (not found)
				lua_settop (L, top);
		}
		
		return found;
	}
	
	bool                        FindFunctionInTable         (i32 i_tableIndex, cstr_t i_path, i32 const i_stackTop, i32 i_depth)
	{
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
			
			if (i_depth == 0)	// and i_tableIndex = 0 ??
			{
//				i32 top = lua_gettop (L);
				
				lua_getglobal (L, searchFor.c_str ());
				
				if (FindFunctionInTable (lua_gettop (L), subPath.c_str (), i_stackTop, i_depth + 1))
					return true;
				
				lua_pop (L, 1);
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
							if (FindFunctionInTable (lua_gettop (L), subPath.c_str (), i_stackTop, i_depth + 1))
								return true;
						}
						else if (valueType == LUA_TFUNCTION)
						{
							lua_replace		(L, i_stackTop - 1);		// return function
							lua_pushvalue	(L, i_tableIndex);
							lua_replace		(L, i_stackTop);			// return table
							lua_settop		(L, i_stackTop);
							
							return true;
						}
						
						lua_pop (L, 2);
						break;
					}
				}
				
				lua_pop (L, 1); // pop value
			}
			
			// look through metatable
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
									if (FindFunctionInTable (lua_gettop (L), searchFor.c_str (), i_stackTop, i_depth + 1))
										return true;
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
		
		return false;
	}

	
	template <typename EpigramType>
	static void  PushEpigramToTable  (lua_State * L, EpigramType const & i_epigram, i32 i_tableIndex = 0)
	{
		if (i_tableIndex == 0)
		{
			lua_newtable (L);
			i_tableIndex = lua_gettop (L);
		}
		
		for (auto & i : i_epigram)
		{
			u8 keyType = i.GetKeyTypeId ();

			if (Jd::IsFloatingPointType (keyType))
			{
				f64 key = i.template GetKey <f64> ();
				lua_pushnumber (L, key);
			}
			else if (Jd::IsNumberType (keyType))
			{
				i64 key = i.template GetKey <i64> ();
				lua_pushnumber (L, key);
			}
			else if (keyType == c_jdTypeId::string)
			{
				cstr_t name = i.GetKeyString ();
				lua_pushstring (L, name);
			}

			if (i.template is <string> ())			lua_pushstring (L, i.template as <cstr_t> ());
			else if (i.template is <Epigram> ())
			{
				d_jdThrow("implme");
			}
			else									lua_pushnumber (L, i);
			
			lua_settable (L, -3);
		}
		
	}
	

	
	template <typename T>
	void				BindFunctionAndRef					(cstr_t i_functionName, lua_CFunction i_cFunction, T & i_arg)
	{
		lua_pushlightuserdata	(L, & i_arg);
		lua_pushcclosure		(L, i_cFunction, 1);
		lua_setglobal			(L, i_functionName);
	}

	template <typename T>
	void				BindFunctionAndPtr					(cstr_t i_functionName, lua_CFunction i_cFunction, T * i_arg)
	{
		lua_pushlightuserdata	(L, i_arg);
		lua_pushcclosure		(L, i_cFunction, 1);
		lua_setglobal			(L, i_functionName);
	}

	void				BindFunction						(cstr_t i_functionName, lua_CFunction i_cFunction)
	{
		lua_pushcfunction		(L, i_cFunction);
		lua_setglobal			(L, i_functionName);
	}

	Epigram						ConvertTableToEpigram (i32 i_tableIndex)
	{
		Epigram e;
		ConvertLuaTableToEpigram (i_tableIndex, e);
		return e;
	}

	
	
	private: //--------------------------------------------------------------------------------------------------------------------------------
	
	// TODO: change to IEpigramOut
	// if i_functionName is nullptr, function is expected on top of stack
//	Epigram               ExecuteFunction         (cstr_t i_functionName, EpigramRef i_args, i32 i_selfIndex, Result * o_luaError = nullptr)
	
	Result               ExecuteFunction         (cstr_t i_functionName, Epigram & o_return, EpigramRef i_args, i32 i_selfIndex)
	{
		Result result;
		
		if (L)
		{
			int top = lua_gettop (L);
			int funcIndex = top;

			i32 numCallingArgs = 0;
	
			if (i_selfIndex)
			{
				numCallingArgs = 1;
				
				if (i_selfIndex < 0)
					i_selfIndex = top + 1 + i_selfIndex;

				if (i_selfIndex == top)
					--funcIndex;

				if (i_selfIndex < funcIndex)
					lua_pushvalue (L, i_selfIndex);
				
				if (not lua_istable (L, lua_gettop (L)))
				{
					result = GenerateError (-84745, "missing self argument");
					goto exit;
				}
			}
			else if (i_functionName)
				lua_getglobal (L, i_functionName);
			
			
			if (lua_isfunction (L, funcIndex))
			{
				if (i_args.HasElements ())
				{
					++numCallingArgs;  // 1 lua table
					
					PushEpigramToTable (L, i_args);
				}
				
				int luaResult = lua_pcall (L, numCallingArgs, 1, 1);
				
				if (not luaResult)
				{
					int t = lua_gettop (L);
					
					if (t)
					{
						int type = lua_type (L, t);						//				cout << "type: " << type << endl;
						
						if		(type == LUA_TTABLE)	ConvertLuaTableToEpigram (t, o_return);
						else if (type == LUA_TSTRING)   o_return (lua_tostring (L, -1));
						else if (type == LUA_TBOOLEAN)  o_return ((bool) lua_toboolean (L, -1));
						else if (type == LUA_TNUMBER)   o_return (lua_tonumber (L, -1));
						else if (type == LUA_TNIL or
								 type == LUA_TLIGHTUSERDATA or
								 type == LUA_TFUNCTION or
								 type == LUA_TUSERDATA or
								 type == LUA_TTHREAD)	o_return ("warning", "unimplemented Lua return type");
						else d_jdThrow ("unknown return type");
					}
					
					lua_pop (L, 1);
				}
				else
				{
					result = ParseErrorMessage (luaResult, i_functionName ? i_functionName : "");
				}
				
			}
			else result = GenerateError (-84745, "no lua function to call");
		}
		else result = GenerateError (-393, "lua state is null");
		
		exit:
		
		return result;
	}
	
	void                        ConvertLuaTableToEpigram	(i32 i_tableIndex, Epigram & o_msg)
	{
		TableToEpigram (L, i_tableIndex, o_msg);
	}
	
	public:
	static void					LuaArgsToEpigram			(Epigram & o_args, lua_State * L, i32 const i_startIndex);
	
	static Epigram				TableToEpigram				(lua_State * L, i32 i_tableIndex)
	{
		Epigram table;
		TableToEpigram (L, i_tableIndex, table);
		return table;
	}
	
	static void					TableToEpigram				(lua_State * L, i32 i_tableIndex, Epigram & o_msg, u32 i_maxDepth = 10000, u32 i_depth = 0)
	{
		if (not L)	return;
		
		if (i_depth > i_maxDepth)
			return;
		
		if (i_tableIndex < 0)
		{
			i_tableIndex = lua_gettop (L) + i_tableIndex + 1;
		}
		
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
						TableToEpigram (L, lua_gettop (L), archive, i_maxDepth, i_depth + 1);
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
							TableToEpigram (L, lua_gettop (L), msg, i_maxDepth, i_depth + 1);
							
							bool isArray = false;
							if (msg.Count() == 1)
							{
								auto it = msg.begin ();
								auto & i = * it;
								
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
		if (not L) return;
		
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
			L = m_allocator ? lua_newstate (m_allocator, m_allocatorObj) : luaL_newstate ();
			
			luaL_openlibs 			(L);
			
			lua_pushstring			(L, "JdLua");
			lua_pushlightuserdata	(L, this);
			lua_settable			(L, LUA_REGISTRYINDEX);
			
			// LuaJIT modified to push require()'d .lua paths into this table
			lua_pushstring			(L, "required");
			lua_newtable			(L);
			lua_settable			(L, LUA_REGISTRYINDEX);

			lua_pushcfunction 		(L, HandleLuaError);

	//			lua_sethook (L, Hook, LUA_MASKCOUNT, 0xA0000000);
		}
	}
	
	
	lua_State *						L					= nullptr;
	bool							m_luaStateOwned		= true;
	
	lua_Alloc						m_allocator			= nullptr;
	void *							m_allocatorObj		= nullptr;

	vector <int>					m_stackTops;
	u64								m_instanceId		= 0;
	u32								m_exectionSequence	= 0;
	
	JdString64						m_functionName;					// trying to limit mallocs in audio thread
	
	deque <Result::Location>		m_errorLocation;
};


std::ostream & operator << (std::ostream & output, JdLua::Result const & i_result);


#endif
