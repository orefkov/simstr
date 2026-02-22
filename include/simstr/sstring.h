/*
* ver. 1.7.1
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * Классы для работы со строками
* (c) Project "SimStr", Aleksandr Orefkov orefkov@gmail.com
* Classes for working with strings
 */

/*!
 * @ru @mainpage Библиотека simstr.
 * @include{doc} "../readme_ru.md"
 * @page overview Обзор
 * @includedoc{doc} "overview_ru.md"
 * @en @mainpage Simstr lib.
 * @include{doc} "../readme.md"
 * @page overview Overview
 * @includedoc{doc} "overview.md"
 */
#pragma once

#ifndef __has_declspec_attribute
#define __has_declspec_attribute(x) 0
#endif
const bool isWindowsOs = // NOLINT
#ifdef _WIN32
    true
#else
    false
#endif
    ;

#ifdef SIMSTR_IN_SHARED
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

#define IN_FULL_SIMSTR
#include "strexpr.h"

#include <format>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <string.h>
#include <iostream>
#include <cmath>

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
    // These utf-8 operations can change the length of the string
    // Therefore their specializations are different
    // In addition to the text and address of the buffer for writing, the buffer size is passed to the function
    // Returns the length of the resulting string.
    // If the resulting string does not fit into the allocated buffer, pointers are set to the last
    // processed characters to resume work again,
    // and for the remaining characters the required buffer size is calculated.
    static SIMSTR_API size_t upper(const u8s*& src, size_t len, u8s*& dest, size_t lenBuf);
    static SIMSTR_API size_t lower(const u8s*& src, size_t len, u8s*& dest, size_t lenBuf);
    static SIMSTR_API size_t upper_len(const u8s* src, size_t len);
    static SIMSTR_API size_t lower_len(const u8s* src, size_t len);

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

template<>
struct unicode_traits<char8_t> {
    static size_t upper(const ubs*& src, size_t lenStr, ubs*& dest, size_t lenBuf) {
        return unicode_traits<char>::upper((const u8s*&)src, lenStr, (u8s*&)dest, lenBuf);
    }
    static size_t lower(const ubs*& src, size_t len, ubs*& dest, size_t lenBuf) {
        return unicode_traits<char>::lower((const u8s*&)src, len, (u8s*&)dest, lenBuf);
    }
    static size_t upper_len(const ubs* src, size_t len) {
        return unicode_traits<char>::upper_len((const u8s*)src, len);
    }
    static size_t lower_len(const ubs* src, size_t len) {
        return unicode_traits<char>::lower_len((const u8s*)src, len);
    }

    static int compareiu(const char8_t* text1, size_t len1, const char8_t* text2, size_t len2) {
        return unicode_traits<char>::compareiu((const char*)text1, len1, (const char*)text2, len2);
    }
    static size_t hashia(const char8_t* src, size_t s) {
        return unicode_traits<char>::hashia((const char*)src, s);
    }
    static size_t hashiu(const char8_t* src, size_t s) {
        return unicode_traits<char>::hashiu((const char*)src, s);
    }
};

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

template<typename K>
SIMSTR_API std::optional<double> impl_to_double(const K* start, const K* end);

/*!
 * @ru @brief Класс с дополнительными константными строковыми алгоритмами.
 * @details Дополняет алгоритмы из str_src_algs теми, которые связаны с упрощённым юникодом и парсингом double.
 * @tparam K       - тип символов.
 * @tparam StrRef  - тип хранилища куска строки.
 * @tparam Impl    - конечный класс наследник.
 * @en @brief A class with additional constant string algorithms.
 * @details Supplements the algorithms from str_src_algs with those related to simplified Unicode and double parsing.
 * @tparam K - character type.
 * @tparam StrRef - storage type for the string chunk.
 * @tparam Impl - the final class is the successor.
 */
template<typename K, typename StrRef, typename Impl, bool Mutable>
class str_algs : public str_src_algs<K, StrRef, Impl, Mutable> {
    constexpr const Impl& d() const noexcept {
        return *static_cast<const Impl*>(this);
    }
    constexpr size_t _len() const noexcept {
        return d().length();
    }
    constexpr const K* _str() const noexcept {
        return d().symbols();
    }
    constexpr bool _is_empty() const noexcept {
        return d().is_empty();
    }

public:
    using symb_type = K;
    using str_piece = StrRef;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = Impl;
    using base = str_src_algs<K, StrRef, Impl, Mutable>;
    str_algs() = default;

    int compare_iu(const K* text, size_t len) const noexcept { // NOLINT
        if (!len)
            return _is_empty() ? 0 : 1;
        return uni::compareiu(_str(), _len(), text, len);
    }
    /*!
     * @ru @brief Сравнение строк посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF).
     * @param text - другая строка.
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше.
     * @en @brief Compare strings character by character without taking into account the case of Unicode characters of the first plane (<0xFFFF).
     * @param text - another string.
     * @return <0 this string is less, ==0 - strings are equal, >0 - this string is greater.
     */
    int compare_iu(str_piece text) const noexcept { // NOLINT
        return compare_iu(text.symbols(), text.length());
    }
    /*!
     * @ru @brief Равна ли строка другой строке посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF).
     * @param text - другая строка.
     * @return равны ли строки.
     * @en @brief Whether a string is equal to another string, character-by-character-insensitive, of the Unicode characters of the first plane (<0xFFFF).
     * @param text - another string.
     * @return whether the strings are equal.
     */
    bool equal_iu(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_iu(text.symbols(), text.length()) == 0;
    }
    /*!
     * @ru @brief Меньше ли строка другой строки посимвольно без учёта регистра Unicode символов первой плоскости (<0xFFFF).
     * @param text - другая строка.
     * @return меньше ли строка.
     * @en @brief Whether a string is smaller than another string, character-by-character-insensitive, of the Unicode characters of the first plane (<0xFFFF).
     * @param text - another string.
     * @return whether the string is smaller.
     */
    bool less_iu(str_piece text) const noexcept { // NOLINT
        return compare_iu(text.symbols(), text.length()) < 0;
    }
    // Начинается ли эта строка с указанной подстроки без учета unicode регистра
    // Does this string begin with the specified substring, insensitive to unicode case
    bool starts_with_iu(const K* prefix, size_t len) const noexcept {
        return _len() >= len && 0 == uni::compareiu(_str(), len, prefix, len);
    }
    /*!
     * @ru @brief Начинается ли строка с заданной подстроки без учёта регистра Unicode символов первой плоскости (<0xFFFF).
     * @param prefix - подстрока.
     * @en @brief Whether the string starts with the given substring, case-insensitive Unicode characters of the first plane (<0xFFFF).
     * @param prefix - substring.
     */
    bool starts_with_iu(str_piece prefix) const noexcept {
        return starts_with_iu(prefix.symbols(), prefix.length());
    }
    // Заканчивается ли строка указанной подстрокой без учета регистра UNICODE
    // Whether the string ends with the specified substring, case insensitive UNICODE
    constexpr bool ends_with_iu(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return myLen >= len && 0 == uni::compareiu(_str() + myLen - len, len, suffix, len);
    }
    /*!
     * @ru @brief Заканчивается ли строка указанной подстрокой без учёта регистра Unicode символов первой плоскости (<0xFFFF).
     * @param suffix - подстрока.
     * @en @brief Whether the string ends with the specified substring, case-insensitive Unicode characters of the first plane (<0xFFFF).
     * @param suffix - substring.
     */
    constexpr bool ends_with_iu(str_piece suffix) const noexcept {
        return ends_with_iu(suffix.symbols(), suffix.length());
    }
    /*!
     * @ru @brief Получить копию строки в верхнем регистре Unicode символов первой плоскости (<0xFFFF).
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в верхнем регистре.
     * @en @brief Get a copy of the string in upper case Unicode characters of the first plane (<0xFFFF).
     * @tparam R - the desired string type, by default the same whose method was called.
     * @return R - uppercase copy of the string.
     */
    template<typename R = my_type>
    R upperred() const {
        return R::upperred_from(d());
    }
    /*!
     * @ru @brief Получить копию строки в нижнем регистре Unicode символов первой плоскости (<0xFFFF).
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в нижнем регистре.
     * @en @brief Get a copy of the string in lowercase Unicode characters of the first plane (<0xFFFF).
     * @tparam R - the desired string type, by default the same whose method was called.
     * @return R - lowercase copy of the string.
     */
    template<typename R = my_type>
    R lowered() const {
        return R::lowered_from(d());
    }
    /*!
     * @ru @brief Преобразовать строку в double.
     * @return std::optional<double>.
     * @en @brief Convert string to double.
     * @return std::optional<double>.
     */
    template<bool SkipWS = true, bool AllowPlus = true>
    std::optional<double> to_double() const noexcept {
        size_t len = _len();
        const K* ptr = _str();
        if constexpr (SkipWS) {
            while (len && uns_type(*ptr) <= ' ') {
                len--;
                ptr++;
            }
        }
        if constexpr (AllowPlus) {
            if (len && *ptr == K('+')) {
                ptr++;
                len--;
            }
        }
        if (!len) {
            return {};
        }
        #ifdef __linux__
        if constexpr(sizeof(K) == 1) {
            double d{};
            if (std::from_chars((const u8s*)ptr, (const u8s*)ptr + len, d).ec == std::errc{}) {
                return d;
            }
            return {};
        }
        #endif
        if constexpr (sizeof(K) == 1) {
            return impl_to_double((const char*)ptr, (const char*)ptr + len);
        } else if constexpr (sizeof(K) == 2) {
            return impl_to_double((const char16_t*)ptr, (const char16_t*)ptr + len);
        } else {
            return impl_to_double((const char32_t*)ptr, (const char32_t*)ptr + len);
        }
    }
    /*!
     * @ru @brief Преобразовать строку в double.
     * @param t - переменная, в которую записывается результат.
     * @en @brief Convert string to double.
     * @param t - the variable into which the result is written.
     */
    void as_number(double& t) const {
        auto res = to_double();
        t = res ? *res : std::nan("0");
    }
    /*!
     * @ru @brief Преобразовать строку в целое число.
     * @details Так как `as_number(double& t)` перекрывает видимость `as_number` из базового класса,
     *      придётся добавить его ещё раз.
     * @tparam T - тип числа, выводится из аргумента.
     * @param t - переменная, в которую записывается результат.
     * @en @brief Convert a string to an integer.
     * @details Since `as_number(double& t)` overrides the visibility of `as_number` from the base class,
     *      will have to add it again.
     * @tparam T - number type, inferred from the argument.
     * @param t - the variable into which the result is written.
     */
    template<ToIntNumber T>
    constexpr void as_number(T& t) const {
        base::as_number(t);
    }
};

/*
* Базовая структура с информацией о строке.
* Это структура для не владеющих строк.
* Так как здесь только один базовый класс, MSVC компилятор автоматом применяет empty base optimization,
* в результате размер класса не увеличивается
* Basic structure with string information.
* This is the structure for non-owning strings.
* Since there is only one base class, the MSVC compiler automatically applies empty base optimization,
* as a result the class size does not increase
*/

/*!
 * @ru @brief Простейший класс иммутабельной не владеющей строки.
 * @details Аналог std::string_view. Содержит только указатель и длину.
 *          Как наследник от str_algs поддерживает все константные строковые методы.
 * @tparam K - тип символов строки.
 * @en @brief The simplest immutable non-owning string class.
 * @details Similar to std::string_view. Contains only a pointer and a length.
 *      As a descendant of str_algs, it supports all constant string methods.
 * @tparam K - the character type of the string.
 */
template<typename K>
struct simple_str : str_algs<K, simple_str<K>, simple_str<K>, false> {
    using symb_type = K;
    using my_type = simple_str<K>;

    const symb_type* str;
    size_t len;

    constexpr simple_str() = default;

    constexpr simple_str(str_src<K> src) : str(src.str), len(src.len){}

    /*!
     * @ru @brief Конструктор из строкового литерала.
     * @en @brief Constructor from a string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr simple_str(T&& v) noexcept : str((const K*)v), len(N - 1) {}
    /*!
     * @ru @brief Конструктор из указателя и длины.
     * @en @brief Constructor from pointer and length.
     */
    constexpr simple_str(const K* p, size_t l) noexcept : str(p), len(l) {}
    /*!
     *@ru @brief Конструктор из std::basic_string.
     *@en @brief Constructor from std::basic_string.
     */
    template<typename A>
    constexpr simple_str(const std::basic_string<K, std::char_traits<K>, A>& s) noexcept : str(s.data()), len(s.length()) {}
    /*!
     *@ru @brief Конструктор из std::basic_string_view.
     *@en @brief Constructor from std::basic_string_view.
     */
    constexpr simple_str(const std::basic_string_view<K, std::char_traits<K>>& s) noexcept : str(s.data()), len(s.length()) {}
    /*!
     * @ru @brief Получить длину строки.
     * @en @brief Get the length of the string.
     */
    constexpr size_t length() const noexcept {
        return len;
    }
    /*!
     * @ru @brief Получить указатель на константный буфер с символами строки.
     * @en @brief Get a pointer to a constant buffer containing string characters.
     */
    constexpr const symb_type* symbols() const noexcept {
        return str;
    }
    /*!
     * @ru @brief Проверить, не пуста ли строка.
     * @en @brief Check if a string is empty.
     */
    constexpr bool is_empty() const noexcept {
        return len == 0;
    }
    /*!
     * @ru @brief Проверить, не указывают ли два объекта на одну строку.
     * @param other - другая строка.
     * @en @brief Check if two objects point to the same string.
     * @param other - another string.
     */
    constexpr bool is_same(simple_str<K> other) const noexcept {
        return str == other.str && len == other.len;
    }
    /*!
     * @ru @brief Проверить, не является ли строка частью другой строки.
     * @param other - другая строка.
     * @en @brief Check if a string is part of another string.
     * @param other - another string.
     */
    constexpr bool is_part_of(simple_str<K> other) const noexcept {
        return str >= other.str && str + len <= other.str + other.len;
    }
    /*!
     * @ru @brief Получить символ из указанной позиции. Проверка границ не выполняется.
     * @param idx - позиция символа.
     * @return K  - символ.
     * @en @brief Get the character from the specified position. Bounds checking is not performed.
     * @param idx - position of the symbol.
     * @return K is a symbol.
     */
    constexpr K operator[](size_t idx) const {
        return str[idx];
    }
    /*!
     * @ru @brief Сдвигает начало строки на заданное количество символов.
     * @param delta - количество символов.
     * @return my_type&.
     * @en @brief Shifts the start of a string by the specified number of characters.
     * @param delta - number of characters.
     * @return my_type&.
     */
    constexpr my_type& remove_prefix(size_t delta) {
        str += delta;
        len -= delta;
        return *this;
    }
    /*!
     * @ru @brief Укорачивает строку на заданное количество символов.
     * @param delta - количество символов.
     * @return my_type&.
     * @en @brief Shortens the string by the specified number of characters.
     * @param delta - number of characters.
     * @return my_type&.
     */
    constexpr my_type& remove_suffix(size_t delta) {
        len -= delta;
        return *this;
    }
};

template<typename K>
struct simple_str_selector {
    using type = simple_str<K>;
};

/*!
 * @ru @brief Класс, заявляющий, что ссылается на нуль-терминированную строку.
 * @tparam K - тип символов строки.
 * @details Служит для показа того, что функция параметром хочет получить
 *      строку с нулем в конце, например, ей надо дальше передавать его в
 *      стороннее API. Без этого ей надо было бы либо указывать параметром
 *      конкретный класс строки, что лишает универсальности, либо приводило бы
 *      к постоянным накладным расходам на излишнее копирование строк во временный
 *      буфер. Источником нуль-терминированных строк могут быть строковые литералы
 *      при компиляции, либо классы, хранящие строки.
 * @en @brief A class that claims to refer to a null-terminated string.
 * @tparam K - the character type of the string.
 * @details Shows what the function wants to receive as a parameter
 * a string with a zero at the end, for example, she needs to further transfer it to
 * third party API. Without this, she would have to either specify the parameter
 * specific string class, which deprives universality, or would lead
 * to the constant overhead of unnecessary copying of string into the temporary
 * buffer. Null-terminated strings can be sourced from string literals
 * during compilation, or classes that store strings.
 */
template<typename K>
struct simple_str_nt : simple_str<K>, null_terminated<K, simple_str_nt<K>> {
    using symb_type = K;
    using my_type = simple_str_nt<K>;
    using base = simple_str<K>;

    constexpr static const K empty_string[1] = {0};

    simple_str_nt() = default;
    /*!
     * @ru @brief Явный конструктор из С-строки.
     * @param p - указатель на C-строку (нуль-терминированная строка).
     * @details Это единственный конструктор из всех строковых объектов, принимающий C-строку.
     * Вычисляет её длину при инициализации. Все остальные строковые объекты не инициализируются
     * C-строками. Это для того, чтобы `strlen` вызывалась только в одном месте библиотеки,
     * длина C-строки вычислялась только один раз и далее не терялась случайно при передаче между разными
     * типами строковых объектов.
     * @en @brief Explicit constructor from C-string.
     * @param p - pointer to a C-string (null-terminated string).
     * @details This is the only constructor of all string objects that accepts a C-string.
     * Calculates its length upon initialization. All other string objects are not initialized
     * C-strings. This is to ensure that `strlen` is called only in one place in the library,
     * the length of the C-string was calculated only once and was not subsequently lost accidentally when transferred between different
     * types of string objects.
     */
    template<typename T> requires is_one_of_type<std::remove_cvref_t<T>, const K*, K*>::value
    constexpr explicit simple_str_nt(T&& p) noexcept {
        base::len = p ? static_cast<size_t>(base::traits::length(p)) : 0;
        base::str = base::len ? p : empty_string;
    }
    /*!
     * @ru @brief Конструктор из строкового литерала.
     * @en @brief Constructor from a string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr simple_str_nt(T&& v) noexcept : base(std::forward<T>(v)) {}

    /*!
     * @ru @brief Конструктор из указателя и длины.
     * @en @brief Constructor from pointer and length.
     */
    constexpr simple_str_nt(const K* p, size_t l) noexcept : base(p, l) {}

