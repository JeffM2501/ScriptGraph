
baseName = path.getbasename(os.getcwd());

project (baseName)
    kind "ConsoleApp"
    location "../_build"
    targetdir "../_bin/%{cfg.buildcfg}"

    filter "action:vs*"
        debugdir "$(SolutionDir)"
		
	filter {"action:vs*", "configurations:Release"}
		kind "WindowedApp"
		entrypoint "mainCRTStartup"
		
    filter{}

    vpaths 
    {
        ["Header Files/*"] = { "include/**.h",  "include/**.hpp", "src/**.h", "src/**.hpp", "**.h", "**.hpp"},
        ["Source Files/*"] = {"src/**.c", "src/**.cpp","**.c", "**.cpp"},
    }
    files {"**.c", "**.cpp", "**.h", "**.hpp"}
  
    includedirs { "./", "src", "include"}
    link_raylib()
	link_to("script_graph")
    link_to("rlImGui")
    link_to("imgui")
    link_to("imnodes")
	link_to("script_graph")
    link_to("tinyfiledialogs")
	-- To link to a lib use link_to("LIB_FOLDER_NAME")