/*
 * ver. 1.7.0
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * База для строковых конкатенаций через выражения времени компиляции
 * (c) Project "SimStr", Aleksandr Orefkov orefkov@gmail.com
 * Base for string concatenations via compile-time expressions
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <climits>
#include <limits>
#include <string>
#include <string_view>
#include <concepts>
#include <vector>
#include <optional>
#include <charconv>

#if defined __has_builtin
#  if __has_builtin(__builtin_mul_overflow) && __has_builtin(__builtin_add_overflow)
#    define HAS_BUILTIN_OVERFLOW
#  endif
#endif

#ifdef _MSC_VER
#define _no_unique_address msvc::no_unique_address
#define decl_empty_bases __declspec(empty_bases)
#else
#define _no_unique_address no_unique_address
#define decl_empty_bases
#endif

#ifdef _MSC_VER
/* MSVC иногда не может сделать "text"_ss consteval, выдает ошибку C7595.
Находил подобное https://developercommunity.visualstudio.com/t/User-defined-literals-not-constant-expre/10108165
Пишут, что баг исправлен, но видимо не до конца.
Без этого в тестах в двух местах не понимает "text"_ss, хотя в других местах - нормально работает*/
/* MSVC sometimes fails to do "text"_ss consteval and gives error C7595.
Found something like this https://developercommunity.visualstudio.com/t/User-defined-literals-not-constant-expre/10108165
They write that the bug has been fixed, but apparently not completely.
Without this, in tests in two places it does not understand “text”_ss, although in other places it works fine */
#define SS_CONSTEVAL constexpr
#else
#define SS_CONSTEVAL consteval
#endif

/*!
 * @ru @brief Пространство имён для объектов библиотеки
 * @en @brief Library namespace
 */
namespace simstr {

// Выводим типы для 16 и 32 битных символов в зависимости от размера wchar_t
// Infer types for 16 and 32 bit characters depending on the size of wchar_t
inline constexpr bool wchar_is_u16 = sizeof(wchar_t) == sizeof(char16_t);

using wchar_type = std::conditional<wchar_is_u16, char16_t, char32_t>::type;

inline wchar_type* to_w(wchar_t* p) {
    return (reinterpret_cast<wchar_type*>(p));
}

inline const wchar_type* to_w(const wchar_t* p) {
    return (reinterpret_cast<const wchar_type*>(p));
}

inline wchar_t* from_w(wchar_type* p) {
    return (reinterpret_cast<wchar_t*>(p));
}

inline const wchar_t* from_w(const wchar_type* p) {
    return (reinterpret_cast<const wchar_t*>(p));
}

using u8s = char;
using ubs = char8_t;
using uws = wchar_t;
using u16s = char16_t;
using u32s = char32_t;

using uu8s = std::make_unsigned<u8s>::type;

template<typename T, typename K = void, typename... Types>
struct is_one_of_type {
    static constexpr bool value = std::is_same_v<T, K> || is_one_of_type<T, Types...>::value;
};
template<typename T>
struct is_one_of_type<T, void> : std::false_type {};

template<typename K>
concept is_one_of_char_v = is_one_of_type<K, u8s, ubs, wchar_t, u16s, u32s>::value;

template<typename K>
concept is_one_of_std_char_v = is_one_of_type<K, u8s, ubs, wchar_t, wchar_type>::value;

template<is_one_of_std_char_v From>
auto to_one_of_std_char(From* from) {
    if constexpr (std::is_same_v<From, u8s> || std::is_same_v<From, wchar_t>) {
        return from;
    } else if constexpr (std::is_same_v<From, ubs>) {
        return reinterpret_cast<u8s*>(from);
    } else {
        return from_w(from);
    }
}

template<is_one_of_std_char_v From>
auto to_one_of_std_char(const From* from) {
    if constexpr (std::is_same_v<From, u8s> || std::is_same_v<From, wchar_t>) {
        return from;
    } else if constexpr (std::is_same_v<From, ubs>) {
        return reinterpret_cast<const u8s*>(from);
    } else {
        return from_w(from);
    }
}

template<typename K>
struct to_std_char_type : std::type_identity<K>{};

template<>
struct to_std_char_type<char8_t>{
    using type = char;
};

template<>
struct to_std_char_type<char16_t>{
    using type = std::conditional_t<sizeof(char16_t) == sizeof(wchar_t), wchar_t, void>;
};

template<>
struct to_std_char_type<char32_t>{
    using type = std::conditional_t<sizeof(char32_t) == sizeof(wchar_t), wchar_t, void>;
};

template<typename K>
using to_std_char_t = typename to_std_char_type<K>::type;

template<typename K>
struct to_base_char_type : std::type_identity<K>{};

template<>
struct to_base_char_type<char8_t>{
    using type = char;
};

template<>
struct to_base_char_type<wchar_t>{
    using type = std::conditional_t<sizeof(char16_t) == sizeof(wchar_t), char16_t, char32_t>;
};

template<typename K>
using to_base_char_t = typename to_base_char_type<K>::type;

/*!
 * @ingroup StrExprs
 * @ru @brief Проверка, являются ли два типа совместимыми строковыми типами.
 * @tparam K1 - первый проверяемый тип.
 * @tparam K2 - второй проверяемый тип.
 * @details Оба типа должны быть строковыми типами и совпадать по размеру.
 *  То есть char и char8_t всегда совместимы, wchar_t тождественен в Linux char32_t, а в Windows - char16_t.
 *  Это для возможности смешивать строковые выражения совместимых типов.
 * @en @brief Checks whether two types are compatible string types.
 * @tparam K1 is the first type to check.
 * @tparam K2 is the second type to check.
 * @details Both types must be string types and the same size.
 * That is, char and char8_t are always compatible, wchar_t is identical to char32_t on Linux, and char16_t on Windows.
 * This is for the ability to mix string expressions of compatible types.
 */
template<typename K1, typename K2>
concept is_equal_str_type_v = sizeof(K1) == sizeof(K2) && is_one_of_char_v<K1> && is_one_of_char_v<K2>;

/*
Вспомогательные шаблоны для определения строковых литералов.
Используются для того, чтобы в параметрах функций ограничивать типы строго как `const K(&)[N]`
Если пишем
template<size_t N>
void func(const char(&lit)[N]);
то в такую функцию можно будет передать не константный буфер, что может вызывать ошибку:

// Выделили место под символы
char buf[100];
// Как то наполнили буфер, допустим, до половины.

stringa text = buf;
Тут компилятор приведет buf из типа char[100] в тип const char[100] и вызовет конструктор
для строкового литерала, в text запишется просто указатель на buf и длина 100.

Поэтому такие параметры объявляем как
template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
void func(T&& lit);

Тогда компилятор будет подставлять для T точный тип параметра без попыток привести тип к другому типу,
и выражение с параметром char[100] - не скомпилируется.

Helper templates for defining string literals.
They are used to limit types in function parameters strictly as `const K(&)[N]`
If we write
template<size_t N>
void func(const char(&lit)[N]);
then it will be possible to pass a non-constant buffer to such a function, which may cause an error:

// Allocate space for symbols
char buf[100];
// Somehow the buffer was filled, say, to half.

stringa text = buf;
Here the compiler will convert buf from type char[100] to type const char[100] and call the constructor
for a string literal, text will simply contain a pointer to buf and length 100.

Therefore, we declare such parameters as
template<typename T, typename K = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count>
void func(T&& lit);

Then the compiler will substitute for T the exact type of the parameter without attempting to cast the type to another type,
and an expression with the char[100] parameter will not compile.
*/

template<typename T> struct const_lit; // sfinae отработает, так как не найдёт определения | sfinae will work because it won't find a definition
// Для правильных типов параметров есть определение, в виде специализации шаблона
// There is a definition for the correct parameter types, in the form of a template specialization
template<is_one_of_char_v T, size_t N>
struct const_lit<const T(&)[N]> {
    using symb_type = T;
    constexpr static size_t Count = N;
};

template<typename T>
concept is_const_lit_v = requires {
    typename const_lit<T>::symb_type;
};

// Тут ещё дополнительно ограничиваем тип литерала
// Here we further restrict the type of the literal
template<typename K, typename T> struct const_lit_for;

template<typename K, is_equal_str_type_v<K> P, size_t N>
struct const_lit_for<K, const P(&)[N]> {
    constexpr static size_t Count = N;
};

template<typename K, size_t N>
class const_lit_to_array {

    template<size_t Idx>
    constexpr size_t find(K s) const {
        if constexpr (Idx < N) {
            return s == symbols_[Idx] ? Idx : find<Idx + 1>(s);
        }
        return -1;
    }

    template<size_t Idx>
    constexpr bool exist(K s) const {
        if constexpr (Idx < N) {
            return s == symbols_[Idx] || exist<Idx + 1>(s);
        }
        return false;
    }
public:
    const K (&symbols_)[N + 1];

    template<typename T, size_t M = const_lit_for<K, T>::Count> requires (M == N + 1)
    constexpr const_lit_to_array(T&& s)
        : symbols_((const K(&)[M])s) {}

    constexpr bool contain(K s) const {
        return exist<0>(s);
    }
    constexpr size_t index_of(K s) const {
        return find<0>(s);
    }
};

template<typename A>
concept StrTypeCommon = requires(const A& a) {
    typename std::remove_cvref_t<A>::symb_type;
    { a.is_empty() } -> std::same_as<bool>;
    { a.length() } -> std::convertible_to<size_t>;
    { a.symbols() };
};

template<typename A>
concept HasSymbType = requires {
    typename std::remove_cvref_t<A>::symb_type;
};

/*!
 * @ru @brief Базовая концепция строкового объекта.
 * @tparam A - проверяемый тип
 * @tparam K - тип символов
 * @details В библиотеке для разных целей могут использоваться различные типы объектов строк.
 *  Мы считаем строковым объектом любой объект, поддерживающий методы:
 *  - `is_empty()`: возвращает, пуста ли строка.
 *  - `length()`: возвращает длину строки без нулевого терминатора.
 *  - `symbols()`: возвращает указатель на строку символов.
 *  - `typename symb_type`: задаёт тип символов строки
 *
 * @en @brief Base concept of string object.
 * @tparam  A - tested type
 * @tparam K - type of symbols
 * @details The library can use different types of string objects for different purposes.
 * We consider a string object to be any object that supports methods:
 * - `is_empty()`: Returns whether the string is empty.
 * - `length()`: returns the length of a string without a null terminator.
 * - `symbols()`: returns a pointer to a string of symbols.
 * - `typename symb_type`: sets the character type of the string
 */
template<typename A, typename K>
concept StrType = StrTypeCommon<A> && requires(const A& a) {
    { a.symbols() } -> std::same_as<const K*>;
} && std::is_same_v<typename std::remove_cvref_t<A>::symb_type, K>;

template<typename T> struct is_std_string_source : std::false_type{};

template<typename K, typename A>
struct is_std_string_source<std::basic_string<K, std::char_traits<K>, A>> : std::true_type{};

template<typename K>
struct is_std_string_source<std::basic_string_view<K, std::char_traits<K>>> : std::true_type{};

template<typename T>
concept is_std_string_source_v = is_std_string_source<T>::value;

template<typename T>
concept StdStrSource = is_std_string_source_v<std::remove_cvref_t<T>>;

template<typename T, typename K>
concept StdStrSourceForType = StdStrSource<T> && is_equal_str_type_v<K, typename T::value_type>;

template<typename T>
concept StrSource = StdStrSource<T> || is_const_lit_v<T> || StrTypeCommon<T>;

template<typename T>
concept StrSourceNoLiteral = StdStrSource<T> || StrTypeCommon<T>;

template<typename T> struct is_std_string : std::false_type{};

template<typename K, typename A>
struct is_std_string<std::basic_string<K, std::char_traits<K>, A>> : std::true_type{};

template<typename T>
concept is_std_string_v = is_std_string<T>::value;
template<typename T, typename K>
concept StdStringForType = is_std_string_v<T> && is_equal_str_type_v<K, typename T::value_type>;

template<typename T>
struct symb_type_from_src {
    using type = void;

};

template<typename K, size_t N>
struct symb_type_from_src<const K(&)[N]> {
    using type = K;
};

template<StdStrSource T>
struct symb_type_from_src<T> {
    using type = typename std::remove_cvref_t<T>::value_type;
};

template<HasSymbType T>
struct symb_type_from_src<T> {
    using type = typename std::remove_cvref_t<T>::symb_type;
};

template<typename T>
using symb_type_from_src_t = symb_type_from_src<T>::type;


/*!
 * @ru @defgroup StrExprs Строковые выражения
 * @brief Описание строковых выражений
 * @details Все типы владеющих строк в simstr могут инициализироваться с помощью "строковых выражений".
 *  (по сути это вариант https://en.wikipedia.org/wiki/Expression_templates для строк).
 *  Строковое выражение - это объект произвольного типа, у которого имеются методы:
 *  - `size_t length() const`: выдает длину строки
 *  - `K* place(K*) const`: скопировать символы строки в предназначенный буфер и вернуть указатель за последним символом
 *  - `typename symb_type`: показывает, с каким типом символов он работает.
 *
 *  При инициализации строковый объект запрашивает у строкового выражения его размер, выделяет необходимую память,
 *  и передает память строковому выражению, которое помещает символы в отведённый буфер.
 *
 *  Кроме того, для совместимости с `std`, строковые выражения simstr могут конвертироваться в стандартные строки
 *  (std::basic_string) совместимых типов. До C++23 используется `resize` и потом заполнение через `data()`, начиная с C++23
 *  используется более оптимальный `resize_and_overwrite`. Это позволяет использовать быструю конкатенацию там, где требуются стандартные строки.
 *
 *  Все строковые объекты библиотеки сами являются строковыми выражениями, которые просто копирует исходную строку.
 *  В-основном строковые выражения используются для конкатенации или конвертации строк.
 *
 *  Для всех строковых выражений определен @ref op_plus_str_expr "operator +", который из двух операндов создает новое строковое выражение
 *  simstr::strexprjoin, объединяющее два строковых выражения, и которое в методе `length` возвращает сумму `length` исходных операндов,
 *  а в методе `place` - размещает в буфер результата сначала первый операнд, потом второй.
 *  И так как этот оператор сам возвращает строковое выражение, то к нему снова можно применить `operator +`, формируя цепочку из нескольких
 *  строковых выражений, и в итоге "материализовать" последний получившийся объект, который сначала посчитает размер всей общей памяти
 *  для конечного результата, а затем разместит вложенные подвыражения в один буфер.
 *
 *  Также `operator +` определён для строковых выражений и строковых литералов, строковых выражений и чисел
 *  (числа конвертируются в десятичное представление), а также вы можете сами добавить желаемые типы строковых выражений.
 *  Пример:
 *    ```cpp
 *        stringa text = header + ", count = " + count + ", done";
 *    ```
 *  Существует несколько типов строковых выражений "из коробки", для выполнения различных операций со строками
 *    - `expr_spaces< ТипСимвола, КоличествоСимволов, Символ>{}`: выдает строку длиной КоличествоСимволов,
 *        заполненную заданным символом. Количество символов и символ - константы времени компиляции.
 *        Для некоторых случаев есть сокращенная запись:
 *        - `e_spca< КоличествоСимволов>()`: строка char пробелов
 *        - `e_spcw< КоличествоСимволов>()`: строка w_char пробелов
 *    - `expr_pad< ТипСимвола>{КоличествоСимволов, Символ}`: выдает строку длинной КоличествоСимволов,
 *        заполненную заданным символом. Количество символов и символ могут задаваться в рантайме.
 *        Сокращенная запись: `e_c (КоличествоСимволов, Символ)`
 *    - `e_choice (bool Condition, StrExpr1, StrExpr2)`: если Condition == true, результат будет равен StrExpr1, иначе StrExpr2
 *    - `e_num< ТипСимвола>(ЦелоеЧисло)`: конвертирует число в десятичное представление. Редко используется, так как
 *         для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как text + number;
 *    - `e_real< ТипСимвола>(ВещественноеЧисло)`: конвертирует число в десятичное представление. Редко используется, так как
 *         для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как `text + number`;
 *    - `e_join< bool ПослеПоследнего = false, bool ПропускатьПустые = false>(контейнер, "Разделитель")`: конкатенирует все строки
 *        в контейнере, используя разделитель.
 *        Если `ПослеПоследнего == true`, то разделитель добавляется и после последнего элемента контейнера, иначе только
 *        между элементами.
 *        Если `ПропускатьПустые == true`, то пустые строки не добавляют разделитель, иначе для каждой пустой строки
 *        тоже вставляется разделитель
 *    - `e_repl(ИсходнаяСтрока, Искать, Заменять)`: заменяет в исходной строке вхождения Искать на Заменять.
 *    - `e_hex(Число)`: генерирует строку с 16ричным представлением числа.
 *    - `e_fill_left(StrExpr, width, symbol)`, `e_fill_right(StrExpr, width, symbol)`: дополняет строковое выражение до нужной длины заданным символом.
 *  и т.д. и т.п.
 *
 *  В одно выражение могут объединятся строковые выражения для символов разных, но совместимых типов.
 *  То есть можно сочетать `char` и `char8_t`, под Linux `wchar_t` и `char32_t`, под Windows `wchar_t` и `char16_t`.
 *
 *  Помните: строковое выражение - это не строка, это только "инструкция", как собрать строку.
 *
 * @en @defgroup StrExprs String Expressions
 * @brief Description of String Expressions
 * @details All owning string types can be initialized using "string expressions"
 *  (essentially a variant of https://en.wikipedia.org/wiki/Expression_templates for strings).
 *  A string expression is an object of an arbitrary type that has methods:
 *   - `size_t length() const`: returns the length of the string
 *   - `K* place(K*) const`: copy the characters of the string to the intended buffer and return a pointer behind the last character
 *   - `typename symb_type`: shows what type of symbols it works with.
 *
 *  During initialization, a string object asks the string expression for its size, allocates the necessary memory,
 *  and passes the memory to a string expression that places the characters in the allocated buffer.
 *
 *  Additionally, for compatibility with `std`, simstr string expressions can be converted to standard strings
 *  (std::basic_string) compatible types. Before C++23, `resize` and `data` is used, starting with C++23
 *  the more optimal `resize_and_overwrite` is used. This allows for fast concatenation where standard strings are required.
 *
 *  All library string objects are themselves string expressions that simply copy the original string.
 *  Basically, string expressions are used to concatenate or convert strings.
 *
 *  For all string expressions, @ref op_plus_str_expr "operator +" is defined, which creates a new string expression from two operands
 *  simstr::strexprjoin, which combines two string expressions, and which in the `length` method returns the sum of the `length` original operands,
 *  and in the `place` method - places first the first operand, then the second, into the result buffer.
 *  And since this operator itself returns a string expression, you can again apply `operator +` to it, forming a chain of several
 *  string expressions, and eventually “materialize” the last resulting object, which will first calculate the size of the entire shared memory
 *  for the final result, and then will place the nested subexpressions into a single buffer.
 *
 *  Also `operator +` is defined for string expressions and string literals, string expressions and numbers
 *  (numbers are converted to decimal representation), and you can also add the desired types of string expressions yourself.
 *  Example:
 *    ```cpp
 *        stringa text = header + ", count = " + count + ", done";
 *    ```
 *  There are several types of string expressions out of the box to perform various operations on strings
 *      - `expr_spaces< Character Type, Number of Characters, Symbol>{}`: returns a string of length Number of Characters,
 *         filled with the specified character. The number of characters and character are compile-time constants.
 *         For some cases there is a shorthand notation:
 *          - `e_spca< Number of Characters>()`: char string of spaces
 *          - `e_spcw< Number of Characters>()`: w_char string of spaces
 *      - `expr_pad< Character Type>{Number of Characters, Symbol}`: produces a string long Number of Characters,
 *         filled with the specified character. The number of characters and the symbol can be set at runtime.
 *         Shorthand: `e_c(Number of Characters, Character)`
 *      - `e_choice(bool Condition, StrExpr1, StrExpr2)`: if Condition == true, the result will be StrExpr1, otherwise StrExpr2
 *      - `e_num< CharacterType>(IntegerNumber)`: Converts a number to decimal notation. Rarely used because
 *         for string expressions and numbers the "+" operator is redefined, and the number can simply be written as text + number;
 *      - `e_real< CharacterType>(RealNumber)`: Converts a number to decimal notation. Rarely used because
 *         for string expressions and numbers the "+" operator is overridden, and the number can simply be written as `text + number`;
 *      - `e_join< bool AfterLast = false, bool SkipEmpty = false>(container, "Separator")`: concatenates all strings
 *         in a container using a separator.
 *         If `AfterLast == true`, then the separator is added after the last element of the container, otherwise only
 *         between elements.
 *         If `Skip Empty == true`, then empty lines do not add a separator, otherwise for each empty line
 *         a separator is also inserted
 *      - `e_repl(SourceString, Search, Replace)`: replaces the occurrences of Search with Replace in the source string.
 *      - `e_hex(Number)`: generates a string with hexadecimal representation of the number.
 *      - `e_fill_left(StrExpr, width, symbol)`, `e_fill_right(StrExpr, width, symbol)`: fills a string expression to the required length with a given character.
 * etc. etc.
 *
 * String expressions for characters of different but compatible types can be combined into one expression.
 * That is, you can combine `char` and `char8_t`, and under Linux `wchar_t` and `char32_t`, under Windows `wchar_t` and `char16_t`.
 *
 * Remember: a string expression is not a string, it is only an "instruction" on how to assemble the string.
 */

/*!
 * @ingroup StrExprs
 * @ru @brief Концепт "Строковых выражений".
 * @details Это концепт, проверяющий, является ли тип "строковым выражением".
 * @en @brief Concept of "String Expressions".
 * @details This is a concept that checks whether a type is a "string expression".
 */
template<typename A>
concept StrExpr = requires(const A& a) {
    typename std::remove_cvref_t<A>::symb_type;
    { a.length() } -> std::convertible_to<size_t>;
    { a.place(std::declval<typename std::remove_cvref_t<A>::symb_type*>()) } -> std::same_as<typename std::remove_cvref_t<A>::symb_type*>;
};

/*!
 * @ingroup StrExprs
 * @ru @brief Концепт строкового выражения, совместимого с заданным типом символов.
 * @tparam A - проверяемый тип.
 * @tparam K - проверяемый тип символов.
 * @details Служит для задания ограничения к строковому выражению по типу символов.
 * @en @brief The concept of a string expression compatible with a given character type.
 * @tparam A - type being checked.
 * @tparam K - character type to be checked.
 * @details Used to set restrictions on a string expression by character type.
 */
template<typename A, typename K>
concept StrExprForType = StrExpr<A> && is_equal_str_type_v<K, typename std::remove_cvref_t<A>::symb_type>;

template<typename K, typename T>
struct convert_to_strexpr;

template<typename A, typename K>
concept strexpr_from = requires(const A& a) {
    {convert_to_strexpr<K, std::remove_cvref_t<A>>::convert(a)} -> StrExprForType<K>;
};

template<typename A, typename K>
concept strexpr_std = requires(const A& a) {
    {convert_to_strexpr<K, std::remove_cvref_t<A>>::convert(a)} -> StdStringForType<K>;
};

template<typename A, typename K>
concept strexpr_for = StrExprForType<typename convert_to_strexpr<K, std::remove_cvref_t<A>>::type, K>;

template<typename A, typename K>
concept to_strexpr_type = StrExprForType<typename std::remove_cvref_t<A>::strexpr, K>;

template<typename A, typename K>
concept to_strexpr_meth = requires(const A& a) {
    {a.template to_strexpr<K>()} -> StrExprForType<K>;
};

template<typename A, typename K>
concept to_strexpr_std = requires(const A& a) {
    {a.template to_strexpr<K>()} -> StdStringForType<K>;
};

template<typename A, typename K>
concept convertible_to_strexpr = strexpr_for<A, K> || strexpr_from<A, K> || strexpr_std<A, K> || to_strexpr_type<A, K> || to_strexpr_meth<A, K> || to_strexpr_std<A, K>;

template<typename K, strexpr_from<K> T>
constexpr auto to_strexpr(T&& t) {
    return convert_to_strexpr<K, std::remove_cvref_t<T>>::convert(std::forward<T>(t));
}

template<typename K, strexpr_for<K> T>
constexpr typename convert_to_strexpr<K, std::remove_cvref_t<T>>::type to_strexpr(T&& t) {
    return {std::forward<T>(t)};
}

template<typename K, to_strexpr_type<K> T>
constexpr typename std::remove_cvref_t<T>::strexpr to_strexpr(T&& t) {
    return {std::forward<T>(t)};
}

template<typename K, to_strexpr_meth<K> T>
constexpr auto to_strexpr(T&& t) {
    return t.template to_strexpr<K>();
}

template<typename K, StdStrSource T> requires is_equal_str_type_v<K, typename T::value_type>
struct expr_stdstr_c {
    using symb_type = K;
    T t_;

    expr_stdstr_c(T t) : t_(std::move(t)){}