    template<StrType<K> T>
    constexpr simple_str_nt(T&& t) {
        base::str = t.symbols();
        base::len = t.length();
    }
    /*!
     *@ru @brief Конструктор из std::basic_string.
     *@en @brief Constructor from std::basic_string.
     */
    template<typename A>
    constexpr simple_str_nt(const std::basic_string<K, std::char_traits<K>, A>& s) noexcept : base(s) {}

    static const my_type empty_str;
    /*!
     * @ru @brief Получить нуль-терминированную строку, сдвинув начало на заданное количество символов.
     * @param from - на сколько символов сдвинуть начало строки.
     * @return my_type.
     * @en @brief Get a null-terminated string by shifting the start by the specified number of characters.
     * @param from - by how many characters to shift the beginning of the string.
     * @return my_type.
     */
    constexpr my_type to_nts(size_t from) {
        if (from > base::len) {
            from = base::len;
        }
        return {base::str + from, base::len - from};
    }
};

template<typename K>
inline const simple_str_nt<K> simple_str_nt<K>::empty_str{simple_str_nt<K>::empty_string, 0};

template<typename K>
using Splitter = SplitterBase<K, simple_str<K>>;

using ssa = simple_str<u8s>;
using ssb = simple_str<ubs>;
using ssw = simple_str<wchar_t>;
using ssu = simple_str<u16s>;
using ssuu = simple_str<u32s>;
using stra = simple_str_nt<u8s>;
using strb = simple_str_nt<ubs>;
using strw = simple_str_nt<wchar_t>;
using stru = simple_str_nt<u16s>;
using struu = simple_str_nt<u32s>;

template<typename Src, typename Dest>
struct utf_convert_selector;

template<typename Src>
struct utf_convert_selector<Src, Src> {
    static size_t need_len(const Src* src, size_t srcLen) {
        return srcLen;
    }
    static size_t convert(const Src* src, size_t srcLen, Src* dest) {
        ch_traits<Src>::copy(dest, src, srcLen + 1);
        return srcLen;
    }
};

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
struct utf_convert_selector<u32s, u8s> {
    static SIMSTR_API size_t need_len(const u32s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u32s* src, size_t srcLen, u8s* dest);
};

template<>
struct utf_convert_selector<u32s, u16s> {
    static SIMSTR_API size_t need_len(const u32s* src, size_t srcLen);
    static SIMSTR_API size_t convert(const u32s* src, size_t srcLen, u16s* dest);
};

/*!
 * @ru @brief Базовый класс для строк, могущих конвертироваться из другого типа символов.
 * @tparam K - тип символов.
 * @tparam Impl - конечный класс.
 * @details Конвертация выполняется через UTF преобразование.
 *  Считаем, что строки `char` - в UTF-8, `char16_t` - в UTF-16, `char32_t` - в UTF-32.
 *  `wchar_t` - под Windows UTF-16, в Linux - UTF-32.
 * @en @brief Base class for strings that can be converted from another character type.
 * @tparam K - character type.
 * @tparam Impl - final class.
 * @details Conversion is performed via UTF conversion.
 *   We assume that the strings `char` are in UTF-8, `char16_t` - in UTF-16, `char32_t` - in UTF-32.
 *  `wchar_t` - in Windows UTF-16, in Linux - UTF-32.
 */
template<typename K, typename Impl>
class from_utf_convertible {
protected:
    from_utf_convertible() = default;
    using my_type = Impl;
    /*
     Эти методы должен реализовать класс-наследник.
     вызывается только при создании объекта
       init(size_t size)
       set_size(size_t size)
    */
    template<typename O>
        requires(!std::is_same_v<O, K>)
    void init_from_utf_convertible(simple_str<O> init) {
        using from_t = to_base_char_t<O>;
        using to_t = to_base_char_t<K>;

        using worker = utf_convert_selector<from_t, to_t>;
        Impl* d = static_cast<Impl*>(this);
        size_t len = init.length();
        if (!len)
            d->create_empty();
        else {
            size_t need = worker::need_len((const from_t*)init.symbols(), len);
            K* str = d->init(need);
            str[need] = 0;
            worker::convert((const from_t*)init.symbols(), len, (to_t*)str);
        }
    }
};

/*!
 * @ru @brief Строковое выражение для конвертации строк в разные виды UTF.
 * @tparam From - Тип какой строки конвертируем.
 * @tparam To - В какого типа строку конвертируем.
 * @en @brief String expression to convert strings to different UTF types.
 * @tparam From - The type of which string we are converting.
 * @tparam To - What type of string we convert to.
 */
template<typename From, typename To> requires (!std::is_same_v<From, To>)
struct expr_utf : expr_to_std_string<expr_utf<From, To>> {
    using symb_type = To;
    using from_t = to_base_char_t<From>;
    using to_t = to_base_char_t<To>;
    using worker = utf_convert_selector<from_t, to_t>;

    simple_str<From> source_;

    constexpr expr_utf(simple_str<From> source) : source_(source){}

    size_t length() const noexcept {
        return worker::need_len((const from_t*)source_.symbols(), source_.length());
    }
    To* place(To* ptr) const noexcept {
        return ptr + worker::convert((const from_t*)source_.symbols(), source_.length(), (to_t*)ptr);
    }
};

/*!
 * @ru @brief Возвращает строковое выражение, преобразующую строку из одного типа символов
 * в другой тип, через UTF-конвертирование.
 * @tparam To - тип строки, в которую надо конвертировать.
 * @tparam From - тип строки, из которого надо конвертировать. Выводится из аргумента.
 * @param from - строка, из которой надо конвертировать.
 * @en @brief Returns a string expression that converts a string of one character type
 * to another type, via UTF conversion.
 * @tparam To - the type of string to convert to.
 * @tparam From - the type of string to convert from. Derived from the argument.
 * @param from - the string from which to convert.
 */
template<typename To, typename From> requires (!std::is_same_v<From, To>)
expr_utf<From, To> e_utf(simple_str<From> from) {
    return {from};
}

/*!
 * @ru @brief Концепт типа, который может сохранить строку.
 * @en @brief A type concept that can store a string.
 */
template<typename A, typename K>
concept storable_str = requires {
    A::is_str_storable == true;
    std::is_same_v<typename A::symb_type, K>;
};

/*!
 * @ru @brief Концепт типа, который может модифицировать хранимую строку.
 * @en @brief A type concept that can modify a stored string.
 */
template<typename A, typename K>
concept mutable_str = storable_str<A, K> && requires { A::is_str_mutable == true; };

/*!
 * @ru @brief Концепт типа, который не может модифицировать хранимую строку.
 * @en @brief A type concept that cannot modify a stored string.
 */
template<typename A, typename K>
concept immutable_str = storable_str<A, K> && !mutable_str<A, K>;

/*!
 * @ru @brief База для объектов, владеющих строкой.
 * @tparam K - тип символов.
 * @tparam Impl - конечный класс наследник.
 * @tparam Allocator - тип аллокатора.
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
 * Хотя тип аллокатора и задаётся параметром шаблона, делается это только для проброса
 * его типа в конструкторы, методы аллокатора не вызываются. Если наследник не пользуется
 * аллокатором, а сам в `init` и `set_size` как-то выделяет место, может указать типом аллокатора
 * какой-либо пустой класс.
 * @en @brief The base for the objects that own the string.
 * @tparam K - character type.
 * @tparam Impl - the final class is the successor.
 * @tparam Allocator - type of allocator.
 * @details Still knows nothing about where the heir stores the string and its size.
 * Simply calls its methods to get the space, and fills it as needed.
 * Works only when creating an object, does not work with string modification after
 * its creation and ensures that if these methods are called, the object is only
 * is being created and no data sharing has yet taken place.
 *
 * These methods must be implemented by the descendant class and are called only when an object is created
 *  - `K* init(size_t size)`       - allocate space for a string of the specified size, return the address
 *  - `void create_empty()`        - create an empty object
 *  - `K* set_size(size_t size)`   - re-allocate space for the string if you didn’t guess correctly when creating
 *                                   the size you need and the space you need is larger or smaller.
 *                                   The contents of the string must be left.
 * Although the allocator type is specified by the template parameter, this is done only for forwarding
 * of its type in constructors, allocator methods are not called. If the heir does not use
 * an allocator, and in `init` and `set_size` it somehow allocates space, can indicate the type of the allocator
 * any empty class.
 */
template<typename K, typename Impl, typename Allocator>
class str_storable : protected Allocator {
public:
    using my_type = Impl;
    using traits = ch_traits<K>;
    using allocator_t = Allocator;
    using s_str = simple_str<K>;
    using s_str_nt = simple_str_nt<K>;

protected:
    /*!
     * @ru @brief Получить аллокатор.
     * @en @brief Get the allocator.
     */
    constexpr allocator_t& allocator() {
        return *static_cast<Allocator*>(this);
    }
    constexpr const allocator_t& allocator() const {
        return *static_cast<const Allocator*>(this);
    }

    using uni = unicode_traits<K>;

    constexpr Impl& d() noexcept {
        return *static_cast<Impl*>(this);
    }
    constexpr const Impl& d() const noexcept {
        return *static_cast<const Impl*>(this);
    }

