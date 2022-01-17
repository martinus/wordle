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


# Build

## C++

```cpp
clang++ -O2 -std=c++17 src/Cpp/wordle.cpp -o wordle
./wordle
```
