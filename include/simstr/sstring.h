/*
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * ver. 1.0
 * Классы для работы со строками
 */

/**
 * @mainpage Библиотека simstr
 * @include{doc} "../readme.md"
 * @page overview Обзор
 * @includedoc{doc} "overview.md"
 */
#pragma once
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
#define empty_bases __declspec(empty_bases)
#else
#define _no_unique_address no_unique_address
#define empty_bases
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
concept ToIntNumber = FromIntNumber<T> || is_one_of_type<T, int8_t>::value;

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

/*!
 * @brief Перечисление с возможными результатами преобразования строки в целое число
 */
enum class IntConvertResult : char {
    Success,            //!< Успешно
    BadSymbolAtTail,    //!< Число закончилось не числовым символом
    Overflow,           //!< Переполнение, число не помещается в заданный тип
    NotNumber           //!< Вообще не число
};

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
constexpr unsigned max_overflow_digits = (sizeof(T) * CHAR_BIT) / digit_width<Base>();

struct int_convert { // NOLINT
private:
    inline static const uint8_t NUMBERS[] = {
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
        u_type  maxMult = 0, maxAdd = 0;
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
        if (ptr != end) {
            if constexpr (std::is_signed_v<T>) {
                if constexpr (AllowSign) {
                    // Может быть число, +число или -число
                    if (*ptr == '+') {
                        ptr++;
                    } else if (*ptr == '-') {
                        negate = true;
                        ptr++;
                    }
                } else {
                    // Может быть число или -число
                    if (*ptr == '-') {
                        negate = true;
                        ptr++;
                    }
                }
            } else if constexpr (AllowSign) {
                // Может быть число или +число
                if (*ptr == '+') {
                    ptr++;
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

template<typename K, typename Impl, bool Mutable> class buffer_pointers;

template<typename K, typename Impl>
class buffer_pointers<K, Impl, false> {
    const Impl& d() const { return *static_cast<const Impl*>(this); }
public:
    /*!
     * @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     */
    const K* c_str() const { return d().symbols(); }
    /*!
     * @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     */
    const K* data() const  { return d().symbols(); }
    /*!
     * @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     */
    const K* begin() const { return d().symbols(); }
    /*!
     * @brief Указатель на константный символ после после последнего символа строки
     * @return const K* - конец строки
     */
    const K* end() const   { return d().symbols() + d().length(); }
};

template<typename K, typename Impl>
class buffer_pointers<K, Impl, true> : public buffer_pointers<K, Impl, false> {
    Impl& d() { return *static_cast<Impl*>(this); }
    using base = buffer_pointers<K, Impl, false>;
public:
    /*!
     * @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     */
    const K* data() const  { return base::data(); }
    /*!
     * @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     */
    const K* begin() const { return base::begin(); }
    /*!
     * @brief Указатель на константный символ после после последнего символа строки
     * @return const K* - конец строки
     */
    const K* end() const   { return base::end(); }
    /*!
     * @brief Получить указатель на буфер символов строки
     * @return K* - указатель на буфер символов строки
     */
    K* data()  { return d().str(); }
    /*!
     * @brief Получить указатель на буфер символов строки
     * @return K* - указатель на буфер символов строки
     */
    K* begin() { return d().str(); }
    /*!
     * @brief Указатель на символ после после последнего символа строки
     * @return K* - конец строки
     */
    K* end()   { return d().str() + d().length(); }
};

/*!
 * @brief Класс с базовыми константными строковыми алгоритмами.
 * @details Является базой для классов, могущих выполнять константные операции со строками.
 * Ничего не знает о хранении строк, ни сам, ни у класса наследника, то есть работает
 * только с указателем на строку и её длиной.
 * Для работы класс-наследник должен реализовать методы:
 *   size_t length() const noexcept     - возвращает длину строки
 *   const K* symbols() const noexcept  - возвращает указатель на начало строки
 *   bool is_empty() const noexcept    - проверка, не пустая ли строка
 * @tparam K       - тип символов
 * @tparam StrRef  - тип хранилища куска строки
 * @tparam Impl    - конечный класс наследник
 */
template<typename K, typename StrRef, typename Impl, bool Mutable>
class str_algs : public buffer_pointers<K, Impl, Mutable> {
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
    using base = str_algs<K, StrRef, Impl, Mutable>;
    // Пустой конструктор
    str_algs() = default;

    /*!
     * @brief Копировать строку в указанный буфер.
     * @details Метод предполагает, что размер выделенного буфера достаточен для всей строки, т.е.
     * предварительно была запрошена `length()`. Не добавляет `\0`.
     * @param ptr - указатель на буфер
     * @return указатель на символ после конца размещённой в буфере строки
     */
    constexpr K* place(K* ptr) const noexcept {
        size_t myLen = _len();
        if (myLen) {
            traits::copy(ptr, _str(), myLen);
            return ptr + myLen;
        }
        return ptr;
    }
    /*!
     * @brief Копировать строку в указанный буфер.
     * @details Метод добавляет `\0` после скопированных символов. Не выходит за границы буфера.
     * @param buffer - указатель на буфер
     * @param bufSize - размер буфера в символах.
     */
    void copy_to(K* buffer, size_t bufSize) {
        size_t tlen = std::min(_len(), bufSize - 1);
        if (tlen)
            traits::copy(buffer, _str(), tlen);
        buffer[tlen] = 0;
    }
    /*!
     * @brief Размер строки в символах.
     * @return size_t 
     */
    size_t size() const {
        return _len();
    }

    /*!
     * @brief Преобразовать себя в "кусок строки", включающий всю строку
     * @return str_piece 
     */
    constexpr operator str_piece() const noexcept {
        return str_piece{_str(), _len()};
    }
    /*!
     * @brief Преобразовать себя в "кусок строки", включающий всю строку
     * @return str_piece 
     */
    str_piece to_str() const noexcept {
        return {_str(), _len()};
    }
    /*!
     * @brief Конвертировать в std::string_view
     * @return std::string_view
     */
    std::string_view to_sv() const noexcept {
        return {_str(), _len()};
    }
    /*!
     * @brief Конвертировать в std::string
     * @return std::string
     */
    std::string to_string() const noexcept {
        return {_str(), _len()};
    }
    /*!
     * @brief Получить часть строки как "simple_str"
     * @param from - количество символов от начала строки.
     * @param len - количество символов в получаемом "куске".
     * @return Подстроку, simple_str.
     * @details  Если `from` меньше нуля, то отсчитывается `-from` символов от конца строки в сторону начала.
     *           Если `len` меньше или равно нулю, то отсчитать `-len` символов от конца строки
     *  ```cpp
     *      "0123456789"_ss(5, 2) == "56";
     *      "0123456789"_ss(5) == "56789";
     *      "0123456789"_ss(5, -1) == "5678";
     *      "0123456789"_ss(-3) == "789";
     *      "0123456789"_ss(-3, 2) == "78";
     *      "0123456789"_ss(-4, -1) == "678";
     *  ```
     */
    constexpr str_piece operator()(ptrdiff_t from, ptrdiff_t len = 0) const noexcept {
        size_t myLen = _len(), idxStart = from >= 0 ? from : myLen > -from ? myLen + from : 0,
            idxEnd = len > 0 ? idxStart + len : myLen > -len ? myLen + len : 0;
        if (idxEnd > myLen)
            idxEnd = myLen;
        if (idxStart > idxEnd)
            idxStart = idxEnd;
        return str_piece{_str() + idxStart, idxEnd - idxStart};
    }
    /*!
     * @brief Получить часть строки как "кусок строки".
     * @param from - количество символов от начала строки. При превышении размера строки вернёт пустую строку.
     * @param len - количество символов в получаемом "куске". При выходе за пределы строки вернёт всё до конца строки.
     * @return Подстроку, simple_str.
     */
    constexpr str_piece mid(size_t from, size_t len = -1) const noexcept {
        size_t myLen = _len(), idxStart = from, idxEnd = from > std::numeric_limits<size_t>::max() - len ? myLen : from + len;
        if (idxEnd > myLen)
            idxEnd = myLen;
        if (idxStart > idxEnd)
            idxStart = idxEnd;
        return str_piece{_str() + idxStart, idxEnd - idxStart};
    }
    /*!
     * @brief Получить подстроку simple_str с позиции от from до позиции to (не включая её)
     * @details Для производительности метод никак не проверяет выходы за границы строки, используйте
     *          в сценариях, когда точно знаете, что это позиции внутри строки и to >= from.
     * @param from - начальная позиция
     * @param to - конечная позиция (не входит в результат)
     * @return Подстроку, simple_str.
     */
    constexpr str_piece from_to(size_t from, size_t to) const noexcept {
        return str_piece{_str() + from, to - from};
    }
    /*!
     * @brief Проверка на пустоту
     */
    bool operator!() const noexcept {
        return _is_empty();
    }
    /*!
     * @brief Получить символ на заданной позиции 
     * @param idx - индекс символа. Для отрицательных значений отсчитывается от конца строки.
     * @return K - символ
     * @details Не производит проверку на выход за границы строки
     */
    K at(ptrdiff_t idx) const {
        return _str()[idx >= 0 ? idx : _len() + idx];
    }
    // Сравнение строк
    constexpr int compare(const K* text, size_t len) const {
        size_t myLen = _len();
        int cmp = traits::compare(_str(), text, std::min(myLen, len));
        return cmp == 0 ? (myLen > len ? 1 : myLen == len ? 0 : -1) : cmp;
    }
    /*!
     * @brief Сравнение строк посимвольно
     * @param o - другая строка
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше
     */
    constexpr int compare(str_piece o) const {
        return compare(o.symbols(), o.length());
    }
    /*!
     * @brief Сравнение с C-строкой посимвольно
     * @param o - другая строка
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше
     */
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
    /*!
     * @brief Сравнение строк на равенство
     * @param other - другая строка
     * @return равны ли строки
     */
    constexpr bool equal(str_piece other) const noexcept {
        return equal(other.symbols(), other.length());
    }
    /*!
     * @brief Оператор сравнение строк на равенство
     * @param other - другая строка
     * @return равны ли строки
     */
    constexpr bool operator==(const base& other) const noexcept {
        return equal(other._str(), other._len());
    }
    /*!
     * @brief Оператор сравнения строк
     * @param other - другая строка
     */
    constexpr auto operator<=>(const base& other) const noexcept {
        return compare(other._str(), other._len()) <=> 0;
    }
    /*!
     * @brief Оператор сравнения строки и строкового литерала на равенство
     * @param other - строковый литерал
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    bool operator==(T&& other) const noexcept {
        return N - 1 == _len() && traits::compare(_str(), other, N - 1) == 0;
    }

    /*!
     * @brief Оператор сравнения строки и строкового литерала
     * @param other - строковый литерал
     */
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
    /*!
     * @brief Сравнение строк посимвольно без учёта регистра ASCII символов
     * @param text - другая строка
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше
     */
    int compare_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length());
    }

    /*!
     * @brief Равна ли строка другой строке посимвольно без учёта регистра ASCII символов
     * @param text - другая строка
     * @return равны ли строки
     */
    bool equal_ia(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_ia(text.symbols(), text.length()) == 0;
    }
    /*!
     * @brief Меньше ли строка другой строки посимвольно без учёта регистра ASCII символов
     * @param text - другая строка
     * @return меньше ли строка
     */
    bool less_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length()) < 0;
    }

    int compare_iu(const K* text, size_t len) const noexcept { // NOLINT
        if (!len)
            return _is_empty() ? 0 : 1;
        return uni::compareiu(_str(), _len(), text, len);
    }
    /*!
     * @brief Сравнение строк посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF)
     * @param text - другая строка
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше
     */
    int compare_iu(str_piece text) const noexcept { // NOLINT
        return compare_iu(text.symbols(), text.length());
    }

    /*!
     * @brief Равна ли строка другой строке посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF)
     * @param text - другая строка
     * @return равны ли строки
     */
    bool equal_iu(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_iu(text.symbols(), text.length()) == 0;
    }
    /*!
     * @brief Меньше ли строка другой строки посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF)
     * @param text - другая строка
     * @return меньше ли строка
     */
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
    /*!
     * @brief Найти начало первого вхождения подстроки в этой строке.
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию начала вхождения подстроки, или -1, если не найдена
     */
    size_t find(str_piece pattern, size_t offset = 0) const noexcept {
        return find(pattern.symbols(), pattern.length(), offset);
    }
    /*!
     * @brief Найти начало первого вхождения подстроки в этой строке или выкинуть исключение.
     * @tparam Exc - тип исключения
     * @tparam Args... - типы параметров для конструирования исключения, выводятся из аргументов
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию начала вхождения подстроки, или выбрасывает исключение Exc, если не найдена
     */
    template<typename Exc, typename ... Args> requires std::is_constructible_v<Exc, Args...>
    size_t find_or_throw(str_piece pattern, size_t offset = 0, Args&& ... args) const noexcept {
        if (auto fnd = find(pattern.symbols(), pattern.length(), offset); fnd != str::npos) {
            return fnd;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /*!
     * @brief Найти конец вхождения подстроки в этой строке.
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию сразу за вхождением подстроки, или -1, если не найдена
     */
    size_t find_end(str_piece pattern, size_t offset = 0) const noexcept {
        size_t fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? fnd : fnd + pattern.length();
    }
    /*!
     * @brief Найти начало первого вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию начала вхождения подстроки, или длину строки, если не найдена
     */
    size_t find_or_all(str_piece pattern, size_t offset = 0) const noexcept {
        auto fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd;
    }
    /*!
     * @brief Найти конец первого вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию сразу за вхождением подстроки, или длину строки, если не найдена
     */
    size_t find_end_or_all(str_piece pattern, size_t offset = 0) const noexcept {
        auto fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd + pattern.length();
    }

    size_t find_last(const K* pattern, size_t lenPattern, size_t offset) const noexcept {
        if (lenPattern == 1)
            return find_last(pattern[0], offset);
        size_t lenText = std::min(_len(), offset);
        // Образец, не вмещающийся в строку и пустой образец не находим
        if (!lenPattern || lenPattern > lenText)
            return str::npos;
        
        lenPattern--;
        const K *text = _str() + lenPattern, last = pattern[lenPattern];
        lenText -= lenPattern;
        while(lenText) {
            if (text[--lenText] == last) {
                if (traits::compare(text + lenText - lenPattern, pattern, lenPattern) == 0) {
                    return lenText;
                }
            }
        }
        return str::npos;
    }
    /*!
     * @brief Найти начало последнего вхождения подстроки в этой строке.
     * @param pattern - искомая строка
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца
     * @return size_t - позицию начала вхождения подстроки, или -1, если не найдена
     */
    size_t find_last(str_piece pattern, size_t offset = -1) const noexcept {
        return find_last(pattern.symbols(), pattern.length(), offset);
    }
    /*!
     * @brief Найти конец последнего вхождения подстроки в этой строке.
     * @param pattern - искомая строка
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца
     * @return size_t - позицию сразу за последним вхождением подстроки, или -1, если не найдена
     */
    size_t find_end_of_last(str_piece pattern, size_t offset = -1) const noexcept {
        size_t fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? fnd : fnd + pattern.length();
    }
    /*!
     * @brief Найти начало последнего вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца
     * @return size_t - позицию начала вхождения подстроки, или длину строки, если не найдена
     */
    size_t find_last_or_all(str_piece pattern, size_t offset = -1) const noexcept {
        auto fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd;
    }
    /*!
     * @brief Найти конец последнего вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца
     * @return size_t - позицию сразу за последним вхождением подстроки, или длину строки, если не найдена
     */
    size_t find_end_of_last_or_all(str_piece pattern, size_t offset = -1) const noexcept {
        size_t fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd + pattern.length();
    }
    /*!
     * @brief Содержит ли строка указанную подстроку.
     * @param pattern - искомая строка
     * @param offset - с какой позиции начинать поиск
     * @return bool
     */
    bool contains(str_piece pattern, size_t offset = 0) const noexcept {
        return find(pattern, offset) != str::npos;
    }
    /*!
     * @brief Найти символ в этой строке.
     * @param s - искомый символ
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию найденного символа, или -1, если не найден
     */
    size_t find(K s, size_t offset = 0) const noexcept {
        size_t len = _len();
        if (offset < len) {
            const K *str = _str(), *fnd = traits::find(str + offset, len - offset, s);
            if (fnd)
                return static_cast<size_t>(fnd - str);
        }
        return str::npos;
    }
    /*!
     * @brief Найти символ в этой строке или конец строки.
     * @param s - искомый символ
     * @param offset - с какой позиции начинать поиск
     * @return size_t - позицию найденного символа, или длину строки, если не найден
     */
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
    /*!
     * @brief Вызвать функтор для всех найденных вхождений подстроки в этой строке
     * @param op - функтор, принимающий строку
     * @param pattern - искомая подстрока
     * @param offset - позиция начала поиска
     * @param maxCount - максимальное количество обрабатываемых вхождений, 0 - без ограничений
     */
    template<typename Op>
    void for_all_finded(const Op& op, str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        for_all_finded(op, pattern.symbols(), pattern.length(), offset, maxCount);
    }

    std::vector<size_t> find_all(const K* pattern, size_t patternLen, size_t offset, size_t maxCount) const {
        std::vector<size_t> result;
        for_all_finded([&](auto f) { result.push_back(f); }, pattern, patternLen, offset, maxCount);
        return result;
    }
    /*!
     * @brief Найти все вхождения подстроки в этой строке
     * @param pattern - искомая подстрока
     * @param offset - позиция начала поиска
     * @param maxCount - максимальное количество обрабатываемых вхождений, 0 - без ограничений
     * @return std::vector<size_t> - вектор с позициями начал найденных вхождений
     */
    std::vector<size_t> find_all(str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        return find_all(pattern.symbols(), pattern.length(), offset, maxCount);
    }
    /*!
     * @brief Найти последнее вхождения символа в этой строке
     * @param s - искомый символ
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца
     * @return size_t - позицию найденного символа, или -1, если не найден
     */
    size_t find_last(K s, size_t offset = -1) const noexcept {
        size_t len = std::min(_len(), offset);
        const K *text = _str();
        while (len > 0) {
            if (text[--len] == s)
                return len;
        }
        return str::npos;
    }
    /*!
     * @brief Найти первое вхождение символа из заданного набора символов
     * @param pattern - строка, задающая набор искомых символов
     * @param offset - позиция начала поиска
     * @return size_t - позицию найденного вхождения, или -1, если не найден
     */
    size_t find_first_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::string_view{_str(), _len()}.find_first_of(std::string_view{pattern.str, pattern.len}, offset);
    }
    /*!
     * @brief Найти первое вхождение символа из заданного набора символов
     * @param pattern - строка, задающая набор искомых символов
     * @param offset - позиция начала поиска
     * @return std::pair<size_t, size_t> - пару из позиции найденного вхождения и номера найденного символа в наборе, или -1, если не найден
     */
    std::pair<size_t, size_t> find_first_of_idx(str_piece pattern, size_t offset = 0) const noexcept {
        const K* text = _str();
        size_t fnd = std::string_view{text, _len()}.find_first_of(std::string_view{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::string::npos ? fnd : pattern.find(text[fnd]) };
    }
    /*!
     * @brief Найти первое вхождение символа не из заданного набора символов
     * @param pattern - строка, задающая набор символов
     * @param offset - позиция начала поиска
     * @return size_t - позицию найденного вхождения, или -1, если не найден
     */
    size_t find_first_not_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::string_view{_str(), _len()}.find_first_not_of(std::string_view{pattern.str, pattern.len}, offset);
    }
    /*!
     * @brief Найти последнее вхождение символа из заданного набора символов
     * @param pattern - строка, задающая набор искомых символов
     * @param offset - позиция начала поиска
     * @return size_t - позицию найденного вхождения, или -1, если не найден
     */
    size_t find_last_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::string_view{_str(), _len()}.find_last_of(std::string_view{pattern.str, pattern.len}, offset);
    }
    /*!
     * @brief Найти последнее вхождение символа из заданного набора символов
     * @param pattern - строка, задающая набор искомых символов
     * @param offset - позиция начала поиска
     * @return std::pair<size_t, size_t> - пару из позиции найденного вхождения и номера найденного символа в наборе, или -1, если не найден
     */
    std::pair<size_t, size_t> find_last_of_idx(str_piece pattern, size_t offset = str::npos) const noexcept {
        const K* text = _str();
        size_t fnd = std::string_view{text, _len()}.find_last_of(std::string_view{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::string::npos ? fnd : pattern.find(text[fnd]) };
    }
    /*!
     * @brief Найти последнее вхождение символа не из заданного набора символов
     * @param pattern - строка, задающая набор символов
     * @param offset - позиция начала поиска
     * @return size_t - позицию найденного вхождения, или -1, если не найден
     */
    size_t find_last_not_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::string_view{_str(), _len()}.find_last_not_of(std::string_view{pattern.str, pattern.len}, offset);
    }
    /*!
     * @brief Получить подстроку. Работает аналогично operator(), только результат выдает того же типа, к которому применён метод
     * @param from - количество символов от начала строки. Если меньше нуля, отсчитывается от конца строки в сторону начала.
     * @param len - количество символов в получаемом "куске". Если меньше или равно нулю, то отсчитать len символов от конца строки
     * @return my_type - подстроку, объект того же типа, к которому применён метод
     */
    my_type substr(ptrdiff_t from, ptrdiff_t len = 0) const { // индексация в code units
        return my_type{d()(from, len)};
    }
    /*!
     * @brief Получить часть строки объектом того же типа, к которому применён метод, аналогично mid.
     * @param from - количество символов от начала строки. При превышении размера строки вернёт пустую строку.
     * @param len - количество символов в получаемом "куске". При выходе за пределы строки вернёт всё до конца строки.
     * @return Строку того же типа, к которому применён метод.
     */
    my_type str_mid(size_t from, size_t len = -1) const { // индексация в code units
        return my_type{d().mid(from, len)};
    }
    /*!
     * @brief Преобразовать строку в число заданного типа
     * @tparam T - желаемый тип числа
     * @tparam CheckOverflow - проверять на переполнение
     * @tparam Base - основание счисления числа, от -1 до 36, кроме 1.
     *         - Если 0: то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10
     *         - Если -1: то пытается определить основание по префиксам:
     *            - 0 или 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *            - в остальных случаях 10.
     * @tparam SkipWs - пропускать пробельные символы в начале строки
     * @tparam AllowSign - допустим ли знак '+' перед числом
     * @return T - число, результат преобразования, насколько оно получилось, или 0 при переполнении.
     */
    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true, bool AllowSign = true>
    T as_int() const noexcept {
        auto [res, err, _] = int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs, AllowSign>(_str(), _len());
        return err == IntConvertResult::Overflow ? 0 : res;
    }
    /*!
     * @brief Преобразовать строку в число заданного типа
     * @tparam T - желаемый тип числа
     * @tparam CheckOverflow - проверять на переполнение
     * @tparam Base - основание счисления числа, от -1 до 36, кроме 1.
     *         - Если 0: то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10
     *         - Если -1: то пытается определить основание по префиксам:
     *            - 0 или 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *            - в остальных случаях 10.
     * @tparam SkipWs - пропускать пробельные символы в начале строки. Пропускаются все символы с ASCII кодами <= 32.
     * @tparam AllowSign - допустим ли знак '+' перед числом
     * @return std::tuple<T, IntConvertResult, size_t> - кортеж из полученного числа, успешности преобразования и количестве обработанных символов
     */
    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true, bool AllowSign = true>
    std::tuple<T, IntConvertResult, size_t> to_int() const noexcept {
        return int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs, AllowSign>(_str(), _len());
    }
    /*!
     * @brief Преобразовать строку в double
     * @return double. Пока работает только для строк из char, wchar_t и типов, совместимых с wchar_t по размеру.
     */
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

    /*!
     * @brief Преобразовать строку в целое число 
     * @tparam T - тип числа, выводится из аргумента
     * @param t - переменная, в которую записывается результат
     */
    template<ToIntNumber T>
    void as_number(T& t) {
        t = as_int<T>();
    }
    /*!
     * @brief Преобразовать строку в double
     * @param t - переменная, в которую записывается результат
     */
    void as_number(double& t) {
        t = to_double();
    }

    template<typename T, typename Op>
    T splitf(const K* delimeter, size_t lenDelimeter, const Op& beforeFunc, size_t offset) const {
        size_t mylen = _len();
        std::conditional_t<std::is_same_v<T, void>, char, T> results;
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
                } else if constexpr (requires {results[i] = last;} && requires{std::size(results);}) {
                    if (i < std::size(results)) {
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
            } else if constexpr (requires { results[i] = piece; } && requires{std::size(results);}) {
                if (i < std::size(results)) {
                    results[i] = piece;
                    if (i == results.size() - 1) {
                        break;
                    }
                }
            }
            offset = beginOfDelim + lenDelimeter;
        }
        if constexpr (!std::is_same_v<T, void>) {
            return results;
        }
    }
    /*!
     * @brief Разделить строку на части по заданному разделителю, с возможным применением функтора к каждой подстроке
     * @tparam T - тип контейнера для складывания подстрок. 
     * @param delimeter - подстрока разделитель
     * @param beforeFunc - функтор для применения к найденным подстрокам, перед помещением их в результат
     * @param offset - позиция начала поиска разделителя
     * @return T - результат.
     * @details Для каждой найденной подстроки, если функтор может принять её, вызывается функтор, и подстрока
     *          присваивается результату функтора. Далее подстрока пытается добавиться в результат,
     *          вызывая один из его методов - `emplace_back`, `push_back`, `operator[]`. Если ни одного этого метода
     *          нет, ничего не делается, только вызов функтора.
     *          `operator[]` пытается применится, если у результата можно получить размер через `std::size` и
     *          мы не выходим за этот размер.
     *          При этом, если найденная подстрока получается совпадающей со всей строкой - в результат пытается
     *          поместить не подстроку, а весь объект строки, что позволяет, например, эффективно копировать sstring.
     */
    template<typename T, typename Op>
    T splitf(str_piece delimeter, const Op& beforeFunc, size_t offset = 0) const {
        return splitf<T>(delimeter.symbols(), delimeter.length(), beforeFunc, offset);
    }
    /*!
     * @brief Разделить строку на подстроки по заданному разделителю
     * @tparam T - тип контейнера для результата
     * @param delimeter - разделитель
     * @param offset - позиция начала поиска разделителя
     * @return T - контейнер с результатом
     */
    template<typename T>
    T split(str_piece delimeter, size_t offset = 0) const {
        return splitf<T>(delimeter.symbols(), delimeter.length(), 0, offset);
    }
    /*!
     * @brief Получить объект `Splitter` по заданному разделителю, который позволяет последовательно
     *        получать подстроки методом `next()`, пока `is_done()` false.
     * @param delimeter - разделитель
     * @return Splitter<K>
     */
    Splitter<K> splitter(str_piece delimeter) const;

