/*
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * ver. 1.0
 * Классы для работы со строками
 */
#pragma once

#include <span>
#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif

#ifdef SIMSTR_SHARED
    #if defined(_MSC_VER) || (defined(__clang__) && __has_declspec_attribute(dllexport))
        #ifdef SIMSTR_EXPORT
            #define SIMSTR_API __declspec(dllexport)
        #else
            #define SIMSTR_API __declspec(dllimport)
        #endif
    #elif (defined(__GNUC__) || defined(__GNUG__)) && defined(SIMSTR_EXPORT)
        #define SIMSTR_API __attribute__((visibility("default")))
    #else
        #define SIMSTR_API
    #endif
#else
    #define SIMSTR_API
#endif
const bool isWindowsOs = // NOLINT
#ifdef _WIN32
    true
#else
    false
#endif
    ;
const bool isx64 = sizeof(void*) == 8; // NOLINT

#ifdef _MSC_VER
#define _no_unique_address msvc::no_unique_address
#else
#define _no_unique_address no_unique_address
#endif

#if defined __has_builtin
#  if __has_builtin (__builtin_mul_overflow) && __has_builtin (__builtin_add_overflow)
#    define HAS_BUILTIN_OVERFLOW
#  endif
#endif

#include "strexpr.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <vector>
#include <format>
#include <unordered_map>
#include <tuple>
#include <limits>
#include <cstdint>
#include <atomic>
#include <memory>
#include <string.h>
#include <iostream>

#ifdef _WIN32
#include <stdio.h>
#endif

#ifdef _MSC_VER
// warning C4201 : nonstandard extension used : nameless struct / union
#pragma warning(disable : 4201)
#endif

namespace simstr {

template<typename T>
struct unicode_traits {};   // NOLINT

template<>
struct unicode_traits<u8s> {
    // Эти операции с utf-8 могут изменить длину строки
    // Поэтому их специализации отличаются
    // В функцию помимо текста и адреса буфера для записи передается размер буфера
    // Возвращает длину получающейся строки.
    // Если получающеюся строка не влезает в отведенный буфер, указатели устанавливаются на последние
    // обработанные символы, для повторного возобновления работы,
    // а для оставшихся символов считается нужный размер буфера.
    static SIMSTR_API size_t upper(const u8s*& src, size_t lenStr, u8s*& dest, size_t lenBuf);
    static SIMSTR_API size_t lower(const u8s*& src, size_t len, u8s*& dest, size_t lenBuf);

    static SIMSTR_API int compareiu(const u8s* text1, size_t len1, const u8s* text2, size_t len2);

    static SIMSTR_API size_t hashia(const u8s* src, size_t l);
    static SIMSTR_API size_t hashiu(const u8s* src, size_t l);
};

template<>
struct unicode_traits<u16s> {
    static SIMSTR_API void upper(const u16s* src, size_t len, u16s* dest);
    static SIMSTR_API void lower(const u16s* src, size_t len, u16s* dest);

    static SIMSTR_API int compareiu(const u16s* text1, size_t len1, const u16s* text2, size_t len2);
    static SIMSTR_API size_t hashia(const u16s* src, size_t l);
    static SIMSTR_API size_t hashiu(const u16s* src, size_t l);
};

template<>
struct unicode_traits<u32s> {
    static SIMSTR_API void upper(const u32s* src, size_t len, u32s* dest);
    static SIMSTR_API void lower(const u32s* src, size_t len, u32s* dest);

    static SIMSTR_API int compareiu(const u32s* text1, size_t len1, const u32s* text2, size_t len2);
    static SIMSTR_API size_t hashia(const u32s* src, size_t s);
    static SIMSTR_API size_t hashiu(const u32s* src, size_t s);
};

template<>
struct unicode_traits<wchar_t> {
    static void upper(const wchar_t* src, size_t len, wchar_t* dest) {
        unicode_traits<wchar_type>::upper(to_w(src), len, to_w(dest));
    }
    static void lower(const wchar_t* src, size_t len, wchar_t* dest) {
        unicode_traits<wchar_type>::lower(to_w(src), len, to_w(dest));
    }

    static int compareiu(const wchar_t* text1, size_t len1, const wchar_t* text2, size_t len2) {
        return unicode_traits<wchar_type>::compareiu(to_w(text1), len1, to_w(text2), len2);
    }
    static size_t hashia(const wchar_t* src, size_t s) {
        return unicode_traits<wchar_type>::hashia(to_w(src), s);
    }
    static size_t hashiu(const wchar_t* src, size_t s) {
        return unicode_traits<wchar_type>::hashiu(to_w(src), s);
    }
};

namespace str {
constexpr const size_t npos = static_cast<size_t>(-1); //NOLINT
} // namespace str

template<typename K>
struct ch_traits : std::char_traits<K>{};

template<size_t N>
concept is_const_pattern = N > 1 && N <= 17;

template<typename K, size_t I>
struct _ascii_mask { // NOLINT
    constexpr static const size_t value = size_t(K(~0x7F)) << ((I - 1) * sizeof(K) * 8) | _ascii_mask<K, I - 1>::value;
};

template<typename K>
struct _ascii_mask<K, 0> {
    constexpr static const size_t value = 0;
};

template<typename K>
struct ascii_mask { // NOLINT
    using uns = std::make_unsigned_t<K>;
    constexpr static const size_t WIDTH = sizeof(size_t) / sizeof(uns);
    constexpr static const size_t VALUE = _ascii_mask<uns, WIDTH>::value;
};

template<typename K>
constexpr inline bool isAsciiUpper(K k) {
    return k >= 'A' && k <= 'Z';
}

template<typename K>
constexpr inline bool isAsciiLower(K k) {
    return k >= 'a' && k <= 'z';
}

template<typename K>
constexpr inline K makeAsciiLower(K k) {
    return isAsciiUpper(k) ? k | 0x20 : k;
}

template<typename K>
constexpr inline K makeAsciiUpper(K k) {
    return isAsciiLower(k) ? k & ~0x20 : k;
}

enum TrimSides { TrimLeft = 1, TrimRight = 2, TrimAll = 3 };
template<TrimSides S, typename K, size_t N, bool withSpaces = false>
struct trim_operator;

template<typename K, size_t N, size_t L>
struct expr_replaces;

template<typename T>
concept FromIntNumber =
    is_one_of_type<std::remove_cv_t<T>, unsigned char, int, short, long, long long, unsigned, unsigned short, unsigned long, unsigned long long>::value;

template<typename T>
concept ToIntNumber = FromIntNumber<T> || is_one_of_type<T, int8_t, uint8_t>::value;

#if defined(_MSC_VER) && _MSC_VER <= 1933
template<typename K, typename... Args>
using FmtString = std::_Basic_format_string<K, std::type_identity_t<Args>...>;
#elif __clang_major__ >= 15 || _MSC_VER > 1933 || __GNUC__ >= 13
template<typename K, typename... Args>
using FmtString = std::basic_format_string<K, std::type_identity_t<Args>...>;
#else
template<typename K, typename... Args>
using FmtString = std::basic_string_view<K>;
#endif

template<typename K, bool I, typename T>
struct need_sign { // NOLINT
    bool sign;
    need_sign(T& t) : sign(t < 0) {
        if (sign && t != std::numeric_limits<T>::min())
            t = -t;
    }
    void after(K*& ptr) {
        if (sign)
            *--ptr = '-';
    }
};

template<typename K, typename T>
struct need_sign<K, false, T> {
    need_sign(T&) {}
    void after(K*&) {}
};

enum class IntConvertResult : char { Success, BadSymbolAtTail, Overflow, NotNumber };

template<bool CanNegate, bool CheckOverflow, typename T>
struct result_type_selector { // NOLINT
    using type = T;
};

template<typename T>
struct result_type_selector<true, false, T> {
    using type = std::make_unsigned_t<T>;
};

template<unsigned Base>
constexpr unsigned digit_width() {
    if (Base <=2) {
        return 1;
    }
    if (Base <= 4) {
        return 2;
    }
    if (Base <= 8) {
        return 3;
    }
    if (Base <= 16) {
        return 4;
    }
    if (Base <= 32) {
        return 5;
    }
    return 6;
}

template<typename T, unsigned Base>
constexpr unsigned max_overflow_digits = (sizeof(T) * 8) / digit_width<Base>();

struct int_convert { // NOLINT
private:
    inline static constexpr uint8_t NUMBERS[] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,   1,   2,   3,
        4,   5,   6,   7,   8,   9,   255, 255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
        23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255, 255, 10,  11,  12,  13,  14,  15,  16,
        17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

    template<typename K, unsigned Base>
    static uint8_t toDigit(K s) {
        auto us = static_cast<std::make_unsigned_t<K>>(s);
        if constexpr (Base <= 10) {
            return us - '0';
        } else {
            if constexpr (sizeof(K) == 1) {
                return NUMBERS[us];
            } else {
                return us < 256 ? NUMBERS[us] : 255;
            }
        }
    }

    template<typename K, ToIntNumber T, unsigned Base, bool CheckOverflow>
        requires(Base != 0)
    static std::tuple<T, IntConvertResult, size_t> parse(const K* start, const K* current, const K* end, bool negate) {
        using u_type = std::make_unsigned_t<T>;
        #ifndef HAS_BUILTIN_OVERFLOW
        u_type  maxMult, maxAdd;
        if constexpr (CheckOverflow) {
            maxMult = std::numeric_limits<u_type>::max() / Base;
            maxAdd = std::numeric_limits<u_type>::max() % Base;
        }
        #endif
        u_type number = 0;
        unsigned maxDigits = max_overflow_digits<u_type, Base>;
        IntConvertResult error = IntConvertResult::NotNumber;
        const K* from = current;

        bool no_need_check_o_f = !CheckOverflow || end - current <= maxDigits;

        if (no_need_check_o_f) {
            for (;;) {
                const unsigned char digit = toDigit<K, Base>(*current);
                if (digit >= Base) {
                    break;
                }
                number = number * Base + digit;
                if (++current == end) {
                    error = IntConvertResult::Success;
                    break;
                }
            }
        } else {
            for (;maxDigits; maxDigits--) {
                const unsigned char digit = toDigit<K, Base>(*current);
                if (digit >= Base) {
                    break;
                }
                number = number * Base + digit;
                ++current;
            }
            if (!maxDigits) {
                // Прошли все цифры, дальше надо с проверкой на overflow
                for (;;) {
                    const unsigned char digit = toDigit<K, Base>(*current);
                    if (digit >= Base) {
                        break;
                    }
                    #ifdef HAS_BUILTIN_OVERFLOW
                    if (__builtin_mul_overflow(number, Base, &number) ||
                        __builtin_add_overflow(number, digit, &number)) {
                    #else
                    if (number < maxMult || (number == maxMult && number < maxAdd)) {
                        number = number * Base + digit;
                    } else {
                    #endif
                        error = IntConvertResult::Overflow;
                        while(++current < end) {
                            if (toDigit<K, Base>(*current) >= Base) {
                                break;
                            }
                        }
                        break;
                    }
                    if (++current == end) {
                        error = IntConvertResult::Success;
                        break;
                    }
                }
            }
        }
        T result;
        if constexpr (std::is_signed_v<T>) {
            result = negate ? 0 - number : number;
            if constexpr (CheckOverflow) {
                if (error != IntConvertResult::Overflow) {
                    if (number > std::numeric_limits<T>::max() + (negate ? 1 : 0)) {
                        error = IntConvertResult::Overflow;
                    }
                }
            }
        } else {
            result = number;
        }
        if (error == IntConvertResult::NotNumber && current > from) {
            error = IntConvertResult::BadSymbolAtTail;
        }
        return {result, error, current - start};
    }
public:
    // Если Base = 0 - то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10
    // Если Base = -1 - то пытается определить основание по префиксу 0[xX] как 16, 0[bB] как 2, 0[oO] или 0 как 8, иначе 10
    template<typename K, ToIntNumber T, unsigned Base = 0, bool CheckOverflow = true, bool SkipWs = true, bool AllowSign = true>
        requires(Base == -1 || (Base < 37 && Base != 1))
    static std::tuple<T, IntConvertResult, size_t> to_integer(const K* start, size_t len) noexcept {
        const K *ptr = start, *end = ptr + len;
        bool negate = false;
        if constexpr (SkipWs) {
            while (ptr < end && std::make_unsigned_t<K>(*ptr) <= ' ')
                ptr++;
        }
        if constexpr (AllowSign) {
            if (ptr != end) {
                if (*ptr == '+') {
                    ptr++;
                } else {
                    if constexpr (std::is_signed_v<T>) {
                        if (*ptr == '-') {
                            negate = true;
                            ptr++;
                        }
                    }
                }
            }
        }
        if (ptr != end) {
            if constexpr (Base == 0 || Base == -1) {
                if (*ptr == '0') {
                    ptr++;
                    if (ptr != end) {
                        if (*ptr == 'x' || *ptr == 'X') {
                            return parse<K, T, 16, CheckOverflow>(start, ++ptr, end, negate);
                        }
                        if constexpr (Base == -1) {
                            if (*ptr == 'b' || *ptr == 'B') {
                                return parse<K, T, 2, CheckOverflow>(start, ++ptr, end, negate);
                            }
                            if (*ptr == 'o' || *ptr == 'O') {
                                return parse<K, T, 8, CheckOverflow>(start, ++ptr, end, negate);
                            }
                        }
                        return parse<K, T, 8, CheckOverflow>(start, --ptr, end, negate);
                    }
                    return {0, IntConvertResult::Success, ptr - start};
                }
                return parse<K, T, 10, CheckOverflow>(start, ptr, end, negate);
            } else
                return parse<K, T, Base, CheckOverflow>(start, ptr, end, negate);
        }
        return {0, IntConvertResult::NotNumber, ptr - start};
    }
};

template<typename K>
class Splitter;

/*
* Класс с базовыми строковыми алгоритмами
* Является базой для классов, могущих выполнять константные операции со строками.
* Ничего не знает о хранении строк, ни сам, ни у класса наследника, то есть работает
* только с указателем на строку и её длиной.
* Для работы класс-наследник должен реализовать методы:
*   size_t length() const noexcept     - возвращает длину строки
*   const K* symbols() const noexcept  - возвращает указатель на начало строки
*   bool is_empty() const noexcept    - проверка, не пустая ли строка
* K       - тип символов
* StrRef  - тип хранилища куска строки
* Impl    - конечный класс наследник
*/

template<typename K, typename StrRef, typename Impl>
class str_algs {
    const Impl& d() const noexcept {
        return *static_cast<const Impl*>(this);
    }
    size_t _len() const noexcept {
        return d().length();
    }
    const K* _str() const noexcept {
        return d().symbols();
    }
    bool _is_empty() const noexcept {
        return d().is_empty();
    }

public:
    using symb_type = K;
    using str_piece = StrRef;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = Impl;
    using base = str_algs<K, StrRef, Impl>;
    // Пустой конструктор
    str_algs() = default;

    constexpr K* place(K* ptr) const noexcept {
        size_t myLen = _len();
        if (myLen) {
            traits::copy(ptr, _str(), myLen);
            return ptr + myLen;
        }
        return ptr;
    }

    void copyTo(K* buffer, size_t bufSize) {
        size_t tlen = std::min(_len(), bufSize - 1);
        if (tlen)
            traits::copy(buffer, _str(), tlen);
        buffer[tlen] = 0;
    }

    const K* begin() const noexcept {
        return _str();
    }
    const K* end() const noexcept {
        return _str() + _len();
    }

    size_t size() const {
        return _len();
    }

    // Чтобы быть источником строкового объекта
    constexpr operator str_piece() const noexcept {
        return str_piece{_str(), _len()};
    }
    str_piece to_str() const noexcept {
        return {_str(), _len()};
    }
    std::string_view to_sv() const noexcept {
        return {_str(), _len()};
    }

    std::string to_string() const noexcept {
        return {_str(), _len()};
    }

    constexpr str_piece operator()(ptrdiff_t from, ptrdiff_t len = 0) const noexcept {
        size_t myLen = _len(), idxStart = from >= 0 ? from : myLen + from, idxEnd = (len > 0 ? idxStart : myLen) + len;
        if (idxEnd > myLen)
            idxEnd = myLen;
        if (idxStart > idxEnd)
            idxStart = idxEnd;
        return str_piece{_str() + idxStart, idxEnd - idxStart};
    }
    constexpr str_piece mid(size_t from, size_t len = -1) const noexcept {
        size_t myLen = _len(), idxStart = from, idxEnd = from > std::numeric_limits<size_t>::max() - len ? myLen : from + len;
        if (idxEnd > myLen)
            idxEnd = myLen;
        if (idxStart > idxEnd)
            idxStart = idxEnd;
        return str_piece{_str() + idxStart, idxEnd - idxStart};
    }
    constexpr str_piece from_to(size_t from, size_t to) const noexcept {
        return str_piece{_str() + from, to - from};
    }
    void store(char*& ptr) const noexcept {
        size_t len = (_len() + 1) * sizeof(K);
        memcpy(ptr, _str(), len);
        ptr += len;
    }

    bool operator!() const noexcept {
        return _is_empty();
    }
    // Доступ к символу, отрицательные - с конца строки
    K at(int idx) const {
        return _str()[idx >= 0 ? idx : _len() + idx];
    }
    // Сравнение строк
    constexpr int compare(const K* text, size_t len) const {
        size_t myLen = _len();
        int cmp = traits::compare(_str(), text, std::min(myLen, len));
        return cmp == 0 ? (myLen > len ? 1 : myLen == len ? 0 : -1) : cmp;
    }
    constexpr int compare(str_piece o) const {
        return compare(o.symbols(), o.length());
    }
    // Сравнение c C-строками
    constexpr int strcmp(const K* text) const {
        size_t myLen = _len(), idx = 0;
        const K* ptr = _str();
        for (; idx < myLen; idx++) {
            uns_type s1 = (uns_type)text[idx];
            if (!s1) {
                return 1;
            }
            uns_type s2 = (uns_type)ptr[idx];
            if (s1 < s2) {
                return 1;
            } else if (s1 > s2) {
                return -1;
            }
        }
        return text[idx] == 0 ? 0 : -1;
    }