    /*!
     * @ru @brief Создать пустой объект.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Create an empty object.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
    explicit constexpr str_storable(Args&&... args) : Allocator(std::forward<Args>(args)...) {}

    /*!
     * @ru @brief Инициализация из другого строкового объекта.
     * @param other - другой строковый объект, simple_str.
     * @en @brief Initialization from another string object.
     * @param other - another string object, simple_str.
     */
    constexpr void init_from_str_other(s_str other) {
        if (other.length()) {
            K* ptr = d().init(other.length());
            traits::copy(ptr, other.symbols(), other.length());
            ptr[other.length()] = 0;
        } else
            d().create_empty();
    }
    /*!
     * @ru @brief Инициализация повторением строки.
     * @param repeat - количество повторов.
     * @param pattern - строка, которую надо повторить.
     * @en @brief String repetition initialization.
     * @param repeat - number of repetitions.
     * @param pattern - the string to be repeated.
     */
    constexpr void init_str_repeat(size_t repeat, s_str pattern) {
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
     * @ru @brief Инициализация повторением символа.
     * @param count - количество повторов.
     * @param pad - символ, который надо повторить.
     * @en @brief Character repetition initialization.
     * @param count - number of repetitions.
     * @param pad - the character to be repeated.
     */
    constexpr void init_symb_repeat(size_t count, K pad) {
        if (count) {
            K* str = d().init(count);
            traits::assign(str, count, pad);
            str[count] = 0;
        } else
            d().create_empty();
    }
    /*!
     * @ru @brief Инициализация из строкового выражения.
     * @param expr - строковое выражение.
     * @details Запрашивает у строкового выражения `length()`,
     *  выделяет память нужного размера, и вызывает метод `place()` для размещения
     *  результата в буфере.
     * @en @brief Initialization from a string expression.
     * @param expr - string expression.
     * @details Queries the string expression `length()`,
     *  allocates memory of the required size, and calls the `place()` method to allocate
     *  result in buffer.
     */
    template<StrExprForType<K> A>
    constexpr void init_str_expr(const A& expr) {
        size_t len = expr.length();
        if (len)
            *expr.place((typename A::symb_type*)d().init(len)) = 0;
        else
            d().create_empty();
    }
    /*!
     * @ru @brief Инициализация из строкового источника с заменой.
     * @param f - строковый объект, из которого берётся исходная строка.
     * @param pattern - подстрока, которую надо заменить.
     * @param repl  - строка, на которую надо заменить.
     * @param offset - начальная позиция для поиска подстрок.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @en @brief Initialization from string source with replacement.
     * @param f - the string object from which the source string is taken.
     * @param pattern - substring to be replaced.
     * @param repl - the string to be replaced with.
     * @param offset - starting position for searching substrings.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     */
    template<StrType<K> From>
    void init_replaced(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0) {
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

    template<StrType<K> From, typename Op1, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type changeCaseAscii(const From& f, const Op1& opMakeNeedCase, Args&&... args) {
        my_type result{std::forward<Args>(args)...};
        size_t len = f.length();
        if (len) {
            const K* source = f.symbols();
            K* destination = result.init(len);
            while(len--) {
                *destination++ = opMakeNeedCase(*source++);
            }
            *destination = 0;
        }
        return result;
    }
    // GCC до сих пор не даёт делать полную специализацию вложенного шаблонного класса внутри внешнего класса, только частичную.
    // Поэтому добавим фиктивный параметр шаблона, чтобы сделать специализацию для u8s прямо в классе.
    // GCC still does not allow full specialization of a nested template class inside an outer class, only partial.
    // So let's add a dummy template parameter to make the specialization for u8s right in the class.
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
    // For utf8 we will make a separate specification, since changing the register may change the length of the string
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
                    // The string was simply shortened
                    result.set_size(newLen);
                } else if (newLen > len) {
                    // Строка не влезла в буфер.
                    // The string did not fit into the buffer.
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

    inline static constexpr bool is_str_storable = true;
    /*!
     * @ru @brief Оператор преобразования в нуль-терминированную C-строку.
     * @return const K* - указатель на начало строки.
     * @en @brief Conversion operator to a null-terminated C string.
     * @return const K* - pointer to the beginning of the string.
     */
    constexpr operator const K*() const noexcept {
        return d().symbols();
    }
    /*!
     * @ru @brief Получить simple_str_nt, начиная с заданного символа.
     * @param from - позиция начального символа, по умолчанию 0.
     * @return simple_str_nt,
     * @en @brief Get simple_str_nt starting at the given character.
     * @param from - position of the starting character, default 0.
     * @return simple_str_nt,
     */
    constexpr s_str_nt to_nts(size_t from = 0) const {
        size_t len = d().length();
        if (from >= len) {
            from = len;
        }
        return {d().symbols() + from, len - from};
    }
    /*!
     * @ru @brief Преобразовать в simple_str_nt.
     * @return simple_str_nt.
     * @en @brief Convert to simple_str_nt.
     * @return simple_str_nt.
     */
    constexpr operator s_str_nt() const {
        return {d().symbols(), d().length()};
    }
    /*!
     * @ru @brief Конкатенация строк из контейнера в одну строку.
     * @param strings - контейнер со строками.
     * @param delimiter - разделитель, добавляемый между строками.
     * @param tail - добавить разделитель после последней строки.
     * @param skip_empty - пропускать пустые строки без добавления разделителя.
     * @param ...args - параметры для инициализации аллокатора.
     * @details Функция служит для слияния контейнера строк в одну строку с разделителем.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  ssa delim = get_current_delimiter();
     *  auto line = lstringa<200>::join(strings, delimiter);
     *  ```
     * Стоит отметить, что при заранее известном разделителе лучше пользоваться строковым выражением `e_join`.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  lstringa<200> line{e_join(strings, "/")};
     *  ```
     * В этом случае компилятор может лучше оптимизировать код слияния строк.
     * @en @brief Concatenate strings from the container into one string.
     * @param strings - container with strings.
     * @param delimiter - delimiter added between lines.
     * @param tail - add a separator after the last string.
     * @param skip_empty - skip empty lines without adding a separator.
     * @param ...args - parameters for initializing the allocator.
     * @details The function is used to merge a container of strings into one delimited string.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  ssa delim = get_current_delimiter();
     *  auto line = lstringa<200>::join(strings, delimiter);
     *  ```
     * It is worth noting that if the separator is known in advance, it is better to use the string expression `e_join`.
     *  ```cpp
     *  std::vector<ssa> strings = get_strings();
     *  lstringa<200> line{e_join(strings, "/")};
     *  ```
     * In this case, the compiler can better optimize the string merging code.
     */
    template<typename T, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type join(const T& strings, s_str delimiter, bool tail = false, bool skip_empty = false, Args&&... args) {
        my_type result(std::forward<Args>(args)...);
        if (strings.size()) {
            if (strings.size() == 1 && (!delimiter.length() || !tail)) {
                result = strings.front();
            } else {
                size_t commonLen = 0;
                for (const auto& t: strings) {
                    size_t len = t.length();
                    if (len > 0 || !skip_empty) {
                        if (commonLen > 0) {
                            commonLen += delimiter.len;
                        }
                        commonLen += len;
                    }
                }
                commonLen += (tail && delimiter.len > 0 && (commonLen > 0 || (!skip_empty && strings.size() > 0))? delimiter.len : 0);
                if (commonLen) {
                    K* ptr = result.init(commonLen);
                    K* write = ptr;
                    for (const auto& t: strings) {
                        size_t copyLen = t.length();
                        if (delimiter.len > 0 && write != ptr && (copyLen || !skip_empty)) {
                            ch_traits<K>::copy(write, delimiter.str, delimiter.len);
                            write += delimiter.len;
                        }
                        ch_traits<K>::copy(write, t.symbols(), copyLen);
                        write += copyLen;
                    }
                    if (delimiter.len > 0 && tail && (write != ptr || (!skip_empty && strings.size() > 0))) {
                        ch_traits<K>::copy(write, delimiter.str, delimiter.len);
                        write += delimiter.len;
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
     * @ru @brief Создать строку, копию переданной в верхнем регистре символов ASCII.
     * @param f - строка источник.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Create a string copy of the passed in uppercase ASCII characters.
     * @param f - source string.
     * @param ...args - parameters for initializing the allocator.
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type upperred_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiUpper<K>, std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Создать копию переданной строки в нижнем регистре символов ASCII.
     * @param f - строка источник.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Create a copy of the passed string in lowercase ASCII characters.
     * @param f - source string.
     * @param ...args - parameters for initializing the allocator.
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_only_ascii_from(const From& f, Args&&... args) {
        return changeCaseAscii(f, makeAsciiLower<K>, std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Создать копию переданной строки в верхнем регистре символов Unicode первой плоскости (<0xFFFF).
     * @param f - строка источник.
     * @param ...args - параметры для инициализации аллокатора.
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @en @brief Create a copy of the passed string in uppercase Unicode characters of the first plane (<0xFFFF).
     * @param f - source string.
     * @param ...args - parameters for initializing the allocator.
     * @details Case is changed by simplified tables, where one code_point is always changed to one code_point
     *          (but for UTF-8 it is possible that the length in code units will change).
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type upperred_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::upper, std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Создать копию переданной строки в нижнем регистре символов Unicode первой плоскости (<0xFFFF).
     * @param f - строка источник.
     * @param ...args - параметры для инициализации аллокатора.
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @en @brief Create a copy of the passed string in lowercase Unicode characters of the first plane (<0xFFFF).
     * @param f - source string.
     * @param ...args - parameters for initializing the allocator.
     * @details Case is changed by simplified tables, where one code_point is always changed to one code_point
     *          (but for UTF-8 it is possible that the length in code units will change).
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type lowered_from(const From& f, Args&&... args) {
        return ChangeCase<K>::changeCase(f, uni::lower, std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Создать копию переданной строки с заменой подстрок.
     * @param f - строка источник.
     * @param pattern - подстрока, которую надо заменить.
     * @param repl - строка, на которую надо заменить.
     * @param offset - начальная позиция для поиска подстрок.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Create a copy of the passed string with substrings replaced.
     * @param f - source string.
     * @param pattern - substring to be replaced.
     * @param repl - the string to be replaced with.
     * @param offset - starting position for searching substrings.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @param ...args - parameters for initializing the allocator.
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    static my_type replaced_from(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args) {
        return my_type{f, pattern, repl, offset, maxCount, std::forward<Args>(args)...};
    }
};

/*!
 * @ru @brief Концепт типа, управляющего памятью
 * @en @brief Concept of a memory management type
 */
template<typename A>
concept Allocatorable = requires(A& a, size_t size, void* void_ptr) {
    { a.allocate(size) } -> std::same_as<void*>;
    { a.deallocate(void_ptr) } noexcept -> std::same_as<void>;
};

struct printf_selector {
    template<typename K, typename... T>  requires (is_one_of_std_char_v<K>)
    static int snprintf(K* buffer, size_t count, const K* format, T&&... args) {
        if constexpr (sizeof(K) == 1) {
          #ifndef _WIN32
            return std::snprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), std::forward<T>(args)...);
          #else
            // Поддерживает позиционные параметры
            // Supports positional parameters
            return _sprintf_p(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #endif
        } else {
          #ifndef _WIN32
            return std::swprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args...);
          #else
            // Поддерживает позиционные параметры
            // Supports positional parameters
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
            // Supports positional parameters
            return _vsprintf_p(buffer, count, format, args);
          #endif
        } else {
          #ifndef _WIN32
            return std::vswprintf(to_one_of_std_char(buffer), count, to_one_of_std_char(format), args);
          #else
            // Поддерживает позиционные параметры
            // Supports positional parameters
            return _vswprintf_p(buffer, count, format, args);
          #endif
        }
    }
};

inline size_t grow2(size_t ret, size_t currentCapacity) {
    return ret <= currentCapacity ? ret : ret * 2;
}

/*!
 * @ru @brief Базовый класс работы с изменяемыми строками
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
 *   - `size_t capacity() const noexcept`    - вернуть текущую ёмкость строки, сколько может поместится без аллокации.
 * @en @brief Base class for working with mutable strings
 * @tparam K - character type
 * @tparam Impl - the final type of the successor
 * @details Still knows nothing about where the heir stores the string and its size.
 * Simply calls its methods to get the space, and fills it as needed.
 * To work, the descendant class must implement the following methods:
 *  - `size_t length() const noexcept`      - returns the length of the string
 *  - `const K* symbols() const`            - returns a pointer to the beginning of the line
 *  - `bool is_empty() const noexcept`      - checks whether the string is empty
 *  - `K* str() noexcept`                   - Non-const pointer to the beginning of the string
 *  - `K* set_size(size_t size)`            - Change the size of the string, either larger or smaller.
 *                                            The contents of the string must be left.
 *  - `K* reserve_no_preserve(size_t size)` - allocate space for a line, you don’t have to save the old one
 *  - `K* alloc_for_copy(size_t size)`      - allocate space for a copy of a string of a given size, without changing it yet
 *                                            the string itself, you can return the current buffer if space allows.
 *  - `set_from_copy(K* str, size_t size)`  - assign a string from memory previously allocated in alloc_for_copy.
 *                                            If space was allocated in the current buffer, do nothing.
 *  - `size_t capacity() const noexcept`    - return the current capacity of the string, as much as can fit without allocation.
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
        str_piece me = d(), pos = op(me);
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
    // GCC still does not allow full specialization of a nested class within a class,
    // only partial. Resources additive unused parameter template.
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
        // For utf-8, such an operation can change the length of the string, so we make different specializations for them
        size_t len = _len();
        if (len) {
            u8s* writePos = str();
            const u8s *startData = writePos, *readPos = writePos;
            size_t newLen = Op(readPos, len, writePos, len);
            if (newLen < len) {
                // Строка просто укоротилась
                // The string was simply shortened
                d().set_size(newLen);
            } else if (newLen > len) {
                // Строка не влезла в буфер.
                // The string did not fit into the buffer.
                size_t readed = static_cast<size_t>(readPos - startData);
                size_t writed = static_cast<size_t>(writePos - startData);
                d().set_size(newLen);
                startData = str(); // при изменении размера могло изменится | may change when resizing
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
     * @ru @brief Получить указатель на буфер строки.
     * @return K* - указатель на буфер строки.
     * @en @brief Get a pointer to the string buffer.
     * @return K* - pointer to the string buffer.
     */
    K* str() noexcept {
        return d().str();
    }
    /*!
     * @ru @brief Получить указатель на буфер строки.
     * @return K* - указатель на буфер строки.
     * @en @brief Get a pointer to the string buffer.
     * @return K* - pointer to the string buffer.
     */
    explicit operator K*() noexcept {
        return str();
    }
    /*!
     * @ru @brief Удалить пробельные символы в начале и в конце строки.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove whitespace from the beginning and end of a string.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim() {
        return make_trim_op(SimpleTrim<TrimSides::TrimAll, K>{});
    }
    /*!
     * @ru @brief Удалить пробельные символы в начале строки.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove whitespace at the beginning of a string.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_left() {
        return make_trim_op(SimpleTrim<TrimSides::TrimLeft, K>{});
    }
    /*!
     * @ru @brief Удалить пробельные символы в конце строки.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove whitespace from the end of a string.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_right() {
        return make_trim_op(SimpleTrim<TrimSides::TrimRight, K>{});
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, в начале и в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove characters included in a string literal at the beginning and end of the string.
     * @param pattern is a string literal containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, false>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, в начале строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove characters included in a string literal at the beginning of the string.
     * @param pattern is a string literal containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, false>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove characters included in a string literal at the end of the string.
     * @param pattern is a string literal containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, false>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в начале и в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in a string literal, as well as whitespace, at the beginning and end of the string.
     * @param pattern is a string literal containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в начале строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in a string literal, as well as whitespace, at the beginning of a string.
     * @param pattern is a string literal containing the characters to be removed.
     * @en @brief* @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_left_with_spaces(T&& pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в строковый литерал, а также пробельные символы, в конце строки.
     * @param pattern - строковый литерал, содержащий символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in a string literal, as well as whitespace, at the end of a string.
     * @param pattern is a string literal containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    Impl& trim_right_with_wpaces(T&& pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, в начале и в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string at the beginning and end of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimAll, false>(pattern) : d();
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, в начале строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string at the beginning of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_left(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimLeft, false>(pattern) : d();
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string from the end of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_right(str_piece pattern) {
        return pattern.length() ? makeTrim<TrimSides::TrimRight, false>(pattern) : d();
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в начале и в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string, as well as whitespace characters, at the beginning and end of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimAll, true>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в начале строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string, as well as whitespace, at the beginning of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_left_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimLeft, true>(pattern);
    }
    /*!
     * @ru @brief Удалить символы, входящие в переданную строку, а также пробельные символы, в конце строки.
     * @param pattern - строка, содержащая символы, которые надо удалить.
     * @return Impl& - ссылку на себя же
     * @en @brief Remove characters included in the passed string, as well as whitespace at the end of the string.
     * @param pattern - a string containing the characters to be removed.
     * @return Impl& - a reference to yourself.
     */
    Impl& trim_right_with_spaces(str_piece pattern) {
        return makeTrim<TrimSides::TrimRight, true>(pattern);
    }
    /*!
     * @ru @brief Преобразовать в верхний регистр ASCII символы.
     * @return Impl& - ссылку на себя же.
     * @en @brief Convert ASCII characters to uppercase.
     * @return Impl& - a reference to yourself.
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
     * @ru @brief Преобразовать в нижний регистр ASCII символы.
     * @return Impl& - ссылку на себя же.
     * @en @brief Convert ASCII characters to lowercase.
     * @return Impl& - a reference to yourself.
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
     * @ru @brief Преобразовать в верхний регистр Unicode символы первой плоскости (<0xFFFF).
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @return Impl& - ссылку на себя же
     * @en @brief Convert first plane characters (<0xFFFF) to uppercase Unicode.
     * @details Case is changed by simplified tables, where one code_point is always changed to one code_point
     *          (but for UTF-8 it is possible that the length in code units will change).
     * @return Impl& - a reference to yourself.
     */
    Impl& upper() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        // For utf-8, such an operation can change the length of the string, so we make different specializations for them
        return CaseTraits<K>::upper(d());
    }
    /*!
     * @ru @brief Преобразовать в нижний регистр Unicode символы первой плоскости (<0xFFFF).
     * @details Регистр меняется упрощенными таблицами, где один code_point всегда меняется в один code_point
     *          (но для UTF-8 возможно, что длина в code unit'ах изменится).
     * @return Impl& - ссылку на себя же
     * @en @brief Convert first plane characters (<0xFFFF) to lowercase Unicode.
     * @details Case is changed by simplified tables, where one code_point is always changed to one code_point
     *          (but for UTF-8 it is possible that the length in code units will change).
     * @return Impl& - a reference to yourself.
     */
    Impl& lower() {
        // Для utf-8 такая операция может изменить длину строки, поэтому для них делаем разные специализации
        // For utf-8, such an operation can change the length of the string, so we make different specializations for them
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
     * @ru @brief Добавить другую строку в конец строки.
     * @param other - другая строка.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add another string to the end of the string.
     * @param other - another string.
     * @return Impl& - a reference to yourself.
     */
    Impl& append(str_piece other) {
        return appendImpl<str_piece>(other);
    }
    /*!
     * @ru @brief Добавить строковое выражение в конец строки.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add a string expression to the end of the string.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     */
    template<StrExprForType<K> A>
    Impl& append(const A& expr) {
        return appendImpl<const A&>(expr);
    }
    /*!
     * @ru @brief Добавить другую строку в конец строки.
     * @param other - другая строка.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add another string to the end of the string.
     * @param other - another string.
     * @return Impl& - a reference to yourself.
     */
    Impl& operator+=(str_piece other) {
        return appendImpl<str_piece>(other);
    }
    /*!
     * @ru @brief Добавить строковое выражение в конец строки.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add a string expression to the end of the string.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     */
    template<StrExprForType<K> A>
    Impl& operator+=(const A& expr) {
        return appendImpl<const A&>(expr);
    }
    /*!
     * @ru @brief Добавить другую строку, начиная с заданной позиции.
     * @param pos - позиция, с которой добавлять. Сначала строка укорачивается до заданного
     *        размера, а потом добавляется другая строка.
     * @param other - другая строка.
     * @return Impl& - ссылку на себя же.
     * @details Если строка длиинее`pos`, то она укорачивается до этого размера, а потом добавляется `other`.
     * @en @brief Add another string starting at the given position.
     * @param pos - the position from which to add. First, the string is shortened to the specified value
     *      size, and then another string is added.
     * @param other - another string.
     * @return Impl& - a reference to yourself.
     * @details If the string is longer than `pos`, then it is shortened to this size, and then `other` is added.
     */
    Impl& append_in(size_t pos, str_piece other) {
        return appendFromImpl<str_piece>(pos, other);
    }
    /*!
     * @ru @brief Добавить строковое выражение, начиная с заданной позиции.
     * @param pos - позиция, с которой добавлять. Сначала строка укорачивается до заданного
     *        размера, а потом добавляется строковое выражение.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @details Если строка длиннее`pos`, то она укорачивается до этого размера, а потом добавляется `expr`.
     * @en @brief Add a string expression starting at the given position.
     * @param pos - the position from which to add. First, the string is shortened to the specified value
     * size, and then a string expression is added.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     * @details If the string is longer than `pos`, then it is shortened to this size, and then `expr` is added.
     */
    template<StrExprForType<K> A>
    Impl& append_in(size_t pos, const A& expr) {
        return appendFromImpl<const A&>(pos, expr);
    }
    /*!
     * @ru @brief Заменить кусок строки на другую строку.
     * @param from - начальная позиция для замены.
     * @param len - длина заменяемой части.
     * @param other - строка, на которую эта часть меняется .
     * @return Impl& - ссылку на себя же
     * @en @brief Replace a piece of string with another string.
     * @param from - starting position for replacement.
     * @param len - length of the part to be replaced.
     * @param other - the string this part is changed to.
     * @return Impl& - a reference to yourself.
     */
    Impl& change(size_t from, size_t len, str_piece other) {
        return changeImpl<str_piece>(from, len, other);
    }
    /*!
     * @ru @brief Заменить кусок строки на строковое выражение.
     * @param from - начальная позиция для замены.
     * @param len - длина заменяемой части.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @en @brief Replace a piece of string with a string expression.
     * @param from - starting position for replacement.
     * @param len - length of the part to be replaced.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     */
    template<StrExprForType<K> A>
    Impl& change(size_t from, size_t len, const A& expr) {
        return changeImpl<const A&>(from, len, expr);
    }
    /*!
     * @ru @brief Вставить строку в указанную позицию.
     * @param to - позиция для вставки.
     * @param other - вставляемая строка.
     * @return Impl& - ссылку на себя же.
     * @en @brief Insert a string at the specified position.
     * @param to - insertion position.
     * @param other - the string to be inserted.
     * @return Impl& - a reference to yourself.
     */
    Impl& insert(size_t to, str_piece other) {
        return changeImpl<str_piece>(to, 0, other);
    }
    /*!
     * @ru @brief Вставить строковое выражение в указанную позицию.
     * @param to - позиция для вставки.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @en @brief Insert a string expression at the specified position.
     * @param to - insertion position.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     */
    template<StrExprForType<K> A>
    Impl& insert(size_t to, const A& expr) {
        return changeImpl<const A&>(to, 0, expr);
    }
    /*!
     * @ru @brief Удалить часть строки.
     * @param from - позиция, с которой удалить.
     * @param len - длина удаляемой части.
     * @return Impl& - ссылку на себя же.
     * @en @brief Remove part of a string.
     * @param from - the position from which to delete.
     * @param len - length of the part to be deleted.
     * @return Impl& - a reference to yourself.
     */
    Impl& remove(size_t from, size_t len) {
        return changeImpl<const empty_expr<K>&>(from, len, {});
    }
    /*!
     * @ru @brief Добавить другую строку в начало строки.
     * @param other - другая строка.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add another string to the beginning of the string.
     * @param other - another string.
     * @return Impl& - a reference to yourself.
     */
    Impl& prepend(str_piece other) {
        return changeImpl<str_piece>(0, 0, other);
    }
    /*!
     * @ru @brief Добавить строковое выражение в начало строки.
     * @param expr - строковое выражение.
     * @return Impl& - ссылку на себя же.
     * @en @brief Add a string expression to the beginning of a string.
     * @param expr - string expression.
     * @return Impl& - a reference to yourself.
     */
    template<StrExprForType<K> A>
    Impl& prepend(const A& expr) {
        return changeImpl<const A&>(0, 0, expr);
    }
    /*!
     * @ru @brief Заменить вхождения подстроки на другую строку.
     * @param pattern - искомая подстрока.
     * @param repl - строка замены.
     * @param offset  - начальная позиция для поиска.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @return Impl& - ссылку на себя же.
     * @en @brief Replace occurrences of a substring with another string.
     * @param pattern - the substring to search for.
     * @param repl - replacement string.
     * @param offset - the starting position for the search.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @return Impl& - a reference to yourself.
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
            // Replace inplace with a substring of the same length
            K* ptr = str();
            while (maxCount--) {
                traits::copy(ptr + offset, repl.symbols(), replLength);
                offset = d().find(pattern, offset + replLength);// replLength == patternLength
                if (offset == str::npos)
                    break;
            }
        } else if (patternLength > replLength) {
            // Заменяем на более короткий кусок, длина текста уменьшится, идём слева направо
            // Replace with a shorter piece, the length of the text will decrease, go from left to right
            K* ptr = str();
            traits::copy(ptr + offset, repl.symbols(), replLength);
            size_t posWrite = offset + replLength;
            offset += patternLength;

            while (--maxCount) {
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
                    size_t found[16] = {offset};
                    maxCount--;
                    offset += pattern.length();
                    all_delta += delta;
                    size_t idx = 1;
                    for (; idx < std::size(found) && maxCount > 0; idx++, maxCount--) {
                        found[idx] = source.find(pattern, offset);
                        if (found[idx] == str::npos) {
                            break;
                        }
                        offset = found[idx] + pattern.length();
                        all_delta += delta;
                    }
                    if (idx == std::size(found) && maxCount > 0 && (offset = source.find(pattern, offset)) != str::npos) {
                        replace(offset); // здесь произойдут замены в оставшемся хвосте | replacements will be made here in the remaining tail
                    }
                    // Теперь делаем свои замены
                    // Now we make our replacements
                    if (!reserve_for_copy) {
                        // Только начинаем
                        // Just getting started
                        end_of_piece = source.length();
                        total_length = end_of_piece + all_delta;
                        reserve_for_copy = source.alloc_for_copy(total_length);
                    }
                    K* dst_start = reserve_for_copy;
                    const K* src_start = source.symbols();
                    while(idx-- > 0) {
                        size_t pos = found[idx] + pattern.length();
                        size_t lenOfPiece = end_of_piece - pos;
                        ch_traits<K>::move(dst_start + pos + all_delta, src_start + pos, lenOfPiece);
                        ch_traits<K>::copy(dst_start + pos + all_delta - repl.length(), repl.symbols(), repl.length());
                        all_delta -= delta;
                        end_of_piece = found[idx];
                    }
                    if (!all_delta && reserve_for_copy != src_start) {
                        ch_traits<K>::copy(dst_start, src_start, found[0]);
                    }
                }
            } helper(d(), pattern, repl, maxCount, repl.length() - pattern.length());
            helper.replace(offset);
            d().set_from_copy(helper.reserve_for_copy, helper.total_length);
        }
        return d();
    }
    /*!
     * @ru @brief Скопировать строку-источник, заменив вхождения подстрок на другую строку.
     * @param f - строка-источник.
     * @param pattern - искомая подстрока.
     * @param repl - строка замены.
     * @param offset  - начальная позиция для поиска.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @return Impl& - ссылку на себя же.
     * @en @brief Copy the source string, replacing occurrences of substrings with another string.
     * @param f - source string.
     * @param pattern - the substring to search for.
     * @param repl - replacement string.
     * @param offset - the starting position for the search.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @return Impl& - a reference to yourself.
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
     * @ru @brief Заполнение буфера строки с помощью функтора.
     * @param from - начальная позиция для заполнения.
     * @param fillFunction - size_t(K*, size_t) функтор, получающий адрес буфера строки и его ёмкость,
     *        возвращающий необходимый размер строки.
     * @return Impl& - ссылку на себя же.
     * @details Функция вызывает функтор, передавая ему адрес буфера строки и его ёмкость.
     *    Функтор может изменять буфер в пределах выделенной ёмкости, и должен вернуть размер итоговой строки.
     *    Пока возвращаемый размер больше ёмкости (т.е. строка не может поместиться в буфер),
     *    выделятся память как минимум возвращенного размера, и функтор вызывается снова.
     *    До тех пор, пока возвращённый размер не будет помещаться в буфер строки.
     *    Этот размер и становится длиной строки.
     * @en @brief Fill a string buffer using a functor.
     * @param from - starting position to fill.
     * @param fillFunction - size_t(K*, size_t) functor that receives the address of the string buffer and its capacity,
     * returning the required string size.
     * @return Impl& - a reference to yourself.
     * @details The function calls the functor, passing it the address of the string buffer and its capacity.
     *    The functor can modify the buffer within the allocated capacity, and must return the size of the resulting string.
     *    As long as the returned size is larger than capacity (i.e. the string cannot fit into the buffer),
     *    memory of at least the returned size is allocated and the functor is called again.
     *    Until the returned size fits into the string buffer.
     *    This size becomes the length of the string.
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
     * @ru @brief Заполняет строку методом fill с нулевой позиции.
     * @param fillFunction - функтор заполнения строки, size_t(K*, size_t).
     * @return Impl& - ссылку на себя же.
     * @en @brief Fills a string with the fill method from position zero.
     * @param fillFunction - string filling functor, size_t(K*, size_t).
     * @return Impl& - a reference to yourself.
     */
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    Impl& operator<<(const Op& fillFunction) {
        return fill(0, fillFunction);
    }
    /*!
     * @ru @brief Заполняет строку методом fill после конца строки.
     * @param fillFunction - функтор заполнения строки, size_t(K*, size_t).
     * @return Impl& - ссылку на себя же.
     * @en @brief Fills a string with the fill method after the end of the string.
     * @param fillFunction - string filling functor, size_t(K*, size_t).
     * @return Impl& - a reference to yourself.
     */
    template<typename Op>
        requires std::is_invocable_v<Op, K*, size_t>
    Impl& operator<<=(const Op& fillFunction) {
        return fill(_len(), fillFunction);
    }
    /*!
     * @ru @brief Вызывает переданный функтор, передав ссылку на себя.
     * @param fillFunction - фуктор void(my_type&).
     * @return Impl& - ссылку на себя же.
     * @en @brief Calls the passed functor, passing a reference to itself.
     * @param fillFunction - фуктор void(my_type&).
     * @return Impl& - a reference to yourself.
     */
    template<typename Op>
        requires std::is_invocable_v<Op, my_type&>
    Impl& operator<<(const Op& fillFunction) {
        fillFunction(d());
        return d();
    }
    /*!
     * @ru @brief Добавляет отформатированный с помощью sprintf вывод, начиная с указанной позиции.
     * @param from - начальная позиция добавления.
     * @param format - форматная строка.
     * @param ...args - аргументы для sprintf.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends sprintf formatted output starting at the specified position.
     * @param from - starting position of adding.
     * @param format - format string.
     * @param ...args - arguments for sprintf.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
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
        // Here's a dirty hack for u8s and wide_char. u8s version of snprintf immediately returns the size of the required buffer if it is small
        // and swprintf returns -1. Under Windows, both options xxx_p also return -1.
        // Therefore, for them you need to stupidly increase the buffer at random until it fits
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
                    // Not enough buffer or conversion error.
                    // Let's try to double the buffer
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
     * @ru @brief Форматирует строку помощью sprintf.
     * @param format - форматная строка.
     * @param ...args - аргументы для sprintf.
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Formats a string using sprintf.
     * @param format - format string.
     * @param ...args - arguments for sprintf.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& printf(const K* format, T&&... args) {
        return printf_from(0, format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Добавляет отформатированный с помощью sprintf вывод в конец строки.
     * @param format - форматная строка.
     * @param ...args - аргументы для sprintf.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends sprintf formatted output to the end of the string.
     * @param format - format string.
     * @param ...args - arguments for sprintf.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the row buffer size if necessary.
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
    using fmt_type = to_std_char_t<K>;
    /*!
     * @ru @brief Добавляет отформатированный с помощью std::format вывод, начиная с указанной позиции.
     * @param from - начальная позиция добавления.
     * @param format - форматная строка, константная.
     * @param ...args - аргументы для std::format.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends std::format-formatted output starting at the specified position.
     * @param from - starting position of adding.
     * @param format - format string, constant.
     * @param ...args - arguments for std::format.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& format_from(size_t from, const FmtString<fmt_type, T...>& format, T&&... args) {
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
     * @ru @brief Добавляет отформатированный с помощью std::vformat вывод, начиная с указанной позиции.
     * @param from - начальная позиция добавления.
     * @param max_write - максимальное количество записываемых символов.
     * @param format - форматная строка.
     * @param ...args - аргументы для std::vformat.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends std::vformat formatted output starting at the specified position.
     * @param from - starting position of adding.
     * @param max_write - the maximum number of characters to write.
     * @param format - format string.
     * @param ...args - arguments for std::vformat.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
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
     * @ru @brief Форматирует строку с помощью std::format.
     * @param pattern - форматная строка, константная.
     * @param ...args - аргументы для std::format.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Formats a string using std::format.
     * @param pattern - format string, constant.
     * @param ...args - arguments for std::format.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& format(const FmtString<fmt_type, T...>& pattern, T&&... args) {
        return format_from(0, pattern, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Добавляет отформатированный с помощью std::format вывод в конец строки.
     * @param format - форматная строка, константная.
     * @param ...args - аргументы для std::format.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends std::format-formatted output to the end of the string.
     * @param format - format string, constant.
     * @param ...args - arguments for std::format.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_formatted(const FmtString<fmt_type, T...>& format, T&&... args) {
        return format_from(_len(), format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Форматирует строку с помощью std::vformat.
     * @param format - форматная строка.
     * @param ...args - аргументы для std::vformat.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Formats a string using std::vformat.
     * @param format - format string.
     * @param ...args - arguments for std::vformat.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& vformat(str_piece format, T&&... args) {
        return vformat_from(0, -1, format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Добавляет отформатированный с помощью std::vformat вывод в конец строки.
     * @param format - форматная строка.
     * @param ... - аргументы для std::vformat.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends std::vformat-formatted output to the end of the string.
     * @param format - format string.
     * @param ... - arguments for std::vformat.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_vformatted(str_piece format, T&&... args) {
        return vformat_from(_len(), -1, format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Форматирует строку с помощью std::vformat не более указанного размера.
     * @param max_write - максимальное количество записываемых символов.
     * @param format - форматная строка.
     * @param ...args - аргументы для std::vformat.
     * @return Impl& - ссылку на себя же.
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Formats a string using std::vformat up to the specified size.
     * @param max_write - the maximum number of characters to write.
     * @param format - format string.
     * @param ...args - arguments for std::vformat.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& vformat_n(size_t max_write, str_piece format, T&&... args) {
        return vformat_from(0, max_write, format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Добавляет отформатированный с помощью std::vformat вывод в конец строки, записывая не более указанного количества символов.
     * @param max_write - максимальное количество записываемых символов.
     * @param format - форматная строка.
     * @param ...args - аргументы для std::vformat.
     * @return Impl& - ссылку на себя же
     * @details При необходимости автоматически увеличивает размер буфера строки.
     * @en @brief Appends std::vformat-formatted output to the end of the line, writing no more than the specified number of characters.
     * @param max_write - the maximum number of characters to write.
     * @param format - format string.
     * @param ...args - arguments for std::vformat.
     * @return Impl& - a reference to yourself.
     * @details Automatically increases the string buffer size if necessary.
     */
    template<typename... T> requires (is_one_of_std_char_v<K>)
    Impl& append_vformatted_n(size_t max_write, str_piece format, T&&... args) {
        return vformat_from(_len(), max_write, format, std::forward<T>(args)...);
    }
    /*!
     * @ru @brief Вызов функтора со строкой и переданными аргументами.
     * @param fillFunction - функтор, принимающий первым параметром ссылку на строку.
     * @param ...args - аргументы, передаваемые в функтор.
     * @return Impl& - ссылку на себя же.
     * @en @brief Call a functor with a string and passed arguments.
     * @param fillFunction - a functor that takes a string reference as its first parameter.
     * @param ...args - arguments passed to the functor.
     * @return Impl& - a reference to yourself.
     */
    template<typename Op, typename... Args>
    Impl& with(const Op& fillFunction, Args&&... args) {
        fillFunction(d(), std::forward<Args>(args)...);
        return d();
    }
};

template<typename K>
struct SharedStringData {
    std::atomic_size_t ref_; // Счетчик ссылок | Reference count

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
// Default allocator for strings, can work statically
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
// If you want to set your default allocator for strings, before including sstring.h
// declare a function
// your_allocator_type default_string_allocator_selector(int);
using allocator_string = decltype(default_string_allocator_selector(int(0)));

template<typename K, Allocatorable Allocator>
class sstring;

/*
* Так как у класса несколько базовых классов, MSVC не применяет автоматом empty base optimization,
* и без явного указания - вставит в начало класса пустые байты, сдвинув поле size на 4-8 байта.
* Укажем ему явно.
* Since a class has several base classes, MSVC does not automatically apply empty base optimization,
* and without explicit indication - will insert empty bytes at the beginning of the class, shifting the size field by 4-8 bytes.
* Let's tell him explicitly.
*/

/*!
 * @ru @brief Класс мутабельной, владеющей строки. Содержит внутренний буфер для строк заданного размера.
 * @tparam K - тип символа.
 * @tparam N - размер внутреннего строкового буфера не менее N.
 * @tparam forShared - аллоцировать внешний буфер в формате, совместимом с sstring.
 * @tparam Allocator - тип аллокатора.
 * @details "Локальная" строка. Хранит в себе указатель на символы и длину строки, а за ней либо сами данные до N
 * символов + нуль, либо если данные длиннее N, то размер выделенного буфера.
 * При этом, если планируется потом результат переместить в sstring, то для динамического буфера
 * выделяется +n байтов, чтобы потом не копировать данные.
 * @en @brief The mutable, owning string class. Contains an internal buffer for text of a given size.
 * @tparam K - symbol type.
 * @tparam N - the size of the internal string buffer is at least N.
 * @tparam forShared - allocate an external buffer in a format compatible with sstring.
 * @tparam Allocator - allocator type.
 * @details "Local" string. Stores a pointer to characters and the length of the string, followed by either the data itself up to N
 * characters + zero, or if the data is longer than N, then the size of the allocated buffer.
 * At the same time, if you plan to later move the result to sstring, then for a dynamic buffer
 * +n bytes are allocated so as not to copy the data later.
 */
template<typename K, size_t N, bool forShared = false, Allocatorable Allocator = allocator_string>
class decl_empty_bases lstring :
    public str_algs<K, simple_str<K>, lstring<K, N, forShared, Allocator>, true>,
    public str_mutable<K, lstring<K, N, forShared, Allocator>>,
    public str_storable<K, lstring<K, N, forShared, Allocator>, Allocator>,
    public null_terminated<K, lstring<K, N, forShared, Allocator>>,
    public from_utf_convertible<K, lstring<K, N, forShared, Allocator>> {
public:
    using symb_type = K;
    using my_type = lstring<K, N, forShared, Allocator>;
    using allocator_t = Allocator;

    enum : size_t {
        /// @ru Размер внутреннего буфера в символах @en Size of internal buffer
        LocalCapacity = N | (sizeof(void*) / sizeof(K) - 1),
    };

protected:
    enum : size_t {
        extra = forShared ? sizeof(SharedStringData<K>) : 0,
    };

    using base_algs = str_algs<K, simple_str<K>, my_type, true>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_mutable = str_mutable<K, my_type>;
    using base_utf = from_utf_convertible<K, my_type>;
    using traits = ch_traits<K>;
    using s_str = base_storable::s_str;

    friend base_storable;
    friend base_mutable;
    friend base_utf;
    friend class sstring<K, Allocator>;

    K* data_;
    size_t size_;

    union {
        size_t capacity_;
        K local_[LocalCapacity + 1];
    };

    constexpr void create_empty() {
        data_ = local_;
        size_ = 0;
        local_[0] = 0;
    }
    constexpr static size_t calc_capacity(size_t s) {
        const int al = alignof(std::max_align_t) < 16 ? 16 : alignof(std::max_align_t);
        size_t real_need = (s + 1) * sizeof(K) + extra;
        size_t aligned_alloced = (real_need + al - 1) / al * al;
        return (aligned_alloced - extra) / sizeof(K) - 1;
    }

    constexpr K* init(size_t s) {
        size_t need_cap = s;
        if (need_cap > LocalCapacity) {
            need_cap = calc_capacity(s);
            data_ = alloc_place(need_cap);
            capacity_ = need_cap;
        } else {
            data_ = local_;
        }
        size_ = s;
        return str();
    }
    // Методы для себя | Methods for yourself
    constexpr bool is_alloced() const noexcept {
        return data_ != local_;
    }

    constexpr void dealloc() {
        if (is_alloced()) {
            base_storable::allocator().deallocate(to_real_address(data_));
            data_ = local_;
        }
    }

    constexpr static K* to_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) - extra);
    }
    constexpr static K* from_real_address(void* ptr) {
        return reinterpret_cast<K*>(reinterpret_cast<u8s*>(ptr) + extra);
    }

    constexpr K* alloc_place(size_t newSize) {
        return from_real_address(base_storable::allocator().allocate((newSize + 1) * sizeof(K) + extra));
    }
    // Вызывается при replace, когда меняют на более длинную замену
    // Called on replace when changing to a longer replacement
    constexpr K* alloc_for_copy(size_t newSize) {
        if (capacity() >= newSize) {
            // Замена войдёт в текущий буфер
            // Replacement will go into the current buffer
            return data_;
        }
        return alloc_place(calc_capacity(newSize));
    }
    // Вызывается после replace, когда меняли на более длинную замену, могли скопировать в новый буфер
    // Called after replace, when they changed to a longer replacement, they could have copied it to a new buffer
    constexpr void set_from_copy(K* ptr, size_t newSize) {
        if (ptr != data_) {
            // Да, копировали в новый буфер
            // Yes, copied to a new buffer
            dealloc();
            data_ = ptr;
            capacity_ = calc_capacity(newSize);
        }
        size_ = newSize;
        data_[newSize] = 0;
    }
    constexpr void copy_from_another(K* buf, const K* src, size_t size) {
        // Реальный размер буфера всегда кратен sizeof(void*), поэтому копируя по sizeof(void*) байтов, мы не выйдем за пределы буфера
        // The actual buffer size is always a multiple of sizeof(void*), so by copying sizeof(void*) bytes at a time, we won't go beyond the buffer's limits.
        size_t need_copy_bytes = (size + 1) * sizeof(K);
        size_t cnt = (need_copy_bytes + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
        traits::copy(buf, src, cnt / sizeof(K));
    }
public:
    /*!
     * @ru @brief Создать пустой объект.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Create an empty object.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires (std::is_constructible_v<allocator_t, Args...> && sizeof...(Args) > 0)
    constexpr lstring(Args&&... args) noexcept(std::is_nothrow_constructible_v<allocator_t, Args...>)
        : base_storable(std::forward<Args>(args)...) {
        create_empty();
    }

    /*!
     * @ru @brief Конструктор из другого строкового объекта.
     * @param other - другой строковый объект, simple_str.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief A constructor from another string object.
     * @param other - another string object, simple_str.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(s_str other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_from_str_other(other);
    }
    /*!
     * @ru @brief Конструктор повторения строки.
     * @param repeat - количество повторов.
     * @param pattern - строка, которую надо повторить.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief String repetition constructor.
     * @param repeat - number of repetitions.
     * @param pattern - the string to be repeated.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(size_t repeat, s_str pattern, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_str_repeat(repeat, pattern);
    }
    /*!
     * @ru @brief Конструктор повторения символа.
     * @param count - количество повторов.
     * @param pad - символ, который надо повторить.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Character repetition constructor.
     * @param count - number of repetitions.
     * @param pad - the character to be repeated.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(size_t count, K pad, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_symb_repeat(count, pad);
    }
    /*!
     * @ru @brief Конструктор из строкового выражения.
     * @param expr - строковое выражение.
     * @param ...args - параметры для инициализации аллокатора.
     * @details Конструктор запрашивает у строкового выражения `length()`,
     *  выделяет память нужного размера, и вызывает метод `place()` для размещения
     *  результата в буфере.
     * @en @brief Constructor from a string expression.
     * @param expr - string expression.
     * @param ...args - parameters for initializing the allocator.
     * @details The constructor queries the string expression `length()`,
     *  allocates memory of the required size, and calls the `place()` method to allocate
     *  result in buffer.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(const StrExprForType<K> auto& expr, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_str_expr(expr);
    }
    /*!
     * @ru @brief Конструктор из строкового источника с заменой.
     * @param f - строковый объект, из которого берётся исходная строка.
     * @param pattern - подстрока, которую надо заменить.
     * @param repl  - строка, на которую надо заменить.
     * @param offset - начальная позиция для поиска подстрок.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Constructor from string source with replacement.
     * @param f - the string object from which the source string is taken.
     * @param pattern - substring to be replaced.
     * @param repl - the string to be replaced with.
     * @param offset - starting position for searching substrings.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @param ...args - parameters for initializing the allocator.
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args)
        : base_storable(std::forward<Args>(args)...) {
        base_storable::init_replaced(f, pattern, repl, offset, maxCount);
    }

    constexpr lstring() {
        create_empty();
    }

    constexpr ~lstring() {
        dealloc();
    }

    /*!
     * @ru @brief Копирование из другой строки такого же типа.
     * @param other - другая строка.
     * @en @brief Copy from another string of the same type.
     * @param other - another string.
     */
    constexpr lstring(const my_type& other) : base_storable(other.allocator()) {
        struct copy{uint64_t p[2];};
        constexpr size_t short_str = sizeof(copy) / sizeof(K);
        if constexpr (LocalCapacity >= short_str - 1) {
            if (other.size_ < short_str) {
                data_ = local_;
                size_ = other.size_;
                *(copy*)local_ = *(const copy*)other.local_;
                return;
            }
        }
        if (size_t size = other.size_) {
            copy_from_another(init(size), other.symbols(), size);
        } else {
            create_empty();
        }
    }
    /*!
     * @ru @brief Копирование из другой строки такого же типа, но с другим аллокатором.
     * @param other - другая строка.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Copy from another string of the same type, but with a different allocator.
     * @param other - another string.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires(sizeof...(Args) > 0 && std::is_convertible_v<allocator_t, Args...>)
    constexpr lstring(const my_type& other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if (other.size_) {
            copy_from_another(init(other.size_), other.symbols(), other.size_);
        } else {
            create_empty();
        }
    }
    /*!
     * @ru @brief Конструктор из строкового литерала.
     * @param value - строковый литерал.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief String literal constructor.
     * @param value - string literal.
     * @param ...args - parameter for initialization allocator.
     */
    template<typename T, size_t I = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr lstring(T&& value, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        if constexpr (I > 1) {
            K* ptr = init(I - 1);
            traits::copy(ptr, (const K*)value, I - 1);
            ptr[I - 1] = K{};
        } else {
            create_empty();
        }
    }
    /*!
     * @ru @brief Конструктор перемещения из строки такого же типа.
     * @param other - другая строка.
     * @en @brief Constructor for moving from a string of the same type.
     * @param other - another string.
     */
    constexpr lstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        if (other.size_) {
            size_ = other.size_;
            if (other.is_alloced()) {
                data_ = other.data_;
                other.data_ = other.local_;
                capacity_ = other.capacity_;
            } else {
                data_ = local_;
                copy_from_another(data_, other.local_, size_);
            }
            other.size_ = 0;
            other.local_[0] = 0;
        } else {
            create_empty();
        }
    }
    /*!
     * @ru @brief Конструктор заполнения с помощью функтора (см. str_mutable::fill).
     * @param op - функтов заполнения.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief A fill constructor using a functor (see str_mutable::fill).
     * @param op - filling functions.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename Op, typename... Args>
        requires(std::is_constructible_v<Allocator, Args...> && (std::is_invocable_v<Op, my_type&> || std::is_invocable_v<Op, K*, size_t>))
    lstring(const Op& op, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        create_empty();
        this->operator<<(op);
    }
    template<typename O>
        requires(!std::is_same_v<O, K>)
    lstring(simple_str<O> init) {
        this->init_from_utf_convertible(init);
    }

    template<typename O, typename I, bool M>
        requires(!std::is_same_v<O, K>)
    lstring(const str_algs<O, simple_str<O>, I, M>& init) {
        this->init_from_utf_convertible(init.to_str());
    }

    // copy and swap для присваиваний здесь не очень применимо, так как для строк с большим локальным буфером лишняя копия даже перемещением будет дорого стоить
    // Поэтому реализуем копирующее и перемещающее присваивание отдельно
    // copy and swap for assignments is not very applicable here, since for strings with a large local buffer, an extra copy, even by moving, will be expensive
    // Therefore, we implement the copy and move assignment separately

    /*!
     * @ru @brief Оператор присваивания копией из строки такого же типа.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Copy assignment operator from a string of the same type.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    my_type& operator=(const my_type& other) {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        // Since there cannot be an indirect dependency between these objects, it is enough to check only for equality
        if (&other != this) {
            traits::copy(reserve_no_preserve(other.size_), other.data_, other.size_ + 1);
            size_ = other.size_;
        }
        return *this;
    }
    /*!
     * @ru @brief Оператор присваивания перемещением из строки такого же типа.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator by moving from a string of the same type.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    my_type& operator=(my_type&& other) noexcept {
        // Так как между этими объектами не может быть косвенной зависимости, достаточно проверить только на равенство
        // Since there cannot be an indirect dependency between these objects, it is enough to check only for equality
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
                // A special case, they are trying to assign us a piece of our own string.
                // Just move the text in the buffer and set a new size
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
     * @ru @brief Оператор присваивания из simple_str.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator from simple_str.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    my_type& operator=(simple_str<K> other) {
        return assign(other.str, other.len);
    }
    /*!
     * @ru @brief Оператор присваивания строкового литерала.
     * @param other - строковый литерал, копируется в буфер строки.
     * @return my_type& - ссылку на себя же.
     * @en @brief String literal assignment operator.
     * @param other - string literal, copied to the string buffer.
     * @return my_type& - a reference to yourself.
     */
    template<typename T, size_t S = const_lit_for<K, T>::Count>
    my_type& operator=(T&& other) {
        return assign((const K*)other, S - 1);
    }
    /*!
     * @ru @brief Оператор присваивания строкового выражения.
     * @param expr - строковое выражение, материализуемое в буфер строки.
     * @return my_type& - ссылку на себя же.
     * @details Если в строковом выражении что-либо ссылается на части этой же строки, то результат не определён.
     * @en @brief String expression appending operator.
     * @param expr - a string expression materialized into the string buffer.
     * @return my_type& - a reference to yourself.
     * @details If anything in a string expression refers to parts of the same string, then the result is undefined.
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
    /// @ru Длина строки. @en String length.
    constexpr size_t length() const noexcept {
        return size_;
    }
    /// @ru Указатель на константные символы. @en Pointer to constant characters.
    constexpr const K* symbols() const noexcept {
        return data_;
    }
    /// @ru Указатель на буфер строки. @en Pointer to a string buffer.
    constexpr K* str() noexcept {
        return data_;
    }
    /// @ru Пустая ли строка. @en Is the string empty?
    constexpr bool is_empty() const noexcept {
        return size_ == 0;
    }
    /// @ru Пустая ли строка, для совместимости с std::string. @en Whether the string is empty, for compatibility with std::string.
    constexpr bool empty() const noexcept {
        return size_ == 0;
    }
    /// @ru Текущая ёмкость буфера строки. @en Current row buffer capacity.
    constexpr size_t capacity() const noexcept {
        return is_alloced() ? capacity_ : LocalCapacity;
    }
    /*!
     * @ru @brief Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
     * @param newSize - новый размер строки.
     * @return K* - указатель на буфер.
     * @details Содержимое буфера не определено, и не гарантируется сохранение старого содержимого.
     *          Размер строки устанавливается в newSize.
     * @en @brief Allocate a buffer large enough to hold newSize characters plus a terminating null.
     * @param newSize - new string size.
     * @return K* - pointer to the buffer.
     * @details The contents of the buffer are undefined and the old contents are not guaranteed to be retained.
     *          The string size is set to newSize.
     */
    constexpr K* reserve_no_preserve(size_t newSize) {
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
     * @ru @brief Выделить буфер, достаточный для размещения newSize символов плюс завершающий ноль.
     * @param newSize - новый размер строки.
     * @return K* - указатель на буфер.
     * @details Содержимое строки сохраняется. При увеличении буфера размер выделяется не больше запрошенного.
     *          Размер строки устанавливается в newSize.
     * @en @brief Allocate a buffer large enough to hold newSize characters plus a terminating null.
     * @param newSize - new string size.
     * @return K* - pointer to the buffer.
     * @details The contents of the string are preserved. When increasing the buffer, the size allocated is no larger than the requested one.
     *          The string size is set to newSize.
     */
    constexpr K* reserve(size_t newSize) {
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
     * @ru @brief Устанавливает размер текущей строки, при необходимости выделяя место.
     * @param newSize - новый размер строки.
     * @return K* - указатель на буфер.
     * @details Содержимое строки сохраняется. При увеличении буфера размер выделяется не менее чем 2 старого размера буфера.
     *          Размер строки устанавливается в newSize.
     * @en @brief Sets the size of the current string, allocating space if necessary.
     * @param newSize - new string size.
     * @return K* - pointer to the buffer.
     * @details The contents of the string are preserved. When increasing the buffer size, at least 2 times the old buffer size are allocated.
     *          The string size is set to newSize.
     */
    constexpr K* set_size(size_t newSize) {
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
     * @ru @brief Узнать, локальный или внешний буфер используется для символов.
     * @en @brief Find out whether a local or external buffer is used for characters.
     */
    constexpr bool is_local() const noexcept {
        return !is_alloced();
    }
    /*!
     * @ru @brief Определить длину строки.
     * Ищет символ 0 в буфере строки до его ёмкости, после чего устаналивает длину строки по найденному 0.
     * @en @brief Determine the length of the string.
     * Searches for the character 0 in the string buffer to its capacity, and then sets the length of the string to the found 0.
     */
    constexpr void define_size() {
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
     * @ru @brief Уменьшает размер внешнего буфера до минимально возможного для хранения строки.
     * Если строка уместится во внутренний буфер - копирует её в него и освобождает внешний буфер.
     * @en @brief Reduces the size of the external buffer to the smallest possible size to hold the string.
     * If the string fits into the internal buffer, it copies it into it and frees the external buffer.
     */
    constexpr void shrink_to_fit() {
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
    /// @ru Делает строку пустой, не меняя буфер строки. @en Makes a string empty without changing the string buffer.
    constexpr void clear() {
        set_size(0);
    }
    /// @ru Делает строку пустой и освобождает внешний буфер, если он был. @en Makes the string empty and frees the external buffer, if there was one.
    constexpr void reset() {
        dealloc();
        local_[0] = 0;
        size_ = 0;
    }
};

template<size_t N = 15>
using lstringa = lstring<u8s, N>;
template<size_t N = 15>
using lstringb = lstring<ubs, N>;
template<size_t N = 15>
using lstringw = lstring<wchar_t, N>;
template<size_t N = 15>
using lstringu = lstring<u16s, N>;
template<size_t N = 15>
using lstringuu = lstring<u32s, N>;

template<size_t N = 15>
using lstringsa = lstring<u8s, N, true>;
template<size_t N = 15>
using lstringsb = lstring<ubs, N, true>;
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
 * @ru @brief Класс иммутабельной владеющей строки.
 * @tparam K - тип символов.
 * @tparam Allocator - тип аллокатора.
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
 * @en @brief Immutable owning string class.
 * @tparam K - character type.
 * @tparam Allocator - allocator type.
 * @details "shared" string.
 * Class with small string optimization plus a shared immutable string buffer.
 * Since the string buffer in this class is immutable, then:
 * Firstly, there is no need to store the size of the allocated buffer; we will not change it anyway.
 * Secondly, another type of string appears - a string initialized with a string literal.
 * For it, we simply save a pointer to symbols and do not count references.
 * Thus, initializing a string object in a program with a literal does not copy anything anywhere -
 * neither into itself nor into dynamic memory, and does not cost more than initialization
 * a raw pointer to a string, and even more optimal, since it also immediately substitutes the size,
 * but does not calculate it at runtime.
 * ```cpp
 *     stringa text = "text or very very very long text"; // costs nothing!
 *     string copy = anotherString; // All you need to do is copy the bytes of the object itself, plus possibly one atomic increment
 * ```
 * In the case of a shared buffer, the size of the string is still stored not in the shared buffer, but in each object.
 * Because of SSO, there is still enough space, and you will have to go to memory less for the length.
 * For example, calculating the sum of the lengths of strings in a vector will only go through the memory in the vector.
 *
 * Sizes for x64:
 * - for u8s - 24 bytes, stores strings up to 23 characters + 0
 * - for u16s - 32 bytes, stores strings of up to 15 characters + 0
 * - for u32s - 32 bytes, stores strings of up to 7 characters + 0
 */
template<typename K, Allocatorable Allocator = allocator_string>
class decl_empty_bases sstring :
    public str_algs<K, simple_str<K>, sstring<K, Allocator>, false>,
    public str_storable<K, sstring<K, Allocator>, Allocator>,
    public null_terminated<K, sstring<K, Allocator>>,
    public from_utf_convertible<K, sstring<K, Allocator>> {
public:
    using symb_type = K;
    using uns_type = std::make_unsigned_t<K>;
    using my_type = sstring<K, Allocator>;
    using allocator_t = Allocator;

    enum { LocalCount = local_count<K> };

protected:
    using base_algs = str_algs<K, simple_str<K>, my_type, false>;
    using base_storable = str_storable<K, my_type, Allocator>;
    using base_utf = from_utf_convertible<K, my_type>;
    using traits = ch_traits<K>;
    using uni = unicode_traits<K>;
    using s_str = base_storable::s_str;

    friend base_storable;
    friend base_utf;

    enum Types { Local, Constant, Shared };

    union {
        // Когда у нас короткая строка, она лежит в самом объекте, а в localRemain
        // пишется, сколько символов ещё можно вписать. Когда строка занимает всё
        // возможное место, то localRemain становится 0, type в этом случае тоже 0,
        // и в итоге после символов строки получается 0, как и надо!
        // When we have a short string, it lies in the object itself, and in localRemain
        // writes how many more characters can be entered. When a string takes up everything
        // possible location, then localRemain becomes 0, type in this case is also 0,
        // and as a result, after the characters of the string we get 0, as it should!
        struct {
            K buf_[LocalCount]; // Локальный буфер строки | Local string buffer
            uns_type localRemain_ : sizeof(uns_type) * CHAR_BIT - 2;
            uns_type type_ : 2;
        };
        struct {
            union {
                // Указатель на конcтантную строку | Pointer to a constant string
                const K* cstr_;
                // Указатель на строку, перед которой лежит SharedStringData
                // Pointer to the string preceded by SharedStringData
                const K* sstr_;
            };
            size_t bigLen_; // Длина не локальной строки | Non-local string length
        };
    };

    constexpr void create_empty() {
        type_ = Local;
        localRemain_ = LocalCount;
        buf_[0] = 0;
    }
    constexpr K* init(size_t s) {
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
        // Called when a string is created and needs to be resized.
        // There are no other references to the shared buffer.
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

    sstring() {
        create_empty();
    }

    /*!
     * @ru @brief Конструктор пустой строки.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Constructor for the empty string.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires (std::is_constructible_v<allocator_t, Args...> && sizeof...(Args) > 0)
    sstring(Args&&... args) noexcept(std::is_nothrow_constructible_v<allocator_t, Args...>)
        : base_storable(std::forward<Args>(args)...) {
        create_empty();
    }

    /*!
     * @ru @brief Конструктор из другого строкового объекта.
     * @param other - другой строковый объект, simple_str.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief A constructor from another string object.
     * @param other - another string object, simple_str.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(s_str other, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_from_str_other(other);
    }
    /*!
     * @ru @brief Конструктор повторения строки.
     * @param repeat - количество повторов.
     * @param pattern - строка, которую надо повторить.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief String repetition constructor.
     * @param repeat - number of repetitions.
     * @param pattern - the string to be repeated.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(size_t repeat, s_str pattern, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_str_repeat(repeat, pattern);
    }
    /*!
     * @ru @brief Конструктор повторения символа.
     * @param count - количество повторов.
     * @param pad - символ, который надо повторить.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Character repetition constructor.
     * @param count - number of repetitions.
     * @param pad - the character to be repeated.
     * @param ...args - parameters for initializing the allocator.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(size_t count, K pad, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_symb_repeat(count, pad);
    }
    /*!
     * @ru @brief Конструктор из строкового выражения.
     * @param expr - строковое выражение.
     * @param ...args - параметры для инициализации аллокатора.
     * @details Конструктор запрашивает у строкового выражения `length()`,
     *  выделяет память нужного размера, и вызывает метод `place()` для размещения
     *  результата в буфере.
     * @en @brief Constructor from a string expression.
     * @param expr - string expression.
     * @param ...args - parameters for initializing the allocator.
     * @details The constructor queries the string expression `length()`,
     *  allocates memory of the required size, and calls the `place()` method to allocate
     *  result in buffer.
     */
    template<typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    constexpr sstring(const StrExprForType<K> auto& expr, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        base_storable::init_str_expr(expr);
    }
    /*!
     * @ru @brief Конструктор из строкового источника с заменой.
     * @param f - строковый объект, из которого берётся исходная строка.
     * @param pattern - подстрока, которую надо заменить.
     * @param repl  - строка, на которую надо заменить.
     * @param offset - начальная позиция для поиска подстрок.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @param ...args - параметры для инициализации аллокатора.
     * @en @brief Constructor from string source with replacement.
     * @param f - the string object from which the source string is taken.
     * @param pattern - substring to be replaced.
     * @param repl - the string to be replaced with.
     * @param offset - starting position for searching substrings.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @param ...args - parameters for initializing the allocator.
     */
    template<StrType<K> From, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0, Args&&... args)
        : base_storable(std::forward<Args>(args)...) {
        base_storable::init_replaced(f, pattern, repl, offset, maxCount);
    }

    static const sstring<K> empty_str;
    /// @ru Деструктор строки. @en String destructor.
    constexpr ~sstring() {
        if (type_ == Shared) {
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        }
    }
    /*!
     * @ru @brief Конструктор копирования строки.
     * @param other - копируемая строка.
     * @en @brief String copy constructor.
     * @param other - the string to be copied.
     */
    constexpr sstring(const my_type& other) noexcept : base_storable(other.allocator()) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->incr();
    }
    /*!
     * @ru @brief Конструктор перемещения.
     * @param other - перемещаемая строка.
     * @en @brief Move constructor.
     * @param other - the string to be moved.
     */
    constexpr sstring(my_type&& other) noexcept : base_storable(std::move(other.allocator())) {
        memcpy(buf_, other.buf_, sizeof(buf_) + sizeof(K));
        other.create_empty();
    }

    /*!
     * @ru @brief Конструктор перемещения из lstring с совместимым с sstring внешним буфером.
     * @param src - перемещаемая строка.
     * @details В случае, если символы в lstring лежат во внешнем аллоцированном буфере,
     *  просто забираем указатель на буфер, он нам подойдёт.
     * @en @brief A move constructor from lstring with an sstring-compatible external buffer.
     * @param src - the string to be moved.
     * @details If the characters in lstring are in an external allocated buffer,
     * we just take the pointer to the buffer, it will suit us.
     */
    template<size_t N>
    constexpr sstring(lstring<K, N, true, Allocator>&& src) : base_storable(std::move(src.allocator())) {
        size_t size = src.length();
        if (size) {
            if (src.is_alloced()) {
                // Там динамический буфер, выделенный с запасом для SharedStringData.
                // There is a dynamic buffer allocated with a reserve for SharedStringData.
                K* str = src.str();
                if (size > LocalCount) {
                    // Просто присвоим его себе.
                    // Let's just assign it to ourselves.
                    sstr_ = str;
                    bigLen_ = size;
                    type_ = Shared;
                    localRemain_ = 0;
                    new (SharedStringData<K>::from_str(str)) SharedStringData<K>();
                } else {
                    // Скопируем локально
                    // Copy locally
                    type_ = Local;
                    localRemain_ = LocalCount - size;
                    traits::copy(buf_, str, size + 1);
                    // Освободим тот буфер, у локальной строки буфер не разделяется с другими
                    // Let's free that buffer; a local string's buffer is not shared with others
                    src.dealloc();
                }
            } else {
                // Копируем из локального буфера
                // Copy from local buffer
                K* str = init(src.size_);
                traits::copy(str, src.symbols(), size + 1);
            }
            src.create_empty();
        } else
            create_empty();
    }

    /*!
     * @ru @brief Инициализация из строкового литерала.
     * @param s - строковый литерал.
     * @param ...args - параметры для инициализации аллокатора.
     * @details В этом случае просто запоминаем указатель на строку и её длину.
     * @en @brief Initialize from a string literal.
     * @param s - string literal.
     * @param ...args - parameters for initializing the allocator.
     * @details In this case, we simply remember the pointer to the string and its length.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count, typename... Args>
        requires std::is_constructible_v<allocator_t, Args...>
    sstring(T&& s, Args&&... args) : base_storable(std::forward<Args>(args)...) {
        type_ = Constant;
        localRemain_ = 0;
        cstr_ = (const K*)s;
        bigLen_ = N - 1;
    }

    /*!
     * @ru @brief Инициализация из строкового источника с другим типом символов. Конвертирует через UTF.
     * @tparam O - тип жругих символов.
     * @en @brief Initialization from a string source with a different character type. Converts via UTF.
     * @tparam O - type of other characters.     */
    template<typename O> requires(!std::is_same_v<O, K>)
    sstring(simple_str<O> init) {
        this->init_from_utf_convertible(init);
    }

    template<typename O, typename I, bool M> requires(!std::is_same_v<O, K>)
    sstring(const str_algs<O, simple_str<O>, I, M>& init) {
        this->init_from_utf_convertible(init.to_str());
    }

    constexpr void swap(my_type&& other) noexcept {
        char buf[sizeof(buf_) + sizeof(K)];
        memcpy(buf, buf_, sizeof(buf));
        memcpy(buf_, other.buf_, sizeof(buf));
        memcpy(other.buf_, buf, sizeof(buf));

        std::swap(base_storable::allocator(), other.allocator());
    }
    /*!
     * @ru @brief Оператор присвоения другой строки того же типа.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator to another string of the same type.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    constexpr my_type& operator=(my_type other) noexcept {
        swap(std::move(other));
        return *this;
    }
    /*!
     * @ru @brief Оператор присвоения другой строки другого типа.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator to another string of a different type.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    constexpr my_type& operator=(simple_str<K> other) {
        return operator=(my_type{other, base_storable::allocator()});
    }
    /*!
     * @ru @brief Оператор присвоения строкового литерала.
     * @param other - строковый литера.
     * @return my_type& - ссылку на себя же.
     * @en @brief String literal assignment operator.
     * @param other - string character.
     * @return my_type& - a reference to yourself.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr my_type& operator=(T&& other) {
        return operator=(my_type{std::forward<T>(other), base_storable::allocator()});
    }
    /*!
     * @ru @brief Оператор присвоения другой строки типа lstring.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator to another string of type lstring.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    template<size_t N, bool forShared, typename A>
    constexpr my_type& operator=(const lstring<K, N, forShared, A>& other) {
        return operator=(my_type{other.to_str(), base_storable::allocator()});
    }
    /*!
     * @ru @brief Оператор присвоения перемещаемой строки типа lstring с совместимым буфером.
     * @param other - другая строка.
     * @return my_type& - ссылку на себя же.
     * @en @brief Assignment operator to a movable string of type lstring with a compatible buffer.
     * @param other - another string.
     * @return my_type& - a reference to yourself.
     */
    template<size_t N>
    constexpr my_type& operator=(lstring<K, N, true, Allocator>&& other) {
        return operator=(my_type{std::move(other)});
    }
    /*!
     * @ru @brief Оператор присвоения строкового выражения.
     * @param expr - строковое выражения.
     * @return my_type& - ссылку на себя же.
     * @details В строковом выражение допустимо ссылаться на части этой же строки, так как сначала создаётся копия.
     * @en @brief String expression assignment operator.
     * @param expr - string expression.
     * @return my_type& - a reference to yourself.
     * @details In a string expression, it is possible to refer to parts of the same string, since a copy is created first.
     */
    constexpr my_type& operator=(const StrExprForType<K> auto& expr) {
        return operator=(my_type{expr, base_storable::allocator()});
    }
    /*!
     * @ru @brief Сделать строку пустой.
     * @return my_type& - ссылку на себя же
     * @en @brief Make the string empty.
     * @return my_type& - a reference to yourself.
     */
    constexpr my_type& make_empty() noexcept {
        if (type_ == Shared)
            SharedStringData<K>::from_str(sstr_)->decr(base_storable::allocator());
        create_empty();
        return *this;
    }
    /// @ru Указатель на символы строки. @en Pointer to characters in the string.
    constexpr const K* symbols() const noexcept {
        return type_ == Local ? buf_ : cstr_;
    }
    /// @ru Длина строки. @en string length.
    constexpr size_t length() const noexcept {
        return type_ == Local ? LocalCount - localRemain_ : bigLen_;
    }
    /// @ru Пустая ли строка. @en Is the string empty?
    constexpr bool is_empty() const noexcept {
        return length() == 0;
    }
    /// @ru Пустая ли строка, для совместимости с std::string. @en Whether the string is empty, for compatibility with std::string.
    constexpr bool empty() const noexcept {
        return is_empty();
    }
    /*!
     * @ru @brief Получить строку, отформатированную с помощью `std::sprintf`.
     * @param pattern - форматная строка.
     * @param ...args  - аргументы для `sprintf`.
     * @return my_type.
     * @details Для Windows поддерживаются posix позиционные аргументы, используется `_sprintf_p`.
     * @en @brief Get a string formatted with `std::sprintf`.
     * @param pattern - format string.
     * @param ...args - arguments for `sprintf`.
     * @return my_type.
     * @details On Windows, posix positional arguments are supported, using `_sprintf_p`.
     */
    template<typename... T>
    static my_type printf(const K* pattern, T&&... args) {
        return my_type{lstring<K, 256, true>{}.printf(pattern, std::forward<T>(args)...)};
    }
    /*!
     * @ru @brief Получить строку, отформатированную с помощью `std::format`.
     * @param fmtString - константная форматная строка.
     * @param ...args  - аргументы для `std::format`.
     * @return my_type.
     * @en @brief Get a string formatted with `std::format`.
     * @param fmtString - constant format string.
     * @param ...args - arguments for `std::format`.
     * @return my_type.
     */
    template<typename... T>
    static my_type format(const FmtString<to_std_char_t<K>, T...>& fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.format(fmtString, std::forward<T>(args)...)};
    }
    /*!
     * @ru @brief Получить строку, отформатированную с помощью `std::vformat`.
     * @param fmtString - форматная строка.
     * @param ...args  - аргументы для `std::vformat`.
     * @return my_type.
     * @en @brief Get a string formatted with `std::vformat`.
     * @param fmtString - format string.
     * @param ...args - arguments for `std::vformat`.
     * @return my_type.
     */
    template<typename... T>
    static my_type vformat(simple_str<K> fmtString, T&&... args) {
        return my_type{lstring<K, 256, true, Allocator>{}.vformat(fmtString, std::forward<T>(args)...)};
    }
};

template<typename K, Allocatorable Allocator>
inline const sstring<K> sstring<K, Allocator>::empty_str{};

struct no_alloc{};

template<typename K, size_t N>
class decl_empty_bases cestring :
    public str_algs<K, simple_str<K>, cestring<K, N>, true>,
    public str_storable<K, cestring<K, N>, no_alloc>,
    public null_terminated<K, lstring<K, N>>
    //, public from_utf_convertible<K, lstring<K, N, forShared, Allocator>>
{
public:
    using symb_type = K;
    using my_type = cestring<K, N>;

    enum : size_t {
        /// @ru Размер внутреннего буфера в символах @en Size of internal buffer
        LocalCapacity = N | (sizeof(void*) / sizeof(K) - 1),
    };

protected:

    using base_algs = str_algs<K, simple_str<K>, my_type, true>;
    using base_storable = str_storable<K, my_type, no_alloc>;
    //using base_utf = from_utf_convertible<K, my_type>;
    using traits = ch_traits<K>;
    using s_str = base_storable::s_str;

    friend base_storable;
    //friend base_utf;
    const K* cstr_{};
    size_t size_{};
    bool is_cstr_{};
    K local_[LocalCapacity + 1]{};

    constexpr void create_empty() {
        is_cstr_ = false;
        size_ = 0;
        local_[0] = 0;
    }

    constexpr K* init(size_t s) {
        size_ = s;
        if (size_ > LocalCapacity) {
            throw std::bad_alloc{};
        }
        is_cstr_ = false;
        return local_;
    }
public:
    /// @ru Длина строки. @en String length.
    constexpr size_t length() const noexcept {
        return size_;
    }
    /// @ru Указатель на константные символы. @en Pointer to constant characters.
    constexpr const K* symbols() const noexcept {
        return is_cstr_ ? cstr_ : local_;
    }
    /// @ru Пустая ли строка. @en Is the string empty?
    constexpr bool is_empty() const noexcept {
        return size_ == 0;
    }
    /// @ru Пустая ли строка, для совместимости с std::string. @en Whether the string is empty, for compatibility with std::string.
    constexpr bool empty() const noexcept {
        return size_ == 0;
    }
    /// @ru Текущая ёмкость буфера строки. @en Current row buffer capacity.
    constexpr size_t capacity() const noexcept {
        return LocalCapacity;
    }
    /*!
     * @ru @brief Конструктор пустой строки.
     * @en @brief Constructor for the empty string.
     */
    constexpr cestring() noexcept = default;

    /*!
     * @ru @brief Конструктор из другого строкового объекта.
     * @param other - другой строковый объект, simple_str.
     * @en @brief A constructor from another string object.
     * @param other - another string object, simple_str.
     */
    constexpr cestring(s_str other) : base_storable() {
        base_storable::init_from_str_other(other);
    }
    /*!
     * @ru @brief Конструктор повторения строки.
     * @param repeat - количество повторов.
     * @param pattern - строка, которую надо повторить.
     * @en @brief String repetition constructor.
     * @param repeat - number of repetitions.
     * @param pattern - the string to be repeated.
     */
    constexpr cestring(size_t repeat, s_str pattern) : base_storable() {
        base_storable::init_str_repeat(repeat, pattern);
    }
    /*!
     * @ru @brief Конструктор повторения символа.
     * @param count - количество повторов.
     * @param pad - символ, который надо повторить.
     * @en @brief Character repetition constructor.
     * @param count - number of repetitions.
     * @param pad - the character to be repeated.
     */
    constexpr cestring(size_t count, K pad) : base_storable() {
        base_storable::init_symb_repeat(count, pad);
    }
    /*!
     * @ru @brief Конструктор из строкового выражения.
     * @param expr - строковое выражение.
     * @details Конструктор запрашивает у строкового выражения `length()`,
     *  выделяет память нужного размера, и вызывает метод `place()` для размещения
     *  результата в буфере.
     * @en @brief Constructor from a string expression.
     * @param expr - string expression.
     * @details The constructor queries the string expression `length()`,
     *  allocates memory of the required size, and calls the `place()` method to allocate
     *  result in buffer.
     */
    constexpr  cestring(const StrExprForType<K> auto& expr) : base_storable() {
        base_storable::init_str_expr(expr);
    }
    /*!
     * @ru @brief Конструктор из строкового источника с заменой.
     * @param f - строковый объект, из которого берётся исходная строка.
     * @param pattern - подстрока, которую надо заменить.
     * @param repl  - строка, на которую надо заменить.
     * @param offset - начальная позиция для поиска подстрок.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @en @brief Constructor from string source with replacement.
     * @param f - the string object from which the source string is taken.
     * @param pattern - substring to be replaced.
     * @param repl - the string to be replaced with.
     * @param offset - starting position for searching substrings.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     */
    template<StrType<K> From>
    constexpr cestring(const From& f, s_str pattern, s_str repl, size_t offset = 0, size_t maxCount = 0)
        : base_storable() {
        base_storable::init_replaced(f, pattern, repl, offset, maxCount);
    }

    /// @ru Деструктор строки. @en String destructor.
    constexpr ~cestring() {}

    /*!
     * @ru @brief Инициализация из строкового литерала.
     * @param s - строковый литерал.
     * @details В этом случае просто запоминаем указатель на строку и её длину.
     * @en @brief Initialize from a string literal.
     * @param s - string literal.
     * @details In this case, we simply remember the pointer to the string and its length.
     */
    template<typename T, size_t M = const_lit_for<K, T>::Count>
    constexpr cestring(T&& s) : base_storable(), cstr_((const K*)s), size_(M - 1), is_cstr_(true), local_{0} {}
};

template<typename K>
consteval simple_str_nt<K> select_str(simple_str_nt<u8s> s8, simple_str_nt<ubs> sb, simple_str_nt<uws> sw, simple_str_nt<u16s> s16, simple_str_nt<u32s> s32) {
    if constexpr (std::is_same_v<K, u8s>)
        return s8;
    if constexpr (std::is_same_v<K, ubs>)
        return sb;
    if constexpr (std::is_same_v<K, uws>)
        return sw;
    if constexpr (std::is_same_v<K, u16s>)
        return s16;
    if constexpr (std::is_same_v<K, u32s>)
        return s32;
}

#define uni_string(K, p) select_str<K>(p, u8##p, L##p, u##p, U##p)

template<typename K, template<typename C> typename H>
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
    bool operator==(const StoreType& other) const {
        return hash == other.hash && typename H<K>::eql{}(*this, other);
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

template<typename K>
struct str_exact;

static_assert(std::is_trivially_copyable_v<StoreType<u8s, str_exact>>, "Store type must be trivially copyable");


template<typename K, typename V>
std::allocator<std::pair<K, V>> default_hashstrmap_allocator_selector(...);
// Если вы хотите задать свой дефолтный аллокатор для hashStrmap, перед включение sstring.h
// объявите функцию
// template<typename K, typename V>
// ваш_тип_аллокатора default_hashstrmap_allocator_selector(int);
// If you want to set your default allocator for hashStrMap, before including sstring.h
// declare a function
// template<typename K, typename V>
// your_allocator_type default_hashstrmap_allocator_selector(int);
template<typename K, template<typename C> typename H, typename V>
using allocator_hashstrmap = decltype(default_hashstrmap_allocator_selector<const StoreType<K, H>, V>(int(0)));

/*!
 * @ru @brief Контейнер для более эффективного поиска по строковым ключам.
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
 * @en @brief Container for more efficient searching by string keys.
 * @details Used to store and lookup keys of any string type.
 * Like unordered_map, but a little better.
 * Stores simple_str as keys along with the calculated hash and an empty space for sstring.
 * After insertion, creates an sstring in this empty space so that simple_str has something to refer to.
 * Allows you to use any simstr string objects for insertion and search, creating an object from them
 * sstring only for real insertion.
 * Since C++20, unordered_map has the ability to perform heterogeneous search by key with type,
 * different from the type of the key being stored, but this requires some dancing with hash types and comparisons.
 * Search with insertion (try_emplace) by a type other than the key type appears in the standard only with C++26.
 * There is a hashing and comparison implementation for the options:
 * - case sensitive
 * - case insensitive, ASCII only
 * - case insensitive, simplified Unicode (up to 0xFFFF).
 * Hashing is performed by the FNV-1A algorithm.
 * Automatic conversion of simstr string objects to the type of the stored key (i.e. with a calculated hash),
 * and also saving sstring in the key when inserting - performed for the try_emplace, emplace, at, methods
 * find, operator[], erase. When using other search methods, you need to provide the correct
 * calculated hash in the StoreType key. Do not use insertion methods other than those listed; they are not
 * will save sstring in the key.
 *
 * The main thing I would like to focus on is the very purpose of strHashMap, for which it was originally invented.
 * The goal was not to somehow beat unordered_map in terms of performance/memory (otherwise why make strHashMap on
 * unordered_map database?) The main problem that was solved is that we have several options for string objects,
 * and I would like to be able to use any type of string object to insert and search for keys.
 * We have simple_str, simple_str_nt, sstring, and lstring<N> in general for every N is a new object type.
 * In std everything was simple - there was only string, and accordingly, we used unordered_map<string, Something there>.
 * And by the way, in std we encountered the same problem: the keys are const char*, and with C++17 and string_view (this is like our simple_str),
 * searching in unordered_map<string, Type> is not very good, you have to convert the key to string every time, although you can
 * do without this.
 * Therefore, starting from C++20, the possibility of heterogeneous search was added to unordered_map - on
 * https://en.cppreference.com/w/cpp/container/unordered_map/find these are syntaxes (3) and (4).
 * True, this requires ritual dances with a tambourine - a special type is required for hashing (which, of course, in
 * the standard is no longer included and you need to implement it yourself), which can hash different types of keys,
 * and contains typename `is_transparent`. In fact, there is an example of it on the same page.
 * This solved part of the problem - search, but did not solve the insertion problem - for inserting into unordered_map<string, Type>
 * still only requires string. And only in the future C++26 we expect the possibility of heterogeneous insertion in the form
 * try_emplace (https://en.cppreference.com/w/cpp/container/unordered_map/try_emplace) in syntax (6), which
 * will allow you to accept keys of other types and convert them to the desired type only if the insertion is actually carried out.
 * In my string library, I encountered this problem even before C++17, and so I’m not so cool as to
 * influence the standard, I simply created my own class - a descendant of unordered_map, with wrappers around the insertion/search methods.
 * Well, I also added two more hashing/comparison options: case-insensitive ASCII, case-insensitive simple unicode.
 * Thus, we have a class that extends the standard unordered_map with the ability to work with string keys of different
 * types, allowing you to avoid unnecessary unnecessary key conversions when searching and inserting.
 * The fact that performance may improve is not the goal, but a side effect - pleasant, if there is one,
 * but not fatal if it is not there.
 */
template<typename K, typename T, template<typename C> typename HE = str_exact, typename A = allocator_hashstrmap<K, HE, T>>
class hashStrMap : public std::unordered_map<StoreType<K, HE>, T, typename HE<K>::hash, typename HE<K>::eql, A> {
protected:
    using InStore = StoreType<K, HE>;

public:
    using hasher = typename HE<K>::hash;
    using comparator = typename HE<K>::eql;
    using my_type = hashStrMap<K, T, HE, A>;
    using hash_t = std::unordered_map<StoreType<K, HE>, T, hasher, comparator, A>;

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
    // When entering, the hash must already be calculated
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
        return {key, hasher{}(key)};
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
struct str_exact {
    struct eql {
        bool operator()(const StoreType<K, str_exact>& _Left, const StoreType<K, str_exact>& _Right) const {
            return _Left.hash == _Right.hash && _Left.str == _Right.str;
        }
    };
    struct hash { // hash functor for basic_string
        size_t operator()(simple_str<K> _Keyval) const {
            return fnv_hash(_Keyval.symbols(), _Keyval.length());
        }
        size_t operator()(const StoreType<K, str_exact>& _Keyval) const {
            return _Keyval.hash;
        }
    };
};

template<typename K>
struct str_eqlia {
    struct eql {
        bool operator()(const StoreType<K, str_eqlia>& _Left, const StoreType<K, str_eqlia>& _Right) const {
            return _Left.hash == _Right.hash && _Left.str.equal_ia(_Right.str);
        }
    };
    struct hash {
        size_t operator()(simple_str<K> _Keyval) const {
            return fnv_hash_ia(_Keyval.symbols(), _Keyval.length());
        }
        size_t operator()(const StoreType<K, str_eqlia>& _Keyval) const {
            return _Keyval.hash;
        }
    };
};

template<typename K>
struct str_eqliu {
    struct eql {
        bool operator()(const StoreType<K, str_eqliu>& _Left, const StoreType<K, str_eqliu>& _Right) const {
            return _Left.hash == _Right.hash && _Left.str.equal_iu(_Right.str);
        }
    };
    struct hash {
        size_t operator()(simple_str<K> _Keyval) const {
            using bc = to_base_char_t<K>;
            return unicode_traits<bc>::hashiu((const bc*)_Keyval.symbols(), _Keyval.length());
        }
        size_t operator()(const StoreType<K, str_eqliu>& _Keyval) const {
            return _Keyval.hash;
        }
    };
};

/*!
 * @ru @brief Для построения длинных динамических строк конкатенацией мелких кусочков.
 * @details Выделяет по мере надобности отдельные блоки заданного размера (или кратного ему для больших вставок),
 * чтобы избежать релокации длинных строк. После построения можно слить в одну строку.
 * Как показали замеры, если сливать потом в одну строку, работает медленнее, чем lstring +=,
 * но экономнее по памяти. Если не сливать в одну строку, а дальше перебирать буфера - быстрее.
 * Сам является строковым выражением.
 * @en @brief For constructing long dynamic strings by concatenating small pieces.
 * @details Selects individual blocks of a given size (or a multiple of it for large inserts) as needed.
 * to avoid relocation of long strings. After construction, you can merge it into one string.
 * As measurements have shown, if you then merge it into one line, it works slower than lstring +=,
 * but more economical in memory. If you don't merge it into one line, and then iterate through buffers, it's faster.
 * Itself is a string expression.
*/
template<typename K>
class chunked_string_builder : expr_to_std_string<chunked_string_builder<K>> {
    using chunk_t = std::pair<std::unique_ptr<K[]>, size_t>;
    std::vector<chunk_t> chunks; // блоки и длина данных в них | blocks and data length in them
    K* write{};                  // Текущая позиция записи | Current write position
    size_t len{};                // Общая длина | Total length
    size_t remain{};             // Сколько осталось места в текущем блоке | How much space is left in the current block
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

    /// @ru Добавление порции данных. @en Adding a piece of data.
    my_type& operator<<(simple_str<K> data) {
        if (data.len) {
            len += data.len;
            if (data.len <= remain) {
                // Добавляемые данные влезают в выделенный блок, просто скопируем их
                // The added data fits into the selected block, just copy it
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;                // Сдвинем позицию  записи | Let's move the recording position
                chunks.back().second += data.len; // Увеличим длину хранимых в блоке данных | Let's increase the length of the data stored in the block
                remain -= data.len;               // Уменьшим остаток места в блоке | Reduce the remaining space in the block
            } else {
                // Не влезают | They don't fit
                if (remain) {
                    // Сначала запишем сколько влезет
                    // First, write down as much as we can
                    ch_traits<K>::copy(write, data.str, remain);
                    data.len -= remain;
                    data.str += remain;
                    chunks.back().second += remain; // Увеличим длину хранимых в блоке данных | Let's increase the length of the data stored in the block
                }
                // Выделим новый блок и впишем в него данные
                // Рассчитаем размер блока, кратного заданному выравниванию
                // Select a new block and write data into it
                // Calculate the block size that is a multiple of the given alignment
                size_t blockSize = (data.len + align - 1) / align * align;
                chunks.emplace_back(std::make_unique<K[]>(blockSize), data.len);
                write = chunks.back().first.get();
                ch_traits<K>::copy(write, data.str, data.len);
                write += data.len;
                remain = blockSize - data.len;
            }
        }
        return *this;
    }
    /// @ru Добавление строкового выражения. @en Adding a string expression.
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
    /// @ru Добавление символа. @en Adding a symbol.
    template<typename T>
    my_type& operator<<(T data)
        requires std::is_same_v<T, K>
    {
        return operator<<(expr_char<K>(data));
    }
    /// @ru Длина сохранённого текста. @en Length of the saved text.
    constexpr size_t length() const noexcept {
        return len;
    }
    /// @ru Сбрасывает содержимое, но при этом не удаляет первый буфер, чтобы потом избежать аллокации. @en Resets the contents, but does not delete the first buffer in order to avoid allocation later.
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
     * @ru @brief Применяет функтор к каждому сохранённому буферу.
     * @tparam Op - тип функтора, функция вида (const K* ptr, size_t len).
     * @param o - функтор.
     * @en @brief Applies a functor to each stored buffer.
     * @tparam Op - type of the functor, function type (const K* ptr, size_t len).
     * @param o is a functor.
     */
    template<typename Op>
    void out(const Op& o) const {
        for (const auto& block: chunks)
            o(block.first.get(), block.second);
    }
    /*!
     * @ru @brief Проверяет, расположен ли весь текст одним непрерывным куском в памяти.
     * @en @brief Checks whether all text is located in one contiguous chunk in memory.
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
     * @ru @brief Получить указатель на начало первого буфера.
     * Имеет смысл применять только если is_continuous true.
     * @en @brief Get a pointer to the beginning of the first buffer.
     * It makes sense to apply only if is_continuous true.
     */
    const K* begin() const {
        return chunks.size() ? chunks.front().first.get() : simple_str_nt<K>::empty_str.str;
    }
    /*!
     * @ru @brief Очистить объект, освободив все выделенные буфера.
     * @en @brief Clear the object, freeing all allocated buffers.
     */
    void clear() {
        chunks.clear();
        write = nullptr;
        len = 0;
        remain = 0;
    }
    /*!
     * @ru @brief Объект, позволяющий последовательно копировать содержимое в буфер заданного размера.
     * @en @brief An object that allows you to sequentially copy content into a buffer of a given size.
     */
    struct portion_store {
        typename decltype(chunks)::const_iterator it, end;
        size_t writedFromCurrentChunk;
        /*!
         * @ru @brief Проверить, что данные ещё не кончились.
         * @en @brief Check that the data has not yet run out.
         */
        bool is_end() {
            return it == end;
        }
        /*!
         * @ru @brief Сохранить очередную порцию данных в буфер.
         * @param buffer - указатель на буфер для сохранения данных.
         * @param size - размер буфера.
         * @return size_t - количество скопированных СИМВОЛОВ (не байтов).
         * @en @brief Save the next portion of data to the buffer.
         * @param buffer - pointer to the buffer for storing data.
         * @param size - buffer size.
         * @return size_t - the number of CHARACTERS (not bytes) copied.
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
     * @ru @brief Получить portion_store, через который можно последовательно извлекать данные во внешний буфер.
     * @return portion_store.
     * @en @brief Get a portion_store through which data can be sequentially retrieved into an external buffer.
     * @return portion_store.
     */
    portion_store get_portion() const {
        return {chunks.begin(), chunks.end(), 0};
    }
    /*!
     * @ru @brief Получить внутренние буфера с данными.
     * @return const auto&.
     * @en @brief Get internal data buffers.
     * @return const auto&.
     */
    const auto& data() const {
        return chunks;
    }
};

using stringa = sstring<u8s>;
using stringb = sstring<ubs>;
using stringw = sstring<wchar_t>;
using stringu = sstring<u16s>;
using stringuu = sstring<u32s>;
static_assert(sizeof(stringa) == (sizeof(void*) == 8 ? 24 : 16), "Bad size of sstring");

/*!
 * @ru @brief Тип хеш-словаря для char строк, регистрозависимый поиск.
 * @en @brief Type of hash dictionary for char strings, case sensitive search.
 */
template<typename T, typename A = allocator_hashstrmap<u8s, str_exact, T>>
using hashStrMapA = hashStrMap<u8s, T, str_exact, A>;
/*!
 * @ru @brief Тип хеш-словаря для char строк, регистронезависимый поиск для ASCII символов.
 * @en @brief Type of hash dictionary for char strings, case-insensitive lookup for ASCII characters.
 */
template<typename T, typename A = allocator_hashstrmap<u8s, str_eqlia, T>>
using hashStrMapAIA = hashStrMap<u8s, T, str_eqlia, A>;
/*!
 * @ru @brief Тип хеш-словаря для char строк, регистронезависимый поиск для Unicode символов до 0xFFFF.
 * @en @brief Hash dictionary type for char strings, case-insensitive search for Unicode characters up to 0xFFFF.
 */
template<typename T, typename A = allocator_hashstrmap<u8s, str_eqliu, T>>
using hashStrMapAIU = hashStrMap<u8s, T, str_eqliu, A>;

/*!
 * @ru @brief Тип хеш-словаря для char8_t строк, регистрозависимый поиск.
 * @en @brief Type of hash dictionary for char8_t strings, case sensitive search.
 */
template<typename T, typename A = allocator_hashstrmap<ubs, str_exact, T>>
using hashStrMapB = hashStrMap<ubs, T, str_exact, A>;
/*!
 * @ru @brief Тип хеш-словаря для char8_t строк, регистронезависимый поиск для ASCII символов.
 * @en @brief Type of hash dictionary for char8_t strings, case-insensitive lookup for ASCII characters.
 */
template<typename T, typename A = allocator_hashstrmap<ubs, str_eqlia, T>>
using hashStrMapBIA = hashStrMap<ubs, T, str_eqlia, A>;
/*!
 * @ru @brief Тип хеш-словаря для char8_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF.
 * @en @brief Hash dictionary type for char8_t strings, case-insensitive search for Unicode characters up to 0xFFFF.
 */
template<typename T, typename A = allocator_hashstrmap<ubs, str_eqliu, T>>
using hashStrMapBIU = hashStrMap<ubs, T, str_eqliu, A>;

/*!
 * @ru @brief Тип хеш-словаря для wchar_t строк, регистрозависимый поиск.
 * @en @brief Hash dictionary type for wchar_t strings, case sensitive search.
 */
template<typename T, typename A = allocator_hashstrmap<wchar_t, str_exact, T>>
using hashStrMapW = hashStrMap<wchar_t, T, str_exact, A>;

/*!
 * @ru @brief Тип хеш-словаря для wchar_t строк, регистронезависимый поиск для ASCII символов.
 * @en @brief Hash dictionary type for wchar_t strings, case-insensitive lookup for ASCII characters.
 */
template<typename T, typename A = allocator_hashstrmap<wchar_t, str_eqlia, T>>
using hashStrMapWIA = hashStrMap<wchar_t, T, str_eqlia, A>;

/*!
 * @ru @brief Тип хеш-словаря для wchar_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF.
 * @en @brief Hash dictionary type for wchar_t strings, case insensitive search for Unicode characters up to 0xFFFF.
 */
template<typename T, typename A = allocator_hashstrmap<wchar_t, str_eqliu, T>>
using hashStrMapWIU = hashStrMap<wchar_t, T, str_eqliu, A>;

/*!
 * @ru @brief Тип хеш-словаря для char16_t строк, регистрозависимый поиск.
 * @en @brief Hash dictionary type for char16_t strings, case sensitive search.
 */
template<typename T, typename A = allocator_hashstrmap<u16s, str_exact, T>>
using hashStrMapU = hashStrMap<u16s, T, str_exact, A>;
/*!
 * @ru @brief Тип хеш-словаря для char16_t строк, регистронезависимый поиск для ASCII символов.
 * @en @brief Hash dictionary type for char16_t strings, case-insensitive lookup for ASCII characters.
 */
template<typename T, typename A = allocator_hashstrmap<u16s, str_eqlia, T>>
using hashStrMapUIA = hashStrMap<u16s, T, str_eqlia, A>;
/*!
 * @ru @brief Тип хеш-словаря для char16_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF.
 * @en @brief Hash dictionary type for char16_t strings, case insensitive search for Unicode characters up to 0xFFFF.
 */
template<typename T, typename A = allocator_hashstrmap<u16s, str_eqliu, T>>
using hashStrMapUIU = hashStrMap<u16s, T, str_eqliu, A>;

/*!
 * @ru @brief Тип хеш-словаря для char32_t строк, регистрозависимый поиск.
 * @en @brief Hash dictionary type for char32_t strings, case sensitive search.
 */
template<typename T, typename A = allocator_hashstrmap<u32s, str_exact, T>>
using hashStrMapUU = hashStrMap<u32s, T, str_exact, A>;
/*!
 * @ru @brief Тип хеш-словаря для char32_t строк, регистронезависимый поиск для ASCII символов.
 * @en @brief Hash dictionary type for char32_t strings, case-insensitive lookup for ASCII characters.
 */
template<typename T, typename A = allocator_hashstrmap<u32s, str_eqlia, T>>
using hashStrMapUUIA = hashStrMap<u32s, T, str_eqlia, A>;
/*!
 * @ru @brief Тип хеш-словаря для char32_t строк, регистронезависимый поиск для Unicode символов до 0xFFFF.
 * @en @brief Hash dictionary type for char32_t strings, case insensitive search for Unicode characters up to 0xFFFF.
 */
template<typename T, typename A = allocator_hashstrmap<u32s, str_eqliu, T>>
using hashStrMapUUIU = hashStrMap<u32s, T, str_eqliu, A>;

inline constexpr simple_str_nt<u8s> utf8_bom{"\xEF\xBB\xBF", 3}; // NOLINT

inline namespace literals {

/*!
 * @ru @brief Оператор литерал в simple_str_nt.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return simple_str_nt.
 * @en @brief Operator literal in simple_str_nt.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return simple_str_nt.
 */
SS_CONSTEVAL simple_str_nt<u8s> operator""_ss(const u8s* ptr, size_t l) {
    return simple_str_nt<u8s>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в simple_str_nt.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return simple_str_nt.
 * @en @brief Operator literal in simple_str_nt.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return simple_str_nt.
 */
SS_CONSTEVAL simple_str_nt<ubs> operator""_ss(const ubs* ptr, size_t l) {
    return simple_str_nt<ubs>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в simple_str_nt.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return simple_str_nt.
 * @en @brief Operator literal in simple_str_nt.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return simple_str_nt.
 */
SS_CONSTEVAL simple_str_nt<uws> operator""_ss(const uws* ptr, size_t l) {
    return simple_str_nt<uws>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в simple_str_nt.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return simple_str_nt.
 * @en @brief Operator literal in simple_str_nt.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return simple_str_nt.
 */
SS_CONSTEVAL simple_str_nt<u16s> operator""_ss(const u16s* ptr, size_t l) {
    return simple_str_nt<u16s>{ptr, l};
}

/*!
 * @ru @brief Оператор литерал в simple_str_nt.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return simple_str_nt.
 * @en @brief Operator literal in simple_str_nt.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return simple_str_nt.
 */
SS_CONSTEVAL simple_str_nt<u32s> operator""_ss(const u32s* ptr, size_t l) {
    return simple_str_nt<u32s>{ptr, l};
}

template<typename K> using HashKey = StoreType<K, str_exact>;
template<typename K> using HashKeyIA = StoreType<K, str_eqlia>;
template<typename K> using HashKeyIU = StoreType<K, str_eqliu>;

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-sensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKey<u8s> operator""_h(const u8s* ptr, size_t l) {
    return HashKey<u8s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-insensitive ASCII hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKeyIA<u8s> operator""_ia(const u8s* ptr, size_t l) {
    return HashKeyIA<u8s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a simple unicode case-insensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
inline HashKeyIU<u8s> operator""_iu(const u8s* ptr, size_t l) {
    return HashKeyIU<u8s>{{ptr, l}, str_eqliu<u8s>::hash{}(simple_str<u8s>{ptr, l})};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-sensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKey<ubs> operator""_h(const ubs* ptr, size_t l) {
    return HashKey<ubs>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-insensitive ASCII hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKeyIA<ubs> operator""_ia(const ubs* ptr, size_t l) {
    return HashKeyIA<ubs>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a simple unicode case-insensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
inline HashKeyIU<ubs> operator""_iu(const ubs* ptr, size_t l) {
    return HashKeyIU<ubs>{{ptr, l}, str_eqliu<u8s>::hash{}(simple_str<u8s>{(const u8s*)ptr, l})};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-sensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKey<u16s> operator""_h(const u16s* ptr, size_t l) {
    return HashKey<u16s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-insensitive ASCII hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKeyIA<u16s> operator""_ia(const u16s* ptr, size_t l) {
    return HashKeyIA<u16s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a simple unicode case-insensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
inline HashKeyIU<u16s> operator""_iu(const u16s* ptr, size_t l) {
    return HashKeyIU<u16s>{{ptr, l}, str_eqliu<u16s>::hash{}(simple_str<u16s>{ptr, l})};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-sensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKey<u32s> operator""_h(const u32s* ptr, size_t l) {
    return HashKey<u32s>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-insensitive ASCII hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKeyIA<u32s> operator""_ia(const u32s* ptr, size_t l) {
    return HashKeyIA<u32s>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a simple unicode case-insensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
inline HashKeyIU<u32s> operator""_iu(const u32s* ptr, size_t l) {
    return HashKeyIU<u32s>{{ptr, l}, str_eqliu<u32s>::hash{}(simple_str<u32s>{ptr, l})};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем с учётом регистра.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-sensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKey<uws> operator""_h(const uws* ptr, size_t l) {
    return HashKey<uws>{{ptr, l}, fnv_hash_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра ASCII.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a case-insensitive ASCII hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
consteval HashKeyIA<uws> operator""_ia(const uws* ptr, size_t l) {
    return HashKeyIA<uws>{{ptr, l}, fnv_hash_ia_compile(ptr, l)};
}

/*!
 * @ru @brief Оператор литерал в ключ для hashStrMap с посчитанным в compile time хешем без учёта регистра simple unicode.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return StoreType.
 * @en @brief Key literal operator for hashStrMap with a simple unicode case-insensitive hash calculated at compile time.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return StoreType.
 */
inline HashKeyIU<uws> operator""_iu(const uws* ptr, size_t l) {
    return HashKeyIU<uws>{{ptr, l}, str_eqliu<uws>::hash{}(simple_str<uws>{ptr, l})};
}
} // namespace literals

template<is_one_of_char_v K, bool upper>
struct expr_change_case : expr_to_std_string<expr_change_case<K, upper>>{
    using symb_type = K;

    str_src<K> src_;
    mutable size_t len_{};

    template<StrSource S>
    expr_change_case(S&& s) : src_(std::forward<S>(s)){}

    constexpr size_t length() const noexcept {
        if constexpr (sizeof(K) > 1) {
            return src_.length();
        } else {
            if constexpr (upper) {
                len_ = unicode_traits<K>::upper_len(src_.str, src_.len);
            } else {
                len_ = unicode_traits<K>::lower_len(src_.str, src_.len);
            }
            return len_;
        }
    }
    constexpr K* place(K* ptr) const noexcept {
        if constexpr (sizeof(K) > 1) {
            if constexpr (upper) {
                unicode_traits<K>::upper(src_.str, src_.len, ptr);
            } else {
                unicode_traits<K>::lower(src_.str, src_.len, ptr);
            }
            return ptr + src_.len;
        } else {
            const K* src = src_.str;
            if constexpr (upper) {
                unicode_traits<K>::upper(src, src_.len, ptr, len_);
            } else {
                unicode_traits<K>::lower(src, src_.len, ptr, len_);
            }
            return ptr;
        }
    }
};

/*!
 * @ru @brief Генерирует строку на основе исходной, заменяя все строчные буквы первой плоскости Юникода на прописные.
 * @tparam K - тип символов, выводится на основе исходной строки.
 * @details Берёт исходную строку и копирует её, заменяя строчные буквы первой плоскости Юникода на прописные.
 *  В качестве исходной строки может браться любой строковый объект.
 * @en @brief Generates a string based on the original one, replacing all lowercase letters of the first Unicode plane with uppercase ones.
 * @tparam K - character type, inferred based on the source string.
 * @details Takes the original string and copies it, replacing the lowercase letters of the first Unicode plane with uppercase ones.
 *  Any string object can be taken as the source string.
 * @ru Пример @en Example @~
 * ```cpp
 *  stringa upper = "Upper case version is: '" + e_upper(source_str) + "'.";
 * ```
 */
template<is_one_of_char_v K>
struct e_upper : expr_change_case<K, true> {
    using base = expr_change_case<K, true>;
    using base::base;
};

template<StrSource S>
e_upper(S&&) -> e_upper<symb_type_from_src_t<S>>;

/*!
 * @ru @brief Генерирует строку на основе исходной, заменяя все прописные буквы первой плоскости Юникода на строчные.
 * @tparam K - тип символов, выводится на основе исходной строки.
 * @details Берёт исходную строку и копирует её, заменяя прописные буквы первой плоскости Юникода на строчные.
 *  В качестве исходной строки может браться любой строковый объект.
 * @ru @brief Generate a string from the original one, replacing all uppercase letters of the first Unicode plane with lowercase ones.
 * @tparam K - character type, inferred based on the source string.
 * @details Takes the original string and copies it, replacing the uppercase letters of the first Unicode plane with lowercase ones.
 *  Any string object can be taken as the source string.
 * @ru Пример @en Example @~
 * ```cpp
 *  stringa lower = "Lower case version is: '" + e_lower(source_str) + "'.";
 * ```
 */
template<is_one_of_char_v K>
struct e_lower : expr_change_case<K, false> {
    using base = expr_change_case<K, false>;
    using base::base;
};

template<StrSource S>
e_lower(S&&) -> e_lower<symb_type_from_src_t<S>>;

/*!
 * @ru @brief Оператор вывода в поток simple_str.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Stream output operator simple_str.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::ostream& operator<<(std::ostream& stream, ssa text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток simple_str.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Stream output operator simple_str.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::wostream& operator<<(std::wostream& stream, ssw text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток simple_str.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Stream output operator simple_str.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::wostream& operator<<(std::wostream& stream, simple_str<wchar_type> text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток sstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator for outputting sstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::ostream& operator<<(std::ostream& stream, const stringa& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток sstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator for outputting sstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::wostream& operator<<(std::wostream& stream, const stringw& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток sstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator for outputting sstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
inline std::wostream& operator<<(std::wostream& stream, const sstring<wchar_type>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток lstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator to output lstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::ostream& operator<<(std::ostream& stream, const lstring<u8s, N, S, A>& text) {
    return stream << std::string_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток lstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator to output lstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<uws, N, S, A>& text) {
    return stream << std::wstring_view{text.symbols(), text.length()};
}

/*!
 * @ru @brief Оператор вывода в поток lstring.
 * @param stream - поток вывода.
 * @param text - текст.
 * @return std::ostream&.
 * @en @brief Operator to output lstring to stream.
 * @param stream - output stream.
 * @param text - text.
 * @return std::ostream&.
 */
template<size_t N, bool S, simstr::Allocatorable A>
inline std::wostream& operator<<(std::wostream& stream, const lstring<wchar_type, N, S, A>& text) {
    return stream << std::wstring_view{from_w(text.symbols()), text.length()};
}

} // namespace simstr

/*!
 * @ru @brief Форматтер для использования в std::format значений типа simple_str.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str.
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
 * @ru @brief Форматтер для использования в std::format значений типа simple_str_nt.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str_nt.
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
 * @ru @brief Форматтер для использования в std::format значений типа sstring.
 * @en @brief Formatter to use in std::format for values ​​of type string.
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
 * @ru @brief Форматтер для использования в std::format значений типа lstring.
 * @en @brief Formatter to use in std::format for values ​​of type lstring.
 */
template<typename K, size_t N, bool S, typename A>
struct std::formatter<simstr::lstring<K, N, S, A>, K> : std::formatter<std::basic_string_view<K>, K> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<K, N, S, A>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<K>, K>::format({t.symbols(), t.length()}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа simple_str<char8_t>.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str<char8_t>.
 */
template<>
struct std::formatter<simstr::simple_str<char8_t>, char> : std::formatter<std::basic_string_view<char>, char> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str<char8_t> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<char>, char>::format({(const char*)t.str, t.len}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа simple_str_nt<char8_t>.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str_nt<char8_t>.
 */
template<>
struct std::formatter<simstr::simple_str_nt<char8_t>, char> : std::formatter<std::basic_string_view<char>, char> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str_nt<char8_t> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<char>, char>::format({(const char*)t.str, t.len}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа sstring<char8_t>.
 * @en @brief Formatter to use in std::format for values ​​of type string<char8_t>.
 */
template<>
struct std::formatter<simstr::sstring<char8_t>, char> : std::formatter<std::basic_string_view<char>, char> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::sstring<char8_t>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<char>, char>::format({(const char*)t.symbols(), t.length()}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа lstring<char8_t>.
 * @en @brief Formatter to use in std::format for values ​​of type lstring<char8_t>.
 */
template<size_t N, bool S, typename A>
struct std::formatter<simstr::lstring<char8_t, N, S, A>, char> : std::formatter<std::basic_string_view<char>, char> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<char8_t, N, S, A>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<char>, char>::format({(const char*)t.symbols(), t.length()}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа simple_str<char16_t/char32_t>.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str<char16_t/char32_t>.
 */
template<>
struct std::formatter<simstr::simple_str<simstr::wchar_type>, wchar_t> : std::formatter<std::basic_string_view<wchar_t>, wchar_t> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str<simstr::wchar_type> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format({(const wchar_t*)t.str, t.len}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа simple_str_nt<char16_t/char32_t>.
 * @en @brief Formatter to use in std::format for values ​​of type simple_str_nt<char16_t/char32_t>.
 */
template<>
struct std::formatter<simstr::simple_str_nt<simstr::wchar_type>, wchar_t> : std::formatter<std::basic_string_view<wchar_t>, wchar_t> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(simstr::simple_str_nt<simstr::wchar_type> t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format({(const wchar_t*)t.str, t.len}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа sstring<char16_t/char32_t>.
 * @en @brief Formatter to use in std::format for values ​​of type string<char16_t/char32_t>.
 */
template<>
struct std::formatter<simstr::sstring<simstr::wchar_type>, wchar_t> : std::formatter<std::basic_string_view<wchar_t>, wchar_t> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::sstring<simstr::wchar_type>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format({(const wchar_t*)t.symbols(), t.length()}, fc);
    }
};

/*!
 * @ru @brief Форматтер для использования в std::format значений типа lstring<char16_t/char32_t>.
 * @en @brief Formatter to use in std::format for values ​​of type lstring<char16_t/char32_t>.
 */
template<size_t N, bool S, typename A>
struct std::formatter<simstr::lstring<simstr::wchar_type, N, S, A>, wchar_t> : std::formatter<std::basic_string_view<wchar_t>, wchar_t> {
    // Define format() by calling the base class implementation with the wrapped value
    template<typename FormatContext>
    auto format(const simstr::lstring<simstr::wchar_type, N, S, A>& t, FormatContext& fc) const {
        return std::formatter<std::basic_string_view<wchar_t>, wchar_t>::format({(const wchar_t*)t.symbols(), t.length()}, fc);
    }
};
