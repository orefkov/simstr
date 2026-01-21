# simstr - String object and function library
<h2>Speed up your work with strings by 2-10 times!</h2>

[![CMake on multiple platforms](https://github.com/orefkov/simstr/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/orefkov/simstr/actions/workflows/cmake-multi-platform.yml)

Version 1.4.0.

<span class="obfuscator"><a href="readme_ru.md">On Russian | По-русски</a></span>

This library contains a modern implementation of several types of string objects and various algorithms for working with strings.

The goal of the library is to make working with strings in C++ as simple and easy as in many other languages, especially
scripting languages, while maintaining optimal performance at the level of C and C++, and even improving them.

It's no secret that working with strings in C++ often causes pain. The `std::string` class is often inconvenient or inefficient.
Many functions that are usually needed when working with strings are simply not there, and everyone has to write them themselves.
Even concatenating `std::string` and `std::string_view` became possible only with C++26.
That's why I started creating this library for myself around 2012, and now I'm ready to share it with all C++ developers.

This library was not made as a universal combine that "can do everything", I implemented what I had to
use in my work, trying to do it in the most efficient way, and I modestly hope that I have succeeded in something
and will be useful to other people, either directly or as a source of ideas.

The library contains two parts:
- Implementation of [*"String Expressions"*](https://orefkov.github.io/simstr/docs_en/overview.html#autotoc_md68) and algorithms for working
  with constant strings.\
  To use this part, just take the file `"include/simstr/strexpr.h"` and write in your code
  ```cpp
  #include "path/to/file/strexpr.h"
  ```
  This will allow you to use powerful and fast *"string expressions"* for concatenation and string construction for standard string types (`std::basic_string`, `std::basic_string_view`), as well as simplified versions of the `simple_str` and
  `simple_str_nt` classes, which implement all those string algorithms of the library that do not require storing or modifying strings.
  Since this is a header-only part, it does not include working with UTF encodings and simplified Unicode.
- The full version, which requires connecting the entire library (`"include/simstr/sstring.h"`), adds its own string types with
  the ability to store and modify strings, works with UTF encodings and simplified Unicode.

The library does not pretend to be "changed the header and everything worked better" - it gets along well with standard strings
and does not change the behavior of existing code working with them. I tried to make many methods in it compatible
with `std::string` and `std::string_view`, but I didn't bother with this much. Rewriting your code to work with `simstr`
will require some effort, but I assure you that it will pay off. And thanks to compatibility with standard strings, this work can be done
in stages, in small pieces. Creating new code for working with strings with its use is easy and enjoyable :)


The main difference between `simstr` and `std::string` is that not a single universal class is used for working with strings, but several
types of objects, each of which is good for its own purposes, and at the same time interact well with each other.
If you actively used `std::string_view` and understood its advantages and disadvantages compared to `std::string`,
then the `simstr` approach will also be clear to you.

## Main features of the library
When using only `#include "simstr\strexpr.h"`:
- Support for working with strings `char`, `char8_t`, `char16_t`, `char32_t`, `wchar_t`.
- Powerful and extensible *"String Expressions"* system.
  Allows you to efficiently implement the conversion and addition (concatenation) of strings, string literals, numbers (and possibly other objects),
  achieving significant acceleration of string operations.
  Compatible with both `simstr` string objects and standard strings (`std::basic_string`,
  `std::basic_string_view`), which allows you to use fast concatenation even where it is not yet possible to abandon standard strings.
  Also allows you to mix strings of compatible character types in operations.
- Constant string functions (do not change the original string):
  - Getting substrings.
  - Comparing strings, comparing strings ignoring the case of ASCII characters.
  - Searching for substrings and characters - from the beginning or from the end of the string.
  - Various trimming of strings - right, left, everywhere, by whitespace characters, by specified characters.
  - Replacing substrings (creating a copy of the string with the replacement).
  - Replacing a set of characters with a set of corresponding substrings (creating a copy of the string with the replacement).
  - Merging (join) containers of strings into a single string, with specifying delimiters and options - "skip empty", "delimiter after last".
  - Splitting strings into parts by a specified delimiter. Splitting is possible immediately into a container with strings, or by calling a functor for
    each substring, or by iterating using the `Splitter` iterator.
- Parsing integers with the possibility of "fine" tuning at compile time - you can set options for checking overflow,
  skipping whitespace characters, a specific base or auto-selection by prefixes `0x`, `0`, `0b`, `0o`,
  admissibility of the `+` sign. Parsing is implemented for all types of strings and characters.
- Parsing double for `char` and `wchar_t`, as well as character types compatible with them in size.

When using the full version of the library:
- Everything that is listed above, plus
- Additional efficient string objects - `sstring` (shared string), `lstring` (local string).
- `lstring` - supports many mutable operations with strings - various replacements, insertions, deletions, etc.
  Allows you to set the size for the internal character buffer, which can turn *Small String Optimization* into *Big String Optimization* :).
- Transparent conversion of strings from one character type to another, with automatic conversion between UTF-8, UTF-16, UTF-32,
  using [simdutf](https://github.com/simdutf/simdutf). Strings of "compatible" types are converted by simple copying:
  `char <-> char8_t`, `wchar_t <-> char32_t` in Linux, `wchar_t <-> char16_t` in Windows.
- Integration with `format` and `sprintf` formatting functions (with automatic buffer increase).
  Formatting is possible for `char`, `wchar_t` strings and strings compatible with them in size.
  That is, under Windows it is `char8_t`, `char16_t`, under Linux - `char8_t`, `char32_t` (writing my own formatting library for all types of
  characters was not part of my plans).
- Contains minimal Unicode support when converting `upper`, `lower` and case-insensitive string comparison.
  Works only for characters of the first Unicode plane (up to 0xFFFF), and when changing the case, cases are not taken into account when one code point
  can be converted into several, that is, the case conversion of characters corresponds to `std::towupper`, `std::towlower` for the unicode locale, only faster and can work with any type of characters.
- Implemented `hash map` for string type keys, based on `std::unordered_map`, with the possibility of more efficient storage and
  comparison of keys compared to `std::string` keys. The possibility of case-insensitive comparison of keys is supported (Ascii or
  minimal Unicode (see previous paragraph)).

## String expressions
These are special objects that efficiently implement string concatenation using `operator+`.
The main principle, due to which efficient work is achieved - no matter how many operands are included in the entire expression,
no temporary (intermediate) strings are created, the total length of the entire result is calculated only once,
memory is allocated for the character buffer of the result only once, after which the characters are copied immediately to the buffer of the result
to its place. No memory reallocations, no moving characters in various intermediate buffers - everything is
as efficient as possible. Thanks to the capabilities of C++ templates and operator overloading, the expression is written as close as possible
to the usual string addition syntax.
In addition, there are special overloads for adding string objects and string literals, strings and numbers,
for copying with replacement, for merging containers of strings and much more.
Thanks to the extensibility of this system, it is possible to create new options for building strings, development is constantly ongoing.

All string objects from `simstr` are themselves string expressions, that is, they can be used in concatenation operations
of string expressions directly. Standard strings (`std::basic_string`, `std::basic_string_view`) can also serve as operands
in addition operations with string expressions. Or they can be easily converted into a string expression by placing a
unary `+` in front of them.

## Usage examples
### Adding strings with numbers
```cpp
std::string s1 = "start ";
int i;
....
// Was
    std::string str = s1 + std::to_string(i) + " end";
// Became
    std::string str = +s1 + i + " end";
```
`+s1` - converts `std::string` into an object - a string expression, for which there is an efficient concatenation with numbers and string literals.

According to benchmarks, [acceleration is 1.6 - 2 times](https://orefkov.github.io/simstr/results.html#bs70109915512075798510).

### Adding strings with numbers in hex format
```cpp
....
// Was
    std::string str = s1 + std::format("0x{:x}", i) + " end";
// Became
    std::string str = +s1 + e_hex<HexFlags::Short>(i) + " end";
```
Acceleration in [**9 - 14 times!!!**](https://orefkov.github.io/simstr/results.html#bs146911715078927772520)

### Adding multiple literals and searching in `std::string_view`
```cpp
// It was like this
size_t find_pos(std::string_view src, std::string_view name) {
    // before C++26 we can not concatenate string and string_view...
    return src.find("\n- "s + std::string{name} + " -\n");
}
// When using only "strexpr.h" it became like this
size_t find_pos(ssa src, ssa name) {
    return src.find(std::string{"\n- " + name + " -\n"});
}

// And when using the full library, you can do this
size_t find_pos(ssa src, ssa name) {
    // In this version, if the result of the concatenation fits into 207 characters, it is produced in a buffer on the stack,
    // without allocation and deallocation of memory, acceleration is several times. And only if the result is longer than 207 characters -
    // there will be only one allocation, and the concatenation will be immediately into the allocated buffer, without copying characters.
    return src.find(lstringa<200>{"\n- " + name + " -\n"});
}
```
`ssa` - alias for `simple_str<char>` - analogue of `std::string_view`, allows you to accept any string object as a function parameter with minimal costs,
which does not need to be modified or passed to the C-API: `std::string`, `std::string_view`, `"string literal"`,
`simple_str_nt`, `sstring`, `lstring`. Also, since it is also a "string expression", it allows you to easily
build concatenations with its participation.

According to measurements, [acceleration is 1.5 - 9 times](https://orefkov.github.io/simstr/results.html#bs68116594352702954700).

### Addition with conditions
```cpp
// Was
std::string buildTypeName(std::string_view type_name, size_t prec, size_t scale) {
    std::string res{type_name};
    if (prec) {
        res += "(" + std::to_string(prec);
        if (scale) {
            res += "," + std::to_string(scale);
        }
        res += ")";
    }
    return res;
}
// Became when using only strexpr.h and wanting to use only standard strings
std::string buildTypeName(std::string_view type_name, size_t prec, size_t scale) {
    if (prec) {
        //     + turns type_name from string_view into a string expression
        return +type_name + "(" + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return type_name;
}
// Became when using only strexpr.h and simple_str string
std::string buildTypeName(ssa type_name, size_t prec, size_t scale) {
    if (prec) {
        //     ssa is already a string expression, + before it is not needed
        return type_name + "(" + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return type_name;
}
// Became when using the full library
stringa buildTypeName(ssa type_name, size_t prec, size_t scale) {
    if (prec) {
        return type_name + "(" + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return type_name;
}
```
When `prec != 0`, [acceleration is 1.5 - 2.2 times](https://orefkov.github.io/simstr/results.html#bs145290966789248325200).

### Addition with replacements
```cpp
// Was
// There is no standard analogue of the replace function from other programming languages, let's write our own "head-on".
std::string str_replace(std::string_view from, std::string_view pattern, std::string_view repl) {
    std::string result;
    for (size_t offset = 0;;) {
        size_t pos = from.find(pattern, offset);
        if (pos == std::string::npos) {
            result += from.substr(offset);
            break;
        }
        result += from.substr(offset, pos - offset);
        result += repl;
        offset = pos + pattern.length();
    }
    return result;
}

std::string make_str_str(std::string_view from, std::string_view pattern, std::string_view repl) {
    return "<" + str_replace(from, pattern, repl) + ">";
}
// Became - copying with replacements
std::string make_str_exp(std::string_view from, std::string_view pattern, std::string_view repl) {
    return "<" + e_repl(from, pattern, repl) + ">";
}
```
[Acceleration from 1.5 times and higher](https://orefkov.github.io/simstr/results.html#bs54035654251116789780) - depending on the content of the strings.

### Splitting strings into parts, parsing numbers
```cpp
// Was - split the string by delimiter and calculate the sum of numbers
int split_and_calc_total_str(std::string_view numbers, std::string_view delimiter) {
    int total = 0;
    for (size_t start = 0; start < numbers.length(); ) {
        int delim = numbers.find(delimiter, start);
        if (delim == std::string::npos) {
            delim = numbers.size();
        }
        std::string part{numbers.substr(start, delim - start)};
        total += std::strtol(part.c_str(), nullptr, 0);
        start = delim + delimiter.length();
    }
    return total;
}
// Became
int split_and_calc_total_sim(ssa numbers, ssa delimiter) {
    int total = 0;
    for (auto splitter = numbers.splitter(delimiter); !splitter.is_done();) {
        total += splitter.next().as_int<int>();
    }
    return total;
}
```
[Acceleration in 2-3 times](https://orefkov.github.io/simstr/results.html#bs7106975351756760120).

In addition to the individual examples given here, you can look at the sources:
- [tests of the entire library](https://github.com/orefkov/simstr/blob/main/tests/test_str.cpp)
- [tests of only the strexpr part](https://github.com/orefkov/simstr/blob/main/tests/test_expr_only.cpp)
- [benchmarks](https://github.com/orefkov/simstr/blob/main/bench/bench_str.cpp)
- [utility for preparing html](https://github.com/orefkov/simstr/blob/main/bench/process_result.cpp) from benchmark results.

## Main objects of the library
Available with any use:
- `simple_str<K>` - the simplest string (or piece of string), immutable, not owning, analogue of `std::string_view`.
- `simple_str_nt<K>` - the same, only declares that it ends with 0. For working with third-party C-API.

Available when using the entire library:
- `sstring<K>` - shared string, immutable, owning, with shared character buffer, SSO support.
- `lstring<K, N>` - local string, mutable, owning, with a specified size of the SSO buffer.

When connecting only `strexpr.h` - the types `simple_str<K>` and `simple_str_nt<K>` do not contain methods for working with UTF and Unicode.

## Articles
- [Overview and introduction](docs/overview.md)
- [Overview article on Habr](https://habr.com/ru/articles/935590) (On Russian)
- [Description of the "Expression Templates" technique used](https://habr.com/ru/articles/936468/) (On Russian)

## Usage
The library can be used partially, just by taking the file `"include\simstr\strexpr.h"` and including it in your sources
```cpp
#include "include\simstr\strexpr.h"
```
This will only connect string expressions and simplified implementations of `simple_str` and `simple_str_nt`, without UTF and Unicode functions.

The full version of the `simstr` library consists of three header files and two source files.
You can connect as a CMake project via `add_subdirectory` (the `simstr` library),
you can simply include the files in your project. Building also requires [simdutf](https://github.com/simdutf/simdutf) (when using CMake
it is downloaded automatically).

The library is included in [vcpkg](https://vcpkg.io), connected as `orefkov-simstr`.

`simstr` requires a compiler of at least the C++20 standard - concepts and std::format are used.
The work was tested under Windows on MSVC-19 and Clang-19, under Linux - on GCC-13 and Clang-21.
The work in WASM was also tested, built in Emscripten 4.0.6, Clang-21.


## Convenient debugging
Together with the library, two files are supplied that make viewing simstr string objects in debuggers
more convenient.\
More details are described [here](for_debug/readme_ru.md).

## Benchmarks
Benchmarks are performed using the [Google benchmark](https://github.com/google/benchmark) framework.
I tried to take measurements for the most typical operations that occur in normal work. I took measurements on my equipment, under
Windows and Linux (in WSL), using MSVC, Clang, GCC compilers. Third-party results are welcome.
I also took measurements in WASM, built in Emscripten. I draw your attention to the fact that a 32-bit build is assembled under WASM in Emscripten, which means that
the sizes of SSO buffers in objects are smaller.

- [Source code of benchmarks](bench/bench_str.cpp)
- [Benchmark results](https://orefkov.github.io/simstr/results.html)

Also, simstr is used in my projects:
- [simjson](https://github.com/orefkov/simjson) - a library for simple work with JSON using simstr strings.
- [simrex](https://github.com/orefkov/simrex) - wrapper for working with regular expressions [Oniguruma](https://github.com/kkos/oniguruma) using simstr strings.
- [v8sqlite](https://github.com/orefkov/v8sqlite) - external component for 1C-Enterprise V8 for working with sqlite.


## Generated documentation
[Located here](https://orefkov.github.io/simstr/docs_en/)
