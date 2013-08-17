#ifndef LUAPP_HPP
#define LUAPP_HPP

// STL
#include <string>
#include <exception>
#include <list>

// lua
#include <lua.hpp>

namespace Lua
{
	using string = std::string;
	
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
	
	class State;
	class Variable
	{
	public:
		Type    _Type;
		State*  _State;
		string  _Key;
		bool    _Global;
		
		bool _IsReference;
		struct {
			void* Pointer;
			string String;
			int Integer;
			double Real;
			bool Boolean;
			int Reference;
		} Data;
		
	public:
		Variable(State* state);
		
		Variable(State* state, const string& value);
		Variable(State* state, const char* value);
		Variable(State* state, bool value);
		Variable(State* state, double value);
		Variable(State* state, long long value);
		Variable(State* state, int value);
		
		template<typename T>
		void operator=(const T& val);
		
		~Variable();
	
		
		Type GetType();
		string GetTypeName();
		void ToStack();
		
		template<typename T>
		T As();
		
		template<typename T>
		bool Is();
		
		template<typename... Args>
		std::list<Variable> operator()(Args... args);
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
		
		void DoString(const string& code, const string& name = "DoString") throw(CompileError, RuntimeError)
		{
			if(luaL_loadbuffer(_State, code.c_str(), code.length(), name.c_str()))
				throw CompileError(lua_tostring(_State, -1));
			
			if(lua_pcall(_State, 0, 0, 0))
				throw RuntimeError(lua_tostring(_State, -1));
		}
		
		Variable operator[](const string& key)
		{
			lua_getglobal(*this, key.c_str());
			Variable var = Variable(this);
			
			var._Global = true;
			var._Key = key;
			
			return var;
		}
	};
	
	namespace Extensions
	{
		template <class T1, class T2>
		struct GetValue;
	}
	
	// ---------------------
	//   Variable imp
	// ---------------------
	namespace _Variable
	{
		void PushRecursive(State& state, int& argc, Variable& var)
		{
			argc++;
			var.ToStack();
		}
		
		template<typename T>
		void PushRecursive(State& state, int& argc, T arg)
		{
			argc++;
			Variable var(&state, arg);
			
			var.ToStack();
		}
		
		void PushRecursive(State& state, int& argc)
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
	std::list<Variable> Variable::operator()(Args... args)
	{
		if(GetType() != Type::Function)
		{
			throw RuntimeError("Attempted to call '" + (_Key != "" ? _Key : "unknown") + "' (a " + GetTypeName() + " value)");
			return {};
		}
		
		int top = lua_gettop(*_State);
		
		this->ToStack();
		int argc = 0;
		_Variable::PushRecursive(*_State, argc, args...);
		
		
		if (lua_pcall(*_State, argc, LUA_MULTRET, 0))
			throw RuntimeError(lua_tostring(*_State, -1));
		
		if(lua_gettop(*_State) == top)
			return {};
		
		std::list<Variable> rets;
		
		while(lua_gettop(*_State) != top)
		{
			Variable r(_State);
			rets.push_front(r);
		}
		
		return rets;
	}
		
	Variable::Variable(State* state) : _State(state) // the variable should be on the stack
	{
		_Type = (Type)lua_type(*_State, -1);
		_IsReference = false;
		
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
			Data.Reference = luaL_ref(*_State, LUA_REGISTRYINDEX);
			_IsReference = true;
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
		
		if(!_IsReference)
			lua_pop(*_State, 1);
	}
	
	Variable::Variable(State* state, const string& value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		Data.String = value;
	}
	
	Variable::Variable(State* state, const char* value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		Data.String = value;
	}
	
	Variable::Variable(State* state, bool value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Boolean;
		Data.Boolean = value;
	}
	
	Variable::Variable(State* state, double value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		Data.Real = value;
	}
	
	Variable::Variable(State* state, long long value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		Data.Real = (double)value;
	}
	
	Variable::Variable(State* state, int value) : _State(state)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		Data.Real = (double)value;
	}
	
	Variable::~Variable()
	{
		if(_IsReference) // TODO: wtf, _State is already de-referenced?
			;//luaL_unref(*_State, 0, Data.Reference);
	}
	
	template<typename T>
	void Variable::operator=(const T& val)
	{
		if(_Global)
		{
			Variable tmp(nullptr, val);
			this->_IsReference = tmp._IsReference;
			this->_Type = tmp._Type;
			this->Data = tmp.Data;
			
			this->ToStack();
			
			lua_setglobal(*_State, _Key.c_str());
		}
	}
	
	void Variable::ToStack()
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
			lua_rawgeti(*_State, LUA_REGISTRYINDEX, Data.Reference);
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
	}
	
	Type Variable::GetType()
	{
		return _Type;
	}
	
	string Variable::GetTypeName()
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
		
	}
}

#endif


/*

idea:

Lua::State state;

state.RunString("a = 1337 b = "hello, world"");

Lua::Variable var_a = state["a"];
Lua::Variable var_b = state["b"];

int a = var_a.as<int>();
string b = var_b.as<string>();










 * */
