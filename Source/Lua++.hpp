#ifndef LUAPP_HPP
#define LUAPP_HPP

// STL
#include <string>
#include <exception>
#include <list>
#include <memory>
#include <sstream>

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
		Type                       _Type;
		State*                     _State;
		std::shared_ptr<Variable>  _Key;
		Variable*                  _KeyTo;
		bool                       _Global;
		
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
		Variable(State* state, Type type);
		
		Variable(State* state, const string& value);
		Variable(State* state, const char* value);
		Variable(State* state, bool value);
		Variable(State* state, double value);
		Variable(State* state, long long value);
		Variable(State* state, int value);
		
		template<typename T>
		void operator=(const T& val);
		
		// for table
		template<typename T>
		Variable operator[](const T& val);
		
		~Variable();
	
		void SetKey(std::shared_ptr<Variable> key, Variable* to);
		
		Type GetType();
		string GetTypeName();
		void ToStack();
		
		bool IsNil()
		{
			return _Type == Type::Nil;
		}
		
		template<typename T>
		T As();
		
		template<typename T>
		bool Is();
		
		string ToString();
		
		// functions
		template<typename... Args>
		std::list<Variable> operator()(Args... args);
		
		// tables
		std::list<std::pair<Variable, Variable>> pairs();
		std::list<std::pair<Variable, Variable>> ipairs();
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
			var.SetKey(std::make_shared<Variable>(this, key), nullptr);
			
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
			throw RuntimeError("Attempted to call '" + (_Key->ToString()) + "' (a " + GetTypeName() + " value)");
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
		
	Variable::Variable(State* state) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_Type = (Type)lua_type(*_State, -1);
		_IsReference = false;
		_Global = false;
		
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
		case Type::Table:
			Data.Reference = luaL_ref(*_State, LUA_REGISTRYINDEX);
			_IsReference = true;
			break;
		default:
			throw RuntimeError("The type `" + GetTypeName() + "' hasn't been implimented!");
		}
		
		if(!_IsReference)
			lua_pop(*_State, 1);
	}
	
	Variable::Variable(State* state, Type type) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_Type = type;
		_IsReference = false;
		_Global = false;
		
		Data.Reference = 0;
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
			Data.Reference = luaL_ref(*_State, LUA_REGISTRYINDEX);
			_IsReference = true;
			break;
		}
	}
	
	Variable::Variable(State* state, const string& value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		_Global = false;
		
		Data.String = value;
	}
	
	Variable::Variable(State* state, const char* value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::String;
		Data.String = value;
	}
	
	Variable::Variable(State* state, bool value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Boolean;
		_Global = false;
		
		Data.Boolean = value;
	}
	
	Variable::Variable(State* state, double value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		
		Data.Real = value;
	}
	
	Variable::Variable(State* state, long long value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		
		Data.Real = (double)value;
	}
	
	Variable::Variable(State* state, int value) : _State(state), _Key(nullptr), _KeyTo(nullptr)
	{
		_State = state;
		_IsReference = false;
		_Type = Type::Number;
		_Global = false;
		
		Data.Real = (double)value;
	}
	
	Variable::~Variable()
	{
		if(_IsReference) // TODO: wtf, _State is already de-referenced?
			;//luaL_unref(*_State, 0, Data.Reference);
	}
	
	void Variable::SetKey(std::shared_ptr<Variable> key, Variable* to)
	{
		_Key = nullptr;
		_Key = key;
		
		_KeyTo = to;
	}
	
	string Variable::ToString()
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
			
			lua_setglobal(*_State, _Key->As<string>().c_str()); // TODO: use Variable to index
		}
		else
		{
			if(_KeyTo == nullptr)
				throw RuntimeError("_KeyTo is null!");
			
			Variable tmp(_State, val);
			this->_IsReference = tmp._IsReference;
			this->_Type = tmp._Type;
			this->Data = tmp.Data;
			
			_KeyTo->ToStack();
			_Key->ToStack();
			tmp.ToStack();
			
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
		
		
		this->ToStack();
		key->ToStack();
				
		lua_gettable(*_State, -2);
		
		Variable ret = Variable(_State); // takes it from the stack
		ret.SetKey(key, this);
		
		lua_pop(*_State, 1);
		return ret;
	}
	
	std::list<std::pair<Variable, Variable>> Variable::pairs()
	{
		if(GetType() != Type::Table)
		{
			throw RuntimeError("Attempted to pairs '" + _Key->ToString() + "' (a " + GetTypeName() + " value)");
			return {};
		}
		std::list<std::pair<Variable, Variable>> ret;
		
		this->ToStack();
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
	
	std::list<std::pair<Variable, Variable>> Variable::ipairs()
	{
		if(GetType() != Type::Table)
		{
			throw RuntimeError("Attempted to ipairs '" + _Key->ToString() + "' (a " + GetTypeName() + " value)");
			return {};
		}
		
		int i = 1;
		Variable& self = *this;
		std::list<std::pair<Variable, Variable>> ret;
		
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
		case Type::Table:
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
