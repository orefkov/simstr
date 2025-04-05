/*
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * ver. 1.0
 * База для строковых конкатенаций через выражения времени компиляции
 */
#pragma once
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <concepts>
#include <utility>

/*!
 * @brief Пространство имён для объектов библиотеки
 */
namespace simstr {

// Выводим типы для 16 и 32 битных символов в зависимости от размера wchar_t
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
*/

template<typename T> struct const_lit; // sfinae отработает, так как не найдёт определения
// Для правильных типов параметров есть определение, в виде специализации шаблона
template<typename T, size_t N>
    requires(is_one_of_char_v<T>)
struct const_lit<const T(&)[N]> {
    using symb_type = T;
    constexpr static size_t Count = N;
};

// Тут ещё дополнительно ограничиваем тип литерала
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
 * @brief Базовая концепция строкового объекта.
 * @tparam A - проверяемый тип
 * @tparam K - тип символов
 * @details В библиотеке для разных целей могут использоваться различные типы объектов строк.
 *  Мы считаем строковым объектом любой объект, поддерживающий методы:
 *  - `is_empty()`: возвращает, пуста ли строка.
 *  - `length()`: возвращает длину строки без нулевого терминатора.
 *  - `symbols()`: возвращает указатель на строку символов.
 *  - `typename symb_type`: задаёт тип символов строки
 */
template<typename A, typename K>
concept StrType = requires(const A& a) {
    { a.is_empty() } -> std::same_as<bool>;
    { a.length() } -> std::convertible_to<size_t>;
    { a.symbols() } -> std::same_as<const K*>;
} && std::is_same_v<typename std::remove_cvref_t<A>::symb_type, K>;

/*!
 * @brief Строковые выражения
 * @defgroup StrExprs Строковые выражения
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
 *    - `expr_spaces<ТипСимвола, КоличествоСимволов, Символ>{}`: выдает строку длиной КоличествоСимволов,
 *        заполненную заданным символом. Количество символов и символ - константы времени компиляции.
 *        Для некоторых случаев есть сокращенная запись:
 *        - `e_spca<КоличествоСимволов>()`: строка char пробелов
 *        - `e_spcw<КоличествоСимволов>()`: строка w_char пробелов
 *    - `expr_pad<ТипСимвола>{КоличествоСимволов, Символ}`: выдает строку длинной КоличествоСимволов,
 *        заполненную заданным символом. Количество символов и символ могут задаваться в рантайме.
 *        Сокращенная запись: `e_c(КоличествоСимволов, Символ)`
 *    - `e_choice(bool Condition, StrExpr1, StrExpr2)`: если Condition == true, результат будет равен StrExpr1, иначе StrExpr2
 *    - `e_num<ТипСимвола>(ЦелоеЧисло)`: конвертирует число в десятичное представление. Редко используется, так как
 *         для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как text + number;
 *    - `e_real<ТипСимвола>(ВещественноеЧисло)`: конвертирует число в десятичное представление. Редко используется, так как
 *         для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как `text + number`;
 *    - `e_join<bool ПослеПоследнего = false, bool ПропускатьПустые = false>(контейнер, "Разделитель")`: конкатенирует все строки
 *        в контейнере, используя разделитель.
 *        Если `ПослеПоследнего == true`, то разделитель добавляется и после последнего элемента контейнера, иначе только
 *        между элементами.
 *        Если `ПропускатьПустые == true`, то пустые строки не добавляют разделитель, иначе для каждой пустой строки
 *        тоже вставляется разделитель
 *    - `e_repl(ИсходнаяСтрока, "Искать", "Заменять")`: заменяет в исходной строке вхождения "Искать" на "Заменять".
 *        Шаблоны поиска и замены - строковые литералы времени компиляции.
 *    - `expr_replaced<ТипСимвола>{ИсходнаяСтрока, Искать, Заменять}`: заменяет в исходной строке вхождения Искать на Заменять.
 *         Шаблоны поиска и замены - могут быть любыми строковыми объектами в рантайме.
 *
 *  и т.д. и т.п.
 */

/*!
 * @brief Концепт "Строковых выражений"
 * @ingroup StrExprs
 * @details Это концепт, проверяющий, является ли тип "строковым выражением".
 */
template<typename A>
concept StrExpr = requires(const A& a) {
    typename A::symb_type;
    { a.length() } -> std::convertible_to<size_t>;
    { a.place(std::declval<typename A::symb_type*>()) } -> std::same_as<typename A::symb_type*>;
};

/*!
 * @brief Концепт строкового выражения заданного типа символов
 * @ingroup StrExprs
 * @tparam A - проверяемый тип
 * @tparam K - проверяемый тип символов
 * @details Служит для задания ограничения к строковому выражению по типу символов
 */
template<typename A, typename K>
concept StrExprForType = StrExpr<A> && std::is_same_v<K, typename A::symb_type>;

/*
* Шаблонные классы для создания строковых выражений из нескольких источников
* Благодаря компиляторно-шаблонной "магии" позволяют максимально эффективно
* получать результирующую строку - сначала вычисляется длина результирующей строки,
* потом один раз выделяется память для результата, после символы помещаются в
* выделенную память.
* Для конкатенация двух объектов строковых выражений в один
*/

/*!
 * @brief Шаблонный класс для конкатенации двух строковых выражений в одно с помощью `operator +`
 * @ingroup StrExprs
 * @tparam A - Тип первого операнда
 * @tparam B - Тип второго операнда
 * @details Этот объект запоминает ссылки на два операнда операции сложения.
 * Когда у него запрашивают необходимый для результата размер буфера - он выдает сумму длин своих операндов.
 * Когда запрашивают размещение символов в буфере - размещает сначала первый операнд, затем второй.
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
 * @brief Оператор сложения двух произвольных строковых выражения для одинакового типа символов
 * @ingroup StrExprs
 * @anchor op_plus_str_expr
 * @param a - первое строковое выражение
 * @param b - второе строковое выражение
 * @return strexprjoin<A, B>, строковое выражение, генерирующее объединение переданных выражений
 * @details Когда складываются два объекта - строковых выражения, один типа `A`, другой типа `B`,
 * мы возвращаем объект типа strexprjoin<A, B>, который содержит ссылки на два этих операнда.
 * А сам объект strexprjoin<A, B> тоже в свою очередь является строковым выражением, и может участвовать
 * в следующих операциях сложения. Таким образом формируется "дерево" из исходных строковых
 * выражений, которое потом за один вызов "материализуется" в конечный результат.
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
inline auto operator+(const A& a, const B& b) {
    return strexprjoin<A, B>{a, b};
}

/*!
 * @brief Конкатенация ссылки на строковое выражение и значения строкового выражения
 * @ingroup StrExprs
 * @tparam A - Тип одного строкового выражения
 * @tparam B - Тип другого строкового выражения
 * @tparam last - какое из них первое
 * @details Чтобы иметь возможность складывать строковое выражение с операндами, не являющимися строковым выражением,
 *  нам нужно иметь возможность вернуть из `operator+` объект, который сохранит ссылку на операнд, являющийся строковым
 *  выражением, а для не строкового операнда будет иметь поле со строковым выражением, обрабатывающим второй операнд.
 *  Можно посмотреть пример в simstr::operator+<StrExpr A, FromIntNumber T>()
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
 * @brief "Пустое" строковое выражение
 * @ingroup StrExprs
 * @tparam K - тип символа
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
 *  Пример:
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
 * @brief Пустое строковое выражение типа char
 * @ingroup StrExprs
 */
inline constexpr empty_expr<u8s> eea{};
/*!
 * @brief Пустое строковое выражение типа wchar_t
 * @ingroup StrExprs
 */
inline constexpr empty_expr<uws> eew{};
/*!
 * @brief Пустое строковое выражение типа char16_t
 * @ingroup StrExprs
 */
inline constexpr empty_expr<u16s> eeu{};
/*!
 * @brief Пустое строковое выражение типа char32_t
 * @ingroup StrExprs
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
 * @brief Оператор сложения строкового выражения и одного символа
 * @ingroup StrExprs
 * @return строковое выражение, объединяющее переданное выражение и символ
 * @details Пример:
 *  ```cpp
 *  reply = prompt + '>' + result;
 *  ```
 */
template<typename K, StrExprForType<K> A>
constexpr inline auto operator+(const A& a, K s) {
    return strexprjoin_c<A, expr_char<K>>{a, s};
}

/*!
 * @brief Генерирует строку из 1 заданного символа
 * @ingroup StrExprs
 * @param S - символ
 * @return строковое выражение для строки из одного символа
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
 * @brief Преобразует строковый литерал в строковое выражение.
 * @ingroup StrExprs
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
 * @brief Оператор сложения для строкового выражения и строкового литерала такого же типа символов
 * @ingroup StrExprs
 * @return Строковое выражение, объединяющее операнды
 */
template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(const A& a, T&& s) {
    return expr_literal_join<false, K, (N - 1), A>{s, a};
}

/*!
 * @brief Оператор сложения для строкового литерала такого же типа символов и строкового выражения
 * @ingroup StrExprs
 * @return Строковое выражение, объединяющее операнды
 */
template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(T&& s, const A& a) {
    return expr_literal_join<true, K, (N - 1), A>{s, a};
}

/*!
 * @brief Тип строкового выражения, возвращающего N заданных символов.
 * @ingroup StrExprs
 * @details Количество символов и сам символ константы, т.е. задаются при компиляции.
 * @tparam K - тип символа
 * @tparam N - количество символов
 * @tparam S - символ, по умолчанию пробел
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
 * @brief Генерирует строку из N char пробелов 
 * @ingroup StrExprs
 * @tparam N - Количество пробелов
 * @return строковое выражение для N char пробелов
 * @details Пример: 
 *  ```cpp
 *  stringa text = e_spca<10>() + text + e_spca<10>();
 *  ```
 */
template<size_t N>
constexpr inline auto e_spca() {
    return expr_spaces<u8s, N>();
}

/*!
 * @brief Генерирует строку из N wchar_t пробелов 
 * @ingroup StrExprs
 * @tparam N - Количество пробелов
 * @return строковое выражение для N wchar_t пробелов
 * @details Пример:
 *  ```cpp
 *  stringw text = e_spcw<10>() + text + e_spcw<10>();
 *  ```
 */
template<size_t N>
constexpr inline auto e_spcw() {
    return expr_spaces<uws, N>();
}

/*!
 * @brief Тип строкового выражения, возвращающего N заданных символов.
 * @ingroup StrExprs
 * @tparam K - тип символа
 * @details Количество символов и сам символ переменные, т.е. могут меняться в рантайм.
 *  Напрямую обычно не используется, создается через e_c()
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
 * @brief Генерирует строку из l символов s типа K
 * @ingroup StrExprs
 * @tparam K - тип символа
 * @param l - количество символов
 * @param s - символ
 * @return строковое выражение, генерирующее строку из l символов k
 */
template<typename K>
constexpr inline auto e_c(size_t l, K s) {
    return expr_pad<K>{ l, s };
}

/*!
 * @brief Строковое выражение условного выбора
 * @ingroup StrExprs
 * @tparam A Тип ветки для true
 * @tparam B Тип ветки для false
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо выражение B.
 *  Напрямую тип обычно не используется, создаётся через e_choice()
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
 * @brief Строковое выражение условного выбора
 * @ingroup StrExprs
 * @tparam A Тип ветки для true
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо пустую строку.
 *  Напрямую тип обычно не используется, создаётся через e_if()
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
 * @brief Строковое выражение условного выбора
 * @ingroup StrExprs
 * @tparam A Тип ветки для true
 * @details Выражение, в зависимости от истинности условия генерирующее либо выражение A, либо строку из строкового литерала.
 *  Напрямую тип обычно не используется, создаётся через e_choice()
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
 * @brief Строковое выражение условного выбора
 * @ingroup StrExprs
 * @details Выражение, в зависимости от истинности условия генерирующее либо один строковый литерал, либо другой.
 *  Напрямую тип обычно не используется, создаётся через e_choice()
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
 * @brief Создание условного строкового выражения expr_choice
 * @ingroup StrExprs
 * @tparam A - Тип выражение при истинности условия, выводится из аргумента
 * @tparam B - Тип выражения при ложности условия, выводится из аргумента
 * @param c - булево условие
 * @param a - строковое выражение, выполняющееся при `c == true`
 * @param b - строковое выражение, выполняющееся при `c == false`
 * @details Служит для возможности в одном выражении выбирать разные варианты в зависимости от условия.
 *
 *  Примеры:
 *  ```cpp
 *  columns_metadata.emplace_back(e_choice(name.is_empty(), "?column?", name) + "::" + metadata_column.type.to_string());
 *  ```
 *  ```cpp
 *  lstringa<512> str = e_choice(!ret_type_resolver_, sql_value::type_name(ret_type_), "any") + " " + name_ + "(";
 *  ```
 *  Иначе такие операции приходилось бы разбивать на несколько модификаций строки или применению временных строк,
 *  что не оптимально и снизит производительность. (Это проверяется в бенчмарке "Build Full Func Name")
 */
template<StrExpr A, StrExprForType<typename A::symb_type> B>
inline constexpr auto e_choice(bool c, const A& a, const B& b) {
    return expr_choice<A, B>{a, b, c};
}

/*!
 * @brief Перегрузка e_choice, когда третий аргумент - строковый литерал
 * @ingroup StrExprs
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
inline constexpr auto e_choice(bool c, const A& a, T&& str) {
    return expr_choice_one_lit<A, N - 1, true>{str, a, c};
}

/*!
 * @brief Перегрузка e_choice, когда второй аргумент - строковый литерал
 * @ingroup StrExprs
 */
template<StrExpr A, typename T, size_t N = const_lit_for<typename A::symb_type, T>::Count>
inline constexpr auto e_choice(bool c, T&& str, const A& a) {
    return expr_choice_one_lit<A, N - 1, false>{str, a, c};
}
/*!
 * @brief Перегрузка e_choice, когда второй и третий аргумент - строковые литералы
 * @ingroup StrExprs
 */
template<typename T, typename L, size_t N = const_lit<T>::Count, size_t M = const_lit_for<typename const_lit<T>::symb_type, L>::Count>
inline constexpr auto e_choice(bool c, T&& str_a, L&& str_b) {
    return expr_choice_two_lit<typename const_lit<T>::symb_type, N -1, M - 1>{str_a, str_b, c};
}

/*!
 * @brief Создание условного строкового выражения expr_if
 * @ingroup StrExprs
 * @tparam A - Тип выражение при истинности условия, выводится из аргумента
 * @param c - булево условие
 * @param a - строковое выражение, выполняющееся при `c == true`
 * @details Служит для возможности в одном выражении генерировать в зависимости от условия либо указанный вариант, либо пустую строку.
 *
 *  Примеры:
 *  ```cpp
 *  void sql_func_info::build_full_name() {
 *      // Временный буфер для результата, возьмём с запасом
 *      lstringa<512> str = e_choice(!ret_type_resolver_, sql_value::type_name(ret_type_), "any") + " " + name_ + "(";
 *
 *      bool add_comma = false;
 *
 *      for (const auto& param : params_) {
 *          str += e_if(add_comma, ", ") + e_if(param.optional_, "[");
 *          param.allowed_types.to_string(str); // Добавляет к str названия допустимых типов
 *          if (param.optional_) {
 *              str += "]";
 *          }
 *          add_comma = true;
 *      }
 *      // Сохраним в stringa
 *      full_name_ = str + e_if(unlim_params_, e_if(add_comma, ", ") + "...") + ")";
 *  }
 *  ```
 *  Иначе такие операции приходилось бы разбивать на несколько модификаций строки или применению временных строк,
 *  что не оптимально и снизит производительность. (Этот пример проверяется в бенчмарке "Build Full Func Name")
 */
template<StrExpr A>
inline constexpr auto e_if(bool c, const A& a) {
    return expr_if<A>{a, c};
}
/*!
 * @brief Перегрузка e_if, когда второй аргумент - строковый литерал
 * @ingroup StrExprs
 */
template<typename T, size_t N = const_lit<T>::Count>
inline constexpr auto e_if(bool c, T&& str) {
    const typename const_lit<T>::symb_type empty[1] = {0};
    return expr_choice_two_lit<typename const_lit<T>::symb_type, N - 1, 0>{str, empty, c};
}

/*!
 * @brief Тип для использования std::string и std::string_view как источников в строковых выражениях
 * @ingroup StrExprs
 * @tparam K - тип символа
 * @tparam T - тип источника
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
 * @brief Оператор сложения для char строкового выражения и std::string
 * @ingroup StrExprs
 */
template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::string и char строкового выражения
 * @ingroup StrExprs
 */
template<StrExprForType<u8s> A>
auto operator+(const std::string& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, false>{a, s};
}

/*!
 * @brief Оператор сложения для char строкового выражения и std::string_view
 * @ingroup StrExprs
 */
template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string_view& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::string_view и char строкового выражения
 * @ingroup StrExprs
 */
template<StrExprForType<u8s> A>
auto operator+(const std::string_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, false>{a, s};
}

/*!
 * @brief Оператор сложения для wchar_t строкового выражения и std::wstring
 * @ingroup StrExprs
 */
template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::wstring и wchar_t строкового выражения
 * @ingroup StrExprs
 */
template<StrExprForType<uws> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, false>{a, s};
}

/*!
 * @brief Оператор сложения для wchar_t строкового выражения и std::wstring_view
 * @ingroup StrExprs
 */
template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::wstring_view и wchar_t строкового выражения
 * @ingroup StrExprs
 */
template<StrExprForType<uws> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, false>{a, s};
}

/*!
 * @brief Оператор сложения для совместимого с wchar_t строкового выражения (char16_t или
 * char32_t, в зависимости от компилятора) и std::wstring
 * @ingroup StrExprs
 */
template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::wstring и совместимого с wchar_t строкового выражения
 * (char16_t или char32_t, в зависимости от компилятора)
 * @ingroup StrExprs
 */
template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, false>{a, s};
}

/*!
 * @brief Оператор сложения для совместимого с wchar_t строкового выражения (char16_t или
 * char32_t, в зависимости от компилятора) и std::wstring_view
 * @ingroup StrExprs
 */
template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, true>{a, s};
}

/*!
 * @brief Оператор сложения для std::wstring_view и совместимого с wchar_t строкового выражения
 * (char16_t или char32_t, в зависимости от компилятора)
 * @ingroup StrExprs
 */
template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, false>{a, s};
}

}// namespace simstr