    constexpr size_t length() const noexcept {
        return t_.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        size_t s = t_.size();
        std::char_traits<K>::copy(p, (const K*)t_.data(), s);
        return p + s;
    }
};

template<typename K, strexpr_std<K> T>
constexpr auto to_strexpr(T&& t) {
    using type = decltype(convert_to_strexpr<K, std::remove_cvref_t<T>>::convert(std::forward<T>(t)));
    return expr_stdstr_c<K, type>{convert_to_strexpr<K, std::remove_cvref_t<T>>::convert(std::forward<T>(t))};
}

template<typename K, to_strexpr_std<K> T>
constexpr auto to_strexpr(T&& t) {
    using type = decltype(t.template to_strexpr<K>());
    return expr_stdstr_c<K, type>{t.template to_strexpr<K>()};
}

template<typename K, typename T>
using convert_to_strexpr_t = decltype(to_strexpr<K>(std::declval<T>()));

/*
* Шаблонные классы для создания строковых выражений из нескольких источников.
* Благодаря компиляторно-шаблонной "магии" позволяют максимально эффективно
* получать результирующую строку - сначала вычисляется длина результирующей строки,
* потом один раз выделяется память для результата, после символы помещаются в
* выделенную память.
* Для конкатенация двух объектов строковых выражений в один

* Template classes for creating string expressions from multiple sources.
* Thanks to compiler-template "magic" they allow you to maximize efficiency
* get the resulting string - first the length of the resulting string is calculated,
* then memory is allocated once for the result, after which the characters are placed in
* allocated memory.
* For concatenating two string expression objects into one.
*/

template<typename K, typename Allocator, StrExpr A>
constexpr std::basic_string<K, std::char_traits<K>, Allocator> to_std_string(const A& expr) {
    std::basic_string<K, std::char_traits<K>, Allocator> res;
    if (size_t l = expr.length()) {
        auto fill = [&](K* ptr, size_t size) -> size_t {
            expr.place((typename A::symb_type*)ptr);
            return l;
        };
        if constexpr (requires { res.resize_and_overwrite(l, fill); }) {
            res.resize_and_overwrite(l, fill);
        } else if constexpr (requires{ res._Resize_and_overwrite(l, fill); }) {
            // Work in MSVC std lib before C++23
            res._Resize_and_overwrite(l, fill);
        } else {
            res.resize(l);  // bad, fill by 0 first.
            expr.place((typename A::symb_type*)res.data());
        }
    }
    return res;
}

/*!
 * @ru @brief Базовый класс для преобразования строковых выражений в стандартные строки
 * @details Если хотите, чтобы ваш тип строкового выражения конвертировался в стандартную строку,
 *  наследуйтесь от этого класса.
 * @tparam Impl - конечный класс-наследник, для CRTP.
 * @en @brief Base class for converting string expressions to standard strings
 * @details If you want your string expression type to be converted to a standard string,
 * inherit from this class.
 * @tparam Impl - final descendant class for CRTP.
 */
template<typename Impl>
struct expr_to_std_string {
    template<is_equal_str_type_v<typename Impl::symb_type> P, typename Allocator>
    constexpr operator std::basic_string<P, std::char_traits<P>, Allocator>() const {
        return to_std_string<P, Allocator>(*static_cast<const Impl*>(this));
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Шаблонный класс для конкатенации двух строковых выражений в одно с помощью `operator +`
 * @tparam A - Тип первого операнда
 * @tparam B - Тип второго операнда
 * @details Этот объект запоминает ссылки на два операнда операции сложения.
 * Когда у него запрашивают необходимый для результата размер буфера - он выдает сумму длин своих операндов.
 * Когда запрашивают размещение символов в буфере - размещает сначала первый операнд, затем второй.
 * @en @brief Template class for concatenating two string expressions into one using `operator +`
 * @tparam A - Type of first operand
 * @tparam B - Type of second operand
 * @details This object remembers references to the two operands of the addition operation.
 * When asked for the required buffer size for a result, it gives the sum of the lengths of its operands.
 * When asked to place characters in a buffer, place the first operand first, then the second.
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
struct strexprjoin : expr_to_std_string<strexprjoin<A, B>>{
    using symb_type = typename A::symb_type;
    const A& a;
    const B& b;
    constexpr strexprjoin(const A& a_, const B& b_) : a(a_), b(b_){}
    constexpr size_t length() const noexcept {
        return a.length() + b.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        return (symb_type*)b.place((typename B::symb_type*)a.place(p));
    }
};

/*!
 * @ingroup StrExprs
 * @anchor op_plus_str_expr
 * @ru @brief Оператор сложения двух произвольных строковых выражения для одинакового типа символов.
 * @param a - первое строковое выражение.
 * @param b - второе строковое выражение.
 * @return strexprjoin<A, B>, строковое выражение, генерирующее объединение переданных выражений.
 * @details Когда складываются два объекта - строковых выражения, один типа `A`, другой типа `B`,
 * мы возвращаем объект типа strexprjoin<A, B>, который содержит ссылки на два этих операнда.
 * А сам объект strexprjoin<A, B> тоже в свою очередь является строковым выражением, и может участвовать
 * в следующих операциях сложения. Таким образом формируется "дерево" из исходных строковых
 * выражений, которое потом за один вызов "материализуется" в конечный результат.
 *
 * @en @brief An addition operator for two arbitrary string expressions of the same character type.
 * @param a - first string expression
 * @param b - second string expression
 * @return strexprjoin<A, B>, a string expression that generates a join of the given expressions.
 * @details When two objects are added - string expressions, one of type `A`, the other of type `B`,
 * we return an object of type strexprjoin<A, B>, which contains references to these two operands.
 * And the strexprjoin<A, B> object itself, in turn, is also a string expression, and can participate
 * in the following addition operations. In this way, a “tree” is formed from the original strings
 * expressions, which are then “materialized” into the final result in one call.
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
constexpr strexprjoin<A, B> operator+(const A& a, const B& b) {
    return {a, b};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Конкатенация ссылки на строковое выражение и значения строкового выражения.
 * @tparam A - Тип одного строкового выражения.
 * @tparam B - Тип другого строкового выражения.
 * @tparam last - какое из них первое.
 * @details Чтобы иметь возможность складывать строковое выражение с операндами, не являющимися строковым выражением,
 *  нам нужно иметь возможность вернуть из `operator+` объект, который сохранит ссылку на операнд, являющийся строковым
 *  выражением, а для не строкового операнда будет иметь поле со строковым выражением, обрабатывающим второй операнд.
 *  Можно посмотреть пример в simstr::operator+<StrExpr A, FromIntNumber T>()
 *
 * @en @brief Concatenation of a reference to a string expression and the value of the string expression.
 * @tparam A - Type of a single string expression.
 * @tparam B - Type of another string expression.
 * @tparam last - which one is the first.
 * @details To be able to add a string expression with non-string operands,
 * we need to be able to return an object from `operator+` that will retain a reference to the operand, which is a string
 * expression, and for a non-string operand will have a field with a string expression that processes the second operand.
 * You can see an example in simstr::operator+<StrExpr A, FromIntNumber T>()
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B, bool last = true>
struct strexprjoin_c : expr_to_std_string<strexprjoin_c<A, B, last>>{
    using symb_type = typename A::symb_type;
    const A& a;
    B b;
    template<typename... Args>
    constexpr strexprjoin_c(const A& a_, Args&&... args_) : a(a_), b(std::forward<Args>(args_)...) {}
    constexpr size_t length() const noexcept {
        return a.length() + b.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (last) {
            return (symb_type*)b.place((typename B::symb_type*)a.place(p));
        } else {
            return a.place((symb_type*)b.place((typename B::symb_type*)p));
        }
    }
};

/*!
 * @ru @brief Оператор сложения строкового выражения и типов, для которых есть преобразование в строковое выражение.
 * @tparam A - тип строкового выражения.
 * @tparam B - тип слагаемого.
 * @param a - строковое выражение.
 * @param b - слагаемое.
 * @return constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, true>
 * @en @brief Addition operator for string expressions and types for which there is a conversion to string expressions.
 * @tparam A - the type of the string expression.
 * @tparam B - the type of the addend.
 * @param a - the string expression.
 * @param b - the addend.
 * @return constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, true>
 */
template<StrExpr A, convertible_to_strexpr<typename A::symb_type> B>
inline constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, true> operator+(const A& a, B&& b) {
    return {a, to_strexpr<typename A::symb_type>(std::forward<B>(b))};
}

/*!
 * @ru @brief Оператор сложения типов, для которых есть преобразование в строковое выражение и строкового выражения.
 * @tparam A - тип строкового выражения.
 * @tparam B - тип слагаемого.
 * @param b - слагаемое.
 * @param a - строковое выражение.
 * @return constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, true>
 * @en @brief Addition operator for types for which there is a conversion to string expressions and string expressions.
 * @tparam A - the type of the string expression.
 * @tparam B - the type of the addend.
 * @param b - the addend.
 * @param a - the string expression.
 * @return constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, false>
 */
template<StrExpr A, convertible_to_strexpr<typename A::symb_type> B>
inline constexpr strexprjoin_c<A, convert_to_strexpr_t<typename A::symb_type, B>, false> operator+(B&& b, const A& a) {
    return {a, to_strexpr<typename A::symb_type>(std::forward<B>(b))};
}

/*!
 * @ingroup StrExprs
 * @ru @brief "Пустое" строковое выражение.
 * @tparam K - тип символа.
 * @details Простое строковое выражение, генерирующее пустую строку.
 *  В основном применяется в функции e_choice, когда одна из веток должна вернуть пустую строку.
 *  Либо для начала операции сложения строковых выражений, когда другой операнд не является строковым выражением,
 *  но для него есть оператор сложения со строковыми выражениями.
 *  Для удобства уже определены константные объекты этого типа для разных видов символов:
 *  - eea для пустой строки char
 *  - eew для пустой строки wchar_t
 *  - eeu для пустой строки char16_t
 *  - eeuu для пустой строки char32_t
 *
 * @en @brief An "empty" string expression.
 * @tparam K is a symbol.
 * @details A simple string expression that generates an empty string.
 * Mainly used in the e_choice function when one of the branches should return an empty string.
 * Either to start the addition operation of string expressions when the other operand is not a string expression,
 * but there is an addition operator for it with string expressions.
 * For convenience, constant objects of this type have already been defined for different types of symbols:
 *  - eea for empty char string
 *  - eew for empty string wchar_t
 *  - eeu for empty string char16_t
 *  - eeuu for empty string char32_t
 *
 *
 *  @ru Пример: @en Example: @~
 *  ```cpp
 *  // строковые литералы и числа не являются строковыми выражениями, нам надо с чего то начать операцию сложения со строковым выражением
 *  // string literals and numbers are not string expressions, we need to start the addition operation with a string expression somewhere
 *  result = eea + "Count is " + count;
 *  ```
 */
template<typename K>
struct empty_expr : expr_to_std_string<empty_expr<K>>{
    using symb_type = K;
    constexpr size_t length() const noexcept {
        return 0;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        return p;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Пустое строковое выражение типа char.
 * @en @brief Empty string expression of type char.
 */
inline constexpr empty_expr<u8s> eea{};
/*!
 * @ingroup StrExprs
 * @ru @brief Пустое строковое выражение типа char8_t.
 * @en @brief Empty string expression of type char8_t.
 */
inline constexpr empty_expr<u8s> eeb{};
/*!
 * @ingroup StrExprs
 * @ru @brief Пустое строковое выражение типа wchar_t.
 * @en @brief Empty string expression of type wchar_t.
 */
inline constexpr empty_expr<uws> eew{};
/*!
 * @ingroup StrExprs
 * @ru @brief Пустое строковое выражение типа char16_t.
 * @en @brief Empty string expression of type char16_t.
 */
inline constexpr empty_expr<u16s> eeu{};
/*!
 * @ingroup StrExprs
 * @ru @brief Пустое строковое выражение типа char32_t.
 * @en @brief Empty string expression of type char32_t.
 */
inline constexpr empty_expr<u32s> eeuu{};

template<typename K>
struct expr_char : expr_to_std_string<expr_char<K>>{
    using symb_type = K;
    K value;
    constexpr expr_char(K v) : value(v){}
    constexpr size_t length() const noexcept {
        return 1;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        *p++ = value;
        return p;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения строкового выражения и одного символа.
 * @return строковое выражение, объединяющее переданное выражение и символ.
 * @en @brief Addition operator of a string expression and one character.
 * @return a string expression that combines the passed expression and a character.
 * @details @ru Пример: @en Example: @~
 * @~
 *  ```cpp
 *  reply = prompt + '>' + result;
 *  ```
 */
template<typename K, StrExprForType<K> A>
constexpr strexprjoin_c<A, expr_char<K>> operator+(const A& a, K s) {
    return {a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из 1 заданного символа.
 * @param s - символ.
 * @return строковое выражение для строки из одного символа.
 *
 * @en @brief Generates a string of 1 given character.
 * @param s - symbol.
 * @return string expression for a single character string.
 */
template<typename K>
constexpr expr_char<K> e_char(K s) {
    return {s};
}

template<typename K, size_t N>
struct expr_literal : expr_to_std_string<expr_literal<K, N>> {
    using symb_type = K;
    const K (&str)[N + 1];
    constexpr expr_literal(const K (&str_)[N + 1]) : str(str_){}

    constexpr size_t length() const noexcept {
        return N;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (N != 0)
            std::char_traits<K>::copy(p, str, N);
        return p + N;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Преобразует строковый литерал в строковое выражение.
 * @details Строковые литералы сами по себе не являются строковыми выражениями.
 * Обычно в операциях конкатенации это не вызывает проблем, так как второй операнд уже является строковым выражением,
 * и для него срабатывает сложение с литералом. Но есть ситуации, когда второй операнд тоже не является
 * строковым выражением. Например:
 *  ```cpp
 *  int intVar = calculate();
 *  ...
 *  res = "text" + intVar;
 *  ...
 *  res = intVar + "text";
 *  ```
 *  В этом случае можно преобразовать литерал в строковое выражение двумя способами:
 * - дописать _ss: `"text"_ss`, что преобразует литерал в simple_str_nt: `res = "text"_ss + intVar`
 * - применить e_t: `e_t("text")`, что преобразует литерал в expr_literal: `res = e_t("text") + intVar`
 *
 * Во втором способе компилятор может более агрессивно применить оптимизации, связанные с известным при компиляции
 * размером литерала.
 *
 * Хотя строго говоря, в этих ситуации можно пользоваться и другими способами:
 * - Добавить операнд - пустое строковое выражение: `result = eea + "text" + intVar`, `result = "text" + eea + intVar`
 * - Преобразовать другой операнд в строковое выражение: `result = "text" + e_num<u8s>(intVar)`.
 *
 * Все эти способы работают и выдают одинаковый результат. Каким пользоваться - дело вкуса.
 *
 * @en @brief Converts a string literal to a string expression.
 * @details String literals are not themselves string expressions.
 * This usually does not cause problems in concatenation operations, since the second operand is already a string expression,
 * and addition with a literal works for it. But there are situations when the second operand is not either
 * string expression. For example:
 *  ```cpp
 *  int intVar = calculate();
 *  ...
 * res = "text" + intVar;
 *  ...
 * res = intVar + "text";
 *  ```
 * In this case, you can convert the literal to a string expression in two ways:
 * - add _ss: `"text"_ss`, which converts the literal to simple_str_nt: `res = "text"_ss + intVar`
 * - apply e_t: `e_t("text")`, which converts the literal to expr_literal: `res = e_t("text") + intVar`
 *
 * In the second method, the compiler can more aggressively apply optimizations related to what is known at compilation
 * literal size.
 *
 * Although strictly speaking, in these situations you can use other methods:
 * - Add an operand - an empty string expression: `result = eea + "text" + intVar`, `result = "text" + eea + intVar`
 * - Convert another operand to a string expression: `result = "text" + e_num<u8s>(intVar)`.
 *
 * All these methods work and give the same result. Which one to use is a matter of taste.
 */
template<typename T, size_t N = const_lit<T>::Count>
constexpr expr_literal<typename const_lit<T>::symb_type, static_cast<size_t>(N - 1)> e_t(T&& s) {
    return {s};
}

template<bool first, typename K, size_t N, typename A>
struct expr_literal_join : expr_to_std_string<expr_literal_join<first, K, N, A>> {
    using symb_type = K;
    using atype = typename A::symb_type;
    const K (&str)[N + 1];
    const A& a;
    constexpr expr_literal_join(const K (&str_)[N + 1], const A& a_) : str(str_), a(a_){}

    constexpr size_t length() const noexcept {
        return N + a.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (N != 0) {
            if constexpr (first) {
                std::char_traits<K>::copy(p, str, N);
                return (symb_type*)a.place((atype*)(p + N));
            } else {
                p = (symb_type*)a.place((atype*)p);
                std::char_traits<K>::copy(p, str, N);
                return p + N;
            }
        } else {
            return a.place(p);
        }
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для строкового выражения и строкового литерала такого же типа символов.
 * @return Строковое выражение, объединяющее операнды.
 * @en @brief The addition operator for a string expression and a string literal of the same character type.
 * @return A string expression concatenating the operands.
 */
template<StrExpr A, typename T, typename P = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count> requires is_equal_str_type_v<typename A::symb_type, P>
constexpr expr_literal_join<false, P, (N - 1), A> operator+(const A& a, T&& s) {
    return {s, a};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для строкового литерала такого же типа символов и строкового выражения.
 * @return Строковое выражение, объединяющее операнды.
 * @en @brief The addition operator for a string literal of the same character type and string expression.
 * @return A string expression concatenating the operands.
 */
template<StrExpr A, typename T, typename P = typename const_lit<T>::symb_type, size_t N = const_lit<T>::Count> requires is_equal_str_type_v<typename A::symb_type, P>
constexpr expr_literal_join<true, P, (N - 1), A> operator+(T&& s, const A& a) {
    return {s, a};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Тип строкового выражения, возвращающего N заданных символов.
 * @details Количество символов и сам символ константы, т.е. задаются при компиляции.
 * @tparam K - тип символа.
 * @tparam N - количество символов.
 * @tparam S - символ, по умолчанию пробел.
 * @en @brief A type of string expression that returns N specified characters.
 * @details The number of characters and the constant symbol itself, i.e. are specified during compilation.
 * @tparam K is a symbol.
 * @tparam N - number of characters.
 * @tparam S - character, space by default.
 */
template<typename K, size_t N, size_t S = ' '>
struct expr_spaces : expr_to_std_string<expr_spaces<K, N>> {
    using symb_type = K;
    constexpr size_t length() const noexcept {
        return N;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (N != 0)
            std::char_traits<K>::assign(p, N, static_cast<K>(S));
        return p + N;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из N char пробелов.
 * @tparam N - Количество пробелов.
 * @return строковое выражение для N char пробелов.
 * @en @brief Generates a string of N char spaces.
 * @tparam N - Number of spaces.
 * @return string expression for N char spaces.
 * @details @ru Пример: @en Example: @~
 *  ```cpp
 *  stringa text = e_spca<10>() + text + e_spca<10>();
 *  ```
 */
template<size_t N>
constexpr expr_spaces<u8s, N> e_spca() {
    return {};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из N wchar_t пробелов.
 * @tparam N - Количество пробелов.
 * @return строковое выражение для N wchar_t пробелов.
 * @en @brief Generates a string of N wchar_t spaces.
 * @tparam N - Number of spaces.
 * @return string expression for N wchar_t spaces.
 * @~ @details @ru Пример: @en Example: @~
 *  ```cpp
 *  stringw text = e_spcw<10>() + text + e_spcw<10>();
 *  ```
 */
template<size_t N>
constexpr expr_spaces<uws, N> e_spcw() {
    return {};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Тип строкового выражения, возвращающего N заданных символов.
 * @tparam K - тип символа.
 * @details Количество символов и сам символ переменные, т.е. могут меняться в рантайм.
 *  Напрямую обычно не используется, создается через e_c().
 * @en @brief A type of string expression that returns N specified characters.
 * @tparam K is a symbol.
 * @details The number of characters and the character itself are variable, i.e. can change at runtime.
 * Usually not used directly, created via e_c().
 */
template<typename K>
struct expr_pad : expr_to_std_string<expr_pad<K>> {
    using symb_type = K;
    size_t len;
    K s;
    constexpr expr_pad(size_t len_, K s_) : len(len_), s(s_){}
    constexpr size_t length() const noexcept {
        return len;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if (len)
            std::char_traits<K>::assign(p, len, s);
        return p + len;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из l символов s типа K.
 * @tparam K - тип символа.
 * @param l - количество символов.
 * @param s - символ.
 * @return строковое выражение, генерирующее строку из l символов k.
 * @en @brief Generates a string of l characters s of type K.
 * @tparam K is a symbol.
 * @param l - number of characters.
 * @param s - symbol.
 * @return a string expression that generates a string of l characters k.
 */
template<typename K>
constexpr expr_pad<K> e_c(size_t l, K s) {
    return { l, s };
}

template<typename K, size_t N>
struct expr_repeat_lit : expr_to_std_string<expr_repeat_lit<K, N>> {
    using symb_type = K;
    size_t repeat_;
    const K (&s)[N + 1];
    constexpr expr_repeat_lit(size_t repeat, const K (&s_)[N + 1]) : repeat_(repeat), s(s_){}
    constexpr size_t length() const noexcept {
        return N * repeat_;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (N) {
            for (size_t i = 0; i < repeat_; i++) {
                std::char_traits<K>::copy(p, s, N);
                p += N;
            }
        }
        return p;
    }
};

template<StrExpr A>
struct expr_repeat_expr : expr_to_std_string<expr_repeat_expr<A>> {
    using symb_type = typename A::symb_type;
    size_t repeat_;
    const A& expr_;
    constexpr expr_repeat_expr(size_t repeat, const A& expr) : repeat_(repeat), expr_(expr){}
    constexpr size_t length() const noexcept {
        if (repeat_) {
            return repeat_ * expr_.length();
        }
        return 0;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if (repeat_) {
            if (repeat_ == 1) {
                return expr_.place(p);
            }
            symb_type* start = p;
            p = expr_.place(p);
            size_t len = size_t(p - start);
            if (len) {
                for (size_t i = 1; i < repeat_; i++) {
                    std::char_traits<symb_type>::copy(p, start, len);
                    p += len;
                }
            }
        }
        return p;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из l строковых констант s типа K
 * @tparam K - тип символа
 * @param l - количество повторов
 * @param s - строковый литерал
 * @return строковое выражение, генерирующее строку из l строк s
 * @en @brief Generate a string from l string constants s of type K
 * @tparam K - character type
 * @param l - number of repetitions
 * @param s - string literal
 * @return a string expression generating a string of l strings s
 */
template<typename T, typename K = const_lit<T>::symb_type, size_t M = const_lit<T>::Count> requires (M > 0)
constexpr expr_repeat_lit<K, M - 1> e_repeat(T&& s, size_t l) {
    return { l, s };
}

/*!
 * @ingroup StrExprs
 * @ru @brief Генерирует строку из l строковых выражений s типа K
 * @tparam K - тип символа
 * @param l - количество повторов
 * @param s - строковое выражение
 * @return строковое выражение, генерирующее строку из l строковых выражений s
 * @en @brief Generate a string from l string expressions s of type K
 * @tparam K - character type
 * @param l - number of repetitions
 * @param s - string expression
 * @return a string expression generating a string of l string expressions s
 */
template<StrExpr A>
constexpr expr_repeat_expr<A> e_repeat(const A& s, size_t l) {
    return { l, s };
}

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение условного выбора.
 * @tparam A Тип ветки для true.
 * @tparam B Тип ветки для false.
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо выражение B.
 *  Напрямую тип обычно не используется, создаётся через e_choice().
 * @en @brief Conditional selection string expression.
 * @tparam A Branch type for true.
 * @tparam B Branch type for false.
 * @details An expression that, depending on the truth of the condition, generates either expression A or expression B.
 * The type is usually not used directly; it is created via e_choice().
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
struct expr_choice : expr_to_std_string<expr_choice<A, B>> {
    using symb_type = typename A::symb_type;
    using my_type = expr_choice<A, B>;
    const A& a;
    const B& b;
    bool choice;

    constexpr expr_choice(const A& _a, const B& _b, bool _choice) : a(_a), b(_b), choice(_choice){}

    constexpr size_t length() const noexcept {
        return choice ? a.length() : b.length();
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        return choice ? a.place(ptr) : (symb_type*)b.place((typename B::symb_type*)ptr);
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение условного выбора.
 * @tparam A Тип ветки для true.
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо пустую строку.
 *  Напрямую тип обычно не используется, создаётся через e_if().
 * @en @brief Conditional selection string expression.
 * @tparam A Branch type for true.
 * @details An expression that, depending on the truth of the condition, generates either expression A or an empty string.
 * Title type usually not used, create through e_if().
 */
template<StrExpr A>
struct expr_if : expr_to_std_string<expr_if<A>> {
    using symb_type = typename A::symb_type;
    using my_type = expr_if<A>;
    const A& a;
    bool choice;
    constexpr expr_if(const A& _a, bool _choice) : a(_a), choice(_choice){}

    constexpr size_t length() const noexcept {
        return choice ? a.length() : 0;
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        return choice ? a.place(ptr) : ptr;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение условного выбора.
 * @tparam A Тип ветки для true.
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо строку из строкового литерала.
 *  Напрямую тип обычно не используется, создаётся через e_choice().
 *
 *  Так как строковые литералы не являются строковыми выражениями, то использовать их в виде одиночного выражения в частях
 *  e_choice или e_if требовало бы их обрамления какими-либо конструкциями, преобразующими их в строковое выражение.
 *  Приходилось бы писать например так:
 *  ```cpp
 *  e_choice(condition, text, e_t("empty"));
 *  e_choice(condition, text, eea + "empty");
 *  e_choice(condition, text, "empty"_ss);
 *  e_if(!condition, "empty"_ss);
 *  ```
 *  Это, с одной стороны - захламляет код, с другой - делает его менее оптимальным.
 *  Поэтому для таких случаев сделаны перегрузки e_choice и e_if для случаев, когда их параметрами являются строковые литералы.
 *  В этих перегрузках и используются expr_choice_one_lit и expr_choice_two_lit, позволяя писать так:
 *  ```cpp
 *  e_choice(condition, text, "empty");
 *  e_choice(condition, "false", "true");
 *  e_if(!condition, "empty");
 *  ```
 * @en @brief Conditional selection string expression.
 * @tparam A Branch type for true.
 * @details An expression that, depending on the truth of the condition, generates either expression A or a string from a string literal.
 * The type is usually not used directly; it is created via e_choice().
 *
 * Since string literals are not string expressions, use them as a single expression in parts
 * e_choice or e_if would require them to be surrounded by some constructs that convert them to a string expression.
 * You would have to write something like this:
 *  ```cpp
 *  e_choice(condition, text, e_t("empty"));
 *  e_choice(condition, text, eea + "empty");
 *  e_choice(condition, text, "empty"_ss);
 *  e_if(!condition, "empty"_ss);
 *  ```
 * This, on the one hand, clutters up the code, on the other, makes it less optimal.
 * These overloads use expr_choice_one_lit and expr_choice_two_lit, allowing you to write like this:
 *  ```cpp
 *  e_choice(condition, text, "empty");
 *  e_choice(condition, "false", "true");
 *  e_if(!condition, "empty");
 *  ```
 */
template<typename L, StrExprForType<L> A, size_t N, bool Compare>
struct expr_choice_one_lit : expr_to_std_string<expr_choice_one_lit<L, A, N, Compare>> {
    using symb_type = L;
    const symb_type (&str)[N + 1];
    const A& a;
    bool choice;
    constexpr expr_choice_one_lit(const symb_type (&_str)[N + 1], const A& _a, bool _choice) : str(_str), a(_a), choice(_choice){}

    constexpr size_t length() const noexcept {
        return choice == Compare ? a.length() : N;
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        if (choice == Compare) {
            return (L*)a.place((typename A::symb_type*)ptr);
        }
        if constexpr (N != 0) {
            std::char_traits<symb_type>::copy(ptr, str, N);
        }
        return ptr + N;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение условного выбора.
 * @details Выражение, в зависимости от истинности условия генерирующее либо один строковый литерал, либо другой.
 *  Напрямую тип обычно не используется, создаётся через e_choice().
 *
 *  Так как строковые литералы не являются строковыми выражениями, то использовать их в виде одиночного выражения в частях
 *  e_choice или e_if требовало бы их обрамления какими-либо конструкциями, преобразующими их в строковое выражение.
 *  Приходилось бы писать например так:
 *  ```cpp
 *  e_choice(condition, text, e_t("empty"));
 *  e_choice(condition, text, eea + "empty");
 *  e_choice(condition, text, "empty"_ss);
 *  e_if(!condition, "empty"_ss);
 *  ```
 *  Это, с одной стороны - захламляет код, с другой - делает его менее оптимальным.
 *  Поэтому для таких случаев сделаны перегрузки e_choice и e_if для случаев, когда их параметрами являются строковые литералы.
 *  В этих перегрузках и используются expr_choice_one_lit и expr_choice_two_lit, позволяя писать так:
 *  ```cpp
 *  e_choice(condition, text, "empty");
 *  e_choice(condition, "false", "true");
 *  e_if(!condition, "empty");
 *  ```
 * @en @brief Conditional selection string expression.
 * @details An expression that, depending on the truth of the condition, generates either one string literal or another.
 * The type is usually not used directly; it is created via e_choice().
 *
 * Since string literals are not string expressions, use them as a single expression in parts
 * e_choice or e_if would require them to be surrounded by some constructs that convert them to a string expression.
 * You would have to write something like this:
 *  ```cpp
 *  e_choice(condition, text, e_t("empty"));
 *  e_choice(condition, text, eea + "empty");
 *  e_choice(condition, text, "empty"_ss);
 *  e_if(!condition, "empty"_ss);
 *  ```
 * This, on the one hand, clutters up the code, on the other, makes it less optimal.
 * These overloads use expr_choice_one_lit and expr_choice_two_lit, allowing you to write like this:
 *  ```cpp
 *  e_choice(condition, text, "empty");
 *  e_choice(condition, "false", "true");
 *  e_if(!condition, "empty");
 *  ```
 */
template<typename K, size_t N, typename P, size_t M>
struct expr_choice_two_lit : expr_to_std_string<expr_choice_two_lit<K, N, P, M>> {
    using symb_type = K;
    const K (&str_a)[N + 1];
    const P (&str_b)[M + 1];
    bool choice;
    constexpr expr_choice_two_lit(const K(&_str_a)[N + 1], const P(&_str_b)[M + 1], bool _choice)
        : str_a(_str_a), str_b(_str_b), choice(_choice){}

    constexpr size_t length() const noexcept {
        return choice ? N : M;
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        if (choice) {
            if constexpr (N != 0) {
                std::char_traits<symb_type>::copy(ptr, str_a, N);
            }
            return ptr + N;
        }
        if constexpr (M != 0) {
            std::char_traits<symb_type>::copy(ptr, (const K(&)[M + 1])str_b, M);
        }
        return ptr + M;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Создание условного строкового выражения expr_choice.
 * @tparam A - Тип выражение при истинности условия, выводится из аргумента.
 * @tparam B - Тип выражения при ложности условия, выводится из аргумента.
 * @param c - булево условие.
 * @param a - строковое выражение, выполняющееся при `c == true`.
 * @param b - строковое выражение, выполняющееся при `c == false`.
 * @details Служит для возможности в одном выражении выбирать разные варианты в зависимости от условия.
 * @en @brief Create a conditional string expression expr_choice.
 * @tparam A - Type expression when the condition is true, inferred from the argument.
 * @tparam B - The type of expression when the condition is false, inferred from the argument.
 * @param c is a Boolean condition.
 * @param a is a string expression that is executed when `c == true`.
 * @param b is a string expression that is executed when `c == false`.
 * @details Serves to allow you to select different options in one expression depending on the condition.
 *
 *  @ru Примеры: @en Example: @~
 *  ```cpp
 *  columns_metadata.emplace_back(e_choice(name.is_empty(), "?column?", name) + "::" + metadata_column.type.to_string());
 *  ```
 *  ```cpp
 *  lstringa<512> str = e_choice(!ret_type_resolver_, sql_value::type_name(ret_type_), "any") + " " + name_ + "(";
 *  ```
 *  @ru Иначе такие операции приходилось бы разбивать на несколько модификаций строки или применению временных строк,
 *  что не оптимально и снизит производительность. (Это проверяется в бенчмарке "Build Full Func Name")
 *  @en Otherwise, such operations would have to be split into several string modifications or the use of temporary strings,
 *  which is not optimal and will reduce performance. (This is checked in the "Build Full Func Name" benchmark)
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
constexpr expr_choice<A, B> e_choice(bool c, const A& a, const B& b) {
    return {a, b, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда третий аргумент - строковый литерал.
 * @en @brief Overload e_choice when the third argument is a string literal.
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
constexpr expr_choice_one_lit<typename const_lit<T>::symb_type, A, N - 1, true> e_choice(bool c, const A& a, T&& str) {
    return {str, a, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда второй аргумент - строковый литерал.
 * @en @brief Overload e_choice when the second argument is a string literal.
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
constexpr expr_choice_one_lit<typename const_lit<T>::symb_type, A, N - 1, false> e_choice(bool c, T&& str, const A& a) {
    return {str, a, c};
}
/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда второй и третий аргумент - строковые литералы.
 * @en @brief Overload e_choice when the second and third arguments are string literals.
 */
template<typename T, typename L, typename K = typename const_lit<T>::symb_type, typename P = typename const_lit<L>::symb_type,
    size_t N = const_lit<T>::Count, size_t M = const_lit_for<typename const_lit<T>::symb_type, L>::Count>
    requires is_equal_str_type_v<K, P>
constexpr expr_choice_two_lit<K, N -1, P, M - 1> e_choice(bool c, T&& str_a, L&& str_b) {
    return {str_a, str_b, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Создание условного строкового выражения expr_if
 * @tparam A - Тип выражение при истинности условия, выводится из аргумента
 * @param c - булево условие
 * @param a - строковое выражение, выполняющееся при `c == true`
 * @details Служит для возможности в одном выражении генерировать в зависимости от условия либо указанный вариант, либо пустую строку.
 * @en @brief Creating a conditional string expression expr_if
 * @tparam A - Type expression when the condition is true, inferred from the argument
 * @param c - boolean condition
 * @param a - string expression executed when `c == true`
 * @details Serves to allow one expression to generate, depending on the condition, either the specified option or an empty string.
 *
 * @ru Примеры: @en Example @~
 *  ```cpp
 *  void sql_func_info::build_full_name() {
 *      // Временный буфер для результата, возьмём с запасом
 *      // Temporary buffer for the result, take it with reserve
 *      lstringa<512> str = e_choice(!ret_type_resolver_, sql_value::type_name(ret_type_), "any") + " " + name_ + "(";
 *
 *      bool add_comma = false;
 *
 *      for (const auto& param : params_) {
 *          str += e_if(add_comma, ", ") + e_if(param.optional_, "[");
 *          // Добавляет к str названия допустимых типов
 *          // Adds the names of valid types to str
 *          param.allowed_types.to_string(str);
 *          if (param.optional_) {
 *              str += "]";
 *          }
 *          add_comma = true;
 *      }
 *      // Сохраним в stringa
 *      // Save it in stringa
 *      full_name_ = str + e_if(unlim_params_, e_if(add_comma, ", ") + "...") + ")";
 *  }
 *  ```
 *  @ru Иначе такие операции приходилось бы разбивать на несколько модификаций строки или применению временных строк,
 *  что не оптимально и снизит производительность. (Этот пример проверяется в бенчмарке "Build Full Func Name")
 *  @en Otherwise, such operations would have to be split into several string modifications or the use of temporary strings,
 *  which is not optimal and will reduce performance. (This example is tested in the "Build Full Func Name" benchmark)
 */
template<StrExpr A>
constexpr expr_if<A> e_if(bool c, const A& a) {
    return {a, c};
}
/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_if, когда второй аргумент - строковый литерал.
 * @en @brief Overload e_if when the second argument is a string literal.
 */
template<typename T, size_t N = const_lit<T>::Count>
constexpr auto e_if(bool c, T&& str) {
    using K = typename const_lit<T>::symb_type;
    const K empty[1] = {0};
    return expr_choice_two_lit<K, N - 1, K, 0>{str, empty, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Тип для использования std::basic_string и std::basic_string_view как источников в строковых выражениях.
 * @tparam K - тип символа.
 * @tparam T - тип источника.
 * @en @brief A type for using std::basic_string and std::basic_string_view as sources in string expressions.
 * @tparam K is a symbol.
 * @tparam T - source type.
 */
template<typename K, StdStrSourceForType<K> T>
struct expr_stdstr {
    using symb_type = K;
    const T& t_;

    expr_stdstr(const T& t) : t_(t){}

    constexpr size_t length() const noexcept {
        return t_.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        size_t s = t_.size();
        std::char_traits<K>::copy(p, (const K*)t_.data(), s);
        return p + s;
    }
};

/*!
 * @ru @brief Специализация шаблона для преобразования стандартный строк в строковое выражение, позволяет использовать их в
 * операциях конкатенации со строковыми выражениями.
 * @tparam K - тип символов строкового выражения.
 * @tparam T - тип числа.
 * @en @brief A template specialization for converting standard strings to string expressions, allowing their use in
 * concatenation operations with string expressions.
 * @tparam K - the character type of the string expression.
 * @tparam T - the number type.
 */
template<typename K, StdStrSource T>
struct convert_to_strexpr<K, T> {
    using type = expr_stdstr<K, T>;
};

namespace str {
constexpr const size_t npos = static_cast<size_t>(-1); //NOLINT
} // namespace str

template<typename K>
struct ch_traits : std::char_traits<K>{};

template<typename T>
concept FromIntNumber =
    is_one_of_type<std::remove_cv_t<T>, unsigned char, int, short, long, long long, unsigned, unsigned short, unsigned long, unsigned long long>::value;

template<typename T>
concept ToIntNumber = FromIntNumber<T> || is_one_of_type<T, int8_t>::value;

template<typename K, bool I, typename T>
struct need_sign { // NOLINT
    bool negate;
    std::make_unsigned_t<T> val;
    constexpr need_sign(T t) : negate(t < 0), val(t < 0 ? std::make_unsigned_t<T>{} - t : t) {}
    constexpr void after(K*& ptr) {
        if (negate)
            *--ptr = '-';
    }
};

template<typename K, typename T>
struct need_sign<K, false, T> {
    T val;
    constexpr need_sign(T t) : val(t){}
    constexpr void after(K*&) {}
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
        need_sign<K, std::is_signed_v<T>, T> store(val);
        K* itr = bufEnd;
        while (store.val >= 100) {
            const char* ptr = twoDigit + (store.val % 100) * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
            store.val /= 100;
        }
        if (store.val < 10) {
            *--itr = static_cast<K>('0' + store.val);
        } else {
            const char* ptr = twoDigit + store.val * 2;
            *--itr = static_cast<K>(ptr[1]);
            *--itr = static_cast<K>(ptr[0]);
        }
        store.after(itr);
        return size_t(bufEnd - itr);
    }
    bufEnd[-1] = '0';
    return 1;
}

template<typename K, typename T>
struct expr_num : expr_to_std_string<expr_num<K, T>> {
    using symb_type = K;
    using my_type = expr_num<K, T>;

    enum { bufSize = 24 };
    mutable K buf[bufSize];
    mutable T value;

    constexpr expr_num(T t) : value(t) {}
    constexpr expr_num(expr_num&& t) noexcept : value(t.value) {}

    constexpr size_t length() const noexcept {
        value = (T)fromInt(buf + bufSize, value);
        return (size_t)value;
    }
    constexpr K* place(K* ptr) const noexcept {
        size_t len = (size_t)value;
        ch_traits<K>::copy(ptr, buf + bufSize - len, len);
        return ptr + len;
    }
};

/*!
 * @ru @brief Специализация шаблона для преобразования целых чисел в строковое выражение, позволяет использовать их в
 * операциях конкатенации со строковыми выражениями.
 * @tparam K - тип символов строкового выражения.
 * @tparam T - тип числа.
 * @en @brief A template specialization for converting integers to string expressions, allowing their use in
 * concatenation operations with string expressions.
 * @tparam K - the character type of the string expression.
 * @tparam T - the number type.
 */
template<typename K, FromIntNumber T>
struct convert_to_strexpr<K, T> {
    using type = expr_num<K, T>;
};

/*!
 * @ingroup StrExprs
 * @ru @brief Преобразование целого числа в строковое выражение.
 * @tparam K - тип символов.
 * @tparam T - тип числа, выводится из аргумента.
 * @param t - число.
 * @details Возвращает строковое выражение, которое генерирует десятичное представление заданного числа.
 * Может использоваться, когда надо конкатенировать число и строковый литерал.
 * @en @brief Convert an integer to a string expression.
 * @tparam K - character type.
 * @tparam T - number type, inferred from the argument.
 * @param t - number.
 * @details Returns a string expression that generates the decimal representation of the given number.
 * Can be used when you need to concatenate a number and a string literal.
 */
template<typename K, FromIntNumber T>
constexpr expr_num<K, T> e_num(T t) {
    return {t};
}

namespace f{

enum class int_align { none, left, right, center };
enum class int_plus_sign {none, plus, space};
enum class int_prefix {none, lcase, ucase};

enum fmt_kinds : unsigned {
    fmt_none = 0,
    fmt_align = 1,
    fmt_width = 2,
    fmt_fill = 4,
    fmt_sign = 8,
    fmt_upper = 16,
    fmt_prefix = 32,
    fmt_zero = 64,
};

template<typename A, typename B, unsigned Kind>
struct fmt_info {
    using a_t = A;
    using b_t = B;
    inline static constexpr unsigned kind = Kind;
};

struct fmt_default {
    inline static constexpr unsigned kind = fmt_none;
};

template<int_align A>
struct align_info{
    inline static constexpr unsigned kind = fmt_align;
};

template<size_t Width = 0>
struct width_info {
    inline static constexpr unsigned kind = fmt_width;
};

template<unsigned Fill = ' '>
struct fill_info {
    inline static constexpr unsigned kind = fmt_fill;
};

template<int_plus_sign S>
struct sign_info {
    inline static constexpr unsigned kind = fmt_sign;
};

struct upper_info {
    inline static constexpr unsigned kind = fmt_upper;
};

template<int_prefix>
struct prefix_info {
    inline static constexpr unsigned kind = fmt_prefix;
};

struct zero_info {
    inline static constexpr unsigned kind = fmt_zero;
};

template<typename T>
concept FmtParam = requires {
    {std::remove_cvref_t<T>::kind} -> std::same_as<const unsigned&>;
};

template<FmtParam A, FmtParam B>
requires((A::kind & B::kind) == 0)
constexpr auto operator | (const A&, const B&) {
    return fmt_info<A, B, A::kind | B::kind>{};
}

inline constexpr align_info<int_align::left> l;
inline constexpr align_info<int_align::right>  r;
inline constexpr align_info<int_align::center> c;

template<size_t W>
inline constexpr width_info<W> w;

template<unsigned F>
inline constexpr fill_info<F> f;

inline constexpr sign_info<int_plus_sign::plus> sp;
inline constexpr sign_info<int_plus_sign::space> ss;
inline constexpr upper_info u;
inline constexpr prefix_info<int_prefix::lcase> p;
inline constexpr prefix_info<int_prefix::ucase> P;
inline constexpr zero_info z;

inline constexpr width_info<unsigned(-1)> wp;
inline constexpr fmt_default df;

template<typename T>
struct extract_align : std::false_type {
    inline static constexpr int_align align = int_align::none;
};

template<int_align A>
struct extract_align<align_info<A>> : std::true_type {
    inline static constexpr int_align align = A;
};

template<typename A, typename B, unsigned U>
struct extract_align<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_align<A>::value, in_b = extract_align<B>::value, value = in_a | in_b;
    inline static constexpr int_align align = extract_align<std::conditional_t<in_a, A, std::conditional_t<in_b, B, align_info<int_align::none>>>>::align;
};

template<typename T>
struct extract_width : std::false_type {
    inline static constexpr size_t width = 0;
};

template<size_t W>
struct extract_width<width_info<W>> : std::true_type {
    inline static constexpr size_t width = W;
};

template<typename A, typename B, unsigned U>
struct extract_width<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_width<A>::value, in_b = extract_width<B>::value, value = in_a | in_b;
    inline static constexpr size_t width = extract_width<std::conditional_t<in_a, A, std::conditional_t<in_b, B, width_info<0>>>>::width;
};

template<typename T>
struct extract_fill : std::false_type {
    inline static constexpr unsigned fill = ' ';
};

template<unsigned F>
struct extract_fill<fill_info<F>> : std::true_type {
    inline static constexpr unsigned fill = F;
};

template<typename A, typename B, unsigned U>
struct extract_fill<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_fill<A>::value, in_b = extract_fill<B>::value, value = in_a | in_b;
    inline static constexpr unsigned fill = extract_fill<std::conditional_t<in_a, A, std::conditional_t<in_b, B, fill_info<' '>>>>::fill;
};

template<typename T>
struct extract_sign : std::false_type {
    inline static constexpr int_plus_sign sign = int_plus_sign::none;
};

template<int_plus_sign S>
struct extract_sign<sign_info<S>> : std::true_type {
    inline static constexpr int_plus_sign sign = S;
};

template<typename A, typename B, unsigned U>
struct extract_sign<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_sign<A>::value, in_b = extract_sign<B>::value, value = in_a | in_b;
    inline static constexpr int_plus_sign sign = extract_sign<std::conditional_t<in_a, A, std::conditional_t<in_b, B, sign_info<int_plus_sign::none>>>>::sign;
};

template<typename T>
struct extract_upper : std::false_type {};

template<>
struct extract_upper<upper_info> : std::true_type {};

template<typename A, typename B, unsigned U>
struct extract_upper<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_upper<A>::value, in_b = extract_upper<B>::value,
        value = extract_upper<std::conditional_t<in_a, A, std::conditional_t<in_b, B, std::false_type>>>::value;
};

template<typename T>
struct extract_prefix : std::false_type{
    inline static constexpr int_prefix prefix = int_prefix::none;
};

template<int_prefix P>
struct extract_prefix<prefix_info<P>> : std::true_type {
    inline static constexpr int_prefix prefix = P;
};

template<typename A, typename B, unsigned U>
struct extract_prefix<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_prefix<A>::value, in_b = extract_prefix<B>::value, value = in_a | in_b;
    inline static constexpr int_prefix prefix = extract_prefix<std::conditional_t<in_a, A, std::conditional_t<in_b, B, prefix_info<int_prefix::none>>>>::prefix;
};

template<typename T>
struct extract_zero : std::false_type{};

template<>
struct extract_zero<zero_info> : std::true_type {};

template<typename A, typename B, unsigned U>
struct extract_zero<fmt_info<A, B, U>> {
    inline static constexpr bool in_a = extract_zero<A>::value, in_b = extract_zero<B>::value,
        value = extract_zero<std::conditional_t<in_a, A, std::conditional_t<in_b, B, std::false_type>>>::value;
};

template<int_align Align, unsigned Width, unsigned Fill, int_plus_sign Sign, bool Upper, int_prefix Prefix, bool Zero>
struct fmt_params {
    inline static constexpr int_align align = Align;
    inline static constexpr unsigned width = Width;
    inline static constexpr unsigned fill = Fill;
    inline static constexpr int_plus_sign sign = Sign;
    inline static constexpr bool upper = Upper;
    inline static constexpr int_prefix prefix = Prefix;
    inline static constexpr bool zero = Zero;
};

template<FmtParam T>
using p_to_fmt_t = fmt_params<
    extract_align<T>::align,
    extract_width<T>::width,
    extract_fill<T>::fill,
    extract_sign<T>::sign,
    extract_upper<T>::value,
    extract_prefix<T>::prefix,
    extract_zero<T>::value>;

template<FmtParam T>
using to_fmt_t = p_to_fmt_t<std::remove_cvref_t<T>>;

template<typename T>
struct is_fmt_params : std::false_type{};

template<int_align Align, unsigned Width, unsigned Fill, int_plus_sign Sign, bool Upper, int_prefix Prefix, bool Zero>
struct is_fmt_params<fmt_params<Align, Width, Fill, Sign, Upper, Prefix, Zero>> : std::true_type{};

template<typename T>
concept FmtParamSet = is_fmt_params<T>::value;

struct fmt_end{};

template<f::FmtParam F>
constexpr auto operator|(const F& f, fmt_end) {
    return f;
}

template<char C = 0, char...Chars>
constexpr auto parse_fmt_symbol();

template<unsigned N, char C = 'Z', char...Chars>
constexpr auto parse_width() {
    if constexpr (C >= '0' && C <= '9') {
        return parse_width<N * 10 + C - '0', Chars...>();
    } else {
        return w<N> | parse_fmt_symbol<C, Chars...>();
    }
}

template<unsigned N, char C = 'Z', char...Chars>
constexpr auto parse_fill() {
    if constexpr (C >= '0' && C <= '9') {
        return parse_fill<N * 16 + C - '0', Chars...>();
    } else if constexpr (C >= 'a' && C <= 'f') {
        return parse_fill<N * 16 + C - 'a' + 10, Chars...>();
    } else if constexpr (C >= 'A' && C <= 'F') {
        return parse_fill<N * 16 + C - 'A' + 10, Chars...>();
    } else {
        return f<N> | parse_fmt_symbol<C, Chars...>();
    }
}

template<char C, char...Chars>
constexpr auto parse_fmt_symbol() {
    if constexpr (C == '0') {
        return z | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'a') {
        return p | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'A') {
        return P | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'b') {
        return l | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'c') {
        return c | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'd') {
        return r | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'e') {
        return sp | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'f') {
        return ss | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'E') {
        return u | parse_fmt_symbol<Chars...>();
    } else if constexpr (C == '\'') {
        return parse_fmt_symbol<Chars...>();
    } else if constexpr (C == 'F') {
        return parse_fill<0, Chars...>();
    } else if constexpr (C >= '1' && C <= '9') {
        return parse_width<C - '0', Chars...>();
    } else {
        return fmt_end{};
    }
}

template<unsigned R, typename T>
struct fmt_radix_info {};

template<unsigned N, char C = 0, char...Chars>
constexpr auto parse_radix() {
    if constexpr (C >='0' && C <= '9') {
        return parse_radix<N * 10 + C - '0', Chars...>();
    } else if constexpr (C == 0) {
        return fmt_radix_info<N, to_fmt_t<fmt_default>>{};
    } else {
        return fmt_radix_info<N, decltype(parse_fmt_symbol<C, Chars...>())>{};
    }
};

template<char C1, char C2, char C3, char...Chars>
constexpr auto skip_0x() {
    static_assert(C1 == '0' && C2 == 'x' && "Fmt symbols must begin with 0x");
    static_assert(C3 >= '1' && C3 <= '9' && "Radix must begin with 1-9");
    return parse_radix<C3 - '0', Chars...>();
}

} // namespace f

template<typename K, bool Ucase>
inline constexpr K digits_symbols[36] = {K('0'), K('1'), K('2'), K('3'), K('4'), K('5'), K('6'), K('7'), K('8'), K('9'),
    K(Ucase ? 'A' : 'a'), K(Ucase ? 'B' : 'b'), K(Ucase ? 'C' : 'c'), K(Ucase ? 'D' : 'd'), K(Ucase ? 'E' : 'e'),
    K(Ucase ? 'F' : 'f'), K(Ucase ? 'G' : 'g'), K(Ucase ? 'H' : 'h'), K(Ucase ? 'I' : 'i'), K(Ucase ? 'J' : 'j'),
    K(Ucase ? 'K' : 'k'), K(Ucase ? 'L' : 'l'), K(Ucase ? 'M' : 'm'), K(Ucase ? 'N' : 'n'), K(Ucase ? 'O' : 'o'),
    K(Ucase ? 'P' : 'p'), K(Ucase ? 'Q' : 'q'), K(Ucase ? 'R' : 'r'), K(Ucase ? 'S' : 's'), K(Ucase ? 'T' : 't'),
    K(Ucase ? 'U' : 'u'), K(Ucase ? 'V' : 'v'), K(Ucase ? 'W' : 'w'), K(Ucase ? 'X' : 'x'), K(Ucase ? 'Y' : 'y'),
    K(Ucase ? 'Z' : 'z'),
};

template<FromIntNumber T, unsigned Radix, f::FmtParamSet FP> requires (Radix > 1 && Radix <= 36)
struct expr_integer_src {
    T value_;
    unsigned width_{};
    constexpr expr_integer_src(T v) : value_(v){}
    constexpr expr_integer_src(T v, unsigned w) : value_(v), width_(w){}

    template<is_std_string_v S>
    operator S() const;

    template<typename S> requires std::is_constructible_v<S, empty_expr<typename S::symb_type>>
    operator S() const;
};

template<is_one_of_char_v K, FromIntNumber T, unsigned Radix, f::FmtParamSet FP>
requires (Radix > 1 && Radix <= 36)
struct expr_integer : expr_to_std_string<expr_integer<K, T, Radix, FP>> {
    using symb_type = K;
    using my_type = expr_num<K, T>;

    enum { bufSize = 64 };
    mutable K buf[bufSize];
    mutable T value_;
    unsigned width_{};

    mutable bool negate_{};

    constexpr expr_integer(T t) : value_(t){}
    constexpr expr_integer(T t, unsigned w) : value_(t), width_(w){}
    constexpr expr_integer(const expr_integer_src<T, Radix, FP>& v) : value_(v.value_), width_(v.width_){}

    constexpr expr_integer(expr_integer&& t) noexcept : value_(t.value_), width_(t.width_){}

    constexpr size_t length() const noexcept {
        K* bufEnd = std::end(buf), *itr = bufEnd;

        if (value_) {
            need_sign<K, std::is_signed_v<T>, T> store(value_);
            while (store.val) {
                *--itr = digits_symbols<K, FP::upper>[store.val % Radix];
                store.val /= Radix;
            }
            if constexpr (std::is_signed_v<T>) {
                negate_ = store.negate;
            }
        } else {
            *--itr = digits_symbols<K, FP::upper>[0];
        }
        size_t len = bufEnd - itr;
        value_ = T(len);
        if constexpr (std::is_signed_v<T>) {
            if constexpr (FP::sign != f::int_plus_sign::none) {
                len++;
            } else {
                if (negate_) {
                    len++;
                }
            }
        }
        if constexpr (FP::prefix != f::int_prefix::none && (Radix == 2 || Radix == 8 || Radix == 16)) {
            len++;  // add 0
            if constexpr (Radix != 8) { // for octo just add 0,
                len++; // for other b or x
            }
        }
        if constexpr (FP::width == unsigned(-1)) {
            return std::max<size_t>(width_, len);
        } else {
            return std::max<size_t>(FP::width, len);
        }
    }
    constexpr K* place(K* ptr) const noexcept {
        size_t len = (size_t)value_, all_len = len;
        if constexpr (std::is_signed_v<T>) {
            if constexpr (FP::sign != f::int_plus_sign::none) {
                all_len++;
            } else {
                if (negate_) {
                    all_len++;
                }
            }
        }
        if constexpr (FP::prefix != f::int_prefix::none && (Radix == 2 || Radix == 8 || Radix == 16)) {
            all_len++;  // add 0
            if constexpr (Radix != 8) { // for octo just add 0,
                all_len++; // for other b or x
            }
        }
        if constexpr (FP::zero) {
            if constexpr (std::is_signed_v<T>) {
                if (negate_) {
                    *ptr++ = K('-');
                } else {
                    if constexpr (FP::sign == f::int_plus_sign::plus) {
                        *ptr++ = K('+');
                    } else if constexpr (FP::sign == f::int_plus_sign::space) {
                        *ptr++ = K(' ');
                    }
                }
            }
            if constexpr (FP::prefix != f::int_prefix::none && (Radix == 2 || Radix == 8 || Radix == 16)) {
                *ptr++ = K('0');
                if constexpr (Radix == 2) {
                    *ptr++ = FP::prefix == f::int_prefix::lcase ? K('b') : K('B');
                } else if constexpr (Radix == 16) {
                    *ptr++ = FP::prefix == f::int_prefix::lcase ? K('x')  : K('X');
                }
            }
            size_t before = 0;
            if constexpr (FP::width == unsigned(-1)) {
                if (width_ > all_len) {
                    before = width_ - all_len;
                }
            } else {
                if (FP::width > all_len) {
                    before = FP::width - all_len;
                }
            }
            if (before) {
                ch_traits<K>::assign(ptr, before, K('0'));
                ptr += before;
            }
            ch_traits<K>::copy(ptr, std::end(buf) - len, len);
            ptr += len;
        } else {
            size_t before = 0, after = 0;
            if constexpr (FP::width == unsigned(-1)) {
                if (width_ > all_len) {
                    if constexpr (FP::align == f::int_align::left) {
                        after = width_ - all_len;
                    } else if constexpr (FP::align == f::int_align::center) {
                        before = (width_ - all_len) / 2;
                        after = width_ - all_len - before;
                    } else {
                        before = width_ - all_len;
                    }
                }
            } else {
                if (FP::width > all_len) {
                    if constexpr (FP::align == f::int_align::left) {
                        after = FP::width - all_len;
                    } else if constexpr (FP::align == f::int_align::center) {
                        before = (FP::width - all_len) / 2;
                        after = FP::width - all_len - before;
                    } else {
                        before = FP::width - all_len;
                    }
                }
            }
            if (before) {
                ch_traits<K>::assign(ptr, before, K(FP::fill));
                ptr += before;
            }
            if constexpr (std::is_signed_v<T>) {
                if (negate_) {
                    *ptr++ = K('-');
                } else {
                    if constexpr (FP::sign == f::int_plus_sign::plus) {
                        *ptr++ = K('+');
                    } else if constexpr (FP::sign == f::int_plus_sign::space) {
                        *ptr++ = K(' ');
                    }
                }
            }
            if constexpr (FP::prefix != f::int_prefix::none && (Radix == 2 || Radix == 8 || Radix == 16)) {
                *ptr++ = K('0');
                if constexpr (Radix == 2) {
                    *ptr++ = FP::prefix == f::int_prefix::lcase ? K('b') : K('B');
                } else if constexpr (Radix == 16) {
                    *ptr++ = FP::prefix == f::int_prefix::lcase ? K('x') : K('X');
                }
            }
            ch_traits<K>::copy(ptr, std::end(buf) - len, len);
            ptr += len;

            if (after) {
                ch_traits<K>::assign(ptr, after, K(FP::fill));
                ptr += after;
            }
        }
        return ptr;
    }
};

template<FromIntNumber T, unsigned Radix, f::FmtParamSet FP> requires (Radix > 1 && Radix <= 36)
template<is_std_string_v S>
expr_integer_src<T, Radix, FP>::operator S() const {
    using st = typename S::value_type;
    using Al = typename S::allocator_type;
    return to_std_string<st, Al>(expr_integer<st, T, Radix, FP>{value_, width_});
}

template<FromIntNumber T, unsigned Radix, f::FmtParamSet FP> requires (Radix > 1 && Radix <= 36)
template<typename S> requires std::is_constructible_v<S, empty_expr<typename S::symb_type>>
expr_integer_src<T, Radix, FP>::operator S() const {
    using st = typename S::symb_type;
    return S{expr_integer<st, T, Radix, FP>{value_, width_}};
}

template<typename K, FromIntNumber T, unsigned Radix, f::FmtParamSet FP>
struct convert_to_strexpr<K, expr_integer_src<T, Radix, FP>> {
    using type = expr_integer<K, T, Radix, FP>;
};

template<unsigned R, typename T, typename F>
struct flags_checker {
    using val_t = T;
    using flags_t = f::to_fmt_t<F>;
    inline static constexpr unsigned Radix = R;
};

template<typename T, bool ArgWidth>
concept good_int_flags =
    T::Radix > 1 && T::Radix <= 36
    // Должно быть задано только или выравнивание, или заполнение нулём.
    // Only either alignment or zero-padding must be specified.
    && (T::flags_t::align == f::int_align::none || !T::flags_t::zero)
    // Флаг вывода знака должен задаваться только для знаковых чисел
    // The sign output flag should only be set for signed numbers.
    && (std::is_signed_v<typename T::val_t> || T::flags_t::sign == f::int_plus_sign::none)
    // Неверное поле ширины
    // Invalid width field
    && (T::flags_t::width == unsigned(-1)) == ArgWidth
    // Префикс может быть только при основании 2, 8, 16
    // Prefix may by only for radix 2, 8, 16
    && (T::flags_t::prefix == f::int_prefix::none || T::Radix == 2 || T::Radix == 8 || T::Radix == 16);

/*!
 * @ingroup StrExprs
 * @ru @brief Создает объект, который преобразовывается в строковое выражение, генерирующее строковое представление числа.
 * @tparam R - Основание счисления, должно быть от 2 до 36.
 * @tparam fp - параметры для форматирования числа.
 * @tparam T - тип числа, выводится из аргумента.
 * @param v - число.
 * @return источник для строкового выражения expr_integer_src.
 * @details параметры форматирования числа задаются набором констант, через '|'.
 * - f::l при задании ширины поля выравнивать влево.
 * - f::r при задании ширины поля выравнивать вправо.
 * - f::с при задании ширины поля выравнивать по центру.
 * - f::p выводить префикс системы счисления, 0b для 2, 0 для 8, 0x для 16.
 * - f::P выводить префикс системы счисления, 0B для 2, 0 для 8, 0X для 16.
 * - f::z дополнять число до заданной ширины нулями слева. Не совместимо с опциями выравнивания.
 * - f::u Для систем счисления более 10, выводить символы в верхнем регистре.
 * - f::sp Для знаковых типов чисел для положительных значений выводить '+' перед числом.
 * - f::ss Для знаковых типов чисел для положительных значений выводить пробел перед числом.
 * - f::f<'c'>` Задаёт символ-заполнитель при указании ширины поля. По умолчанию - пробел.
 * - f::w<N> Задаёт ширину поля. При указании ширины можно либо указать желаемое выравнивание (r, l, c),
 *   либо дополнение нулями (z), но не оба сразу. Если ничего из этого не указано, применяется выравнивание вправо.
 *   Число дополняется до заданной ширины, если оно короче. Если длиннее, то выводится всё число.
 * - f::wp То же самое, что и f::w<-1>. В этом случае ширина должна передаваться дополнительным аргументом e_int.
 * <br/>
 * @en @brief Creates an object that is converted to a string expression that generates a string representation of a number.
 * @tparam R - Number base, must be from 2 to 36.
 * @tparam fp - parameters for format number.
 * @tparam T - number type, deduced from the argument.
 * @param v - number.
 * @return the source for the string expression expr_integer_src.
 * @details number formatting parameters are specified by a set of constants, via '|'.
 * - f::l when setting the field width, align to the left.
 * - f::r when setting the field width, align to the right.
 * - f::с when setting the field width, align to the center.
 * - f::p print number system prefix, 0b for 2, 0 for 8, 0x for 16.
 * - f::P print number system prefix, 0B for 2, 0 for 8, 0X for 16.
 * - f::z pad the number to the specified width with zeros on the let. Not compatible with alignment options.
 * - f::u For radix greater than 10, output characters in uppercase.
 * - f::sp For signed number types, print '+' before the number for positive values.
 * - f::ss For signed number types, print a space before the number for positive values.
 * - f::f<'c'> Specifies a placeholder character when specifying the field width. Default is space.
 * - f::w<N> Sets the field width. When specifying the width, you can either specify the desired alignment (r, l, c),
 *   or zero padding (z), but not both. If none of these are specified, right alignment is applied.
 *   The number is padded to the given width if it is shorter. If it is longer, then the entire number is displayed.
 * - f::wp Same as f::w<-1>. In this case, the width must be passed as an additional argument e_int.
 * <br/>
 * @ru Пример: @en Example: @~
 * ```cpp
 *  stringu u16t = u"Number "_ss + num
 *      + " in octal is " + e_int<8, f::w<16> | f::z>(num)
 *      + ", in binary is " + e_int<2, f::w<32> | f::p | f::z>(num);
 * ```
 * @ru Возможна также сокращённая запись этой функции в виде `num / 0xПараметрыФорматирования_fmt`.
 * Параметры форматирования задаются следующим образом: сначала идёт `0x`, затем основание счисления, записанное в
 * десятичном виде. Далее могут идти символы, обозначающие различные флаги:
 * - b при задании ширины поля выравнивать влево, аналог f::l.
 * - d при задании ширины поля выравнивать вправо, аналог f::r.
 * - с при задании ширины поля выравнивать по центру, аналог f::c.
 * - a выводить префикс системы счисления, 0b для 2, 0 для 8, 0x для 16, аналог f::p.
 * - A выводить префикс системы счисления, 0B для 2, 0 для 8, 0X для 16, аналог f::P.
 * - 0 дополнять число до заданной ширины нулями слева. Не совместимо с опциями выравнивания, аналог f::z.
 * - E Для систем счисления более 10, выводить символы в верхнем регистре, аналог f::u.
 * - e Для знаковых типов чисел для положительных значений выводить '+' перед числом, аналог f::sp.
 * - f Для знаковых типов чисел для положительных значений выводить пробел перед числом, аналог f::ss.
 * - FКодCимволаВhex Задаёт код символа-заполнителя при указании ширины поля, аналог f::f<'c'>.
 * - Число в десятичном виде, начинающееся не с 0. Задаёт ширину поля, аналог f::w<N>.
 *   При указании ширины можно либо указать желаемое выравнивание (b, c, d),
 *   либо дополнение нулями (0), но не оба сразу. Если ничего из этого не указано, применяется выравнивание вправо.
 *   Число дополняется до заданной ширины, если оно короче. Если длиннее, то выводится всё число.
 * - ' разделитель, пропускается.
 *
 * Так как после `F` все символы воспринимаются как hex код символа разделителя, при необходимости его использования
 * лучше ставить его в конце литерала форматирования, или отделять '.
 * <br/>
 * @en It is also possible to write this function in a shortened form: `num / 0xFormatOptions_fmt`.
 * Formatting parameters are set as follows: first comes `0x`, then the radix written in
 * decimal form. Next may be symbols indicating various flags:
 * - b when setting the field width, align to the left, analogous to f::l.
 * - d when setting the field width, align to the right, analogous to f::r.
 * - c when setting the field width, align to the center, analogous to f::c.
 * - a display the number system prefix, 0b for 2, 0 for 8, 0x for 16, analogous to f::p.
 * - A display the number system prefix, 0B for 2, 0 for 8, 0X for 16, analogous to f::P.
 * - 0 pad the number to the specified width with zeros on the left. Not compatible with alignment options, similar to f::z.
 * - E For radix greater than 10, display characters in uppercase, analogous to f::u.
 * - e For signed number types, for positive values, print '+' before the number, analogous to f::sp.
 * - f For signed number types, for positive values, print a space before the number, analogous to f::ss.
 * - FCodeInhex Sets the code of the filler character when specifying the field width, analogous to f::f<'c'>.
 * - Number in decimal form, not starting from 0. Sets the field width, analogous to f::w<N>.
 *   When specifying the width, you can either specify the desired alignment (b, c, d),
 *   or zero padding (0), but not both. If none of these are specified, right alignment is applied.
 *   The number is padded to the given width if it is shorter. If it is longer, then the entire number is displayed.
 * - ' delimiter, skipped.
 *
 * Since after `F` all characters are perceived as hex code of the delimiter character, if necessary, use it
 * it is better to put it at the end of format literal, or separate with '.
 * <br/>
 * @ru Пример: @en Example: @~
 * ```cpp
 *  stringu u16t = u"Number "_ss + num
 *      + " in octal is " + num / 0x8'016_fmt      // same as e_int<8, f::w<16> | f::z>(num)
 *      + ", in binary is " + num / 0x2a032_fmt;   // same as e_int<2, f::w<32> | f::p | f::z>(num);
 *   ....
 *  stringa text = "Count is "_ss + count / 0x16A08E_fmt; // same as e_int<16, f::P | f::z | f::w<8> | f::u>(count)
 *   ....
 *  stringa text = "Number "_ss + count / 0x16c20EF5F_fmt; // same as e_int<16, f::c | f::w<20> | f::u | f::f<'_'>>(count)
 * ```
 */
template<unsigned R, auto fp = f::df, FromIntNumber T> requires good_int_flags<flags_checker<R, T,  decltype(fp)>, false>
constexpr auto e_int(T v) {
    using fmt_param = f::to_fmt_t<decltype(fp)>;
    return expr_integer_src<T, R, fmt_param>{v};
}
/*!
 * @ru @brief То же самое, что `e_int(num)`, только дополнительно передаётся ширина поля.
 * @en @brief Same as `e_int(num)`, only the field width is additionally specified.
 */
template<unsigned R, auto fp = f::df, FromIntNumber T> requires good_int_flags<flags_checker<R, T,  decltype(fp)>, true>
constexpr auto e_int(T v, unsigned w) {
    using fmt_param = f::to_fmt_t<decltype(fp)>;
    return expr_integer_src<T, R, fmt_param>{v, w};
}

template<typename T, unsigned R, typename F> requires good_int_flags<flags_checker<R, T, F>, false>
auto operator / (T v, const f::fmt_radix_info<R, F>&) {
    return expr_integer_src<T, R, f::to_fmt_t<F>>{v};
}

template<typename K>
struct expr_real : expr_to_std_string<expr_real<K>> {
    using symb_type = K;
    mutable u8s buf[40];
    mutable size_t l;
    double v;
    constexpr expr_real(double d) : v(d) {}
    constexpr expr_real(float d) : v(d) {}

    size_t length() const noexcept {
        auto [ptr, ec] = std::to_chars(buf, buf + std::size(buf), v);
        l = ec != std::errc{} ? 0 : ptr - buf;
        return l;
    }
    K* place(K* ptr) const noexcept {
        if constexpr (sizeof(K) == sizeof(buf[0])) {
            ch_traits<K>::copy(ptr, (K*)buf, l);
        } else {
            for (size_t i = 0; i < l; i++) {
                ptr[i] = buf[i];
            }
        }
        return ptr + l;
    }
};

/*!
 * @ru @brief Специализация шаблона для преобразования целых чисел в строковое выражение, позволяет использовать их в
 * операциях конкатенации со строковыми выражениями.
 * @tparam K - тип символов строкового выражения.
 * @tparam T - тип числа.
 * @en @brief A template specialization for converting integers to string expressions, allowing their use in
 * concatenation operations with string expressions.
 * @tparam K - the character type of the string expression.
 * @tparam T - the number type.
 */
template<typename K, std::floating_point T>
struct convert_to_strexpr<K, T> {
    using type = expr_real<K>;
};

/*!
 * @ingroup StrExprs
 * @ru @brief Преобразование `double` числа в строковое выражение.
 * @param t - число.
 * @details Возвращает строковое выражение, которое генерирует десятичное представление заданного числа.
 * с помощью `sprintf("%.16g")`. Может использоваться, когда надо конкатенировть число и строковый литерал.
 * @en @brief Convert a `double` number to a string expression.
 * @param t - number.
 * @details Returns a string expression that generates the decimal representation of the given number.
 * using `sprintf("%.16g")`. Can be used when you need to concatenate a number and a string literal.
 */
template<typename K> requires is_one_of_char_v<K>
inline constexpr expr_real<K> e_num(double t) {
    return {t};
}

template<FromIntNumber Val, bool All, bool Ucase, bool Ox>
struct expr_hex_src {
    explicit constexpr expr_hex_src(Val v) : v_(v){}
    Val v_;
};

template<typename K, FromIntNumber Val, bool All, bool Ucase, bool Ox>
struct expr_hex : expr_to_std_string<expr_hex<K, Val, All, Ucase, Ox>> {
    using symb_type = K;
    mutable need_sign<K, std::is_signed_v<Val>, Val> v_;
    mutable K buf_[sizeof(Val) * 2]{};

    explicit constexpr expr_hex(Val v) : v_(v){}
    constexpr expr_hex(const expr_hex_src<Val, All, Ucase, Ox>& v) : v_(v.v_){}

    constexpr size_t length() const noexcept {
        K *ptr = buf_ + std::size(buf_);
        size_t l = 0;
        for (;;) {
            *--ptr = digits_symbols<K, Ucase>[v_.val & 0xF];
            v_.val >>= 4;
            l++;
            if (v_.val) {
                *--ptr = digits_symbols<K, Ucase>[v_.val & 0xF];
                v_.val >>= 4;
                l++;
            }
            if (!v_.val) {
                if constexpr (All) {
                    if (size_t need = sizeof(Val) * 2 - l) {
                        ch_traits<K>::assign(buf_, need, K('0'));
                    }
                    l = sizeof(Val) * 2;
                }
                break;
            }
        }
        v_.val = l;
        if constexpr (std::is_signed_v<Val>) {
            return l + (Ox ? 2 : 0) + (v_.negate ? 1 : 0);
        }
        return l + (Ox ? 2 : 0);
    }
    constexpr K* place(K* ptr) const noexcept {
        if constexpr (std::is_signed_v<Val>) {
            if (v_.negate) {
                *ptr++ = K('-');
            }
        }
        if constexpr (Ox) {
            *ptr++ = K('0');
            *ptr++ = K('x');
        }
        if constexpr (All) {
            ch_traits<K>::copy(ptr, buf_, sizeof(Val) * 2);
            return ptr + sizeof(Val) * 2;
        } else {
            ch_traits<K>::copy(ptr, buf_ + std::size(buf_) - v_.val, v_.val);
            return ptr + v_.val;
        }
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Флаги для функции e_hex.
 * @en @brief Flags for the e_hex function.
 */
enum HexFlags : unsigned {
    Short = 1,  ///< without leading zeroes
    No0x = 2,   //< without 0x prefix
    Lcase = 4,  //< Use lower case
};

/*!
 * @ingroup StrExprs
 * @ru @brief Создает объект, который может преобразовываться в строковое выражение, генерирующее 16ричное представление числа.
 * @tparam Flags - флаги форматирования, побитовое ИЛИ из HexFlags.
 * @tparam T - тип числа, выводится автоматически.
 * @param v - число.
 * @details Это более быстрое преобразование в строку в 16ричном виде, чем через `e_int`, однако с меньшими возможностями
 *  форматирования. По умолчанию создается представление в виде `0x000012AB`, то есть префикс `0x` и дополнения нулями до ширины
 *  по размеру типа числа (2 символа на байт). Можно указать флаги:
 * - HexFlags::Short - не дополнять нулями до фиксированной ширины.
 * - HexFlags::Lcase - выводить символы в нижнем регистре.
 * - HexFlags::No0x - не выводить префикс.
 *
 * Кроме того, возможна краткая запись этого выражения в виде `num / N_f16`
 * N обозначает флаги форматирования
 * - 1 - HexFlags::Short
 * - 2 - HexFlags::Lcase
 * - 3 - HexFlags::No0x
 *
 * @en @brief Creates an object that can be converted to a string expression that generates a hexadecimal representation of a number.
 * @tparam Flags - format flags, bitwise OR of HexFlags.
 * @tparam T - number type, deducted automatically.
 * @param v - number.
 * @details This is a faster conversion to a hexadecimal string than `e_int`, but with less power formatting.
 * By default, a representation of the form `0x000012AB` is created, that is, prefixed with `0x` and padded with zeros to the width
 * by number type size (2 characters per byte). You can specify flags:
 * - HexFlags::Short - do not pad with zeros to a fixed width.
 * - HexFlags::Lcase - display characters in lowercase.
 * - HexFlags::No0x - do not display the prefix.
 *
 * In addition, this expression can be written briefly as `num / N_f16`
 * N stands for formatting flags
 * - 1 - HexFlags::Short
 * - 2 - HexFlags::Lcase
 * - 3 - HexFlags::No0x
 *
 * @ru Пример @en Example @~
 * ```cpp
 *    stringa text = +"val = "sv + e_hex(10);
 *    EXPECT_EQ(text, "val = 0x0000000A");
 *
 *    stringu textu = +u"val = 0X"sv + e_hex<HexFlags::No0x | HexFlags::Short | HexFlags::Lcase>(0x12A);
 *    EXPECT_EQ(textu, u"val = 0X12a");
 *
 *    text = "Num in hex: " + num / 0_f16;      // same as e_hex(num);
 *    text = "Num in hex: " + num / 13_f16;     // same as e_hex<HexFlags::Short | HexFlags::No0x>(num);
 *    text = "Num in hex: " + num / 123_f16;    // same as e_hex<HexFlags::Short | HexFlags::No0x | HexFlags::Lcase>(num);
 *    text = "Num in hex: " + num / 2_f16;      // same as e_hex<HexFlags::No0x | HexFlags::Lcase>(num);
 * ```
 */
template<unsigned Flags = 0, FromIntNumber T>
constexpr auto e_hex(T v) {
    return expr_hex_src<T, (Flags & HexFlags::Short) == 0, (Flags & HexFlags::Lcase) == 0, (Flags & HexFlags::No0x) == 0>{v};
}

template<char c = 0, char...Chars>
constexpr unsigned parse_f16_flags() {
    if constexpr (c == '1') {
        return HexFlags::Short | parse_f16_flags<Chars...>();
    } else if constexpr (c == '2') {
        return HexFlags::Lcase | parse_f16_flags<Chars...>();
    } else if constexpr (c == '3') {
        return HexFlags::No0x | parse_f16_flags<Chars...>();
    } else {
        return 0;
    }
}

template<unsigned N>
struct f16flags{};

template<FromIntNumber T, unsigned Flags>
constexpr auto operator/(T v, const f16flags<Flags>&) {
    return expr_hex_src<T, (Flags & HexFlags::Short) == 0, (Flags & HexFlags::Lcase) == 0, (Flags & HexFlags::No0x) == 0>{v};
}

inline namespace literals {
template<char...Chars>
constexpr auto operator""_f16() {
    return f16flags<parse_f16_flags<Chars...>()>{};
}
} // namespace literals

/*!
 * @ru @brief Специализация шаблона для преобразования e_hex в строковое выражение, позволяет использовать их в
 * операциях конкатенации со строковыми выражениями.
 * @tparam K - тип символов строкового выражения.
 * @en @brief A template specialization for converting e_hex to string expressions, allowing their use in
 * concatenation operations with string expressions.
 * @tparam K - the character type of the string expression.
 */
template<typename K, typename Val, bool All, bool Ucase, bool Ox>
struct convert_to_strexpr<K, expr_hex_src<Val, All, Ucase, Ox>> {
    using type = expr_hex<K, Val, All, Ucase, Ox>;
};

template<typename K>
struct expr_pointer : expr_hex<K, uintptr_t, true, true, true> {
    constexpr expr_pointer(const void* ptr) : expr_hex<K, uintptr_t, true, true, true>((uintptr_t)ptr){}
};

/*!
 * @ru @brief Специализация шаблона для преобразования указателей в строковое выражение, позволяет использовать их в
 * операциях конкатенации со строковыми выражениями.
 * @tparam K - тип символов строкового выражения.
 * @en @brief A template specialization for converting pointers to string expressions, allowing their use in
 * concatenation operations with string expressions.
 * @tparam K - the character type of the string expression.
 */
template<typename K, typename T>
struct convert_to_strexpr<K, const T*> {
    using type = expr_pointer<K>;
};

template<typename K, StrExprForType<K> A, bool Left>
struct expr_fill : expr_to_std_string<expr_fill<K, A, Left>>{
    using symb_type = K;
    K symbol_;
    size_t width_;
    const A& a_;
    mutable size_t alen_{};
    constexpr expr_fill(K symbol, size_t width, const A& a) : symbol_(symbol), width_(width), a_(a){}

    constexpr size_t length() const noexcept {
        alen_ = a_.length();
        return std::max(alen_, width_);
    }
    constexpr K* place(K* ptr) const noexcept {
        if (alen_ >= width_) {
            return (K*)a_.place((typename A::symb_type*)ptr);
        }
        size_t w = width_ - alen_;
        if constexpr (Left) {
            ch_traits<K>::assign(ptr, w, symbol_);
            ptr += w;
            return (K*)a_.place((typename A::symb_type*)ptr);
        } else {
            ptr = (K*)a_.place((typename A::symb_type*)ptr);
            ch_traits<K>::assign(ptr, w, symbol_);
            return ptr + w;
        }
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Создает выражение, которое дополняет указанное строковое выражение до заданной длины
 *  заданным символом слева.
 * @param width - дополнять до этой ширины
 * @param symbol - символ для заполнения
 * @details Если строковое выражение выдаёт строку короче заданной длины, добавляет перед ней
 *  указанный символ, дополняя до нужной длины. Не обрезает строку до указанной длины.
 *  Будьте внимательны, длина берётся в code units, не в code points, а символ заполнения
 *  не может быть суррогатным, то есть занимать более одного code unit. Если надо быть точным
 *  с Unicode-символами - используйте конвертацию в char32_t и обратно.
 * @en @brief Creates an expression that expands the specified string expression to the specified length
 * given character to the left.
 * @param width - pad to this width
 * @param symbol - symbol to fill
 * @details If a string expression produces a string shorter than the given length, prepend it with
 * the specified character, padding to the desired length. Does not truncate the string to the specified length.
 * Be careful, the length is taken in code units, not in code points, and the padding character
 * cannot be a surrogate, that is, occupy more than one code unit. If you have to be precise
 * with Unicode characters - use conversion to char32_t and vice versa.
 */
template<StrExpr A, typename K = typename A::symb_type>
expr_fill<K, A, true> e_fill_left(const A& a, size_t width, K symbol = K(' ')) {
    return {symbol, width, a};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Создает выражение, которое дополняет указанное строковое выражение до заданной длины
 *  заданным символом справа.
 * @param width - дополнять до этой ширины
 * @param symbol - символ для заполнения
 * @details Если строковое выражение выдаёт строку короче заданной длины, добавляет к ней
 *  указанный символ, дополняя до нужной длины. Не обрезает строку до указанной длины.
 *  Будьте внимательны, длина берётся в code units, не в code points, а символ заполнения
 *  не может быть суррогатным, то есть занимать более одного code unit. Если надо быть точным
 *  с Unicode-символами - используйте конвертацию в char32_t и обратно.
 * @en @brief Creates an expression that expands the specified string expression to the specified length
 * given character to the right.
 * @param width - pad to this width
 * @param symbol - symbol to fill
 * @details If a string expression produces a string shorter than the given length, appends it
 * the specified character, padding to the desired length. Does not truncate the string to the specified length.
 * Be careful, the length is taken in code units, not in code points, and the padding character
 * cannot be a surrogate, that is, occupy more than one code unit. If you have to be precise
 * with Unicode characters - use conversion to char32_t and vice versa.
 */
template<StrExpr A, typename K = typename A::symb_type>
expr_fill<K, A, false> e_fill_right(const A& a, size_t width, K symbol = K(' ')) {
    return {symbol, width, a};
}

/*
* Для создания строковых конкатенаций с векторами и списками, сджойненными константным разделителем
* K - тип символов строки
* T - тип контейнера строк (vector, list)
* I - длина разделителя в символах
* tail - добавлять разделитель после последнего элемента контейнера.
*        Если контейнер пустой, разделитель в любом случае не добавляется
* skip_empty - пропускать пустые строки без добавления разделителя
* To create string concatenations with vectors and lists joined by a constant delimiter
* K is the symbols
* T - type of string container (vector, list)
* I - length of separator in characters
* tail - add a separator after the last element of the container.
* If the container is empty, the separator is not added anyway
* skip_empty - skip empty lines without adding a separator
*/
template<typename K, typename T, size_t I, bool tail, bool skip_empty>
struct expr_join : expr_to_std_string<expr_join<K, T, I, tail, skip_empty>> {
    using symb_type = K;
    using my_type = expr_join<K, T, I, tail, skip_empty>;

    const T& s;
    const K* delim;
    constexpr expr_join(const T& _s, const K* _delim) : s(_s), delim(_delim){}

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
            ch_traits<K>::copy(write, t.data(), copyLen);
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
 * @ingroup StrExprs
 * @ru @brief Получить строковое выражение, конкатенирующее строки в контейнере в одну строку с заданным разделителем.
 * @tparam tail - добавлять ли разделитель после последней строки.
 * @tparam skip_empty - пропускать пустые строки без добавления разделителя.
 * @param s - контейнер со строками, должен поддерживать `range for`.
 * @param d - разделитель, строковый литерал.
 * @en @brief Get a string expression concatenating the strings in the container into a single string with the given delimiter.limiter.limiter.
 * @tparam tail - whether to add a separator after the last line.
 * @tparam skip_empty - skip empty lines without adding a separator.
 * @param s - container with strings, must support `range for`.
 * @param d - delimiter, string literal.
 */
template<bool tail = false, bool skip_empty = false, typename L, typename K = typename const_lit<L>::symb_type, size_t I = const_lit<L>::Count, typename T>
inline constexpr auto e_join(const T& s, L&& d) {
    return expr_join<K, T, I - 1, tail, skip_empty>{s, d};
}

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

/*!
 * @ru @brief Перечисление с возможными результатами преобразования строки в целое число
 * @en @brief Enumeration with possible results of converting a string to an integer
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

template<typename T>
struct convert_result {
    T value;
    IntConvertResult ec;
    size_t read;
};

struct int_convert { // NOLINT
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
    static constexpr std::make_unsigned_t<K> toDigit(K s) {
        auto us = static_cast<std::make_unsigned_t<K>>(s);
        if constexpr (Base <= 10) {
            return us - '0';
        } else {
            if constexpr (sizeof(K) == 1) {
                return NUMBERS[us];
            } else {
                return us < 256 ? NUMBERS[us] : us;
            }
        }
    }

    template<typename K, ToIntNumber T, unsigned Base, bool CheckOverflow>
        requires(Base != 0)
    static constexpr convert_result<T> parse(const K* start, const K* current, const K* end, bool negate) {
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
                const u_type digit = toDigit<K, Base>(*current);
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
                const u_type digit = toDigit<K, Base>(*current);
                if (digit >= Base) {
                    break;
                }
                number = number * Base + digit;
                ++current;
            }
            if (!maxDigits) {
                // Прошли все цифры, дальше надо с проверкой на overflow
                // All numbers have passed, then we need to check for overflow
                for (;;) {
                    const u_type digit = toDigit<K, Base>(*current);
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
        return {result, error, size_t(current - start)};
    }
public:
    // Если Base = 0 - то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10
    // Если Base = -1 - то пытается определить основание по префиксу 0[xX] как 16, 0[bB] как 2, 0[oO] или 0 как 8, иначе 10
    // If Base = 0, then it tries to determine the base by the prefix 0[xX] as 16, 0 as 8, otherwise 10
    // If Base = -1 - then tries to determine the base by the prefix 0[xX] as 16, 0[bB] as 2, 0[oO] or 0 as 8, otherwise 10
    template<typename K, ToIntNumber T, unsigned Base = 0, bool CheckOverflow = true, bool SkipWs = true, bool AllowSign = true>
        requires(Base == -1 || (Base < 37 && Base != 1))
    static constexpr convert_result<T> to_integer(const K* start, size_t len) noexcept {
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
                    // Can be a number, +number or -number
                    if (*ptr == '+') {
                        ptr++;
                    } else if (*ptr == '-') {
                        negate = true;
                        ptr++;
                    }
                } else {
                    // Может быть число или -число
                    // Can be a number or -number
                    if (*ptr == '-') {
                        negate = true;
                        ptr++;
                    }
                }
            } else if constexpr (AllowSign) {
                // Может быть число или +число
                // Can be a number or +number
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
                    return {0, IntConvertResult::Success, size_t(ptr - start)};
                }
                return parse<K, T, 10, CheckOverflow>(start, ptr, end, negate);
            } else
                return parse<K, T, Base, CheckOverflow>(start, ptr, end, negate);
        }
        return {0, IntConvertResult::NotNumber, size_t(ptr - start)};
    }
};

template<typename K, typename Impl>
class null_terminated {
public:
    /*!
     * @ru @brief Получить указатель на константный буфер символов строки
     * @return const K* - указатель на константный буфер символов строки
     * @en @brief Get a pointer to a constant character buffer of a string
     * @return const K* - pointer to a constant string character buffer
     */
    constexpr const K* c_str() const { return static_cast<const Impl*>(this)->symbols(); }
};

template<typename K, typename Impl, bool Mutable> class buffer_pointers;

/*!
 * @ru @brief Базовый класс для строкового буфера.
 * @tparam K - тип символов.
 * @tparam Impl - класс реализации.
 * @en @brief Base class for a string buffer.
 * @tparam K - character type.
 * @tparam Impl - implementation class.
 */
template<typename K, typename Impl>
class buffer_pointers<K, Impl, false> {
    constexpr const Impl& d() const { return *static_cast<const Impl*>(this); }
public:
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* data() const  { return d().symbols(); }
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* begin() const { return d().symbols(); }
    /*!
     * @ru @brief  Указатель на константный символ после после последнего символа строки.
     * @return const K* - конец строки.
     * @en @brief  Pointer to a constant character after the last character of the string.
     * @return const K* - end of line.
     */
    constexpr const K* end() const   { return d().symbols() + d().length(); }
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* cbegin() const { return d().symbols(); }
    /*!
     * @ru @brief  Указатель на константный символ после после последнего символа строки.
     * @return const K* - конец строки.
     * @en @brief  Pointer to a constant character after the last character of the string.
     * @return const K* - end of line.
     */
    constexpr const K* cend() const   { return d().symbols() + d().length(); }
};

template<typename K, typename Impl>
class buffer_pointers<K, Impl, true> : public buffer_pointers<K, Impl, false> {
    constexpr Impl& d() { return *static_cast<Impl*>(this); }
    using base = buffer_pointers<K, Impl, false>;
public:
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* data() const  { return base::data(); }
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* begin() const { return base::begin(); }
    /*!
     * @ru @brief  Указатель на константный символ после после последнего символа строки.
     * @return const K* - конец строки.
     * @en @brief  Pointer to a constant character after the last character of the string.
     * @return const K* - end of line.
     */
    constexpr const K* end() const   { return base::end(); }
    /*!
     * @ru @brief  Получить указатель на константный буфер символов строки.
     * @return const K* - указатель на константный буфер символов строки.
     * @en @brief  Get a pointer to a constant character buffer of a string.
     * @return const K* - pointer to a constant buffer of string characters.
     */
    constexpr const K* cbegin() const { return base::cbegin(); }
    /*!
     * @ru @brief  Указатель на константный символ после после последнего символа строки.
     * @return const K* - конец строки.
     * @en @brief  Pointer to a constant character after the last character of the string.
     * @return const K* - end of line.
     */
    constexpr const K* cend() const   { return base::cend(); }
    /*!
     * @ru @brief  Получить указатель на буфер символов строки.
     * @return K* - указатель на буфер символов строки.
     * @en @brief  Get a pointer to the string's character buffer.
     * @return K* - pointer to a string character buffer.
     */
    constexpr K* data()  { return d().str(); }
    /*!
     * @ru @brief  Получить указатель на буфер символов строки.
     * @return K* - указатель на буфер символов строки.
     * @en @brief  Get a pointer to the string's character buffer.
     * @return K* - pointer to a string character buffer.
     */
    constexpr K* begin() { return d().str(); }
    /*!
     * @ru @brief  Указатель на символ после после последнего символа строки.
     * @return K* - конец строки.
     * @en @brief  Pointer to the character after the last character of the string.
     * @return K* - end of line.
     */
    constexpr K* end()   { return d().str() + d().length(); }
};

/*!
 * @ru @brief Класс для последовательного получения подстрок по заданному разделителю.
 * @tparam K - тип символов.
 * @en @brief Class for sequentially obtaining substrings by a given delimiter.
 * @tparam K - character type.
 */
template<typename K, typename StrSrc>
class SplitterBase {
    using str_t = StrSrc;
    str_t text_;
    str_t delim_;

public:
    constexpr SplitterBase(str_t text, str_t delim) : text_(text), delim_(delim) {}
    /*!
     * @ru @brief Узнать, не закончились ли подстроки.
     * @en @brief Find out if substrings are running out.
     */
    constexpr bool is_done() const {
        return text_.length() == str::npos;
    }
    /*!
     * @ru @brief Получить следующую подстроку.
     * @return simple_str.
     * @en @brief Get the next substring.
     * @return simple_str.
     */
    constexpr str_t next() {
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
        str_t result{text_.str, pos};
        text_.str += next;
        text_.len -= next;
        return result;
    }
};

/*!
 * @ru @brief Класс с базовыми константными строковыми алгоритмами.
 * @details Является базой для классов, могущих выполнять константные операции со строками.
 * Ничего не знает о хранении строк, ни сам, ни у класса наследника, то есть работает
 * только с указателем на строку и её длиной.
 * Для работы класс-наследник должен реализовать методы:
 *   - size_t length() const noexcept     - возвращает длину строки.
 *   - const K* symbols() const noexcept  - возвращает указатель на начало строки.
 *   - bool is_empty() const noexcept     - проверка, не пустая ли строка.
 * @tparam K       - тип символов.
 * @tparam StrRef  - тип хранилища куска строки.
 * @tparam Impl    - конечный класс наследник.
 * @en @brief A class with basic constant string algorithms.
 * @details Is the base for classes that can perform constant operations on strings.
 * Doesn’t know anything about storing strings, neither itself nor the descendant class, that is, it works
 * only with a pointer to a string and its length.
 * To work, the descendant class must implement the following methods:
 *   - size_t length() const noexcept - returns the length of the string.
 *   - const K* symbols() const noexcept - returns a pointer to the beginning of the line.
 *   - bool is_empty() const noexcept - checks whether the string is empty.
 * @tparam K - character type.
 * @tparam StrRef - storage type for the string chunk.
 * @tparam Impl - the final class is the successor.
 */
template<typename K, typename StrRef, typename Impl, bool Mutable>
class str_src_algs : public buffer_pointers<K, Impl, Mutable> {
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
    using uns_type = std::make_unsigned_t<K>;
    using my_type = Impl;
    using base = str_src_algs<K, StrRef, Impl, Mutable>;
    str_src_algs() = default;

    /*!
     * @ru @brief Копировать строку в указанный буфер.
     * @details Метод предполагает, что размер выделенного буфера достаточен для всей строки, т.е.
     * предварительно была запрошена `length()`. Не добавляет `\0`.
     * @param ptr - указатель на буфер.
     * @return указатель на символ после конца размещённой в буфере строки.
     * @en @brief Copy the string to the specified buffer.
     * @details The method assumes that the size of the allocated buffer is sufficient for the entire line, i.e.
     * `length()` was previously requested. Does not add `\0`.
     * @param ptr - pointer to the buffer.
     * @return pointer to the character after the end of the symbols placed in the buffer.
     */
    constexpr K* place(K* ptr) const noexcept {
        size_t myLen = _len();
        traits::copy(ptr, _str(), myLen);
        return ptr + myLen;
    }
    /*!
     * @ru @brief Копировать строку в указанный буфер.
     * @details Метод добавляет `\0` после скопированных символов. Не выходит за границы буфера.
     * @param buffer - указатель на буфер
     * @param bufSize - размер буфера в символах.
     * @en @brief Copy the string to the specified buffer.
     * @details The method adds `\0` after the copied characters. Does not exceed buffer boundaries.
     * @param buffer - pointer to buffer
     * @param bufSize - buffer size in characters.
     */
    void copy_to(K* buffer, size_t bufSize) {
        size_t tlen = std::min(_len(), bufSize - 1);
        traits::copy(buffer, _str(), tlen);
        buffer[tlen] = 0;
    }
    /*!
     * @ru @brief Размер строки в символах.
     * @return size_t
     * @en @brief The size of the string in characters.
     * @return size_t
     */
    constexpr size_t size() const {
        return _len();
    }

    /*!
     * @ru @brief Конвертировать в std::basic_string_view.
     * @return std::basic_string_view<K>.
     * @en @brief Convert to std::basic_string_view.
     * @return std::basic_string_view<K>.
     */
    template<typename D = K> requires is_equal_str_type_v<K, D>
    constexpr std::basic_string_view<D> to_sv() const noexcept {
        return {(const D*)_str(), _len()};
    }
    /*!
     * @ru @brief Конвертировать в std::basic_string_view.
     * @return std::basic_string_view<K>.
     * @en @brief Convert to std::basic_string_view.
     * @return std::basic_string_view<K>.
     */
    template<typename D, typename Traits> requires is_equal_str_type_v<K, D>
    constexpr operator std::basic_string_view<D, Traits>() const {
        return {(const D*)_str(), _len()};
    }
    /*!
     * @ru @brief Конвертировать в std::basic_string.
     * @return std::basic_string<K>.
     * @en @brief Convert to std::basic_string.
     * @return std::basic_string<K>.
     */
    template<typename D = K, typename Traits = std::char_traits<D>, typename Allocator = std::allocator<D>> requires is_equal_str_type_v<K, D>
    constexpr std::basic_string<D, Traits, Allocator> to_string() const {
        return {(const D*)_str(), _len()};
    }
    /*!
     * @ru @brief Конвертировать в std::basic_string.
     * @return std::basic_string<K>.
     * @en @brief Convert to std::basic_string.
     * @return std::basic_string<K>.
     */
    template<typename D, typename Traits, typename Allocator> requires is_equal_str_type_v<K, D>
    constexpr operator std::basic_string<D, Traits, Allocator>() const {
        return {(const D*)_str(), _len()};
    }
    /*!
     * @ru @brief Преобразовать себя в "кусок строки", включающий всю строку.
     * @return str_piece.
     * @en @brief Convert itself to a "string chunk" that includes the entire string.
     * @return str_piece.
     */
    constexpr operator str_piece() const noexcept {
        return str_piece{_str(), _len()};
    }
    /*!
     * @ru @brief Преобразовать себя в "кусок строки", включающий всю строку.
     * @return str_piece.
     * @en @brief Convert itself to a "string chunk" that includes the entire string.
     * @return str_piece.
     */
    constexpr str_piece to_str() const noexcept {
        return {_str(), _len()};
    }
    /*!
     * @ru @brief Получить часть строки как "str_src".
     * @param from - количество символов от начала строки.
     * @param len - количество символов в получаемом "куске".
     * @return Подстроку, str_src.
     * @details  Если `from` меньше нуля, то отсчитывается `-from` символов от конца строки в сторону начала.
     *           Если `len` меньше или равно нулю, то отсчитать `-len` символов от конца строки
     * @en @brief Get part of a string as "str_src".
     * @param from - number of characters from the beginning of the line.
     * @param len - the number of characters in the resulting "chunk".
     * @return Substring, str_src.
     * @details If `from` is less than zero, then `-from` characters are counted from the end of the line towards the beginning.
     * If `len` is less than or equal to zero, then count `-len` characters from the end of the line
     * @~
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
     * @ru @brief Получить часть строки как "кусок строки".
     * @param from - количество символов от начала строки. При превышении размера строки вернёт пустую строку.
     * @param len - количество символов в получаемом "куске". При выходе за пределы строки вернёт всё до конца строки.
     * @return Подстроку, str_src.
     * @en @brief Get part of a string as "string chunk".
     * @param from - number of characters from the beginning of the line. If the string size is exceeded, it will return an empty string.
     * @param len - the number of characters in the resulting "chunk". When going beyond the line, it will return everything up to the end of the line.
     * @return Substring, str_src.
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
     * @ru @brief Получить подстроку str_src с позиции от from до позиции to (не включая её).
     * @details Для производительности метод никак не проверяет выходы за границы строки, используйте
     *          в сценариях, когда точно знаете, что это позиции внутри строки и to >= from.
     * @param from - начальная позиция.
     * @param to - конечная позиция (не входит в результат).
     * @return Подстроку, str_src.
     * @en @brief Get the substring str_src from position from to position to (not including it).
     * @details For performance reasons, the method does not check for line boundaries in any way, use
     * in scenarios when you know for sure that these are positions inside the line and to >= from.
     * @param from - starting position.
     * @param to - final position (not included in the result).
     * @return Substring, str_src.
     */
    constexpr str_piece from_to(size_t from, size_t to) const noexcept {
        return str_piece{_str() + from, to - from};
    }
    /*!
     * @ru @brief Получить подстроку str_src от начала и до первого найденного вхождения указанной подстроки.
     * @details Если не найдено, вернёт всю строку.
     * @param offset - позиция для начала поиска.
     * @return Подстроку, str_src.
     * @en @brief Get the substring str_src from the beginning to the first found occurrence of the specified substring.
     * @details If not found, returns the entire string.
     * @param offset - position to start the search.
     * @return Substring, str_src.
     */
    constexpr str_piece until(str_piece pattern, size_t offset = 0) const noexcept {
        return (*this)(0, find_or_all(pattern, offset));
    }
    /*!
     * @ru @brief Проверка на пустоту.
     * @en @brief Check for emptiness.
     */
    constexpr bool operator!() const noexcept {
        return _is_empty();
    }
    /*!
     * @ru @brief Получить символ на заданной позиции .
     * @param idx - индекс символа. Для отрицательных значений отсчитывается от конца строки.
     * @return K - символ.
     * @details Не производит проверку на выход за границы строки.
     * @en @brief Get the character at the given position.
     * @param idx - symbol index. For negative values, it is counted from the end of the line.
     * @return K - character.
     * @details Does not check for line boundaries.
     */
    constexpr K at(ptrdiff_t idx) const {
        return _str()[idx >= 0 ? idx : _len() + idx];
    }
    // Сравнение строк
    // String comparison
    constexpr int compare(const K* text, size_t len) const {
        size_t myLen = _len();
        int cmp = traits::compare(_str(), text, std::min(myLen, len));
        return cmp == 0 ? (myLen > len ? 1 : myLen == len ? 0 : -1) : cmp;
    }
    /*!
     * @ru @brief Сравнение строк посимвольно.
     * @param o - другая строка.
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше.
     * @en @brief Compare strings character by character.
     * @param o - another line.
     * @return <0 this string is less, ==0 - strings are equal, >0 - this string is greater.
     */
    constexpr int compare(str_piece o) const {
        return compare(o.symbols(), o.length());
    }
    /*!
     * @ru @brief Сравнение с C-строкой посимвольно.
     * @param text - другая строка.
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше.
     * @en @brief Compare with C-string character by character.
     * @param text - another line.
     * @return <0 this string is less, ==0 - strings are equal, >0 - this string is greater.
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
     * @ru @brief Сравнение строк на равенство.
     * @param other - другая строка.
     * @return равны ли строки.
     * @en @brief String comparison for equality.
     * @param other - another line.
     * @return whether the strings are equal.
     */
    constexpr bool equal(str_piece other) const noexcept {
        return equal(other.symbols(), other.length());
    }
    /*!
     * @ru @brief Оператор сравнение строк на равенство.
     * @param other - другая строка.
     * @return равны ли строки.
     * @en @brief Operator comparing strings for equality.
     * @param other - another line.
     * @return whether the strings are equal.
     */
    constexpr bool operator==(const base& other) const noexcept {
        return equal(other._str(), other._len());
    }
    /*!
     * @ru @brief Оператор сравнения строк.
     * @param other - другая строка.
     * @en @brief String comparison operator.
     * @param other - another line.
     */
    constexpr auto operator<=>(const base& other) const noexcept {
        return compare(other._str(), other._len()) <=> 0;
    }
    /*!
     * @ru @brief Оператор сравнения строки и строкового литерала на равенство.
     * @param other - строковый литерал.
     * @en @brief Operator for comparing a string and a string literal for equality.
     * @param other - string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr bool operator==(T&& other) const noexcept {
        return N - 1 == _len() && traits::compare(_str(), (const K*)other, N - 1) == 0;
    }
    /*!
     * @ru @brief Оператор сравнения строки и строкового литерала.
     * @param other - строковый литерал.
     * @en @brief Comparison operator between a string and a string literal.
     * @param other is a string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr auto operator<=>(T&& other) const noexcept {
        size_t myLen = _len();
        int cmp = traits::compare(_str(), (const K*)other, std::min(myLen, N - 1));
        int res = cmp == 0 ? (myLen > N - 1 ? 1 : myLen == N - 1 ? 0 : -1) : cmp;
        return res <=> 0;
    }

    // Сравнение ascii строк без учёта регистра
    // Compare ascii strings without taking into account case
    constexpr int compare_ia(const K* text, size_t len) const noexcept { // NOLINT
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
     * @ru @brief Сравнение строк посимвольно без учёта регистра ASCII символов.
     * @param text - другая строка.
     * @return <0 эта строка меньше, ==0 - строки равны, >0 - эта строка больше.
     * @en @brief Compare strings character by character and not case sensitive ASCII characters.
     * @param text - another line.
     * @return <0 this string is less, ==0 - strings are equal, >0 - this string is greater.
     */
    constexpr int compare_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length());
    }

    /*!
     * @ru @brief Равна ли строка другой строке посимвольно без учёта регистра ASCII символов.
     * @param text - другая строка.
     * @return равны ли строки.
     * @en @brief Whether a string is equal to another string, character-by-character-insensitive, of ASCII characters.
     * @param text - another line.
     * @return whether the strings are equal.
     */
    constexpr bool equal_ia(str_piece text) const noexcept { // NOLINT
        return text.length() == _len() && compare_ia(text.symbols(), text.length()) == 0;
    }
    /*!
     * @ru @brief Меньше ли строка другой строки посимвольно без учёта регистра ASCII символов.
     * @param text - другая строка.
     * @return меньше ли строка.
     * @en @brief Whether a string is smaller than another string, character-by-character-insensitive, ASCII characters.
     * @param text - another line.
     * @return whether the string is smaller.
     */
    constexpr bool less_ia(str_piece text) const noexcept { // NOLINT
        return compare_ia(text.symbols(), text.length()) < 0;
    }

    constexpr size_t find(const K* pattern, size_t lenPattern, size_t offset) const noexcept {
        size_t lenText = _len();
        // Образец, не вмещающийся в строку и пустой образец не находим
        // We don't look for an empty line or a line longer than the text.
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
     * @ru @brief Найти начало первого вхождения подстроки в этой строке.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию начала вхождения подстроки, или -1, если не найдена.
     * @en @brief Find the beginning of the first occurrence of a substring in this string.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @return size_t - the position of the beginning of the occurrence of the substring, or -1 if not found.
     */
    constexpr size_t find(str_piece pattern, size_t offset = 0) const noexcept {
        return find(pattern.symbols(), pattern.length(), offset);
    }
    /*!
     * @ru @brief Найти начало первого вхождения подстроки в этой строке или выкинуть исключение.
     * @tparam Exc - тип исключения.
     * @tparam Args... - типы параметров для конструирования исключения, выводятся из аргументов.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @param args - аргументы для конструктора исключения.
     * @return size_t - позицию начала вхождения подстроки, или выбрасывает исключение Exc, если не найдена.
     * @en @brief Find the beginning of the first occurrence of a substring in this string or throw an exception.
     * @tparam Exc - exception type.
     * @tparam Args... - types of parameters for constructing an exception, inferred from the arguments.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @param args - arguments for the exception constructor.
     * @return size_t - the position of the beginning of the substring occurrence, or throws an Exc exception if not found.
     */
    template<typename Exc, typename ... Args> requires std::is_constructible_v<Exc, Args...>
    constexpr size_t find_or_throw(str_piece pattern, size_t offset = 0, Args&& ... args) const noexcept {
        if (auto fnd = find(pattern.symbols(), pattern.length(), offset); fnd != str::npos) {
            return fnd;
        }
        throw Exc(std::forward<Args>(args)...);
    }
    /*!
     * @ru @brief Найти конец вхождения подстроки в этой строке.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию сразу за вхождением подстроки, или -1, если не найдена.
     * @en @brief Find the end of the occurrence of a substring in this string.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @return size_t - the position immediately after the occurrence of the substring, or -1 if not found.
     */
    constexpr size_t find_end(str_piece pattern, size_t offset = 0) const noexcept {
        size_t fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? fnd : fnd + pattern.length();
    }
    /*!
     * @ru @brief Найти начало первого вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию начала вхождения подстроки, или длину строки, если не найдена.
     * @en @brief Find the beginning of the first occurrence of a substring in this string or the end of the string.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @return size_t - the position at which the substring begins, or the length of the string if not found.
     */
    constexpr size_t find_or_all(str_piece pattern, size_t offset = 0) const noexcept {
        auto fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd;
    }
    /*!
     * @ru @brief Найти конец первого вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию сразу за вхождением подстроки, или длину строки, если не найдена.
     * @en @brief Find the end of the first occurrence of a substring in this string, or the end of a string.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @return size_t - the position immediately after the occurrence of the substring, or the length of the string if not found.
     */
    constexpr size_t find_end_or_all(str_piece pattern, size_t offset = 0) const noexcept {
        auto fnd = find(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd + pattern.length();
    }

    constexpr size_t find_last(const K* pattern, size_t lenPattern, size_t offset) const noexcept {
        if (lenPattern == 1)
            return find_last(pattern[0], offset);
        size_t lenText = std::min(_len(), offset);
        // Образец, не вмещающийся в строку и пустой образец не находим
        // We don't look for an empty line or a line longer than the text.
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
     * @ru @brief Найти начало последнего вхождения подстроки в этой строке.
     * @param pattern - искомая строка.
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца.
     * @return size_t - позицию начала вхождения подстроки, или -1, если не найдена.
     * @en @brief Find the beginning of the last occurrence of a substring in this string.
     * @param pattern - the search string.
     * @param offset - from which position to search in the opposite direction, -1 - from the very end.
     * @return size_t - the position of the beginning of the occurrence of the substring, or -1 if not found.
     */
    constexpr size_t find_last(str_piece pattern, size_t offset = -1) const noexcept {
        return find_last(pattern.symbols(), pattern.length(), offset);
    }
    /*!
     * @ru @brief Найти конец последнего вхождения подстроки в этой строке.
     * @param pattern - искомая строка.
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца.
     * @return size_t - позицию сразу за последним вхождением подстроки, или -1, если не найдена.
     * @en @brief Find the end of the last occurrence of a substring in this string.
     * @param pattern - the search string.
     * @param offset - from which position to search in the opposite direction, -1 - from the very end.
     * @return size_t - the position immediately after the last occurrence of the substring, or -1 if not found.
     */
    constexpr size_t find_end_of_last(str_piece pattern, size_t offset = -1) const noexcept {
        size_t fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? fnd : fnd + pattern.length();
    }
    /*!
     * @ru @brief Найти начало последнего вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка.
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца.
     * @return size_t - позицию начала вхождения подстроки, или длину строки, если не найдена.
     * @en @brief Find the beginning of the last occurrence of a substring in this string or the end of the string.
     * @param pattern - the search string.
     * @param offset - from which position to search in the opposite direction, -1 - from the very end.
     * @return size_t - the position at which the substring begins, or the length of the string if not found.
     */
    constexpr size_t find_last_or_all(str_piece pattern, size_t offset = -1) const noexcept {
        auto fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd;
    }
    /*!
     * @ru @brief Найти конец последнего вхождения подстроки в этой строке или конец строки.
     * @param pattern - искомая строка.
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца.
     * @return size_t - позицию сразу за последним вхождением подстроки, или длину строки, если не найдена.
     * @en @brief Find the end of the last occurrence of a substring in this string, or the end of a string.
     * @param pattern - the search string.
     * @param offset - from which position to search in the opposite direction, -1 - from the very end.
     * @return size_t - the position immediately after the last occurrence of the substring, or the length of the string if not found.
     */
    constexpr size_t find_end_of_last_or_all(str_piece pattern, size_t offset = -1) const noexcept {
        size_t fnd = find_last(pattern.symbols(), pattern.length(), offset);
        return fnd == str::npos ? _len() : fnd + pattern.length();
    }
    /*!
     * @ru @brief Содержит ли строка указанную подстроку.
     * @param pattern - искомая строка.
     * @param offset - с какой позиции начинать поиск.
     * @return bool.
     * @en @brief Whether the string contains the specified substring.
     * @param pattern - the search string.
     * @param offset - from which position to start the search.
     * @return bool.
     */
    constexpr bool contains(str_piece pattern, size_t offset = 0) const noexcept {
        return find(pattern, offset) != str::npos;
    }
    /*!
     * @ru @brief Найти символ в этой строке.
     * @param s - искомый символ.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию найденного символа, или -1, если не найден.
     * @en @brief Find a character in this string.
     * @param s is an optional character.
     * @param offset - from which position to start the search.
     * @return size_t - position of the found character, or -1 if not found.
     */
    constexpr size_t find(K s, size_t offset = 0) const noexcept {
        size_t len = _len();
        if (offset < len) {
            const K *str = _str(), *fnd = traits::find(str + offset, len - offset, s);
            if (fnd)
                return static_cast<size_t>(fnd - str);
        }
        return str::npos;
    }
    /*!
     * @ru @brief Найти символ в этой строке или конец строки.
     * @param s - искомый символ.
     * @param offset - с какой позиции начинать поиск.
     * @return size_t - позицию найденного символа, или длину строки, если не найден.
     * @en @brief Find a character in this string or the end of a string.
     * @param s is an optional character.
     * @param offset - from which position to start the search.
     * @return size_t - position of the found character, or string length if not found.
     */
    constexpr size_t find_or_all(K s, size_t offset = 0) const noexcept {
        size_t len = _len();
        if (offset < len) {
            const K *str = _str(), *fnd = traits::find(str + offset, len - offset, s);
            if (fnd)
                return static_cast<size_t>(fnd - str);
        }
        return len;
    }

    template<typename Op>
    constexpr void for_all_finded(const Op& op, const K* pattern, size_t patternLen, size_t offset, size_t maxCount) const {
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
     * @ru @brief Вызвать функтор для всех найденных вхождений подстроки в этой строке.
     * @param op - функтор, принимающий строку.
     * @param pattern - искомая подстрока.
     * @param offset - позиция начала поиска.
     * @param maxCount - максимальное количество обрабатываемых вхождений, 0 - без ограничений.
     * @en @brief Call a functor on all found occurrences of a substring in this string.
     * @param op is a functor that takes a string.
     * @param pattern - the substring to search for.
     * @param offset - search start position.
     * @param maxCount - the maximum number of occurrences to be processed, 0 - no restrictions.
     */
    template<typename Op>
    constexpr void for_all_finded(const Op& op, str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        for_all_finded(op, pattern.symbols(), pattern.length(), offset, maxCount);
    }

    template<typename To = std::vector<size_t>>
    constexpr To find_all(const K* pattern, size_t patternLen, size_t offset, size_t maxCount) const {
        To result;
        for_all_finded([&](auto f) { result.emplace_back(f); }, pattern, patternLen, offset, maxCount);
        return result;
    }
    /*!
     * @ru @brief Найти все вхождения подстроки в этой строке.
     * @param pattern - искомая подстрока.
     * @param offset - позиция начала поиска.
     * @param maxCount - максимальное количество обрабатываемых вхождений, 0 - без ограничений.
     * @return std::vector<size_t> - вектор с позициями начал найденных вхождений.
     * @en @brief Find all occurrences of a substring in this string.
     * @param pattern - the substring to search for.
     * @param offset - search start position.
     * @param maxCount - the maximum number of occurrences to be processed, 0 - no restrictions.
     * @return std::vector<size_t> - a vector with the positions of the beginnings of the found occurrences.
     */
    template<typename To = std::vector<size_t>>
    constexpr To find_all(str_piece pattern, size_t offset = 0, size_t maxCount = 0) const {
        return find_all(pattern.symbols(), pattern.length(), offset, maxCount);
    }
    template<typename To = std::vector<size_t>>
    constexpr void find_all_to(To& to, const K* pattern, size_t len, size_t offset = 0, size_t maxCount = 0) const {
        return for_all_finded([&](size_t pos) {
            to.emplace_back(pos);
        }, pattern, len, offset, maxCount);
    }
    /*!
     * @ru @brief Найти последнее вхождения символа в этой строке.
     * @param s - искомый символ.
     * @param offset - c какой позиции вести поиск в обратную сторону, -1 - с самого конца.
     * @return size_t - позицию найденного символа, или -1, если не найден.
     * @en @brief Find the last occurrence of a character in this string.
     * @param s is an optional character.
     * @param offset - from which position to search in the opposite direction, -1 - from the very end.
     * @return size_t - position of the found character, or -1 if not found.
     */
    constexpr size_t find_last(K s, size_t offset = -1) const noexcept {
        size_t len = std::min(_len(), offset);
        const K *text = _str();
        while (len > 0) {
            if (text[--len] == s)
                return len;
        }
        return str::npos;
    }
    /*!
     * @ru @brief Найти первое вхождение символа из заданного набора символов.
     * @param pattern - строка, задающая набор искомых символов.
     * @param offset - позиция начала поиска.
     * @return size_t - позицию найденного вхождения, или -1, если не найден.
     * @en @brief Find the first occurrence of a character from a given character set.
     * @param pattern - a string specifying the set of characters to search for.
     * @param offset - search start position.
     * @return size_t - position of the found occurrence, or -1 if not found.
     */
    constexpr size_t find_first_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::basic_string_view<K>{_str(), _len()}.find_first_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
    }
    /*!
     * @ru @brief Найти первое вхождение символа из заданного набора символов.
     * @param pattern - строка, задающая набор искомых символов.
     * @param offset - позиция начала поиска.
     * @return std::pair<size_t, size_t> - пару из позиции найденного вхождения и номера найденного символа в наборе, или -1, если не найден.
     * @en @brief Find the first occurrence of a character from a given character set.
     * @param pattern - a string specifying the set of characters to search for.
     * @param offset - search start position.
     * @return std::pair<size_t, size_t> - a pair from the position of the found occurrence and the number of the found character in the set, or -1 if not found.
     */
    constexpr std::pair<size_t, size_t> find_first_of_idx(str_piece pattern, size_t offset = 0) const noexcept {
        const K* text = _str();
        size_t fnd = std::basic_string_view<K>{text, _len()}.find_first_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::basic_string<K>::npos ? fnd : pattern.find(text[fnd]) };
    }
    /*!
     * @ru @brief Найти первое вхождение символа не из заданного набора символов.
     * @param pattern - строка, задающая набор символов.
     * @param offset - позиция начала поиска.
     * @return size_t - позицию найденного вхождения, или -1, если не найден.
     * @en @brief Find the first occurrence of a character not from the given character set.
     * @param pattern - a string specifying the character set.
     * @param offset - search start position.
     * @return size_t - position of the found occurrence, or -1 if not found.
     */
    constexpr size_t find_first_not_of(str_piece pattern, size_t offset = 0) const noexcept {
        return std::basic_string_view<K>{_str(), _len()}.find_first_not_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
    }
    /*!
     * @ru @brief Найти последнее вхождение символа из заданного набора символов.
     * @param pattern - строка, задающая набор искомых символов.
     * @param offset - позиция начала поиска.
     * @return size_t - позицию найденного вхождения, или -1, если не найден.
     * @en @brief Find the last occurrence of a character from a given character set.
     * @param pattern - a string specifying the set of characters to search for.
     * @param offset - search start position.
     * @return size_t - position of the found occurrence, or -1 if not found.
     */
    constexpr size_t find_last_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::basic_string_view<K>{_str(), _len()}.find_last_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
    }
    /*!
     * @ru @brief Найти последнее вхождение символа из заданного набора символов.
     * @param pattern - строка, задающая набор искомых символов.
     * @param offset - позиция начала поиска.
     * @return std::pair<size_t, size_t> - пару из позиции найденного вхождения и номера найденного символа в наборе, или -1, если не найден.
     * @en @brief Find the last occurrence of a character from a given character set.
     * @param pattern - a string specifying the set of characters to search for.
     * @param offset - search start position.
     * @return std::pair<size_t, size_t> - a pair from the position of the found occurrence and the number of the found character in the set, or -1 if not found.
     */
    constexpr std::pair<size_t, size_t> find_last_of_idx(str_piece pattern, size_t offset = str::npos) const noexcept {
        const K* text = _str();
        size_t fnd = std::basic_string_view<K>{text, _len()}.find_last_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
        return {fnd, fnd == std::basic_string<K>::npos ? fnd : pattern.find(text[fnd]) };
    }
    /*!
     * @ru @brief Найти последнее вхождение символа не из заданного набора символов.
     * @param pattern - строка, задающая набор символов.
     * @param offset - позиция начала поиска.
     * @return size_t - позицию найденного вхождения, или -1, если не найден.
     * @en @brief Find the last occurrence of a character not from the given character set.
     * @param pattern - a string specifying the character set.
     * @param offset - search start position.
     * @return size_t - position of the found occurrence, or -1 if not found.
     */
    constexpr size_t find_last_not_of(str_piece pattern, size_t offset = str::npos) const noexcept {
        return std::basic_string_view<K>{_str(), _len()}.find_last_not_of(std::basic_string_view<K>{pattern.str, pattern.len}, offset);
    }
    /*!
     * @ru @brief Получить подстроку. Работает аналогично operator(), только результат выдает того же типа, к которому применён метод.
     * @param from - количество символов от начала строки. Если меньше нуля, отсчитывается от конца строки в сторону начала.
     * @param len - количество символов в получаемом "куске". Если меньше или равно нулю, то отсчитать len символов от конца строки.
     * @return my_type - подстроку, объект того же типа, к которому применён метод.
     * @en @brief Get a substring. Works similarly to operator(), only the result is the same type as the method applied to.
     * @param from - number of characters from the beginning of the line. If less than zero, it is counted from the end of the line towards the beginning.
     * @param len - the number of characters in the resulting "chunk". If less than or equal to zero, then count len ​​characters from the end of the line.
     * @return my_type - a substring, an object of the same type to which the method is applied.
     */
    constexpr my_type substr(ptrdiff_t from, ptrdiff_t len = 0) const { // индексация в code units | indexing in code units
        return my_type{d()(from, len)};
    }
    /*!
     * @ru @brief Получить часть строки объектом того же типа, к которому применён метод, аналогично mid.
     * @param from - количество символов от начала строки. При превышении размера строки вернёт пустую строку.
     * @param len - количество символов в получаемом "куске". При выходе за пределы строки вернёт всё до конца строки.
     * @return Строку того же типа, к которому применён метод.
     * @en @brief Get part of a string with an object of the same type to which the method is applied, similar to mid.
     * @param from - number of characters from the beginning of the line. If the string size is exceeded, it will return an empty string.
     * @param len - the number of characters in the resulting "chunk". When going beyond the line, it will return everything up to the end of the line.
     * @return A string of the same type to which the method is applied.
     */
    constexpr my_type str_mid(size_t from, size_t len = -1) const { // индексация в code units | indexing in code units
        return my_type{d().mid(from, len)};
    }
    /*!
     * @ru @brief Преобразовать строку в число заданного типа.
     * @tparam T - желаемый тип числа.
     * @tparam CheckOverflow - проверять на переполнение.
     * @tparam Base - основание счисления числа, от -1 до 36, кроме 1.
     *         - Если 0: то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10.
     *         - Если -1: то пытается определить основание по префиксам:
     *            - 0 или 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *            - в остальных случаях 10.
     * @tparam SkipWs - пропускать пробельные символы в начале строки.
     * @tparam AllowSign - допустим ли знак '+' перед числом.
     * @return T - число, результат преобразования, насколько оно получилось, или 0 при переполнении.
     * @en @brief Convert a string to a number of the given type.
     * @tparam T - the desired number type.
     * @tparam CheckOverflow - check for overflow.
     * @tparam Base - the base of the number, from -1 to 36, except 1.
     *         - If 0: then tries to determine the base by the prefix 0[xX] as 16, 0 as 8, otherwise 10.
     *         - If -1: then tries to determine the base by prefixes:
     *            - 0 or 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *         - in other cases 10.
     * @tparam SkipWs - skip whitespace characters at the beginning of the line.
     * @tparam AllowSign - whether the '+' sign is allowed before a number.
     * @return T - a number, the result of the transformation, how much it turned out, or 0 if it overflows.
     */
    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true, bool AllowSign = true>
    constexpr T as_int() const noexcept {
        auto [res, err, _] = int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs, AllowSign>(_str(), _len());
        return err == IntConvertResult::Overflow ? 0 : res;
    }
    /*!
     * @ru @brief Преобразовать строку в число заданного типа.
     * @tparam T - желаемый тип числа.
     * @tparam CheckOverflow - проверять на переполнение.
     * @tparam Base - основание счисления числа, от -1 до 36, кроме 1.
     *         - Если 0: то пытается определить основание по префиксу 0[xX] как 16, 0 как 8, иначе 10
     *         - Если -1: то пытается определить основание по префиксам:
     *            - 0 или 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *            - в остальных случаях 10.
     * @tparam SkipWs - пропускать пробельные символы в начале строки. Пропускаются все символы с ASCII кодами <= 32.
     * @tparam AllowSign - допустим ли знак '+' перед числом.
     * @return convert_result<T> - кортеж из полученного числа, успешности преобразования и количестве обработанных символов.
     * @en @brief Convert a string to a number of the given type.
     * @tparam T - the desired number type.
     * @tparam CheckOverflow - check for overflow.
     * @tparam Base - the base of the number, from -1 to 36, except 1.
     *        - If 0: then tries to determine the base by the prefix 0[xX] as 16, 0 as 8, otherwise 10
     *        - If -1: then tries to determine the base by prefixes:
     *            - 0 or 0[oO]: 8
     *            - 0[bB]: 2
     *            - 0[xX]: 16
     *        - in other cases 10.
     * @tparam SkipWs - skip whitespace characters at the beginning of the line. All characters with ASCII codes <= 32 are skipped.
     * @tparam AllowSign - whether the '+' sign is allowed before a number.
     * @return convert_result<T> - a tuple of the received number, the success of the conversion and the number of characters processed.
     */
    template<ToIntNumber T, bool CheckOverflow = true, unsigned Base = 0, bool SkipWs = true, bool AllowSign = true>
    constexpr convert_result<T> to_int() const noexcept {
        return int_convert::to_integer<K, T, Base, CheckOverflow, SkipWs, AllowSign>(_str(), _len());
    }
    /*!
     * @ru @brief Преобразовать строку в double.
     * @return std::optional<double>.
     * @en @brief Convert string to double.
     * @return std::optional<double>.
     */
    template<bool SkipWS = true, bool AllowPlus = true> requires (sizeof(K) == 1)
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
        double d{};
        if (std::from_chars((const u8s*)ptr, (const u8s*)ptr + len, d).ec == std::errc{}) {
            return d;
        }
        return {};
    }
    /*!
     * @ru @brief Преобразовать строку в 16ричной записи в double. Пока работает только для char.
     * @return std::optional<double>.
     * @en @brief Convert string in hex form to double.
     * @return std::optional<double>.
     */
    template<bool SkipWS = true> requires (sizeof(K) == 1)
    std::optional<double> to_double_hex() const noexcept {
        size_t len = _len();
        const K* ptr = _str();
        if constexpr (SkipWS) {
            while (len && uns_type(*ptr) <= ' ') {
                len--;
                ptr++;
            }
        }
        if (len) {
            double d{};
            if (std::from_chars(ptr, ptr + len, d, std::chars_format::hex).ec == std::errc{}) {
                return d;
            }
        }
        return {};
    }
    /*!
     * @ru @brief Преобразовать строку в целое число.
     * @tparam T - тип числа, выводится из аргумента.
     * @param t - переменная, в которую записывается результат.
     * @en @brief Convert a string to an integer.
     * @tparam T - number type, inferred from the argument.
     * @param t - the variable into which the result is written.
     */
    template<ToIntNumber T>
    constexpr void as_number(T& t) const {
        t = as_int<T>();
    }

    template<typename T, typename Op>
    constexpr T splitf(const K* delimiter, size_t lendelimiter, const Op& beforeFunc, size_t offset) const {
        size_t mylen = _len();
        std::conditional_t<std::is_same_v<T, void>, char, T> results;
        str_piece me{_str(), mylen};
        for (int i = 0;; i++) {
            size_t beginOfDelim = find(delimiter, lendelimiter, offset);
            if (beginOfDelim == str::npos) {
                str_piece last{me.symbols() + offset, me.length() - offset};
                if constexpr (std::is_invocable_v<Op, str_piece&>) {
                    beforeFunc(last);
                }
                if constexpr (requires { results.emplace_back(last); }) {
                    if (last.is_same(me)) {
                        // Пробуем положить весь объект.
                        // Try to put the entire object.
                        results.emplace_back(d());
                    } else {
                        results.emplace_back(last);
                    }
                } else if constexpr (requires { results.push_back(last); }) {
                    if (last.is_same(me)) {
                        // Пробуем положить весь объект.
                        // Try to put the entire object.
                        results.push_back(d());
                    } else {
                        results.push_back(last);
                    }
                } else if constexpr (requires {results[i] = last;} && requires{std::size(results);}) {
                    if (i < std::size(results)) {
                        if (last.is_same(me)) {
                            // Пробуем положить весь объект.
                            // Try to put the entire object.
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
            offset = beginOfDelim + lendelimiter;
        }
        if constexpr (!std::is_same_v<T, void>) {
            return results;
        }
    }
    /*!
     * @ru @brief Разделить строку на части по заданному разделителю, с возможным применением функтора к каждой подстроке.
     * @tparam T - тип контейнера для складывания подстрок.
     * @param delimiter - подстрока разделитель.
     * @param beforeFunc - функтор для применения к найденным подстрокам, перед помещением их в результат.
     * @param offset - позиция начала поиска разделителя.
     * @return T - результат.
     * @details Для каждой найденной подстроки, если функтор может принять её, вызывается функтор, и подстрока
     *          присваивается результату функтора. Далее подстрока пытается добавиться в результат,
     *          вызывая один из его методов - `emplace_back`, `push_back`, `operator[]`. Если ни одного этого метода
     *          нет, ничего не делается, только вызов функтора.
     *          `operator[]` пытается применится, если у результата можно получить размер через `std::size` и
     *          мы не выходим за этот размер.
     *          При этом, если найденная подстрока получается совпадающей со всей строкой - в результат пытается
     *          поместить не подстроку, а весь объект строки, что позволяет, например, эффективно копировать sstring.
     * @en @brief Split a string into parts at a given delimiter, possibly applying a functor to each substring.
     * @tparam T - type of container for folding substrings.
     * @param delimiter - substring delimiter.
     * @param beforeFunc - a functor to apply to the found substrings, before placing them in the result.
     * @param offset - the position to start searching for the separator.
     * @return T - result.
     * @details For each substring found, if the functor can accept it, the functor is called, and the substring
     *          is assigned to the result of the functor. Next, the substring tries to be added to the result,
     *          calling one of its methods - `emplace_back`, `push_back`, `operator[]`. If none of this method
     *          no, nothing is done, just calling the functor.
     *          `operator[]` tries to apply if the result can have a size via `std::size` and
     *          we do not exceed this size.
     *          At the same time, if the found substring turns out to match the entire string, the result is attempted
     *          place not a substring, but the entire string object, which allows, for example, to effectively copy sstring.
     */
    template<typename T, typename Op>
    constexpr T splitf(str_piece delimiter, const Op& beforeFunc, size_t offset = 0) const {
        return splitf<T>(delimiter.symbols(), delimiter.length(), beforeFunc, offset);
    }
    /*!
     * @ru @brief Разделить строку на подстроки по заданному разделителю.
     * @tparam T - тип контейнера для результата.
     * @param delimiter - разделитель.
     * @param offset - позиция начала поиска разделителя.
     * @return T - контейнер с результатом.
     * @en @brief Split a string into substrings using a given delimiter.
     * @tparam T - container type for the result.
     * @param delimiter - delimiter.
     * @param offset - the position to start searching for the separator.
     * @return T - container with the result.
     */
    template<typename T>
    constexpr T split(str_piece delimiter, size_t offset = 0) const {
        return splitf<T>(delimiter.symbols(), delimiter.length(), 0, offset);
    }

    // Начинается ли эта строка с указанной подстроки
    // Does this string start with the specified substring
    constexpr bool starts_with(const K* prefix, size_t l) const noexcept {
        return _len() >= l && 0 == traits::compare(_str(), prefix, l);
    }
    /*!
     * @ru @brief Начинается ли строка с заданной подстроки.
     * @param prefix - подстрока.
     * @en @brief Whether the string begins with the given substring.
     * @param prefix - substring.
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
     * @ru @brief Начинается ли строка с заданной подстроки без учёта регистра ASCII символов.
     * @param prefix - подстрока.
     * @en @brief Whether the string begins with the given substring in a case-insensitive ASCII character.
     * @param prefix - substring.
     */
    constexpr bool starts_with_ia(str_piece prefix) const noexcept {
        return starts_with_ia(prefix.symbols(), prefix.length());
    }

    // Является ли эта строка началом указанной строки
    // Is this string the beginning of the specified string
    constexpr bool prefix_in(const K* text, size_t len) const noexcept {
        size_t myLen = _len();
        if (myLen > len)
            return false;
        return !myLen || 0 == traits::compare(text, _str(), myLen);
    }
    /*!
     * @ru @brief Является ли эта строка началом другой строки.
     * @param text - другая строка.
     * @en @brief Whether this string is the beginning of another string.
     * @param text - another string.
     */
    constexpr bool prefix_in(str_piece text) const noexcept {
        return prefix_in(text.symbols(), text.length());
    }
    // Заканчивается ли строка указанной подстрокой
    // Does the string end with the specified substring
    constexpr bool ends_with(const K* suffix, size_t len) const noexcept {
        size_t myLen = _len();
        return len <= myLen && traits::compare(_str() + myLen - len, suffix, len) == 0;
    }
    /*!
     * @ru @brief Заканчивается ли строка указанной подстрокой.
     * @param suffix - подстрока.
     * @en @brief Whether the string ends with the specified substring.
     * @param suffix - substring.
     */
    constexpr bool ends_with(str_piece suffix) const noexcept {
        return ends_with(suffix.symbols(), suffix.length());
    }
    // Заканчивается ли строка указанной подстрокой без учета регистра ASCII
    // Whether the string ends with the specified substring, case insensitive ASCII
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
     * @ru @brief Заканчивается ли строка указанной подстрокой без учёта регистра ASCII символов.
     * @param suffix - подстрока.
     * @en @brief Whether the string ends with the specified substring in a case-insensitive ASCII character.
     * @param suffix - substring.
     */
    constexpr bool ends_with_ia(str_piece suffix) const noexcept {
        return ends_with_ia(suffix.symbols(), suffix.length());
    }
    /*!
     * @ru @brief Содержит ли строка только ASCII символы.
     * @en @brief Whether the string contains only ASCII characters.
     */
    constexpr bool is_ascii() const noexcept {
        if (_is_empty())
            return true;
        if (std::is_constant_evaluated()) {
            for (size_t idx = 0; idx < _len(); idx++) {
                if (uns_type(_str()[idx]) > 127) {
                    return false;
                }
            }
            return true;
        }
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
     * @ru @brief Получить копию строки в верхнем регистре ASCII символов.
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в верхнем регистре.
     * @en @brief Get a copy of the string in uppercase ASCII characters.
     * @tparam R - the desired string type, by default the same whose method was called.
     * @return R - uppercase copy of the string.
     */
    template<typename R = my_type>
    R upperred_only_ascii() const {
        return R::upperred_only_ascii_from(d());
    }
    /*!
     * @ru @brief Получить копию строки в нижнем регистре ASCII символов.
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @return R - копию строки в нижнем регистре.
     * @en @brief Get a copy of the string in lowercase ASCII characters.
     * @tparam R - the desired string type, by default the same whose method was called.
     * @return R - lowercase copy of the string.
     */
    template<typename R = my_type>
    R lowered_only_ascii() const {
        return R::lowered_only_ascii_from(d());
    }
    /*!
     * @ru @brief Получить копию строки с заменёнными вхождениями подстрок.
     * @tparam R - желаемый тип строки, по умолчанию тот же, чей метод вызывался.
     * @param pattern - искомая подстрока.
     * @param repl - строка, на которую заменять.
     * @param offset - начальная позиция поиска.
     * @param maxCount - максимальное количество замен, 0 - без ограничений.
     * @return R строку заданного типа, по умолчанию того же, чей метод вызывался.
     * @en @brief Get a copy of the string with occurrences of substrings replaced.
     * @tparam R - the desired string type, by default the same whose method was called.
     * @param pattern - the substring to search for.
     * @param repl - the string to replace with.
     * @param offset - starting position of the search.
     * @param maxCount - maximum number of replacements, 0 - no restrictions.
     * @return R a string of the given type, by default the same whose method was called.
     */
    template<typename R = my_type>
    R replaced(str_piece pattern, str_piece repl, size_t offset = 0, size_t maxCount = 0) const {
        return R::replaced_from(d(), pattern, repl, offset, maxCount);
    }

    template<StrType<K> From>
    constexpr static my_type make_trim_op(const From& from, const auto& opTrim) {
        str_piece sfrom = from, newPos = opTrim(sfrom);
        if (newPos.is_same(sfrom)) {
            my_type res = from;
            return res;
        }
        return my_type{newPos};
    }
    template<TrimSides S, StrType<K> From>
    constexpr static my_type trim_static(const From& from) {
        return make_trim_op(from, trim_operator<S, K, static_cast<size_t>(-1), true>{});
    }

    template<TrimSides S, bool withSpaces, typename T, size_t N = const_lit_for<K, T>::Count, StrType<K> From>
        requires is_const_pattern<N>
    constexpr static my_type trim_static(const From& from, T&& pattern) {
        return make_trim_op(from, trim_operator<S, K, N - 1, withSpaces>{pattern});
    }

    template<TrimSides S, bool withSpaces, StrType<K> From>
    constexpr static my_type trim_static(const From& from, str_piece pattern) {
        return make_trim_op(from, trim_operator<S, K, 0, withSpaces>{{pattern}});
    }
    /*!
     * @ru @brief Получить строку с удалением пробельных символов слева и справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @return R - строка, с удалёнными в начале и в конце пробельными символами.
     * @en @brief Get a string with whitespace removed on the left and right.
     * @tparam R - desired string type, default str_src.
     * @return R - a string with whitespace characters removed at the beginning and end.
     */
    template<typename R = str_piece>
    constexpr R trimmed() const {
        return R::template trim_static<TrimSides::TrimAll>(d());
    }
    /*!
     * @ru @brief Получить строку с удалением пробельных символов слева.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @return R - строка, с удалёнными в начале пробельными символами.
     * @en @brief Get a string with whitespace removed on the left.
     * @tparam R - desired string type, default str_src.
     * @return R - a string with leading whitespace characters removed.
     */
    template<typename R = str_piece>
    R trimmed_left() const {
        return R::template trim_static<TrimSides::TrimLeft>(d());
    }
    /*!
     * @ru @brief Получить строку с удалением пробельных символов справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @return R - строка, с удалёнными в конце пробельными символами.
     * @en @brief Get a string with whitespace removed on the right.
     * @tparam R - desired string type, default str_src.
     * @return R - a string with whitespace characters removed at the end.
     */
    template<typename R = str_piece>
    R trimmed_right() const {
        return R::template trim_static<TrimSides::TrimRight>(d());
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, слева и справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в литерале.
     * @en @brief Get a string with the characters specified by the string literal removed from the left and right.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the literal removed at the beginning and at the end.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, слева.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале символами, содержащимися в литерале.
     * @en @brief Get a string with the characters specified by the string literal removed from the left.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the literal removed at the beginning.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в конце символами, содержащимися в литерале.
     * @en @brief Get a string with the characters specified by the string literal removed from the right.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with characters contained in the literal removed at the end.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    // Триминг по символам в литерале и пробелам
    // Trimming by characters in literal and spaces

    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, слева и справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в литерале
     *  и пробельными символами.
     * @en @brief Get a string with the characters specified by the string literal removed, as well as
     * whitespace characters, left and right.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the literal removed at the beginning and at the end
     * and whitespace characters.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, слева.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале символами, содержащимися в литерале
     *  и пробельными символами.
     * @en @brief Get a string with the characters specified by the string literal removed, as well as
     * whitespace characters, left.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the literal removed at the beginning
     *  and whitespace characters.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_left_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных строковым литералом, а также
     *  пробельных символов, справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строковый литерал, задающий символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в конце символами, содержащимися в литерале
     *  и пробельными символами.
     * @en @brief Get a string with the characters specified by the string literal removed, as well as
     * whitespace characters, right.
     * @tparam R - desired string type, default str_src.
     * @param pattern is a string literal specifying the characters that will be trimmed.
     * @return R - a string with characters contained in the literal removed at the end
     *  and whitespace characters.
     */
    template<typename R = str_piece, typename T, size_t N = const_lit_for<K, T>::Count>
        requires is_const_pattern<N>
    R trimmed_right_with_spaces(T&& pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }
    // Триминг по динамическому источнику
    // Trimming by dynamic source

    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, слева и справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в шаблоне.
     * @en @brief Get a string with characters specified by another string removed, left and right.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the pattern removed at the beginning and at the end.
     */
    template<typename R = str_piece>
    R trimmed(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, false>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, слева.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале символами, содержащимися в шаблоне.
     * @en @brief Get a string with characters specified by another string removed from the left.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the pattern removed at the beginning.
     */
    template<typename R = str_piece>
    R trimmed_left(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, false>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в конце символами, содержащимися в шаблоне.
     * @en @brief Get a string with characters specified by another string removed to the right.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with characters contained in the pattern removed at the end.
     */
    template<typename R = str_piece>
    R trimmed_right(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, false>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, слева и справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале и в конце символами, содержащимися в шаблоне
     *  и пробельными символами.
     * @en @brief Get a string, removing characters specified by another string, as well as
     * whitespace characters, left and right.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the pattern removed at the beginning and at the end
     *  and whitespace characters.
     */
    template<typename R = str_piece>
    R trimmed_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimAll, true>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, слева.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в начале символами, содержащимися в шаблоне
     *  и пробельными символами.
     * @en @brief Get a string, removing characters specified by another string, as well as
     * whitespace characters, left.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with the characters contained in the pattern removed at the beginning
     *  and whitespace characters.
     */
    template<typename R = str_piece>
    R trimmed_left_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimLeft, true>(d(), pattern);
    }
    /*!
     * @ru @brief Получить строку с удалением символов, заданных другой строкой, а также
     *  пробельных символов, справа.
     * @tparam R - желаемый тип строки, по умолчанию str_src.
     * @param pattern - строка, задающая символы, которые будут обрезаться.
     * @return R - строка, с удалёнными в конце символами, содержащимися в шаблоне
     *  и пробельными символами.
     * @en @brief Get a string, removing characters specified by another string, as well as
     * whitespace characters, right.
     * @tparam R - desired string type, default str_src.
     * @param pattern - a string specifying the characters that will be trimmed.
     * @return R - a string with characters contained in the template removed at the end
     * and whitespace characters.
     */
    template<typename R = str_piece>
    R trimmed_right_with_spaces(str_piece pattern) const {
        return R::template trim_static<TrimSides::TrimRight, true>(d(), pattern);
    }

    /*!
     * @ru @brief Получить объект `Splitter` по заданному разделителю, который позволяет последовательно
     *        получать подстроки методом `next()`, пока `is_done()` false.
     * @param delimiter - разделитель.
     * @return Splitter<K>.
     * @en @brief Retrieve a `Splitter` object by the given splitter, which allows sequential
     * get substrings using the `next()` method while `is_done()` is false.
     * @param delimiter - delimiter.
     * @return Splitter<K>.
     */
    constexpr SplitterBase<K, str_piece> splitter(str_piece delimiter) const {
        return SplitterBase<K, str_piece>{*this, delimiter};
    }
};

template<size_t N> requires (N > 1)
struct find_all_container {
    static constexpr size_t max_capacity = N;
    size_t positions_[N];
    size_t added_{};

    constexpr void emplace_back(size_t pos) {
        positions_[added_++] = pos;
    }
};

/*!
 * @ru @brief Простейший класс иммутабельной не владеющей строки.
 * @details Этот класс заменяет `simple_str`, если вы используете только "strexpr.h".
 *  Аналог std::string_view. Содержит только указатель и длину.
 *  Как наследник от str_algs поддерживает все константные строковые методы,
 *  за исключением парсинга double из любых типов символов - поддерживаются только
 *  char, wchar_t, и совместимые с ними по размеру символы. Также не содержит
 *  работы с упрощённым юникодом.
 * @tparam K - тип символов строки.
 * @en @brief The simplest class of an immutable non-owning string.
 * @details This class replaces `simple_str` if you only use `strexpr.h`.
 *  Analogous to std::string_view. Contains only a pointer and a length.
 *  As a descendant of str_algs, it supports all constant string methods,
 *  except for parsing double from any character types - only supported
 *  char, wchar_t, and characters compatible with them in size. Also does not contain
 *  work with simplified Unicode.
 * @tparam K - the character type of the string.
 */
template<typename K>
struct str_src : str_src_algs<K, str_src<K>, str_src<K>, false> {
    using symb_type = K;
    using my_type = str_src<K>;

    const symb_type* str;
    size_t len;

    str_src() = default;
    /*!
     * @ru @brief Конструктор из строкового литерала.
     * @en @brief Constructor from a string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr str_src(T&& v) noexcept : str((const K*)v), len(N - 1) {}

    /*!
     * @ru @brief Конструктор из указателя и длины.
     * @en @brief Constructor from pointer and length.
     */
    constexpr str_src(const K* p, size_t l) noexcept : str(p), len(l) {}

    template<StrType<K> T>
    constexpr str_src(T&& t) : str(t.symbols()), len(t.length()){}

    /*!
     *@ru @brief Конструктор из std::basic_string.
     *@en @brief Constructor from std::basic_string.
     */
    template<typename A>
    constexpr str_src(const std::basic_string<K, std::char_traits<K>, A>& s) noexcept : str(s.data()), len(s.length()) {}
    /*!
     *@ru @brief Конструктор из std::basic_string_view.
     *@en @brief Constructor from std::basic_string_view.
     */
    constexpr str_src(const std::basic_string_view<K, std::char_traits<K>>& s) noexcept : str(s.data()), len(s.length()) {}

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
    constexpr bool is_same(str_src<K> other) const noexcept {
        return str == other.str && len == other.len;
    }
    /*!
     * @ru @brief Проверить, не является ли строка частью другой строки.
     * @param other - другая строка.
     * @en @brief Check if a string is part of another string.
     * @param other - another string.
     */
    constexpr bool is_part_of(str_src<K> other) const noexcept {
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
     * @en @brief Shifts the start of a line by the specified number of characters.
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

/*!
 * @ru @brief Класс, заявляющий, что ссылается на нуль-терминированную строку.
 * @tparam K - тип символов строки.
 * @details Упрощённая реализация simple_str_nt, когда вы используете только strexpr.h.
 *      Служит для показа того, что функция параметром хочет получить
 *      строку с нулем в конце, например, ей надо дальше передавать его в
 *      стороннее API. Без этого ей надо было бы либо указывать параметром
 *      конкретный класс строки, что лишает универсальности, либо приводило бы
 *      к постоянным накладным расходам на излишнее копирование строк во временный
 *      буфер. Источником нуль-терминированных строк могут быть строковые литералы
 *      при компиляции, либо классы, хранящие строки.
 * @en @brief A class that claims to refer to a null-terminated string.
 * @tparam K - the character type of the string.
 * @details Simplified implementation of simple_str_nt when you only use strexpr.h.
 * Shows what the function wants to receive as a parameter
 * a string with a zero at the end, for example, she needs to further transfer it to
 * third party API. Without this, she would have to either specify the parameter
 * specific string class, which deprives universality, or would lead
 * to the constant overhead of unnecessary copying of string into the temporary
 * buffer. Null-terminated strings can be sourced from string literals
 * during compilation, or classes that store strings.
 */
template<typename K>
struct str_src_nt : str_src<K>, null_terminated<K, str_src_nt<K>> {
    using symb_type = K;
    using my_type = str_src_nt<K>;
    using base = str_src<K>;

    constexpr static const K empty_string[1] = {0};

    str_src_nt() = default;
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
    template<typename T> requires std::is_same_v<std::remove_const_t<std::remove_pointer_t<std::remove_cvref_t<T>>>, K>
    constexpr explicit str_src_nt(T&& p) noexcept {
        base::len = p ? static_cast<size_t>(base::traits::length(p)) : 0;
        base::str = base::len ? p : empty_string;
    }
    /*!
     * @ru @brief Конструктор из строкового литерала.
     * @en @brief Constructor from a string literal.
     */
    template<typename T, size_t N = const_lit_for<K, T>::Count>
    constexpr str_src_nt(T&& v) noexcept : base(std::forward<T>(v)) {}

    /*!
     * @ru @brief Конструктор из указателя и длины.
     * @en @brief Constructor from pointer and length.
     */
    constexpr str_src_nt(const K* p, size_t l) noexcept : base(p, l) {}

    template<StrType<K> T>
    constexpr str_src_nt(T&& t) {
        base::str = t.symbols();
        base::len = t.length();
    }
    /*!
     *@ru @brief Конструктор из std::basic_string.
     *@en @brief Constructor from std::basic_string.
     */
    template<typename A>
    constexpr str_src_nt(const std::basic_string<K, std::char_traits<K>, A>& s) noexcept : base(s) {}

    static const my_type empty_str;
    /*!
     * @ru @brief Получить нуль-терминированную строку, сдвинув начало на заданное количество символов.
     * @param from - на сколько символов сдвинуть начало строки.
     * @return my_type.
     * @en @brief Get a null-terminated string by shifting the start by the specified number of characters.
     * @param from - by how many characters to shift the beginning of the line.
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
inline const str_src_nt<K> str_src_nt<K>::empty_str{str_src_nt<K>::empty_string, 0};
template<typename K> struct simple_str_selector;

inline namespace literals {
/*!
 * @ingroup StrExprs
 * @ru @brief Создает набор из основания системы счисления и флагов, который может быть применён к целым
 *  числам для задания параметров форматирования с помощью оператора деления: `num / 0xПараметрыФорматирования_fmt`.
 * Параметры форматирования задаются следующим образом: сначала идёт `0x`, затем основание счисления, записанное в
 * десятичном виде. Далее могут идти символы, обозначающие различные флаги:
 * - b при задании ширины поля выравнивать влево, аналог f::l.
 * - d при задании ширины поля выравнивать вправо, аналог f::r.
 * - c при задании ширины поля выравнивать по центру, аналог f::c.
 * - a выводить префикс системы счисления, 0b для 2, 0 для 8, 0x для 16, аналог f::p.
 * - A выводить префикс системы счисления, 0B для 2, 0 для 8, 0X для 16, аналог f::P.
 * - 0 дополнять число до заданной ширины нулями слева. Не совместимо с опциями выравнивания, аналог f::z.
 * - E Для систем счисления более 10, выводить символы в верхнем регистре, аналог f::u.
 * - e Для знаковых типов чисел для положительных значений выводить '+' перед числом, аналог f::sp.
 * - f Для знаковых типов чисел для положительных значений выводить пробел перед числом, аналог f::ss.
 * - FКодCимволаВhex Задаёт код символа-заполнителя при указании ширины поля, аналог f::f<'c'>.
 * - Число в десятичном виде, начинающееся не с 0. Задаёт ширину поля, аналог f::w<N>.
 *   При указании ширины можно либо указать желаемое выравнивание (b, c, d),
 *   либо дополнение нулями (0), но не оба сразу. Если ничего из этого не указано, применяется выравнивание вправо.
 *   Число дополняется до заданной ширины, если оно короче. Если длиннее, то выводится всё число.
 * - ' разделитель, пропускается.
 *
 * Так как после `F` все символы воспринимаются как hex код символа разделителя, при необходимости его использования
 * лучше ставить его в конце литерала форматирования, или отделять '.
 * <br/>
 * @en @brief Creates a set of radix and flags that can be applied to integers
 * numbers to set formatting parameters using the division operator: `num / 0xFormatOptions_fmt`.
 * Formatting parameters are set as follows: first comes `0x`, then the radix written in
 * decimal form. Next may be symbols indicating various flags:
 * - b when setting the field width, align to the left, analogous to f::l.
 * - d when setting the field width, align to the right, analogous to f::r.
 * - c when setting the field width, align to the center, analogous to f::c.
 * - a display the number system prefix, 0b for 2, 0 for 8, 0x for 16, analogous to f::p.
 * - A display the number system prefix, 0B for 2, 0 for 8, 0X for 16, analogous to f::P.
 * - 0 pad the number to the specified width with zeros on the left. Not compatible with alignment options, similar to f::z.
 * - E For radix greater than 10, display characters in uppercase, analogous to f::u.
 * - e For signed number types, for positive values, print '+' before the number, analogous to f::sp.
 * - f For signed number types, for positive values, print a space before the number, analogous to f::ss.
 * - FCodeInhex Sets the code of the filler character when specifying the field width, analogous to f::f<'c'>.
 * - Number in decimal form, not starting from 0. Sets the field width, analogous to f::w<N>.
 *   When specifying the width, you can either specify the desired alignment (b, c, d),
 *   or zero padding (0), but not both. If none of these are specified, right alignment is applied.
 *   The number is padded to the given width if it is shorter. If it is longer, then the entire number is displayed.
 * - ' delimiter, skipped.
 *
 * Since after `F` all characters are perceived as hex code of the delimiter character, if necessary, use it
 * it is better to put it at the end of format literal, or separate with '.
 * <br/>
 * @ru Пример: @en Example: @~
 * ```cpp
 *  stringu u16t = u"Number "_ss + num
 *      + " in octal is " + num / 0x8'016_fmt      // same as e_int<8, f::w<16> | f::z>(num)
 *      + ", in binary is " + num / 0x2a032_fmt;   // same as e_int<2, f::w<32> | f::p | f::z>(num);
 *   ....
 *  stringa text = "Count is "_ss + count / 0x16A08E_fmt; // same as e_int<16, f::P | f::z | f::w<8> | f::u>(count)
 *   ....
 *  stringa text = "Number "_ss + count / 0x16c20EF5F_fmt; // same as e_int<16, f::c | f::w<20> | f::u | f::f<'_'>>(count)
 * ```
 */
template<char...Chars>
SS_CONSTEVAL auto operator""_fmt() {
    return f::skip_0x<Chars...>();
}

} // namespace literals

#ifndef IN_FULL_SIMSTR

template<typename K>
using simple_str = str_src<K>;

template<typename K>
struct simple_str_selector {
    using type = simple_str<K>;
};

template<typename K>
using simple_str_nt = str_src_nt<K>;

template<typename K>
using Splitter = SplitterBase<K, str_src<K>>;

using ssa = str_src<u8s>;
using ssb = str_src<ubs>;
using ssw = str_src<wchar_t>;
using ssu = str_src<u16s>;
using ssuu = str_src<u32s>;
using stra = str_src_nt<u8s>;
using strb = str_src_nt<ubs>;
using strw = str_src_nt<wchar_t>;
using stru = str_src_nt<u16s>;
using struu = str_src_nt<u32s>;

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

inline namespace literals {

/*!
 * @ru @brief Оператор литерал в str_src.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return str_src.
 * @en @brief Operator literal in str_src.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return str_src.
 */
SS_CONSTEVAL str_src_nt<u8s> operator""_ss(const u8s* ptr, size_t l) {
    return str_src_nt<u8s>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в str_src.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return str_src.
 * @en @brief Operator literal in str_src.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return str_src.
 */
SS_CONSTEVAL str_src_nt<ubs> operator""_ss(const ubs* ptr, size_t l) {
    return str_src_nt<ubs>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в str_src.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return str_src.
 * @en @brief Operator literal in str_src.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return str_src.
 */
SS_CONSTEVAL str_src_nt<uws> operator""_ss(const uws* ptr, size_t l) {
    return str_src_nt<uws>{ptr, l};
}
/*!
 * @ru @brief Оператор литерал в str_src.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return str_src.
 * @en @brief Operator literal in str_src.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return str_src.
 */
SS_CONSTEVAL str_src_nt<u16s> operator""_ss(const u16s* ptr, size_t l) {
    return str_src_nt<u16s>{ptr, l};
}

/*!
 * @ru @brief Оператор литерал в str_src.
 * @param ptr - указатель на строку.
 * @param l - длина строки.
 * @return str_src.
 * @en @brief Operator literal in str_src.
 * @param ptr - pointer to a string.
 * @param l - string length.
 * @return str_src.
 */
SS_CONSTEVAL str_src_nt<u32s> operator""_ss(const u32s* ptr, size_t l) {
    return str_src_nt<u32s>{ptr, l};
}

} // namespace literals

#endif

template<typename K, bool withSpaces>
struct CheckSpaceTrim {
    constexpr bool is_trim_spaces(K s) const {
        return s == ' ' || (s >= 9 && s <= 13); // || isspace(s);
    }
};
template<typename K>
struct CheckSpaceTrim<K, false> {
    constexpr bool is_trim_spaces(K) const {
        return false;
    }
};

template<typename K>
struct CheckSymbolsTrim {
    str_src<K> symbols;
    constexpr bool is_trim_symbols(K s) const {
        return symbols.len != 0 && str_src<K>::traits::find(symbols.str, symbols.len, s) != nullptr;
    }
};

template<typename K, size_t N>
struct CheckConstSymbolsTrim {
    const const_lit_to_array<K, N> symbols;

    template<typename T, size_t M = const_lit_for<K, T>::Count> requires (M == N + 1)
    constexpr CheckConstSymbolsTrim(T&& s) : symbols(std::forward<T>(s)) {}

    constexpr bool is_trim_symbols(K s) const noexcept {
        return symbols.contain(s);
    }
};

template<typename K>
struct CheckConstSymbolsTrim<K, 0> {
    constexpr bool is_trim_symbols(K) const {
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
    constexpr bool isTrim(K s) const {
        return CheckSpaceTrim<K, withSpaces>::is_trim_spaces(s) || SymbSelector<K, N>::type::is_trim_symbols(s);
    }
    constexpr str_src<K> operator()(str_src<K> from) const {
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

template<TrimSides S = TrimSides::TrimAll, bool withSpaces = false, typename K, typename T, size_t N = const_lit_for<K, T>::Count>
    requires is_const_pattern<N>
constexpr inline auto trimOp(T&& pattern) {
    return trim_operator<S, K, N - 1, withSpaces>{pattern};
}

template<TrimSides S = TrimSides::TrimAll, bool withSpaces = false, typename K>
constexpr inline auto trimOp(str_src<K> pattern) {
    return trim_operator<S, K, 0, withSpaces>{pattern};
}

static constexpr size_t FIND_CACHE_SIZE = 16;

template<typename K, size_t N, size_t L>
struct expr_replaces : expr_to_std_string<expr_replaces<K, N, L>> {
    using symb_type = K;
    using my_type = expr_replaces<K, N, L>;
    str_src<K> what;
    const K(&pattern)[N + 1];
    const K(&repl)[L + 1];
    mutable find_all_container<FIND_CACHE_SIZE> matches_;
    mutable size_t last_;

    constexpr expr_replaces(str_src<K> w, const K(&p)[N + 1], const K(&r)[L + 1]) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length();
        if constexpr (N == L) {
            return l;
        }
        what.find_all_to(matches_, pattern, N, 0, FIND_CACHE_SIZE);
        if (matches_.added_) {
            last_ = matches_.positions_[matches_.added_ - 1] + N;
            l += int(L - N) * matches_.added_;

            if (matches_.added_ == FIND_CACHE_SIZE) {
                for (;;) {
                    size_t next = what.find(pattern, N, last_);
                    if (next == str::npos) {
                        break;
                    }
                    last_ = next + N;
                    l += L - N;
                }
            }
        }
        if (!l) {
            matches_.added_ = -1;
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
        if (matches_.added_ == 0) {
            return what.place(ptr);
        } else if (matches_.added_ == -1) {
            // after replaces text become empty
            return ptr;
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = matches_.positions_[0], idx = 1; ;) {
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
                offset = idx < FIND_CACHE_SIZE ? matches_.positions_[idx++] : what.find(pattern, N, start);
            }
        }
        return ptr;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Получить строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки.
 * @tparam K - тип символа, выводится из первого аргумента.
 * @param w - начальная строка.
 * @param p - строковый литерал, искомая подстрока.
 * @param r - строковый литерал, на что заменять.
 * @en @brief Get a string expression that generates a string with all occurrences of a given substring replaced.
 * @tparam K - the type of the symbol, inferred from the first argument.
 * @param w - starting string.
 * @param p - string literal, searched substring.
 * @param r - string literal, what to replace with.
 */
template<StrSource A, typename K = symb_type_from_src_t<A>, typename T, size_t N = const_lit_for<K, T>::Count, typename X, size_t L = const_lit_for<K, X>::Count>
    requires(N > 1)
constexpr auto e_repl(A&& w, T&& p, X&& r) {
    return expr_replaces<K, N - 1, L - 1>{std::forward<A>(w), p, r};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки на другую строку.
 * @tparam K - тип строки.
 * @en @brief A string expression that generates a string replacing all occurrences of the given substring to another string.
 * @tparam K - string type.
 */
template<typename K>
struct expr_replaced : expr_to_std_string<expr_replaced<K>> {
    using symb_type = K;
    using my_type = expr_replaced<K>;
    str_src<K> what;
    const str_src<K> pattern;
    const str_src<K> repl;
    mutable find_all_container<FIND_CACHE_SIZE> matches_;
    mutable size_t last_;
    /*!
     * @ru @brief Конструктор.
     * @param w - исходная строка.
     * @param p - искомая подстрока.
     * @param r - строка замены.
     * @en @brief Constructor.
     * @param w - source string.
     * @param p - the searched substring.
     * @param r - replacement string.
     */
    constexpr expr_replaced(str_src<K> w, str_src<K> p, str_src<K> r) : what(w), pattern(p), repl(r) {}

    constexpr size_t length() const {
        size_t l = what.length(), plen = pattern.length(), rlen = repl.length();
        if (!plen || plen == rlen) {
            return l;
        }
        what.find_all_to(matches_, pattern.symbols(), plen, 0, FIND_CACHE_SIZE);
        if (matches_.added_) {
            last_ = matches_.positions_[matches_.added_ - 1] + plen;
            l += int(rlen - plen) * matches_.added_;

            if (matches_.added_ == FIND_CACHE_SIZE) {
                for (;;) {
                    size_t next = what.find(pattern.symbols(), plen, last_);
                    if (next == str::npos) {
                        break;
                    }
                    last_ = next + plen;
                    l += rlen - plen;
                }
            }
        }
        if (!l) {
            matches_.added_ = -1;
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        size_t plen = pattern.length(), rlen = repl.length();
        if (plen == rlen) {
            const K* from = what.symbols();
            for (size_t start = 0; start < what.length();) {
                size_t next = what.find(pattern, start);
                if (next == str::npos) {
                    next = what.length();
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr,  from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl.symbols(), rlen);
                ptr += rlen;
                start = next + plen;
            }
            return ptr;
        }
        if (matches_.added_ == 0) {
            return what.place(ptr);
        } else if (matches_.added_ == -1) {
            // after replaces text become empty
            return ptr;
        }
        const K* from = what.symbols();
        for (size_t start = 0, offset = matches_.positions_[0], idx = 1; ;) {
            ch_traits<K>::copy(ptr, from + start, offset - start);
            ptr += offset - start;
            ch_traits<K>::copy(ptr, repl.symbols(), rlen);
            ptr += rlen;
            start = offset + plen;
            if (start >= last_) {
                size_t tail = what.length() - start;
                ch_traits<K>::copy(ptr, from + start, tail);
                ptr += tail;
                break;
            } else {
                offset = idx < FIND_CACHE_SIZE ? matches_.positions_[idx++] : what.find(pattern.symbols(), plen, start);
            }
        }
        return ptr;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки на строковое выражение.
 * @tparam K - тип строки.
 * @details Если искомая подстрока не найдена, то строковое выражение даже не вычисляется.
 *  Затем при осуществлении замены строковое выражение вычисляется только один раз в место первой замены,
 *  а в следующие места замен просто копируется символы из первого места. Это позволяет экономить память
 *  и время, если вам надо сделать замену на какую-либо "сборную" строку.
 * @en @brief A string expression that generates a string replacing all occurrences of the given substring to string expression.
 * @tparam K - string type.
 * @details If the search substring is not found, the string expression is not even evaluated.
 * Then, when performing a replacement, the string expression is evaluated only once at the first replacement location,
 * and characters from the first location are simply copied to subsequent replacement locations. This saves memory
 * and time if you need to replace with some kind of "composite" string.
 */
template<typename K, StrExprForType<K> E>
struct expr_replaced_e : expr_to_std_string<expr_replaced_e<K, E>> {
    using symb_type = K;
    using my_type = expr_replaced<K>;
    str_src<K> what;
    const str_src<K> pattern;
    mutable size_t replLen;
    mutable find_all_container<FIND_CACHE_SIZE> matches_;
    mutable size_t last_;
    const E& expr;
    /*!
     * @ru @brief Конструктор.
     * @param w - исходная строка.
     * @param p - искомая подстрока.
     * @param e - строковое выражение для замены.
     * @en @brief Constructor.
     * @param w - source string.
     * @param p - the searched substring.
     * @param e - string expression to replace.
     */
    constexpr expr_replaced_e(str_src<K> w, str_src<K> p, const E& e) : what(w), pattern(p), expr(e) {}

    constexpr size_t length() const {
        size_t l = what.length(), plen = pattern.length();
        if (!plen) {
            return l;
        }
        matches_.positions_[0] = what.find(pattern);
        if (matches_.positions_[0] == -1) {
            // Не нашли вхождений, нечего менять
            return l;
        }
        matches_.added_ = 1;
        // Вхождение есть, надо теперь получить длину замены
        replLen = expr.length();
        if (replLen == plen) {
            // Замена той же длины, общая длина не изменится
            return l;
        }
        what.find_all_to(matches_, pattern.symbols(), plen, matches_.positions_[0] + plen, FIND_CACHE_SIZE - 1);

        last_ = matches_.positions_[matches_.added_ - 1] + plen;
        l += int(replLen - plen) * matches_.added_;

        if (matches_.added_ == FIND_CACHE_SIZE) {
            for (;;) {
                size_t next = what.find(pattern.symbols(), plen, last_);
                if (next == str::npos) {
                    break;
                }
                last_ = next + plen;
                l += replLen - plen;
            }
        }
        if (!l) {
            matches_.added_ = -1;
        }
        return l;
    }
    constexpr K* place(K* ptr) const noexcept {
        if (matches_.added_ == 0) {
            // не было найдено вхождений
            return what.place(ptr);
        } else if (matches_.added_ == -1) {
            // Строка стала пустой
            return ptr;
        }
        size_t plen = pattern.length();
        const K* from = what.symbols();
        ch_traits<K>::copy(ptr, from, matches_.positions_[0]);
        ptr += matches_.positions_[0];
        const K* repl = ptr;
        expr.place((typename E::symb_type*)ptr);
        ptr += replLen;
        size_t start = matches_.positions_[0] + plen;

        if (plen == replLen) {
            for (;;) {
                size_t next = what.find(pattern, start);
                if (next == str::npos) {
                    break;
                }
                size_t delta = next - start;
                ch_traits<K>::copy(ptr, from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl, replLen);
                ptr += replLen;
                start = next + plen;
            }
        } else {
            for (size_t idx = 1;;) {
                if (start >= last_) {
                    break;
                }
                size_t next = idx < FIND_CACHE_SIZE ? matches_.positions_[idx++] : what.find(pattern, start);
                size_t delta = next - start;
                ch_traits<K>::copy(ptr, from + start, delta);
                ptr += delta;
                ch_traits<K>::copy(ptr, repl, replLen);
                ptr += replLen;
                start = next + plen;
            }
        }
        size_t tail = what.length() - start;
        ch_traits<K>::copy(ptr, from + start, tail);
        return ptr + tail;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Получить строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки.
 * @tparam K - тип символа, выводится из первого аргумента.
 * @param w - начальная строка.
 * @param p - строковый объект, искомая подстрока, может быть рантайм.
 * @param r - строковый объект, на что заменять, может быть рантайм.
 * @en @brief Get a string expression that generates a string with all occurrences of a given substring replaced.
 * @tparam K - the type of the symbol, inferred from the first argument.
 * @param w - starting string.
 * @param p - string object, searched substring, maybe runtime.
 * @param r - string object, replace substring, maybe runtime.
 */
template<StrSource A, typename K = symb_type_from_src_t<A>, typename T, typename X>
    requires (std::is_constructible_v<str_src<K>, T> && std::is_constructible_v<str_src<K>, X> && (!is_const_lit_v<T> || !is_const_lit_v<X>))
constexpr auto e_repl(A&& w, T&& p, X&& r) {
    str_src<K> pattern{std::forward<T>(p)};
    str_src<K> repl{std::forward<X>(r)};
    return expr_replaced<K>{std::forward<A>(w), pattern, repl};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Получить строковое выражение, генерирующее строку с заменой всех вхождений заданной подстроки.
 * @tparam K - тип символа, выводится из первого аргумента.
 * @param w - начальная строка.
 * @param p - строковый объект, искомая подстрока, может быть рантайм.
 * @param expr - строковое выражение, на что заменять.
 * @en @brief Get a string expression that generates a string with all occurrences of a given substring replaced.
 * @tparam K - the type of the symbol, inferred from the first argument.
 * @param w - starting string.
 * @param p - string object, searched substring, maybe runtime.
 * @param expr - string expression, what to replace with.
 */
template<StrSource A, typename K = symb_type_from_src_t<A>, typename T, StrExprForType<K> E>
    requires std::is_constructible_v<str_src<K>, T>
constexpr auto e_repl(A&& w, T&& p, const E& expr) {
    str_src<K> pattern{std::forward<T>(p)};
    return expr_replaced_e<K, E>{std::forward<A>(w), pattern, expr};
}

template<bool UseVectorForReplace>
struct replace_search_result_store {
    size_t count_{};
    std::pair<size_t, size_t> replaces_[16];
};

template<>
struct replace_search_result_store<true> : std::vector<std::pair<size_t, size_t>> {};

// Строковое выражение для замены символов
// String expression to replace characters
template<typename K, size_t N, bool UseVectorForReplace>
struct expr_replace_const_symbols : expr_to_std_string<expr_replace_const_symbols<K, N, UseVectorForReplace>> {
    using symb_type = K;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;
    const K pattern_[N];
    const str_src<K> source_;
    const str_src<K> replaces_[N];

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    [[_no_unique_address]]
    uu8s bit_mask_[N >= BIT_SEARCH_TRESHHOLD ? (sizeof(K) == 1 ? 32 : 64) : 0]{};

    template<typename ... Repl> requires (sizeof...(Repl) == N * 2)
    constexpr expr_replace_const_symbols(str_src<K> source, Repl&& ... repl) : expr_replace_const_symbols(0, source, std::forward<Repl>(repl)...) {}

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
    constexpr expr_replace_const_symbols(int, str_src<K> source, K s, str_src<K> r, Repl&&... repl) :
        expr_replace_const_symbols(0, source, std::forward<Repl>(repl)..., std::make_pair(s, r)){}

    template<typename ... Repl> requires (sizeof...(Repl) == N)
    constexpr expr_replace_const_symbols(int, str_src<K> source, Repl&&... repl) :
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
 * @ingroup StrExprs
 * @ru @brief Возвращает строковое выражение, генерирующее строку, в которой заданные символы
 * заменены на заданные подстроки.
 * @tparam UseVector - использовать вектор для сохранения результатов поиска символов.
 *      Более подробно описано в `expr_replace_symbols`.
 * @param src - исходная строка.
 * @param symbol - константный символ, который надо заменять.
 * @param repl - строковый литерал, на который заменять символ.
 * @param ... symbol, repl - другие символы и строки.
 * @details Применяется для генерации замены символов на строки, в случае если все они известны
 * в compile time. Пример:
 * @en @brief Returns a string expression that generates a string containing the given characters
 * replaced with given substrings.
 * @tparam UseVector - use a vector to save symbol search results.
 *      Described in more detail in `expr_replace_symbols`.
 * @param src - source string.
 * @param symbol - constant symbol that needs to be replaced.
 * @param repl - string literal to replace the character with.
 * @param ... symbol, repl - other symbols and strings.
 * @details Used to generate character replacements for strings if all of them are known
 * at compile time. Example:
 * @~
 *  ```cpp
 *  out += "<div>" + e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;") + "</div>";
 *  ```
 * @ru В принципе, `e_repl_const_symbols` вполне безопасно возвращать из функции, если исходная строка
 * внешняя по отношению к функции.
 * @en In principle, `e_repl_const_symbols` is quite safe to return from a function if the source string
 * external to function.
 * @~
 *  ```cpp
 *  auto repl_html_symbols(ssa text) {
 *      return e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;");
 *  }
 *  ....
 *  out += "<div>" + repl_html_symbols(content) + "</div>";
 *  ```
 */
template<bool UseVector = false, StrSource A, typename K = symb_type_from_src_t<A>, typename ... Repl>
    requires (sizeof...(Repl) % 2 == 0)
auto e_repl_const_symbols(A&& src, Repl&& ... other) {
    return expr_replace_const_symbols<K, sizeof...(Repl) / 2, UseVector>(std::forward<A>(src), std::forward<Repl>(other)...);
}

/*!
 * @ingroup StrExprs
 * @ru @brief Тип для строкового выражения, генерирующее строку, в которой заданные символы заменяются на заданные строки.
 * @tparam K - тип символа.
 * @tparam UseVectorForReplace - использовать вектор для запоминания результатов поиска вхождений символов.
 * @details Этот тип применяется, когда состав символов или соответствующих им замен не известен в compile time,
 * а определяется в runtime. В конструктор передается вектор из пар `символ - строка замены`.
 * Параметр `UseVectorForReplace` задаёт стратегию реализации. Дело в том, что работа любых строковых выражений
 * разбита на две фазы - вызов `length()`, в котором подсчитывается количество символов в результате,
 * и вызов `place()`, в котором результат помещается в предоставленный буфер.
 * При `UseVectorForReplace == true` во время фазы подcчёта количества символов, позиции найденных вхождений
 * сохраняются в векторе, и во время второй фазы поиск уже не выполняется, а позиции берутся из вектора.
 * Это, с одной стороны, уменьшает время во второй фазе - не нужно снова выполнять поиск, но увеличивает
 * время в первой фазе - добавление элементов в вектор не бесплатно, и требует времени.
 * При `UseVectorForReplace == false` во время фазы подcчёта количества символов, в локальном массиве запоминаются позиции
 * первых 16 вхождений и их общее количество, а во время второй фазы, если вхождений больше 16, то поиск повторяется,
 * но уже только с позиции 16го вхождения. Это может увеличить время во второй фазе, но сокращает время в первой
 * фазе - не нужно добавлять элементы в вектор, не нужна динамическая аллокация.
 * В разных сценариях использования более оптимальными могут быть та или иная стратегия, и вы можете сами решить,
 * что в каждом конкретном случае больше подойдёт.
 * @en @brief A type for a string expression that generates a string in which the given characters are replaced by the given strings.
 * @tparam K - symbol type.
 * @tparam UseVectorForReplace - use a vector to remember the results of searching for occurrences of characters.
 * @details This type is used when the composition of symbols or their corresponding replacements is not known at compile time,
 * and is defined at runtime. A vector of `character - replacement string` pairs is passed to the constructor.
 * The `UseVectorForReplace` parameter specifies the implementation strategy. The point is that the work of any string expressions
 * is divided into two phases - the `length()` call, which counts the number of characters in the result,
 * and a call to `place()`, which places the result in the provided buffer.
 * When `UseVectorForReplace == true` during the phase of counting the number of characters, the position of the found occurrences
 * are stored in the vector, and during the second phase the search is no longer performed, and the positions are taken from the vector.
 * This, on the one hand, reduces the time in the second phase - there is no need to search again, but it increases
 * time in the first phase - adding elements to the vector is not free, and takes time.
 * When `UseVectorForReplace == false` during the phase of counting the number of characters, positions in the local array are remembered
 * the first 16 occurrences and their total number, and during the second phase, if there are more than 16 occurrences, then the search is repeated,
 * but only from the position of the 16th occurrence. This may increase the time in the second phase, but reduces the time in the first
 * phase - no need to add elements to the vector, no need for dynamic allocation.
 *In different use cases, one or another strategy may be more optimal, and you can decide for yourself
 * whichever is more suitable in each specific case.
 */
template<typename K, bool UseVectorForReplace = false>
struct expr_replace_symbols : expr_to_std_string<expr_replace_symbols<K, UseVectorForReplace>> {
    using symb_type = K;
    using str_t = typename simple_str_selector<K>::type;
    inline static const int BIT_SEARCH_TRESHHOLD = 4;

    const str_src<K> source_;
    const std::vector<std::pair<K, str_t>>& replaces_;

    std::basic_string<K, ch_traits<K>, std::allocator<K>> pattern_;

    mutable replace_search_result_store<UseVectorForReplace> search_results_;

    uu8s bit_mask_[sizeof(K) == 1 ? 32 : 64]{};
    /*!
     * @ru @brief Конструктор выражения.
     * @param source - исходная строка.
     * @param repl - вектор из пар "символ->строка замены".
     * @details Пример:
     * @en @brief Expression constructor.
     * @param source - source string.
     * @param repl - a vector of "character->replacement string" pairs.
     * @details Example:
     * @~
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
     * @ru Пример приведен для наглядности использования. В данном случае и заменяемые символы, и строки замены
     * известны в compile time, и в этом случае лучше применять e_repl_const_symbols, а этот класс
     * используется, когда символы или замены задаются в runtime.
     * @en An example is provided for clarity of use. In this case, both the characters to be replaced and the replacement strings
     * known at compile time, in which case it is better to use e_repl_const_symbols, and this class
     * is used when characters or replacements are specified at runtime.
     */
    constexpr expr_replace_symbols(str_t source, const std::vector<std::pair<K, str_t>>& repl )
        : source_(source), replaces_(repl)
    {
        size_t pattern_len = replaces_.size();
        pattern_.resize(pattern_len);
        K* pattern = pattern_.data();

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

template<typename K, StrExprForType<K> T>
struct force_copy {
    const T& t_;
    force_copy(const T& t) : t_(t){}
};

template<typename K, typename T>
struct symb_type_from_src<force_copy<K, T>> {
    using type = K;
};


template<StrExpr T>
force_copy(T&&) -> force_copy<typename T::symb_type, T>;

template<typename K, typename T>
constexpr auto to_subst(T&& t) {
    return to_strexpr<K>(std::forward<T>(t));
}

template<typename K, typename T, size_t N>
constexpr auto to_subst(const T(&t)[N]) {
    return expr_literal<T, N - 1>{t};
}

template<typename K, StrExprForType<K> T>
constexpr decltype(auto) to_subst(T&& t) {
    return std::forward<T>(t);
}

template<typename K, typename T>
constexpr T to_subst(const force_copy<K, T>& t) {
    return t.t_;
}

template<typename K, typename T>
constexpr T to_subst(force_copy<K, T>&& t) {
    return t.t_;
}

template<typename K, typename T>
constexpr T to_subst(force_copy<K, T>& t) {
    return t.t_;
}

template<typename K, typename T>
using to_str_exp_t = decltype(to_subst<K>(std::declval<T>()));

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражения, объединяющее указанные строковые выражения, с использованием заданного разделителя.
 * @tparam K - тип символов, выводится из типа разделителя.
 * @en @brief String expression concatenating the specified string expressions using the specified delimiter.
 * @tparam K - character type, deduced from the separator type.
 */
template<typename K, typename G, typename Arg, typename...Args>
struct e_concat : expr_to_std_string<e_concat<K, G, Arg, Args...>> {
    using symb_type = K;
    using store_t = std::tuple<to_str_exp_t<K, Args>...>;
    using arg_t = to_str_exp_t<K, Arg>;
    to_str_exp_t<K, G> glue_;
    arg_t arg_;
    store_t args_;
    mutable size_t glue_len_;
    /*!
    * @ru @brief Создание строкового выражения, объединяющего указанные строковые выражения, с использованием заданного разделителя.
    * @param glue - "клей", используемый при соединении аргументов, вставляется между ними.
    * @param arg, args... - объединяемые аргументы, не менее двух.
    * @details "Склеивает" переданные аргументы, вставляя между ними заданный "клей".
    * Соединителем и аргументами могут быть строковые литералы, строковые выражения, стандартные строки.
    * Аргументами также могут быть любые типы, для которых есть преобразование в строковое выражение.
    * (см. @ref ConvertToStrExpr "Конвертация типов в в строковые выражения").
    * Для аргументов, которые сами являются строковыми выражениями, `e_concat` сохраняет только ссылку на них.
    * Обычно это не является проблемой, если ссылка не на временный объект, или строковое выражение материализуется
    * сейчас же, до ';'. Если же вам необходимо вернуть `e_concat` как строковое выражение из функции,
    * можно заставить его сохранить аргументы строковые выражения по копии, обернув их в `force_copy{}`.
    * См. пример в tests/test_tostrexpr.cpp, Method4.
    * @en @brief Create a string expression concatenating the specified string expressions using the specified delimiter.
    * @param glue - the "glue" used when connecting arguments is inserted between them.
    * @param arg, args... - the arguments to be combined, at least two.
    * @details "Glues" the passed arguments, inserting the specified "glue" between them.
    * The connector and arguments can be string literals, string expressions, standard strings.
    * Arguments can also be any type for which there is a conversion to a string expression.
    * (see @ref ConvertToStrExpr "Converting types to string expressions").
    * For arguments that are themselves string expressions, `e_concat` stores only a reference to them.
    * This is usually not a problem if the reference is not to a temporary object, or the string expression is materialized
    * now, before ';'. If you need to return `e_concat` as a string expression from a function,
    * you can force it to preserve string expression arguments over a copy by wrapping them in `force_copy{}`.
    * See tests/test_tostrexpr.cpp, Method4 for an example.
    * @ru Пример: @en Example @~
    * ```cpp
    *  std::string t = e_concat("", text, " = ", count, " times.");
    *  ....
    *  std::string t = "msg=" + e_concat(", ", text1, text3, text3, count1, count2);
    * ```
    */
    constexpr e_concat(G&& glue, Arg&& arg, Args&&...args)
        : glue_(to_subst<K>(std::forward<G>(glue)))
        , arg_(to_subst<K>(std::forward<Arg>(arg)))
        , args_(to_subst<K>(std::forward<Args>(args))...) {}

    constexpr size_t length() const noexcept {
        return [this]<size_t...Indexes>(std::index_sequence<Indexes...>) {
            glue_len_ = glue_.length();
            size_t l = arg_.length() + glue_len_ * sizeof...(Args);
            ((l += std::get<Indexes>(args_).length()),...);
            return l;
        }(std::make_index_sequence<sizeof...(Args)>());
    }
    constexpr K* place(K* ptr) const noexcept {
        return [this]<size_t...Indexes>(K* ptr, std::index_sequence<Indexes...>) {
            ptr = (K*)arg_.place((typename std::remove_cvref_t<arg_t>::symb_type*)ptr);
            const K* glueStart = ptr;
            ptr = glue_.place(ptr);
            (
                (
                    ptr = (K*)std::get<Indexes>(args_).place((typename std::remove_cvref_t<std::tuple_element_t<Indexes, store_t>>::symb_type*)ptr),
                    glue_len_ > 0 && Indexes < sizeof...(Args) - 1 ? (ch_traits<K>::copy(ptr, glueStart, glue_len_), ptr += glue_len_) : nullptr
                ),
            ...);
            return ptr;
        }(ptr, std::make_index_sequence<sizeof...(Args)>());
    }
};
// CTAD deducing rule for e_concat
template<typename T, typename ... Args> requires (sizeof...(Args) > 1)
e_concat(T&&, Args&&...) -> e_concat<symb_type_from_src_t<T>, T, Args...>;

struct parse_subst_string_error {
    parse_subst_string_error(const char*){}
};

namespace details {

template<typename K, size_t NParams>
constexpr size_t parse_pattern_string(str_src<K> pattern, auto&& add_part, auto&& add_param) {
    char used_args[NParams] = {0};
    const K* first = pattern.begin(), *last = pattern.end(), *start = first;
    size_t all_len = 0;

    auto find = [](const K* from, const K* last, K s) {
        while (from != last) {
            if (*from == s) {
                break;
            }
            from++;
        }
        return from;
    };
    size_t idx_in_params = 0;

    while (first != last) {
        const K* open_pos = first;
        if (*first != '{') {
            open_pos = find(first, last, '{');

            for (;;) {
                const K* close_pos = find(first, open_pos, '}');
                if (close_pos == open_pos) {
                    unsigned len = open_pos - first;
                    add_part(first - start, len);
                    all_len += len;
                    break;
                }
                ++close_pos;
                if (close_pos == open_pos || *close_pos != '}') {
                    throw parse_subst_string_error{"unescaped }"};
                }
                unsigned len = close_pos - first;
                add_part(first - start, len);
                all_len += len;
                first = ++close_pos;
            }
            if (open_pos == last) {
                break;
            }
        }
        if (++open_pos == last) {
            throw parse_subst_string_error{"unescaped {"};
        }
        if (*open_pos == '}') {
            if (idx_in_params == -1) {
                throw parse_subst_string_error{"already used param ids"};
            }
            if (idx_in_params == NParams) {
                throw parse_subst_string_error{"too many params"};
            }
            used_args[idx_in_params] = 1;
            add_param(idx_in_params++);
            first = open_pos + 1;
        } else if (*open_pos == '{') {
            add_part(open_pos - start, 1);
            all_len++;
            first = open_pos + 1;
        } else {
            if (idx_in_params != 0 && idx_in_params != -1) {
                throw parse_subst_string_error{"already used non id params"};
            }
            idx_in_params = -1;
            const K* end = find(open_pos, last, '}');
            if (end == last) {
                throw parse_subst_string_error{"not found }"};
            }
            auto [p, err, _] = str_src<K>(open_pos, end - open_pos).template to_int<unsigned, true, 10, false, false>();
            if (err != IntConvertResult::Success || p < 1 || p > NParams) {
                throw parse_subst_string_error{"bad param id"};
            }
            used_args[--p] = 1;
            add_param(p);
            first = end + 1;
        }
    }
    for (auto c : used_args) {
        if (!c) {
            throw parse_subst_string_error{"unused param"};
        }
    }
    return all_len;
}

struct portion {
    unsigned start: 16;
    unsigned len: 15;
    unsigned is_param: 1;

    portion() = default;

    constexpr void set_param(unsigned param) {
        if (param >= (1 << 16)) {
            throw parse_subst_string_error{"the parameter id is too large"};
        }
        start = param;
        is_param = 1;
    }
    constexpr void set_part(unsigned from, unsigned l) {
        if (from >= (1 << 16) || len >= (1 << 15)) {
            throw parse_subst_string_error{"the string part is too large"};
        }
        start = from;
        len = l;
        is_param = 0;
    }
};

template<typename K, size_t PtLen, size_t NParams>
struct subst_params {
    const K(&source_)[PtLen];
    unsigned all_len_{};
    unsigned actual_{};
    // The pattern string can be divided into a maximum of this number of portions.
    // "a{}a{}a{}a" - One portion of one symbol from the edge, and two portions for every three symbols
    portion portions_[1 + ((PtLen - 2) * 2 / 3)]{};

    consteval subst_params(const K(&pattern)[PtLen]) : source_(pattern) {
        all_len_ = parse_pattern_string<K, NParams>(pattern,
            [this](unsigned from, unsigned len) {
                portions_[actual_++].set_part(from, len);
            }, [this](unsigned param) {
                portions_[actual_++].set_param(param);
            });
    }
};

} // namespace details

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение, которое подставляет в заданные места в строковом литерале - образце значения переданных строковых выражений.
 * @en @brief String expression that substitutes the values ​​of the passed string expressions into the specified places in a string literal.
 */
template<typename K, size_t PtLen, typename ... Args>
struct e_subst : expr_to_std_string<e_subst<K, PtLen, Args...>> {
    inline static constexpr size_t NParams = sizeof...(Args);
    using symb_type = K;
    using store_t = std::tuple<to_str_exp_t<K, Args>...>;

    const details::subst_params<K, PtLen, NParams>& subst_;
    store_t args_;
    /*!
    * @ru @brief Создает строковое выражение, которое подставляет в заданные места в строковом литерале - образце значения переданных строковых выражений.
    * @param pattern - распарсенная во время компиляции информация составе строки-шаблона, содержит длины текстовых порций и места вставки параметров.
    *   Создается автоматически из переданного строкового литерала.
    * @param args... - аргументы, которые будут подставляться в заданные места образца. Также, как и в `e_concat`, могут быть строковые литералы,
    * строковые выражения, стандартные строки, а также любые типы, для которых есть преобразование в строковое выражение.
    * @details Функция создаёт строковое выражение, которое при материализации генерирует текст из образца, подставляя в места подстановки
    *  значения переданных аргументов. Строка-образец задается строковым литералом, константной времени компиляции.
    *  Места вставки обозначаются либо как `{}`, либо как `{номер}`.
    *  В случае без указания номера, параметры подставляются в переданном в функцию порядке. В случае указания номера, параметры подставляются в соответствии
    *  с указанным порядковым номером. Нумерация параметров начинается с 1. Смешивать параметры без номера и с номером нельзя - используется только
    *  один из вариантов для всех подстановок. В случае указания номеров - один параметр может участвовать в нескольких подстановках.
    *  Все переданные параметры должны участвовать в подстановках. Для вставки самих фигурных скобок они должны удваиваться - `{{`, `}}`.
    *  Строка-образец обрабатывается во время компиляции, и для неё сразу создаётся массив с информацией о вставках - какие части строки копировать
    *  в результат, в какие места вставлять переданные значения.
    *  Функция не является заменой `std::format`, не работает с образцом, задаваемым в рантайм, и не поддерживает каких-либо параметров форматирования
    * подставляемых значений. Все передаваемые аргументы должны сами уметь преобразовывать себя в строковые выражения
    *  (см. @ref ConvertToStrExpr "Конвертация типов в в строковые выражения").
    * @en @brief Creates a string expression that substitutes the values ​​of the passed string expressions into the specified places in a string literal.
    * @param pattern - information parsed during compilation in the template string, containing the lengths of text chunks and places to insert parameters.
    *   Created automatically from the passed string literal.
    * @param args... - arguments that will be inserted into the specified places in the sample. Also, as in `e_concat`, there can be string literals,
    * string expressions, standard strings, as well as any types for which there is a conversion to a string expression.
    * @details The function creates a string expression, which, when materialized, generates text from the sample, substituting it in placeholders
    * values ​​of the passed arguments. The pattern string is specified by a string literal, a compile-time constant.
    * Insertion locations are indicated by either `{}` or `{number}`.
    * In case without spectacle number, the parameters are submitted to the order of order. In case of the index number, the parameters are submitted in accordance with
    * with the specified serial number. The numbering of parameters starts from 1. You cannot mix parameters without a number and with a number - only used
    * one of the options for all substitutions. In the case of specifying numbers, one parameter can participate in several substitutions.
    * All passed parameters must participate in substitutions. To insert curly braces themselves, they must be doubled - `{{`, `}}`.
    * The sample string is processed during compilation, and an array is immediately created for it with information about insertions - which parts of the string to copy
    * in the result, where to insert the passed values.
    * The function is not a replacement for `std::format`, does not work with the sample specified at runtime, and does not support any formatting options
    * for substituted values. All passed arguments must be able to convert themselves to string expressions
    * (see @ref ConvertToStrExpr "Converting types to string expressions").
    * @ru Пример: @en Example: @~
    * ```cpp
    *  lstringu<100> u16t = e_subst(u"Test {} from {}, {}.", from, total, success ? u"success"_ss : u"fail"_ss);
    * ```
    */
    constexpr e_subst(const details::subst_params<K, PtLen, NParams>& subst, Args&&...args)
        : subst_(subst)
        , args_(to_subst<K>(std::forward<Args>(args))...){}

    constexpr size_t length() const noexcept {
        return [this]<size_t...Indexes>(std::index_sequence<Indexes...>) {
            size_t idx = 0;
            size_t expr_length_[NParams] = {};
            ((expr_length_[idx++] = std::get<Indexes>(args_).length()),...);
            size_t l = subst_.all_len_;
            for (idx = 0; idx < subst_.actual_; idx++) {
                if (subst_.portions_[idx].is_param) {
                    l += expr_length_[subst_.portions_[idx].start];
                }
            }
            return l;
        }(std::make_index_sequence<sizeof...(Args)>());
    }
    template<size_t Idx>
    constexpr K* place_idx(K* ptr, size_t idx) const noexcept {
        if (idx == Idx) {
            return (K*)std::get<Idx>(args_).place((typename std::remove_cvref_t<std::tuple_element_t<Idx, store_t>>::symb_type*)ptr);
        }
        if constexpr (Idx < NParams - 1) {
            return place_idx<Idx + 1>(ptr, idx);
        }
        return ptr;
    }
    constexpr K* place(K* ptr) const noexcept {
        for (size_t idx = 0; idx < subst_.actual_; idx++) {
            if (subst_.portions_[idx].is_param) {
                ptr = place_idx<0>(ptr, subst_.portions_[idx].start);
            } else {
                ch_traits<K>::copy(ptr, subst_.source_ + subst_.portions_[idx].start, subst_.portions_[idx].len);
                ptr += subst_.portions_[idx].len;
            }
        }
        return ptr;
    }
};

// CTAD deducing rule for e_subst
template<typename K, size_t N, typename...Args> requires (sizeof...(Args) > 0)
e_subst(const K(&)[N], Args&&...) -> e_subst<K, N, Args...>;

/*!
 * @ingroup StrExprs
 * @ru @brief Строковое выражение, которое подставляет в заданные места в строке-образце, задаваемой в рантайме, значения переданных строковых выражений.
 * @en @brief String expression that substitutes the values ​​of the passed string expressions into the specified positions in the pattern string, specified at runtime.
 */
template<typename K, typename ... Args>
struct e_vsubst : expr_to_std_string<e_vsubst<K, Args...>> {
    inline static constexpr size_t Nparams = sizeof...(Args);
    using symb_type = K;
    using store_t = std::tuple<to_str_exp_t<K, Args>...>;

    details::portion portions_[Nparams * 3];
    std::vector<details::portion> more_portions_;
    store_t args_;
    str_src<K> pattern_;
    size_t all_len_{};
    unsigned actual_{};
    /*!
    * @ru @brief Создает строковое выражение, которое подставляет в заданные места в строке-образце, задаваемой в рантайме, значения переданных строковых выражений.
    * @param pattern - Строковый объект-образец, в который будут подставляться значения переданных аргументов.
    * @param args... - аргументы, которые будут подставляться в заданные места образца. Также, как и в `e_concat`, могут быть строковые литералы,
    * строковые выражения, стандартные строки, а также любые типы, для которых есть преобразование в строковое выражение.
    * @details Функция создаёт строковое выражение, которое при материализации генерирует текст из образца, подставляя в места подстановки
    *  значения переданных аргументов. Строка-образец задается в рантайм, строковым объектом, парсинг которого происходит во время выполнения.
    *  Места вставки обозначаются либо как `{}`, либо как `{номер}`.
    *  В случае без указания номера, параметры подставляются в переданном в функцию порядке. В случае указания номера, параметры подставляются в соответствии
    *  с указанным порядковым номером. Нумерация параметров начинается с 1. Смешивать параметры без номера и с номером нельзя - используется только
    *  один из вариантов для всех подстановок. В случае указания номеров - один параметр может участвовать в нескольких подстановках.
    *  Все переданные параметры должны участвовать в подстановках. В случае ошибки парсинга строки будет выкинуто исключение `parse_subst_string_error`.
    *  Для вставки самих фигурных скобок они должны удваиваться - `{{`, `}}`.
    *  Функция не является заменой `std::vformat`, не работает с образцом, задаваемым в compile-time, и не поддерживает каких-либо параметров форматирования
    * подставляемых значений. Все передаваемые аргументы должны сами уметь преобразовывать себя в строковые выражения
    *  (см. @ref ConvertToStrExpr "Конвертация типов в в строковые выражения").
    * @en @brief Creates a string expression that substitutes the values ​​of the passed string expressions into the specified positions in the pattern string, specified at runtime.
    * @param pattern - the pattern string object into which the values ​​of the passed arguments will be substituted.
    * @param args... - the arguments to be substituted into the specified positions in the pattern. As in `e_concat`, these can be string literals,
    * string expressions, standard strings, and any types that can be converted to string expressions.
    * @details The function creates a string expression that, when materialized, generates text from the pattern, substituting the values ​​of the passed arguments into the
    * substitution positions. The pattern string is specified at runtime by a string object that is parsed at runtime.
    * Insertion points are designated either as `{}` or `{number}`.
    * If no number is specified, parameters are substituted in the order passed to the function. If a number is specified, parameters are substituted according to the
    * specified ordinal number. Parameter numbering starts with 1. You cannot mix parameters with and without numbers; only one of the options is used for all substitutions. If numbers are specified, one parameter can participate in multiple substitutions.
    * All passed parameters must participate in substitutions. If a string parsing error occurs, a `parse_subst_string_error` exception will be thrown.
    * To insert the curly braces themselves, they must be doubled - `{{`, `}}`.
    * This function is not a replacement for `std::vformat`, does not work with the pattern specified at compile-time, and does not support any formatting options for
    * substituted values. All passed arguments must be able to convert themselves to string expressions.
    * (See @ref ConvertToStrExpr "Converting Types to String Expressions").
    * @ru Пример: @en Example: @~
    * ```cpp
    *  std::string_view pattern;
    *  .....
    *  lstringa<100> text = e_vsubst(pattern, from, total, success ? "success"_ss : "fail"_ss);
    * ```
    */
    constexpr e_vsubst(str_src<K> pattern, Args&&...args)
        : pattern_(pattern)
        , args_(to_subst<K>(std::forward<Args>(args))...) {

        all_len_ = details::parse_pattern_string<K, Nparams>(pattern_, [this](unsigned from, unsigned len) {
                if (actual_ < std::size(portions_)) {
                    portions_[actual_++].set_part(from, len);
                } else {
                    if (actual_ == std::size(portions_)) {
                        more_portions_.reserve((pattern_.len - 1) * 2 / 3 - std::size(portions_));
                    }
                    more_portions_.emplace_back().set_part(from, len);
                }
            }, [this](unsigned param) {
                if (actual_ < std::size(portions_)) {
                    portions_[actual_++].set_param(param);
                } else {
                    if (actual_ == std::size(portions_)) {
                        more_portions_.reserve((pattern_.len - 1) * 2 / 3 - std::size(portions_));
                    }
                    more_portions_.emplace_back().set_param(param);
                }
            }
        );
    }
    constexpr size_t length() const noexcept {
        return [this]<size_t...Indexes>(std::index_sequence<Indexes...>) {
            size_t idx = 0;
            size_t expr_length_[Nparams] = {};
            ((expr_length_[idx++] = std::get<Indexes>(args_).length()),...);
            size_t l = all_len_;
            for (idx = 0; idx < actual_; idx++) {
                if (portions_[idx].is_param) {
                    l += expr_length_[portions_[idx].start];
                }
            }
            for (const auto& p : more_portions_) {
                if (p.is_param) {
                    l += expr_length_[p.start];
                }
            }
            return l;
        }(std::make_index_sequence<sizeof...(Args)>());
    }
    template<size_t Idx>
    constexpr K* place_idx(K* ptr, size_t idx) const noexcept {
        if (idx == Idx) {
            return (K*)std::get<Idx>(args_).place((typename std::remove_cvref_t<std::tuple_element_t<Idx, store_t>>::symb_type*)ptr);
        }
        if constexpr (Idx < Nparams - 1) {
            return place_idx<Idx + 1>(ptr, idx);
        }
        return ptr;
    }
    constexpr K* place(K* ptr) const noexcept {
        for (size_t idx = 0; idx < actual_; idx++) {
            if (portions_[idx].is_param) {
                ptr = place_idx<0>(ptr, portions_[idx].start);
            } else {
                ch_traits<K>::copy(ptr, pattern_.symbols() + portions_[idx].start, portions_[idx].len);
                ptr += portions_[idx].len;
            }
        }
        for (const auto& p : more_portions_) {
            if (p.is_param) {
                ptr = place_idx<0>(ptr, p.start);
            } else {
                const K* from = pattern_.symbols() + p.start;
                for (size_t idx = p.len; idx--;) {
                    *ptr++ = *from++;
                }
                //ch_traits<K>::copy(ptr, pattern_.symbols() + p.start, p.len);
                //ptr += p.len;
            }
        }
        return ptr;
    }
};
// CTAD deducing rule for e_vsubst
template<StrSourceNoLiteral T, typename...Args> requires (sizeof...(Args) > 0)
e_vsubst(T&&, Args&&...) -> e_vsubst<symb_type_from_src_t<T>, Args...>;

template<is_one_of_char_v K, bool upper>
struct expr_change_case_ascii : expr_to_std_string<expr_change_case_ascii<K, upper>>{
    using symb_type = K;

    str_src<K> src_;

    template<StrSource S>
    expr_change_case_ascii(S&& s) : src_(std::forward<S>(s)){}

    constexpr size_t length() const noexcept {
        return src_.length();
    }
    constexpr K* place(K* ptr) const noexcept {
        const K* read = src_.str;
        for (size_t l = src_.len; l--;) {
            if constexpr (upper) {
                *ptr++ = makeAsciiUpper(*read++);
            } else {
                *ptr++ = makeAsciiLower(*read++);
            }
        }
        return ptr;
    }
};

/*!
 * @ru @brief Генерирует строку на основе исходной, заменяя все ASCII строчные буквы (a-z) на прописные.
 * @tparam K - тип символов, выводится на основе исходной строки.
 * @details Берёт исходную строку и копирует её, заменяя ASCII строчные буквы (a-z) на прописные.
 *  В качестве исходной строки может браться любой строковый объект.
 * @en @brief Generate a string based on the original one, replacing all ASCII lowercase letters (a-z) with uppercase ones.
 * @tparam K - character type, deduced on the source string.
 * @details Takes the original string and copies it, replacing the ASCII lowercase letters (a-z) with uppercase ones.
 *  Any string object can be taken as the source string.
 * @ru Пример @en Example @~
 * ```cpp
 *  stringa upper = "Upper case version is: '" + e_ascii_upper(source_str) + "'.";
 * ```
 */
template<is_one_of_char_v K>
struct e_ascii_upper : expr_change_case_ascii<K, true> {
    using base = expr_change_case_ascii<K, true>;
    using base::base;
};

template<StrSource S>
e_ascii_upper(S&&) -> e_ascii_upper<symb_type_from_src_t<S>>;

/*!
 * @ru @brief Генерирует строку на основе исходной, заменяя все ASCII прописные буквы (A-Z) на строчные.
 * @tparam K - тип символов, выводится на основе исходной строки.
 * @details Берёт исходную строку и копирует её, заменяя ASCII прописные буквы (A-Z) на строчные.
 *  В качестве исходной строки может браться любой строковый объект.
 * @en @brief Generate a string based on the original one, replacing all ASCII uppercase letters (A-Z) with lowercase ones.
 * @tparam K - character type, deduced on the source string.
 * @details Takes the original string and copies it, replacing the ASCII uppercase letters (A-Z) with lowercase ones.
 *  Any string object can be taken as the source string.
 * @ru Пример @en Example @~
 * ```cpp
 *  stringa lower = "Lower case version is: '" + e_ascii_lower(source_str) + "'.";
 * ```
 */
template<is_one_of_char_v K>
struct e_ascii_lower : expr_change_case_ascii<K, false> {
    using base = expr_change_case_ascii<K, false>;
    using base::base;
};

template<StrSource S>
e_ascii_lower(S&&) -> e_ascii_lower<symb_type_from_src_t<S>>;

/*!
 * @ru @brief Небольшое пространство для методов работы со стандартными строками.
 * @en @brief Small namespace for standard string methods.
 */
namespace str {

/*!
 * @ru @brief Изменить часть стандартной строки на заданное строковое выражение.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param from - начальная позиция замены.
 * @param count - количество символов, подлежащих замене.
 * @param expr - строковое выражение для замены.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, при необходимости увеличивает размер строки,
 *  и материализует строковое выражение на нужное место, без использования промежуточных буферов.
 *  При увеличении размера строки - до C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief Replace a portion of a standard string with the given string expression.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param from - starting position of replacement.
 * @param count - number of characters to replace.
 * @param expr - string expression to replace.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the passed string.
 * @details The method gets the length of the string expression, increases the string size if necessary,
 *  and materializes the string expression at the desired location, without using intermediate buffers.
 *  When increasing the string size, resize is used before C++23; resize_and_overwrite is used starting with C++23.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 *
 */
template<typename K, typename A, StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& change(std::basic_string<K, std::char_traits<K>, A>& str, size_t from, size_t count, const E& expr) {
    size_t expr_length = expr.length();
    if (!expr_length) {
        str.erase(from, count);
        return str;
    }
    size_t str_length = str.length();
    if (from > str_length) {
        from = str_length;
    }
    if (from + count > str_length) {
        count = str_length - from;
    }
    size_t new_length = str_length - count + expr_length;
    size_t tail_length = str_length - count - from;

    if (new_length <= str_length) {
        K* data = str.data();
        expr.place((typename E::symb_type*)data + from);
        if (expr_length < count) {
            if (tail_length) {
                std::char_traits<K>::move(data + from + expr_length, data + from + count, tail_length);
            }
            str.resize(new_length);
        }
    } else {
        auto fill = [&](K* data, size_t) -> size_t {
            if (tail_length) {
                std::char_traits<K>::move(data + from + expr_length, data + from + count, tail_length);
            }
            expr.place((typename E::symb_type*)data + from);
            return new_length;
        };
        if constexpr (requires{str.resize_and_overwrite(new_length, fill);}) {
            str.resize_and_overwrite(new_length, fill);
        } else if constexpr (requires{str._Resize_and_overwrite(new_length, fill);}) {
            str._Resize_and_overwrite(new_length, fill);
        } else {
            str.resize(new_length);
            fill(str.data(), 0);
        }
    }
    return str;
}
/*!
 * @ru @brief Добавить к стандартной строке строковое выражение.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param expr - строковое выражение для вставки.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, увеличивает размер строки,
 *  и материализует строковое выражение за концом строки, без использования промежуточных буферов.
 *  До C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief Append a string expression to a standard string.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the passed string.
 * @details The method gets the length of the string expression, increases the size of the string,
 *  and materializes the string expression beyond the end of the string, without using intermediate buffers.
 *  Before C++23, resize was used; since C++23, resize_and_overwrite was used.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 *
 */
template<typename K, typename A, StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& append(std::basic_string<K, std::char_traits<K>, A>& str, const E& expr) {
    return change(str, str.length(), 0, expr);
}

/*!
 * @ru @brief Вставить строковое выражение в начало стандартной строки.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param expr - строковое выражение для вставки.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, увеличивает размер строки, сдвигает её содержимое
 *  и материализует строковое выражение в её начало, без использования промежуточных буферов.
 *  До C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief Insert a string expression at the beginning of a standard string.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the passed string.
 * @details The method gets the length of the string expression, increases the size of the string, shifts its contents
 *  and materializes the string expression at the beginning of the string, without using intermediate buffers.
 *  Before C++23, resize was used; since C++23, resize_and_overwrite was used.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 *
 */
template<typename K, typename A, StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& prepend(std::basic_string<K, std::char_traits<K>, A>& str, const E& expr) {
    return change(str, 0, 0, expr);
}

/*!
 * @ru @brief Вставить строковое выражение в указанную позицию стандартной строки.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param from - позиция вставки.
 * @param expr - строковое выражение для вставки.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, увеличивает размер строки, сдвигает её содержимое
 *  и материализует строковое выражение на нужное место, без использования промежуточных буферов.
 *  До C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief Insert a string expression at the specified position in a standard string.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param from - insertion position.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - a reference to the passed string.
 * @details The method gets the length of the string expression, increases the size of the string, shifts its contents
 * and materializes the string expression to the desired location, without using intermediate buffers.
 * Before C++23, resize was used; since C++23, resize_and_overwrite was used.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 *
 */
template<typename K, typename A, StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& insert(std::basic_string<K, std::char_traits<K>, A>& str, size_t from, const E& expr) {
    return change(str, from, 0, expr);
}

/*!
 * @ru @brief Переписать всю строку заданным строковым выражением, при необходимости изменив размер строки.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param expr - строковое выражение для вставки.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод заменяет содержимое строки результатом строкового выражения.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief Rewrite the entire string with the given string expression, resizing the string if necessary.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - a reference to the passed string.
 * @details The method replaces the contents of a string with the result of a string expression.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 *
 */
template<typename K, typename A, StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& overwrite(std::basic_string<K, std::char_traits<K>, A>& str, const E& expr) {
    if (size_t expr_length = expr.length()) {
        if (expr_length <= str.length()) {
            K* data = str.data();
            expr.place((typename E::symb_type*)data);
            str.resize(expr_length);
        } else {
            auto fill = [&](K* data, size_t) -> size_t {
                expr.place((typename E::symb_type*)data);
                return expr_length;
            };
            if constexpr (requires{str.resize_and_overwrite(expr_length, fill);}) {
                str.resize_and_overwrite(expr_length, fill);
            } else if constexpr (requires{str._Resize_and_overwrite(expr_length, fill);}) {
                str._Resize_and_overwrite(expr_length, fill);
            } else {
                str.resize(expr_length);
                expr.place((typename E::symb_type*)str.data());
            }
        }
    } else {
        str.clear();
    }
    return str;
}

namespace details {

template<typename K, typename A, typename E>
struct replace_grow_helper {
    using my_type = std::basic_string<K, std::char_traits<K>, A>;

    replace_grow_helper(my_type& src, str_src<K> p, const K* r, size_t rl, size_t mc, size_t d, const E& e)
        : str(src), source(src), pattern(p), repl(const_cast<K*>(r)), replLen(rl), maxCount(mc), delta(d), expr(e) {}
    my_type& str;

    const str_src<K> source;
    const str_src<K> pattern;
    K* repl;
    const size_t replLen;

    size_t maxCount;
    const size_t delta;
    size_t all_delta{};
    const E& expr;

    K* reserve_for_copy{};
    size_t end_of_piece{};
    size_t total_length{};

    std::optional<my_type> dst;

    void replace(size_t offset) {
        size_t found[16] = {offset};
        maxCount--;

        offset += pattern.len;
        all_delta += delta;
        size_t idx = 1;
        for (; idx < std::size(found) && maxCount > 0; idx++, maxCount--) {
            found[idx] = source.find(pattern, offset);
            if (found[idx] == npos) {
                break;
            }
            offset = found[idx] + pattern.len;
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
            my_type* dst_str{};
            if (total_length <= str.capacity()) {
                // Строка поместится в старое место | The line will be placed in the old location.
                dst_str = &str;
            } else {
                // Будем создавать в другом буфере | We will create in another buffer.
                dst_str = &dst.emplace();
            }
            auto fill = [this](K* p, size_t) -> size_t {
                reserve_for_copy = p;
                return total_length;
            };
            if constexpr (requires{dst_str->_Resize_and_overwrite(total_length, fill);}) {
                dst_str->_Resize_and_overwrite(total_length, fill);
            } else if constexpr (requires{dst_str->resize_and_overwrite(total_length, fill);}) {
                dst_str->resize_and_overwrite(total_length, fill);
            } else {
                dst_str->resize(total_length);
                reserve_for_copy = dst_str->data();
            }
        }
        const K* src_start = str.c_str();
        while(idx-- > 0) {
            size_t pos = found[idx] + pattern.len;
            size_t lenOfPiece = end_of_piece - pos;
            ch_traits<K>::move(reserve_for_copy + pos + all_delta, src_start + pos, lenOfPiece);
            if constexpr (std::is_same_v<E, int>) {
                ch_traits<K>::copy(reserve_for_copy + pos + all_delta - replLen, repl, replLen);
            } else {
                if (!repl) {
                    repl = reserve_for_copy + pos + all_delta - replLen;
                    expr.place(repl);
                } else {
                    ch_traits<K>::copy(reserve_for_copy + pos + all_delta - replLen, repl, replLen);
                }
            }
            all_delta -= delta;
            end_of_piece = found[idx];
        }
        if (!all_delta && reserve_for_copy != src_start) {
            ch_traits<K>::copy(reserve_for_copy, src_start, found[0]);
            str = std::move(*dst);
        }
    }
};

} // namespace details

/*!
 * @ru @brief Функция поиска подстрок в стандартной строке и замены найденных вхождений на значение строкового выражения.
 * @tparam K - тип символов строки.
 * @tparam A - тип аллокатора строки.
 * @tparam E - тип строкового выражения.
 * @tparam T - тип подстроки поиска.
 * @param str - строка, в которой заменяем вхождения подстрок.
 * @param pattern - искомая подстрока (любой тип, конвертирующийся в simple_str).
 * @param repl - строковое выражение для замены.
 * @param offset - начальное смещение для поиска.
 * @param max_count - максимальное количество замен.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на модифицируемую строку.
 * @details Части строкового выражения не должны ссылаться на саму модифицируемую строку.
 *  Если искомая подстрока не найдена, то строковое выражение даже не вычисляется.
 *  Затем при осуществлении замены строковое выражение вычисляется только один раз в место первой замены,
 *  а в следующие места замен просто копируется символы из первого места. Это позволяет экономить память
 *  и время, если вам надо сделать замену на какую-либо "сборную" строку.
 * @en @brief A function for searching for substrings in a standard string and replacing the found occurrences with a string expression.
 * @tparam K - the string character type.
 * @tparam A - the string allocator type.
 * @tparam E - the string expression type.
 * @tparam T - the search substring type.
 * @param str - the string in which to replace substring occurrences.
 * @param pattern - the search substring (any type convertible to simple_str).
 * @param repl - the string expression to replace.
 * @param offset - the starting offset for the search.
 * @param max_count - the maximum number of replacements.
 * @return std::basic_string<K, std::char_traits<K>, A>& - a reference to the string being modified.
 * @details Parts of the string expression must not reference the string being modified itself.
 * If the search substring is not found, the string expression is not even evaluated.
 * Then, when performing a replacement, the string expression is evaluated only once at the first replacement location,
 * and characters from the first location are simply copied to subsequent replacement locations. This saves memory
 * and time if you need to replace with some kind of "composite" string.
 */
template<typename K, typename A, StrExprForType<K> E, typename T>
requires (std::is_constructible_v<str_src<K>, T>)
std::basic_string<K, std::char_traits<K>, A>& replace(std::basic_string<K, std::char_traits<K>, A>& str, T&& pattern, const E& repl, size_t offset = 0, size_t max_count = -1) {
    if (!max_count) {
        return str;
    }
    str_src<K> src = str;
    str_src<K> spattern{std::forward<T>(pattern)};
    offset = src.find(pattern, offset);
    if (offset == npos) {
        return str;
    }
    size_t replLen = repl.length();
    K* replStart{};
    if (spattern.len == replLen) {
        // Заменяем inplace на подстроку такой же длины
        // Replace inplace with a substring of the same length
        K* ptr = str.data();
        replStart = ptr + offset;
        repl.place(replStart);

        while (--max_count) {
            offset = src.find(spattern, offset + replLen);
            if (offset == npos)
                break;
            ch_traits<K>::copy(ptr + offset, replStart, replLen);
        }
    } else if (spattern.len > replLen) {
        // Заменяем на более короткий кусок, длина текста уменьшится, идём слева направо
        // Replace with a shorter piece, the length of the text will decrease, go from left to right
        K* ptr = str.data();
        replStart = ptr + offset;
        repl.place(replStart);
        size_t posWrite = offset + replLen;
        offset += spattern.len;

        while (--max_count) {
            size_t idx = src.find(spattern, offset);
            if (idx == npos)
                break;
            size_t lenOfPiece = idx - offset;
            ch_traits<K>::move(ptr + posWrite, ptr + offset, lenOfPiece);
            posWrite += lenOfPiece;
            ch_traits<K>::copy(ptr + posWrite, replStart, replLen);
            posWrite += replLen;
            offset = idx + spattern.len;
        }
        size_t tailLen = src.len - offset;
        ch_traits<K>::move(ptr + posWrite, ptr + offset, tailLen);
        str.resize(posWrite + tailLen);
    } else {
        details::replace_grow_helper<K, A, E>(str, spattern, nullptr, replLen, max_count, replLen - spattern.len, repl).replace(offset);
    }
    return str;
}

/*!
 * @ru @brief Функция поиска подстрок в стандартной строке и замены найденных вхождений на другую подстроку.
 * @tparam K - тип символов строки.
 * @tparam A - тип аллокатора строки.
 * @param str - строка, в которой заменяем вхождения подстрок.
 * @param pattern - искомая подстрока (любой тип, конвертирующийся в simple_str).
 * @param repl - строка для замены (любой тип, конвертирующийся в simple_str).
 * @param offset - начальное смещение для поиска.
 * @param max_count - максимальное количество замен.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на модифицируемую строку.
 * @en @brief Function for searching for substrings in a standard string and replacing found occurrences with another substring.
 * @tparam K - the character type of the string.
 * @tparam A - the type of the string allocator.
 * @param str - the string in which we replace occurrences of substrings.
 * @param pattern - the searched substring (any type that converts to simple_str).
 * @param repl - replacement string (any type convertible to simple_str).
 * @param offset - the starting offset for the search.
 * @param max_count - maximum number of replacements.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the modified string.
 */
template<typename K, typename A>
std::basic_string<K, std::char_traits<K>, A>& replace(std::basic_string<K, std::char_traits<K>, A>& str, str_src<K> pattern, str_src<K> repl, size_t offset = 0, size_t max_count = -1) {
    if (!max_count) {
        return str;
    }
    str_src<K> src = str;
    offset = src.find(pattern, offset);
    if (offset == npos) {
        return str;
    }
    if (pattern.len == repl.len) {
        // Заменяем inplace на подстроку такой же длины
        // Replace inplace with a substring of the same length
        K* ptr = str.data();
        while (max_count--) {
            ch_traits<K>::copy(ptr + offset, repl.str, repl.len);
            offset = src.find(pattern, offset + repl.len);
            if (offset == npos)
                break;
        }
    } else if (pattern.len > repl.len) {
        // Заменяем на более короткий кусок, длина текста уменьшится, идём слева направо
        // Replace with a shorter piece, the length of the text will decrease, go from left to right
        K* ptr = str.data();
        ch_traits<K>::copy(ptr + offset, repl.str, repl.len);
        size_t posWrite = offset + repl.len;
        offset += pattern.len;

        while (--max_count) {
            size_t idx = src.find(pattern, offset);
            if (idx == npos)
                break;
            size_t lenOfPiece = idx - offset;
            ch_traits<K>::move(ptr + posWrite, ptr + offset, lenOfPiece);
            posWrite += lenOfPiece;
            ch_traits<K>::copy(ptr + posWrite, repl.str, repl.len);
            posWrite += repl.len;
            offset = idx + pattern.len;
        }
        size_t tailLen = src.len - offset;
        ch_traits<K>::move(ptr + posWrite, ptr + offset, tailLen);
        str.resize(posWrite + tailLen);
    } else {
        details::replace_grow_helper<K, A, int>(str, pattern, repl.str, repl.len, max_count, repl.len - pattern.len, 0).replace(offset);
    }
    return str;
}

template<typename K, typename A, typename T, typename M>
requires (std::is_constructible_v<str_src<K>, T> && std::is_constructible_v<str_src<K>, M>)
std::basic_string<K, std::char_traits<K>, A>& replace(std::basic_string<K, std::char_traits<K>, A>& str, T&& pattern, M&& repl, size_t offset = 0, size_t max_count = -1) {
    return replace(str, str_src<K>{std::forward<T>(pattern)}, str_src<K>{std::forward<M>(repl)}, offset, max_count);
}

/*!
 * @ru @brief Изменить строчные ASCII символы строки (a-z) на прописные.
 * @param str - стандартная строка
 * @param from - позиция начала замены, по умолчанию 0.
 * @param to - по какую позицию (не включительно) заменять (по умолчанию до каонца строки).
 * @return переданную строку
 * @en @brief Change lowercase ASCII string characters (a-z) to uppercase.
 * @param str - standard string
 * @param from - replacement start position, default 0.
 * @param to - at what position (not inclusive) to replace (by default until the end of the line).
 * @return the passed string
 */
template<typename K, typename A>
std::basic_string<K, std::char_traits<K>, A>& make_ascii_upper(std::basic_string<K, std::char_traits<K>, A>& str, size_t from = 0, size_t to = -1) {
    for (K *ptr = str.data() + std::min(from, str.length()), *end = str.data() + std::min(to, str.length()); ptr < end; ptr++) {
        K s = *ptr;
        if (isAsciiLower(s)) {
            *ptr = s & ~0x20;
        }
    }
    return str;
}

/*!
 * @ru @brief Изменить прописные ASCII символы строки (A-Z) на строчные.
 * @param str - стандартная строка
 * @param from - позиция начала замены, по умолчанию 0.
 * @param to - по какую позицию (не включительно) заменять (по умолчанию до каонца строки).
 * @return переданную строку
 * @en @brief Change uppercase ASCII string characters (A-Z) to lowercase.
 * @param str - standard string
 * @param from - replacement start position, default 0.
 * @param to - at what position (not inclusive) to replace (by default until the end of the line).
 * @return the passed string
 */
template<typename K, typename A>
std::basic_string<K, std::char_traits<K>, A>& make_ascii_lower(std::basic_string<K, std::char_traits<K>, A>& str, size_t from = 0, size_t to = -1) {
    for (K *ptr = str.data() + std::min(from, str.length()), *end = str.data() + std::min(to, str.length()); ptr < end; ptr++) {
        K s = *ptr;
        if (isAsciiUpper(s)) {
            *ptr = s | 0x20;
        }
    }
    return str;
}

} // namespace str

} // namespace simstr

/*!
 * @ru @brief Некоторые методы для работы с стандартными строками.
 * @en @brief Some methods for working with standard strings.
 */
namespace std {
/*!
 * @ingroup StrExprs
 * @ru @brief Унарный оператор+ для преобразования стандартных строк в строковые выражения.
 * @details Стандартные строки могут напрямую участвовать в строковых выражениях только когда
 * другой операнд тоже является строковым выражением. Если другой операнд - не строковое выражение,
 * используйте этот оператор, чтобы превратить `std::basic_string` или `std::basic_string_view`
 * в строковое выражение.
 * @en @brief Unary operator + for converting standard strings to string expressions.
 * @details Standard strings can only participate directly in string expressions when
 * the other operand is also a string expression. If the other operand is not a string expression,
 * use this operator to turn `std::basic_string` or `std::basic_string_view`
 * into string expression.
 * @ru Пример: @en Example @~
 * ```cpp
 *  std::string make_text(const std::string& text, int count, std::string_view what, std::string_view what_p = ""sv) {
 *      return +text + " " + count + " " + e_choice(what_p.empty(), what + e_if(count > 1, "s"), e_choice(count > 1, +what_p, +what));
 *  }
 * ```
 */
template<simstr::StdStrSource T>
simstr::expr_stdstr<typename T::value_type, T> operator+(const T& str) {
    return {str};
}

/*!
 * @ru @brief Оператор для добавления строкового выражения к стандартной строке.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param expr - строковое выражение для добавления.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, увеличивает размер строки,
 *  и материализует строковое выражение за концом строки, без использования промежуточных буферов.
 *  До C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief An operator for appending a string expression to a standard string.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the passed string.
 * @details The method gets the length of the string expression, increases the size of the string,
 *  and materializes the string expression beyond the end of the string, without using intermediate buffers.
 *  Before C++23, resize was used; since C++23, resize_and_overwrite was used.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 */
template<typename K, typename A, simstr::StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& operator |=(std::basic_string<K, std::char_traits<K>, A>& str, const E& expr) {
    return simstr::str::change(str, str.length(), 0, expr);
}

/*!
 * @ru @brief Оператор для вставки строкового выражения в начало стандартной строки.
 * @tparam K - тип символов.
 * @tparam A - тип аллокатора.
 * @tparam E - тип строкового выражения.
 * @param str - строка.
 * @param expr - строковое выражение для вставки.
 * @return std::basic_string<K, std::char_traits<K>, A>& - ссылку на переданную строку.
 * @details Метод получает у строкового выражения его длину, увеличивает размер строки, сдвигает её содержимое
 *  и материализует строковое выражение в её начало, без использования промежуточных буферов.
 *  До C++23 используется resize, начиная с C++23 - resize_and_overwrite.
 *
 * ВАЖНО!!! части строкового выражения не должны ссылаться на саму строку, иначе результат неопределён!!!
 *
 * @en @brief An operator for inserting a string expression at the beginning of a standard string.
 * @tparam K - character type.
 * @tparam A - allocator type.
 * @tparam E - string expression type.
 * @param str - string.
 * @param expr - string expression to insert.
 * @return std::basic_string<K, std::char_traits<K>, A>& - reference to the passed string.
 * @details The method gets the length of the string expression, increases the size of the string, shifts its contents
 *  and materializes the string expression at the beginning of the string, without using intermediate buffers.
 *  Before C++23, resize was used; since C++23, resize_and_overwrite was used.
 *
 * IMPORTANT!!! Parts of a string expression must not reference the string itself, otherwise the result is undefined!!!
 */
template<typename K, typename A, simstr::StrExprForType<K> E>
std::basic_string<K, std::char_traits<K>, A>& operator ^=(std::basic_string<K, std::char_traits<K>, A>& str, const E& expr) {
    return simstr::str::change(str, 0, 0, expr);
}

} // namespace std
