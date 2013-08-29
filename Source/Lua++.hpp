#ifndef LUAPP_HPP
#define LUAPP_HPP

// STL
#include <string>
#include <exception>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <functional>
#include <tuple>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

// lua
#include <lua.hpp>

namespace Lua
{
	using string = std::string;
	
	namespace Jookia // this is Jookia's work, it allows me to not have to generate a function ptr in Lua
	{
	#define FUNCS_PER_TYPE 128 // increase this if you have problems
	#define FUNCS_PER_TYPE_STR "128"
	#define LOOP_FUNC(X) \
		X(  0) X(  1) X(  2) X(  3) X(  4) X(  5) X(  6) X(  7) \
		X(  8) X(  9) X( 10) X( 11) X( 12) X( 13) X( 14) X( 15) \
		X( 16) X( 17) X( 18) X( 19) X( 20) X( 21) X( 22) X( 23) \
		X( 24) X( 25) X( 26) X( 27) X( 28) X( 29) X( 30) X( 31) \
		X( 32) X( 33) X( 34) X( 35) X( 36) X( 37) X( 38) X( 39) \
		X( 40) X( 41) X(42 ) X( 43) X( 44) X( 45) X( 46) X( 47) \
		X( 48) X( 49) X( 50) X( 51) X( 52) X( 53) X( 54) X( 55) \
		X( 56) X( 57) X( 58) X( 59) X( 60) X( 61) X( 62) X( 63) \
		X( 64) X( 65) X( 66) X( 67) X( 68) X( 69) X( 70) X( 71) \
		X( 72) X( 73) X( 74) X( 75) X( 76) X( 77) X( 78) X( 79) \
		X( 80) X( 81) X( 82) X( 83) X( 84) X( 85) X( 86) X( 87) \
		X( 88) X( 89) X( 90) X( 91) X( 92) X( 93) X( 94) X( 95) \
		X( 96) X( 97) X( 98) X( 99) X(100) X(101) X(102) X(103) \
		X(104) X(105) X(106) X(107) X(108) X(109) X(110) X(111) \
		X(112) X(113) X(114) X(115) X(116) X(117) X(118) X(119) \
		X(120) X(121) X(122) X(123) X(124) X(125) X(126) X(127) \
		X(128)
		


		template <typename F>
		struct CPP_FUNCTIONS
		{
			static F func[FUNCS_PER_TYPE];
		};

		template <typename F>
		F CPP_FUNCTIONS<F>::func[FUNCS_PER_TYPE];

		template <class F, int N, class R, class... A>
		R C_FUNCTION(A... args)
		{
			return CPP_FUNCTIONS<F>::func[N](args...);
		}

		template <class C_F, class F>
		C_F SEARCH_C_ALLOC(F function)
		{
			#define TEST_ALLOC(N) \
				if(!CPP_FUNCTIONS<F>::func[N]) \
				{ \
					CPP_FUNCTIONS<F>::func[N] = function; \
					return C_FUNCTION<F, N>; \
				}
			LOOP_FUNC(TEST_ALLOC);
			#undef TEST_ALLOC
			throw std::runtime_error("No available C functions to use! Increase Lua::FUNCS_PER_TYPE (is at " FUNCS_PER_TYPE_STR ")");
		}

		template <class C_F, class F>
		bool SEARCH_C_FREE(C_F function)
		{
			#define TEST_FREE(N) \
				{ \
					C_F func = C_FUNCTION<F, N>; \
					if(func == function) \
					{ \
						CPP_FUNCTIONS<F>::func[N] = nullptr; \
						return true; \
					} \
				}
			LOOP_FUNC(TEST_FREE);
			#undef TEST_FREE
			return false;
		}

		template <class C_F, class F>
		C_F allocCPtr(F function)
		{
			return SEARCH_C_ALLOC<C_F, F>(function);
		}

		template <class C_F, class F>
		bool freeCPtr(C_F function)
		{
			return SEARCH_C_FREE<C_F, F>(function);
		}

		#undef FUNCS_PER_TYPE
		#undef LOOP_FUNC
	}
	
	class Exception : public std::exception
	{
		string _what;
	public:
		Exception(const string& what) : std::exception(), _what(what) {}
		const char* what()
		{
			return _what.c_str();
		}
	};
	
	class CompileError : public Exception
	{
	public:
		CompileError(const string& what) : Exception(what) {}
	};
	
