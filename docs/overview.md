# Strings in C++
(, what's wrong with you?)

<span class="obfuscator"><a href="overview_ru.md">On Russian | По-русски</a></span>

<cite>
In a 1991 retrospective on the history of C++, its creator Bjarne Stroustrup called the lack of a standard string type
(and some other standard types) in C++ 1.0 the worst mistake he made in its development:
"Their absence led to everyone reinventing the wheel and to an unnecessary diversity in the most fundamental classes"
</cite>

## What was and is
In the introductory part, I want to briefly describe the current state of strings in C++, how we got to it, and why it is so.
I will also describe the shortcomings of current implementations so that the solutions I use in my string library are clear.

Actually, initially there was no standard type for strings in C++.
The approach from C was used to work with strings – a string is a pointer to an array of bytes ending in zero.
The disadvantages of such strings are that it is impossible to use the byte `0` in the string, i.e. it is not suitable for binary data,
the resource management/ownership strategy is unclear, and the main disadvantage is that the length of the string has to be calculated each time,
iterating over all its characters.

The origin of this solution is quite clear – from the time of the dinosaurs: just as dinosaurs were large, with small brains and
short arms, so computers were large, memory was small, and strings were short. Saving memory on storing the length of a string was more important than losing time on repeatedly calculating the length.

The first attempts to standardize strings as a class began only in C++98 - std::string appeared as part of STL, and like
much of STL, it was extremely ambiguously perceived by programmers.

And the first thing that comes to mind when improving C-strings is that you need to store the length of the string:
```cpp
    struct simple_string {
        const char* data;
        size_t length;
    };
```

With such a string, many algorithms are significantly optimized.
For example, when comparing two strings for equality, we may not even start comparing their characters if the lengths of the strings are not equal.
Moreover, this data is absolutely sufficient for all methods that do not modify the string.
Also note that such an object on modern 64-bit architectures is perfectly passed to functions by value –
both of its fields fit into registers (well, except for Windows), which makes it easier for the compiler optimizer to work.

Meanwhile, such a solution only made it into the standard in C++17, in the form of `std::string_view`.
Apparently, only then could the committee be convinced that strings are different, and using only one universal object
for strings – at the very least, can lead to a decrease in performance, and also violates the principle of "don't pay for what you
don't use". Why are "strings different" and why is one type of string not enough for us, we will consider just below.

### Resources
And the next question that arises with strings is resource ownership.
Almost every major framework solved this problem on its own, inventing its own bicycles.
We have `std::string`, in QT we have `QString`, in MFC - `CString`, in ATL - `CAtlString`, there are own strings in Folly,
in general, "thousands of them", any game engine starts with writing its own strings.

Many of these implementations in the aspect of resource management used the approach to improve performance
**COW** – “Copy On Write”. In this case, the string object referred to a buffer shared between several objects with the characters
of the string and a reference counter to this buffer, which allowed you to quickly create a copy of the string, and actually copy the characters only
when it is modified.

But they all coincided in one thing – the string was always assumed to be mutable, that is, we can modify the characters in the string buffer.

### Mutability / immutability
Because of this, the **COW** approach died by C++11: for each operation that could modify the characters of the string, it was necessary to check
whether we are referring to a shared buffer, and if so, copy the characters to another buffer.
In a multithreaded environment, you also need to check whether you now need to free the old buffer, and of course, all this is smeared with
locks or atomics, which is also not free.
Therefore, starting with C++11, `std::string` does not use **COW**, and each copying of a string object also leads to copying
all the characters of the string to another buffer.

Naturally, each new buffer requires memory allocation, which they are trying to slightly optimize through **SSO** –
“Small String Optimization”, when the string object contains a small buffer inside itself and the characters of short strings
are located directly in it.
But this already depends on the implementation: in some libraries they place up to 15 bytes in the string object, in some up to 23.
However, this optimization is also a double-edged sword, and in various implementations it can complicate the movement of a string - if it stores
a pointer to its internal buffer, it will have to be adjusted.

And without COW, the mutability of strings leads to the fact that any initialization of a string object leads to copying bytes.
Let's look at this code:
```cpp
    const char* text1 = "Hello, World";         // costs nothing
    std::string_view text2 = "Hello, World";    // Costs nothing, calculates the length of the string at compile time
    std::string text3 = "Hello, World";         // Copies the characters of the string every time at runtime
```

(You can verify the truth of the comments at https://godbolt.org/z/51oKGWT5T )

But if we don’t need to modify the string in any way further in the code, we are wasting money on allocation, copying characters,
as well as on the string destructor. That is, I would like to have at least two versions of strings – mutable and immutable,
to explicitly make it clear to the compiler that we are not going to modify the string.
Or a banal example – we are parsing some incoming data buffer, we need to check whether a certain piece of the buffer is equal to the string
"hello" in "pure C++", i.e. without any memcmp and strcmp. Before the advent of string_view, it had to be done something like this:
```cpp
    bool is_part_buffer_equal_hello(const char* data, int start, int end) {
        return std::string(data + start, end - start) == "hello";
    }
```
Here it turns out that first the characters from the data buffer are copied to the buffer of the temporary string, possibly with memory allocation, and only
then the temporary string is compared with "hello", and then also the destructor and stack unwinding in case of an exception.

When using `std::string_view` instead of `std::string` – the code in C++ almost does not change:
```cpp
    bool is_part_buffer_equal_hello_view(const char* data, int start, int end) {
        return std::string_view(data + start, end - start) == "hello";
    }
```
However, the generated machine code is significantly transformed, reaching the level of manual C-code – there it is simply compared
that end – start == 5 and then a piece of the initial buffer is compared via memcmp with the string "hello"
(with -O2 with constants 1819043176 ('hell') and 111 ('o')).
No creation of a temporary object, no copying of bytes, no destructor, no stack unwinding for exceptions.
You can verify this at https://godbolt.org/z/9fo188e7c

It would seem, well, `string_view` appeared in C++17, please, use it in the parameters of your functions instead of `const std::string&`,
and there will be happiness. But there is also a nuance here – everything works fine, as long as we don’t need to pass the string to a third-party C-API: string_view does not give
guarantees of null-termination of the string, therefore its data() cannot be passed to a third-party C-API, and therefore you will still have to
copy it to `std::string` first. And since `std::string` is needed, then it is more optimal to make `const std::string&` the parameter of the function
and further down the chain, all parameters will again become `const std::string&`.

### String concatenation
Next, after initializing a string, the most frequent mutable operation with them is most likely string concatenation, either in the form of simply
adding strings, or adding a string to a string. And it is she who can easily cause both suboptimal performance with illiterate
use, and memory overhead, even with competent use.

Consider a simple code ( https://godbolt.org/z/odx7W1Pv7 )
```cpp
    #include <string>
    void some_outer_function(const std::string&);

    void func(const std::string& s1, const std::string& s2) {
        std::string concat = s1 + s2 + "hello";
        some_outer_function(concat);
    }
```
As we can see, both in clang and in GCC, several temporary objects are created, into which the characters of the strings are sequentially shifted,
and as a result – we get several extra allocations for intermediate buffers, the characters from the strings are copied several extra
times from intermediate buffers. Ideally, for better performance, this code needs to be rewritten like this:
```cpp
    #include <string>
    void some_outer_function(const std::string&);

    void func(const std::string& s1, const std::string& s2) {
        static const std::string_view hello = "hello";
        std::string concat;
        concat.reserve(s1.size() + s2.size() + hello.size());
        concat += s1;
        concat += s2;
        concat += hello;
        some_outer_function(concat);
    }
```

Unfortunately, so far no compiler optimizes the first simple code to the level of the second more optimal code, and writing
such code every time by hand is quite inconvenient. That is, again you have to pay for what you don’t use.
And in this case, memory overhead may well occur – string addition operations usually increase
the size of the string buffer in all implementations by at least two times, assuming that something may soon be added to the string again.
Therefore, if the string is no longer planned to be modified, but its lifetime has not yet come to an end (for example, this is a field
of some class), you should not forget to do `shrink_to_fit` on it.

Meanwhile, often the main scenario for using strings is just some preparation of the string by several modifications and concatenations,
and then it is stored somewhere, no longer changing. In this case, the programmer usually knows approximately what size of strings is expected in this place,
and could allocate a buffer for these intermediate modifications directly on the stack, resorting to dynamic allocation only when exceeding
the size of this buffer. However, with the current implementation of strings, this is quite problematic, or inconvenient.

Let's summarize what we have at the moment:

- "Out of the box" in C++ for working with strings there is now `std::string`.
- Strings are assumed to be mutable, which leads to mandatory copying of all characters of the string during initialization and
   copying of string objects.
- Accordingly, we do not have the ability to quickly copy strings, even if we do not plan to change the copy later.
- Concatenating several strings is a task that can be performed suboptimally, lead to memory overhead, and it is difficult to write optimal code.
- There is a crutch for immutable strings in the form of `std::string_view`, but it does not solve the issues of string ownership, so in fact
  it is only suitable as a type for passing parameters to functions that do not change strings, with the caveat that it cannot be used in functions
  calling C-API, since it does not guarantee null-termination.
- Well, and there are questions to `std::string` that despite the fact that this is a class for strings, in fact, for working with strings it has an extremely meager
  functionality compared to what they are used to in other languages – for example, there is no replacement of substrings by a pattern (in other languages this is usually
  replace, but in C++ this function does something completely different), trim, split, join, upper, lower, etc.
  These functions have to be written by yourself every time, and it is not a fact that everyone will be able to do this optimally.

I hope that after this small introduction you will better understand what problems I solved with my string library and how.

## Simstr library
Actually, you can't say that "I reinvented my implementation of the class for strings."
As I showed earlier, it is difficult, or even impossible, to write one single string class that is well suited for all scenarios
of use. That is why I don’t have a string class, but a string library, which contains several different string types,
from simpler to more complex, each of which has its own strengths and weaknesses, and the user needs to competently approach the
question of which of these classes should be used in which case.

I started developing the library itself little by little back in 2011-2012, when we already had move semantics, but not yet
there was std::string_view. However, now the minimum standard version for the library to work is: **C++20** – concepts and \<format\> are used.

First, I will talk about the library classes for the strings themselves, and then about how the string concatenation problem is optimally solved in it.

Several general points:
- All classes for working with strings are templated by the type of characters, but it is assumed that the characters can be char, char16_t,
  char32_t, wchar_t.
- All strings have an explicit length.
- The string owner classes store them with a trailing zero at the end, which is not included in the length of the string.
- The string itself can contain zero characters, all algorithms work only through the length of the string, without paying attention to them.
- The string owner classes can be initialized with strings of another character type, performing conversion between UTF-8, UTF-16, UTF-32.
- Built-in tables for the first plane of Unicode are used to change the case of characters and compare strings case-insensitively
  (up to 0xFFFF). Strings are considered to be represented in UTF-8, UTF-16, UTF-32 encoding, respectively.
  However, string normalization is not done and situations where changing the case of a character leads to a change in their number are not handled.
  That is, the case conversion of characters corresponds to `std::towupper`, `std::towlower` for the unicode locale,
  only faster and can work with any type of characters.
  If you need strict work with unicode, use other tools, such as ICU.

### String classes.

#### The first simplest string class is called, of course, `simple_str` :)
(simstr::simple_str)

The class simply represents a pointer to the beginning of a constant string and its length, in fact the same as `std::string_view`.
It is intended for working with immutable strings, not owning them, that is, you must take care that the real string,
represented through `simple_str` – is alive during its use.

Implements all string methods that do not modify the string.

Aliases:
- `ssa` for simple_str\<char\>
- `ssu` for simple_str\<char16_t\>
- `ssw` for simple_str\<wchar_t>
- `ssuu` for simple_str\<char32_t\>

It is used mainly for passing strings as a parameter to functions that do not modify the passed string, instead of `const std::string&`,
as well as for local variables when working with parts of strings.

#### The second class is `simple_str_nt`
(simstr::simple_str_nt)

In terms of structure and purpose, it coincides with `simple_str`, but guarantees null-termination of the string.
That is, if the function needs to pass the passed parameter further as a C-string to some API without changes, it should use the
`simple_str_nt` type for the parameter.
All classes of owning strings (simstr::sstring, simstr::lstring) can be converted to `simple_str_nt`, since they store strings with a trailing zero.
This allows you to write functions with a single parameter type that accepts any type of owning string objects as input.

Aliases:
- `stra` for simple_str_nt\<char>
- `stru` for simple_str_nt\<char16_t>
- `strw` for simple_str_nt\<wchar_t>
- `struu` for simple_str_nt\<char32_t>

Can be initialized with string literals:
```cpp
    stra text = "Text";
```
In this case, the length is calculated immediately at compile time. Similarly, `simple_str_nt` is created using `operator""_ss`:

```cpp
    stringa result = "Count: "_ss + count;
```

#### Sstring class (shared string).
(simstr::sstring)

A class that can store an immutable string.
That is, you can only assign a string to it entirely, you cannot modify the characters of the string.

Owns the string, manages the memory for the characters of the string.
Stores a trailing zero with the strings, and can be a source for `simple_str_nt`, for passing to C-API.
Like `simple_str`, it implements all methods that do not modify the string.

Aliases:
- `stringa` for sstring\<char>
- `stringu` for sstring\<char16_t>
- `stringw` for sstring\<wchar_t>
- `stringuu` for sstring\<char32_t>


The fact that the stored string is immutable allows you to apply a number of optimizations:
- For strings that are not suitable for SSO, it uses a common shared buffer with an atomic reference counter.
  Allows you to quickly copy a string without the need to block access to the contents of the buffer.
- There is no need to store the buffer size (capacity) – we are not adding anything to the buffer anyway.
- Allows you to simply refer to program literals without copying their characters to any buffer:
    ```cpp
	stringa str = "Hello!";       // Costs nothing, does not copy the bytes of the string
	stringa ltr = stra{"Hello!"}; // But here it copies the bytes of the string to ltr
    ```

The class also uses **SSO** – Small String Optimization.
Short strings are placed inside the object itself in an internal buffer.

Sizes:

For 64 bits:
- `stringa` – class 24 bytes, SSO up to 23 characters.
- `stringu` – class 32 bytes, SSO up to 15 characters.
- `stringuu` – class 32 bytes, SSO up to 7 characters.

For 32 bits:
- `stringa` – class 16 bytes, SSO up to 15 characters.
- `stringu` – class 24 bytes, SSO up to 11 characters.
- `stringuu` – class 24 bytes, SSO up to 5 characters.

#### Class lstring<K, N, forShared> (local string)
(simstr::lstring)

A class that stores a string and allows it to be modified.
Owns the string, manages the memory for the characters of the string.
Stores a trailing zero with the strings, and can be a source for `simple_str_nt`, for passing to C-API.
Like all other classes, it implements all methods that do not modify the string.

The size of the internal buffer for storing characters is specified as `N` in the template parameter.
Strings up to N characters long are stored inside the object, and when this number is exceeded, a dynamic buffer is allocated,
in which the characters are saved. When copying an object, all characters are also always copied.

If `forShare` == true and the characters do not fit into the local buffer, then a dynamic buffer is created with additional space,
so that it matches the structure of the `sstring` buffer. Then, when moving `lstring` to `sstring` – only the pointer will move
to the buffer, without unnecessary copying of characters.

This class is convenient for working with strings as a local variable on the stack.
Usually we assume the approximate size of the strings we will be working with, and we can create a local string with a buffer on the stack,
and work with it. At the same time, without fear of buffer overflow, since in this case the string will switch to a dynamic buffer.

Aliases:
- `lstringa<N=16>` for lsrting\<char, N, false>
- `lstringu<N=16>` for lsrting\<char16_t, N, false>
- `lstringw<N=16>` for lsrting\<wchar_t, N, false>
- `lstringuu<N=16>` for lsrting\<char32_t, N, false>
- `lstringsa<N=16>` for lsrting\<char, N, true>
- `lstringsu<N=16>` for lsrting\<char16_t, N, true>
- `lstringsw<N=16>` for lsrting\<wchar_t, N, true>
- `lstringsuu<N=16>` for lsrting\<char32_t, N, true>


A small example of use with explanations:
```cpp
    #ifdef _WIN32
    const char path_separator = '\\';
    #else
    const size_t MAX_PATH = 260;
    const char path_separator = '/';
    #endif

    auto get_current_dir() {
    #ifdef _WIN32
        /* fills the buffer of the wchar_t string lstringw<MAX_PATH> from GetCurrentDirectoryW with possible
        increasing the buffer and converting to ut8 char. The constructor uses what appeared
        only in C++23 as `resize_and_overwrite`, and we had it originally :) */

        lstringa<MAX_PATH> path{lstringw<MAX_PATH>{ [](auto p, auto s) { return GetCurrentDirectoryW(DWORD(s + 1), p); }}};

        /* This one line does approximately the same thing as this code.
        typedef struct lstringa_MAX_PATH_t {
            char* data;
            size_t length;
            size_t capacity;
            char local_buffer[MAX_PATH + 1];
        } lstringa_MAX_PATH;

        lstringa_MAX_PATH* get_current_dir(lstringa_MAX_PATH* result) {
            wchar_t buffer[MAX_PATH + 1], *buf = buffer;
            DWORD size = sizeof(buffer) / sizeof(wchar_t), lengthOfpath;
            for (;;) {
                // Returns either the number of copied characters without taking into account the trailing zero,
                // or if the buffer is small, then the required buffer size along with the trailing zero
                DWORD ret = GetCurrentDirectoryW(size, buf);
                if (ret < size) {
                    // Fits into the buffer, although in Windows paths can be longer than MAX_PATH if they start with \\?\
                    // https://learn.microsoft.com/ru-ru/windows/win32/fileio/maximum-file-path-limitation?tabs=registry
                    lenOfpath = ret;
                    break;
                }
                size = ret;
                if (buf != buffer)
                    free(buf);
                buf = malloc(size);
            }
            utf16toUtf8(buf, lengthOfPath, result);
            if (buf != buffer)
                free(buf);
            return result;
        }
        */
    #else
        lstringa<MAX_PATH> path{ [](char* p, size_t s) {
            const char* res = getcwd(p, s + 1);
            if (res) {
                return stra{res}.length(); // Returns the length of the string
            }
            if (errno == ERANGE)  // Did not fit into the buffer, let's try twice as much
                return s * 2;
            return 0ul;
        }};
    #endif
        // Let's make sure that the string will end with a directory separator
        if (!path.length() || path.at(-1) != path_separator) {
            path += e_c(1, path_separator);
        }
        return path;
    }

    stringa build_full_path(ssa fileName) {
        return get_current_dir() + fileName + ".txt";
        /*
        Here, a temporary lstringa<MAX_PATH> object will first be created on the stack to call get_current_dir.
        The get_current_dir function will fill it with the name of the current directory.
        In 99.9% of cases, the local buffer on the stack will be enough for this.
        After that, the total length for the result is calculated: the length of current_dir + the length of fileName + 4.
        The buffer for the string of the final result is determined - if the length is less than 24 - the string will be placed directly in stringa,
        otherwise a buffer for the resulting string is allocated immediately of the required size.
        Then the characters from current_dir, file_name, ".txt" are sequentially copied to the buffer of the resulting string;
        Well, thanks to RVO - the place for the result itself (stringa) - is allocated in the calling function,
        that is, there will be no additional copying upon return.

        Thus, there will be a maximum of only two memory allocations (if current_dir does not fit into MAX_PATH),
        or one, if the resulting string is longer than 23 characters, while this allocation will be immediately of the required size.
        */
    }
```

In this example, you probably noticed how strings are concatenated and wondered – how was the
length of the entire result calculated with two additions in order to allocate the necessary space at once, without intermediate buffers?

The answer to this question:

### String Expressions
The fact is that there is no addition of string objects as such in the library. Addition is performed for "string expressions".

A *string expression* is any object of arbitrary type that has `length` and `place` functions.
The `length` function returns the length of the string, and the `place` function places the characters of the string into the buffer passed to it.

Any owning string (simstr::sstring, simstr::lstring) can be initialized with a string expression — it requests its length,
allocates space for storing characters, and passes this space to the string expression, calling its place function.

A template addition function is defined for string expressions:
```cpp
    template<StrExpr A, StrExprForType<typename A::symb_type> B>
    inline auto operator + (const A& a, const B& b) {
        return strexprjoin<A, B>{a, b};
    }
```

`strexprjoin` is a template type that is itself a string expression.
It stores references to the two string expressions passed to it.
When the length is requested, it returns the sum of the lengths of the two string expressions, and when placing characters, it first places
the first expression in the passed buffer, then the second.
```cpp
    template<StrExpr A, StrExprForType<typename A::symb_type> B>
    struct strexprjoin {
        using symb_type = typename A::symb_type;
        const A& a;
        const B& b;
        constexpr strexprjoin(const A& a_, const B& b_) : a(a_), b(b_){}
        constexpr size_t length() const noexcept { return a.length() + b.length(); }
        constexpr symb_type* place(symb_type* p) const noexcept { return b.place(a.place(p)); }
    };
```
Thus, the addition operation of string expressions creates an object that is also a string expression,
to which the next addition operation can also be applied, and which recursively stores references to the component parts,
each of which knows its size and knows how to place itself in the result buffer. And so on, to each resulting
string expression, you can reapply `operator +`, forming a chain of several string expressions,
and eventually "materialize" the last resulting object, which first calculates the size of the entire total memory for
the final result, and then places the nested subexpressions into one buffer.

All string types in the library are themselves string expressions, that is, they can serve as terms in concatenations
of string expressions.

Also, `operator+` is defined for string expressions and string literals, string expressions and numbers (numbers are converted
to decimal representation), and you can add the desired types yourself.

Example:
```cpp
	stringa text = header + " count=" + count + ", done";
```

There are several types of string expressions "out of the box" for performing various operations on strings:

#### expr_spaces<CharacterType, NumberOfCharacters, Symbol = ' '>{}
Returns a string of length NumberOfCharacters, filled with the specified character. The number of characters and the symbol are compile-time constants. For some cases, there is a shorthand notation:

    e_spca(NumberOfCharacters) - string of char spaces
    e_spcw(NumberOfCharacters) - string of w_char spaces

#### expr_pad<CharacterType>{NumberOfCharacters, Symbol = ' '}
Returns a string of length NumberOfCharacters, filled with the specified character.
The number of characters and the symbol can be specified at runtime. Shorthand notation:

    e_c(NumberOfCharacters, Symbol)

#### e_repeat{Str, count}
Generates a repetition of a string literal or expression `Str` `count` times.

    e_repeat("aa", 10);

#### e_choice(bool Condition, StrExpr1, StrExpr2)
If Condition == true, the result will be StrExpr1, otherwise StrExpr2.

#### e_if(bool Condition, StrExpr1)
If Condition == true, the result will be StrExpr1, otherwise an empty string.

#### expr_num<CharacterType>(Integer)
Converts a number to decimal representation. Rarely used, since the "+" operator is overloaded for string expressions and numbers, and the number can simply be written as `text + number`;

#### expr_real<CharacterType>(RealNumber)
converts a number to decimal representation. Rarely used, since the "+" operator is overloaded for string expressions and numbers, and the number can simply be written as `text + number`;

#### e_join<bool AfterLast = false, bool OnlyNotEmpty = false>(container, "Separator")
Concatenates all strings in the container, using a separator. If AfterLast == true,
then the separator is added after the last element of the container as well, otherwise only between elements.
If OnlyNotEmpty == true, then empty strings are skipped without adding a separator.

#### e_repl(OriginalString, "Search", "Replace")
Replaces occurrences of "Search" in the original string with "Replace".
Search and replace patterns are compile-time string literals.

#### expr_replaced<CharacterType>{OriginalString, Search, Replace}
Replaces occurrences of Search in the original string with Replace.
Search and replace patterns can be any string objects at runtime.

#### empty_expr<CharacterType>
Returns an empty string. Abbreviated notation — eea, eeu, eew, eeuu. Used if the string formation starts with a number and a string literal:
```cpp
	str = eea + count + " times.";
```
since the addition operator is only defined for adding a string expression and a number.
I also note that there is `operator""_ss`, which turns a string literal into a `simple_str_nt` object, which is already a string expression:
```cpp
	str = "Count = "_ss + count;
    ...
    str = count + " times."_ss;
```

#### Your own string expressions
You can create your own string expression types to optimally form strings for your specific purposes and algorithms.
To do this, simply create a type with `length`, `place` and `typename symb_type` methods.
Examples of creation and use from real projects:

```cpp
/* Form a string in JSON format, in 16-bit characters */
struct expr_json_str {
    using symb_type = u16s;
    ssu text;
    size_t l;
    size_t length() const noexcept {
        return l;
    }
    u16s* place(u16s* ptr) const noexcept;
    expr_json_str(ssu t);
};

inline expr_json_str::expr_json_str(ssu t) : text(t) {
    const u16s* ptr = text.symbols();
    size_t add = 0;

    for (size_t i = 0; i < text.length(); i++) {
        switch (*ptr++) {
        case '\b':
        case '\f':
        case '\r':
        case '\n':
        case '\t':
        case '\"':
        case '\\':
            add++;
        }
    }
    l = text.len + add;
}

inline u16s* expr_json_str::place(u16s* ptr) const noexcept {
    const u16s *r = text.symbols();
    size_t lenOfText = text.length(), lenOfTail = l;
    while (lenOfTail > lenOfText) {
        u16s s = *r++;
        switch (s) {
        case '\b':
            *ptr++ = '\\';
            *ptr++ = 'b';
            lenOfTail--;
            break;
        case '\f':
            *ptr++ = '\\';
            *ptr++ = 'f';
            lenOfTail--;
            break;
        case '\r':
            *ptr++ = '\\';
            *ptr++ = 'r';
            lenOfTail--;
            break;
        case '\n':
            *ptr++ = '\\';
            *ptr++ = 'n';
            lenOfTail--;
            break;
        case '\t':
            *ptr++ = '\\';
            *ptr++ = 't';
            lenOfTail--;
            break;
        case '\"':
            *ptr++ = '\\';
            *ptr++ = '\"';
            lenOfTail--;
            break;
        case '\\':
            *ptr++ = '\\';
            *ptr++ = '\\';
            lenOfTail--;
            break;
        default:
            *ptr++ = s;
            break;
        }
        lenOfTail--;
        lenOfText--;
    }
    if (lenOfTail) {
        std::char_traits<u16s>::copy(ptr, r, lenOfTail);
        ptr += lenOfTail;
    }
    return ptr;
}
```

Usage:

```cpp
........
chunked_string_builder<u16s> vtText;
........
vtText << uR"({"#type":"jxs:string","#value":")" + expr_json_str(name) + u"\"}";
.......
```

Another example
```cpp
/* Need to form binary data in BASE64 format, in 16-bit characters */
struct expr_str_base64 {
    using symb_type = u16s;
    ssa text;
    size_t length() const noexcept {
        return (text.len + 2) / 3 * 4;
    }
    u16s* place(u16s* ptr) const noexcept;
    expr_str_base64(ssa t) : text(t) {}
};

inline u16s* expr_str_base64::place(u16s* ptr) const noexcept {
    static constexpr u8s alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const unsigned char* t = (const unsigned char*)text.str;

    size_t i = 0;
    if (text.len > 2) {
        for (; i < text.len - 2; i += 3) {
            *ptr++ = alphabet[(t[i] >> 2) & 0x3F];
            *ptr++ = alphabet[((t[i] & 0x3) << 4) | ((int)(t[i + 1] & 0xF0) >> 4)];
            *ptr++ = alphabet[((t[i + 1] & 0xF) << 2) | ((int)(t[i + 2] & 0xC0) >> 6)];
            *ptr++ = alphabet[t[i + 2] & 0x3F];
        }
    }

    if (i < text.len) {
        *ptr++ = alphabet[(t[i] >> 2) & 0x3F];
        if (i == (text.len - 1)) {
            *ptr++ = alphabet[((t[i] & 0x3) << 4)];
            *ptr++ = '=';
        } else {
            *ptr++ = alphabet[((t[i] & 0x3) << 4) | ((int)(t[i + 1] & 0xF0) >> 4)];
            *ptr++ = alphabet[((t[i + 1] & 0xF) << 2)];
        }
        *ptr++ = '=';
    }
    return ptr;
}
```

Usage:
```cpp
......
chunked_string_builder<u16s> vtText;
......
vtText << u"{\"#\",87126200-3e98-44e0-b931-ccb1d7edc497,{1,{#base64:" + expr_str_base64(v) + u"}}},";
......
```

And more

```cpp
/* Need to convert tm to a date/time string in 16-bit characters */
struct expr_str_tm {
    using symb_type = u16s;
    const tm& t;
    size_t length() const noexcept {
        return 19;
    }
    u16s* place(u16s* ptr) const noexcept;
    expr_str_tm(const tm& _t) : t(_t) {}
};

inline u16s* expr_str_tm::place(u16s* ptr) const noexcept {
    if constexpr (sizeof(wchar_t) == 2) {
        // Under Windows, you can immediately format the string into the desired buffer
        std::swprintf((wchar_t*)ptr, 20, L"%04i-%02i-%02i %02i:%02i:%02i", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);
    } else {
        // First, format into an intermediate buffer, then copy to the result
        char buf[20];
        std::snprintf(buf, 20, "%04i-%02i-%02i %02i:%02i:%02i", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
            t.tm_min, t.tm_sec);
        for (unsigned i = 0; i < 19; i++) {
            ptr[i] = buf[i];
        }
    }
    return ptr + 19;
}
```

Usage

```cpp
......
bool makeBind(SqliteQuery& query, tVariant& param, unsigned paramNum) {
    switch (param.vt) {
......
    case VTYPE_DATE:
        query.bind(paramNum, lstringu<30>{expr_str_tm{winDateToTm(param.date)}}.to_str());
        break;
    case VTYPE_TM:
        query.bind(paramNum, lstringu<30>{expr_str_tm{param.tmVal}}.to_str());
        break;
......
```

ATTENTION: usually the fields in string expression objects are references to the source data.
And these references almost always lead to local or temporary objects. Therefore, it is extremely risky to return string expressions
from functions — you need to check a hundred times that they do not contain references to local or temporary variables.
Make it a rule — you can easily pass string expressions to functions, and it is dangerous to return them from functions.
It is better to materialize a string expression into a string object containing the final string when returning.
If desired, the type of the returned string can be specified by a template parameter.


### Class chunked_string_builder

Designed for concatenating multiple strings.
When you need to sequentially form a long text from many small pieces (for example, you are forming an html response
etc.) - sequentially adding everything to one string object is extremely suboptimal - there will be many reallocations and
copying of already accumulated characters. In this case, it is convenient to use chunked_string_builder - all it can do is
add a string to the accumulated characters. However, it does this not in a single sequential memory buffer, but in separate
buffers, no less than the specified alignment. When filling the next buffer, it simply creates another buffer and continues
to add data to it.

That is, suppose you set the alignment to 1024.
Added several strings, filled the buffer with 100 characters. And you add a string of 3000 characters long.
In this case, 924 characters will be copied to the first buffer, filling it to the end.
For the remaining 2076, a buffer of 3072 characters will be created, and they will be copied into it, leaving space for 996 characters in it.
Thus, each buffer is sequentially filled to the end and has a size that is a multiple of the specified alignment.
This avoids reallocations and copying of processed characters.

After the final filling, you can work with the accumulated data - either merge all the buffers into one sequential
string (the size for the buffer of which you now already know), or iterate over them separately, for example, sending these buffers
to the network. Or sequentially copying data into a buffer of a given size.
