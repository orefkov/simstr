/*
 * ver. 1.6.2
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * Тесты simstr
 * (c) Project "SimStr", Aleksandr Orefkov orefkov@gmail.com
 * Test of simstr
 */

#include "../include/simstr/strexpr.h"
#include <gtest/gtest.h>
#include <format>
#include <list>

using namespace std::literals;

namespace simstr::tests {
/*!
 * @ru
 * @defgroup ConvertToStrExpr Конвертация типов в в строковые выражения
 * Различные способы конвертации типов в строковые выражения.
 * Если вы реализуете один (и только один) из способов преобразования вашего типа в строковое выражение,
 * то сможете использовать ваш тип напрямую в операциях конкатенации со строковыми выражениями и как
 * аргументы в функциях `e_concat` и `e_subst`.
 * @en
 * @defgroup ConvertToStrExpr Converting types to string expressions
 * Various ways to convert types to string expressions.
 * If you implement one (and only one) of the ways to convert your type to a string expression,
 * then you can use your type directly in concatenation operations with string expressions and how
 * arguments in the `e_concat` and `e_subst` functions.
 */

/*!
 * @ingroup ConvertToStrExpr
 * @ru @page method1 Способ 1
 * Просто реализуйте в своём типе требования, чтобы он являлся строковым выражением.
 * @en @page method1 Method 1
 * Just implement the requirement in your type that it be a string expression.
 * @ru @par Пример:
 * @en @par Example:
 * @~
 * @snippet test_tostrexpr.cpp Method1
 */

//! [Method1]
struct add_exclamation {
    ssa text_;
    unsigned count_;

    // symb_type
    using symb_type = char;
    // length
    size_t length() const noexcept {
        return text_.length() + count_;
    }
    // place
    char* place(char* ptr) const noexcept{
        ptr = text_.place(ptr);
        std::char_traits<char>::assign(ptr, count_, '!');
        return ptr + count_;
    }
};

TEST(ToStrExpr, CheckExclamation) {
    std::string test = "Msg is <" + add_exclamation{"Happy Birthday", 5} + ">";
    EXPECT_EQ(test, "Msg is <Happy Birthday!!!!!>");

    add_exclamation msg{"Happy Birthday", 3};
    test = e_concat("", "Msg is <", msg, ">");
    EXPECT_EQ(test, "Msg is <Happy Birthday!!!>");

    const add_exclamation cmsg{"Happy Birthday", 0};
    test = e_subst(S_FRM("Msg is <{}>"), cmsg);
    EXPECT_EQ(test, "Msg is <Happy Birthday>");
}
//! [Method1]

/*!
 * @ingroup ConvertToStrExpr
 * @ru @page method2 Способ 2
 * Создайте тип-обёртку, который является строковым выражением и может инициализироваться
 * вашим типом. После задайте их соответствие с помощью специализации шаблона `convert_to_strexpr`.
 * @en @page method2 Method 2
 * Create a wrapper type that is a string expression and can be initialized
 * your type. Then set their correspondence using the `convert_to_strexpr` template specialization.
 * @ru @par Пример:
 * @en @par Example:
 * @~
 * @snippet test_tostrexpr.cpp Method2
 */

//! [Method2]

// Тип / Type
struct car_info {
    std::string model;
    int year;
};

// Обёртка для превращения его в строковое выражение
// Wrapper to turn it into a string expression
struct car_info_expr {
    const car_info& car_;
    expr_num<char, int> year;

    car_info_expr(const car_info& car) : car_(car), year(car.year){}
    inline static constexpr ssa ModelTag = "Model: ";
    inline static constexpr ssa YearTag = ", Year: ";

    using symb_type = char;

    size_t length() const {
        return ModelTag.length() + car_.model.length() + YearTag.length() + year.length();
    }

    char* place(char* ptr) const {
        ptr = ModelTag.place(ptr);
        std::char_traits<char>::copy(ptr, car_.model.data(), car_.model.length());
        ptr += car_.model.length();
        ptr = YearTag.place(ptr);
        return year.place(ptr);
    }
};

} // namespace simstr::tests


namespace simstr {

// Специализируем шаблон, задавая соответствие типа и его обёртки
// Specialize the template by specifying a match between the type and its wrapper
template<>
struct convert_to_strexpr<char, tests::car_info> {
    // Указываем тип, который будет "обёрткой" для нашего типа
    // Specify the type that will be a "wrapper" for our type
    using type = tests::car_info_expr;
};

} // namespace simstr

namespace simstr::tests {

TEST(ToStrExpr, CheckCarInfo) {
    car_info ci{"Ford", 2020};
    std::string test = "Car is <"_ss + ci + ">";
    EXPECT_EQ(test, "Car is <Model: Ford, Year: 2020>");

    ci.year++;
    test = e_concat("", "Car is <", ci, ">");
    EXPECT_EQ(test, "Car is <Model: Ford, Year: 2021>");

    ci.year++;
    test = e_subst(S_FRM("Car is <{}>"), ci);
    EXPECT_EQ(test, "Car is <Model: Ford, Year: 2022>");
}
//! [Method2]

/*!
 * @ingroup ConvertToStrExpr
 * @ru @page method3 Способ 3
 * Специализируйте шаблон `convert_to_strexpr` для вашего типа и создайте в нём статическую
 * функцию `convert`, принимающую ваш объект и возвращающую строковое выражение, строковый объект
 * simstr или std::basic_string.
 * @en @page method3 Method 3
 * Specialize the `convert_to_strexpr` template for your type and create a static
 * `convert` function in it that takes your object and returns a string expression, a simstr string object,
 * or std::basic_string.
 * @ru @par Пример:
 * @en @par Example:
 * @~
 * @snippet test_tostrexpr.cpp Method3
 */

//! [Method3]

struct animal {
    std::string name_;
    std::string sound_;
};

} //namespace simstr::tests