    constexpr bool equal(const K* text, size_t len) const noexcept {
        return len == _len() && traits::compare(_str(), text, len) == 0;
    }

    constexpr bool equal(str_piece other) const noexcept {
        return equal(other.symbols(), other.length());
    }
    
    constexpr bool operator==(const base& other) const noexcept {
        return equal(other._str(), other._len());
    }

    constexpr auto operator<=>(const base& other) const noexcept {
        return compare(other._str(), other._len()) <=> 0;
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
    bool operator==(T&& other) const noexcept {
        return N - 1 == _len() && traits::compare(_str(), other, N - 1) == 0;
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
    auto operator<=>(T&& other) const noexcept {
        size_t myLen = _len();
        int cmp = traits::compare(_str(), other, std::min(myLen, N - 1));
        int res = cmp == 0 ? (myLen > N - 1 ? 1 : myLen == N - 1 ? 0 : -1) : cmp;
        return res <=> 0;
    }

    // Сравнение ascii строк без учёта регистра
    int compare_ia(const K* text, size_t len) const noexcept { // NOLINT
        if (!len)
            return _is_empty() ? 0 : 1;
        size_t myLen = _len(), checkLen = std::min(myLen, len);
        const uns_type *ptr1 = reinterpret_cast<const uns_type*>(_str()), *ptr2 = reinterpret_cast<const uns_type*>(text);
        while (checkLen--) {
            uns_type s1 = *ptr1++, s2 = *ptr2++;
            if (s1 == s2)
                continue;
            s1 = makeAsciiLower(s1);
            s2 = makeAsciiLower(s2);
            if (s1 > s2)
                return 1;
            else if (s1 < s2)
                return -1;
        }
        return myLen == len ? 0 : myLen > len ? 1 : -1;
    }

    int compare_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length());
    }

    bool equal_ia(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_ia(text.symbols(), text.length()) == 0;
    }

    bool less_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length()) < 0;
    }

    int compare_iu(const K* text, size_t len) const noexcept { // NOLINT
        if (!len)
            return _is_empty() ? 0 : 1;
        return uni::compareiu(_str(), _len(), text, len);
    }
    int compare_iu(str_piece text) const noexcept { // NOLINT
        return compare_iu(text.symbols(), text.length());
    }

    bool equal_iu(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_iu(text.symbols(), text.length()) == 0;
    }

    bool less_iu(str_piece text) const noexcept { // NOLINT
        return compare_iu(text.symbols(), text.length()) < 0;
    }

    size_t find(const K* pattern, size_t lenPattern, size_t offset) const noexcept {
        size_t lenText = _len();
        // Образец, не вмещающийся в строку и пустой образец не находим
        if (!lenPattern || offset >= lenText || offset + lenPattern > lenText)
            return str::npos;
        lenPattern--;
        const K *text = _str(), *last = text + lenText - lenPattern, first = pattern[0];
        pattern++;
        for (const K* fnd = text + offset;; ++fnd) {
            fnd = traits::find(fnd, last - fnd, first);
            if (!fnd)
                return str::npos;
            if (traits::compare(fnd + 1, pattern, lenPattern) == 0)
                return static_cast<size_t>(fnd - text);
        }
    }
    size_t find(str_piece pattern, size_t offset = 0) const noexcept {
        return find(pattern.symbols(), pattern.length(), offset);
    }
    size_t find_end(str_piece pattern, size_t offset = 0) const noexcept {
        size_t fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? fnd : fnd + pattern.length();
    }

    size_t find_or_all(str_piece pattern, size_t offset = 0) const noexcept {
        auto fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd;
    }

    size_t find(K s, size_t offset = 0) const noexcept {
        size_t len = _len();
        if (offset < len) {
            const K *str = _str(), *fnd = traits::find(str + offset, len - offset, s);
            if (fnd)
                return static_cast<size_t>(fnd - str);
        }
        return str::npos;
    }

    size_t find_or_all(K s, size_t offset = 0) const noexcept {
        size_t len = _len();
        if (offset < len) {
            const K *str = _str(), *fnd = traits::find(str + offset, len - offset, s);
            if (fnd)
                return static_cast<size_t>(fnd - str);
        }
        return len;
    }

    template<typename Op>
    void for_all_finded(const Op& op, const K* pattern, size_t patternLen, size_t offset, size_t maxCount) const {
        if (!maxCount)
            maxCount--;
        while (maxCount-- > 0) {
            size_t fnd = find(pattern, patternLen, offset);
            if (fnd == str::npos)
                break;
            op(fnd);
            offset = fnd + patternLen;
        }
    }
    template<typename Op>
    void for_all_finded(const Op& op, str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        for_all_finded(op, pattern.symbols(), pattern.length(), offset, maxCount);
    }

    std::vector<size_t> find_all(const K* pattern, size_t patternLen, size_t offset, size_t maxCount) const {
        std::vector<size_t> result;
        for_all_finded([&](auto f) { result.push_back(f); }, pattern, patternLen, offset, maxCount);
        return result;
    }
    std::vector<size_t> find_all(str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        return find_all(pattern.symbols(), pattern.length(), offset, maxCount);
    }

    size_t find_last(K s, size_t offset = -1) const noexcept {
        size_t len = std::min(_len(), offset);
        const K *text = _str();
        while (len > 0) {
            if (text[--len] == s)
                return len;
        }
        return str::npos;
    }

