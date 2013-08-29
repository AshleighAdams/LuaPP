// STL
#include <iostream>

// Lua
#include "Lua++.hpp"

using namespace std;

class Vector
{
public:
	double X = {}, Y = {}, Z = {};
	Vector()
	{
	}
	
	Vector(double x, double y, double z) : X(x), Y(y), Z(z)
	{
	}
	
	double Length()
	{
		return sqrt(X*X+Y*Y+Z*Z);
	}
	
	std::vector<Lua::Variable> LuaLength(Lua::State* state, std::vector<Lua::Variable>& args)
	{
		return {{state, Length()}};
	}

	std::vector<Lua::Variable> LuaNormalize(Lua::State* state, std::vector<Lua::Variable>& args)
	{
		auto vec = args[0].As<std::shared_ptr<Vector>>();

		double len = this->Length();
		X /= len;
		Y /= len;
		Z /= len;
		
		return {};
	}
};

std::vector<Lua::Variable> LuaNewVector(Lua::State* state, std::vector<Lua::Variable>& args)
{
	size_t len = args.size();
	double X, Y, Z;
	X = Y = Z = 0;
	
	if(len >= 1)
		X = args[0].As<double>();
	if(len >= 2)
		Y = args[1].As<double>();
	if(len >= 3)
		Z = args[2].As<double>();
	
	auto ptr = std::make_shared<Vector>(X, Y, Z);
	
	return {state->GeneratePointer(ptr)};
}

int Add(int a, int b)
{
	return a + b;
}


int main(int argc, char** argv)
{
	using namespace Lua;
	
	State state;
	
	try
	{
		state.LoadStandardLibary();

		state.GenerateMemberFunction("Length", &Vector::LuaLength);
		state.GenerateMemberFunction("Normalize", &Vector::LuaNormalize);
		
		state["Vector"] = state.GenerateFunction(LuaNewVector);
		//state["Add"] = state.GenerateFunction<int(int, int)>(Add);
		
		assert(Variable(&state, 5) == Variable(&state, 5));
		
		state.DoFile("/home/kobra/Dropbox/Projects/Lua++/Projects/test.lua"); 
		
		state["TestFunction"]("Hello, Ddrl46");
	}
	catch(Lua::Exception ex)
	{
		cout << "Lua exception: \n" << ex.what() << "\n";
		return 1;
	}
	
	return 0;
}