namespace simstr {

// Специализируем шаблон, задавая соответствие типа и его обёртки
// Specialize the template by specifying a match between the type and its wrapper
template<>
struct convert_to_strexpr<char, tests::animal> {
    // Создаём функцию `convert`, которая вернёт строковое представление объекта
    // Create a function `convert`, that will return a string representation of the object
    static auto convert(const tests::animal& a) {
        // Так делать нельзя - операция + складывает в выражение ссылки на временные объекты,
        // которые разрушаются после ";", и возвращать такой объект нельзя.
        // This can't be done - the operator+ adds references to temporary objects to the expression,
        // which are destroyed after ";", and such an object cannot be returned.
        //return "Animal: " + a.name_ + ", Sound: " + a.sound_;

        // А вот такой можно - он не хранит ссылки на временные объекты
        // But this one is possible - it doesn't store references to temporary objects
        return e_concat("", "Animal: ", a.name_, ", Sound: ", a.sound_);
        // Также можно возвращать просто std::string, stringa, lstringa
        // You can also return just std::string, stringa, lstringa
    }
};

} //namespace simstr

namespace simstr::tests {

TEST(ToStrExpr, CheckAnimal) {
    animal cat{"Cat", "Meow"};

    std::string test = "<"_ss + cat + ">";
    EXPECT_EQ(test, "<Animal: Cat, Sound: Meow>");

    const animal dog{"Dog", "Woof"};
    test = e_concat("", "<", dog, ">");
    EXPECT_EQ(test, "<Animal: Dog, Sound: Woof>");

    test = e_subst(S_FRM("\\_{}_/"), animal{"Snake", "Pssstt"});
    EXPECT_EQ(test, "\\_Animal: Snake, Sound: Pssstt_/");
}
//! [Method3]

/*!
 * @ingroup ConvertToStrExpr
 * @ru @page method4 Способ 4
 * Просто в своём типе сделайте функцию `template<typename K> auto to_strexpr()const`, которая возвращает
 * строковое выражение или строковый объект.
 * @en @page method4 Method 4
 * Simply create a `template<typename K> auto to_strexpr()const` function in your type that returns a
 * string expression or string object.
 * @ru @par Пример:
 * @en @par Example:
 * @~
 * @snippet test_tostrexpr.cpp Method4
 */

//! [Method4]

struct test {
    int number;
    int from;
    // Сделаем две отдельные реализации: для char (попроще) и для остальных типов (там придётся возвращать не литералы, а simple_str, и требуется копия)
    // Let's make two separate implementations: for char (simpler) and for other types (there we'll have to return not literals, but simple_str, and a copy is required)
    template<typename K> requires (std::is_same_v<K, char>)
    auto to_strexpr() const {
        return e_concat("", "test ", number, " from ", from);
    }

    template<typename K> requires (!std::is_same_v<K, char>)
    auto to_strexpr() const {
        // uni_string возвращает локальный объект, а ссылку на них нельзя возвращать из функции,
        // поэтому в e_concat надо форсировать сохранение по копии, а не по ссылке.
        // uni_string returns a local object, and references to them cannot be returned from a function,
        // so in e_concat we need to force saving by copy, not by reference.
        return e_concat(force_copy{empty_expr<K>{}}, force_copy{uni_string(K, "test ")}, number, force_copy{uni_string(K, " from ")}, from);
    }
};

TEST(ToStrExpr, CheckTest) {
    test t1{1, 10};

    std::string t = "Begin <"_ss + t1 + ">";
    EXPECT_EQ(t, "Begin <test 1 from 10>");

    const test t2{8, 12};

    t = e_concat("", "<", t2, ">");
    EXPECT_EQ(t, "<test 8 from 12>");

    t = e_subst(S_FRM("<{}>"), test{99, 100});
    EXPECT_EQ(t, "<test 99 from 100>");

    // Проверим работу для не char
    // Let's check the work for non-char
    std::wstring wt = L"aaa "_ss + test{99, 100};
    EXPECT_EQ(wt, L"aaa test 99 from 100");

    std::u16string ut = u"aaa "_ss + test{99, 100};
    EXPECT_EQ(ut, u"aaa test 99 from 100");
}
//! [Method4]

TEST(ToStrExpr, ConcatCustom) {
    std::string tt;
    std::string c = e_concat(eea + "," + " ", u8"bb", tt, 1, e_if(true, "--"), e_hex<HexFlags::Short>(16), 1.2, test{10, 30});
    EXPECT_EQ(c, "bb, , 1, --, 0x10, 1.2, test 10 from 30");

    std::string_view text = "testes";
    int count = 10;
    std::string t = e_concat("", text, " = ", count, " times.");
    EXPECT_EQ(t, "testes = 10 times.");
}

TEST(ToStrExpr, SubstCustom) {
    const auto ttt = "test"_ss;
    int ii = 3;
    const test tr{10, 10};
    std::string t = e_subst(S_FRM("Test {{--}} {}={}, {}"), ttt, tr, ii);
    EXPECT_EQ(t, "Test {--} test=test 10 from 10, 3");
    t = e_subst(S_FRM("Test {2}={1}, {2}, {1}"), "test", 2);
    EXPECT_EQ(t, "Test 2=test, 2, test");

    std::u16string u16t = e_subst(S_FRM(u"Test {}={}, {}"), u"test", 2, u"test"sv);
    EXPECT_EQ(u16t, u"Test test=2, test");
}
} // namespace simstr::tests