    size_t find_first_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::string_view{_str(), _len()}.find_first_of(std::string_view{pattern.str, pattern.len}, offset);
    }

    std::pair<size_t, size_t> find_first_of_idx(str_piece pattern, size_t offset = 0) const noexcept {
        const K* text = _str();
        size_t fnd = std::string_view{text, _len()}.find_first_of(std::string_view{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::string::npos ? fnd : pattern.find(text[fnd]) };
    }

    size_t find_first_not_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::string_view{_str(), _len()}.find_first_not_of(std::string_view{pattern.str, pattern.len}, offset);
    }

    size_t find_last_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::string_view{_str(), _len()}.find_last_of(std::string_view{pattern.str, pattern.len}, offset);
    }

    std::pair<size_t, size_t> find_last_of_idx(str_piece pattern, size_t offset = str::npos) const noexcept {
        const K* text = _str();
        size_t fnd = std::string_view{text, _len()}.find_last_of(std::string_view{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::string::npos ? fnd : pattern.find(text[fnd]) };
    }

    size_t find_last_not_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::string_view{_str(), _len()}.find_last_not_of(std::string_view{pattern.str, pattern.len}, offset);
    }

    my_type substr(ptrdiff_t from, ptrdiff_t len = 0) const { // индексация в code units
        return my_type{d()(from, len)};
    }
    my_type str_mid(size_t from, size_t len = -1) const { // индексация в code units
        return my_type{d().mid(from, len)};
    }

    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true>
    T as_int() const noexcept {
        auto [res, err, _] = int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs>(_str(), _len());
        return err == IntConvertResult::Overflow ? 0 : res;
    }

    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true, bool AllowSign = true>
    std::tuple<T, IntConvertResult, size_t> to_int() const noexcept {
        return int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs, AllowSign>(_str(), _len());
    }

    double to_double() const noexcept {
        static_assert(sizeof(K) == 1 || sizeof(K) == sizeof(wchar_t), "Only char and wchar available for conversion to double now");
        size_t len = _len();
        if (len) {
            const size_t copyLen = 64;
            K buf[copyLen + 1];
            const K* ptr = _str();
            if (ptr[len] != 0) {
                while (len && uns_type(*ptr) <= ' ') {
                    len--;
                    ptr++;
                }
                if (len) {
                    len = std::min(copyLen, len);
                    traits::copy(buf, ptr, len);
                    buf[len] = 0;
                    ptr = buf;
                }
            }
            if (len) {
#ifdef _MSC_VER
                static const _locale_t lc = _wcreate_locale(LC_NUMERIC, L"C");
                if constexpr (sizeof(K) == 1) {
                    return _strtod_l(ptr, nullptr, lc);
                }
                if constexpr (sizeof(K) == sizeof(wchar_t)) {
                    return _wcstod_l((const wchar_t*)ptr, nullptr, lc);
                }
#else
                if constexpr (sizeof(K) == 1) {
                    return std::strtod(ptr, nullptr);
                } else if constexpr (sizeof(K) == sizeof(wchar_t)) {
                    return std::wcstod((const wchar_t*)ptr, nullptr);
                }
#endif
            }
        }
        return 0.0;
    }

    template<ToIntNumber T>
    void as_number(T& t) {
        t = as_int<T>();
    }

    void as_number(double& t) {
        t = to_double();
    }

    template<typename T, typename Op>
    T splitf(const K* delimeter, size_t lenDelimeter, const Op& beforeFunc, size_t offset) const {
        size_t mylen = _len();
        T results;
        str_piece me{_str(), mylen};
        for (int i = 0;; i++) {
            size_t beginOfDelim = find(delimeter, lenDelimeter, offset);
            if (beginOfDelim == str::npos) {
                str_piece last{me.symbols() + offset, me.length() - offset};
                if constexpr (std::is_invocable_v<Op, str_piece&>) {
                    beforeFunc(last);
                }
                if constexpr (requires { results.emplace_back(last); }) {
                    if (last.is_same(me)) {
                        // Пробуем положить весь объект.
                        results.emplace_back(d());
                    } else {
                        results.emplace_back(last);
                    }
                } else if constexpr (requires { results.push_back(last); }) {
                    if (last.is_same(me)) {
                        // Пробуем положить весь объект.
                        results.push_back(d());
                    } else {
                        results.push_back(last);
                    }
                } else {
                    if (i < results.size()) {
                        if (last.is_same(me)) {
                            // Пробуем положить весь объект.
                            results[i] = d();
                        } else
                            results[i] = last;
                    }
                }
                break;
            }
            str_piece piece{me.symbols() + offset, beginOfDelim - offset};
            if constexpr (std::is_invocable_v<Op, str_piece&>) {
                beforeFunc(piece);
            }
            if constexpr (requires { results.emplace_back(piece); }) {
                results.emplace_back(piece);
            } else if constexpr (requires { results.push_back(piece); }) {
                results.push_back(piece);
            } else {
                if (i < results.size()) {
                    results[i] = piece;
                    if (i == results.size() - 1) {
                        break;
                    }
                }
            }
            offset = beginOfDelim + lenDelimeter;
        }
        return results;
    }
    template<typename T, typename Op>
    T splitf(str_piece delimeter, const Op& beforeFunc, size_t offset = 0) const {
        return splitf<T>(delimeter.symbols(), delimeter.length(), beforeFunc, offset);
    }
    // Разбиение строки на части
    template<typename T>
    T split(str_piece delimeter, size_t offset = 0) const {
        return splitf<T>(delimeter.symbols(), delimeter.length(), 0, offset);
    }

    Splitter<K> splitter(str_piece delimeter) const;

    // Начинается ли эта строка с указанной подстроки
    constexpr bool starts_with(const K* prefix, size_t l) const noexcept {
        return _len() >= l && 0 == traits::compare(_str(), prefix, l);
    }
    constexpr bool starts_with(str_piece prefix) const noexcept {
        return starts_with(prefix.symbols(), prefix.length());
    }
    // Начинается ли эта строка с указанной подстроки без учета ascii регистра
    constexpr bool starts_with_ia(const K* prefix, size_t len) const noexcept {
        size_t myLen = _len();
        if (myLen < len) {
            return false;
        }
        const K* ptr1 = _str();
        while (len--) {
            K s1 = *ptr1++, s2 = *prefix++;
            if (s1 == s2)
                continue;
            if (makeAsciiLower(s1) != makeAsciiLower(s2))
                return false;
        }
        return true;
    }
    constexpr bool starts_with_ia(str_piece prefix) const noexcept {
        return starts_with_ia(prefix.symbols(), prefix.length());
    }
    // Начинается ли эта строка с указанной подстроки без учета unicode регистра
    bool starts_with_iu(const K* prefix, size_t len) const noexcept {
        return _len() >= len && 0 == uni::compareiu(_str(), len, prefix, len);
    }
    bool starts_with_iu(str_piece prefix) const noexcept {
        return starts_with_iu(prefix.symbols(), prefix.length());
    }

    // Является ли эта строка началом указанной строки
    constexpr bool prefix_in(const K* text, size_t len) const noexcept {
        size_t myLen = _len();
        if (myLen > len)
            return false;
        return !myLen || 0 == traits::compare(text, _str(), myLen);
    }
    constexpr bool prefix_in(str_piece text) const noexcept {
        return prefix_in(text.symbols(), text.length());
    }
    // Заканчивается ли строка указанной подстрокой
    constexpr bool ends_with(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return len <= myLen && traits::compare(_str() + myLen - len, suffix, len) == 0;
    }
    constexpr bool ends_with(str_piece suffix) const noexcept {
        return ends_with(suffix.symbols(), suffix.length());
    }
    // Заканчивается ли строка указанной подстрокой без учета регистра ASCII
    constexpr bool ends_with_ia(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        if (myLen < len) {
            return false;
        }
        const K* ptr1 = _str() + myLen - len;
        while (len--) {
            K s1 = *ptr1++, s2 = *suffix++;
            if (s1 == s2)
                continue;
            if (makeAsciiLower(s1) != makeAsciiLower(s2))
                return false;
        }
        return true;
    }
    constexpr bool ends_with_ia(str_piece suffix) const noexcept {
        return ends_with_ia(suffix.symbols(), suffix.length());
    }
    // Заканчивается ли строка указанной подстрокой без учета регистра UNICODE
    constexpr bool ends_with_iu(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return myLen >= len && 0 == uni::compareiu(_str() + myLen - len, len, suffix, len);
    }
    constexpr bool ends_with_iu(str_piece suffix) const noexcept {
        return ends_with_iu(suffix.symbols(), suffix.length());
    }

    bool is_ascii() const noexcept {
        if (_is_empty())
            return true;
        const int sl = ascii_mask<K>::WIDTH;
        const size_t mask = ascii_mask<K>::VALUE;
        size_t len = _len();
        const uns_type* ptr = reinterpret_cast<const uns_type*>(_str());
        if constexpr (sl > 1) {
            const size_t roundMask = sizeof(size_t) - 1;
            while (len >= sl && (reinterpret_cast<size_t>(ptr) & roundMask) != 0) {
                if (*ptr++ > 127)
                    return false;
                len--;
            }
            while (len >= sl) {
                if (*reinterpret_cast<const size_t*>(ptr) & mask)
                    return false;
                ptr += sl;
                len -= sl;
            }
        }
        while (len--) {
            if (*ptr++ > 127)
                return false;
        }
        return true;
    }
    // ascii версия upper
    template<typename R = my_type>
    R uppered_only_ascii() const {
        return R::uppered_only_ascii_from(d());
    }
    // ascii версия lower
    template<typename R = my_type>
    R lowered_only_ascii() const {
        return R::lowered_only_ascii_from(d());
    }
    template<typename R = my_type>
    R uppered() const {
        return R::uppered_from(d());
    }
    template<typename R = my_type>
    R lowered() const {
        return R::lowered_from(d());
    }

    template<typename R = my_type>
    R replaced(str_piece pattern, str_piece repl, size_t offset = 0, size_t maxCount = 0) const {
        return R::replaced_from(d(), pattern, repl, offset, maxCount);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count, typename M, size_t L = const_lit_for<K, M>::Count>
    expr_replaces<K, N - 1, L - 1> replace_init(T&& pattern, M&& repl) {
        return expr_replaces<K, N - 1, L - 1>{d(), pattern, repl};
    }

    template<StrType<K> From>
    static my_type make_trim_op(const From& from, const auto& opTrim) {
        str_piece sfrom = from, newPos = opTrim(sfrom);
        return newPos.is_same(sfrom) ? my_type{from} : my_type{newPos};
    }
    template<TrimSides S, StrType<K> From>
    static my_type trim_static(const From& from) {
        return make_trim_op(from, trim_operator<S, K, static_cast<size_t>(-1), true>{});
    }

    template<TrimSides S, bool withSpaces, typename T, size_t N = const_lit_for<K, T>::Count, StrType<K> From>
        requires is_const_pattern<N>
    static my_type trim_static(const From& from, T&& pattern) {
        return make_trim_op(from, trim_operator<S, K, N - 1, withSpaces>{pattern});
    }

    template<TrimSides S, bool withSpaces, StrType<K> From>
    static my_type trim_static(const From& from, str_piece pattern) {
        return make_trim_op(from, trim_operator<S, K, 0, withSpaces>{{pattern}});
    }
    // Триминг по пробельным символам - ' ', \t\n\v\f\r
    template<typename R = my_type>
    R trimmed() const {
        return R::template trim_static<TrimSides::TrimAll>(d());
    }
    template<typename R = my_type>
    R trimmed_left() const {
        return R::template trim_static<TrimSides::TrimLeft>(d());
    }
    template<typename R = my_type>
    R trimmed_right() const {
        return R::template trim_static<TrimSides::TrimRight>(d());
    }
    // Триминг по символам в литерале
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    // Триминг по символам в литерале и пробелам
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    template<typename R = my_type, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }
    // Триминг по динамическому источнику
    template<typename R = my_type>
    R trimmed(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    template<typename R = my_type>
    R trimmed_left(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    template<typename R = my_type>
    R trimmed_right(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    // Триминг по символам в литерале и пробелам
    template<typename R = my_type>
    R trimmed_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    template<typename R = my_type>
    R trimmed_left_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    template<typename R = my_type>
    R trimmed_right_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }
};

#ifdef _MSC_VER
#define empty_bases __declspec(empty_bases)
#else
#define empty_bases
#endif

/*
* Базовая структура с информацией о строке.
* Это структура для невладеющих строк.
* Так как здесь только один базовый класс, MSVC компилятор автоматом применяет empty base optimization,
* в результате размер класса не увеличивается
*/
template<typename K>
struct simple_str : str_algs<K, simple_str<K>, simple_str<K>> {
    using symb_type = K;
    using my_type = simple_str<K>;

    const symb_type* str;
    size_t len;

    simple_str() = default;

    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr simple_str(T&& v) noexcept : str(v), len(N - 1) {}

    constexpr simple_str(const K* p, size_t l) noexcept : str(p), len(l) {}

    // Конструктор, позволяющий инициализировать объектами std::string, и std::string_view
    // при условии, что они lvalue, то есть не временные.
    template<typename S>
        requires(std::is_same_v<S, std::string&> || std::is_same_v<S, const std::string&>
            || std::is_same_v<S, std::string_view&> || std::is_same_v<S, const std::string_view&>)
    constexpr simple_str(S&& s) noexcept : str(s.data()), len(s.length()) {}

    constexpr size_t length() const noexcept {
        return len;
    }
    constexpr const symb_type* symbols() const noexcept {
        return str;
    }
    constexpr bool is_empty() const noexcept {
        return len == 0;
    }

    bool is_same(simple_str<K> other) const noexcept {
        return str == other.str && len == other.len;
    }

    bool is_part_of(simple_str<K> other) const noexcept {
        return str >= other.str && str + len <= other.str + other.len;
    }

    K operator[](size_t idx) const {
        return str[idx];
    }

    my_type& remove_prefix(size_t delta) {
        str += delta;
        len -= delta;
        return *this;
    }

    my_type& remove_suffix(size_t delta) {
        len -= delta;
        return *this;
    }
};

/*
* Класс, заявляющий, что ссылается на нуль-терминированную строку.
* Служит для показателя того, что функция параметром хочет получить
* строку с нулем в конце, например, ей надо дальше передавать его в
* стороннее API. Без этого ей надо было бы либо указывать параметром
* конкретный класс строки, что лишает универсальности, либо приводило бы
* к постоянным накладным расходам на излишнее копирование строк во временный
* буфер. Источником нуль-терминированных строк могут быть константные строки
* при компиляции, либо классы, хранящие строки.
*/
template<typename K>
struct simple_str_nt : simple_str<K> {
    using symb_type = K;
    using my_type = simple_str_nt<K>;
    using base = simple_str<K>;
    using base::base;

    constexpr static const K empty_string[1] = {0};

    simple_str_nt() = default;

    template<typename T> requires std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<T>>>, K>
    explicit simple_str_nt(T&& p) noexcept {
        base::len = p ? static_cast<size_t>(base::traits::length(p)) : 0;
        base::str = base::len ? p : empty_string;
    }
    // Конструктор, позволяющий инициализировать объектами std::string, при условии, что они lvalue, то есть не временные.
    template<typename S>
        requires(std::is_same_v<std::remove_cvref_t<S>, std::string> && std::is_lvalue_reference_v<S>)
    constexpr simple_str_nt(S&& s) noexcept : base(s) {}

    constexpr simple_str_nt(std::string_view s) noexcept : base(s) {}

    static const my_type empty_str;

    operator const K*() const noexcept {
        return base::str;
    }

    my_type toNts(size_t from) {
        if (from > base::len) {
            from = base::len;
        }
        return {base::str + from, base::len - from};
    }

    const K* c_str() const noexcept {
        return base::str;
    }
    // for std::string compatibility
    const K* data() const {
        return c_str();
    }
};

template<typename K>
inline const simple_str_nt<K> simple_str_nt<K>::empty_str{simple_str_nt<K>::empty_string, 0};

using ssa = simple_str<u8s>;
using ssw = simple_str<wchar_t>;
using ssu = simple_str<u16s>;
using ssuu = simple_str<u32s>;
using stra = simple_str_nt<u8s>;
using strw = simple_str_nt<wchar_t>;
using stru = simple_str_nt<u16s>;
using struu = simple_str_nt<u32s>;

template<typename K>
class Splitter {
    simple_str<K> text_;
    simple_str<K> delim_;

public:
    Splitter(simple_str<K> text, simple_str<K> delim) : text_(text), delim_(delim) {}

    bool isDone() const {
        return text_.length() == str::npos;
    }

    simple_str<K> next() {
        if (!text_.length()) {
            auto ret = text_;
            text_.str++;
            text_.len--;
            return ret;
        } else if (text_.length() == str::npos) {
            return {nullptr, 0};
        }
        size_t pos = text_.find(delim_), next = 0;
        if (pos == str::npos) {
            pos = text_.length();
            next = pos + 1;
        } else {
            next = pos + delim_.length();
        }
        simple_str<K> result{text_.str, pos};
        text_.str += next;
        text_.len -= next;
        return result;
    }
};

template<typename K, typename StrRef, typename Impl>
Splitter<K> str_algs<K, StrRef, Impl>::splitter(StrRef delimeter) const {
    return Splitter<K>{*this, delimeter};
}

template<typename K, bool withSpaces>
struct CheckSpaceTrim {
    bool is_trim_spaces(K s) const {
        return s == ' ' || (s >= 9 && s <= 13); // || isspace(s);
    }
};
template<typename K>
struct CheckSpaceTrim<K, false> {
    bool is_trim_spaces(K) const {
        return false;
    }
};

template<typename K>
struct CheckSymbolsTrim {
    simple_str<K> symbols;
    bool is_trim_symbols(K s) const {
        return symbols.len != 0 && simple_str<K>::traits::find(symbols.str, symbols.len, s) != nullptr;
    }
};

template<typename K, size_t N>
struct CheckConstSymbolsTrim {
    const const_lit_to_array<K, N> symbols;

    template<typename T, size_t M = const_lit_for<K, T>::Count> requires (M == N + 1)
    constexpr CheckConstSymbolsTrim(T&& s) : symbols(std::forward<T>(s)) {}

    bool is_trim_symbols(K s) const noexcept {
        return symbols.contain(s);
    }
};

template<typename K>
struct CheckConstSymbolsTrim<K, 0> {
    bool is_trim_symbols(K) const {
        return false;
    }
};

template<typename K, size_t N>
struct SymbSelector {
    using type = CheckConstSymbolsTrim<K, N>;
};

template<typename K>
struct SymbSelector<K, 0> {
    using type = CheckSymbolsTrim<K>;
};

template<typename K>
struct SymbSelector<K, static_cast<size_t>(-1)> {
    using type = CheckConstSymbolsTrim<K, 0>;
};

template<TrimSides S, typename K, size_t N, bool withSpaces>
struct trim_operator : SymbSelector<K, N>::type, CheckSpaceTrim<K, withSpaces> {
    bool isTrim(K s) const {
        return CheckSpaceTrim<K, withSpaces>::is_trim_spaces(s) || SymbSelector<K, N>::type::is_trim_symbols(s);
    }
    simple_str<K> operator()(simple_str<K> from) const {
        if constexpr ((S & TrimSides::TrimLeft) != 0) {
            while (from.len) {
                if (isTrim(*from.str)) {
                    from.str++;
                    from.len--;
                } else
                    break;
            }
        }
        if constexpr ((S & TrimSides::TrimRight) != 0) {
            const K* back = from.str + from.len - 1;
            while (from.len) {
                if (isTrim(*back)) {
                    back--;
                    from.len--;
                } else
                    break;
            }
        }
        return from;
    }
};

template<TrimSides S, typename K>
using SimpleTrim = trim_operator<S, K, size_t(-1), true>;

using trim_w = SimpleTrim<TrimSides::TrimAll, u16s>;
using trim_a = SimpleTrim<TrimSides::TrimAll, u8s>;
using triml_w = SimpleTrim<TrimSides::TrimLeft, u16s>;
using triml_a = SimpleTrim<TrimSides::TrimLeft, u8s>;
using trimr_w = SimpleTrim<TrimSides::TrimRight, u16s>;
using trimr_a = SimpleTrim<TrimSides::TrimRight, u8s>;

template<TrimSides S = TrimSides::TrimAll, bool withSpaces = false, typename K, typename T, size_t N = const_lit_for<K, T>::Count>
    requires is_const_pattern<N>
inline auto trimOp(T&& pattern) {
    return trim_operator<S, K, N - 1, withSpaces>{pattern};
}

template<TrimSides S = TrimSides::TrimAll, bool withSpaces = false, typename K>
inline auto trimOp(simple_str<K> pattern) {
    return trim_operator<S, K, 0, withSpaces>{pattern};
}

template<typename Src, typename Dest>
struct utf_convert_selector;

template<>
struct utf_convert_selector<u8s, u16s> {
    static SIMSTR_API size_t need_len(const u8s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u8s* src, size_t srcLen, u16s* dest);
};

template<>
struct utf_convert_selector<u8s, u32s> {
    static SIMSTR_API size_t need_len(const u8s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u8s* src, size_t srcLen, u32s* dest);
};

template<>
struct utf_convert_selector<u8s, wchar_t> {
    static size_t need_len(const u8s* src, size_t srcLen) {
        return utf_convert_selector<u8s, wchar_type>::need_len(src, srcLen);
    }
    static size_t convert(const u8s* src, size_t srcLen, wchar_t* dest) {
        return utf_convert_selector<u8s, wchar_type>::convert(src, srcLen, to_w(dest));
    }
};

template<>
struct utf_convert_selector<u16s, u8s> {
    static SIMSTR_API size_t need_len(const u16s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u16s* src, size_t srcLen, u8s* dest);
};

template<>
struct utf_convert_selector<u16s, u32s> {
    static SIMSTR_API size_t need_len(const u16s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u16s* src, size_t srcLen, u32s* dest);
};

template<>
struct utf_convert_selector<u16s, u16s> {
    // При конвертации char16_t в wchar_t под windows будет вызываться эта реализация
    static size_t need_len(const u16s* src, size_t srcLen) {
        return srcLen;
    }
    static size_t convert(const u16s* src, size_t srcLen, u16s* dest) {
        ch_traits<u16s>::copy(dest, src, srcLen + 1);
        return srcLen;
    }
};

template<>
struct utf_convert_selector<u32s, u32s> {
    // При конвертации char32_t в wchar_t под linux будет вызываться эта реализация
    static size_t need_len(const u32s* src, size_t srcLen) {
        return srcLen;
    }
    static size_t convert(const u32s* src, size_t srcLen, u32s* dest) {
        ch_traits<u32s>::copy(dest, src, srcLen + 1);
        return srcLen;
    }
};

template<>
struct utf_convert_selector<u16s, wchar_t> {
    static size_t need_len(const u16s* src, size_t srcLen) {
        return utf_convert_selector<u16s, wchar_type>::need_len(src, srcLen);
    }
    static size_t convert(const u16s* src, size_t srcLen, wchar_t* dest) {
        return utf_convert_selector<u16s, wchar_type>::convert(src, srcLen, to_w(dest));
    }
};

template<>
struct utf_convert_selector<u32s, u8s> {
    static SIMSTR_API size_t need_len(const u32s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u32s* src, size_t srcLen, u8s* dest);
};

template<>
struct utf_convert_selector<u32s, u16s> {
    static SIMSTR_API size_t need_len(const u32s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u32s* src, size_t srcLen, u16s* dest);
};

template<>
struct utf_convert_selector<u32s, wchar_t> {
    static size_t need_len(const u32s* src, size_t srcLen) {
        return utf_convert_selector<u32s, wchar_type>::need_len(src, srcLen);
    }
    static size_t convert(const u32s* src, size_t srcLen, wchar_t* dest) {
        return utf_convert_selector<u32s, wchar_type>::convert(src, srcLen, to_w(dest));
    }
};

template<>
struct utf_convert_selector<wchar_t, u8s> {
    static size_t need_len(const wchar_t* src, size_t srcLen) {
        return utf_convert_selector<wchar_type, u8s>::need_len(to_w(src), srcLen);
    }
    static size_t convert(const wchar_t* src, size_t srcLen, u8s* dest) {
        return utf_convert_selector<wchar_type, u8s>::convert(to_w(src), srcLen, dest);
    }
};

template<>
struct utf_convert_selector<wchar_t, u16s> {
    static size_t need_len(const wchar_t* src, size_t srcLen) {
        return utf_convert_selector<wchar_type, u16s>::need_len(to_w(src), srcLen);
    }
    static size_t convert(const wchar_t* src, size_t srcLen, u16s* dest) {
        return utf_convert_selector<wchar_type, u16s>::convert(to_w(src), srcLen, dest);
    }
};

template<>
struct utf_convert_selector<wchar_t, u32s> {
    static size_t need_len(const wchar_t* src, size_t srcLen) {
        return utf_convert_selector<wchar_type, u32s>::need_len(to_w(src), srcLen);
    }
    static size_t convert(const wchar_t* src, size_t srcLen, u32s* dest) {
        return utf_convert_selector<wchar_type, u32s>::convert(to_w(src), srcLen, dest);
    }
};

template<typename K, typename Impl>
class from_utf_convertable {
protected:
    from_utf_convertable() = default;
    using my_type = Impl;
    /*
     Эти методы должен реализовать класс-наследник.
     вызывается только при создании объекта
       init(size_t size)
       set_size(size_t size)
    */
public:
    template<typename O>
        requires(!std::is_same_v<O, K>)
    from_utf_convertable(simple_str<O> init) {
        using worker = utf_convert_selector<O, K>;
        Impl* d = static_cast<Impl*>(this);
        size_t len = init.length();
        if (!len)
            d->create_empty();
        else {
            size_t need = worker::need_len(init.symbols(), len);
            K* str = d->init(need);
            str[need] = 0;
            worker::convert(init.symbols(), len, str);
        }
    }
    template<typename O, typename I>
        requires(!std::is_same_v<O, K>)
    from_utf_convertable(const str_algs<O, simple_str<O>, I>& init) : from_utf_convertable(init.to_str()) {}
};

/*!
 * @brief Строковое выражение для конвертации строк в разные виды UTF
 * @tparam From - Тип какой строки конвертируем
 * @tparam To - В какого типа строку конвертируем
 */
template<typename From, typename To> requires (!std::is_same_v<From, To>)
struct expr_utf {
    using symb_type = To;
    using worker = utf_convert_selector<From, To>;

    simple_str<From> source_;

    size_t length() const noexcept {
        return worker::need_len(source_.symbols(), source_.length());
    }
    To* place(To* ptr) const noexcept {
        return ptr + worker::convert(source_.symbols(), source_.length(), ptr);
    }
};

template<typename To, typename From> requires (!std::is_same_v<From, To>)
auto e_utf(simple_str<From> from) {
    return expr_utf<From, To>{from};
}

template<typename A, typename K>
concept storable_str = requires {
    A::is_str_storable == true;
    std::is_same_v<typename A::symb_type, K>;
};

template<typename A, typename K>
concept mutable_str = storable_str<A, K> && requires { A::is_str_mutable == true; };

template<typename A, typename K>
concept immutable_str = storable_str<A, K> && !mutable_str<A, K>;

template<typename A>
concept Allocatorable = requires(A& a, size_t size, void* void_ptr) {
    { a.allocate(size) } -> std::same_as<void*>;
    { a.deallocate(void_ptr) } noexcept -> std::same_as<void>;
};

/*
* База для объектов, владеющих строкой
* По прежнему ничего не знает о том, где наследник хранит строку и её размер
* Просто вызывает его методы для получения места, и заполняет его при необходимости.
* Работает только при создании объекта, не работает с модификацией строки после
* ее создания и гарантирует, что если вызываются эти методы, объект еще только
* создается, и какого-либо расшаривания данных еще не было.
* Эти методы должен реализовать класс-наследник, вызываются только при создании объекта
*   K* init(size_t size)     - выделить место для строки указанного размера, вернуть адрес
*   void create_empty()      - создать пустой объект
*   K* set_size(size_t size) - перевыделить место для строки, если при создании не угадали
*                              нужный размер и место нужно больше или меньше.
*                              Содержимое строки нужно оставить.
*
* K     - тип символов
* Impl  - тип наследника
*/
template<typename K, typename Impl, Allocatorable Allocator>
class str_storable {
public:
    using my_type = Impl;
    using traits = ch_traits<K>;
    using allocator_t = Allocator;

protected:
    [[_no_unique_address]]
    allocator_t allocator_;

    allocator_t& allocator() {
        return allocator_;
    }
    const allocator_t& allocator() const {
        return allocator_;
    }

    using uni = unicode_traits<K>;

    Impl& d() noexcept {
        return *static_cast<Impl*>(this);
    }
    const Impl& d() const noexcept {
        return *static_cast<const Impl*>(this);
    }
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    explicit constexpr str_storable(size_t size, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (size)
            d().init(size);
        else
            d().create_empty();
    }

    template<StrType<K> From, typename Op1, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type changeCaseAscii(const From& f, const Op1& opMakeNeedCase, Args&&... args) {
        my_type result{std::forward<Args>(args)...};
        size_t len = f.length();
        if (len) {
            const K* source = f.symbols();
            K* destination = result.init(len);
            for (size_t l = 0; l < len; l++) {
                destination[l] = opMakeNeedCase(source[l]);
            }
        }
        return result;
    }

    template<typename T, bool Dummy = true>
    struct ChangeCase {
        template<typename From, typename Op1, typename... Args>
            requires std::is_constructible_v<allocator_t, Args...>
        static my_type changeCase(const From& f, const Op1& opChangeCase, Args&&... args) {
            my_type result{std::forward<Args>(args)...};
            ;
            size_t len = f.length();
            if (len) {
                opChangeCase(f.symbols(), len, result.init(len));
            }
            return result;
        }
    };
    // Для utf8 сделаем отдельную спецификацию, так как при смене регистра может изменится длина строки
    template<bool Dummy>
    struct ChangeCase<u8s, Dummy> {
        template<typename From, typename Op1, typename... Args>
            requires std::is_constructible_v<allocator_t, Args...>
        static my_type changeCase(const From& f, const Op1& opChangeCase, Args&&... args) {
            my_type result{std::forward<Args>(args)...};
            ;
            size_t len = f.length();
            if (len) {
                const K* ptr = f.symbols();
                K* pWrite = result.init(len);

                const u8s* source = ptr;
                u8s* dest = pWrite;
                size_t newLen = opChangeCase(source, len, dest, len);
                if (newLen < len) {
                    // Строка просто укоротилась
                    result.set_size(newLen);
                } else if (newLen > len) {
                    // Строка не влезла в буфер.
                    size_t readed = static_cast<size_t>(source - ptr);
                    size_t writed = static_cast<size_t>(dest - pWrite);
                    pWrite = result.set_size(newLen);
                    dest = pWrite + writed;
                    opChangeCase(source, len - readed, dest, newLen - writed);
                }
                pWrite[newLen] = 0;
            }
            return result;
        }
    };

public:
    using s_str = simple_str<K>;
    using s_str_nt = simple_str_nt<K>;

    inline static constexpr bool is_str_storable = true;

    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(Args&&... args) noexcept(std::is_nothrow_constructible_v<allocator_t, Args...>)
        : allocator_(std::forward<Args>(args)...) {
        d().create_empty();
    }

    // Конструктор из другого строкового объекта
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(s_str other, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (other.length()) {
            K* ptr = d().init(other.length());
            traits::copy(ptr, other.symbols(), other.length());
            ptr[other.length()] = 0;
        } else
            d().create_empty();
    }
    // Конструктор повторения
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(size_t repeat, s_str pattern, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        size_t l = pattern.length(), allLen = l * repeat;
        if (allLen) {
            K* ptr = d().init(allLen);
            for (size_t i = 0; i < repeat; i++) {
                traits::copy(ptr, pattern.symbols(), l);
                ptr += l;
            }
            *ptr = 0;
        } else
            d().create_empty();
    }

    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(size_t count, K pad, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        if (count) {
            K* str = d().init(count);
            traits::assign(str, count, pad);
            str[count] = 0;
        } else
            d().create_empty();
    }

    // Конструктор из строкового выражения
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(const StrExprForType<K> auto& expr, Args&&... args) : allocator_(std::forward<Args>(args)...) {
        size_t len = expr.length();
        if (len)
            *expr.place(d().init(len)) = 0;
        else
            d().create_empty();
    }

    // Конструктор из строкового источника с заменой
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args)
        : allocator_(std::forward<Args>(args)...) {

        auto findes = f.find_all(pattern, offset, maxCount);
        if (!findes.size()) {
            new (this) my_type{f};
            return;
        }
        size_t srcLen = f.length();
        size_t newSize = srcLen + static_cast<ptrdiff_t>(repl.len - pattern.len) * findes.size();

        if (!newSize) {
            new (this) my_type{};
            return;
        }

        K* ptr = d().init(newSize);
        const K* src = f.symbols();
        size_t from = 0;
        for (const auto& s: findes) {
            size_t copyLen = s - from;
            if (copyLen) {
                traits::copy(ptr, src + from, copyLen);
                ptr += copyLen;
            }
            if (repl.len) {
                traits::copy(ptr, repl.str, repl.len);
                ptr += repl.len;
            }
            from = s + pattern.len;
        }
        srcLen -= from;
        if (srcLen) {
            traits::copy(ptr, src + from, srcLen);
            ptr += srcLen;
        }
        *ptr = 0;
    }
    operator const K*() const noexcept {
        return d().symbols();
    }
    // for std::string compatibility
    const K* c_str() const {
        return d().symbols();
    }
    const K* data() const {
        return d().symbols();
    }

    s_str_nt to_nts(size_t from = 0) const {
        size_t len = d().length();
        if (from >= len) {
            from = len;
        }
        return {d().symbols() + from, len - from};
    }

    operator s_str_nt() const {
        return {d().symbols(), d().length()};
    }

    // Слияние контейнера строк
    template<typename T, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type join(const T& strings, s_str delimeter, bool tail = false, Args&&... args) {
        my_type result(std::forward<Args>(args)...);
        if (strings.size()) {
            if (strings.size() == 1 && (!delimeter.length() || !tail)) {
                result = strings.front();
            } else {
                size_t commonLen = 0;
                for (auto it = strings.begin(), e = strings.end(); it != e;) {
                    commonLen += it->length();
                    ++it;
                    if (it != e || tail)
                        commonLen += delimeter.length();
                }
                if (commonLen) {
                    K* ptr = result.init(commonLen);
                    for (auto it = strings.begin(), e = strings.end(); it != e;) {
                        size_t copyLen = it->length();
                        if (copyLen) {
                            traits::copy(ptr, it->symbols(), copyLen);
                            ptr += copyLen;
                        }
                        ++it;
                        if (delimeter.length() && (it != e || tail)) {
                            traits::copy(ptr, delimeter.symbols(), delimeter.length());
                            ptr += delimeter.length();
                        }
                    }
                    *ptr = 0;
                }
            }
        }
        return result;
    }
    // ascii версия upper
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiUpper<K>, std::forward<Args>(args)...);
    }

    // ascii версия lower
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiLower<K>, std::forward<Args>(args)...);
    }

    // Юникодная версия
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::upper, std::forward<Args>(args)...);
    }

    // Юникодная версия
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::lower, std::forward<Args>(args)...);
    }

    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type replaced_from(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args) {
        return my_type{f, pattern, repl, offset, maxCount, std::forward<Args>(args)...};
    }
};

