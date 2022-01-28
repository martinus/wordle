workspace "wordle"
    configurations { "release", "debug" }

project "wordle"
    kind "ConsoleApp"
    language "C++"
    targetdir "bin/%{cfg.buildcfg}"
    defaultplatform "configurations:release"

    toolset "clang"
    cppdialect "C++17"
    warnings "Extra"

    files { "src/**.h", "src/**.cpp" }
    includedirs { "src"}
    linkoptions { "-lpthread" }

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Debug"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "Full"
