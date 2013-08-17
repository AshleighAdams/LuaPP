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
		state.DoString("test = 5");
		state["test"]();
		
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
	}
	
	return 0;
}
