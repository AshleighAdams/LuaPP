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
		state.DoString("test = {42, 1337, 'Hello, world'}", "test");
		
		for(auto pair : state["test"].ipairs())
		{
			cout << pair.first.ToString() << " = " << pair.second.GetTypeName() << ": " << pair.second.ToString() << "\n";
		}
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
	}
	
	return 0;
}
