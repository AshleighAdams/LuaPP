solution "Lua++"
	language "C++"
	location "Projects"
	targetdir "Binaries"
	configurations { "Release", "Debug" }

	configuration "Debug"
		flags { "Symbols" }
	configuration "Release"
		flags { "Optimize" }
	
	project "Lua++"
		files
		{
			"Source/**.hpp", "Source/**.cpp"
		}
		vpaths
		{
			["Source Files"] = "Source/**.cpp",
			["Header Files"] = "Source/**.hpp"
		}
		
		kind "ConsoleApp" -- StaticLib, SharedLib
		
		configuration "windows"
			libdirs { "ThirdParty/Libraries" }
			includedirs { "ThirdParty/Include" }
			defines { "WINDOWS" }

		configuration "linux"
			buildoptions { "-std=c++11", "`pkg-config --cflags lua5.2`" }
			links { "pthread", "lua5.2" } -- for std::thread
			defines { "LINUX" }
			
		configuration "Debug"
			targetsuffix "_d"
			
		links { } -- Such as { "GL", "X11" }

		configuration "linux"
			excludes { } -- "Source/WindowsX.cpp"
		configuration "windows"
			excludes { }
