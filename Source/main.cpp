// STL
#include <iostream>

// Lua
#include "Lua++.hpp"

using namespace std;
using namespace Lua;
// tests
#define check(_X_) if(!(_X_)) return false

bool test_std()
{
	State state;
	
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
	
	check(Variable(&state, 5) == Variable(&state, 5));
	check(Variable(&state, "testing").GetTypeName() == "string");
	
	return true;
}

bool test_error_runtime()
{
	try
	{
		State state;
		state.DoString("local a, b = 10; return a .. b");
	}
	catch(Lua::Exception ex)
	{
		return true;
	}
	
	return false;
}

bool test_cppfunction()
{
	class Class
	{
	public:
		void void_func(int a, int b)
		{
		}
		int int_func(int a, int b)
		{
			return a + b;
		}
		static int lua(lua_State* L) { return 0; }
	};
	Class c;
	State state;
	Variable i = Variable::FromMemberFunction<Class>(&state, &Class::int_func);
	check(i(&c, 1, 2)[0] == Variable(&state, 3));
	Variable v = Variable::FromMemberFunction<Class>(&state, &Class::void_func);
	check(v(&c, 1, 2).size() == 0);
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
		cout << what << "... ok\n";
}

int test()
{
	failed = false;
	
	test("Standard libary loads", test_std);
	test("Variable conversions", test_conversions);
	test("Exceptions on runtime lua", test_error_runtime);
	test("C++ function manipulate", test_cppfunction);
	
	return failed ? 1 : 0;
}

int main(int argc, char** argv)
{
	return test();
}
