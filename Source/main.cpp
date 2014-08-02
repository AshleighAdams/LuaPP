// STL
#include <iostream>

// Lua
#include "Lua++.hpp"

using namespace std;
using namespace Lua;
// tests
#define check(_X_) if(!(_X_)) return false

bool stack_error = false;
struct StackCheck
{
	State& s;
	StackCheck(State& s) : s(s) {}
	~StackCheck()
	{
		if (lua_gettop(s) != 0)
			stack_error = true;
	}
};

#define CHECK_STACK StackCheck _check(state);

bool test_std()
{
	State state;
	CHECK_STACK;

	check(state["string"].IsNil());
	check(state["table"].IsNil());
	check(state["debug"].IsNil());
	check(state["package"].IsNil());
	check(state["math"].IsNil());
	check(state["io"].IsNil());
	check(state["os"].IsNil());

	state.LoadStandardLibary();
	
	check(state["string"].GetTypeName() == "table");
	check(state["table"].GetTypeName() == "table");
	check(state["debug"].GetTypeName() == "table");
	check(state["package"].GetTypeName() == "table");
	check(state["math"].GetTypeName() == "table");
	check(state["io"].GetTypeName() == "table");
	check(state["os"].GetTypeName() == "table");
	
	
	return true;
}

bool test_conversions()
{
	State state;
	CHECK_STACK;

	check(Variable(&state, 5) == Variable(&state, 5));
	check(Variable(&state, 5) == 5);
	check(Variable(&state, "testing").GetTypeName() == "string");

	state["int_value"] = 1;
	check(state["int_value"] == 1);
	
	return true;
}

bool test_error_runtime()
{
	try
	{
		State state;
		state.DoString("return nil .. 0");
	}
	catch(Lua::Exception ex)
	{
		return true;
	}
	
	return false;
}

bool test_cppobject()
{
	stringstream s;
	struct Class
	{
		stringstream& s;
		int value;
		Class(const Class& b) : s(b.s) { s << "cp"; }
		Class(stringstream& i) : s(i) { s << "ct"; }
		~Class() { s << "dt"; }
	};
	Lua::State state;
	CHECK_STACK;
	{
		Variable v(&state, Class(s));
		v.As<Class>().value = 1;
		check(v.As<Class>().value == 1);
	}
	lua_gc(state, LUA_GCCOLLECT, 0);
	return s.str() == "ctcpdtdt";
}

bool test_cppfunction()
{
	class Class
	{
	public:
		int value;
		void void_func(int a, int b)
		{
		}
		bool bool_func(int a, int b)
		{
			return a == 1 && b == 2;
		}
		int class_func(Class& c)
		{
			return c.value + value;
		}
		static bool int_func_s(int a, int b)
		{
			return a == 3 && b == 4;
		}
	};
	Class c{ 1 };
	State state;
	CHECK_STACK;
	Variable v1 = Variable::FromMemberFunction<Class>(&state, &Class::bool_func);
	check(v1(&c, 1, 2).First() == true);
	Variable v2 = Variable::FromMemberFunction<Class>(&state, &Class::void_func);
	check(v2(&c, 1, 2).Size() == 0);
	Variable v3 = Variable::FromMemberFunction<Class>(&state, &Class::class_func);
	check(v3(&c, Class{ 2 }).ToArray()[0] == Variable(&state, 3));
	Variable v4 = Variable::FromFunction(&state, &Class::int_func_s);
	check(v4(3, 4).First() == true);
	return true;
}

bool failed;
void test(const std::string& what, std::function<bool()> func)
{
	if(!func())
	{
		failed = true;
		cout << what << "... failed\n";
	}
	else
	{
		cout << what << "... ok\n";
	}
	if (stack_error)
	{
		cout << what << ": stack not cleared\n";
	}
}

void test()
{
	test("Standard libary loads", test_std);
	test("Variable conversions", test_conversions);
	test("Exceptions on runtime lua", test_error_runtime);
	test("C++ object manipulate", test_cppobject);
	test("C++ function manipulate", test_cppfunction);
}

int main(int argc, char** argv)
{
	test();
	return 0;
}
