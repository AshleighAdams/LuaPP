Lua++
=========

![Build Status](http://vps.arb0c.net/build/LuaPP/state.png?wtf=1)

Provides a modern Lua header-only wrapper, utilizing C++11 features and reamining
as close to Lua's syntax as possible.

Some example(s):



```lua
function Test(...)
	local args = {...}
	for k,v in ipairs(args) do
		print(type(v) .. ": " .. tostring(v))
	end
end
```

```c++
try
{
	Lua::State state;
	state.LoadStandardLibary();
	state.DoFile("test.lua");
	
	state["Test"]("Hello", Lua::NewTable(), 42, 3.141);
	
	// you can create references to it:
	Lua::Variable func = state["Test"];
	func("world");
}
catch(Lua::Exception ex)
{
	cout << "Lua exception: \n" << ex.what() << "\n";
}
```
