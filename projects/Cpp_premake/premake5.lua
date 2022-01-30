workspace "wordle"
    configurations { "release", "debug" }
    defaultplatform "configurations:release"
    toolset "clang"
    kind "ConsoleApp"
    cppdialect "C++17"
    warnings "Extra"
    targetdir "bin/%{cfg.buildcfg}"
    language "C++"

    linkoptions { "-lpthread" }
    includedirs { "src"}

    filter "configurations:debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Debug"

    filter "configurations:release"
        defines { "NDEBUG" }
        optimize "Full"


project "tests"
    files { "src/**.h", "src/test/**.cpp" }

project "wordle"
    files { "src/**.h", "src/wordle/**.cpp" }
