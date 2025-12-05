# simstr - String object and function library
[![CMake on multiple platforms](https://github.com/orefkov/simstr/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/orefkov/simstr/actions/workflows/cmake-multi-platform.yml)

Version 1.2.8.

<span class="obfuscator"><a href="readme_ru.md">On Russian | По-русски</a></span>

This library contains the modern implementation of several types of string objects and various algorithms for working with strings.

The goal of the library is to make working with strings in C++ as simple and easy as in many other languages, especially
scripting languages, while maintaining optimality and performance at the level of C and C++, and even improving them.

It's no secret that working with strings in C++ often causes pain. The `std::string` class is often inconvenient or inefficient.
Many functions that are usually necessary when working with strings are simply not there, and everyone has to write them themselves.

This library was not made as a universal combine that "can do everything", I implemented what I had to
use at work, trying to do it in the most efficient way, and I modestly hope that I succeeded in something
and will be useful to other people, either directly or as a source of ideas.

The library does not pretend to be a "change the header and everything works better" solution. I tried to make many methods compatible
with `std::string` and `std::string_view`, but I didn't bother with it much. Rewriting old code to work with simstr
will require some effort, but I assure you that it will pay off. And writing new code with its use is easy and enjoyable :)

The main difference between simstr and std::string is that instead of a single universal class, several
types of objects are used to work with strings, each of which is good for its own purposes, and at the same time interacts well with each other.
If you actively used std::string_view and understood its advantages and disadvantages compared to std::string,
then the simstr approach will also be clear to you.

## Main features of the library
- Strings `char`, `char16_t`, `char32_t`, `wchar_t`.
- Transparent conversion of strings from one character type to another, with automatic conversion between UTF-8, UTF-16, UTF-32,
  using [simdutf](https://github.com/simdutf/simdutf).
- Extensible "String Expression" system. Allows you to efficiently implement the conversion and addition (concatenation) of strings, literals,
  numbers and possibly other objects.
- String functions:
  - Getting substrings.
  - Searching for substrings and characters - from the beginning or from the end of the string.
  - Various string trimming - right, left, everywhere, by whitespace characters, by specified characters.
  - Replacing substrings.
  - Replacing a set of characters with a set of corresponding substrings.
  - Merging (join) containers of strings into a single string, with specifying separators and options - "skip empty", "separator after last".
  - Splitting strings into parts by a specified separator. Splitting is possible directly into a container with strings, or by calling a functor for
    each substring, or by iterating using the `Splitter` iterator.
- Integration with `format` and `sprintf` formatting functions (with automatic buffer increase).
  Formatting is possible for `char`, `wchar_t` strings and strings compatible with `wchar_t` in size.
  That is, under Windows it is `char16_t`, under Linux - `char32_t`. Writing my own formatting library was not part of my plans.
- Parsing integers with the possibility of "fine" tuning during compilation - you can set options for checking overflow,
  skipping whitespace characters, a specific radix or auto-selection by prefixes `0x`, `0`, `0b`, `0o`,
  admissibility of the `+` sign. Parsing is implemented for all types of strings and characters.
- Parsing doubles for all types of characters.
- Minimal Unicode support is included when converting `upper`, `lower` and case-insensitive string comparison.
  It only works for characters in the first plane of Unicode (up to 0xFFFF), and when changing case, it does not take into account cases where one code point
  can be converted into several, that is, the case conversion of characters corresponds to `std::towupper`, `std::towlower` for the unicode locale, only faster and can work with any type of characters.
- Implemented `hash map` for string type keys, based on `std::unordered_map`, with the possibility of more efficient storage and
  comparison of keys compared to `std::string` keys. Case-insensitive key comparison is supported (Ascii or
  minimal Unicode (see previous paragraph)).

## Main objects of the library
- simple_str&lt;K> - the simplest string (or piece of string), immutable, not owning, analogue of `std::string_view`.
- simple_str_nt&lt;K> - the same, only declares that it ends with 0. For working with third-party C-API.
- sstring&lt;K> - shared string, immutable, owning, with shared character buffer, SSO support.
- lstring&lt;K, N> - local string, mutable, owning, with a specified size of the SSO buffer.

## Articles
- [Overview and introduction](docs/overview.md)
- [Overview article on Habr](https://habr.com/ru/articles/935590)
- [Description of the "Expression Templates" technique used](https://habr.com/ru/articles/936468/)

## Usage
`simstr` consists of three header files and two source files. You can connect as a CMake project via `add_subdirectory` (the `simstr` library),
you can simply include the files in your project. Building also requires [simdutf](https://github.com/simdutf/simdutf) (when using CMake
it is downloaded automatically).

`simstr` requires a compiler of standard no lower than C++20 to work - concepts and std::format are used.
The work was tested under Windows on MSVC-19 and Clang-19, under Linux - on GCC-13 and Clang-21.
The work in WASM was also tested, built in Emscripten 4.0.6, Clang-21.

## Convenient debugging
Along with the library, two files are supplied that allow viewing simstr string objects in debuggers
more convenient.\
It is described in more detail in [here](for_debug/readme.md).

## Benchmarks
Benchmarks are performed using the [Google benchmark](https://github.com/google/benchmark) framework.
I tried to make measurements for the most typical operations that occur in normal work. I took measurements on my equipment, under
Windows and Linux (in WSL), using MSVC, Clang, GCC compilers. Third-party results are welcome.
I also took measurements in WASM, built in Emscripten. I draw your attention to the fact that a 32-bit build is assembled under WASM in Emscripten, which means that
the sizes of SSO buffers in objects are smaller.

- [Benchmark source code](bench/bench_str.cpp)
- [Benchmark results](https://snegopat.ru/simstr/results.html)

## Usage examples
While no separate usage examples have been prepared, you can look at the texts of [tests](https://github.com/orefkov/simstr/blob/main/tests/test_str.cpp),
[benchmarks](https://github.com/orefkov/simstr/blob/main/bench/bench_str.cpp), and
[html preparation utilities](https://github.com/orefkov/simstr/blob/main/bench/process_result.cpp) from the benchmark results.

Also simstr is used in my projects:
- [simjson](https://github.com/orefkov/simjson) - a library for simple work with JSON using simstr strings.
- [simrex](https://github.com/orefkov/simrex) - a wrapper for working with [Oniguruma](https://github.com/kkos/oniguruma) regular expressions using simstr strings.
- [v8sqlite](https://github.com/orefkov/v8sqlite) - external component for 1C-Enterprise V8 for working with sqlite.

## Generated documentation
[Located here](https://snegopat.ru/simstr/docs/)
