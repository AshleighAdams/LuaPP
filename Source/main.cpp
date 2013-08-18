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
	{
		if(var.GetType() == Lua::Type::Table)
		{
			cout << "\t" << var.GetTypeName() << ":\n";
			PrintTable(var, 2);
			continue;
		}
		cout << "\t" << var.GetTypeName() << ": " << var.ToString() << "\n";
	}
	
	return {};
}

int main(int argc, char** argv)
{
	using namespace Lua;
	
	try
	{
		State state;
		state.LoadStandardLibary();
		
		// Generate a Lua variable for our function
		Variable testfunc = state.GenerateFunction(TestFunc);
		// Set it as a global variable
		state["TestFunc"] = testfunc;
		
		// load and execute a file
		state.DoFile("test.lua"); 
		
		
		state["GiveMeAString"]("hello"); // call the function GiveMeAString, with the argument "Hello"
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
		return 1;
	}
	
	return 0;
}
