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
    
    bool contain(K s) const {
        return exist<0>(s);
    }
    size_t index_of(K s) const {
        return find<0>(s);
    }
};

/*
Базовая концепция строкового объекта.
В библиотеке для разных целей могут использоваться различные типы объектов строк.
Мы считаем строковым объектом любой объект, поддерживающий методы:
isEmpty() - возвращает, пуста ли строка.
length() - длину строки без нулевого терминатора.
symbols() - указатель на строку символов.
и содержит объявление типа symb_type, задающего тип символов строки
*/

template<typename A, typename K>
concept StrType = requires(const A& a) {
    { a.is_empty() } -> std::same_as<bool>;
    { a.length() } -> std::convertible_to<size_t>;
    { a.symbols() } -> std::same_as<const K*>;
} && std::is_same_v<typename std::remove_cvref_t<A>::symb_type, K>;

/*
* Все типы владеющих строк могут инициализироваться с помощью "строковых выражений".
* Строковое выражение - это объект произвольного типа, у которого имеются методы:
* size_t length() const - выдает длину строки
* K* place(K*) const - скопировать символы строки в предназначенный буфер и вернуть указатель за последним символом

* Т.е. при инициализации строковый объект запрашивает размер у строкового выражения, выделяет необходимую память,
* и передает память строковому выражению, которое помещает символы в отведённый буфер.
* Все строковые объекты сами могут выступать источником строкового выражения.

* В-основном строковые выражения используются для конкатенации или конвертации строк.
* Для них определен оператор +, который создает строковое выражение, объединениющее два строковых выражения,
* и последовательно вызывающее их методы length и place.
* Также оператор + определён для строковых выражений и строковых литералов, строковых выражений и чисел
* (числа конвертируются в десятичное представление), а также вы можете сами добавить желаемые типы.
* Пример:
    stringa text = header + " count=" + count + ", done";

* Существует несколько типов строковых выражений "из коробки", для выполнения различных операций со строками

    expr_spaces<ТипСимвола, КоличествоСимволов, Символ = ' '>{} - выдает строку длинной КоличествоСимволов,
        заполненную заданным символом. Количество символов и символ - константы времени компиляции.
        Для некоторых случаев есть сокращенная запись:
            e_spca(КоличествоСимволов) - строка char пробелов
            e_spcw(КоличествоСимволов) - строка w_char пробелов

    expr_pad<ТипСимвола>{КоличествоСимволов, Символ = ' '} - выдает строку длинной КоличествоСимволов,
        заполненную заданным символом. Количество символов и символ могут задаваться в рантайме.
        Сокращенная запись:
            e_c(КоличествоСимволов, Символ)

    e_choice(bool Condition, StrExpr1, StrExpr2) - если Condition == true, результат будет равен StrExpr1, иначе StrExpr2

    e_num<ТипСимвола>(ЦелоеЧисло) - конвертирует число в десятичное представление. Редко используется, так как
        для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как text + number;

    e_real<ТипСимвола>(ВещественноеЧисло) - конвертирует число в десятичное представление. Редко используется, так как
        для строковых выражений и чисел переопределен оператор "+", и число можно просто написать как text + number;

    e_join<bool ПослеПоследнего = false>(контейнер, "Разделитель") - конкатенирует все строки в контейнере, используя разделитель.
        Если ПослеПоследнего == true, то разделитель добавляется и после последнего элемента контейнера, иначе только
        между элементами.

    e_repl(ИсходнаяСтрока, "Искать", "Заменять") - заменяет в исходной строке вхождения "Искать" на "Заменять".
        Шаблоны поиска и замены - строковые литералы времени компиляции.

    expr_replaced<ТипСимвола>{ИсходнаяСтрока, Искать, Заменять} - заменяет в исходной строке вхождения Искать на Заменять.
        Шаблоны поиска и замены - могут быть любыми строковыми объектами в рантайме.
}
*/

/*
* Концепт строкового выражения.
* Источником строковых выражений может быть любой объект, поддерживающий эти операции:
* тип symb_type, length(), place()
*/
template<typename A>
concept StrExpr = requires(A&& a) {
    typename A::symb_type;
    { a.length() } -> std::convertible_to<size_t>;
    { a.place(std::declval<typename A::symb_type*>()) } -> std::same_as<typename A::symb_type*>;
};

/*
* Концепт строкового выражения заданного типа символов
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

template<StrExpr A, StrExprForType<typename A::symb_type> B>
inline auto operator+(const A& a, const B& b) {
    return strexprjoin<A, B>{a, b};
}

// Для возможности конкатенации ссылок на строковое выражение и создаваемого временного объекта, путём его копии
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

inline constexpr empty_expr<u8s> eea{};
inline constexpr empty_expr<uws> eew{};
inline constexpr empty_expr<u16s> eeu{};
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

template<typename K, StrExprForType<K> A>
constexpr inline auto operator+(const A& a, K s) {
    return strexprjoin_c<A, expr_char<K>>{a, s};
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

template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(const A& a, T&& s) {
    return expr_literal_join<false, K, (N - 1), A>{s, a};
}

template<StrExpr A, typename K = typename A::symb_type, typename T, size_t N = const_lit_for<K, T>::Count>
constexpr inline auto operator+(T&& s, const A& a) {
    return expr_literal_join<true, K, (N - 1), A>{s, a};
}

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

template<size_t N>
constexpr inline auto e_spca() {
    return expr_spaces<u8s, N>();
}

template<size_t N>
constexpr inline auto e_spcw() {
    return expr_spaces<uws, N>();
}

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

template<typename K>
constexpr inline auto e_c(size_t l, K s) {
    return expr_pad<K>{ l, s };
}

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

template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, true>{a, s};
}

template<StrExprForType<u8s> A>
auto operator+(const std::string& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string>, false>{a, s};
}

template<StrExprForType<u8s> A>
auto operator+(const A& a, const std::string_view& s) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, true>{a, s};
}

template<StrExprForType<u8s> A>
auto operator+(const std::string_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<u8s, std::string_view>, false>{a, s};
}

template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, true>{a, s};
}

template<StrExprForType<uws> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring>, false>{a, s};
}

template<StrExprForType<uws> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, true>{a, s};
}

template<StrExprForType<uws> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<uws, std::wstring_view>, false>{a, s};
}

template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, true>{a, s};
}

template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring>, false>{a, s};
}

template<StrExprForType<wchar_type> A>
auto operator+(const A& a, const std::wstring_view& s) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, true>{a, s};
}

template<StrExprForType<wchar_type> A>
auto operator+(const std::wstring_view& s, const A& a) {
    return strexprjoin_c<A, expr_stdstr<wchar_type, std::wstring_view>, false>{a, s};
}
}// namespace simstr
