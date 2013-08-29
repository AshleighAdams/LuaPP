#!/usr/bin/env lua

loadfile("ThirdParty/util.lua")(...)

local project_file = "Project.lua"
local premake = platform_specific[os.name].premake
local target = options["target"] or platform_specific[os.name].target
local premake_options = {
}

local cleans = {
	"Projects",
	"Binaries",
	"*~"
}

--------- clean
local clean_cmd = string.format("%s %s --file=" .. project_file .. " %s", premake, table.concat(premake_options, " "), "clean")

if os.execute(clean_cmd) ~= 0 then -- cleaning failed
	print("cleaning failed!")
	return
end
clean(cleans)

if options["clean"] then -- only wanted us to clean
	return
end

-------- generate
local cmd = string.format("%s %s --file=" .. project_file .. " %s", premake, table.concat(premake_options, " "), target)
os.execute(cmd)
