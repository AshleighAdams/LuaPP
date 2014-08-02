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
	
	namespace Extensions
	{
		template <typename T>
		struct AllowedType;
	}
	namespace CppFunction
	{
		template <typename Ret>
		struct MemberFunctionWrapper
		{
			template <typename Clazz, typename... Args>
			struct Wrapper
			{
				typedef Ret(Clazz::*Func)(Args...);
				static int invoke(lua_State* L)
				{
					Func func;
					memcpy(&func, lua_touserdata(L, lua_upvalueindex(1)), sizeof(Func));
					Clazz* self = static_cast<Clazz*>(lua_touserdata(L, 1));
					int count = lua_gettop(L);
					Extensions::AllowedType<Ret>::Push(L, (self->*func)(Extensions::AllowedType<Args>::GetParameter(L, count)...));
					return 1;
				}

				static bool store(lua_State* L, Func func)
				{
					memcpy(lua_newuserdata(L, sizeof(Func)), &func, sizeof(Func));
					lua_pushcclosure(L, invoke, 1);
					return true;
				}
			};

		};
		template <>
		struct MemberFunctionWrapper<void>
		{
			template <typename Clazz, typename... Args>
			struct Wrapper
			{
				typedef void (Clazz::*Func)(Args...);
				static int invoke(lua_State* L)
				{
					Func func;
					memcpy(&func, lua_touserdata(L, lua_upvalueindex(1)), sizeof(Func));
					Clazz* self = static_cast<Clazz*>(lua_touserdata(L, 1));
					int count = lua_gettop(L);
					(self->*func)(Extensions::AllowedType<Args>::GetParameter(L, count)...);
					return 0;
				}

				static bool store(lua_State* L, Func func)
				{
					memcpy(lua_newuserdata(L, sizeof(Func)), &func, sizeof(Func));
					lua_pushcclosure(L, invoke, 1);
					return true;
				}
			};
		};
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
	
	class LuaTable
	{
	public:
		LuaTable(){}
	};
	
	class State;
	class Reference;
	class Variable;
	class ReturnValue;
	
	typedef std::function<std::vector<Variable>(State*, std::vector<Variable>&)> CFunction;
	
	class Variable
	{
	private:
		template <typename T>
		struct AsType
		{
			typedef decltype(Extensions::AllowedType<T>::GetFromVar(Variable(nullptr))) type;
		};
	public:
		Type                         _Type;
		State*                       _State;
		std::shared_ptr<Variable>    _Key;
		std::shared_ptr<Reference>   _KeyTo;
		bool                         _Global;
		bool                         _Registry;
		
		bool _IsReference;
		std::shared_ptr<Reference> Ref;
		string String;
		
		union {
			int Integer;
			double Real;
			bool Boolean;
			void* Pointer;
		} Data;
		
	protected:
		inline Variable(State* state);
		inline void SetAsStack(int index);

	public:
		inline Variable(State* state, Type type);

		template <typename T>
		inline Variable(State* state, const T& value);

		template <typename Clazz, typename Ret, typename... Args>
		static inline Variable FromMemberFunction(State* state, Ret(Clazz::*func)(Args...));

		static inline Variable FromStack(State* state, int index);
		static inline Variable FromStack(State* state);

	public:
		inline void operator=(const Variable& val);
		inline void operator=(const LuaTable& t);

		template<typename T>
		void operator=(const T& val);
		
		// for table
		template<typename T>
		Variable operator[](const T& val);
		
		bool operator==(const Variable& other) const; // from these 2 methods, the rest must be drived
		bool operator<(const Variable& other) const; /// from ...
		
		bool operator!=(const Variable& other) const;
		bool operator<=(const Variable& other) const;
		bool operator>(const Variable& other) const;
		bool operator>=(const Variable& other) const;

		template <typename T>
		bool operator==(const T& other) const
		{
			return *this == Variable(_State, other);
		}
		
		inline ~Variable();
	
		inline void SetKey(std::shared_ptr<Variable> key, Variable* to);
		
		inline Type GetType() const;
		inline string GetTypeName() const;
		inline void Push() const;
		
		bool IsNil() const
		{
			return _Type == Type::Nil;
		}
		
		template<typename T>
		typename AsType<T>::type As();
		
		template<typename T>
		bool Is() const;
		
		inline string ToString() const;
		
		inline Variable MetaTable() const;
		inline Variable SetMetaTable(const Variable& tbl);
		
		// functions
		template<typename... Args>
		ReturnValue operator()(Args... args) const;
		
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
			
			Variable _r = Variable::FromStack(this);
			_r._Registry = true;
			
			return _r;
		}
		
		Variable GetEnviroment()
		{
			lua_rawgeti(*this, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
			
			Variable var = Variable::FromStack(this);
			var._Global = true;
			
			return var;
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
			
			Variable userdata = Variable(this);// Use FromStack
			
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
		}
		
		~Reference()
		{
			luaL_unref(*_State, LUA_REGISTRYINDEX, _Ref);
		}
		
		void Push()
		{
			lua_rawgeti(*_State, LUA_REGISTRYINDEX, _Ref);
		}
		
		static std::shared_ptr<Reference> FromStack(State* state)
		{
			return std::make_shared<Reference>(state, luaL_ref(*state, LUA_REGISTRYINDEX));
		}
	};

	class ReturnValue
	{
	private:
		State* state;
		int s;

	public:
		ReturnValue() : state(nullptr), s(0) {}
		ReturnValue(State* state, int size) : state(state), s(size) {}
		~ReturnValue()
		{
			if (s)
				lua_pop(*state, s);
		}

		ReturnValue(const ReturnValue& b) = delete;
		ReturnValue(ReturnValue&& b) = delete;
		ReturnValue& operator=(const ReturnValue& b) = delete;

	public:
		int size()
		{
			return s;
		}

		Variable first()
		{
			if (s)
				return Variable::FromStack(state, -s);
			else
				return Variable(state, Type::None);
		}

		std::vector<Variable> toArray()
		{
			if (!s)
			{
				return {};
			}
			std::vector<Variable> ret;
			ret.reserve(s);
			int index = -s;
			while (index < 0)
			{
				ret.push_back(Variable::FromStack(state, index++));
			}
			return ret;
		}
	};
	
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
			Extensions::AllowedType<T>::Push(state, arg);
			//Variable var(&state, arg);
			//
			//var.Push();
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
	ReturnValue Variable::operator()(Args... args) const
	{
		if(GetType() != Type::Function)
		{
			throw RuntimeError("Attempted to call " + (_Key != nullptr ? "'"+_Key->ToString()+"'" : "an unindexed variable") + " (a " + GetTypeName() + " value)");
		}
		
		int top = lua_gettop(*_State);
		
		this->Push();

		int argc = 0;
		_Variable::PushRecursive(*_State, argc, args...);
		
		if (lua_pcall(*_State, argc, LUA_MULTRET, 0))
		{
			string err = lua_tostring(*_State, -1);
			lua_pop(*_State, 1);
			
			throw RuntimeError(err);
		}
		
		int ret = lua_gettop(*_State) - top;
		if (ret) // no returns
			return ReturnValue(_State, ret);
		else
			return ReturnValue();
	}

	inline Variable::Variable(State* state) :
		_State(state), _Key(nullptr), _KeyTo(nullptr)
	{
	}
	
	inline void Variable::SetAsStack(int index)
	{
		_Type = static_cast<Type>(lua_type(*_State, -1));
		_IsReference = false;
		_Global = false;
		_Registry = false;
		
		switch (_Type)
		{
		case Type::Nil:
			break;
		case Type::String:
			String = lua_tostring(*_State, -1);
			break;
		case Type::Number:
			Data.Real = lua_tonumber(*_State, -1);
			break;
		case Type::Boolean:
			Data.Boolean = lua_toboolean(*_State, -1) != 0;
			break;
		case Type::LightUserData:
			Data.Pointer = lua_touserdata(*_State, -1);
			break;
		case Type::Function:
		case Type::Table:
		case Type::UserData:
			lua_pushvalue(*_State, index);
			Ref = Reference::FromStack(_State);
			_IsReference = true;
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
	}

	inline Variable Variable::FromStack(State* state, int index)
	{
		Variable ret(state);
		ret.SetAsStack(index);
		return ret;
	}
	inline Variable Variable::FromStack(State* state)
	{
		Variable ret = FromStack(state, -1);
		lua_pop(*state, 1);
		return ret;
	}
	
	inline Variable::Variable(State* state, Type type) : Variable(state)
	{
		_Type = type;
		_IsReference = false;
		_Global = false;
		_Registry = false;
		
		Ref = nullptr;
		Data.Integer = 0;
		String = "";
		
		switch(type)
		{
		case Type::Function:
			_IsReference = true;
			break;
		case Type::Table:
			lua_newtable(*_State);
			Ref = Reference::FromStack(_State);
			_IsReference = true;
			break;
		}
	}

	template <typename T>
	inline Variable::Variable(State* state, const T& v) : Variable(state)
	{
		Extensions::AllowedType<T>::Push(*state, v);
		SetAsStack(-1);
		lua_pop(*state, 1);
	}
	
	template <typename Clazz, typename Ret, typename... Args>
	static inline Variable Variable::FromMemberFunction(State* state, Ret (Clazz::*func)(Args...))
	{
		CppFunction::MemberFunctionWrapper<Ret>::Wrapper<Clazz, Args...>::store(*state, func);

		return Variable::FromStack(state);
	}
	
	inline Variable::~Variable()
	{
	}
	
	inline void Variable::SetKey(std::shared_ptr<Variable> key, Variable* to)
	{
		_Key = nullptr;
		_Key = key;
		
		_KeyTo = to->Ref;
	}
	
	inline string Variable::ToString() const
	{
		std::stringstream ss;
		
		switch(_Type)
		{
		case Type::Nil:
			return "nil";
		case Type::String:
			ss << "\"" << String << "\"";
			return ss.str();
		case Type::Number:
			ss << Data.Real;
			return ss.str();
		case Type::Boolean:
			return Data.Boolean ? "true" : "false";
		case Type::Function:
			return "[function]";
		case Type::Table:
			return "[table]";
		case Type::UserData:
			return "[userdata]";
		case Type::LightUserData:
			return "[lightuserdata]";
		default:
			return "ukn";
		}
	}
	
	inline Variable Variable::MetaTable() const
	{
		this->Push();
		
		if(!lua_getmetatable(*_State, -1))
			return Variable(_State, Type::Nil);
		
		Variable ret = Variable::FromStack(_State);
		assert(ret.GetType() == Type::Table);
		return ret;
	}
	
	inline Variable Variable::SetMetaTable(const Variable& tbl)
	{
		if(tbl.GetType() != Type::Table)
			throw RuntimeError("SetMetaTable(): argument is not a table!");
		
		this->Push();
		tbl.Push();
		
		lua_setmetatable(*_State, -2);
		
		return Variable::FromStack(_State);
	}
	
	inline void Variable::operator=(const LuaTable& t)
	{
		Variable tmp(_State, Type::Table);
		this->_IsReference = tmp._IsReference;
		this->_Type = tmp._Type;
		this->Data = tmp.Data;
		
		if(_Type == Type::String)
			this->String = tmp.String;
		else if(_IsReference)
			this->Ref = tmp.Ref;
		
		if(_Global || _Registry)
		{
			throw RuntimeError("Variable::operator=() used on global or register table!");
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
	
	inline void Variable::operator=(const Variable& val)
	{
		this->_IsReference = val._IsReference;
		this->_Type = val._Type;
		this->Data = val.Data;
		
		if(_Type == Type::String)
			this->String = val.String;
		else if(_IsReference)
			this->Ref = val.Ref;
		
		
		if(_Global || _Registry)
		{
			throw RuntimeError("Variable::operator=() used on global table or register table!");
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
			
			if(_Type == Type::String)
				this->String = tmp.String;
			else if(_IsReference)
				this->Ref = tmp.Ref;
			
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
		}
		
		std::shared_ptr<Variable> key = std::make_shared<Variable>(_State, val);
		
		// push table
		// push key
		// tell lua to index the table -2, with -1
		
		this->Push();
		key->Push();
		
		lua_gettable(*_State, -2);
		
		Variable ret = Variable::FromStack(_State); // takes it from the stack
		ret.SetKey(key, this);
		
		lua_pop(*_State, 1);
		return ret;
	}
	
	inline bool Variable::operator==(const Variable& other) const
	{
		try
		{
			this->Push();
			other.Push();
			bool ret = lua_compare(*_State, -2, -1, LUA_OPEQ) == 1;
			lua_pop(*_State, 2);
			return ret;
		}
		catch(...)
		{
			lua_pop(*_State, 2);
			throw;
		}
	}
	
	inline bool Variable::operator<(const Variable& other) const
	{
		try
		{
			this->Push();
			other.Push();
			bool ret = lua_compare(*_State, -2, -1, LUA_OPLT) == 1;
			lua_pop(*_State, 2);
			return ret;
		}
		catch(...)
		{
			lua_pop(*_State, 2);
			throw;
		}
	}
	
	inline bool Variable::operator!=(const Variable& other) const
	{
		return !operator==(other);
	}
	
	inline bool Variable::operator<=(const Variable& other) const
	{
		return operator==(other) || operator<(other);
	}
	
	inline bool Variable::operator>(const Variable& other) const
	{
		return operator!=(other) && !operator<(other);
	}
	
	inline bool Variable::operator>=(const Variable& other) const
	{
		return operator==(other) || operator>(other);
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
				Variable key = Variable::FromStack(_State);
				/* tbl=-3 key=-2, value=-1 */
				Variable val = Variable::FromStack(_State);
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
	
	inline void Variable::Push() const
	{
		switch(_Type)
		{
		case Type::Nil:
			lua_pushnil(*_State);
			break;
		case Type::String:
			lua_pushstring(*_State, String.c_str());
			break;
		case Type::Number:
			lua_pushnumber(*_State, Data.Real);
			break;
		case Type::Boolean:
			lua_pushboolean(*_State, Data.Boolean);
			break;
		case Type::LightUserData:
			lua_pushlightuserdata(*_State, Data.Pointer);
			break;
		case Type::Function:
		case Type::Table:
		case Type::UserData:
			Ref->Push();
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
	}
	
	inline Type Variable::GetType() const
	{
		return _Type;
	}
	
	inline string Variable::GetTypeName() const
	{
		return lua_typename(*_State, _Type);
	}
	
	template<typename T>
	typename Variable::AsType<T>::type Variable::As()
	{
		return Extensions::AllowedType<T>::GetFromVar(*this);
	}
	
	template<typename T>
	bool Variable::Is() const
	{
		return Extensions::AllowedType<T>::CheckVar(*this);
	}
	
	namespace Extensions
	{
		template <typename T>
		struct AllowedType
		{
			static T& GetFromVar(const Variable& var)
			{
				if (var.GetType() == Type::UserData)
				{
					var.Push();
					T* ret = static_cast<T*>(lua_touserdata(*var._State, -1));
					lua_pop(*var._State, 1);
					return *ret;
				}
				throw RuntimeError(string("Can not convert Variable to ") + typeid(T).name() + ".");
			}
			static void Push(lua_State* L, const T& value)
			{
				T* ud = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
				new(ud) T(value);

				lua_newtable(L);
				lua_pushstring(L, "__gc");
				lua_pushcfunction(L, [](lua_State* L) {
					T* p = static_cast<T*>(lua_touserdata(L, -1));
					p->~T();
					return 0;
				});
				lua_settable(L, -3);
				lua_setmetatable(L, -2);
			}
			static std::remove_reference_t<T>& GetParameter(lua_State* L, int& count)
			{
				return *static_cast<std::remove_reference_t<T>*>(lua_touserdata(L, (count--)));
			}
		};
		template <>
		struct AllowedType<int>
		{
			static int GetFromVar(const Variable& var)
			{
				if (var.GetType() == Type::Number)
				{
					return static_cast<int>(var.Data.Real);
				}
				return 0;
			}
			static bool CheckVar(const Variable& var)
			{
				return var.GetType() == Type::Number;
			}
			static int GetParameter(lua_State* L, int& count)
			{
				return lua_tointeger(L, (count--));
			}
			static void Push(lua_State* L, int value)
			{
				lua_pushinteger(L, value);
			}
		};

		template <size_t LEN>
		struct AllowedType<const char[LEN]>
		{
			static bool CheckVar(const Variable& var)
			{
				return var.GetType() == Type::String;
			}
			static const char* GetParameter(lua_State* L, int& count)
			{
				return lua_tostring(L, (count--));
			}
			static void Push(lua_State* L, const char* value)
			{
				lua_pushstring(L, value);
			}
		};

		template <>
		struct AllowedType<const char*>
		{
			static const char* GetFromVar(const Variable& var)
			{
				if (var.GetType() == Type::String)
				{
					return var.String.c_str();
				}
				return nullptr;
			}
		};

		template <>
		struct AllowedType<string>
		{
			static string GetFromVar(const Variable& var)
			{
				if (var.GetType() == Type::String)
				{
					return var.String;
				}
				return string();
			}
			static bool CheckVar(const Variable& var)
			{
				return var.GetType() == Type::String;
			}
			static string GetParameter(lua_State* L, int& count)
			{
				return lua_tostring(L, (count--));
			}
			static void Push(lua_State* L, const string& value)
			{
				lua_pushstring(L, value.c_str());
			}
		};

		template <typename T>
		struct AllowedType<T*>
		{
			static T* GetFromVar(const Variable& var)
			{
				if (var.GetType() == Type::LightUserData || var.GetType() == Type::UserData)
				{
					var.Ref->Push();
					T* ret = lua_touserdata(*var._State, -1);
					lua_pop(*var._State, 1);
					return ret;
				}
				return nullptr;
			}
			static bool CheckVar(const Variable& var)
			{
				return var.GetType() == Type::LightUserData || var.GetType() == Type::UserData;
			}
			static T* GetParameter(lua_State* L, int& count)
			{
				return lua_touserdata(L, (count--));
			}
			static void Push(lua_State* L, T* value)
			{
				lua_pushlightuserdata(L, value);
			}
		};
	}
}

#endif
