baseName = path.getbasename(os.getcwd());

project (baseName)
    kind "StaticLib"
	location "../build"
    targetdir "../bin/%{cfg.buildcfg}"
    includedirs { "./"}
   
    vpaths 
    {
        ["Header Files/*"] = { "include/**h", "**.h"},
        ["Source Files/*"] = { "**.c", "**.cpp"},
        ["Premake Files/*"] = { "*.lua"},
    }
    files {"*.h", "*.c", "*.cpp", "*.lua"}