	class RuntimeError : public Exception
	{
	public:
		RuntimeError(const string& what) : Exception(what) {}
	};
	
	enum Type
	{
		None = -1,
		Nil = 0,
		Boolean = 1,
		LightUserData,
		Number,
		String,
		Table,
		Function,
		UserData,
		Thread
	};
	
	class NewTable
	{
	public:
		NewTable(){}
	};
	
	class State;
	class Reference;
	class Variable;
	
	typedef std::function<std::vector<Variable>(State*, std::vector<Variable>&)> CFunction;
	
	class Variable
	{
	public:
		Type                         _Type;
		State*                       _State;
		std::shared_ptr<Variable>    _Key;
		std::shared_ptr<Reference>   _KeyTo;
		bool                         _Global;
		bool                         _Registry;
		
		bool _IsReference;
		struct {
			void* Pointer;
			string String;
			int Integer;
			double Real;
			bool Boolean;
			std::shared_ptr<Reference> Ref;
		} Data;
		
	public:
		inline Variable(State* state);
		inline Variable(State* state, Type type);
		inline Variable(State* state, lua_CFunction func); // for (int)(*)(lua_State*)
		inline Variable(State* state, std::function<int(lua_State*)> func); // function<int(lua_State*)>
		inline Variable(State* state, CFunction func); // function<list<Variable>(list<Variable>&)
		
		inline Variable(State* state, const string& value);
		inline Variable(State* state, const char* value);
		inline Variable(State* state, bool value);
		inline Variable(State* state, double value);
		inline Variable(State* state, long long value);
		inline Variable(State* state, int value);
		
		inline void operator=(Variable val);
		inline void operator=(NewTable t);
		template<typename T>
		void operator=(const T& val);
		
		// for table
		template<typename T>
		Variable operator[](const T& val);
		
		inline ~Variable();
	
		inline void SetKey(std::shared_ptr<Variable> key, Variable* to);
		
		inline Type GetType();
		inline string GetTypeName();
		inline void Push();
		
		bool IsNil()
		{
			return _Type == Type::Nil;
		}
		
		template<typename T>
		T As();
		
		template<typename T>
		std::shared_ptr<T> AsPointer();
		
		template<typename T>
		bool Is();
		
		inline string ToString();
		
		inline Variable MetaTable();
		inline Variable SetMetaTable(Variable tbl);
		
		// functions
		template<typename... Args>
		std::vector<Variable> operator()(Args... args);
		
		// tables
		inline std::vector<std::pair<Variable, Variable>> pairs();
		inline std::vector<std::pair<Variable, Variable>> ipairs();
	};
		
	class State
	{
		lua_State* _State;
	public:
		State() : _State(luaL_newstate())
		{
		}
		~State()
		{
			lua_close(_State);
		}
		operator lua_State* () const
		{
			return _State;
		}
		
		void LoadStandardLibary()
		{
			luaL_openlibs(_State);
		}
		
		void LoadString(const string& code, const string& name = "LoadString") throw(CompileError)
		{
			if(luaL_loadbuffer(_State, code.c_str(), code.length(), name.c_str()))
			{
				string err = lua_tostring(_State, -1);
				lua_pop(_State, 1);
				throw CompileError(err);
			}
		}
		
		void DoString(const string& code, const string& name = "DoString") throw(CompileError, RuntimeError)
		{
			this->LoadString(code, name);
			
			if(lua_pcall(_State, 0, 0, 0))
			{
				string err = lua_tostring(_State, -1);
				lua_pop(_State, 1);
				throw RuntimeError(err);
			}
		}
		
		void LoadFile(const string& file)
		{
			if(luaL_loadfile(_State, file.c_str()))
			{
				string err = lua_tostring(_State, -1);
				lua_pop(_State, 1);
				throw CompileError(err);
			}
		}
		
		void DoFile(const string& file)
		{
			this->LoadFile(file);
			
			if(lua_pcall(_State, 0, 0, 0))
			{
				string err = lua_tostring(_State, -1);
				lua_pop(_State, 1);
				throw RuntimeError(err);
			}
		}
		
		Variable GetRegistry()
		{
			lua_pushvalue(*this, LUA_REGISTRYINDEX);
			
			Variable _r = Variable(this);
			_r._Registry = true;
			
			return _r;
		}
		