template<typename K, Allocatorable Allocator>
class sstring;

struct printf_selector {
    template<typename K, typename... T>  requires (is_one_of_std_char_v<K>)
    static int snprintf(K* buffer, size_t count, const K* format, T&&... args) {
        if constexpr (std::is_same_v<K, u8s>) {
          #ifndef _WIN32
            return std::snprintf(buffer, count, format, std::forward<T>(args)...);
          #else
            // Поддерживает позиционные параметры
            return _sprintf_p(buffer, count, format, args...);
          #endif
        } else {
          #ifndef _WIN32
            return std::swprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #else
            // Поддерживает позиционные параметры
            return _swprintf_p(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #endif
        }
    }
    template<typename K>  requires (is_one_of_std_char_v<K>)
    static int vsnprintf(K* buffer, size_t count, const K* format, va_list args) {
        if constexpr (std::is_same_v<K, u8s>) {
          #ifndef _WIN32
            return std::vsnprintf(buffer, count, format, args);
          #else
            // Поддерживает позиционные параметры
            return _vsprintf_p(buffer, count, format, args);
          #endif
        } else {
          #ifndef _WIN32
            return std::vswprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args);
          #else
            // Поддерживает позиционные параметры
            return _vswprintf_p(buffer, count, format, args);
          #endif
        }
    }
};

inline size_t grow2(size_t ret, size_t currentCapacity) {
    return ret <= currentCapacity ? ret : ret * 2;
}

/*
* Базовый класс работы с меняющимися inplace строками
* По прежнему ничего не знает о том, где наследник хранит строку и её размер
* Просто вызывает его методы для получения места, и заполняет его при необходимости.
* Для работы класс-наследник должен реализовать методы:
*   size_t length() const noexcept      - возвращает длину строки
*   const K* symbols() const            - возвращает указатель на начало строки
*   bool is_empty() const noexcept      - проверка, не пустая ли строка
*   K* str() noexcept                   - Неконстантный указатель на начало строки
*   K* set_size(size_t size)            - Изменить размер строки, как больше, так и меньше.
*                                         Содержимое строки нужно оставить.
*   K* reserve_no_preserve(size_t size) - выделить место под строку, старую можно не сохранять
*   size_t capacity() const noexcept    - вернуть текущую ёмкость строки, сколько может поместится
*                                         без аллокации
*
* K      - тип символов
* StrRef - тип хранилища куска строки
* Impl   - тип наследника
*/
template<typename K, typename StrRef, typename Impl>
class str_mutable {
public:
    using my_type = Impl;

private:
    Impl& d() {
        return *static_cast<Impl*>(this);
    }
    const Impl& d() const {
        return *static_cast<const Impl*>(this);
    }
    size_t _len() const noexcept {
        return d().length();
    }
    const K* _str() const noexcept {
        return d().symbols();
    }
    using simple_str = StrRef;
    using symb_type = K;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using uns_type = std::make_unsigned_t<K>;

    template<typename Op>
    Impl& make_trim_op(const Op& op) {
        simple_str me = static_cast<simple_str>(d()), pos = op(me);
        if (me.length() != pos.length()) {
            if (me.symbols() != pos.symbols())
                traits::move(const_cast<K*>(me.symbols()), pos.symbols(), pos.length());
            d().set_size(pos.length());
        }
        return d();
    }

    template<auto Op>
    Impl& commonChangeCase() {
        size_t len = _len();
        if (len)
            Op(_str(), len, str());
        return d();
    }
    // GCC до сих пор не позволяет делать внутри класса полную специализацию вложенного класса,
    // только частичную. Поэтому добавим неиспользуемый параметр шаблона.
    template<typename T, bool Dummy = true>
    struct CaseTraits {
        static Impl& upper(Impl& obj) {
            return obj.template commonChangeCase<unicode_traits<K>::upper>();
        }
        static Impl& lower(Impl& obj) {
            return obj.template commonChangeCase<unicode_traits<K>::lower>();
        }
    };

    template<auto Op>
    Impl& utf8CaseChange() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        size_t len = _len();
        if (len) {
            u8s* writePos = str();
            const u8s *startData = writePos, *readPos = writePos;
            size_t newLen = Op(readPos, len, writePos, len);
            if (newLen < len) {
                // Строка просто укоротилась
                d().set_size(newLen);
            } else if (newLen > len) {
                // Строка не влезла в буфер.
                size_t readed = static_cast<size_t>(readPos - startData);
                size_t writed = static_cast<size_t>(writePos - startData);
                d().set_size(newLen);
                startData = str(); // при изменении размера могло изменится
                readPos = startData + readed;
                writePos = const_cast<u8s*>(startData) + writed;
                Op(readPos, len - readed, writePos, newLen - writed);
            }
        }
        return d();
    }
    template<bool Dummy>
    struct CaseTraits<u8s, Dummy> {
        static Impl& upper(Impl& obj) {
            return obj.template utf8CaseChange<unicode_traits<u8s>::upper>();
        }
        static Impl& lower(Impl& obj) {
            return obj.template utf8CaseChange<unicode_traits<u8s>::lower>();
        }
    };

    template<TrimSides S, bool withSpaces, typename T, size_t N = const_lit_for<K, T>::Count>
    Impl& makeTrim(T&& pattern) {
        return make_trim_op(trim_operator<S, K, N - 1, withSpaces>{pattern});
    }

    template<TrimSides S, bool withSpaces>
    Impl& makeTrim(simple_str pattern) {
        return make_trim_op(trim_operator<S, K, 0, withSpaces>{{pattern}});
    }

public:
    K* str() noexcept {
        return d().str();
    }
    explicit operator K*() noexcept {
        return str();
    }

    Impl& trim() {
        return make_trim_op(SimpleTrim<TrimSides::TrimAll, K>{});
    }
    Impl& trim_left() {
        return make_trim_op(SimpleTrim<TrimSides::TrimLeft, K>{});
    }
    Impl& trim_right() {
        return make_trim_op(SimpleTrim<TrimSides::TrimRight, K>{});
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, false>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right_with_wpaces(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }

    Impl& trim(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimAll, false>(pattern) : d();
    }
    Impl& trim_left(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimLeft, false>(pattern) : d();
    }
    Impl& trim_right(simple_str pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimRight, false>(pattern) : d();
    }
    Impl& trim_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    Impl& trim_left_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    Impl& trim_right_with_spaces(simple_str pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }

    Impl& upper_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiLower(s))
                *ptr = s & ~0x20;
        }
        return d();
    }
    Impl& lower_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiUpper(s))
                *ptr = s | 0x20;
        }
        return d();
    }

    Impl& upper() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        return CaseTraits<K>::upper(d());
    }
    Impl& lower() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        return CaseTraits<K>::lower(d());
    }

private:
    template<typename T>
    Impl& changeImpl(size_t from, size_t len, T expr) {
        size_t myLen = _len();
        if (from > myLen) {
            from = myLen;
        }
        if (from + len > myLen) {
            len = myLen - from;
        }
        K* buffer = str();
        size_t otherLen = expr.length();
        if (len == otherLen) {
            expr.place(buffer + from);
        } else {
            size_t tailLen = myLen - from - len;
            if (len > otherLen) {
                expr.place(buffer + from);
                traits::move(buffer + from + otherLen, buffer + from + len, tailLen);
                d().set_size(myLen - (len - otherLen));
            } else {
                buffer = d().set_size(myLen + otherLen - len);
                traits::move(buffer + from + otherLen, buffer + from + len, tailLen);
                expr.place(buffer + from);
            }
        }
        return d();
    }

    template<typename T>
    Impl& appendImpl(T expr) {
        if (size_t len = expr.length(); len) {
            size_t size = _len();
            expr.place(d().set_size(size + len) + size);
        }
        return d();
    }

    template<typename T>
    Impl& appendFromImpl(size_t pos, T expr) {
        if (pos > _len())
            pos = _len();
        if (size_t len = expr.length())
            expr.place(d().set_size(pos + len) + pos);
        else
            d().set_size(pos);
        return d();
    }