    // Начинается ли эта строка с указанной подстроки
    constexpr bool starts_with(const K* prefix, size_t l) const noexcept {
        return _len() >= l && 0 == traits::compare(_str(), prefix, l);
    }
    /*!
     * @brief Начинается ли строка с заданной подстроки
     * @param prefix - подстрока
     */
    constexpr bool starts_with(str_piece prefix) const noexcept {
        return starts_with(prefix.symbols(), prefix.length());
    }

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
    /*!
     * @brief Начинается ли строка с заданной подстроки без учёта регистра ASCII символов
     * @param prefix - подстрока
     */
    constexpr bool starts_with_ia(str_piece prefix) const noexcept {
        return starts_with_ia(prefix.symbols(), prefix.length());
    }
    // Начинается ли эта строка с указанной подстроки без учета unicode регистра
    bool starts_with_iu(const K* prefix, size_t len) const noexcept {
        return _len() >= len && 0 == uni::compareiu(_str(), len, prefix, len);
    }
    /*!
     * @brief Начинается ли строка с заданной подстроки без учёта регистра Unicode символов первой плоскости (<0xFFFF)
     * @param prefix - подстрока
     */
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
    /*!
     * @brief Является ли эта строка началом другой строки
     * @param text - другая строка
     */
    constexpr bool prefix_in(str_piece text) const noexcept {
        return prefix_in(text.symbols(), text.length());
    }
    // Заканчивается ли строка указанной подстрокой
    constexpr bool ends_with(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return len <= myLen && traits::compare(_str() + myLen - len, suffix, len) == 0;
    }
    /*!
     * @brief Заканчивается ли строка указанной подстрокой
     * @param suffix - подстрока
     */
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
    /*!
     * @brief Заканчивается ли строка указанной подстрокой без учёта регистра ASCII символов
     * @param suffix - подстрока
     */
    constexpr bool ends_with_ia(str_piece suffix) const noexcept {
        return ends_with_ia(suffix.symbols(), suffix.length());
    }
    // Заканчивается ли строка указанной подстрокой без учета регистра UNICODE
    constexpr bool ends_with_iu(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return myLen >= len && 0 == uni::compareiu(_str() + myLen - len, len, suffix, len);
    }
    /*!
     * @brief Заканчивается ли строка указанной подстрокой без учёта регистра Unicode символов первой плоскости (<0xFFFF)
     * @param suffix - подстрока
     */
    constexpr bool ends_with_iu(str_piece suffix) const noexcept {
        return ends_with_iu(suffix.symbols(), suffix.length());
    }
    /*!
     * @brief Содержит ли строка только ASCII символы
     */
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
    /*!
     * @brief Получить копию строки в верхнем регистре ASCII символов
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в верхнем регистре
     */
    template<typename R = my_type>
    R uppered_only_ascii() const {
        return R::uppered_only_ascii_from(d());
    }
    /*!
     * @brief Получить копию строки в нижнем регистре ASCII символов
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в нижнем регистре
     */
    template<typename R = my_type>
    R lowered_only_ascii() const {
        return R::lowered_only_ascii_from(d());
    }
    /*!
     * @brief Получить копию строки в верхнем регистре Unicode символов первой плоскости (<0xFFFF)
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в верхнем регистре
     */
    template<typename R = my_type>
    R uppered() const {
        return R::uppered_from(d());
    }
    /*!
     * @brief Получить копию строки в нижнем регистре Unicode символов первой плоскости (<0xFFFF)
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в нижнем регистре
     */
    template<typename R = my_type>
    R lowered() const {
        return R::lowered_from(d());
    }
    /*!
     * @brief Получить копию строки с заменёнными вхождениями подстрок
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @param pattern - искомая подстрока
     * @param repl - строка, на которую заменять
     * @param offset - начальная позиция поиска
     * @param maxCount - максимальное количество замен, 0 - без ограничений
     * @return R строку заданного типа, по умолчанию того же, чей метод вызывался.
     */
    template<typename R = my_type>
    R replaced(str_piece pattern, str_piece repl, size_t offset = 0, size_t maxCount = 0) const {
        return R::replaced_from(d(), pattern, repl, offset, maxCount);
    }