		Variable GetEnviroment()
		{
			lua_rawgeti(*this, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
			
			Variable var = Variable(this);
			var._Global = true;
			
			return var;
		}
		
		
		Variable GenerateFunction(CFunction func)
		{
			return Variable(this, (CFunction)func);
		}
		
		/*
		
		TODO: MESSY!
		
		*/
		
		Variable GenerateFunction(std::function<void()> func)
		{
			CFunction proxy = [this, func](State* state, std::vector<Variable>& args) -> std::vector<Variable>
			{
				func();
				return {};
			};
			
			return Variable(this, (CFunction)proxy);
		}
		
		template<typename T>
		Variable GenerateFunction(std::function<void(T)> func)
		{
			CFunction proxy = [this, func](State* state, std::vector<Variable>& args) -> std::vector<Variable>
			{
				T arg_0 = args[0].As<T>();
				func(arg_0);
				return {};
			};
			
			return Variable(this, (CFunction)proxy);
		}
		
		template<typename R, typename T>
		Variable GenerateFunction(std::function<R(T)> func)
		{
			CFunction proxy = [this, func](State* state, std::vector<Variable>& args) -> std::vector<Variable>
			{
				T arg_0 = args[0].As<T>();
				R ret = func(arg_0);
				return {state, ret};
			};
			
			return Variable(this, (CFunction)proxy);
		}
		/*
		template<typename R, typename T1, typename T2>
		Variable GenerateFunction(std::function<R(T1, T2)> func)
		{
			CFunction proxy = [this, func](State* state, std::vector<Variable>& args) -> std::vector<Variable>
			{
				T1 arg_0 = args[0].As<T1>();
				T2 arg_1 = args[1].As<T2>();
				R ret = func(arg_0, arg_1);
				return {state, ret};
			};
			
			return Variable(this, (CFunction)proxy);
		}
		*/
		
		/*
		
		END: MEssy
		
		*/
		
		template <typename T>
		struct function_traits
			: public function_traits<decltype(&T::operator())>
		{};
		// For generic types, directly use the result of the signature of its 'operator()'

		template <typename ClassType, typename ReturnType, typename... Args>
		struct function_traits<ReturnType(ClassType::*)(Args...) const>
		// we specialize for pointers to member function
		{
			enum { arity = sizeof...(Args) };
			// arity is the number of arguments.

			typedef ReturnType   result_type;
			typedef ClassType    class_type;

			template <size_t i>
			struct arg
			{
				typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
				// the i-th argument is equivalent to the i-th tuple element of a tuple
				// composed of those arguments.
			};
		};
		
		
		//template<typename T>
		void _GenerateMemberFunction(const string& classname, const string& name, CFunction func)
		{
			/* *
			 * 	local meta = _R[classname]
			 * 	if meta == nil then
			 * 		meta = _R[classname] = {}
			 * 	end
			 * */
			
			Variable var = this->GetRegistry()[classname];
			
			if(var.IsNil())
				var = NewTable();
			
			var[name] = this->GenerateFunction(func);
		}
		
		template<typename T>
		void GenerateMemberFunction(const std::string& name, std::vector<Variable> (T::*f)(State*, std::vector<Variable>&))
		{
			_GenerateMemberFunction(typeid(T).name(), name, [f](State* state, std::vector<Variable>& args) -> std::vector<Variable>
			{
				std::shared_ptr<T> ptr = args.front().AsPointer<T>();
				
				return ((*ptr).*f)(state, args); 
			}); 
		}
		
		template<typename T>
		Variable GeneratePointer(std::shared_ptr<T> ptr)
		{
			struct Callback
			{
				static int garbage(lua_State* lua)
				{
					//std::cout << lua_typename(lua, lua_type(lua, -1)) << "\n";
					
					lua_getmetatable(lua, -1);
					lua_pushstring(lua, "__shared_ptr");
					lua_gettable(lua, -2);
					
					std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)lua_touserdata(lua, -1);
					assert(ptr);
					assert(*ptr);
					ptr->~shared_ptr();
					return 0;
				}
			};
			
			std::shared_ptr<T>* internal_ptr = (std::shared_ptr<T>*)lua_newuserdata(_State, sizeof(std::shared_ptr<T>));
			new (internal_ptr) std::shared_ptr<T>(std::move(ptr));
			
			Variable userdata = Variable(this);
			
			Variable meta = Variable(this, Type::Table);
			meta["__gc"] = Variable(this, Callback::garbage);
			meta["__index"] = this->GetRegistry()[typeid(T).name()];
			meta["__shared_ptr"] = userdata;
			meta["__typeid"] = typeid(T).name();
			
			Variable obj = Variable(this, Type::Table);
			obj.SetMetaTable(meta);
			
			return obj;
		}
		