public:
    inline static constexpr bool is_str_mutable = true;

    Impl& append(simple_str other) {
        return appendImpl<simple_str>(other);
    }

    template<StrExprForType<K> A>
    Impl& append(const A& expr) {
        return appendImpl<const A&>(expr);
    }

    Impl& operator+=(simple_str other) {
        return appendImpl<simple_str>(other);
    }

    template<StrExprForType<K> A>
    Impl& operator+=(const A& expr) {
        return appendImpl<const A&>(expr);
    }

    Impl& append_in(size_t pos, simple_str other) {
        return appendFromImpl<simple_str>(pos, other);
    }

    template<StrExprForType<K> A>
    Impl& append_in(size_t pos, const A& expr) {
        return appendFromImpl<const A&>(pos, expr);
    }

    Impl& change(size_t from, size_t len, simple_str other) {
        return changeImpl<simple_str>(from, len, other);
    }

    template<StrExprForType<K> A>
    Impl& change(size_t from, size_t len, const A& expr) {
        return changeImpl<const A&>(from, len, expr);
    }

    Impl& insert(size_t to, simple_str other) {
        return changeImpl<simple_str>(to, 0, other);
    }
    template<StrExprForType<K> A>
    Impl& insert(size_t to, const A& expr) {
        return changeImpl<const A&>(to, 0, expr);
    }

    Impl& remove(size_t from, size_t to) {
        return changeImpl<const empty_expr<K>&>(from, to, {});
    }

    Impl& prepend(simple_str other) {
        return changeImpl<simple_str>(0, 0, other);
    }
    template<StrExprForType<K> A>
    Impl& prepend(const A& expr) {
        return changeImpl<const A&>(0, 0, expr);
    }

    Impl& replace(simple_str pattern, simple_str repl, size_t offset = 0, size_t maxCount = 0) {
        offset = d().find(pattern, offset);
        if (offset == str::npos) {
            return d();
        }
        if (!maxCount)
            maxCount--;
        size_t replLength = repl.length(), patternLength = pattern.length();

        if (patternLength == replLength) {
            // Заменяем inplace на подстроку такой же длины
            K* ptr = str();
            for (size_t i = 0; i < maxCount; i++) {
                traits::copy(ptr + offset, repl.symbols(), replLength);
                offset = d().find(pattern, offset + replLength);// replLength == patternLength
                if (offset == str::npos)
                    break;
            }
        } else if (patternLength > replLength) {
            // Заменяем на более короткий кусок, длина текста уменьшится, идём слева направо
            K* ptr = str();
            traits::copy(ptr + offset, repl.symbols(), replLength);
            size_t posWrite = offset + replLength;
            maxCount--;
            offset += patternLength;

            for (size_t i = 0; i < maxCount; i++) {
                size_t idx = d().find(pattern, offset);
                if (idx == str::npos)
                    break;
                size_t lenOfPiece = idx - offset;
                traits::move(ptr + posWrite, ptr + offset, lenOfPiece);
                posWrite += lenOfPiece;
                traits::copy(ptr + posWrite, repl.symbols(), replLength);
                posWrite += replLength;
                offset = idx + patternLength;
            }
            size_t tailLen = _len() - offset;
            traits::move(ptr + posWrite, ptr + offset, tailLen);
            d().set_size(posWrite + tailLen);
        } else {
            struct replace_grow_helper {
                replace_grow_helper(my_type& src, simple_str p, simple_str r, size_t mc, size_t d)
                    : source(src), pattern(p), repl(r), maxCount(mc), delta(d) {}
                my_type& source;
                const simple_str pattern;
                const simple_str repl;
                size_t maxCount;
                const size_t delta;
                size_t all_delta{};
                K* reserve_for_copy{};
                size_t end_of_piece{};
                size_t total_length{};

                void replace(size_t offset) {
                    size_t finded[16] = {source.find(pattern, offset)};
                    if (finded[0] == str::npos) {
                        return;
                    }
                    maxCount--;
                    offset = finded[0] + pattern.length();
                    all_delta += delta;
                    size_t idx = 1;
                    for (size_t end = std::min(maxCount, std::size(finded)); idx < end; idx++, maxCount--) {
                        finded[idx] = source.find(pattern, offset);
                        if (finded[idx] == str::npos) {
                            break;
                        }
                        offset = finded[idx] + pattern.length();
                        all_delta += delta;
                    }
                    bool needMore = maxCount > 0 && idx == std::size(finded) && offset < source.length() - pattern.length();
                    if (needMore) {
                        replace(offset); // здесь произведутся замены в оставшемся хвосте
                    }
                    // Теперь делаем свои замены
                    if (!reserve_for_copy) {
                        // Только начинаем
                        end_of_piece = source.length();
                        total_length = end_of_piece + all_delta;
                        reserve_for_copy = source.alloc_for_copy(total_length);
                    }
                    K* dst_start = reserve_for_copy;
                    const K* src_start = source.symbols();
                    while(idx-- > 0) {
                        size_t pos = finded[idx] + pattern.length();
                        size_t lenOfPiece = end_of_piece - pos;
                        ch_traits<K>::move(dst_start + pos + all_delta, src_start + pos, lenOfPiece);
                        ch_traits<K>::copy(dst_start + pos + all_delta - repl.length(), repl.symbols(), repl.length());
                        all_delta -= delta;
                        end_of_piece = finded[idx];
                    }
                    if (!all_delta && reserve_for_copy != src_start) {
                        ch_traits<K>::copy(dst_start, src_start, finded[0]);
                    }
                }
            } helper(d(), pattern, repl, maxCount, repl.length() - pattern.length());
            helper.replace(offset);
            d().set_from_copy(helper.reserve_for_copy, helper.total_length);
        }
        return d();
    }

    template<StrType<K> From>
    Impl& replace_from(const From& f, simple_str pattern, simple_str repl, size_t offset = 0, size_t maxCount = 0) {
        if (pattern.length() >= repl.length()) {
            K* dst = d().reserve_no_preserve(f.length());
            const K* src = f.symbols();
            size_t delta = 0;
            if (maxCount == 0) {
                maxCount--;
            }
            size_t src_length = f.length(), start = 0;
            while (maxCount--) {
                offset = f.find(pattern, offset);
                if (offset == str::npos) {
                    break;
                }
                size_t piece_len = offset - start;
                if (piece_len) {
                    ch_traits<K>::copy(dst, src + start, piece_len);
                    dst += piece_len;
                }
                if (repl.length()) {
                    ch_traits<K>::copy(dst, repl.symbols(), repl.length());
                    dst += repl.length();
                }
                delta += pattern.length() - repl.length();
                offset += pattern.length();
                start = offset;
            }
            if (start < src_length) {
                ch_traits<K>::copy(dst, src + start, src_length - start);
            }
            d().set_size(src_length - delta);
        } else {
            d() = f;
            replace(pattern, repl, offset, maxCount);
        }
        return d();
    }

    // Реализация заполнения данными с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
    my_type& fill(size_t from, const Op& fillFunction) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        capacity -= from;
        for (;;) {
            size_t needSize = (size_t)fillFunction(ptr + from, capacity);
            if (capacity >= needSize) {
                d().set_size(from + needSize);
                break;
            }
            ptr = from == 0 ? d().reserve_no_preserve(needSize) : d().set_size(from + needSize);
            capacity = d().capacity() - from;
        }
        return d();
    }
    // Реализация заполнения данными с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    my_type& operator<<(const Op& fillFunction) {
        return fill(0, fillFunction);
    }
    // Реализация добавления данных с проверкой на длину и перевыделением буфера в случае недостаточной длины.
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    my_type& operator<<=(const Op& fillFunction) {
        return fill(_len(), fillFunction);
    }
    template<typename Op>
        requires std::is_invocable_v<Op, my_type&>
    my_type& operator<<(const Op& fillFunction) {
        fillFunction(d());
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& printf_from(size_t from, const K* format, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        capacity -= from;

        int result = 0;
        // Тут грязный хак для u8s и wide_char. u8s версия snprintf сразу возвращает размер нужного буфера, если он мал
        // а swprintf - возвращает -1. Под windows оба варианта xxx_p - тоже возвращают -1.
        // Поэтому для них надо тупо увеличивать буфер наугад, пока не подойдет
        if constexpr (sizeof(K) == 1 && !isWindowsOs) {
            result = printf_selector::snprintf(ptr + from, capacity + 1, format, std::forward<T>(args)...);
            if (result > (int)capacity) {
                ptr = from == 0 ? d().reserve_no_preserve(result) : d().set_size(from + result);
                result = printf_selector::snprintf(ptr + from, result + 1, format, std::forward<T>(args)...);
            }
        } else {
            for (;;) {
                result = printf_selector::snprintf(ptr + from, capacity + 1, format, std::forward<T>(args)...);
                if (result < 0) {
                    // Не хватило буфера или ошибка конвертации.
                    // Попробуем увеличить буфер в два раза
                    capacity *= 2;
                    ptr = from == 0 ? d().reserve_no_preserve(capacity) : d().set_size(from + capacity);
                } else
                    break;
            }
        }
        if (result < 0)
            d().set_size(static_cast<size_t>(traits::length(_str())));
        else
            d().set_size(from + result);
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& printf(const K* pattern, T&&... args) {
        return printf_from(0, pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_printf(const K* pattern, T&&... args) {
        return printf_from(_len(), pattern, std::forward<T>(args)...);
    }

    struct writer {
        my_type* store;
        K* ptr;
        const K* end;
        size_t max_write;
        size_t writed = 0;
        inline static K pad;
        K& operator*() const {
            return *ptr;
        }
        writer& operator++() {
            if (writed < max_write) {
                ++ptr;
                if (ptr == end) {
                    size_t l = ptr - store->begin();
                    store->set_size(l);
                    ptr = store->set_size(l + std::min(l / 2, size_t(8192))) + l;
                    end = store->end();
                }
            } else {
                ptr = &pad;
            }
            return *this;
        }
        writer operator++(int) {
            auto ret = *this;
            operator++();
            return ret;
        }

        writer(my_type& s, K* p, K* e, size_t ml) : store(&s), ptr(p), end(e), max_write(ml) {}
        writer() = default;
        writer(const writer&) = delete;
        writer& operator=(const writer&) noexcept = delete;
        writer(writer&&) noexcept = default;
        writer& operator=(writer&&) noexcept = default;
        using difference_type = int;
    };

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& format_from(size_t from, const FmtString<K, T...>& pattern, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        
        auto result = std::format_to(writer{d(), ptr + from, ptr + capacity, size_t(-1)},
            std::forward<decltype(pattern)>(pattern), std::forward<T>(args)...);
        d().set_size(result.ptr - _str());
        return d();
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat_from(size_t from, size_t max_write, simple_str pattern, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();

        if constexpr (std::is_same_v<K, u8s>) {
            auto result = std::vformat_to(
                writer{d(), ptr + from, ptr + capacity, max_write},
                std::basic_string_view<K>{pattern.symbols(), pattern.length()},
                std::make_format_args(args...));
            d().set_size(result.ptr - _str());
        } else {
            auto result = std::vformat_to(
                writer{d(), to_one_of_std_char(ptr + from), ptr + capacity, max_write},
                std::basic_string_view<wchar_t>{to_one_of_std_char(pattern.symbols()), pattern.length()},
                std::make_wformat_args(std::forward<T>(args)...));
            d().set_size(result.ptr - _str());
        }
        return d();
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& format(const FmtString<K, T...>& pattern, T&&... args) {
        return format_from(0, pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_formatted(const FmtString<K, T...>& pattern, T&&... args) {
        return format_from(_len(), pattern, std::forward<T>(args)...);
    }
    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat(simple_str pattern, T&&... args) {
        return vformat_from(0, -1, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_vformatted(simple_str pattern, T&&... args) {
        return vformat_from(_len(), -1, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& vformat_n(size_t max_write, simple_str pattern, T&&... args) {
        return vformat_from(0, max_write, pattern, std::forward<T>(args)...);
    }

    template<typename... T> requires (is_one_of_std_char_v<K>)
    my_type& append_vformatted_n(size_t max_write, simple_str pattern, T&&... args) {
        return vformat_from(_len(), max_write, pattern, std::forward<T>(args)...);
    }

    template<typename Op, typename... Args>
    my_type& with(const Op& fillFunction, Args&&... args) {
        fillFunction(d(), std::forward<Args>(args)...);
        return d();
    }
};

template<typename K>
struct SharedStringData {
    std::atomic_size_t ref_; // Счетчик ссылок

    SharedStringData() {
        ref_ = 1;
    }
    K* str() const {
        return (K*)(this + 1);
    }
    void incr() {
        ref_.fetch_add(1, std::memory_order_relaxed);
    }
    void decr(Allocatorable auto& allocator) {
        size_t val = ref_.fetch_sub(1, std::memory_order_relaxed);
        if (val == 1) {
            allocator.deallocate(this);
        }
    }
    static SharedStringData<K>* create(size_t l, Allocatorable auto& allocator) {
        size_t size = sizeof(SharedStringData<K>) + (l + 1) * sizeof(K);
        return new (allocator.allocate(size)) SharedStringData();
    }
    static SharedStringData<K>* from_str(const K* p) {
        return (SharedStringData<K>*)p - 1;
    }
    K* place(K* p, size_t len) {
        ch_traits<K>::copy(p, str(), len);
        return p + len;
    }
};

// Дефолтный аллокатор для строк, может работать статически
class string_common_allocator {
public:
    void* allocate(size_t bytes) {
        return new char[bytes];
    }
    void deallocate(void* address) noexcept {
        delete [] static_cast<char*>(address);
    }
};

string_common_allocator default_string_allocator_selector(...);
// Если вы хотите задать свой дефолтный аллокатор для строк, перед включение sstring.h
// объявите функцию
// ваш_тип_аллокатора default_string_allocator_selector(int);
using allocator_string = decltype(default_string_allocator_selector(int(0)));

/*
* Локальная строка. Хранит в себе указатель на данные и длину строки, а за ней либо сами данные до N символов + нуль,
* либо если данные длиннее N, то размер выделенного буфера.
* При этом, если планируется потом результат переместить в sstring, то для динамического буфера
* выделяется +n байтов, чтобы потом не двигать данные.
* Так как у класса несколько базовых классов, MSVC не применяет автоматом empty base optimization,
* и без явного указания - вставит в начало класса пустые байты, сдвинув поле size на 4 байта.
* Укажем ему явно
*/
template<typename K, size_t N, bool forShared = false, Allocatorable Allocator = allocator_string>
class empty_bases lstring :
    public str_algs<K, simple_str<K>, lstring<K, N, forShared, Allocator>>,
    public str_mutable<K, simple_str<K>, lstring<K, N, forShared, Allocator>>,
    public str_storable<K, lstring<K, N, forShared, Allocator>, Allocator>,
    public from_utf_convertable<K, lstring<K, N, forShared, Allocator>> {
public:
    using symb_type = K;
    using my_type = lstring<K, N, forShared, Allocator>;
    using allocator_t = Allocator;

    enum : size_t { LocalCapacity = N | (sizeof(void*) / sizeof(K) - 1), AlocAlign = (alignof(std::max_align_t) / sizeof(K)) - 1};

protected:
    enum : size_t {
        extra = forShared ? sizeof(SharedStringData<K>) : 0,
    };

    using base_algs = str_algs<K, simple_str<K>, my_type>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_mutable = str_mutable<K, simple_str<K>, my_type>;
    using base_utf = from_utf_convertable<K, my_type>;
    using traits = ch_traits<K>;

    friend base_storable;
    friend base_mutable;
    friend base_utf;
    friend class sstring<K, Allocator>;

    // Данные
    K* data_;
    size_t size_; // Поле не должно инициализироваться, так как может устанавливаться в базовых конструкторах

    union {
        size_t capacity_; // Поле не должно инициализироваться, так как может устанавливаться в базовых конструкторах
        K local_[LocalCapacity + 1];
    };

    void create_empty() {
        data_ = local_;
        size_ = 0;
        local_[0] = 0;
    }
    static size_t calc_capacity(size_t s) {
        size_t real_need = (s + 1) * sizeof(K) + extra;
        size_t aligned_alloced = (real_need + alignof(std::max_align_t) - 1) / alignof(std::max_align_t) * alignof(std::max_align_t);
        return (aligned_alloced - extra) / sizeof(K) - 1;
    }

    K* init(size_t s) {
        size_ = s;
        if (size_ > LocalCapacity) {
            s = calc_capacity(s);
            data_ = alloc_place(s);
            capacity_ = s;
        } else {
            data_ = local_;
        }
        return str();
    }
    // Методы для себя
    bool is_alloced() const noexcept {
        return data_ != local_;
    }

    void dealloc() {
        if (is_alloced()) {
            base_storable::allocator().deallocate(to_real_address(data_));
            data_ = local_;
        }
    }

    static K* to_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) - extra);
    }
    static K* from_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) + extra);
    }

    K* alloc_place(size_t newSize) {
        return from_real_address(base_storable::allocator().allocate((newSize + 1) * sizeof(K) + extra));
    }
    // Вызывается при replace, когда меняют на более длинную замену
    K* alloc_for_copy(size_t newSize) {
        if (capacity() >= newSize) {
            // Замена войдёт в текущий буфер
            return data_;
        }
        return alloc_place(calc_capacity(newSize));
    }
    // Вызывается после replace, когда меняли на более длинную замену, могли скопировать в новый буфер
    void set_from_copy(K* ptr, size_t newSize) {
        if (ptr != data_) {
            // Да, копировали в новый буфер
            dealloc();
            data_ = ptr;
            capacity_ = calc_capacity(newSize);
        }
        size_ = newSize;
        data_[newSize] = 0;
    }

public:
    using base_storable::base_storable;
    using base_utf::base_utf;

    lstring() = default;

    ~lstring() {
        dealloc();
    }

    // Копирование из другой строки
    lstring(const my_type& other) : base_storable(other.allocator()) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }
    // Копирование из другой строки но с другим аллокатором
    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_convertible_v<allocator_t, Args...>)
    lstring(const my_type& other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }

    // Конструктор из строкового литерала
    template<typename T, size_t I = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(T&& value, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if constexpr (I > 1) {
            K* ptr = init(I - 1);
            traits::copy(ptr, value, I - 1);
            ptr[I - 1] = 0;
        } else
            create_empty();
    }

    // Перемещение из другой строки
    lstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        if (other.size_) {
            size_ = other.size_;
            if (other.is_alloced()) {
                data_ = other.data_;
                capacity_ = other.capacity_;
            } else {
                data_ = local_;
                traits::copy(local_, other.local_, size_ + 1);
            }
            other.data_ = other.local_;
            other.size_ = 0;
            other.local_[0] = 0;
        }
    }

    template<typename Op, typename... Args>
        requires(std::is_constructible_v<Allocator, Args...> && (std::is_invocable_v<Op, my_type&> || std::is_invocable_v<Op, K*, size_t>))
    lstring(const Op& op, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        this->operator<<(op);
    }

    // copy and swap для присваиваний здесь не очень применимо, так как для строк с большим локальным буфером лишняя копия даже перемещением будет дорого стоить
    // Поэтому реализуем копирующее и перемещающее присваивание отдельно
    my_type& operator=(const my_type& other) {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        if (&other != this) {
            traits::copy(reserve_no_preserve(other.size_), other.data_, other.size_ + 1);
            size_ = other.size_;
        }
        return *this;
    }

    my_type& operator=(my_type&& other) noexcept {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        if (&other != this) {
            dealloc();
            if (other.is_alloced()) {
                data_ = other.data_;
                capacity_ = other.capacity_;
            } else {
                traits::copy(data_, other.local_, other.size_ + 1);
            }
            base_storable::allocator() = std::move(other.allocator());
            size_ = other.size_;
            other.create_empty();
        }
        return *this;
    }

    my_type& assign(const K* other, size_t len) {
        if (len) {
            bool isIntersect = other >= data_ && other + len <= data_ + size_;
            if (isIntersect) {
                // Особый случай, нам пытаются присвоить кусок нашей же строки.
                // Просто переместим текст в буфере, и установим новый размер
                if (other > data_) {
                    traits::move(data_, other, len);
                }
            } else {
                traits::copy(reserve_no_preserve(len), other, len);
            }
        }
        size_ = len;
        data_[size_] = 0;
        return *this;
    }

    my_type& operator=(simple_str<K> other) {
        return assign(other.str, other.len);
    }

    template<typename T, size_t S = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return assign(other, S - 1);
    }

    // Если в строковом выражении что-либо ссылается на части этой же строки, то результат не определён
    my_type& operator=(const StrExprForType<K> auto& expr) {
        size_t newLen = expr.length();
        if (newLen) {
            expr.place(reserve_no_preserve(newLen));
        }
        size_ = newLen;
        data_[size_] = 0;
        return *this;
    }

    size_t length() const noexcept {
        return size_;
    }

    const K* symbols() const noexcept {
        return data_;
    }

    K* str() noexcept {
        return data_;
    }

    bool is_empty() const noexcept {
        return size_ == 0;
    }
    // for std::string compatibility
    bool empty() const noexcept {
        return size_ == 0;
    }

    size_t capacity() const noexcept {
        return is_alloced() ? capacity_ : LocalCapacity;
    }

    const K* data() const {
        return data_;
    }
    K* data() {
        return data_;
    }

    // Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
    // Содержимое буфера не определено, и не гарантируется сохранение старого содержимого.
    K* reserve_no_preserve(size_t newSize) {
        if (newSize > capacity()) {
            newSize = calc_capacity(newSize);
            K* newData = alloc_place(newSize);
            dealloc();
            data_ = newData;
            capacity_ = newSize;
        }
        return data_;
    }

    // Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
    // Содержимое строки сохраняется. При увеличении буфера размер выделяется не больше запрошенного.
    K* reserve(size_t newSize) {
        if (newSize > capacity()) {
            newSize = calc_capacity(newSize);
            K* newData = alloc_place(newSize);
            traits::copy(newData, data_, size_);
            dealloc();
            data_ = newData;
            capacity_ = newSize;
        }
        return data_;
    }

    // Устанавливает размер текущей строки. При необходимости перемещает данные в другой буфер
    // Содержимое сохраняется. При увеличении буфера размер выделяется не менее чем 2 старого размера буфера.
    K* set_size(size_t newSize) {
        size_t cap = capacity();
        if (newSize > cap) {
            size_t needPlace = newSize;
            if (needPlace < (cap + 1) * 2) {
                needPlace = (cap + 1) * 2 - 1;
            }
            reserve(needPlace);
        }
        size_ = newSize;
        data_[newSize] = 0;
        return data_;
    }

    size_t resize(size_t newSize, bool force = false) {
        if (newSize > capacity())
            set_size(newSize);
        if (force) {
            size_ = newSize;
            data_[newSize] = 0;
        }
        return length();
    }

    bool is_local() const noexcept {
        return !is_alloced();
    }

    void define_size() {
        size_t cap = capacity();
        for (size_t i = 0; i < cap; i++) {
            if (data_[i] == 0) {
                size_ = i;
                return;
            }
        }
        size_ = cap;
        data_[size_] = 0;
    }

    void shrink_to_fit() {
        size_t need_capacity = calc_capacity(size_);
        if (is_alloced() && capacity_ > need_capacity) {
            K* newData = size_ <= LocalCapacity ? local_ : alloc_place(need_capacity);
            traits::copy(newData, data_, size_ + 1);
            base_storable::allocator().deallocate(to_real_address(data_));
            data_ = newData;

            if (size_ > LocalCapacity) {
                capacity_ = need_capacity;
            }
        }
    }

    void clear() {
        set_size(0);
    }

    void reset() {
        dealloc();
        local_[0] = 0;
        size_ = 0;
    }
};

template<size_t N = 15>
using lstringa = lstring<u8s, N>;
template<size_t N = 15>
using lstringw = lstring<wchar_t, N>;
template<size_t N = 15>
using lstringu = lstring<u16s, N>;
template<size_t N = 15>
using lstringuu = lstring<u32s, N>;

template<size_t N = 15>
using lstringsa = lstring<u8s, N, true>;
template<size_t N = 15>
using lstringsw = lstring<wchar_t, N, true>;
template<size_t N = 15>
using lstringsu = lstring<u16s, N, true>;
template<size_t N = 15>
using lstringsuu = lstring<u32s, N, true>;


template<typename T, typename K = typename const_lit<T>::symb_type>
auto getLiteralType(T&&) {
    return K{};
};

template<size_t Arch, size_t L>
inline constexpr const size_t _local_count = 0;

template<>
inline constexpr const size_t _local_count<8, 1> = 23;
template<>
inline constexpr const size_t _local_count<8, 2> = 15;
template<>
inline constexpr const size_t _local_count<8, 4> = 7;
template<>
inline constexpr const size_t _local_count<4, 1> = 15;
template<>
inline constexpr const size_t _local_count<4, 2> = 11;
template<>
inline constexpr const size_t _local_count<4, 4> = 5;

template<typename T>
constexpr const size_t local_count = _local_count<sizeof(size_t), sizeof(T)>;

/*
* Класс с small string optimization плюс разделяемый иммутабельный буфер строки.
* Так как буфер строки в этом классе иммутабельный, то:
* Во-первых, нет нужды хранить размер выделенного буфера, мы его всё-равно не будем изменять
* Во-вторых, появляется ещё один тип строки - строка, инициализированная константным литералом.
* Для неё просто сохраняем указатель на символы, и не считаем ссылки.
* Таким образом, инициализация строкового объекта в программе литералом - ничего никуда не копирует -
* ни в себя, ни в динамическую память, и не стоит дороже по сравнению с инициализацией
* сырого указателя на строку, и даже ещё оптимальнее, так как ещё и сразу подставляет размер,
* а не вычисляет его в рантайме.
*
*     stringa text = "text or very very very long text"; // ничего не стоит!
*     stringa copy = anotherString; // Стоит только копирование байтов самого объекта плюс возможно один атомарный инкремент
*
* В случае разделяемого буфера размер строки всё-равно храним не в общем буфере, а в каждом объекте
* из-за SSO места всё-равно хватает, а в память лезть за длиной придётся меньше.
* Например, подсчитать сумму длин строк в векторе - пройдётся только по памяти в векторе.
*
* Размеры для x64:
* для u8s  - 24 байта, хранит строки до 23 символов + 0
* для u16s - 32 байта, хранит строки до 15 символов + 0
* для u32s - 32 байта, хранит строки до 7 символов + 0
*/

template<typename K, Allocatorable Allocator = allocator_string>
class empty_bases sstring :
    public str_algs<K, simple_str<K>, sstring<K, Allocator>>,
    public str_storable<K, sstring<K, Allocator>, Allocator>,
    public from_utf_convertable<K, sstring<K, Allocator>> {
public:
    using symb_type = K;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = sstring<K, Allocator>;
    using allocator_t = Allocator;

    enum { LocalCount = local_count<K> };

protected:
    using base_algs = str_algs<K, simple_str<K>, my_type>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_utf = from_utf_convertable<K, my_type>;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;

    friend base_storable;
    friend base_utf;

    enum Types { Local, Constant, Shared };

    union {
        // Когда у нас короткая строка, она лежит в самом объекте, а в localRemain
        // пишется, сколько символов ещё можно вписать. Когда строка занимает всё
        // возможное место, то localRemain становится 0, type в этом случае тоже 0,
        // и в итоге после символов строки получается 0, как и надо!
        struct {
            K buf_[LocalCount]; // Локальный буфер строки
            uns_type localRemain_ : sizeof(uns_type) * 8 - 2;
            uns_type type_ : 2;
        };
        struct {
            union {
                const K* cstr_; // Указатель на конcтантную строку
                const K* sstr_; // Указатель на строку, перед которой лежит SharedStringData
            };
            size_t bigLen_; // Длина не локальной строки.
        };
    };

    void create_empty() {
        type_ = Local;
        localRemain_ = LocalCount;
        buf_[0] = 0;
    }
    K* init(size_t s) {
        if (s > LocalCount) {
            type_ = Shared;
            localRemain_ = 0;
            bigLen_ = s;
            sstr_ = SharedStringData<K>::create(s, base_storable::allocator())->str();
            return (K*)sstr_;
        } else {
            type_ = Local;
            localRemain_ = LocalCount - s;
            return buf_;
        }
    }

    K* set_size(size_t newSize) {
        // Вызывается при создании строки при необходимости изменить размер.
        // Других ссылок на shared buffer нет.
        size_t size = length();
        if (newSize != size) {
            if (type_ == Constant) {
                bigLen_ = newSize;
            } else {
                if (newSize <= LocalCount) {
                    if (type_ == Shared) {
                        SharedStringData<K>* str_buf = SharedStringData<K>::from_str(sstr_);
                        traits::copy(buf_, sstr_, newSize);
                        str_buf->decr(base_storable::allocator());
                    }
                    type_ = Local;
                    localRemain_ = LocalCount - newSize;
                } else {
                    if (type_ == Shared) {
                        if (newSize > size || (newSize > 64 && newSize <= size * 3 / 4)) {
                            K* newStr = SharedStringData<K>::create(newSize, base_storable::allocator())->str();
                            traits::copy(newStr, sstr_, newSize);
                            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
                            sstr_ = newStr;
                        }
                    } else if (type_ == Local) {
                        K* newStr = SharedStringData<K>::create(newSize, base_storable::allocator())->str();
                        if (size)
                            traits::copy(newStr, buf_, size);
                        sstr_ = newStr;
                        type_ = Shared;
                        localRemain_ = 0;
                    }
                    bigLen_ = newSize;
                }
            }
        }
        K* str = type_ == Local ? buf_ : (K*)sstr_;
        str[newSize] = 0;
        return str;
    }

public:
    using base_storable::base_storable;
    using base_utf::base_utf;

    sstring() = default;

    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_constructible_v<Allocator, Args...>)
    sstring(Args&&... args) : Allocator(std::forward<Args>(args)...) {}

    static const sstring<K> empty_str;

    ~sstring() {
        if (type_ == Shared) {
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        }
    }

    sstring(const my_type& other) noexcept : base_storable(other.allocator()) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->incr();
    }

    sstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        other.create_empty();
    }

    // Конструктор перемещения из локальной строки
    template<size_t N>
    sstring(lstring<K, N, true, Allocator>&& src) : base_storable(std::move(src.allocator())) {
        size_t size = src.length();
        if (size) {
            if (src.is_alloced()) {
                // Там динамический буфер, выделенный с запасом для SharedStringData.
                K* str = src.str();
                if (size > LocalCount) {
                    // Просто присвоим его себе.
                    sstr_ = str;
                    bigLen_ = size;
                    type_ = Shared;
                    localRemain_ = 0;
                    new (SharedStringData<K>::from_str(str)) SharedStringData<K>();
                } else {
                    // Скопируем локально
                    type_ = Local;
                    localRemain_ = LocalCount - size;
                    traits::copy(buf_, str, size + 1);
                    // Освободим тот буфер, у локальной строки буфер не разделяется с другими
                    src.dealloc();
                }
            } else {
                // Копируем из локального буфера
                K* str = init(src.size_);
                traits::copy(str, src.symbols(), size + 1);
            }
            src.create_empty();
        } else
            create_empty();
    }

    // Инициализация из строкового литерала
    template<typename T, size_t N = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(T&& s, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        type_ = Constant;
        localRemain_ = 0;
        cstr_ = s;
        bigLen_ = N - 1;
    }

    void swap(my_type&& other) noexcept {
        char buf[sizeof(buf_) + sizeof(K)];
        memcpy(buf, buf_, sizeof(buf));
        memcpy(buf_, other.buf_, sizeof(buf));
        memcpy(other.buf_, buf, sizeof(buf));

        std::swap(base_storable::allocator(), other.allocator());
    }

    my_type& operator=(my_type other) noexcept {
        swap(std::move(other));
        return *this;
    }

    my_type& operator=(simple_str<K> other) {
        return operator=(my_type{other, base_storable::allocator()});
    }

    template<typename T, size_t N = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return operator=(my_type{other, base_storable::allocator()});
    }

    template<size_t N, bool forShared, typename A>
    my_type& operator=(const lstring<K, N, forShared, A>& other) {
        return operator=(my_type{other.to_str(), base_storable::allocator()});
    }

    template<size_t N>
    my_type& operator=(lstring<K, N, true, Allocator>&& other) {
        return operator=(my_type{std::move(other)});
    }

    my_type& operator=(const StrExprForType<K> auto& expr) {
        return operator=(my_type{expr, base_storable::allocator()});
    }

    my_type& make_empty() noexcept {
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        create_empty();
        return *this;
    }

    const K* symbols() const noexcept {
        return type_ == Local ? buf_ : cstr_;
    }

    size_t length() const noexcept {
        return type_ == Local ? LocalCount - localRemain_ : bigLen_;
    }

    bool is_empty() const noexcept {
        return length() == 0;
    }
    // for std::string compatibility
    bool empty() const noexcept {
        return is_empty();
    }
    // Форматирование строки.
    template<typename... T>
    static my_type printf(const K* pattern, T&&... args) {
        return my_type{lstring<K, 256, true>{}.printf(pattern, std::forward<T>(args)...)};
    }

    template<typename... T>
    static my_type format(const FmtString<K, T...>& fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.format(fmtString, std::forward<T>(args)...)};
    }
    template<typename... T>
    static my_type vformat(simple_str<K> fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.vformat(fmtString, std::forward<T>(args)...)};
    }
};

