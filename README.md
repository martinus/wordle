# wordle solver

An experimental wordle solver. I intend to write a simple wordle solver (at least with some heuristics that helps solving). My goal is to implement
this in multiple different languages, to learn them. Ideally with proper unit tests in multiple languages.

Play wordle here: https://www.powerlanguage.co.uk/wordle/

## Why?

There are several nice things about wordle solver:

* It's fun
* Requires several different things: command line parser, string handling, file parsing (dictionary), relatively performant comparison, ...

My plan is to

1. Implement a working version in C++, the language I am most proficient.
2. Code cleanup with unit tests etc.
3. Implement in other language (e.g. Rust, C#, Java, Go, ...)


# Projects

## Cpp_simple

Very simple C++ implementation with a single file. Compile like this:

```cpp
clang++ -O3 -std=c++17 src/Cpp/wordle.cpp -o wordle
./wordle
```

Advantages:
* Works well for very small projects

Disadvantages:
* Unit testing difficult
* Can't split up code well.
* Slow compile times

## Cpp_premake

Uses [premake5](https://premake.github.io/)

Advantages:
* Very simple to use
* Easy to setup

Disadvantages:
* Not powerful
* Cannot easily create `compile_commands.json` which is necessary for VScode Clangd
* no conan integration
* Generates makefiles, no Ninja build
* Doesn't automatically configure when files are added

# Cpp_meson