workspace "wordle"
    configurations { "debug", "release" }

project "wordle"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"

    files { "**.h", "**.cpp" }

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Debug"
    
    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "Full"
