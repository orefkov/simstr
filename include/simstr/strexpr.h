/*
 * ver. 1.3.0
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * База для строковых конкатенаций через выражения времени компиляции
 * (c) Project "SimStr", Aleksandr Orefkov orefkov@gmail.com
 * Base for string concatenations via compile-time expressions
 */
#pragma once
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <concepts>
#include <utility>

/*!
 * @ru @brief Пространство имён для объектов библиотеки
 * @en @brief Library namespace
 */
namespace simstr {

// Выводим типы для 16 и 32 битных символов в зависимости от размера wchar_t
// Infer types for 16 and 32 bit characters depending on the size of wchar_t
inline constexpr bool wchar_is_u16 = sizeof(wchar_t) == 2;

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
using uws = wchar_t;
using u16s = char16_t;
using u32s = char32_t;

using uu8s = std::make_unsigned<u8s>::type;

template<typename K>
inline constexpr bool is_one_of_char_v = std::is_same_v<K, u8s> || std::is_same_v<K, wchar_t> || std::is_same_v<K, u16s> || std::is_same_v<K, u32s>;

template<typename K>
inline constexpr bool is_one_of_std_char_v = std::is_same_v<K, u8s> || std::is_same_v<K, wchar_t> || std::is_same_v<K, wchar_type>;

template<typename From>
requires (is_one_of_std_char_v<From>)
auto to_one_of_std_char(From* from) {
    if constexpr (std::is_same_v<From, u8s> || std::is_same_v<From, wchar_t>) {
        return from;
    } else {
        return from_w(from);
    }
}
template<typename From>
requires (is_one_of_std_char_v<From>)
auto to_one_of_std_char(const From* from) {
    if constexpr (std::is_same_v<From, u8s> || std::is_same_v<From, wchar_t>) {
        return from;
    } else {
        return from_w(from);
    }
}

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
template<typename T, size_t N>
    requires(is_one_of_char_v<T>)
struct const_lit<const T(&)[N]> {
    using symb_type = T;
    constexpr static size_t Count = N;
};

// Тут ещё дополнительно ограничиваем тип литерала
// Here we further restrict the type of the literal
template<typename K, typename T> struct const_lit_for;

template<typename K, size_t N>
    requires(is_one_of_char_v<K>)
struct const_lit_for<K, const K(&)[N]> {
    constexpr static size_t Count = N;
};

template<typename K, size_t N>
class const_lit_to_array {

    template<size_t Idx>
    size_t find(K s) const {
        if constexpr (Idx < N) {
            return s == symbols_[Idx] ? Idx : find<Idx + 1>(s);
        }
        return -1;
    }

    template<size_t Idx>
    bool exist(K s) const {
        if constexpr (Idx < N) {
            return s == symbols_[Idx] || exist<Idx + 1>(s);
        }
        return false;
    }
public:
    const K (&symbols_)[N + 1];

    template<typename T, size_t M = const_lit_for<K, T>::Count> requires (M == N + 1)
    constexpr const_lit_to_array(T&& s)
        : symbols_(s) {}

