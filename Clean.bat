@echo off
ThirdParty\Premake4.exe --file=Lua++.lua clean

del /f /Q /S "Projects"
del /f /Q /S "Binaries"

