
function os.capture(cmd, raw)
	local f = assert(io.popen(cmd, 'r'))
	local s = assert(f:read('*a'))
	f:close()
	if raw then return s end
	s = string.gsub(s, '^%s+', '')
	s = string.gsub(s, '%s+$', '')
	s = string.gsub(s, '[\n\r]+', ' ')
	return s
end

platform_specific = {
	windows = {
		premake = "ThirdParty\\Premake4.exe",
		target = "vs2010",
		delete = "del /f /Q /S \"%s\""
	},
	linux = {
		premake = "./ThirdParty/Premake4.elf",
		target = "gmake",
		delete = "rm -rf \"./%s\""
	},
	mac = {
		premake = "./ThirdParty/Premake4.osx",
		target = "xcode",
		delete = "rm -rf \"./%s\""
	}
}

local ret = os.capture("uname -a")

if ret == nil then
	os.name = "windows"
elseif ret:find("Linux") then
	os.name = "linux"
elseif ret:find("Darwin") then
	os.name = "mac"
end

-- parse the arguments
options = {}
args = {}

for k, v in pairs({...}) do
	local long_set = {v:find("--(%w+)=(%w+)")}
	local long = {v:find("--(%w+)")}
	local short = {v:find("-(%w+)")}
	
	if long_set[1] then
		options[long_set[3]] = long_set[4]
	elseif long[1] then
		options[long[3]] = true
	elseif short[1] then
		local opts = short[3]
		for i = 0, opts:len() do
			options[opts[i]] = true
		end
	end
end

if options["target"] and options["target"] == "monodev" then -- becase premake removed this, for some reason...
	options["target"] = "vs2003"
end


function clean(what)
	for k,v in pairs(what) do
		local cmd = string.format(platform_specific[os.name].delete, v)
		os.execute(cmd)
	end
end