    /*!
     * @brief Получить строковое выражение, которое выдает строку с заменёнными подстроками, заданными строковыми литералами.
     * @param pattern - строковый литерал, подстрока, которую меняем
     * @param repl - строковый литерал, подстрока, на которую меняем
     * @return строковое выражение, заменяющее подстроки
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count, typename M, size_t L = const_lit_for<K, M>::Count>
    expr_replaces<K, N - 1, L - 1> replace_init(T&& pattern, M&& repl) const {
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
    /*!
     * @brief Получить строку с удалением пробельных символов слева и справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @return R - строка, с удалёнными в начале и в конце пробельными символами
     */
    template<typename R = str_piece>
    R trimmed() const {
        return R::template trim_static<TrimSides::TrimAll>(d());
    }
    /*!
     * @brief Получить строку с удалением пробельных символов слева
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @return R - строка, с удалёнными в начале пробельными символами
     */
    template<typename R = str_piece>
    R trimmed_left() const {
        return R::template trim_static<TrimSides::TrimLeft>(d());
    }
    /*!
     * @brief Получить строку с удалением пробельных символов справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @return R - строка, с удалёнными в конце пробельными символами
     */
    template<typename R = str_piece>
    R trimmed_right() const {
        return R::template trim_static<TrimSides::TrimRight>(d());
    }
    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, слева и справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в литерале
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, слева
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале символами, содержащимися в литерале
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в конце символами, содержащимися в литерале
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    // Триминг по символам в литерале и пробелам

    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, слева и справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в литерале
     *  и пробельными символами
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, слева
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале символами, содержащимися в литерале
     *  и пробельными символами
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться
     * @return R - строка, с удалёнными в конце символами, содержащимися в литерале
     *  и пробельными символами
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }
    // Триминг по динамическому источнику

    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, слева и справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в шаблоне
     */
    template<typename R = str_piece>
    R trimmed(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, слева
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале символами, содержащимися в шаблоне
     */
    template<typename R = str_piece>
    R trimmed_left(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в конце символами, содержащимися в шаблоне
     */
    template<typename R = str_piece>
    R trimmed_right(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, слева и справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в шаблоне
     *  и пробельными символами
     */
    template<typename R = str_piece>
    R trimmed_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, слева
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в начале символами, содержащимися в шаблоне
     *  и пробельными символами
     */
    template<typename R = str_piece>
    R trimmed_left_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    /*!
     * @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, справа
     * @tparam R - желаемый тип строки, по умолчанию simple_str
     * @param pattern - строка, задающая символы, которые будут обрезаться
     * @return R - строка, с удалёнными в конце символами, содержащимися в шаблоне
     *  и пробельными символами
     */
    template<typename R = str_piece>
    R trimmed_right_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }
};

/*
* Базовая структура с информацией о строке.
* Это структура для невладеющих строк.
* Так как здесь только один базовый класс, MSVC компилятор автоматом применяет empty base optimization,
* в результате размер класса не увеличивается
*/

/*!
 * @brief Простейший класс иммутабельной не владеющей строки.
 * @details Аналог std::string_view. Содержит только указатель и длину.
 *          Как наследник от str_algs поддерживает все константные строковые методы.
 * @tparam K - тип символов строки
 */
template<typename K>
struct simple_str : str_algs<K, simple_str<K>, simple_str<K>, false> {
    using symb_type = K;
    using my_type = simple_str<K>;

    const symb_type* str;
    size_t len;

    simple_str() = default;

    /*!
     * @brief Конструктор из строкового литерала.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr simple_str(T&& v) noexcept : str(v), len(N - 1) {}
    /*!
     * @brief Конструктор из указателя и длины
     */
    constexpr simple_str(const K* p, size_t l) noexcept : str(p), len(l) {}
    /*!
     * @brief Конструктор, позволяющий инициализировать объектами std::string, и std::string_view
     *        при условии, что они lvalue, то есть не временные.
     */
    template<typename S>
        requires(std::is_same_v<S, std::string&> || std::is_same_v<S, const std::string&>
            || std::is_same_v<S, std::string_view&> || std::is_same_v<S, const std::string_view&>)
    constexpr simple_str(S&& s) noexcept : str(s.data()), len(s.length()) {}
    /*!
     * @brief Получить длину строки
     */
    constexpr size_t length() const noexcept {
        return len;
    }
    /*!
     * @brief Получить указатель на константный буфер с символами строки
     */
    constexpr const symb_type* symbols() const noexcept {
        return str;
    }
    /*!
     * @brief Проверить, не пуста ли строка
     */
    constexpr bool is_empty() const noexcept {
        return len == 0;
    }
    /*!
     * @brief Проверить, не указывают ли два объекта на одну строку
     * @param other - другая строка
     */
    bool is_same(simple_str<K> other) const noexcept {
        return str == other.str && len == other.len;
    }
    /*!
     * @brief Проверить, не является ли строка частью другой строки
     * @param other - другая строка
     */
    bool is_part_of(simple_str<K> other) const noexcept {
        return str >= other.str && str + len <= other.str + other.len;
    }
    /*!
     * @brief Получить символ из указанной позиции. Проверка границ не выполняется.
     * @param idx - позиция символа
     * @return K  - символ
     */
    K operator[](size_t idx) const {
        return str[idx];
    }
    /*!
     * @brief Сдвигает начало строки на заданное количество символов
     * @param delta - количество символов
     * @return my_type& 
     */
    my_type& remove_prefix(size_t delta) {
        str += delta;
        len -= delta;
        return *this;
    }
    /*!
     * @brief Укорачивает строку на заданное количество символов
     * @param delta - количество символов
     * @return my_type& 
     */
    my_type& remove_suffix(size_t delta) {
        len -= delta;
        return *this;
    }
};

/*!
 * @brief Класс, заявляющий, что ссылается на нуль-терминированную строку.
 * @tparam K - тип символов строки
 * @details Служит для показа того, что функция параметром хочет получить
 *      строку с нулем в конце, например, ей надо дальше передавать его в
 *      стороннее API. Без этого ей надо было бы либо указывать параметром
 *      конкретный класс строки, что лишает универсальности, либо приводило бы
 *      к постоянным накладным расходам на излишнее копирование строк во временный
 *      буфер. Источником нуль-терминированных строк могут быть строковые литералы
 *      при компиляции, либо классы, хранящие строки.
 */
template<typename K>
struct simple_str_nt : simple_str<K> {
    using symb_type = K;
    using my_type = simple_str_nt<K>;
    using base = simple_str<K>;
    using base::base;

    constexpr static const K empty_string[1] = {0};

    simple_str_nt() = default;
    /*!
     * @brief Явный конструктор из С-строки.
     * @param p - указатель на C-строку (нуль-терминированная строка)
     * @details Это единственный конструктор из всех строковых объектов, принимающий C-строку.
     * Вычисляет её длину при инициализации. Все остальные строковые объекты не инициализируются
     * C-строками. Это для того, чтобы `strlen` вызывалась только в одном месте библиотеки,
     * длина C-строки вычислялась только один раз и далее не терялась случайно при передаче между разными 
     * типами строковых объектов.
     */
    template<typename T> requires std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<T>>>, K>
    explicit simple_str_nt(T&& p) noexcept {
        base::len = p ? static_cast<size_t>(base::traits::length(p)) : 0;
        base::str = base::len ? p : empty_string;
    }
    /*!
     * @brief Конструктор, позволяющий инициализировать объектами std::string, и std::string_view
     *        при условии, что они lvalue, то есть не временные.
     */
    template<typename S>
        requires(std::is_same_v<S, std::string&> || std::is_same_v<S, const std::string&>
            || std::is_same_v<S, std::string_view&> || std::is_same_v<S, const std::string_view&>)
    constexpr simple_str_nt(S&& s) noexcept : base(s) {}

    static const my_type empty_str;
    /*!
     * @brief Оператор преобразования в нуль-терминированную C-строку
     * @return const K* - указатель на начало строки
     */
    operator const K*() const noexcept {
        return base::str;
    }
    /*!
     * @brief Получить нуль-терминированную строку, сдвинув начало на заданное количество символов
     * @param from - на сколько символов сдвинуть начало строки
     * @return my_type 
     */
    my_type to_nts(size_t from) {
        if (from > base::len) {
            from = base::len;
        }
        return {base::str + from, base::len - from};
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

/*!
 * @brief Класс для последовательного получения подстрок по заданному разделителю
 * @tparam K - тип символов
 */
template<typename K>
class Splitter {
    simple_str<K> text_;
    simple_str<K> delim_;

public:
    Splitter(simple_str<K> text, simple_str<K> delim) : text_(text), delim_(delim) {}
    /*!
     * @brief Узнать, не закончились ли подстроки
     */
    bool is_done() const {
        return text_.length() == str::npos;
    }
    /*!
     * @brief Получить следующую подстроку
     * @return simple_str
     */
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

template<typename K, typename StrRef, typename Impl, bool Mutable>
Splitter<K> str_algs<K, StrRef, Impl, Mutable>::splitter(StrRef delimeter) const {
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

/*!
 * @brief Базовый класс для строк, могущих конвертироваться из другого типа символов.
 * @tparam K - тип символов
 * @tparam Impl - конечный класс
 * @details Конвертация выполняется через UTF преобразование.
 *  Считаем, что строки `char` - в UTF-8, `char16_t` - в UTF-16, `char32_t` - в UTF-32.
 *  `wchar_t` - под Windows UTF-16, в Linux - UTF-32.
 */
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
    template<typename O, typename I, bool M>
        requires(!std::is_same_v<O, K>)
    from_utf_convertable(const str_algs<O, simple_str<O>, I, M>& init) : from_utf_convertable(init.to_str()) {}
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

/*!
 * @brief Возвращает строковое выражение, преобразующую строку из одного типа символов
 * в другой тип, через UTF-конвертирование.
 * @tparam To - тип строки, в которую надо конвертировать
 * @tparam From - тип строки, из которого надо конвертировать. Выводится из аргумента.
 * @param from - строка, из которой надо конвертировать.
 */
template<typename To, typename From> requires (!std::is_same_v<From, To>)
auto e_utf(simple_str<From> from) {
    return expr_utf<From, To>{from};
}

/*!
 * @brief Концепт типа, который может сохранить строку
 */
template<typename A, typename K>
concept storable_str = requires {
    A::is_str_storable == true;
    std::is_same_v<typename A::symb_type, K>;
};

/*!
 * @brief Концепт типа, который может модифицировать хранимую строку
 */
template<typename A, typename K>
concept mutable_str = storable_str<A, K> && requires { A::is_str_mutable == true; };

/*!
 * @brief Концепт типа, который не может модифицировать хранимую строку
 */
template<typename A, typename K>
concept immutable_str = storable_str<A, K> && !mutable_str<A, K>;

/*!
 * @brief База для объектов, владеющих строкой
 * @tparam K - тип символов
 * @tparam Impl - конечный класс наследник
 * @tparam Allocator - тип аллокатора
 * @details По прежнему ничего не знает о том, где наследник хранит строку и её размер.
 * Просто вызывает его методы для получения места, и заполняет его при необходимости.
 * Работает только при создании объекта, не работает с модификацией строки после
 * ее создания и гарантирует, что если вызываются эти методы, объект еще только
 * создается, и какого-либо расшаривания данных еще не было.
 *
 * Эти методы должен реализовать класс-наследник, вызываются только при создании объекта
 *   - `K* init(size_t size)`     - выделить место для строки указанного размера, вернуть адрес
 *   - `void create_empty()`      - создать пустой объект
 *   - `K* set_size(size_t size)` - перевыделить место для строки, если при создании не угадали
 *                                  нужный размер и место нужно больше или меньше.
 *                                  Содержимое строки нужно оставить.
 *
 * Хотя тип аллокатора и задаётся параметром шаблона, делается это только для проброса
 * его типа в конструкторы, методы аллокатора не вызываются. Если наследник не пользуется
 * аллокатором, а сам в `init` и `set_size` как-то выделяет место, может указать типом аллокатора
 * какой-либо пустой класс.
 */
template<typename K, typename Impl, typename Allocator>
class str_storable : protected Allocator {
public:
    using my_type = Impl;
    using traits = ch_traits<K>;
    using allocator_t = Allocator;

protected:
    /*!
     * @brief Получить аллокатор
     */
    allocator_t& allocator() {
        return *static_cast<Allocator*>(this);
    }
    const allocator_t& allocator() const {
        return *static_cast<const Allocator*>(this);
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
    explicit constexpr str_storable(size_t size, Args&&... args) : Allocator(std::forward<Args>(args)...) {
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
    // GCC до сих пор не даёт делать полную специализацию вложенного шаблонного класса внутри внешнего класса, только частичную.
    // Поэтому добавим фиктивный параметр шаблона, чтобы сделать специализацию для u8s прямо в классе.
    template<typename T, bool Dummy = true>
    struct ChangeCase {
        template<typename From, typename Op1, typename... Args>
            requires std::is_constructible_v<allocator_t, Args...>
        static my_type changeCase(const From& f, const Op1& opChangeCase, Args&&... args) {
            my_type result{std::forward<Args>(args)...};
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

    /*!
     * @brief Создать пустой объект
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(Args&&... args) noexcept(std::is_nothrow_constructible_v<allocator_t, Args...>)
        : Allocator(std::forward<Args>(args)...) {
        d().create_empty();
    }

    /*!
     * @brief Конструктор из другого строкового объекта
     * @param other - другой строковый объект, simple_str
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(s_str other, Args&&... args) : Allocator(std::forward<Args>(args)...) {
        if (other.length()) {
            K* ptr = d().init(other.length());
            traits::copy(ptr, other.symbols(), other.length());
            ptr[other.length()] = 0;
        } else
            d().create_empty();
    }
    /*!
     * @brief Конструктор повторения строки
     * @param repeat - количество повторов
     * @param pattern - строка, которую надо повторить
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(size_t repeat, s_str pattern, Args&&... args) : Allocator(std::forward<Args>(args)...) {
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
    /*!
     * @brief Конструктор повторения символа
     * @param count - количество повторов
     * @param pad - символ, который надо повторить
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(size_t count, K pad, Args&&... args) : Allocator(std::forward<Args>(args)...) {
        if (count) {
            K* str = d().init(count);
            traits::assign(str, count, pad);
            str[count] = 0;
        } else
            d().create_empty();
    }
    /*!
     * @brief Конструктор из строкового выражения
     * @param expr - строковое выражение
     * @param ...args - параметры для инициализации аллокатора
     * @details Конструктор запрашивает у строкового выражения `length()`,
     *  выделяет память нужного размера, и вызывает метод `place()` для размещения
     *  результата в буфере.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr str_storable(const StrExprForType<K> auto& expr, Args&&... args) : Allocator(std::forward<Args>(args)...) {
        size_t len = expr.length();
        if (len)
            *expr.place(d().init(len)) = 0;
        else
            d().create_empty();
    }
    /*!
     * @brief Конструктор из строкового источника с заменой
     * @param f - строковый объект, из которого берётся исходная строка
     * @param pattern - подстрока, которую надо заменить
     * @param repl  - строка, на которую надо заменить
     * @param offset - начальная позиция для поиска подстрок
     * @param maxCount - максимальное количество замен, 0 - без ограничений
     * @param ...args - параметры для инициализации аллокатора
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    str_storable(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args)
        : Allocator(std::forward<Args>(args)...) {

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
    /*!
     * @brief Оператор преобразования в нуль-терминированную C-строку
     * @return const K* - указатель на начало строки
     */
    operator const K*() const noexcept {
        return d().symbols();
    }
    /*!
     * @brief Получить simple_str_nt, начиная с заданного символа
     * @param from - позиция начального символа, по умолчанию 0
     * @return simple_str_nt
     */
    s_str_nt to_nts(size_t from = 0) const {
        size_t len = d().length();
        if (from >= len) {
            from = len;
        }
        return {d().symbols() + from, len - from};
    }
    /*!
     * @brief Преобразовать в simple_str_nt
     * @return simple_str_nt
     */
    operator s_str_nt() const {
        return {d().symbols(), d().length()};
    }
    /*!
     * @brief Конкатенация строк из контейнера в одну строку
     * @param strings - контейнер со строками
     * @param delimeter - разделитель, добавляемый между строками
     * @param tail - добавить разделитель после последней строки
     * @param skip_empty - пропускать пустые строки без добавления разделителя
     * @param ...args - параметры для инициализации аллокатора
     * @details Функция служит для слияния контейнера строк в одну строку с разделителем.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  ssa delim = get_current_delimeter();
     *  auto line = lstringa<200>::join(strings, delimeter);
     *  ```
     * Стоит отметить, что при заранее известном разделителе лучше пользоваться строковым выражением `e_join`.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  lstringa<200> line{e_join(strings, "/")};
     *  ```
     * В этом случае компилятор может лучше оптимизировать код слияния строк.
     */
    template<typename T, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type join(const T& strings, s_str delimeter, bool tail = false, bool skip_empty = false, Args&&... args) {
        my_type result(std::forward<Args>(args)...);
        if (strings.size()) {
            if (strings.size() == 1 && (!delimeter.length() || !tail)) {
                result = strings.front();
            } else {
                size_t commonLen = 0;
                for (const auto& t: strings) {
                    size_t len = t.length();
                    if (len > 0 || !skip_empty) {
                        if (commonLen > 0) {
                            commonLen += delimeter.len;
                        }
                        commonLen += len;
                    }
                }
                commonLen += (tail && delimeter.len > 0 && (commonLen > 0 || (!skip_empty && strings.size() > 0))? delimeter.len : 0);
                if (commonLen) {
                    K* ptr = result.init(commonLen);
                    K* write = ptr;
                    for (const auto& t: strings) {
                        size_t copyLen = t.length();
                        if (delimeter.len > 0 && write != ptr && (copyLen || !skip_empty)) {
                            ch_traits<K>::copy(write, delimeter.str, delimeter.len);
                            write += delimeter.len;
                        }
                        ch_traits<K>::copy(write, t.symbols(), copyLen);
                        write += copyLen;
                    }
                    if (delimeter.len > 0 && tail && (write != ptr || (!skip_empty && strings.size() > 0))) {
                        ch_traits<K>::copy(write, delimeter.str, delimeter.len);
                        write += delimeter.len;
                    }
                    *write = 0;
                } else {
                    result.create_empty();
                }
            }
        }
        return result;
    }
    /*!
     * @brief Создать строку, копию переданной в верхнем регистре символов ASCII
     * @param f - строка источник
     * @param ...args - параметры для инициализации аллокатора
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiUpper<K>, std::forward<Args>(args)...);
    }
    /*!
     * @brief Создать копию переданной строки в нижнем регистре символов ASCII
     * @param f - строка источник
     * @param ...args - параметры для инициализации аллокатора
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiLower<K>, std::forward<Args>(args)...);
    }
    /*!
     * @brief Создать копию переданной строки в верхнем регистре символов Unicode первой плоскости (<0xFFFF)
     * @param f - строка источник
     * @param ...args - параметры для инициализации аллокатора
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type uppered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::upper, std::forward<Args>(args)...);
    }
    /*!
     * @brief Создать копию переданной строки в нижнем регистре символов Unicode первой плоскости (<0xFFFF)
     * @param f - строка источник
     * @param ...args - параметры для инициализации аллокатора
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::lower, std::forward<Args>(args)...);
    }
    /*!
     * @brief Создать копию переданной строки с заменой подстрок
     * @param f - строка источник
     * @param pattern - подстрока, которую надо заменить
     * @param repl - строка, на которую надо заменить
     * @param offset - начальная позиция для поиска подстрок
     * @param maxCount - максимальное количество замен, 0 - без ограничений
     * @param ...args - параметры для инициализации аллокатора
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type replaced_from(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args) {
        return my_type{f, pattern, repl, offset, maxCount, std::forward<Args>(args)...};
    }
};

/*!
 * @brief Концепт типа, управляющего памятью
 */
template<typename A>
concept Allocatorable = requires(A& a, size_t size, void* void_ptr) {
    { a.allocate(size) } -> std::same_as<void*>;
    { a.deallocate(void_ptr) } noexcept -> std::same_as<void>;
};

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

/*!
 * @brief Базовый класс работы с изменяемыми строками
 * @tparam K - тип символов
 * @tparam Impl - конечный тип наследника
 * @details По прежнему ничего не знает о том, где наследник хранит строку и её размер.
 * Просто вызывает его методы для получения места, и заполняет его при необходимости.
 * Для работы класс-наследник должен реализовать методы:
 *   - `size_t length() const noexcept`      - возвращает длину строки
 *   - `const K* symbols() const`            - возвращает указатель на начало строки
 *   - `bool is_empty() const noexcept`      - проверка, не пустая ли строка
 *   - `K* str() noexcept`                   - Неконстантный указатель на начало строки
 *   - `K* set_size(size_t size)`            - Изменить размер строки, как больше, так и меньше.
 *                                             Содержимое строки нужно оставить.
 *   - `K* reserve_no_preserve(size_t size)` - выделить место под строку, старую можно не сохранять
 *   - `K* alloc_for_copy(size_t size)`      - выделить место для копии строки заданного размера, пока не изменяя
 *                                             саму строку, можно вернуть текущий буфер, если место позволяет.
 *   - `set_from_copy(K* str, size_t size)`  - присвоить строку из памяти, ранее выделенной в alloc_for_copy.
 *                                             Если место выделялось в текущем буфере, ничего не делать.
 *   - `size_t capacity() const noexcept`    - вернуть текущую ёмкость строки, сколько может поместится без аллокации
 */
template<typename K, typename Impl>
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
    using str_piece = simple_str<K>;
    using symb_type = K;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using uns_type = std::make_unsigned_t<K>;

    template<typename Op>
    Impl& make_trim_op(const Op& op) {
        str_piece me = static_cast<str_piece>(d()), pos = op(me);
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
    Impl& makeTrim(str_piece pattern) {
        return make_trim_op(trim_operator<S, K, 0, withSpaces>{{pattern}});
    }

public:
    /*!
     * @brief Получить указатель на буфер строки
     * @return K* - указатель на буфер строки
     */
    K* str() noexcept {
        return d().str();
    }
    /*!
     * @brief Получить указатель на буфер строки
     * @return K* - указатель на буфер строки
     */
    explicit operator K*() noexcept {
        return str();
    }
    /*!
     * @brief Удалить пробельные символы в начале и в конце строки
     * @return Impl& - ссылку на себя же
     */
    Impl& trim() {
        return make_trim_op(SimpleTrim<TrimSides::TrimAll, K>{});
    }
    /*!
     * @brief Удалить пробельные символы в начале строки
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_left() {
        return make_trim_op(SimpleTrim<TrimSides::TrimLeft, K>{});
    }
    /*!
     * @brief Удалить пробельные символы в конце строки
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_right() {
        return make_trim_op(SimpleTrim<TrimSides::TrimRight, K>{});
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, в начале и в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, false>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, в начале строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, false>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, false>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в начале и в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в начале строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right_with_wpaces(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, в начале и в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimAll, false>(pattern) : d();
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, в начале строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_left(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimLeft, false>(pattern) : d();
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_right(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimRight, false>(pattern) : d();
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в начале и в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в начале строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_left_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    /*!
     * @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     */
    Impl& trim_right_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }
    /*!
     * @brief Преобразовать в верхний регистр ASCII символы
     * @return Impl& - ссылку на себя же
     */
    Impl& upper_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiLower(s))
                *ptr = s & ~0x20;
        }
        return d();
    }
    /*!
     * @brief Преобразовать в нижний регистр ASCII символы
     * @return Impl& - ссылку на себя же
     */
    Impl& lower_only_ascii() {
        K* ptr = str();
        for (size_t i = 0, l = _len(); i < l; i++, ptr++) {
            K s = *ptr;
            if (isAsciiUpper(s))
                *ptr = s | 0x20;
        }
        return d();
    }
    /*!
     * @brief Преобразовать в верхний регистр Unicode символы первой плоскости (<0xFFFF).
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @return Impl& - ссылку на себя же
     */
    Impl& upper() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        return CaseTraits<K>::upper(d());
    }
    /*!
     * @brief Преобразовать в нижний регистр Unicode символы первой плоскости (<0xFFFF).
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @return Impl& - ссылку на себя же
     */
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
    /*!
     * @brief Добавить другую строку в конец строки
     * @param other - другая строка
     * @return Impl& - ссылку на себя же
     */
    Impl& append(str_piece other) {
        return appendImpl<str_piece>(other);
    }
    /*!
     * @brief Добавить строковое выражение в конец строки
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     */
    template<StrExprForType<K> A>
    Impl& append(const A& expr) {
        return appendImpl<const A&>(expr);
    }
    /*!
     * @brief Добавить другую строку в конец строки
     * @param other - другая строка
     * @return Impl& - ссылку на себя же
     */
    Impl& operator+=(str_piece other) {
        return appendImpl<str_piece>(other);
    }
    /*!
     * @brief Добавить строковое выражение в конец строки
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     */
    template<StrExprForType<K> A>
    Impl& operator+=(const A& expr) {
        return appendImpl<const A&>(expr);
    }
    /*!
     * @brief Добавить другую строку, начиная с заданной позиции
     * @param pos - позиция, с которой добавлять. Сначала строка укорачивается до заданного
     *        размера, а потом добавляется другая строка
     * @param other - другая строка
     * @return Impl& - ссылку на себя же
     * @details Если строка длиинее`pos`, то она укорачивается до этого размера, а потом добавляется `other`.
     */
    Impl& append_in(size_t pos, str_piece other) {
        return appendFromImpl<str_piece>(pos, other);
    }
    /*!
     * @brief Добавить строковое выражение, начиная с заданной позиции
     * @param pos - позиция, с которой добавлять. Сначала строка укорачивается до заданного
     *        размера, а потом добавляется строковое выражение
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     * @details Если строка длиинее`pos`, то она укорачивается до этого размера, а потом добавляется `expr`.
     */
    template<StrExprForType<K> A>
    Impl& append_in(size_t pos, const A& expr) {
        return appendFromImpl<const A&>(pos, expr);
    }
    /*!
     * @brief Заменить кусок строки на другую строку
     * @param from - начальная позиция для замены
     * @param len - длина заменяемой части
     * @param other - строка, на которую эта часть меняется 
     * @return Impl& - ссылку на себя же
     */
    Impl& change(size_t from, size_t len, str_piece other) {
        return changeImpl<str_piece>(from, len, other);
    }
    /*!
     * @brief Заменить кусок строки на строковое выражение
     * @param from - начальная позиция для замены
     * @param len - длина заменяемой части
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     */
    template<StrExprForType<K> A>
    Impl& change(size_t from, size_t len, const A& expr) {
        return changeImpl<const A&>(from, len, expr);
    }
    /*!
     * @brief Вставить строку в указанную позицию
     * @param to - позиция для вставки
     * @param other - вставляемая строка
     * @return Impl& - ссылку на себя же
     */
    Impl& insert(size_t to, str_piece other) {
        return changeImpl<str_piece>(to, 0, other);
    }
    /*!
     * @brief Вставить строковое выражение в указанную позицию
     * @param to - позиция для вставки
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     */
    template<StrExprForType<K> A>
    Impl& insert(size_t to, const A& expr) {
        return changeImpl<const A&>(to, 0, expr);
    }
    /*!
     * @brief Удалить часть строку
     * @param from - позиция, с которой удалить
     * @param len - длина удаляемой части
     * @return Impl& - ссылку на себя же
     */
    Impl& remove(size_t from, size_t len) {
        return changeImpl<const empty_expr<K>&>(from, len, {});
    }
    /*!
     * @brief Добавить другую строку в начало строки
     * @param other - другая строка
     * @return Impl& - ссылку на себя же
     */
    Impl& prepend(str_piece other) {
        return changeImpl<str_piece>(0, 0, other);
    }
    /*!
     * @brief Добавить строковое выражение в начало строки
     * @param expr - строковое выражение
     * @return Impl& - ссылку на себя же
     */
    template<StrExprForType<K> A>
    Impl& prepend(const A& expr) {
        return changeImpl<const A&>(0, 0, expr);
    }
    /*!
     * @brief Заменить вхождения подстроки на другую строку
     * @param pattern - искомая подстрока
     * @param repl - строка замены
     * @param offset  - начальная позиция для поиска
     * @param maxCount - максимальное количество замен, 0 - без ограничений
     * @return Impl& - ссылку на себя же
     */
    Impl& replace(str_piece pattern, str_piece repl, size_t offset = 0, size_t maxCount = 0) {
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
                replace_grow_helper(my_type& src, str_piece p, str_piece r, size_t mc, size_t d)
                    : source(src), pattern(p), repl(r), maxCount(mc), delta(d) {}
                my_type& source;
                const str_piece pattern;
                const str_piece repl;
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
    /*!
     * @brief Скопировать строку-источник, заменив вхождения подстрок на другую строку
     * @param f - строка-источник
     * @param pattern - искомая подстрока
     * @param repl - строка замены
     * @param offset  - начальная позиция для поиска
     * @param maxCount - максимальное количество замен, 0 - без ограничений
     * @return Impl& - ссылку на себя же
     */
    template<StrType<K> From>
    Impl& replace_from(const From& f, str_piece pattern, str_piece repl, size_t offset = 0, size_t maxCount = 0) {
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
    /*!
     * @brief Заполнение буфера строки с помощью функтора
     * @param from - начальная позиция для заполнения
     * @param fillFunction - size_t(K*, size_t) функтор, получающий адрес буфера строки и его ёмкость,
     *        возвращающий необходимый размер строки
     * @return Impl& - ссылку на себя же
     * @details Функция вызывает функтор, передавая ему адрес буфера строки и его ёмкость.
     *    Функтор может изменять буфер в пределах выделенной ёмкости, и должен вернуть размер итоговой строки.
     *    Пока возвращаемый размер больше ёмкости (т.е. строка не может поместиться в буфер),
     *    выделятся память как минимум возвращенного размера, и функтор вызывается снова.
     *    До тех пор, пока возвращённый размер не будет помещаться в буфер строки.
     *    Этот размер и становится длиной строки.
     */
    template<typename Op>
    Impl& fill(size_t from, const Op& fillFunction) {
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
    /*!
     * @brief Заполняет строку методом fill с нулевой позиции
     * @param fillFunction - функтор заполнения строки, size_t(K*, size_t)
     * @return Impl& - ссылку на себя же
     */
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    Impl& operator<<(const Op& fillFunction) {
        return fill(0, fillFunction);
    }
    /*!
     * @brief Заполняет строку методом fill после конца строки
     * @param fillFunction - функтор заполнения строки, size_t(K*, size_t)
     * @return Impl& - ссылку на себя же
     */
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    Impl& operator<<=(const Op& fillFunction) {
        return fill(_len(), fillFunction);
    }
    /*!
     * @brief Вызывает переданный функтор, передав ссылку на себя
     * @param fillFunction - фуктор void(my_type&)
     * @return Impl& - ссылку на себя же
     */
    template<typename Op>
        requires std::is_invocable_v<Op, my_type&>
    Impl& operator<<(const Op& fillFunction) {
        fillFunction(d());
        return d();
    }
    /*!
     * @brief Добавляет отформатированный с помощью sprintf вывод, начиная с указанной позиции
     * @param from - начальная позиция добавления
     * @param format - форматная строка
     * @param ...args - аргументы для sprintf
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& printf_from(size_t from, const K* format, T&&... args) {
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
    /*!
     * @brief Форматирует строку помощью sprintf
     * @param format - форматная строка
     * @param ...args - аргументы для sprintf
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& printf(const K* format, T&&... args) {
        return printf_from(0, format, std::forward<T>(args)...);
    }
    /*!
     * @brief Добавляет отформатированный с помощью sprintf вывод в конец строки
     * @param format - форматная строка
     * @param ...args - аргументы для sprintf
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_printf(const K* format, T&&... args) {
        return printf_from(_len(), format, std::forward<T>(args)...);
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
    /*!
     * @brief Добавляет отформатированный с помощью std::format вывод, начиная с указанной позиции
     * @param from - начальная позиция добавления
     * @param format - форматная строка, константная
     * @param ...args - аргументы для std::format
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& format_from(size_t from, const FmtString<K, T...>& format, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();
        
        auto result = std::format_to(writer{d(), ptr + from, ptr + capacity, size_t(-1)},
            std::forward<decltype(format)>(format), std::forward<T>(args)...);
        d().set_size(result.ptr - _str());
        return d();
    }
    /*!
     * @brief Добавляет отформатированный с помощью std::vformat вывод, начиная с указанной позиции
     * @param from - начальная позиция добавления
     * @param max_write - максимальное количество записываемых символов
     * @param format - форматная строка
     * @param ...args - аргументы для std::vformat
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& vformat_from(size_t from, size_t max_write, str_piece format, T&&... args) {
        size_t size = _len();
        if (from > size)
            from = size;
        size_t capacity = d().capacity();
        K* ptr = str();

        if constexpr (std::is_same_v<K, u8s>) {
            auto result = std::vformat_to(
                writer{d(), ptr + from, ptr + capacity, max_write},
                std::basic_string_view<K>{format.symbols(), format.length()},
                std::make_format_args(args...));
            d().set_size(result.ptr - _str());
        } else {
            auto result = std::vformat_to(
                writer{d(), to_one_of_std_char(ptr + from), ptr + capacity, max_write},
                std::basic_string_view<wchar_t>{to_one_of_std_char(format.symbols()), format.length()},
                std::make_wformat_args(std::forward<T>(args)...));
            d().set_size(result.ptr - _str());
        }
        return d();
    }
    /*!
     * @brief Форматирует строку с помощью std::format
     * @param format - форматная строка, константная
     * @param ...args - аргументы для std::format
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& format(const FmtString<K, T...>& pattern, T&&... args) {
        return format_from(0, pattern, std::forward<T>(args)...);
    }
    /*!
     * @brief Добавляет отформатированный с помощью std::format вывод в конец строки
     * @param format - форматная строка, константная
     * @param ...args - аргументы для std::format
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_formatted(const FmtString<K, T...>& format, T&&... args) {
        return format_from(_len(), format, std::forward<T>(args)...);
    }
    /*!
     * @brief Форматирует строку с помощью std::vformat
     * @param format - форматная строка
     * @param ...args - аргументы для std::vformat
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& vformat(str_piece format, T&&... args) {
        return vformat_from(0, -1, format, std::forward<T>(args)...);
    }
    /*!
     * @brief Добавляет отформатированный с помощью std::vformat вывод в конец строки
     * @param format - форматная строка
     * @param ... - аргументы для std::vformat
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_vformatted(str_piece format, T&&... args) {
        return vformat_from(_len(), -1, format, std::forward<T>(args)...);
    }
    /*!
     * @brief Форматирует строку с помощью std::vformat не более указанного размера
     * @param max_write - максимальное количество записываемых символов
     * @param format - форматная строка
     * @param ...args - аргументы для std::vformat
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& vformat_n(size_t max_write, str_piece format, T&&... args) {
        return vformat_from(0, max_write, format, std::forward<T>(args)...);
    }
    /*!
     * @brief Добавляет отформатированный с помощью std::vformat вывод в конец строки, записывая не более указанного количества символов
     * @param max_write - максимальное количество записываемых символов
     * @param format - форматная строка
     * @param ...args - аргументы для std::vformat
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_vformatted_n(size_t max_write, str_piece format, T&&... args) {
        return vformat_from(_len(), max_write, format, std::forward<T>(args)...);
    }
    /*!
     * @brief Вызов функтора со строкой и переданными аргументами
     * @param fillFunction - функтор, принимающий первым параметром ссылку на строку
     * @param ...args - аргументы, передаваемые в функтор
     * @return Impl& - ссылку на себя же
     */
    template<typename Op, typename... Args>
    Impl& with(const Op& fillFunction, Args&&... args) {
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

template<typename K, Allocatorable Allocator>
class sstring;

/*
* Так как у класса несколько базовых классов, MSVC не применяет автоматом empty base optimization,
* и без явного указания - вставит в начало класса пустые байты, сдвинув поле size на 4 байта.
* Укажем ему явно
*/

/*!
 * @brief Класс мутабельной, владеющей строки. Содержит внутренний буфер для строк заданного размера.
 * @tparam K - тип символа.
 * @tparam N - размер внутреннего строкового буфера не менее N
 * @tparam forShared - аллоцировать внешний буфер в формате, совместимом с sstring.
 * @tparam Allocator - тип аллокатора
 * @details "Локальная" строка. Хранит в себе указатель на символы и длину строки, а за ней либо сами данные до N
 * символов + нуль, либо если данные длиннее N, то размер выделенного буфера.
 * При этом, если планируется потом результат переместить в sstring, то для динамического буфера
 * выделяется +n байтов, чтобы потом не копировать данные.
 */
template<typename K, size_t N, bool forShared = false, Allocatorable Allocator = allocator_string>
class empty_bases lstring :
    public str_algs<K, simple_str<K>, lstring<K, N, forShared, Allocator>, true>,
    public str_mutable<K, lstring<K, N, forShared, Allocator>>,
    public str_storable<K, lstring<K, N, forShared, Allocator>, Allocator>,
    public from_utf_convertable<K, lstring<K, N, forShared, Allocator>> {
public:
    using symb_type = K;
    using my_type = lstring<K, N, forShared, Allocator>;
    using allocator_t = Allocator;

    enum : size_t {
        LocalCapacity = N | (sizeof(void*) / sizeof(K) - 1),        //!< Размер внутреннего буфера в символах, N выравнивается до `sizeof(void*) / sizeof(K)`
    };

protected:
    enum : size_t {
        extra = forShared ? sizeof(SharedStringData<K>) : 0,
    };

    using base_algs = str_algs<K, simple_str<K>, my_type, true>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_mutable = str_mutable<K, my_type>;
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

    /*!
     * @brief Копирование из другой строки такого же типа
     * @param other - другая строка
     */
    lstring(const my_type& other) : base_storable(other.allocator()) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }
    /*!
     * @brief Копирование из другой строки такого же типа, но с другим аллокатором
     * @param other - другая строка
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_convertible_v<allocator_t, Args...>)
    lstring(const my_type& other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if (other.size_) {
            traits::copy(init(other.size_), other.symbols(), other.size_ + 1);
        }
    }

    /*!
     * @brief Конструктор из строкового литерала
     * @param value - строковый литерал
     * @param ...args - параметры для инициализации аллокатора
     */
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
    /*!
     * @brief Конструктор перемещения из строки такого же типа
     * @param other - другая строка
     */
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
    /*!
     * @brief Конструктор заполнения с помощью функтора (см. str_mutable::fill)
     * @param op - функтов заполнения
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename Op, typename... Args>
        requires(std::is_constructible_v<Allocator, Args...> && (std::is_invocable_v<Op, my_type&> || std::is_invocable_v<Op, K*, size_t>))
    lstring(const Op& op, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        this->operator<<(op);
    }
    
    // copy and swap для присваиваний здесь не очень применимо, так как для строк с большим локальным буфером лишняя копия даже перемещением будет дорого стоить
    // Поэтому реализуем копирующее и перемещающее присваивание отдельно
    
    /*!
     * @brief Оператор присваивания копией из строки такого же типа
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    my_type& operator=(const my_type& other) {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        if (&other != this) {
            traits::copy(reserve_no_preserve(other.size_), other.data_, other.size_ + 1);
            size_ = other.size_;
        }
        return *this;
    }
    /*!
     * @brief Оператор присваивания перемещением из строки такого же типа
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
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
    /*!
     * @brief Оператор присваивания из simple_str
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    my_type& operator=(simple_str<K> other) {
        return assign(other.str, other.len);
    }
    /*!
     * @brief Оператор присваивания строкового литерала
     * @param other - строковый литерал, копируется в буфер строки
     * @return my_type& - ссылку на себя же
     */
    template<typename T, size_t S = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return assign(other, S - 1);
    }
    /*!
     * @brief Оператор присаивания строкового выражения
     * @param other - строковое выражение, материализуемое в буфер строки
     * @return my_type& - ссылку на себя же
     * @details Если в строковом выражении что-либо ссылается на части этой же строки, то результат не определён.
     */
    my_type& operator=(const StrExprForType<K> auto& expr) {
        size_t newLen = expr.length();
        if (newLen) {
            expr.place(reserve_no_preserve(newLen));
        }
        size_ = newLen;
        data_[size_] = 0;
        return *this;
    }
    /// Длина строки
    size_t length() const noexcept {
        return size_;
    }
    /// Указатель на константные символы
    const K* symbols() const noexcept {
        return data_;
    }
    /// Указатель на буфер строки
    K* str() noexcept {
        return data_;
    }
    /// Пустая ли строка
    bool is_empty() const noexcept {
        return size_ == 0;
    }
    /// Пустая ли строка, для совместимости с std::string
    bool empty() const noexcept {
        return size_ == 0;
    }
    /// Текущая ёмкость буфера строки
    size_t capacity() const noexcept {
        return is_alloced() ? capacity_ : LocalCapacity;
    }
    /*!
     * @brief Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
     * @param newSize - новый размер строки
     * @return K* - указатель на буфер
     * @details Содержимое буфера не определено, и не гарантируется сохранение старого содержимого.
     *          Размер строки устанавливается в newSize.
     */
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
    /*!
     * @brief Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
     * @param newSize - новый размер строки
     * @return K* - указатель на буфер
     * @details Содержимое строки сохраняется. При увеличении буфера размер выделяется не больше запрошенного.
     *          Размер строки устанавливается в newSize.
     */
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
    /*!
     * @brief Устанавливает размер текущей строки, при необходимости выделяя место.
     * @param newSize - новый размер строки
     * @return K* - указатель на буфер
     * @details Содержимое строки сохраняется. При увеличении буфера размер выделяется не менее чем 2 старого размера буфера.
     *          Размер строки устанавливается в newSize.
     */
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
    /*!
     * @brief Узнать, локальный или внешний буфер используется для символов
     */
    bool is_local() const noexcept {
        return !is_alloced();
    }
    /*!
     * @brief Определить длину строки.
     * Ищет символ 0 в буфере строки до его ёмкости, после чего устаналивает длину строки по найденному 0.
     */
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
    /*!
     * @brief Уменьшает размер внешнего буфера до минимально возможного для хранения строки.
     * Если строка уместится во внутренний буфер - копирует её в него и освобождает внешний буфер.
     */
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
    /// Делает строку пустой, не меняя буфер строки.
    void clear() {
        set_size(0);
    }
    /// Делает строку пустой и освобождает внешний буфер, если он был
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

/*!
 * @brief Класс иммутабельной владеющей строки
 * @tparam K - тип символов
 * @tparam Allocator - тип аллокатора
 * @details "shared" строка.
 * Класс с small string optimization плюс разделяемый иммутабельный буфер строки.
 * Так как буфер строки в этом классе иммутабельный, то:
 * Во-первых, нет нужды хранить размер выделенного буфера, мы его всё-равно не будем изменять.
 * Во-вторых, появляется ещё один тип строки - строка, инициализированная строковым литералом.
 * Для неё просто сохраняем указатель на символы, и не считаем ссылки.
 * Таким образом, инициализация строкового объекта в программе литералом - ничего никуда не копирует -
 * ни в себя, ни в динамическую память, и не стоит дороже по сравнению с инициализацией
 * сырого указателя на строку, и даже ещё оптимальнее, так как ещё и сразу подставляет размер,
 * а не вычисляет его в рантайме.
 * ```cpp
 *     stringa text = "text or very very very long text"; // ничего не стоит!
 *     stringa copy = anotherString; // Стоит только копирование байтов самого объекта плюс возможно один атомарный инкремент
 * ```
 * В случае разделяемого буфера размер строки всё-равно храним не в общем буфере, а в каждом объекте.
 * Из-за SSO места всё-равно хватает, а в память лезть за длиной придётся меньше.
 * Например, подсчитать сумму длин строк в векторе - пройдётся только по памяти в векторе.
 *
 * Размеры для x64:
 * - для u8s  - 24 байта, хранит строки до 23 символов + 0
 * - для u16s - 32 байта, хранит строки до 15 символов + 0
 * - для u32s - 32 байта, хранит строки до 7 символов + 0
 */

template<typename K, Allocatorable Allocator = allocator_string>
class empty_bases sstring :
    public str_algs<K, simple_str<K>, sstring<K, Allocator>, false>,
    public str_storable<K, sstring<K, Allocator>, Allocator>,
    public from_utf_convertable<K, sstring<K, Allocator>> {
public:
    using symb_type = K;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = sstring<K, Allocator>;
    using allocator_t = Allocator;

    enum { LocalCount = local_count<K> };

protected:
    using base_algs = str_algs<K, simple_str<K>, my_type, false>;
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
            uns_type localRemain_ : sizeof(uns_type) * CHAR_BIT - 2;
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

    /*!
     * @brief Конструктор пустой строки
     * @param ...args - параметры для инициализации аллокатора
     */
    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_constructible_v<Allocator, Args...>)
    sstring(Args&&... args) : Allocator(std::forward<Args>(args)...) {}

    static const sstring<K> empty_str;
    /// Деструктор строки
    ~sstring() {
        if (type_ == Shared) {
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        }
    }
    /*!
     * @brief Конструктор копирования строки
     * @param other - копируемая строка
     */
    sstring(const my_type& other) noexcept : base_storable(other.allocator()) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->incr();
    }
    /*!
     * @brief Конструктор перемещения
     * @param other - перемещаемая строка
     */
    sstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        other.create_empty();
    }

    /*!
     * @brief Конструктор перемещения из lstring с совместимым с sstring внешним буфером
     * @param src - перемещаемая строка
     * @details В случае, если символы в lstring лежат во внешнем аллоцированном буфере,
     *  просто забираем указатель на буфер, он нам подойдёт.
     */
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

    /*!
     * @brief Инициализация из строкового литерала
     * @param s - строковый литерал
     * @param ...args - параметры для инициализации аллокатора
     * @details В этом случае просто запоминаем указатель на строку и её длину.
     */
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
    /*!
     * @brief Оператор присвоения другой строки того же типа
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    my_type& operator=(my_type other) noexcept {
        swap(std::move(other));
        return *this;
    }
    /*!
     * @brief Оператор присвоения другой строки другого типа
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    my_type& operator=(simple_str<K> other) {
        return operator=(my_type{other, base_storable::allocator()});
    }
    /*!
     * @brief Оператор присвоения строкового литерала
     * @param other - строковый литера
     * @return my_type& - ссылку на себя же
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return operator=(my_type{other, base_storable::allocator()});
    }
    /*!
     * @brief Оператор присвоения другой строки типа lstring
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    template<size_t N, bool forShared, typename A>
    my_type& operator=(const lstring<K, N, forShared, A>& other) {
        return operator=(my_type{other.to_str(), base_storable::allocator()});
    }
    /*!
     * @brief Оператор присвоения перемещаемой строки типа lstring с совместимым буфером
     * @param other - другая строка
     * @return my_type& - ссылку на себя же
     */
    template<size_t N>
    my_type& operator=(lstring<K, N, true, Allocator>&& other) {
        return operator=(my_type{std::move(other)});
    }
    /*!
     * @brief Оператор присвоения строкового выражения
     * @param expr - строковое выражения
     * @return my_type& - ссылку на себя же
     * @details В строковом выражение допустимо ссылаться на части этой же строки, так как сначала создаётся копия.
     */
    my_type& operator=(const StrExprForType<K> auto& expr) {
        return operator=(my_type{expr, base_storable::allocator()});
    }
    /*!
     * @brief Сделать строку пустой
     * @return my_type& - ссылку на себя же
     */
    my_type& make_empty() noexcept {
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        create_empty();
        return *this;
    }
    /// Указатель на символы строки
    const K* symbols() const noexcept {
        return type_ == Local ? buf_ : cstr_;
    }
    /// Длина строки
    size_t length() const noexcept {
        return type_ == Local ? LocalCount - localRemain_ : bigLen_;
    }
    /// Пустая ли строка
    bool is_empty() const noexcept {
        return length() == 0;
    }
    /// for std::string compatibility
    bool empty() const noexcept {
        return is_empty();
    }
    /*!
     * @brief Получить строку, отформатированную с помощью `std::sprintf`
     * @param pattern - форматная строка
     * @param ...args  - аргументы для `sprintf`
     * @return my_type
     * @details Для Windows поддерживаются posix позиционные аргументы, используется `_sprintf_p`.
     */
    template<typename... T>
    static my_type printf(const K* pattern, T&&... args) {
        return my_type{lstring<K, 256, true>{}.printf(pattern, std::forward<T>(args)...)};
    }
    /*!
     * @brief Получить строку, отформатированную с помощью `std::format`
     * @param pattern - константная форматная строка
     * @param ...args  - аргументы для `std::format`
     * @return my_type 
     */
    template<typename... T>
    static my_type format(const FmtString<K, T...>& fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.format(fmtString, std::forward<T>(args)...)};
    }
    /*!
     * @brief Получить строку, отформатированную с помощью `std::vformat`
     * @param pattern - форматная строка
     * @param ...args  - аргументы для `std::vformat`
     * @return my_type 
     */
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

/*!
 * @brief Оператор конкатенации для строкового выражения и целого числа.
 * @ingroup StrExprs
 * @param a - строковое выражение
 * @param s - число
 * @details Число конвертируется в десятичное строковое представление.
 */
template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (const A& a, T s) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>>{a, s};
}

/*!
 * @brief Оператор конкатенации для целого числа и строкового выражения.
 * @ingroup StrExprs
 * @param s - число
 * @param a - строковое выражение
 * @details Число конвертируется в десятичное строковое представление.
 */
template<StrExpr A, FromIntNumber T>
inline constexpr auto operator + (T s, const A& a) {
    return strexprjoin_c<A, expr_num<typename A::symb_type, T>, false>{a, s};
}

/*!
 * @brief Преобразование целого числа в строковое выражение
 * @ingroup StrExprs
 * @tparam K - тип символов
 * @tparam T - тип числа, выводится из аргумента
 * @param t - число
 * @details Возвращает строковое выражение, которое генерирует десятичное представление заданного числа.
 * Может использоваться, когда надо конкатенировть число и строковый литерал
 */
template<typename K, typename T>
inline constexpr auto e_num(T t) {
    return expr_num<K, T>{t};
}

template<typename K>
consteval simple_str_nt<K> select_str(simple_str_nt<u8s> s8, simple_str_nt<uws> sw, simple_str_nt<u16s> s16, simple_str_nt<u32s> s32) {
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

/*!
 * @brief Оператор конкатенации для строкового выражения и вещественного числа (`float`, `double`).
 * @ingroup StrExprs
 * @param a - строковое выражение
 * @param s - число
 * @details Число конвертируется в строковое представление через sprintf("%.16g").
 */
template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && (std::is_same_v<R, double> || std::is_same_v<R, float>))
inline constexpr auto operator+(const A& a, R s) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>>{a, s};
}

/*!
 * @brief Оператор конкатенации для вещественного числа (`float`, `double`) и строкового выражения.
 * @ingroup StrExprs
 * @param s - число
 * @param a - строковое выражение
 * @details Число конвертируется в строковое представление через `sprintf("%.16g")`.
 */
template<StrExpr A, typename R>
    requires(is_one_of_std_char_v<typename A::symb_type> && (std::is_same_v<R, double> || std::is_same_v<R, float>))
inline constexpr auto operator+(R s, const A& a) {
    return strexprjoin_c<A, expr_real<typename A::symb_type>, false>{a, s};
}

/*!
 * @brief Преобразование `double` числа в строковое выражение
 * @ingroup StrExprs
 * @param t - число
 * @details Возвращает строковое выражение, которое генерирует десятичное представление заданного числа
 * с помощью `sprintf("%.16g")`. Может использоваться, когда надо конкатенировть число и строковый литерал
 */
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
* skip_empty - пропускать пустые строки без добавления разделителя
*/
template<typename K, typename T, size_t I, bool tail, bool skip_empty>
struct expr_join {
    using symb_type = K;
    using my_type = expr_join<K, T, I, tail, skip_empty>;

    const T& s;
    const K* delim;

    constexpr size_t length() const noexcept {
        size_t l = 0;
        for (const auto& t: s) {
            size_t len = t.length();
            if (len > 0 || !skip_empty) {
                if (I > 0 && l > 0) {
                    l += I;
                }
                l += len;
            }
        }
        return l + (tail && I > 0 && (l > 0 || (!skip_empty && s.size() > 0))? I : 0);
    }
    constexpr K* place(K* ptr) const noexcept {
        if (s.empty()) {
            return ptr;
        }
        K* write = ptr;
        for (const auto& t: s) {
            size_t copyLen = t.length();
            if (I > 0 && write != ptr && (copyLen || !skip_empty)) {
                ch_traits<K>::copy(write, delim, I);
                write += I;
            }
            ch_traits<K>::copy(write, t.symbols(), copyLen);
            write += copyLen;
        }
        if (I > 0 && tail && (write != ptr || (!skip_empty && s.size() > 0))) {
            ch_traits<K>::copy(write, delim, I);
            write += I;
        }
        return write;
    }
};

/*!
 * @brief Получить строковое выражение, конкатенирующее строки в контейнере в одну строку с заданным разделителем
 * @ingroup StrExprs
 * @tparam tail - добавлять ли разделитель после последней строки
 * @tparam skip_empty - пропускать пустые строки без добавления разделителя
 * @param s - контейнер со строками, должен поддерживать `range for`.
 * @param d - разделитель, строковый литерал
 */
template<bool tail = false, bool skip_empty = false, typename L, typename K = typename const_lit<L>::symb_type, size_t I = const_lit<L>::Count, typename T>
inline constexpr auto e_join(const T& s, L&& d) {
    return expr_join<K, T, I - 1, tail, skip_empty>{s, d};
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

/*!
 * @brief Получить строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки
 * @ingroup StrExprs
 * @tparam K - тип символа, выводится из первого аргумента
 * @param w - начальная строка
 * @param p - строковый литерал, искомая подстрока
 * @param r - строковый литерал, на что заменять
 */
template<typename K, typename T, size_t N = const_lit_for<K, T>::Count, typename X, size_t L = const_lit_for<K, X>::Count>
    requires(N > 1)
inline constexpr auto e_repl(simple_str<K> w, T&& p, X&& r) {
    return expr_replaces<K, N - 1, L - 1>{w, p, r};
}

/*!
 * @brief Строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки.
 * @ingroup StrExprs
 * @tparam K - тип строкиs
 * @details `e_repl` позволяет заменять только с использование строковых литералов.
 *  В случае, когда искомая подстрока или строка замены не известны при компиляции, и задаются в runtime,
 *  следует использовать этот тип, например:
 * ```cpp
 *      stringa result = "<header>" + expr_replaced<u8s>{source, pattern, repl} + "</header>";
 * ```
 */
template<typename K>
struct expr_replaced {
    using symb_type = K;
    using my_type = expr_replaced<K>;
    simple_str<K> what;
    const simple_str<K> pattern;
    const simple_str<K> repl;
    mutable size_t first_, last_;
    /*!
     * @brief Конструктор
     * @param w - исходная строка
     * @param p - искомая подстрока
     * @param r - строка замены
     */
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
    size_t count_{};
    std::pair<size_t, size_t> replaces_[16];
};

template<>
struct replace_search_result_store<true> : std::vector<std::pair<size_t, size_t>> {};

/*!
 * @brief Тип для строкового выражения, генерирующее строку, в которой заданные символы заменяются на заданные строки.
 * @ingroup StrExprs
 * @tparam K - тип символа
 * @tparam UseVectorForReplace - использовать вектор для запоминания результатов поиска вхождений символов
 * @details Этот тип применяется, когда состав символов или соответствующих им замен не известен в compile time,
 * а определяется в runtime. В конструктор передается вектор из пар `символ - строка замены`.
 * Параметр `UseVectorForReplace` задаёт стратегию реализации. Дело в том, что работа любых строковых выражений
 * разбита на две фазы - вызов `length()`, в котором подсчитывется количество символов в результате,
 * и вызов `place()`, в котором результат помещается в предоставленный буфер.
 * При `UseVectorForReplace == true` во время фазы подcчёта количества символов, позиции найденных вхождений
 * сохраняются в векторе, и во время второй фазы поиск уже не выполняется, а позиции берутся из вектора.
 * Это, с одной стороны, уменьшает время во второй фазе - не нужно снова выполнять поиск, но увеличивает
 * время в первой фазе - добавление элементов в вектор не бесплатно, и требует времени.
 * При `UseVectorForReplace == false` во время фазы подcчёта количества символов, в локальном массиве запоминается позиции
 * первых 16 вхождений и их общее количество, а во время второй фазы, если вхождений больше 16, то поиск повторяется,
 * но уже только с позиции 16го вхождения. Это может увеличить время во второй фазе, но сокращает время в первой
 * фазе - не нужно добавлять элементы в вектор, не нужна динамическая аллокация.
 * В разных сценариях использования более оптимальными могут быть та или иная стратегия, и вы можете сами решить,
 * что в каждом конкретном случае больше подойдёт.
 */
template<typename K, bool UseVectorForReplace = false>
struct expr_replace_symbols {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;

    const simple_str<K> source_;
    const std::vector<std::pair<K, simple_str<K>>>& replaces_;

    lstring<K, 32> pattern_;

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    uu8s bit_mask_[sizeof(K) == 1 ? 32 : 64]{};
    /*!
     * @brief Конструктор выражения
     * @param source - исходная строка
     * @param repl - вектор из пар "символ->строка замены".
     * @details Пример:
     *  ```cpp
        stringa result = expr_replace_symbols<u8s, true>{source, {
            {'-', ""},
            {'<', "&lt;"},
            {'>', "&gt;"},
            {'\'', "&#39;"},
            {'\"', "&quot;"},
            {'&', "&amp;"},
        }};
     *  ```
     * Пример приведен для наглядности использования. В данном случае и заменяемые символы, и строки замены
     * известны в compile time, и в этом случае лучше применять e_repl_const_symbols, а этот класс
     * используется, когда символы или замены задаются в runtime.
     */
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
        l += replaces_[num].second.len - 1;
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l += replaces_[idx].second.len - 1;
            }
        } else {
            const size_t max_store = std::size(search_results_.replaces_);
            search_results_.replaces_[0] = {fnd, num};
            search_results_.count_++;
            for (size_t start = fnd + 1;;) {
                auto [found, idx] = find_first_of(source_.str, source_.len, start);
                if (found == str::npos) {
                    break;
                }
                if (search_results_.count_ < max_store) {
                    search_results_.replaces_[search_results_.count_] = {found, idx};
                }
                l += replaces_[idx].second.len - 1;
                search_results_.count_++;
                start = found + 1;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        size_t start = 0;
        const K* text = source_.str;
        if constexpr (UseVectorForReplace) {
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].second.place(ptr);
                start = pos + 1;
            }
        } else {
            const size_t max_store = std::size(search_results_.replaces_);
            size_t founded = search_results_.count_;
            for (size_t idx = 0, stop = std::min(founded, max_store); idx < stop; idx++) {
                const auto [pos, num] = search_results_.replaces_[idx];
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].second.place(ptr);
                start = pos + 1;
            }
            if (founded > max_store) {
                founded  -= max_store;
                while (founded--) {
                    auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                    size_t delta = fnd - start;
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                    ptr = replaces_[idx].second.place(ptr);
                    start = fnd + 1;
                }
            }
        }
        size_t tail = source_.len - start;
        ch_traits<K>::copy(ptr, text + start, tail);
        return ptr + tail;
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
        l += replaces_[num].len - 1;
        if constexpr (UseVectorForReplace) {
            search_results_.reserve((l >> 4) + 8);
            search_results_.emplace_back(fnd, num);
            for (size_t start = fnd + 1;;) {
                auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                if (fnd == str::npos) {
                    break;
                }
                search_results_.emplace_back(fnd, idx);
                start = fnd + 1;
                l += replaces_[idx].len - 1;
            }
        } else {
            const size_t max_store = std::size(search_results_.replaces_);
            search_results_.replaces_[0] = {fnd, num};
            search_results_.count_++;
            for (size_t start = fnd + 1;;) {
                auto [found, idx] = find_first_of(source_.str, source_.len, start);
                if (found == str::npos) {
                    break;
                }
                if (search_results_.count_ < max_store) {
                    search_results_.replaces_[search_results_.count_] = {found, idx};
                }
                l += replaces_[idx].len - 1;
                search_results_.count_++;
                start = found + 1;
            }
        }
        return l;
    }
    K* place(K* ptr) const noexcept {
        size_t start = 0;
        const K* text = source_.str;
        if constexpr (UseVectorForReplace) {
            for (const auto& [pos, num] : search_results_) {
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].place(ptr);
                start = pos + 1;
            }
        } else {
            const size_t max_store = std::size(search_results_.replaces_);
            size_t founded = search_results_.count_;
            for (size_t idx = 0, stop = std::min(founded, max_store); idx < stop; idx++) {
                const auto [pos, num] = search_results_.replaces_[idx];
                size_t delta = pos - start;
                ch_traits<K>::copy(ptr, text + start, delta);
                ptr += delta;
                ptr = replaces_[num].place(ptr);
                start = pos + 1;
            }
            if (founded > max_store) {
                founded  -= max_store;
                while (founded--) {
                    auto [fnd, idx] = find_first_of(source_.str, source_.len, start);
                    size_t delta = fnd - start;
                    ch_traits<K>::copy(ptr, text + start, delta);
                    ptr += delta;
                    ptr = replaces_[idx].place(ptr);
                    start = fnd + 1;
                }
            }
        }
        size_t tail = source_.len - start;
        ch_traits<K>::copy(ptr, text + start, tail);
        return ptr + tail;
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

/*!
 * @brief Возвращает строковое выражение, генерирующее строку, в которой заданные символы
 * заменены на заданные подстроки.
 * @ingroup StrExprs
 * @tparam UseVector - использовать вектор для сохранения результатов поиска символов.
 *      Более подробно описано в `expr_replace_symbols`.
 * @param src - исходная строка
 * @param symbol - константный символ, который надо заменять
 * @param repl - строковый литерал, на который заменять символ
 * @param ... symbol, repl - другие символы и строки
 * @details Применяется для генерации замены символов на строки, в случае если все они известны
 * в compile time. Пример:
 *  ```cpp
 *  out += "<div>" + e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;") + "</div>";
 *  ```
 * В принипе, `e_repl_const_symbols` вполне безопасно возвращать из функции, если исходная строка
 * внешняя по отношению к функции
 *  ```cpp
 *  auto repl_html_symbols(ssa text) {
 *      return e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;");
 *  }
 *  ....
 *  out += "<div>" + repl_html_symbols(content) + "</div>";
 *  ```
 */
template<bool UseVector = false, typename K, typename ... Repl>
requires (sizeof...(Repl) % 2 == 0)
auto e_repl_const_symbols(simple_str<K> src, Repl&& ... other) {
    return expr_replace_const_symbols<K, sizeof...(Repl) / 2, UseVector>(src, std::forward<Repl>(other)...);
}

template<typename K, typename H>
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

static_assert(std::is_trivially_copyable_v<StoreType<u8s, int>>, "Store type must be trivially copyable");

template<typename K>
struct streql;
template<typename K>
struct strhash;

/*!
 * @brief Контейнер для более эффективного поиска по строковым ключам.
 * @details Используется для хранения и поиска ключей любого строкового типа.
 * Как unordered_map, но чуть лучше.
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
 *
 * Основное, на чём хотелось бы заострить внимание - само назначение strHashMap, для чего оно изначально придумывалось.
 * Цель была не в том, чтобы как-то побить по производительности/памяти unordered_map (иначе зачем делать strHashMap на
 * базе unordered_map?) Основная проблема, которая решалась - это то, что у нас несколько вариантов строковых объектов,
 * и хотелось бы иметь возможность использовать для вставки и поиска ключей любой из типов строковых объектов.
 * У нас есть simple_str, simple_str_nt, sstring, а lstring<N> вообще на каждое N это новый тип объекта.
 * В std всё было просто - там только string, и соответственно, пользовались unordered_map<string, Что-то там>.
 * И между прочим, в std столкнулись с той же проблемой: ключи const char*, а с C++17 и string_view (это как наш simple_str),
 * искать в unordered_map<string, Type> - не очень хорошо, приходится каждый раз ключ преобразовывать в string, хотя можно
 * обойтись и без этого.
 * Поэтому начиная с С++20 в unordered_map таки добавили возможность гетерогенного поиска - на
 * https://en.cppreference.com/w/cpp/container/unordered_map/find это синтаксисы (3) и (4).
 * Правда, для этого требуются ритуальные танцы с бубном - требуется особый тип для хеширования (который, понятно, в
 * стандарт уже не входит и его надо реализовывать самому), который умеет хешировать разные типы ключей,
 * и содержит typename `is_transparent`. Собственно, на той же странице с ним пример и приведён.
 * Часть проблемы это закрыло - поиска, но проблему вставки не решало - для вставки в unordered_map<string, Type>
 * по прежнему требуется только string. И только в будущем С++26 мы ждём возможности гетерогенной вставки в виде
 * try_emplace (https://en.cppreference.com/w/cpp/container/unordered_map/try_emplace) в синтаксисе (6), который
 * позволит принимать ключи других типов и преобразовывать их в нужный тип только если вставка реально осуществляется.
 * В своей строковой библиотеке я столкнулся с этой проблемой ещё до С++17, и так я как не настолько крут, чтобы
 * влиять на стандарт, просто создал свой класс - наследник от unordered_map, с обёртками вокруг методов вставки/поиска.
 * Ну и дополнительно добавил ещё два варианта хеширования/сравнения: без учёта регистра ASCII, без учёта регистра simple unicode.
 * Таким образом, получился класс, который расширяет стандартный unordered_map возможностями по работе со строковыми ключами разных
 * типов, позволяющий избежать лишних ненужных преобразований ключей при поиске и вставке.
 * То, что при этом может улучшится производительность - не цель, а побочный эффект - приятный, если он есть,
 * но и не смертельный, если его нет.
 */
template<typename K, typename T, typename H = strhash<K>, typename E = streql<K>>
class hashStrMap : public std::unordered_map<StoreType<K, H>, T, H, E> {
protected:
    using InStore = StoreType<K, H>;

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

    hashStrMap(std::initializer_list<std::pair<const InStore, T>>&& init) {
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
    auto try_emplace(const InStore& key, ValArgs&&... args) {
        auto it = hash_t::try_emplace(key, std::forward<ValArgs>(args)...);
        if (it.second) {
            InStore& stored = const_cast<InStore&>(it.first->first);
            new (stored.node) sstring<K>(key.str);
            stored.str.str = stored.to_str().symbols();
        }
        return it;
    }

    static InStore toStoreType(simple_str<K> key) {
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
    auto emplace(const InStore& key, ValArgs&&... args) {
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

    auto& operator[](const InStore& key) {
        return try_emplace(key).first->second;
    }

    template<typename Key>
        requires(std::is_convertible_v<Key, simple_str<K>>)
    auto& operator[](Key&& key) {
        return try_emplace(std::forward<Key>(key)).first->second;
    }

    decltype(auto) at(const InStore& key) {
        return hash_t::at(key);
    }
    decltype(auto) at(const InStore& key) const {
        return hash_t::at(key);
    }

    decltype(auto) at(simple_str<K> key) {
        return hash_t::at(toStoreType(key));
    }
    decltype(auto) at(simple_str<K> key) const {
        return hash_t::at(toStoreType(key));
    }

    auto find(const InStore& key) const {
        return hash_t::find(key);
    }

    auto find(simple_str<K> key) const {
        return find(toStoreType(key));
    }

    auto find(const InStore& key) {
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

    auto erase(const InStore& key) {
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
    bool contains(const InStore& key) const {
        return hash_t::find(key) != this->end();
    }

    bool contains(simple_str<K> key) const {
        return find(toStoreType(key)) != this->end();
    }
};

template<typename K>
struct streql {
    template<typename H>
    bool operator()(const StoreType<K, H>& _Left, const StoreType<K, H>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str == _Right.str;
    }
};

template<typename K>
struct strhash { // hash functor for basic_string
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash(_Keyval.symbols(), _Keyval.length());
    }
    template<typename H>
    size_t operator()(const StoreType<K, H>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqlia {
    template<typename H>
    bool operator()(const StoreType<K, H>& _Left, const StoreType<K, H>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_ia(_Right.str);
    }
};

template<typename K>
struct strhashia {
    size_t operator()(simple_str<K> _Keyval) const {
        return fnv_hash_ia(_Keyval.symbols(), _Keyval.length());
    }
    template<typename H>
    size_t operator()(const StoreType<K, H>& _Keyval) const {
        return _Keyval.hash;
    }
};

template<typename K>
struct streqliu {
    template<typename H>
    bool operator()(const StoreType<K, H>& _Left, const StoreType<K, H>& _Right) const {
        return _Left.hash == _Right.hash && _Left.str.equal_iu(_Right.str);
    }
};

template<typename K>
struct strhashiu {
    size_t operator()(simple_str<K> _Keyval) const {
        return unicode_traits<K>::hashiu(_Keyval.symbols(), _Keyval.length());
    }
    template<typename H>
    size_t operator()(const StoreType<K, H>& _Keyval) const {
        return _Keyval.hash;
    }
};

/*!
 * @brief Для построения длинных динамических строк конкатенацией мелких кусочков.
 * @details Выделяет по мере надобности отдельные блоки заданного размера (или кратного ему для больших вставок),
 * чтобы избежать релокации длинных строк. После построения можно слить в одну строку.
 * Как показали замеры, если сливать потом в одну строку, работает медленнее, чем lstring +=,
 * но экономнее по памяти. Если не сливать в одну строку, а дальше перебирать буфера - быстрее.
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

    ///  Добавление порции данных
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
    /// Добавление строкового выражения
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
    /// Добавление символа
    template<typename T>
    my_type& operator<<(T data)
        requires std::is_same_v<T, K>
    {
        return operator<<(expr_char<K>(data));
    }
    /// Длина сохранённого текста
    constexpr size_t length() const noexcept {
        return len;
    }
    /// Сбрасывает содержимое, но при этом не удаляет первый буфер, чтобы потом избежать аллокации
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
    /*!
     * @brief Применяет функтор к каждому сохранённому буферу
     * @tparam Op - тип функтора, функция вида (const K* ptr, size_t len)
     * @param o - функтор
     */
    template<typename Op>
    void out(const Op& o) const {
        for (const auto& block: chunks)
            o(block.first.get(), block.second);
    }
    /*!
     * @brief Проверяет, расположен ли весь текст одним непрерывным куском в памяти
     */
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
    /*!
     * @brief Получить указатель на начало первого буфера.
     * Имеет смысл применять только если is_continuous true
     */
    const K* begin() const {
        return chunks.size() ? chunks.front().first.get() : simple_str_nt<K>::empty_str.str;
    }
    /*!
     * @brief Очистить объект, освободив все выделенные буфера.
     */
    void clear() {
        chunks.clear();
        write = nullptr;
        len = 0;
        remain = 0;
    }
    /*!
     * @brief Объект, позволяющий последовательно копировать содержимое
     * в буфер заданного размера
     */
    struct portion_store {
        typename decltype(chunks)::const_iterator it, end;
        size_t writedFromCurrentChunk;
        /*!
         * @brief Проверить, что данные ещё не кончились 
         */
        bool is_end() {
            return it == end;
        }
        /*!
         * @brief Сохранить очередную порцию данных в буфер
         * @param buffer - указатель на буфер для сохранения данных
         * @param size - размер буфера
         * @return size_t - количество скопированных СИМВОЛОВ (не байтов).
         */
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
    /*!
     * @brief Получить portion_store, черезк который можно последовательно
     * извлекать данные во внешний буфер
     * @return portion_store 
     */
    portion_store get_portion() const {
        return {chunks.begin(), chunks.end(), 0};
    }
    /*!
     * @brief Получить внутренние буфера с данными
     * @return const auto& 
     */
    const auto& data() const {
        return chunks;
    }
};

using stringa = sstring<u8s>;
using stringw = sstring<wchar_t>;
using stringu = sstring<u16s>;
using stringuu = sstring<u32s>;
static_assert(sizeof(stringa) == 24, "Bad size of sstring");

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

#ifdef _MSC_VER
/* MSVC иногда не может сделать "text"_ss consteval, выдает ошибку C7595.
Находил подобное https://developercommunity.visualstudio.com/t/User-defined-literals-not-constant-expre/10108165
Пишут, что баг исправлен, но видимо не до конца.
Без этого в тестах в двух местах не понимает "text"_ss, хотя в других местах - нормально работает*/
#define SS_CONSTEVAL constexpr
#else
#define SS_CONSTEVAL consteval
#endif

/*!
 * @brief Оператор литерал в simple_str_nt
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return simple_str_nt
 */
SS_CONSTEVAL simple_str_nt<u8s> operator""_ss(const u8s* ptr, size_t l) {
    return simple_str_nt<u8s>{ptr, l};
}

/*!
 * @brief Оператор литерал в simple_str_nt
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return simple_str_nt
 */
SS_CONSTEVAL simple_str_nt<uws> operator""_ss(const uws* ptr, size_t l) {
    return simple_str_nt<uws>{ptr, l};
}

/*!
 * @brief Оператор литерал в simple_str_nt
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return simple_str_nt
 */
SS_CONSTEVAL simple_str_nt<u16s> operator""_ss(const u16s* ptr, size_t l) {
    return simple_str_nt<u16s>{ptr, l};
}

/*!
 * @brief Оператор литерал в simple_str_nt
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return simple_str_nt
 */
SS_CONSTEVAL simple_str_nt<u32s> operator""_ss(const u32s* ptr, size_t l) {
    return simple_str_nt<u32s>{ptr, l};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u8s, strhash<u8s>> operator""_h(const u8s* ptr, size_t l) {
    return StoreType<u8s, strhash<u8s>>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u8s, strhashia<u8s>> operator""_ia(const u8s* ptr, size_t l) {
    return StoreType<u8s, strhashia<u8s>>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
inline StoreType<u8s, strhashiu<u8s>> operator""_iu(const u8s* ptr, size_t l) {
    return StoreType<u8s, strhashiu<u8s>>{{ptr, l}, strhashiu<u8s>{}(simple_str<u8s>{ptr, l})};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u16s, strhash<u16s>> operator""_h(const u16s* ptr, size_t l) {
    return StoreType<u16s, strhash<u16s>>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u16s, strhashia<u16s>> operator""_ia(const u16s* ptr, size_t l) {
    return StoreType<u16s, strhashia<u16s>>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
inline StoreType<u16s, strhashiu<u16s>> operator""_iu(const u16s* ptr, size_t l) {
    return StoreType<u16s, strhashiu<u16s>>{{ptr, l}, strhashiu<u16s>{}(simple_str<u16s>{ptr, l})};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u32s, strhash<u32s>> operator""_h(const u32s* ptr, size_t l) {
    return StoreType<u32s, strhash<u32s>>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
consteval StoreType<u32s, strhashia<u32s>> operator""_ia(const u32s* ptr, size_t l) {
    return StoreType<u32s, strhashia<u32s>>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode
 * @param ptr - указатель на строку
 * @param l - длина строки
 * @return StoreType
 */
inline StoreType<u32s, strhashiu<u32s>> operator""_iu(const u32s* ptr, size_t l) {
    return StoreType<u32s, strhashiu<u32s>>{{ptr, l}, strhashiu<u32s>{}(simple_str<u32s>{ptr, l})};
}

} // namespace literals

/*!
 * @brief Оператор вывода в поток simple_str
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::ostream& operator<<(std::ostream& stream, ssa text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток simple_str
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::wostream& operator<<(std::wostream& stream, ssw text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток simple_str
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::wostream& operator<<(std::wostream& stream, simple_str<wchar_type> text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

/*!
 * @brief Оператор вывода в поток sstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::ostream& operator<<(std::ostream& stream, const stringa& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток sstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::wostream& operator<<(std::wostream& stream, const stringw& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток sstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
inline std::wostream& operator<<(std::wostream& stream, const sstring<wchar_type>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

/*!
 * @brief Оператор вывода в поток lstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::ostream& operator<<(std::ostream& stream, const lstring<u8s, N, S, A>& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток lstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<uws, N, S, A>& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @brief Оператор вывода в поток lstring
 * @param stream - поток вывода
 * @param text - текст
 * @return std::ostream& 
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<wchar_type, N, S, A>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

} // namespace simstr

/*!
 * @brief Форматтер для использования в std::format значений типа simple_str
 */
template<typename K>
struct std::formatter<simstr::simple_str<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

/*!
 * @brief Форматтер для использования в std::format значений типа simple_str_nt
 */
template<typename K>
struct std::formatter<simstr::simple_str_nt<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str_nt<K> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.str, t.len}, fc);
    }
};

/*!
 * @brief Форматтер для использования в std::format значений типа sstring
 */
template<typename K>
struct std::formatter<simstr::sstring<K>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::sstring<K>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};

/*!
 * @brief Форматтер для использования в std::format значений типа lstring
 */
template<typename K, size_t N, bool S, typename A>
struct std::formatter<simstr::lstring<K, N, S, A>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<K, N, S, A>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};