    constexpr bool contain(K s) const {
        return exist<0>(s);
    }
    constexpr size_t index_of(K s) const {
        return find<0>(s);
    }
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
concept StrType = requires(const A& a) {
    { a.is_empty() } -> std::same_as<bool>;
    { a.length() } -> std::convertible_to<size_t>;
    { a.symbols() } -> std::same_as<const K*>;
} && std::is_same_v<typename std::remove_cvref_t<A>::symb_type, K>;

/*!
 * @ru @defgroup StrExprs Строковые выражения
 * @brief Описание строковых выражений
 * @details Все типы владеющих строк могут инициализироваться с помощью "строковых выражений"
 *  (по сути это вариант https://en.wikipedia.org/wiki/Expression_templates для строк).
 *  Строковое выражение - это объект произвольного типа, у которого имеются методы:
 *  - `size_t length() const`: выдает длину строки
 *  - `K* place(K*) const`: скопировать символы строки в предназначенный буфер и вернуть указатель за последним символом
 *  - `typename symb_type`: показывает, с каким типом символов он работает.
 *
 *  При инициализации строковый объект запрашивает у строкового выражения его размер, выделяет необходимую память,
 *  и передает память строковому выражению, которое помещает символы в отведённый буфер.
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
 *    - `e_repl(ИсходнаяСтрока, "Искать", "Заменять")`: заменяет в исходной строке вхождения "Искать" на "Заменять".
 *        Шаблоны поиска и замены - строковые литералы времени компиляции.
 *    - `expr_replaced< ТипСимвола>{ИсходнаяСтрока, Искать, Заменять}`: заменяет в исходной строке вхождения Искать на Заменять.
 *         Шаблоны поиска и замены - могут быть любыми строковыми объектами в рантайме.
 *  и т.д. и т.п.
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
 *      - `e_repl(SourceString, "Search", "Replace")`: replaces occurrences of "Search" with "Replace" in the source string.
 *         Find and replace patterns are compile-time string literals.
 *      - `expr_replaced< CharacterType>{SourceString, Search, Replace}`: replaces occurrences of Search with Replace in the source string.
 *         Search and replace patterns - can be any string objects at runtime.
 * etc. etc.
 */

/*!
 * @ingroup StrExprs
 * @ru @brief Концепт "Строковых выражений"
 * @details Это концепт, проверяющий, является ли тип "строковым выражением".
 * @en @brief Concept of "String Expressions"
 * @details This is a concept that checks whether a type is a "string expression".
 */
template<typename A>
concept StrExpr = requires(const A& a) {
    typename A::symb_type;
    { a.length() } -> std::convertible_to<size_t>;
    { a.place(std::declval<typename A::symb_type*>()) } -> std::same_as<typename A::symb_type*>;
};

/*!
 * @ingroup StrExprs
 * @ru @brief Концепт строкового выражения заданного типа символов
 * @tparam A - проверяемый тип
 * @tparam K - проверяемый тип символов
 * @details Служит для задания ограничения к строковому выражению по типу символов
 * @en @brief The concept of a string expression of a given character type
 * @tparam A - type being checked
 * @tparam K - character type to be checked
 * @details Used to set restrictions on a string expression by character type
 */
template<typename A, typename K>
concept StrExprForType = StrExpr<A> && std::is_same_v<K, typename A::symb_type>;

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
struct strexprjoin {
    using symb_type = typename A::symb_type;
    const A& a;
    const B& b;
    constexpr strexprjoin(const A& a_, const B& b_) : a(a_), b(b_){}
    constexpr size_t length() const noexcept {
        return a.length() + b.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        return b.place(a.place(p));
    }
    constexpr symb_type* len_and_place(symb_type* p) const noexcept {
        a.length();
        b.length();
        return place(p);
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
inline auto operator+(const A& a, const B& b) {
    return strexprjoin<A, B>{a, b};
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
struct strexprjoin_c {
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
            return b.place(a.place(p));
        } else {
            return a.place(b.place(p));
        }
    }
    constexpr symb_type* len_and_place(symb_type* p) const noexcept {
        a.length();
        b.length();
        return place(p);
    }
};

template<typename T, typename K = void, typename... Types>
struct is_one_of_type {
    static constexpr bool value = std::is_same_v<T, K> || is_one_of_type<T, Types...>::value;
};
template<typename T>
struct is_one_of_type<T, void> : std::false_type {};

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
 *  result = shost + e_choice(sserv.is_empty(), eea, ":" + sserv);
 *  ```
 *
 *  ```cpp
 *  result += "E" + e_choice(adjusted_exponent > 0, "+"_ss, eea) + adjusted_exponent;
 *  ```
 */
template<typename K>
struct empty_expr {
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
struct expr_char {
    using symb_type = K;
    K value;
    expr_char(K v) : value(v){}
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
constexpr inline auto operator+(const A& a, K s) {
    return strexprjoin_c<A, expr_char<K>>{a, s};
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
constexpr inline auto e_char(K s) {
    return expr_char<K>{s};
}

template<typename K, size_t N>
struct expr_literal {
    using symb_type = K;
    const K (&str)[N + 1];
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
constexpr inline auto e_t(T&& s) {
    return expr_literal<typename const_lit<T>::symb_type, static_cast<size_t>(N - 1)>{s};
}

template<bool first, typename K, size_t N, typename A>
struct expr_literal_join {
    using symb_type = K;
    const K (&str)[N + 1];
    const A& a;
    constexpr size_t length() const noexcept {
        return N + a.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        if constexpr (N != 0) {
            if constexpr (first) {
                std::char_traits<K>::copy(p, str, N);
                return a.place(p + N);
            } else {
                p = a.place(p);
                std::char_traits<K>::copy(p, str, N);
                return p + N;
            }
        } else {
            return a.place(p);
        }
    }
    constexpr symb_type* len_and_place(symb_type* p) const noexcept {
        a.length();
        return place(p);
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для строкового выражения и строкового литерала такого же типа символов.
 * @return Строковое выражение, объединяющее операнды.
 * @en @brief The addition operator for a string expression and a string literal of the same character type.
 * @return A string expression concatenating the operands.
 */
template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(const A& a, T&& s) {
    return expr_literal_join<false, K, (N - 1), A>{s, a};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для строкового литерала такого же типа символов и строкового выражения.
 * @return Строковое выражение, объединяющее операнды.
 * @en @brief The addition operator for a string literal of the same character type and string expression.
 * @return A string expression concatenating the operands.
 */
template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(T&& s, const A& a) {
    return expr_literal_join<true, K, (N - 1), A>{s, a};
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
struct expr_spaces {
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
constexpr inline auto e_spca() {
    return expr_spaces<u8s, N>();
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
constexpr inline auto e_spcw() {
    return expr_spaces<uws, N>();
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
struct expr_pad {
    using symb_type = K;
    size_t len;
    K s;
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
constexpr inline auto e_c(size_t l, K s) {
    return expr_pad<K>{ l, s };
}

template<typename K, size_t N>
struct expr_repeat_lit {
    using symb_type = K;
    size_t repeat_;
    const K (&s)[N + 1];
    constexpr size_t length() const noexcept {
        return N * repeat_;
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        for (size_t i = 0; i < repeat_; i++) {
            std::char_traits<K>::copy(p, s, N);
            p += N;
        }
        return p;
    }
};

template<StrExpr A>
struct expr_repeat_expr {
    using symb_type = typename A::symb_type;
    size_t repeat_;
    const A& expr_;
    constexpr size_t length() const noexcept {
        return repeat_ * expr_.length();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        for (size_t i = 0; i < repeat_; i++) {
            p = expr_.place(p);
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
constexpr inline auto e_repeat(T&& s, size_t l) {
    return expr_repeat_lit<K, M - 1>{ l, s };
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
constexpr inline auto e_repeat(const A& s, size_t l) {
    return expr_repeat_expr<A>{ l, s };
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
struct expr_if {
    using symb_type = typename A::symb_type;
    using my_type = expr_if<A>;
    const A& a;
    bool choice;

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
template<StrExpr A, size_t N, bool Compare>
struct expr_choice_one_lit {
    using symb_type = typename A::symb_type;
    const symb_type (&str)[N + 1];
    const A& a;
    bool choice;

    constexpr size_t length() const noexcept {
        return choice == Compare ? a.length() : N;
    }
    constexpr symb_type* place(symb_type* ptr) const noexcept {
        if (choice == Compare) {
            return a.place(ptr);
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
template<typename K, size_t N, size_t M>
struct expr_choice_two_lit {
    using symb_type = K;
    const symb_type (&str_a)[N + 1];
    const symb_type (&str_b)[M + 1];
    bool choice;

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
            std::char_traits<symb_type>::copy(ptr, str_b, M);
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
inline constexpr auto e_choice(bool c, const A& a, const B& b) {
    return expr_choice<A, B>{a, b, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда третий аргумент - строковый литерал.
 * @en @brief Overload e_choice when the third argument is a string literal.
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
inline constexpr auto e_choice(bool c, const A& a, T&& str) {
    return expr_choice_one_lit<A, N - 1, true>{str, a, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда второй аргумент - строковый литерал.
 * @en @brief Overload e_choice when the second argument is a string literal.
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
inline constexpr auto e_choice(bool c, T&& str, const A& a) {
    return expr_choice_one_lit<A, N - 1, false>{str, a, c};
}
/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_choice, когда второй и третий аргумент - строковые литералы.
 * @en @brief Overload e_choice when the second and third arguments are string literals.
 */
template<typename T, typename L, size_t N = const_lit<T>::Count, size_t M = const_lit_for<typename const_lit<T>::symb_type, L>::Count>
inline constexpr auto e_choice(bool c, T&& str_a, L&& str_b) {
    return expr_choice_two_lit<typename const_lit<T>::symb_type, N -1, M - 1>{str_a, str_b, c};
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
inline constexpr auto e_if(bool c, const A& a) {
    return expr_if<A>{a, c};
}
/*!
 * @ingroup StrExprs
 * @ru @brief Перегрузка e_if, когда второй аргумент - строковый литерал.
 * @en @brief Overload e_if when the second argument is a string literal.
 */
template<typename T, size_t N = const_lit<T>::Count>
inline constexpr auto e_if(bool c, T&& str) {
    const typename const_lit<T>::symb_type empty[1] = {0};
    return expr_choice_two_lit<typename const_lit<T>::symb_type, N - 1, 0>{str, empty, c};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Тип для использования std::string и std::string_view как источников в строковых выражениях.
 * @tparam K - тип символа.
 * @tparam T - тип источника.
 * @en @brief A type for using std::string and std::string_view as sources in string expressions.
 * @tparam K is a symbol.
 * @tparam T - source type.
 */
template<typename K, typename T>
struct expr_stdstr {
    using symb_type = K;
    const T& t_;

    expr_stdstr(const T& t) : t_(t){}

    constexpr size_t length() const noexcept {
        return t_.size();
    }
    constexpr symb_type* place(symb_type* p) const noexcept {
        size_t s = t_.size();
        std::char_traits<K>::copy(p, (const K*)t_.data(), s);
        return p + s;
    }
};

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для char строкового выражения и std::string.
 * @en @brief Addition operator for char string expression and std::string.
 */
template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::string и char строкового выражения.
 * @en @brief Addition operator for std::string and char string expression.
 */
template<StrExprForType<u8s> A>
auto operator+(const std::string& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, false>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для char строкового выражения и std::string_view.
 * @en @brief Addition operator for char string expression and std::string_view.
 */
template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string_view& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::string_view и char строкового выражения.
 * @en @brief Addition operator for std::string_view and char string expression.
 */
template<StrExprForType<u8s> A>
auto operator+(const std::string_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, false>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для wchar_t строкового выражения и std::wstring.
 * @en @brief Addition operator for wchar_t string expression and std::wstring.
 */
template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::wstring и wchar_t строкового выражения.
 * @en @brief Addition operator for std::wstring and wchar_t string expression.
 */
template<StrExprForType<uws> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, false>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для wchar_t строкового выражения и std::wstring_view.
 * @en @brief Addition operator for wchar_t string expression and std::wstring_view.
 */
template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::wstring_view и wchar_t строкового выражения.
 * @en @brief Addition operator for std::wstring_view and wchar_t string expression.
 */
template<StrExprForType<uws> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, false>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для совместимого с wchar_t строкового выражения (char16_t или
 * char32_t, в зависимости от компилятора) и std::wstring.
 * @en @brief Addition operator for wchar_t compatible string expression (char16_t or
 * char32_t, depending on the compiler) and std::wstring.
 */
template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::wstring и совместимого с wchar_t строкового выражения
 * (char16_t или char32_t, в зависимости от компилятора).
 * @en @brief Addition operator for std::wstring and wchar_t-compatible string expression
 * (char16_t or char32_t, depending on the compiler).
 */
template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, false>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для совместимого с wchar_t строкового выражения (char16_t или
 * char32_t, в зависимости от компилятора) и std::wstring_view.
 * @en @brief Addition operator for wchar_t compatible string expression (char16_t or
 * char32_t, depending on the compiler) and std::wstring_view.
 */
template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, true>{a, s};
}

/*!
 * @ingroup StrExprs
 * @ru @brief Оператор сложения для std::wstring_view и совместимого с wchar_t строкового выражения
 * (char16_t или char32_t, в зависимости от компилятора).
 * @en @brief Addition operator for std::wstring_view and wchar_t-compatible string expression
 * (char16_t or char32_t, depending on the compiler).
 */
template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, false>{a, s};
}

}// namespace simstr
