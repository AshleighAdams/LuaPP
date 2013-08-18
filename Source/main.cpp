// STL
#include <iostream>

// Lua
#include "Lua++.hpp"

using namespace std;

void PrintTable(Lua::Variable tbl, int depth = 0)
{
	for(auto pair : tbl.pairs())
	{
		for(int i = 0; i < depth; i++)
			cout << '\t';
		
		if(pair.second.GetType() == Lua::Type::Table)
		{
			cout << pair.first.ToString() << ":\n";
			return PrintTable(pair.second, depth + 1);
		}
		
		cout << pair.first.ToString() << " = " << pair.second.ToString() << "\n";
	}
}

list<Lua::Variable> TestFunc(Lua::State* state, list<Lua::Variable>& args)
{
	cout << "This is a test function, " << args.size() << " arguments provided, which are: \n";
	
	for(Lua::Variable& var : args)
		cout << "\t" << var.GetTypeName() << ": " << var.ToString() << "\n";
	
	return {};
}

int main(int argc, char** argv)
{
	cout << "\n";
	try
	{
		Lua::State state;
		state.LoadStandardLibary();
		
		
		Lua::Variable func = state.GenerateFunction(TestFunc);
		
		state["test"] = func;
		state.DoString("test('hello, world', 42, false)", "test");
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
	}
	
	return 0;
}
