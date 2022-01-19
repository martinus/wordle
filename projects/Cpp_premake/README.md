Trying to learn premake build system

See https://premake.github.io/docs/pchheader

Build like so:

1. `premake5 gmake`
2. ` make -j14 config=release`

What I like:
* very simple, full programming language.

What I don't like:
* There doesn't seem to be many features. How to enable `ccache`, or `clang-tidy`? Best I found was to code this oneself: https://github.com/premake/premake-core/issues/1475
* I don't know much about `lua`.