		Variable operator[](const string& key)
		{
			return this->GetEnviroment()[key];
		}
	};
	
	class Reference // to be used with shared_ptr
	{
		int _Ref;
		State* _State;
	public:
		Reference(State* state, int ref) : _State(state), _Ref(ref)
		{
			//std::cout << "ref " << _Ref << "\n";
		}
		
		~Reference()
		{
			luaL_unref(*_State, LUA_REGISTRYINDEX, _Ref);
			//std::cout << "unref " << _Ref << "\n";
		}
		
		void Push()
		{
			lua_rawgeti(*_State, LUA_REGISTRYINDEX, _Ref);
		}
		
		static std::shared_ptr<Reference> FromStack(State* state)
		{
			int ref = luaL_ref(*state, LUA_REGISTRYINDEX);
			
			return std::make_shared<Reference>(state, ref);
		}
	};
	
	namespace Extensions
	{
		template <class T1, class T2>
		struct GetValue;
	}
	
	// ---------------------
	//	 Variable imp
	// ---------------------
	namespace _Variable
	{
		inline void PushRecursive(State& state, int& argc, Variable& var)
		{
			argc++;
			var.Push();
		}
		
		inline void PushRecursive(State& state, int& argc, std::vector<Variable> vec)
		{
			for(Variable& var : vec)
				PushRecursive(state, argc, var);
		}
		
		template<typename T>
		void PushRecursive(State& state, int& argc, T arg)
		{
			argc++;
			Variable var(&state, arg);
			
			var.Push();
		}
		
		inline void PushRecursive(State& state, int& argc)
		{
			// finalizer
		}
		
		template<typename T, typename... Args>
		void PushRecursive(State& state, int& argc, T arg, Args... args)
		{
			PushRecursive(state, argc, arg);
			PushRecursive(state, argc, args...);
		}
		
		template<typename... Args>
		void PushRecursive(State& state, int& argc, Args... args)
		{
			PushRecursive(state, argc, args...);
		}
	}
	
	template<typename... Args>
	std::vector<Variable> Variable::operator()(Args... args)
	{
		if(GetType() != Type::Function)
		{
			throw RuntimeError("Attempted to call " + (_Key != nullptr ? "'"+_Key->ToString()+"'" : "an unindexed variable") + " (a " + GetTypeName() + " value)");
			return {};
		}
		
		int top = lua_gettop(*_State);
		
		this->Push();
		int argc = 0;
		_Variable::PushRecursive(*_State, argc, args...);
		
		
		if (lua_pcall(*_State, argc, LUA_MULTRET, 0))
		{
			string err = lua_tostring(*_State, -1);
			lua_pop(*_State, 1);
			
			throw RuntimeError();
			return {};
		}
		
		if(lua_gettop(*_State) == top) // no returns
			return {};
		
		std::vector<Variable> rets;
		rets.reserve(lua_gettop(*_State) - top);
		
		while(lua_gettop(*_State) != top)
		{
			Variable r(_State);
			rets.push_back(r);
		}
		
		std::vector<Variable> flipped;
		flipped.reserve(rets.size());
		
		for(int i = rets.size(); i --> 0;)
			flipped.push_back(rets[i]); // TODO: fix this!
		
		return flipped;
	}
		
	inline Variable::Variable(State* state) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_Type = (Type)lua_type(*_State, -1);
		_IsReference = false;
		_Global = false;
		_Registry = false;
		
		switch(_Type)
		{
		case Type::Nil:
			break;
		case Type::String:
		{
			Data.String = lua_tostring(*_State, -1);
			break;
		}
		case Type::Number:
			Data.Real = lua_tonumber(*_State, -1);
			break;
		case Type::Boolean:
			Data.Boolean = lua_toboolean(*_State, -1);
			break;
		case Type::Function:
		case Type::Table:
		case Type::UserData:
			Data.Ref = Reference::FromStack(_State);
			_IsReference = true;
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
		
		if(!_IsReference)
			lua_pop(*_State, 1);
	}
	
