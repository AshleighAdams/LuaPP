// STL
#include <iostream>

// Lua
#include "Lua++.hpp"

using namespace std;

int main(int argc, char** argv)
{
	cout << "\n";
	try
	{
		Lua::State state;
		
		state.LoadStandardLibary();
		state.DoString("test = {a = 5}");
		
		state["test"]["a"] = 10;
		cout << state["test"]["a"].ToString() << "\n";
		
		state["test"]["a"] = "Hello, world";
		cout << state["test"]["a"].ToString() << "\n";
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
	}
	
	return 0;
}