template<typename K, Allocatorable Allocator>
inline const sstring<K> sstring<K, Allocator>::empty_str{};

template<size_t I>
struct digits_selector {
    using wider_type = uint16_t;
};

template<>
struct digits_selector<2> {
    using wider_type = uint32_t;
};

template<>
struct digits_selector<4> {
    using wider_type = uint64_t;
};

template<typename K, typename T>
constexpr size_t fromInt(K* bufEnd, T val) {
    const char* twoDigit =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    if (val) {
        need_sign<K, std::is_signed_v<T>, T> sign(val);
        K* itr = bufEnd;
        // Когда у нас минимальное отрицательное число, оно не меняется и остается меньше нуля
        if constexpr (std::is_signed_v<T>) {
            if (val < 0) {
                // Возьмем две последние цифры
                const char* ptr = twoDigit - (val % 100) * 2;
                *--itr = static_cast<K>(ptr[1]);
                *--itr = static_cast<K>(ptr[0]);
                val /= 100;
                val = -val;
            }
        }
        while (val >= 100) {
            const char* ptr = twoDigit + (val % 100) * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
            val /= 100;
        }
        if (val < 10) {
            *--itr = static_cast<K>('0' + val);
        } else {
            const char* ptr = twoDigit + val * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
        }
        sign.after(itr);
        return size_t(bufEnd - itr);
    }
    bufEnd[-1] = '0';
    return 1;
}

template<typename K, typename T>
struct expr_num {
    using symb_type = K;
    using my_type = expr_num<K, T>;

    enum { bufSize = 24 };
    mutable T value;
    mutable K buf[bufSize];

    expr_num(T t) : value(t) {}
    expr_num(expr_num<K, T>&& t) : value(t.value) {}

    size_t length() const noexcept {
        value = (T)fromInt(buf + bufSize, value);
        return (size_t)value;
    }
    K* place(K* ptr) const noexcept {
        ch_traits<K>::copy(ptr, buf + bufSize - (size_t)value, (size_t)value);
        return ptr + (size_t)value;
    }
};

template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (const A& a, T s) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>>{a, s};
}

template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (T s, const A& a) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>, false>{a, s};
}

template<typename K, typename T>
inline constexpr auto e_num(T t) {
    return expr_num<K, T>{t};
}

template<typename K>
simple_str_nt<K> select_str(simple_str_nt<u8s> s8, simple_str_nt<uws> sw, simple_str_nt<u16s> s16, simple_str_nt<u32s> s32) {
    if constexpr (std::is_same_v<K, u8s>)
        return s8;
    if constexpr (std::is_same_v<K, uws>)
        return sw;
    if constexpr (std::is_same_v<K, u16s>)
        return s16;
    if constexpr (std::is_same_v<K, u32s>)
        return s32;
}

#define uni_string(K, p) select_str<K>(p, L##p, u##p, U##p)

template<typename K> requires (is_one_of_std_char_v<K>)
struct expr_real {
    using symb_type = K;
    mutable K buf[40];
    mutable size_t l;
    double v;
    expr_real(double d) : v(d) {}
    expr_real(float d) : v(d) {}

    size_t length() const noexcept {
        printf_selector::snprintf(buf, 40, uni_string(K, "%.16g").str, v);
        l = (size_t)ch_traits<K>::length(buf);
        return l;
    }
    K* place(K* ptr) const noexcept {
        ch_traits<K>::copy(ptr, buf, l);
        return ptr + l;
    }
};

template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && (std::is_same_v<R, double> || std::is_same_v<R, float>))
inline constexpr auto operator+(const A& a, R s) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>>{a, s};
}

template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && (std::is_same_v<R, double> || std::is_same_v<R, float>))
inline constexpr auto operator+(R s, const A& a) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>, false>{a, s};
}

template<typename K> requires(is_one_of_std_char_v<K>)
inline constexpr auto e_real(double t) {
    return expr_real<K>{t};
}

/*
* Для создания строковых конкатенаций с векторами и списками, сджойненными константным разделителем
* K - тип символов строки
* T - тип контейнера строк (vector, list)
* I - длина разделителя в символах
* tail - добавлять разделитель после последнего элемента контейнера.
*        Если контейнер пустой, разделитель в любом случае не добавляется
*/

template<typename K, typename T, size_t I, bool tail, bool only_not_empty>
struct expr_join {
    using symb_type = K;
    using my_type = expr_join<K, T, I, tail, only_not_empty>;

    const T& s;
    const K* delim;

    constexpr size_t length() const noexcept {
        size_t l = 0;
        for (const auto& t: s) {
            size_t len = t.length();
            if (len > 0 || !only_not_empty) {
                if (I > 0 && l > 0) {
                    l += I;
                }
                l += len;
            }
        }
        return l + (tail && I > 0 && (l > 0 || (!only_not_empty && s.size() > 0))? I : 0);
    }
    constexpr K* place(K* ptr) const noexcept {
        if (s.empty()) {
            return ptr;
        }
        K* write = ptr;
        for (const auto& t: s) {
            size_t copyLen = t.length();
            if (I > 0 && write != ptr && (copyLen || !only_not_empty)) {
                ch_traits<K>::copy(write, delim, I);
                write += I;
            }
            if (copyLen) {
                ch_traits<K>::copy(write, t.symbols(), copyLen);
                write += copyLen;
            }
        }
        if (I > 0 && tail && (write != ptr || (!only_not_empty && s.size() > 0))) {
            ch_traits<K>::copy(write, delim, I);
            write += I;
        }
        return write;
    }
};

template<bool tail = false, bool only_not_empty = false, typename L, typename K = typename const_lit<L>::symb_type, size_t I = const_lit<L>::Count, typename T>
inline constexpr auto e_join(const T& s, L&& d) {
    return expr_join<K, T, I - 1, tail, only_not_empty>{s, d};
}

