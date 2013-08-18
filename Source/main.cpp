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

int main(int argc, char** argv)
{
	cout << "\n";
	try
	{
		Lua::State state;
		
		state.LoadStandardLibary();
		state.DoString("test = {a = 'b', nested = {hi = 'gutten tag', [function() end] = true}, some_int = 1337}", "test");
		
		PrintTable(state["test"]);
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
	}
	
	return 0;
}
