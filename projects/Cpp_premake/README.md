# Cpp_premake

Uses [premake](https://premake.github.io/). Build like so:

1. `premake5 gmake`
2. ` make -j14 config=release`

What I like:
* Very simple to use
* Easy to setup

What I don't like:
* Not powerful
* There doesn't seem to be many features. How to enable `ccache`, or `clang-tidy`? Best I found was to code this oneself: https://github.com/premake/premake-core/issues/1475
* I don't know much about `lua`.
* Cannot easily create `compile_commands.json` which is necessary for VScode Clangd
* no conan integration
* Generates makefiles, no Ninja build
* Doesn't automatically configure when files are added
