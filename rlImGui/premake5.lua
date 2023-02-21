baseName = path.getbasename(os.getcwd());

project (baseName)
    kind "StaticLib"
    location "../build"
    targetdir "../bin/%{cfg.buildcfg}"
    includedirs { "./", "./include"}
   
    vpaths 
    {
        ["Header Files/*"] = { "include/**h", "**.h"},
        ["Source Files/*"] = { "**.c", "**.cpp"},
        ["Premake Files/*"] = { "*.lua"},
    }
    files {"*.h", "*.c",  "*.cpp", "extras/*.cpp", "extras/*.h","*.lua"}
	link_to("imgui")
	include_raylib();