	inline Variable::Variable(State* state, Type type) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_Type = type;
		_IsReference = false;
		_Global = false;
		_Registry = false;
		
		Data.Ref = nullptr;
		Data.Boolean = false;
		Data.Integer = 0;
		Data.String = "";
		Data.Pointer = nullptr;
		
		switch(type)
		{
		case Type::Function:
			_IsReference = true;
			break;
		case Type::Table:
			lua_newtable(*_State);
			Data.Ref = Reference::FromStack(_State);
			_IsReference = true;
			break;
		}
	}
	
	Variable::Variable(State* state, lua_CFunction func)
	{
		_State = state;
		_IsReference = true;
		_Type = Type::Function;
		_Global = false;
		_Registry = false;
		
		lua_pushcfunction(*_State, func);
		Data.Ref = Reference::FromStack(state);
	}
	
	inline Variable::Variable(State* state, CFunction func)
	{
		_State = state;
		_IsReference = true;
		_Type = Type::Function;
		_Global = false;
		_Registry = false;
		
		std::function<int(lua_State*)> proxy = [func, state](lua_State* L) -> int
		{
			int argc = lua_gettop(L);
			
			std::vector<Variable> args;
			
			for(int i = 0; i < argc; i++) // the first one is the target function
				args.push_back({state});
			
			std::vector<Variable> flipped;
			for(Variable& var : args)
				flipped.push_back(var);

			try
			{
				std::vector<Variable> rets = func(state, flipped);
				
				for(Variable& var : rets)
					var.Push();
				
				return rets.size();
			}
			catch(Exception ex)
			{
				lua_pushstring(*state, ex.what());
				lua_error(*state);
				
				return 0;
			}
		};
		
		lua_CFunction ptr = Lua::Jookia::allocCPtr<lua_CFunction>(proxy);
		lua_pushcfunction(*state, ptr);
		
		Data.Ref = Reference::FromStack(state);
	}
	
	inline Variable::Variable(State* state, const string& value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		_Global = false;
		_Registry = false;
		
		Data.String = value;
	}
	
	inline Variable::Variable(State* state, const char* value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		_Global = false;
		_Registry = false;
		Data.String = value;
	}
	
	inline Variable::Variable(State* state, bool value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Boolean;
		_Global = false;
		_Registry = false;
		
		Data.Boolean = value;
	}
	
	inline Variable::Variable(State* state, double value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		_Registry = false;
		
		Data.Real = value;
	}
	
	inline Variable::Variable(State* state, long long value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		_Registry = false;
		
		Data.Real = (double)value;
	}
	
	inline Variable::Variable(State* state, int value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		_Registry = false;
		
		Data.Real = (double)value;
	}
	
	inline Variable::~Variable()
	{
		if(_IsReference) // TODO: wtf, _State is already de-referenced?
			;//luaL_unref(*_State, 0, Data.Reference);
	}
	
	inline void Variable::SetKey(std::shared_ptr<Variable> key, Variable* to)
	{
		_Key = nullptr;
		_Key = key;
		
		_KeyTo = to->Data.Ref;
	}
	
	inline string Variable::ToString()
	{
		std::stringstream ss;
		
		switch(_Type)
		{
		case Type::Nil:
			return "nil";
		case Type::String:
			return Data.String;
		case Type::Number:
			ss << Data.Real;
			return ss.str();
		case Type::Boolean:
			return Data.Boolean ? "true" : "false";
		case Type::Function:
			return "function";
		case Type::Table:
			return "table";
		default:
			return "ukn";
		}
	}
	
	inline Variable Variable::MetaTable()
	{
		this->Push();
		
		if(!lua_getmetatable(*_State, -1))
			return Variable(_State, Type::Nil);
		
		Variable ret(_State);
		assert(ret.GetType() == Type::Table);
		return ret;
	}
	
	inline Variable Variable::SetMetaTable(Variable tbl)
	{
		if(tbl.GetType() != Type::Table)
			throw RuntimeError("SetMetaTable(): argument is not a table!");
		
		this->Push();
		tbl.Push();
		
		lua_setmetatable(*_State, -2);
		
		Variable ret = Variable(_State);
		return ret;
	}
	
	inline void Variable::operator=(NewTable t)
	{
		Variable tmp(_State, Type::Table);
		this->_IsReference = tmp._IsReference;
		this->_Type = tmp._Type;
		this->Data = tmp.Data;
		
		if(_Global || _Registry)
		{
			throw RuntimeError("Variable::operator=() used on global table or reference table!");
			//this->Push();
			//lua_setglobal(*_State, _Key->As<string>().c_str()); // TODO: use Variable to index
		}
		else
		{
			if(_KeyTo == nullptr)
				throw RuntimeError("_KeyTo is null!");
				
			_KeyTo->Push();
			_Key->Push();
			tmp.Push();
			
			lua_settable(*_State, -3);
			lua_pop(*_State, 1);
		}
	}
	
	inline void Variable::operator=(Variable val)
	{
		this->_IsReference = val._IsReference;
		this->_Type = val._Type;
		this->Data = val.Data;
		
		if(_Global || _Registry)
		{
			throw RuntimeError("Variable::operator=() used on global table or reference table!");
		}
		else
		{
			if(_KeyTo == nullptr)
				throw RuntimeError("_KeyTo is null!");
				
			_KeyTo->Push();
			_Key->Push();
			this->Push();
			
			lua_settable(*_State, -3);
			lua_pop(*_State, 1);
		}
	}
	
	template<typename T>
	void Variable::operator=(const T& val)
	{
		if(_Global || _Registry)
		{
			throw RuntimeError("Variable::operator=() used on global table or reference table!");
		}
		else
		{
			if(_KeyTo == nullptr)
				throw RuntimeError("_KeyTo is null!");
			
			Variable tmp(_State, val);
			this->_IsReference = tmp._IsReference;
			this->_Type = tmp._Type;
			this->Data = tmp.Data;
			
			_KeyTo->Push();
			_Key->Push();
			tmp.Push();
			
			lua_settable(*_State, -3);
			lua_pop(*_State, 1);
		}
	}
	
	template<typename T>
	Variable Variable::operator[](const T& val)
	{
		if(GetType() != Type::Table)
		{
			throw RuntimeError("Attempted to index '" + _Key->ToString() + "' (a " + GetTypeName() + " value)");
			return Variable(_State, 0);
		}
		
		std::shared_ptr<Variable> key = std::make_shared<Variable>(_State, val);
		
		// push table
		// push key
		// tell lua to index the table -2, with -1
		
		
		this->Push();
		key->Push();
				
		lua_gettable(*_State, -2);
		
		Variable ret = Variable(_State); // takes it from the stack
		ret.SetKey(key, this);
		
		lua_pop(*_State, 1);
		return ret;
	}
	
	inline std::vector<std::pair<Variable, Variable>> Variable::pairs()
	{
		if(GetType() != Type::Table)
		{
			throw RuntimeError("Attempted to pairs '" + _Key->ToString() + "' (a " + GetTypeName() + " value)");
			return {};
		}
		std::vector<std::pair<Variable, Variable>> ret;
		
		this->Push();
		{
			lua_pushnil(*_State);
			/* tbl=-2, key=-1 */
			while (lua_next(*_State, -2) != 0)
			{
				/* tbl=-3 key=-2, value=-1 */
				lua_pushvalue(*_State, -2);
				/* tbl=-4 key=-3, value=-2, key=-1 */
				Variable key = Variable(_State);
				/* tbl=-3 key=-2, value=-1 */
				Variable val = Variable(_State);
				/* tbl=-2, key=-1 */
				
				ret.push_back({key, val});
			}
		}
		lua_pop(*_State, 1);
		
		return ret;
	}
	
	inline std::vector<std::pair<Variable, Variable>> Variable::ipairs()
	{
		if(GetType() != Type::Table)
		{
			throw RuntimeError("Attempted to ipairs '" + _Key->ToString() + "' (a " + GetTypeName() + " value)");
			return {};
		}
		
		int i = 1;
		Variable& self = *this;
		std::vector<std::pair<Variable, Variable>> ret;
		
		while(true)
		{
			Variable var = self[i];
			
			if(var.IsNil())
				break;
			
			ret.push_back({Variable(_State, i), var});
			i++;
		}
		
		return ret;
	}
	
	inline void Variable::Push()
	{
		switch(_Type)
		{
		case Type::Nil:
			lua_pushnil(*_State);
			break;
		case Type::String:
			lua_pushstring(*_State, Data.String.c_str());
			break;
		case Type::Number:
			lua_pushnumber(*_State, Data.Real);
			break;
		case Type::Boolean:
			lua_pushboolean(*_State, Data.Boolean);
			break;
		case Type::Function:
		case Type::Table:
		case Type::UserData:
			Data.Ref->Push();
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
	}
	
	inline Type Variable::GetType()
	{
		return _Type;
	}
	
	inline string Variable::GetTypeName()
	{
		return lua_typename(*_State, _Type);
	}
	
	template<typename T>
	T Variable::As()
	{
		T ret = T();
		Extensions::GetValue<Variable&, T&>()(*this, ret);
		//Extensions::ToCPPType(*this, ret);
		return ret;
	}
	
	template<typename T>
	std::shared_ptr<T> Variable::AsPointer()
	{
		if(_Type != Type::Table)
		{
			throw RuntimeError(string("AsPointer<") + typeid(T).name() + ">(): variable is not a pointer");
			return nullptr;
		}
		
		Variable meta = this->MetaTable();
		
		
		if(this->MetaTable()["__typeid"].As<string>() != typeid(T).name())
		{
			throw RuntimeError(string("AsPointer<") + typeid(T).name() + ">(): incorrect type!");
			return nullptr;
		}
		
		this->MetaTable()["__shared_ptr"].Push();
		std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)lua_touserdata(*_State, -1);
		
		assert(ptr);
		assert(*ptr);
		
		return *ptr;
	}
	
	template<typename T>
	bool Variable::Is()
	{
		try
		{
			T ret = T();
			Extensions::GetValue<Variable&, T&>()(*this, ret);
			return true;
		}
		catch(RuntimeError ex)
		{
			return false;
		}
	}
	
	namespace Extensions
	{
		template<> struct GetValue<Variable&, string&>
		{
			void operator()(Variable& var, string& out)
			{
				if(var.GetType() == Type::String)
					out = var.Data.String;
				else
					throw RuntimeError("GetValue<string>(): variable is not a string!");
			}
		};
		
		template<> struct GetValue<Variable&, double&>
		{
			void operator()(Variable& var, double& out)
			{
				if(var.GetType() == Type::Number)
					out = var.Data.Real;
				else
					throw RuntimeError("GetValue<double>(): variable is not a number!");
			}
		};
		
		template<> struct GetValue<Variable&, bool&>
		{
			void operator()(Variable& var, bool& out)
			{
				if(var.GetType() == Type::Boolean)
					out = var.Data.Boolean;
				else
					throw RuntimeError("GetValue<bool>(): variable is not a boolean!");
			}
		};
		
		template<> struct GetValue<Variable&, int&>
		{
			void operator()(Variable& var, int& out)
			{
				if(var.GetType() == Type::Number)
					out = (int)var.Data.Real;
				else
					throw RuntimeError("GetValue<int>(): variable is not a boolean!");
			}
		};
		
		template<typename T>
		struct GetValue<Variable&, std::shared_ptr<T>&>
		{
			void operator()(Variable& var, std::shared_ptr<T>& out)
			{
				State* _State = var._State;
				if(var.GetType() != Type::Table)
				{
					throw RuntimeError("GetValue<std::shared_ptr<T>>(): variable is not a pointer!");
					out = nullptr;
				}
				
				Variable meta = var.MetaTable();
				
				if(meta["__typeid"].As<string>() != typeid(T).name())
				{
					throw RuntimeError(string("As<shared_ptr<") + typeid(T).name() + ">>(): incorrect pointer type!");
					out = nullptr;
					return;
				}
				
				meta["__shared_ptr"].Push();
				std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)lua_touserdata(*_State, -1);
				
				assert(ptr);
				assert(*ptr);
				
				out = *ptr;
			}
		};
		
	}
}

/*
if(_Type != Type::Table)
		{
			throw RuntimeError(string("AsPointer<") + typeid(T).name() + ">(): variable is not a pointer");
			return nullptr;
		}
		
		Variable meta = this->MetaTable();
		
		
		if(this->MetaTable()["__typeid"].As<string>() != typeid(T).name())
		{
			throw RuntimeError(string("AsPointer<") + typeid(T).name() + ">(): incorrect type!");
			return nullptr;
		}
		
		this->MetaTable()["__shared_ptr"].Push();
		std::shared_ptr<T>* ptr = (std::shared_ptr<T>*)lua_touserdata(*_State, -1);
		
		assert(ptr);
		assert(*ptr);
		
		return *ptr;
*/

#endif