template<typename K, size_t N, size_t L>
struct expr_replaces {
    using symb_type = K;
    using my_type = expr_replaces<K, N, L>;
    simple_str<K> what;
    const K* pattern;
    const K* repl;
    mutable size_t first_, last_;

    constexpr expr_replaces(simple_str<K> w, const K* p, const K* r) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length();
        if constexpr (N == L) {
            return l;
        }
        first_ = what.find(pattern, N, 0);
        if (first_ != str::npos) {
            last_ = first_ + N;
            for (;;) {
                l += L - N;
                size_t next = what.find(pattern, N, last_);
                if (next == str::npos) {
                    break;
                }
                last_ = next + N;
            }
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        if constexpr (N == L) {
            const K* from = what.symbols();
            for (size_t start = 0; start < what.length();) {
                size_t next = what.find(pattern, N, start);
                if (next == str::npos) {
                    next = what.length();
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr,  from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl, L);
                ptr += L;
                start = next + N;
            }
            return ptr;
        }
        if (first_ == str::npos) {
            return what.place(ptr);
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = first_; ;) {
            ch_traits<K>::copy(ptr, from + start, offset - start);
            ptr += offset - start;
            ch_traits<K>::copy(ptr, repl, L);
            ptr += L;
            start = offset + N;
            if (start >= last_) {
                size_t tail = what.length() - last_;
                ch_traits<K>::copy(ptr, from + last_, tail);
                ptr += tail;
                break;
            } else {
                offset = what.find(pattern, N, start);
            }
        }
        return ptr;
    }
};

template<typename K, typename T, size_t N = const_lit_for<K, T>::Count, typename X, size_t L = const_lit_for<K, X>::Count>
    requires(N > 1)
inline constexpr auto e_repl(simple_str<K> w, T&& p, X&& r) {
    return expr_replaces<K, N - 1, L - 1>{w, p, r};
}

template<typename K>
struct expr_replaced {
    using symb_type = K;
    using my_type = expr_replaced<K>;
    simple_str<K> what;
    const simple_str<K> pattern;
    const simple_str<K> repl;
    mutable size_t first_, last_;

    constexpr expr_replaced(simple_str<K> w, simple_str<K> p, simple_str<K> r) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length();
        if (pattern.length() == repl.length()) {
            return l;
        }
        first_ = what.find(pattern);
        if (first_ != str::npos) {
            last_ = first_ + pattern.length();
            for (;;) {
                l += repl.length() - pattern.length();
                size_t next = what.find(pattern, last_);
                if (next == str::npos) {
                    break;
                }
                last_ = next + pattern.length();
            }
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        if (repl.length() == pattern.length()) {
            const K* from = what.symbols();
            for (size_t start = 0; start < what.length();) {
                size_t next = what.find(pattern, start);
                if (next == str::npos) {
                    next = what.length();
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr,  from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl.symbols(), repl.length());
                ptr += repl.length();
                start = next + pattern.length();
            }
            return ptr;
        }
        if (first_ == str::npos) {
            return what.place(ptr);
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = first_; ;) {
            ch_traits<K>::copy(ptr, from + start, offset - start);
            ptr += offset - start;
            ch_traits<K>::copy(ptr, repl.symbols(), repl.length());
            ptr += repl.length();
            start = offset + pattern.length();
            if (start >= last_) {
                size_t tail = what.length() - last_;
                ch_traits<K>::copy(ptr, from + last_, tail);
                ptr += tail;
                break;
            } else {
                offset = what.find(pattern, start);
            }
        }
        return ptr;
    }
};

template<bool UseVectorForReplace>
struct replace_search_result_store {
    size_t first_replace_ = str::npos;
    size_t first_idx_;
    size_t last_replace_;
};

template<>
struct replace_search_result_store<true> : std::vector<std::pair<size_t, size_t>> {};

// Строковое выражение для замены символов
template<typename K, bool UseVectorForReplace = true>
struct expr_replace_symbols {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;

    const simple_str<K> source_;
    const std::vector<std::pair<K, simple_str<K>>>& replaces_;

    lstring<K, 32> pattern_;

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    uu8s bit_mask_[sizeof(K) == 1 ? 32 : 64]{};

    constexpr expr_replace_symbols(simple_str<K> source, const std::vector<std::pair<K, simple_str<K>>>& repl )
        : source_(source), replaces_(repl)
    {
        size_t pattern_len = replaces_.size();
        K* pattern = pattern_.set_size(pattern_len);

        for (size_t idx = 0; idx < replaces_.size(); idx++) {
            *pattern++ = replaces_[idx].first;
        }

        if (pattern_len >= BIT_SEARCH_TRESHHOLD) {
            for (size_t idx = 0; idx < pattern_len; idx++) {
                uu8s s = static_cast<uu8s>(pattern_[idx]);
                if constexpr (sizeof(K) == 1) {
                    bit_mask_[s >> 3] |= (1 << (s & 7));
                } else {
                    if (std::make_unsigned_t<K>(pattern_[idx]) > 255) {
                        bit_mask_[32 + (s >> 3)] |= (1 << (s & 7));
                    } else {
                        bit_mask_[s >> 3] |= (1 << (s & 7));
                    }
                }
            }
        }
    }

    size_t length() const {
        size_t l = source_.length();
        auto [fnd, num] = find_first_of(source_.str, source_.len);
        if (fnd == str::npos) {
            return l;
        }
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            l = l - 1 + replaces_[num].second.len;
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l = l - 1 + replaces_[idx].second.len;
            }
        } else {
            l = l - 1 + replaces_[num].second.len;
            search_results_.first_replace_ = fnd;
            search_results_.first_idx_ = num;
            search_results_.last_replace_ = fnd + 1;
            for (;;) {
                auto [start, idx] = find_first_of(source_.str, source_.len, search_results_.last_replace_);
                if (start == str::npos) {
                    break;
                }
                search_results_.last_replace_ = start + 1;
                l = l - 1 + replaces_[idx].second.len;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        if constexpr (UseVectorForReplace) {
            size_t start = 0;
            const K* text = source_.str;
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].second.place(ptr);
                start = pos + 1;
            }
            if (size_t tail = source_.len - start; tail > 0) {
                ch_traits<K>::copy(ptr, text + start, tail);
                ptr += tail;
            }
        } else {
            if (search_results_.first_replace_ == -1) {
                return source_.place(ptr);
            }
            if (search_results_.first_replace_ > 0) {
                ch_traits<K>::copy(ptr, source_.str, search_results_.first_replace_);
                ptr += search_results_.first_replace_;
            }
            ptr = replaces_[search_results_.first_idx_].second.place(ptr);
            const K* text = source_.str;
            for (size_t start = search_results_.first_replace_ + 1; start < search_results_.last_replace_;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                size_t delta = fnd - start;
                if (delta) {
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                }
                ptr = replaces_[idx].second.place(ptr);
                start = fnd + 1;
            }
            if (size_t tail = source_.len - search_results_.last_replace_; tail > 0) {
                ch_traits<K>::copy(ptr, source_.str + search_results_.last_replace_, tail);
                ptr += tail;
            }
        }
        return ptr;
    }
    
protected:
    size_t index_of(K s) const {
        return pattern_.find(s);
    }

    bool is_in_mask(uu8s s) const {
        return (bit_mask_[s >> 3] & (1 << (s & 7))) != 0;
    }
    bool is_in_mask2(uu8s s) const {
        return (bit_mask_[32 + (s >> 3)] & (1 << (s & 7))) != 0;
    }

    bool is_in_pattern(K s, size_t& idx) const {
        if constexpr (sizeof(K) == 1) {
            if (is_in_mask(s)) {
                idx = index_of(s);
                return true;
            }
        } else {
            if (std::make_unsigned_t<const K>(s) > 255) {
                if (is_in_mask2(s)) {
                    return (idx = index_of(s)) != -1;
                }
            } else {
                if (is_in_mask(s)) {
                    idx = index_of(s);
                    return true;
                }
            }
        }
        return false;
    }

    std::pair<size_t, size_t> find_first_of(const K* text, size_t len,  size_t offset = 0) const {
        size_t pl = pattern_.length();
        if  (pl >= BIT_SEARCH_TRESHHOLD) {
            size_t idx;
            while (offset < len) {
                if (is_in_pattern(text[offset], idx)) {
                    return {offset, idx};
                }
                offset++;
            }
        } else {
            while (offset < len) {
                if (size_t idx = index_of(text[offset]); idx != -1) {
                    return {offset, idx};
                }
                offset++;
            }
        }
        return {-1, -1};
    }
};

// Строковое выражение для замены символов
template<typename K, size_t N, bool UseVectorForReplace>
struct expr_replace_const_symbols {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;
    const K pattern_[N];
    const simple_str<K> source_;
    const simple_str<K> replaces_[N];

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    [[_no_unique_address]]
    uu8s bit_mask_[N >= BIT_SEARCH_TRESHHOLD ? (sizeof(K) == 1 ? 32 : 64) : 0]{};

    template<typename ... Repl> requires (sizeof...(Repl) == N * 2)
    constexpr expr_replace_const_symbols(simple_str<K> source, Repl&& ... repl) : expr_replace_const_symbols(0, source, std::forward<Repl>(repl)...) {}

    size_t length() const {
        size_t l = source_.length();
        auto [fnd, num] = find_first_of(source_.str, source_.len);
        if (fnd == str::npos) {
            return l;
        }
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            l = l - 1 + replaces_[num].len;
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l = l - 1 + replaces_[idx].len;
            }
        } else {
            l = l - 1 + replaces_[num].len;
            search_results_.first_replace_ = fnd;
            search_results_.first_idx_ = num;
            search_results_.last_replace_ = fnd + 1;
            for (;;) {
                auto [start, idx] = find_first_of(source_.str, source_.len, search_results_.last_replace_);
                if (start == str::npos) {
                    break;
                }
                search_results_.last_replace_ = start + 1;
                l = l - 1 + replaces_[idx].len;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        if constexpr (UseVectorForReplace) {
            size_t start = 0;
            const K* text = source_.str;
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].place(ptr);
                start = pos + 1;
            }
            if (size_t tail = source_.len - start; tail > 0) {
                ch_traits<K>::copy(ptr, text + start, tail);
                ptr += tail;
            }
        } else {
            if (search_results_.first_replace_ == -1) {
                return source_.place(ptr);
            }
            if (search_results_.first_replace_ > 0) {
                ch_traits<K>::copy(ptr, source_.str, search_results_.first_replace_);
                ptr += search_results_.first_replace_;
            }
            ptr = replaces_[search_results_.first_idx_].place(ptr);
            const K* text = source_.str;
            for (size_t start = search_results_.first_replace_ + 1; start < search_results_.last_replace_;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                size_t delta = fnd - start;
                if (delta) {
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                }
                ptr = replaces_[idx].place(ptr);
                start = fnd + 1;
            }
            if (size_t tail = source_.len - search_results_.last_replace_; tail > 0) {
                ch_traits<K>::copy(ptr, source_.str + search_results_.last_replace_, tail);
                ptr += tail;
            }
        }
        return ptr;
    }
    
protected:
    template<typename ... Repl>
    constexpr expr_replace_const_symbols(int, simple_str<K> source, K s, simple_str<K> r, Repl&&... repl) :
        expr_replace_const_symbols(0, source, std::forward<Repl>(repl)..., std::make_pair(s, r)){}

    template<typename ... Repl> requires (sizeof...(Repl) == N)
    constexpr expr_replace_const_symbols(int, simple_str<K> source, Repl&&... repl) :
        source_(source), pattern_ {repl.first...}, replaces_{repl.second...}
    {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            for (size_t idx = 0; idx < N; idx++) {
                uu8s s = static_cast<uu8s>(pattern_[idx]);
                if constexpr (sizeof(K) == 1) {
                    bit_mask_[s >> 3] |= 1 << (s & 7);
                } else {
                    if (std::make_unsigned_t<const K>(pattern_[idx]) > 255) {
                        bit_mask_[32 + (s >> 3)] |= 1 << (s & 7);
                    } else {
                        bit_mask_[s >> 3] |= 1 << (s & 7);
                    }
                }
            }
        }
    }

    template<size_t Idx>
    size_t index_of(K s) const {
        if constexpr (Idx < N) {
            return pattern_[Idx] == s ? Idx : index_of<Idx + 1>(s);
        }
        return -1;
    }
    bool is_in_mask(uu8s s) const {
        return (bit_mask_[s >> 3] & (1 <<(s & 7))) != 0;
    }
    bool is_in_mask2(uu8s s) const {
        return (bit_mask_[32 + (s >> 3)] & (1 <<(s & 7))) != 0;
    }

    bool is_in_pattern(K s, size_t& idx) const {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            if constexpr (sizeof(K) == 1) {
                if (is_in_mask(s)) {
                    idx = index_of<0>(s);
                    return true;
                }
            } else {
                if (std::make_unsigned_t<const K>(s) > 255) {
                    if (is_in_mask2(s)) {
                        return (idx = index_of<0>(s)) != -1;
                    }
                } else {
                    if (is_in_mask(s)) {
                        idx = index_of<0>(s);
                        return true;
                    }
                }
            }
        }
        return false;
    }
    std::pair<size_t, size_t> find_first_of(const K* text, size_t len,  size_t offset = 0) const {
        if constexpr (N >= BIT_SEARCH_TRESHHOLD) {
            size_t idx;
            while (offset < len) {
                if (is_in_pattern(text[offset], idx)) {
                    return {offset, idx};
                }
                offset++;
            }
        } else {
            while (offset < len) {
                if (size_t idx = index_of<0>(text[offset]); idx != -1) {
                    return {offset, idx};
                }
                offset++;
            }
        }
        return {-1, -1};
    }
};

template<bool UseVector = true, typename K, typename ... Repl>
requires (sizeof...(Repl) % 2 == 0)
auto e_repl_const_symbols(simple_str<K> src, Repl&& ... other) {
    return expr_replace_const_symbols<K, sizeof...(Repl) / 2, UseVector>(src, std::forward<Repl>(other)...);
}

template<StrExpr A, StrExprForType<typename A::symb_type> B>
struct expr_choice {
    using symb_type = typename A::symb_type;
    using my_type = expr_choice<A, B>;
    const A& a;
    const B& b;
    bool choice;

    constexpr size_t length() const noexcept {
        return choice ? a.length() : b.length();
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        return choice ? a.place(ptr) : b.place(ptr);
    }
};

template<StrExpr A, StrExprForType<typename A::symb_type> B>
inline constexpr auto e_choice(bool c, const A& a, const B& b) {
    return expr_choice<A, B>{a, b, c};
}

template<typename K>
struct StoreType {
    simple_str<K> str;
    size_t hash;
    char node[sizeof(sstring<K>)];

    const simple_str_nt<K>& to_nt() const noexcept {
        return static_cast<const simple_str_nt<K>&>(str);
    }
    const sstring<K>& to_str() const noexcept {
        return *reinterpret_cast<const sstring<K>*>(node);
    }
};

using HashKeyA = StoreType<u8s>;
using HashKeyW = StoreType<u16s>;
using HashKeyU = StoreType<u32s>;

template<bool Wide>
struct fnv_const {
    static inline constexpr size_t basis = static_cast<size_t>(14695981039346656037ULL);
    static inline constexpr size_t prime = static_cast<size_t>(1099511628211ULL);
};

template<>
struct fnv_const<false> {
    static inline constexpr size_t basis = static_cast<size_t>(2166136261U);
    static inline constexpr size_t prime = static_cast<size_t>(16777619U);
};

using fnv = fnv_const<sizeof(size_t) == 8>;

template<typename K>
inline constexpr size_t fnv_hash(const K* ptr, size_t l) {
    size_t h = fnv::basis;
    for (size_t i = 0; i < l; i++) {
        h = (h ^ (std::make_unsigned_t<K>)ptr[i]) * fnv::prime;
    }
    return h;
};

template<typename K>
inline constexpr size_t fnv_hash_ia(const K* ptr, size_t l) {
    size_t h = fnv::basis;
    for (size_t i = 0; i < l; i++) {
        std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)ptr[i];
        h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
    }
    return h;
};

template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
inline constexpr size_t fnv_hash(T&& value) {
    size_t h = fnv::basis;
    for (size_t i = 0; i < N - 1; i++) {
        h = (h ^ (std::make_unsigned_t<K>)value[i]) * fnv::prime;
    }
    return h;
};

template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
inline constexpr size_t fnv_hash_ia(T&& value) {
    size_t h = fnv::basis;
    for (size_t i = 0; i < N - 1; i++) {
        std::make_unsigned_t<K> s = (std::make_unsigned_t<K>)value[i];
        h = (h ^ (s >= 'A' && s <= 'Z' ? s | 0x20 : s)) * fnv::prime;
    }
    return h;
};

template<typename K>
inline consteval size_t fnv_hash_compile(const K* ptr, size_t l) {
    return fnv_hash(ptr, l);
};

template<typename K>
inline consteval size_t fnv_hash_ia_compile(const K* ptr, size_t l) {
    return fnv_hash_ia(ptr, l);
};

constexpr bool tc = std::is_trivially_copyable_v<StoreType<u8s>>;

template<typename K>
struct streql;
template<typename K>
struct strhash;

/*
 * Контейнер для более эффективного поиска по строковым ключам. Как unordered_map, но чуть лучше.
 * В качестве ключей хранит simple_str вместе с посчитанным хешем и пустым местом для sstring.
 * После вставки создает в этом пустом месте sstring, чтобы simple_str было на что ссылаться.
 * Позволяет использовать для вставки и поиска любые simstr строковые объекты, создавая из них объект
 * sstring только при реальной вставке.
 * Начиная с С++20 в unordered_map появилась возможность для гетерогенного поиска по ключу с типом,
 * отличным от типа хранящегося ключа, но это требует определённых танцев с типами хэша и сравнения.
 * Поиск же со вставкой (try_emplace) по типу, отличному от типа ключа, появляется в стандарте только с C++26.
 * Имеется реализация хеширования и сравнения для вариантов:
 *  - с учётом регистра символов
 *  - без учёта регистра символов, только ASCII
 *  - без учёта регистра символов, упрощённый Unicode (до 0xFFFF).
 * Хэширование выполняется алгоритмом FNV-1A.
 * Автоматическая конвертация simstr строковых объектов в тип хранящегося ключа (т.е. с посчитанным хешем),
 * а также сохранение sstring в ключе при вставке - выполняется для методов try_emplace, emplace, at,
 * find, operator[], erase. При использовании других методов поиска вам нужно самим обеспечить правильно
 * посчитанный хэш в ключе StoreType. Методами вставки помимо перечисленных не пользоваться, они не
 * сохранят sstring в ключе.
 */
template<typename K, typename T, typename H = strhash<K>, typename E = streql<K>>
class hashStrMap : public std::unordered_map<StoreType<K>, T, H, E> {
protected:
    using InStore = StoreType<K>;

public:
    using my_type = hashStrMap<K, T, H, E>;
    using hash_t = std::unordered_map<InStore, T, H, E>;
    using hasher = H;

    hashStrMap() = default;
    hashStrMap(const my_type& other) : hash_t(other) {
        for (const auto& [k, v] : *this) {
            InStore& stored = const_cast<InStore&>(k);
            sstring<K> tmp = *(sstring<K>*)stored.node;
            new (stored.node) sstring<K>(std::move(tmp));
            stored.str.str = stored.to_str().symbols();
        }
    }
    ~hashStrMap() {
        for (auto& k: *this)
            ((sstring<K>*)k.first.node)->~sstring();
    }

    hashStrMap(my_type&& o) = default;
    
    my_type& operator=(const my_type& other) {
        hash_t::operator=(other);
        for (const auto& [k, v] : *this) {
            InStore& stored = const_cast<InStore&>(k);
            sstring<K> tmp = *(sstring<K>*)stored.node;
            new (stored.node) sstring<K>(std::move(tmp));
            stored.str.str = stored.to_str().symbols();
        }
        return *this;
    };
    my_type& operator=(my_type&&) = default;

    hashStrMap(std::initializer_list<std::pair<const StoreType<K>, T>>&& init) {
        for (const auto& e: init)
            emplace(e.first, e.second);
    }

    using init_str = std::initializer_list<std::pair<const sstring<K>, T>>;

    hashStrMap(init_str&& init) {
        for (const auto& e: init)
            emplace(e.first, e.second);
    }

    // При входе хэш должен быть уже посчитан
    template<typename... ValArgs>
    auto try_emplace(const StoreType<K>& key, ValArgs&&... args) {
        auto it = hash_t::try_emplace(key, std::forward<ValArgs>(args)...);
        if (it.second) {
            InStore& stored = const_cast<InStore&>(it.first->first);
            new (stored.node) sstring<K>(key.str);
            stored.str.str = stored.to_str().symbols();
        }
        return it;
    }

    static StoreType<K> toStoreType(simple_str<K> key) {
        return {key, H{}(key)};
    }

    template<typename Key, typename... ValArgs>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto try_emplace(Key&& key, ValArgs&&... args) {
        auto it = hash_t::try_emplace(toStoreType(key), std::forward<ValArgs>(args)...);
        if (it.second) {
            InStore& stored = const_cast<InStore&>(it.first->first);
            new (stored.node) sstring<K>(std::forward<Key>(key));
            stored.str.str = stored.to_str().symbols();
        }
        return it;
    }

    template<typename... ValArgs>
    auto emplace(const StoreType<K>& key, ValArgs&&... args) {
        auto it = try_emplace(key, std::forward<ValArgs>(args)...);
        if (!it.second) {
            it.first->second = T(std::forward<ValArgs>(args)...);
        }
        return it;
    }

    template<typename Key, typename... ValArgs>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto emplace(Key&& key, ValArgs&&... args) {
        auto it = try_emplace(std::forward<Key>(key), std::forward<ValArgs>(args)...);
        if (!it.second) {
            it.first->second = T(std::forward<ValArgs>(args)...);
        }
        return it;
    }

    template<typename Key>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto& operator[](Key&& key) {
        return try_emplace(std::forward<Key>(key)).first->second;
    }

    auto at(const StoreType<K>& key) {
        return hash_t::at(key);
    }
    auto at(const StoreType<K>& key) const {
        return hash_t::at(key);
    }

    auto at(simple_str<K> key) {
        return hash_t::at(toStoreType(key));
    }
    auto at(simple_str<K> key) const {
        return hash_t::at(toStoreType(key));
    }

    auto find(const StoreType<K>& key) const {
        return hash_t::find(key);
    }

    auto find(simple_str<K> key) const {
        return find(toStoreType(key));
    }

    auto find(const StoreType<K>& key) {
        return hash_t::find(key);
    }

    auto find(simple_str<K> key) {
        return find(toStoreType(key));
    }

    auto erase(typename hash_t::const_iterator it) {
        if (it != hash_t::end()) {
            ((sstring<K>*)it->first.node)->~sstring();
        }
        return hash_t::erase(it);
    }

    auto erase(const StoreType<K>& key) {
        auto it = hash_t::find(key);
        if (it != hash_t::end()) {
            ((sstring<K>*)it->first.node)->~sstring();
            hash_t::erase(it);
            return 1;
        }
        return 0;
    }

    auto erase(simple_str<K> key) {
        return erase(toStoreType(key));
    }

    bool lookup(const K* txt, T& val) const {
        auto it = find(e_s(txt));
        if (it != hash_t::end()) {
            val = it->second;
            return true;
        }
        return false;
    }

    bool lookup(simple_str<K> txt, T& val) const {
        auto it = find(txt);
        if (it != hash_t::end()) {
            val = it->second;
            return true;
        }
        return false;
    }

    void clear() {
        for (auto& k: *this)
            ((sstring<K>*)k.first.node)->~sstring();
        hash_t::clear();
    }
    bool contains(const StoreType<K>& key) const {
        return hash_t::find(key) != this->end();
    }

    bool contains(simple_str<K> key) const {
        return find(toStoreType(key)) != this->end();
    }
};

template<typename K>
struct streql {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str == _Right.str;
    }
};

template<typename K>
struct strhash { // hash functor for basic_string
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqlia {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_ia(_Right.str);
    }
};

template<typename K>
struct strhashia {
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash_ia(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqliu {
    bool operator()(const StoreType<K>& _Left, const StoreType<K>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_iu(_Right.str);
    }
};

template<typename K>
struct strhashiu {
    size_t operator()(simple_str<K> _Keyval) const {
        return unicode_traits<K>::hashiu(_Keyval.symbols(), _Keyval.length());
    }
    size_t operator()(const StoreType<K>& _Keyval) const {
        return _Keyval.hash;
    }
};

/*
 * Для построения длинных динамических строк конкатенацией мелких кусочков.
 * Выделяет по мере надобности отдельные блоки заданного размера (или кратного ему для больших вставок),
 * чтобы избежать релокации длинных строк.
 * После построения можно слить в одну строку.
 * Как показали замеры, работает медленнее, чем lstring +=, но экономнее по памяти.
 * Сам является строковым выражением.
*/
template<typename K>
class chunked_string_builder {
    using chunk_t = std::pair<std::unique_ptr<K[]>, size_t>;
    std::vector<chunk_t> chunks; // блоки и длина данных в них
    K* write{};                  // Текущая позиция записи
    size_t len{};                // Общая длина
    size_t remain{};             // Сколько осталось места в текущем блоке
    size_t align{1024};

public:
    using my_type = chunked_string_builder<K>;
    using symb_type = K;
    chunked_string_builder() = default;
    chunked_string_builder(size_t a) : align(a){};
    chunked_string_builder(const my_type&) = delete;
    chunked_string_builder(my_type&& other) noexcept
        : chunks(std::move(other.chunks)), write(other.write), len(other.len), remain(other.remain), align(other.align) {
        other.len = other.remain = 0;
        other.write = nullptr;
    }
    my_type& operator=(my_type other) noexcept {
        chunks.swap(other.chunks);
        write = other.write;
        len = other.len;
        remain = other.remain;
        align = other.align;
        other.len = other.remain = 0;
        other.write = nullptr;
        return *this;
    }

    // Добавление порции данных
    my_type& operator<<(simple_str<K> data) {
        if (data.len) {
            len += data.len;
            if (data.len <= remain) {
                // Добавляемые данные влезают в выделенный блок, просто скопируем их
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;                // Сдвинем позицию  записи
                chunks.back().second += data.len; // Увеличим длину хранимых в блоке данных
                remain -= data.len;               // Уменьшим остаток места в блоке
            } else {
                // Не влезают
                if (remain) {
                    // Сначала запишем сколько влезет
                    ch_traits<K>::copy(write, data.str, remain);
                    data.len -= remain;
                    data.str += remain;
                    chunks.back().second += remain; // Увеличим длину хранимых в блоке данных
                }
                // Выделим новый блок и впишем в него данные
                size_t blockSize = (data.len + align - 1) / align * align; // Рассчитаем размер блока, кратного заданному выравниванию
                chunks.emplace_back(std::make_unique<K[]>(blockSize), data.len);
                write = chunks.back().first.get();
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;
                remain = blockSize - data.len;
            }
        }
        return *this;
    }

    my_type& operator<<(const StrExprForType<K> auto& expr) {
        size_t l = expr.length();
        if (l) {
            if (l < remain) {
                write = expr.place(write);
                chunks.back().second += l;
                len += l;
                remain -= l;
            } else if (!remain) {
                size_t blockSize = (l + align - 1) / align * align; // Рассчитаем размер блока, кратного заданному выравниванию
                chunks.emplace_back(std::make_unique<K[]>(blockSize), l);
                write = expr.place(chunks.back().first.get());
                len += l;
                remain = blockSize - l;
            } else {
                auto store = std::make_unique<K[]>(l);
                expr.place(store.get());
                return operator<<({store.get(), l});
            }
        }
        return *this;
    }
    template<typename T>
    my_type& operator<<(T data)
        requires std::is_same_v<T, K>
    {
        return operator<<(expr_char<K>(data));
    }
    constexpr size_t length() const noexcept {
        return len;
    }

    void reset() {
        if (chunks.empty()) {
            return;
        }
        if (chunks.size() > 1) {
            remain = 0;
            chunks.resize(1);
        }
        remain += chunks[0].second;
        chunks[0].second = 0;
        len = 0;
        write = chunks[0].first.get();
    }

    constexpr K* place(K* p) const noexcept {
        for (const auto& block: chunks) {
            ch_traits<K>::copy(p, block.first.get(), block.second);
            p += block.second;
        }
        return p;
    }

    template<typename Op>
    void out(const Op& o) const {
        for (const auto& block: chunks)
            o(block.first.get(), block.second);
    }

    bool is_continuous() const {
        if (chunks.size()) {
            const K* ptr = chunks.front().first.get();
            for (const auto& chunk: chunks) {
                if (chunk.first.get() != ptr)
                    return false;
                ptr += chunk.second;
            }
        }
        return true;
    }
    const K* begin() const {
        return chunks.size() ? chunks.front().first.get() : simple_str_nt<K>::empty_str.str;
    }

    void clear() {
        chunks.clear();
        write = nullptr;
        len = 0;
        remain = 0;
    }
    struct portion_store {
        typename decltype(chunks)::const_iterator it, end;
        size_t writedFromCurrentChunk;

        bool is_end() {
            return it == end;
        }
        size_t store(K* buffer, size_t size) {
            size_t writed = 0;
            while (size && !is_end()) {
                size_t remain = it->second - writedFromCurrentChunk;
                size_t write = std::min(size, remain);
                ch_traits<K>::copy(buffer, it->first.get() + writedFromCurrentChunk, write);
                writed += write;
                remain -= write;
                size -= write;
                if (!remain) {
                    ++it;
                    writedFromCurrentChunk = 0;
                } else
                    writedFromCurrentChunk += write;
            }
            return writed;
        }
    };
    portion_store get_portion() const {
        return {chunks.begin(), chunks.end(), 0};
    }
    const auto& data() const {
        return chunks;
    }
};

using stringa = sstring<u8s>;
using stringw = sstring<wchar_t>;
using stringu = sstring<u16s>;
using stringuu = sstring<u32s>;

/*!
 * @brief Тип хеш-словаря для char строк, регистрозависимый поиск
 */
template<typename T>
using hashStrMapA = hashStrMap<u8s, T, strhash<u8s>, streql<u8s>>;
/*!
 * @brief Тип хеш-словаря для char строк, регистронезависимый поиск для ASCII символов
 */
template<typename T>
using hashStrMapAIA = hashStrMap<u8s, T, strhashia<u8s>, streqlia<u8s>>;
/*!
 * @brief Тип хеш-словаря для char строк, регистронезависимый поиск для Unicode символов до 0xFFFF
 */
template<typename T>
using hashStrMapAIU = hashStrMap<u8s, T, strhashiu<u8s>, streqliu<u8s>>;

/*!
 * @brief Тип хеш-словаря для wchar_t строк, регистрозависимый поиск
 */
template<typename T>
using hashStrMapW = hashStrMap<wchar_t, T, strhash<wchar_t>, streql<wchar_t>>;

/*!
 * @brief Тип хеш-словаря для wchar_t строк, регистронезависимый поиск для ASCII символов
 */
template<typename T>
using hashStrMapWIA = hashStrMap<wchar_t, T, strhashia<wchar_t>, streqlia<wchar_t>>;

/*!
 * @brief Тип хеш-словаря для wchar_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF
 */
template<typename T>
using hashStrMapWIU = hashStrMap<wchar_t, T, strhashiu<wchar_t>, streqliu<wchar_t>>;

/*!
 * @brief Тип хеш-словаря для char16_t строк, регистрозависимый поиск
 */
template<typename T>
using hashStrMapU = hashStrMap<u16s, T, strhash<u16s>, streql<u16s>>;
template<typename T>
/*!
 * @brief Тип хеш-словаря для char16_t строк, регистронезависимый поиск для ASCII символов
 */
using hashStrMapUIA = hashStrMap<u16s, T, strhashia<u16s>, streqlia<u16s>>;
/*!
 * @brief Тип хеш-словаря для char16_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF
 */
template<typename T>
using hashStrMapUIU = hashStrMap<u16s, T, strhashiu<u16s>, streqliu<u16s>>;

/*!
 * @brief Тип хеш-словаря для char32_t строк, регистрозависимый поиск
 */
template<typename T>
using hashStrMapUU = hashStrMap<u32s, T, strhash<u32s>, streql<u32s>>;
/*!
 * @brief Тип хеш-словаря для char32_t строк, регистронезависимый поиск для ASCII символов
 */
template<typename T>
using hashStrMapUUIA = hashStrMap<u32s, T, strhashia<u32s>, streqlia<u32s>>;
/*!
 * @brief Тип хеш-словаря для char32_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF
 */
template<typename T>
using hashStrMapUUIU = hashStrMap<u32s, T, strhashiu<u32s>, streqliu<u32s>>;

inline constexpr simple_str_nt<u8s> utf8_bom{"\xEF\xBB\xBF", 3}; // NOLINT

inline namespace literals {

inline simple_str_nt<u8s> operator""_ss(const u8s* ptr, size_t l) {
    return simple_str_nt<u8s>{ptr, l};
}

inline simple_str_nt<uws> operator""_ss(const uws* ptr, size_t l) {
    return simple_str_nt<uws>{ptr, l};
}

inline simple_str_nt<u16s> operator""_ss(const u16s* ptr, size_t l) {
    return simple_str_nt<u16s>{ptr, l};
}

inline simple_str_nt<u32s> operator""_ss(const u32s* ptr, size_t l) {
    return simple_str_nt<u32s>{ptr, l};
}

consteval StoreType<u8s> operator""_h(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

consteval StoreType<u8s> operator""_ia(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

inline StoreType<u8s> operator""_iu(const u8s* ptr, size_t l) {
    return StoreType<u8s>{{ptr, l}, strhashiu<u8s>{}(simple_str<u8s>{ptr, l})};
}

consteval StoreType<u16s> operator""_h(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

consteval StoreType<u16s> operator""_ia(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

inline StoreType<u16s> operator""_iu(const u16s* ptr, size_t l) {
    return StoreType<u16s>{{ptr, l}, strhashiu<u16s>{}(simple_str<u16s>{ptr, l})};
}

consteval StoreType<u32s> operator""_h(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

consteval StoreType<u32s> operator""_ia(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

inline StoreType<u32s> operator""_iu(const u32s* ptr, size_t l) {
    return StoreType<u32s>{{ptr, l}, strhashiu<u32s>{}(simple_str<u32s>{ptr, l})};
}

} // namespace literals

inline std::ostream& operator<<(std::ostream& stream, ssa text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, ssw text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, simple_str<wchar_type> text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

inline std::ostream& operator<<(std::ostream& stream, const stringa& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, const stringw& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

inline std::wostream& operator<<(std::wostream& stream, const sstring<wchar_type>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

template<size_t N, bool S, simstr::Allocatorable A>
inline std::ostream& operator<<(std::ostream& stream, const lstring<u8s, N, S, A>& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<uws, N, S, A>& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<wchar_type, N, S, A>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

} // namespace simstr

template<typename K>
struct std::formatter<simstr::simple_str<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

template<typename K>
struct std::formatter<simstr::simple_str_nt<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str_nt<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

template<typename K>
struct std::formatter<simstr::sstring<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::sstring<K>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};

template<typename K, unsigned N, bool S, typename A>
struct std::formatter<simstr::lstring<K, N, S, A>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<K, N, S, A>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};
