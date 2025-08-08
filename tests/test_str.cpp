﻿#include <simstr/sstring.h>
#include <gtest/gtest.h>

namespace simstr::tests {
using namespace std::literals;

class Tstringa: public stringa {
public:
    using stringa::stringa;

    bool isLocalString() const { return type_ == Local; }
    bool isConstantString() const { return type_ == Constant; }
    bool isSharedString() const { return type_ == Shared; }

    size_t sharedCount() const {
        return type_ == Shared ? SharedStringData<u8s>::from_str(sstr_)->ref_.load() : 0u;
    }
};

TEST(SimStr, CreateSimpleEmpty) {
    ssa testa{nullptr, 0};
    EXPECT_TRUE(testa.is_empty());
    EXPECT_EQ(testa.length(), 0u);

    ssw testw{nullptr, 0};
    EXPECT_TRUE(testw.is_empty());
    EXPECT_EQ(testw.length(), 0u);

    ssu testu{nullptr, 0};
    EXPECT_TRUE(testu.is_empty());
    EXPECT_EQ(testu.length(), 0u);

    ssuu testuu{nullptr, 0};
    EXPECT_TRUE(testuu.is_empty());
    EXPECT_EQ(testuu.length(), 0u);
}

TEST(SimStr, CompareSimpleEmptyEqual) {
    ssa testa{nullptr, 0};
    EXPECT_TRUE(testa == "");
    EXPECT_TRUE(testa != "test");

    ssw testw{nullptr, 0};
    EXPECT_TRUE(testw == L"");
    EXPECT_TRUE(testw != L"test");

    ssu testu{nullptr, 0};
    EXPECT_TRUE(testu == u"");
    EXPECT_TRUE(testu != u"test");

    ssuu testuu{nullptr, 0};
    EXPECT_TRUE(testuu == U"");
    EXPECT_TRUE(testuu != U"test");
}

TEST(SimStr, CompareSimpleNonEmptyEqual) {
    ssa testa{"test"};
    EXPECT_TRUE(testa == "test");
    EXPECT_TRUE(testa != "");

    ssw testw{L"test"};
    EXPECT_TRUE(testw == L"test");
    EXPECT_TRUE(testw != L"");

    ssu testu{u"test"};
    EXPECT_TRUE(testu == u"test");
    EXPECT_TRUE(testu != u"");

    ssuu testuu{U"test"};
    EXPECT_TRUE(testuu == U"test");
    EXPECT_TRUE(testuu != U"");
}

TEST(SimStr, CreateSimple) {
    ssa testa = "test";
    EXPECT_EQ(testa, "test");
    EXPECT_FALSE(testa.is_empty());

    ssu testu = u"test";
    EXPECT_EQ(testu, u"test");
    EXPECT_FALSE(testu.is_empty());

    ssw testw = L"test";
    EXPECT_EQ(testw, L"test");
    EXPECT_FALSE(testw.is_empty());
}

TEST(SimStr, At) {
    ssa testa = "test";
    EXPECT_EQ(testa.at(1), 'e');
    EXPECT_EQ(testa.at(-1), 't');
    EXPECT_EQ(testa.at(2), 's');
    EXPECT_EQ(testa.at(-2), 's');

    ssu testu{u"test"};
    EXPECT_EQ(testu.at(1), u'e');
    EXPECT_EQ(testu.at(-1), u't');
    EXPECT_EQ(testu.at(2), u's');
    EXPECT_EQ(testu.at(-2), u's');

    ssw testw{L"test"};
    EXPECT_EQ(testw.at(1), L'e');
    EXPECT_EQ(testw.at(-1), L't');
    EXPECT_EQ(testw.at(2), L's');
    EXPECT_EQ(testw.at(-2), L's');
}

TEST(SimStr, CompareSimpleAll) {
    ssa testa1{"test"}, testa2{"testa"};
    EXPECT_TRUE(testa1 < "testa");
    EXPECT_TRUE(testa1 < testa2);
    EXPECT_TRUE(testa2 > testa1);
    EXPECT_TRUE(testa1 > "");
    EXPECT_TRUE(testa1 > "te");
    EXPECT_TRUE("" < testa1);
    EXPECT_TRUE("te" < testa1);
    EXPECT_TRUE("test" == testa1);
    EXPECT_TRUE(testa1 != "");
    EXPECT_TRUE(testa1 != testa2);
    EXPECT_EQ(testa1.compare(testa2), -1);
    EXPECT_EQ(testa2.compare(testa1), 1);
    EXPECT_EQ(testa1.compare(testa1), 0);
    testa2.len = testa1.length();
    EXPECT_TRUE(testa1 == testa2);


    ssu testu1{u"test"}, testu2{u"testa"};
    EXPECT_TRUE(testu1 < u"testa");
    EXPECT_TRUE(testu1 < testu2);
    EXPECT_TRUE(testu2 > testu1);
    EXPECT_TRUE(testu1 > u"");
    EXPECT_TRUE(testu1 > u"te");
    EXPECT_TRUE(u"" < testu1);
    EXPECT_TRUE(u"te" < testu1);
    EXPECT_TRUE(u"test" == testu1);
    EXPECT_TRUE(testu1 != u"");
    EXPECT_TRUE(testu1 != testu2);
    EXPECT_EQ(testu1.compare(testu2), -1);
    EXPECT_EQ(testu2.compare(testu1), 1);
    EXPECT_EQ(testu1.compare(testu1), 0);
    testu2.len = testu1.length();
    EXPECT_TRUE(testu1 == testu2);

    ssw testw1{L"test"}, testw2{L"testa"};
    EXPECT_TRUE(testw1 < L"testa");
    EXPECT_TRUE(testw1 < testw2);
    EXPECT_TRUE(testw2 > testw1);
    EXPECT_TRUE(testw1 > L"");
    EXPECT_TRUE(testw1 > L"te");
    EXPECT_TRUE(L"" < testw1);
    EXPECT_TRUE(L"te" < testw1);
    EXPECT_TRUE(L"test" == testw1);
    EXPECT_TRUE(testw1 != L"");
    EXPECT_TRUE(testw1 != testw2);
    EXPECT_EQ(testw1.compare(testw2), -1);
    EXPECT_EQ(testw2.compare(testw1), 1);
    EXPECT_EQ(testw1.compare(testw1), 0);
    testw2.len = testw1.length();
    EXPECT_TRUE(testw1 == testw2);
}

TEST(SimStr, CompareSimpleAllIA) {
    ssa testa1{"tEst"}, testa2{"Testa"};
    EXPECT_TRUE(testa1.equal_ia("test"));
    EXPECT_TRUE(testa1 > testa2);
    EXPECT_TRUE(testa1.compare_ia(testa2) < 0);

    ssu testu1{u"tEst"}, testu2{u"Testa"};
    EXPECT_TRUE(testu1.equal_ia(u"test"));
    EXPECT_TRUE(testu1 > testu2);
    EXPECT_TRUE(testu1.compare_ia(testu2) < 0);

    ssw testw1{L"tEst"}, testw2{L"Testa"};
    EXPECT_TRUE(testw1.equal_ia(L"test"));
    EXPECT_TRUE(testw1 > testw2);
    EXPECT_TRUE(testw1.compare_ia(testw2) < 0);

    ssuu testuu1{U"tEst"}, testuu2{U"Testa"};
    EXPECT_TRUE(testuu1.equal_ia(U"test"));
    EXPECT_TRUE(testuu1 > testuu2);
    EXPECT_TRUE(testuu1.compare_ia(testuu2) < 0);
}

TEST(SimStr, CompareSimpleAllIU) {
    ssa testa1{"шоРох"}, testa2{"ШороХа"};
    EXPECT_TRUE(testa1.equal_iu("Шорох"));
    EXPECT_TRUE(testa1 > testa2);
    EXPECT_TRUE(testa1.compare_iu(testa2) < 0);

    ssu testu1{u"шоРох"}, testu2{u"ШороХа"};
    EXPECT_TRUE(testu1.equal_iu(u"Шорох"));
    EXPECT_TRUE(testu1 > testu2);
    EXPECT_TRUE(testu1.compare_iu(testu2) < 0);

    ssw testw1{L"шоРох"}, testw2{L"ШороХа"};
    EXPECT_TRUE(testw1.equal_iu(L"Шорох"));
    EXPECT_TRUE(testw1 > testw2);
    EXPECT_TRUE(testw1.compare_iu(testw2) < 0);

    ssuu testuu1{U"шоРох"}, testuu2{U"ШороХа"};
    EXPECT_TRUE(testuu1.equal_iu(U"Шорох"));
    EXPECT_TRUE(testuu1 > testuu2);
    EXPECT_TRUE(testuu1.compare_iu(testuu2) < 0);
}

TEST(SimStr, SimpleFind) {
    ssa testa = "Find a needle in a haystack";
    EXPECT_EQ(testa.find("diamond"), str::npos);
    EXPECT_EQ(testa.find("needle"), 7u);
    EXPECT_EQ(testa.find("a"), 5u);
    EXPECT_EQ(testa.find("a", 8), 17u);
    EXPECT_EQ(testa.find('a'), 5u);
    EXPECT_EQ(testa.find('a', 8), 17u);
    EXPECT_EQ(testa.find_last('d'), 10u);

    auto res = testa.find_all("i");
    EXPECT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0], 1u);
    EXPECT_EQ(res[1], 14u);

    res = testa.find_all("i", 8);
    EXPECT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], 14u);

    res = testa.find_all("i", 1, 1);
    EXPECT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0], 1u);

    ssu testu = u"Find a needle in a haystack";
    EXPECT_EQ(testu.find(u"diamond"), str::npos);
    EXPECT_EQ(testu.find(u"needle"), 7u);
    EXPECT_EQ(testu.find(u"a"), 5u);
    EXPECT_EQ(testu.find(u"a", 8), 17u);
    EXPECT_EQ(testu.find(u'a'), 5u);
    EXPECT_EQ(testu.find(u'a', 8), 17u);
    EXPECT_EQ(testu.find_last(u'd'), 10u);

    res = testu.find_all(u"i");
    EXPECT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0], 1u);
    EXPECT_EQ(res[1], 14u);

    ssw testw = L"Find a needle in a haystack";
    EXPECT_EQ(testw.find(L"diamond"), str::npos);
    EXPECT_EQ(testw.find(L"needle"), 7u);
    EXPECT_EQ(testw.find(L"a"), 5u);
    EXPECT_EQ(testw.find(L"a", 8), 17u);
    EXPECT_EQ(testw.find(L'a'), 5u);
    EXPECT_EQ(testw.find(L'a', 8), 17u);
    EXPECT_EQ(testw.find_last(L'd'), 10u);

    res = testw.find_all(L"i");
    EXPECT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0], 1u);
    EXPECT_EQ(res[1], 14u);

    EXPECT_EQ("cccabcccab"_ss.find_last("ab"), 8);
    EXPECT_EQ("cccabcccab"_ss.find_last("abc"), 3);
    EXPECT_EQ("cccabcccabcc"_ss.find_last("ab"), 8);
    EXPECT_EQ("cccabccccc"_ss.find_last("ab"), 3);
    EXPECT_EQ("cccabcccabcc"_ss.find_last("ab", 6), 3);
    EXPECT_EQ("abccccc"_ss.find_last("ab"), 0);
}

TEST(SimStr, SubPiece) {
    ssa testa{"test"};
    EXPECT_EQ(testa(1), "est");
    EXPECT_EQ(testa(1, 2), "es");
    EXPECT_EQ(testa(1, -1), "es");
    EXPECT_EQ(testa(1, -10), "");

    ssu testu{u"test"};
    EXPECT_EQ(testu(1), u"est");
    EXPECT_EQ(testu(1, 2), u"es");

    ssw testw{L"test"};
    EXPECT_EQ(testw(1), L"est");
    EXPECT_EQ(testw(1, 2), L"es");
}

TEST(SimStr, SimpleSubstr) {
    ssa testa = "test";
    EXPECT_EQ(testa.substr(1, 0), "est");
    EXPECT_EQ(testa.substr(1, 2), "es");
    EXPECT_EQ(testa.substr(0, -1), "tes");
    EXPECT_EQ(testa.substr(-2), "st");
    EXPECT_EQ(testa.substr(-3, 2), "es");
    EXPECT_EQ(testa.substr(-3, -1), "es");
    EXPECT_EQ(testa.str_mid(1), "est");
    EXPECT_EQ(testa.str_mid(1, 0), "");
    EXPECT_EQ(testa.str_mid(1, 2), "es");
    EXPECT_EQ(testa.str_mid(2, 2), "st");

    ssu testu = u"test";
    EXPECT_EQ(testu.substr(1, 0), u"est");
    EXPECT_EQ(testu.substr(1, 2), u"es");
    EXPECT_EQ(testu.substr(0, -1), u"tes");
    EXPECT_EQ(testu.substr(-2), u"st");
    EXPECT_EQ(testu.substr(-3, 2), u"es");
    EXPECT_EQ(testu.substr(-3, -1), u"es");
    EXPECT_EQ(testu.str_mid(1), u"est");
    EXPECT_EQ(testu.str_mid(1, 0), u"");
    EXPECT_EQ(testu.str_mid(1, 2), u"es");
    EXPECT_EQ(testu.str_mid(2, 2), u"st");

    ssw testw = L"test";
    EXPECT_EQ(testw.substr(1, 0), L"est");
    EXPECT_EQ(testw.substr(1, 2), L"es");
    EXPECT_EQ(testw.substr(0, -1), L"tes");
    EXPECT_EQ(testw.substr(-2), L"st");
    EXPECT_EQ(testw.substr(-3, 2), L"es");
    EXPECT_EQ(testw.substr(-3, -1), L"es");
    EXPECT_EQ(testw.str_mid(1), L"est");
    EXPECT_EQ(testw.str_mid(1, 0), L"");
    EXPECT_EQ(testw.str_mid(1, 2), L"es");
    EXPECT_EQ(testw.str_mid(2, 2), L"st");
}

TEST(SimStr, ToInt) {
    EXPECT_EQ(ssa{"  123"}.as_int<int>(), 123);
    EXPECT_EQ(ssa{"  123"}(0, -1).as_int<int>(), 12);
    EXPECT_EQ(ssa{"+123"}.as_int<int>(), 123);
    EXPECT_EQ(ssa{"  -123aa"}.as_int<int>(), -123);
    EXPECT_EQ(ssa{"123"}.as_int<size_t>(), 123u);
    EXPECT_EQ(ssa{"-123"}.as_int<size_t>(), 0u);
    EXPECT_EQ(ssa{"   0123  "}.as_int<int>(), 0123);
    EXPECT_EQ(ssa{"-0x123"}.as_int<int>(), -0x123);

    {
        auto [number, err, len] = ssa{"asd"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::NotNumber);
        EXPECT_EQ(len, 0u);
    }

    {
        auto [number, err, len] = ssa{"0"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 1u);
    }

    {
        auto [number, err, len] = ssa{"1"}.to_int<int>();
        EXPECT_EQ(number, 1);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 1u);
    }

    {
        auto [number, err, len] = ssa{"-"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::NotNumber);
        EXPECT_EQ(len, 1u);
    }

    {
        auto [number, err, len] = ssa{"+"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::NotNumber);
        EXPECT_EQ(len, 1u);
    }

    {
        auto [number, err, len] = ssa{"-0"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 2u);
    }

    {
        auto [number, err, len] = ssa{"+0"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 2u);
    }

    {
        auto [number, err, len] = ssa{"-0sd"}.to_int<int>();
        EXPECT_EQ(number, 0);
        EXPECT_EQ(err, IntConvertResult::BadSymbolAtTail);
        EXPECT_EQ(len, 2u);
    }
    {
        auto [number, err, len] = ssa{"1234"}.to_int<int8_t>();
        EXPECT_EQ(err, IntConvertResult::Overflow);
        EXPECT_EQ(len, 4u);
    }
    {
        auto [number, err, len] = ssa{"128"}.to_int<int8_t>();
        EXPECT_EQ(err, IntConvertResult::Overflow);
        EXPECT_EQ(len, 3u);
    }
    {
        auto [number, err, len] = ssa{"127"}.to_int<int8_t>();
        EXPECT_EQ(number, 127);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 3u);
    }
    {
        auto [number, err, len] = ssa{"-128"}.to_int<int8_t>();
        EXPECT_EQ(number, -128);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 4u);
    }

    {
        auto [number, err, len] = ssa{"0xFFFfffFF"}.to_int<size_t>();
        EXPECT_EQ(number, 0xFFFFFFFF);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 10u);
    }

    {
        auto [number, err, len] = ssa{"0x"}.to_int<size_t>();
        EXPECT_EQ(number, 0u);
        EXPECT_EQ(err, IntConvertResult::NotNumber);
        EXPECT_EQ(len, 2u);
    }

    {
        auto [number, err, len] = ssa{"0xS"}.to_int<size_t>();
        EXPECT_EQ(number, 0u);
        EXPECT_EQ(err, IntConvertResult::NotNumber);
        EXPECT_EQ(len, 2u);
    }

    {
        auto [number, err, len] = ssa{"-0x80000000"}.to_int<int>();
        EXPECT_EQ(number, -2147483648);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 11u);
    }

    {
        auto [number, err, len] = ssa{"-0x80000001"}.to_int<int>();
        EXPECT_EQ(err, IntConvertResult::Overflow);
        EXPECT_EQ(len, 11u);
    }

    {
        auto [number, err, len] = ssa{"-0x80000001"}.to_int<int64_t>();
        EXPECT_EQ(number, -2147483649);
        EXPECT_EQ(err, IntConvertResult::Success);
        EXPECT_EQ(len, 11u);
    }

    EXPECT_EQ(ssu{u"  123"}.as_int<int>(), 123);
    EXPECT_EQ(ssu{u"+123"}.as_int<int>(), 123);
    EXPECT_EQ(ssu{u"  -123aa"}.as_int<int>(), -123);
    EXPECT_EQ(ssu{u"123"}.as_int<size_t>(), 123u);
    EXPECT_EQ(ssu{u"-123"}.as_int<size_t>(), 0u);
    EXPECT_EQ(ssu{u"   0123  "}.as_int<int>(), 0123);
    EXPECT_EQ(ssu{u"-0x123"}.as_int<int>(), -0x123);

    EXPECT_EQ(ssw{L"  123"}.as_int<int>(), 123);
    EXPECT_EQ(ssw{L"+123"}.as_int<int>(), 123);
    EXPECT_EQ(ssw{L"  -123aa"}.as_int<int>(), -123);
    EXPECT_EQ(ssw{L"123"}.as_int<size_t>(), 123u);
    EXPECT_EQ(ssw{L"-123"}.as_int<size_t>(), 0u);
    EXPECT_EQ(ssw{L"   0123  "}.as_int<int>(), 0123);
    EXPECT_EQ(ssw{L"-0x123"}.as_int<int>(), -0x123);

    int num1;
    uint64_t num2;
    double num3;
    ssa{"-10"}.as_number(num1);
    EXPECT_EQ(num1, -10);
    ssa{"0xDeadBeef"}.as_number(num2);
    EXPECT_EQ(num2, 0xdeadbeef);
    ssa{"0.128"}.as_number(num3);
    EXPECT_EQ(num3, 0.128);
}

TEST(SimStr, to_double) {
    EXPECT_EQ(ssa{"  123"}.to_double(), 123.0);
    EXPECT_EQ(ssa{"  123.1"}.to_double(), 123.1);
}

TEST(SimStr, Split) {
    auto res = ssa{"1,2,3"}.split<std::vector<ssa>>(",");
    EXPECT_EQ(res.size(), 3u);
    EXPECT_EQ(res[0], "1");
    EXPECT_EQ(res[1], "2");
    EXPECT_EQ(res[2], "3");
}

TEST(SimStr, Parts) {
    ssa test = "prefixВСтрокеSuffix";

    EXPECT_TRUE(test.starts_with("prefix"));
    EXPECT_TRUE(test.ends_with("Suffix"));
    EXPECT_TRUE(ssa{"prefix"}.prefix_in(test));
    EXPECT_FALSE(ssa{"prefix"}.starts_with("prefixa"));

    EXPECT_FALSE(test.starts_with("predix"));
    EXPECT_FALSE(test.ends_with("suffex"));
    EXPECT_FALSE(ssa{"predix"}.prefix_in(test));

    EXPECT_TRUE(test.starts_with_ia("Prefix"));
    EXPECT_TRUE(test.ends_with_ia("suffix"));

    EXPECT_TRUE(test.starts_with_iu("Prefixвс"));
    EXPECT_TRUE(test.ends_with_iu("КЕsuffix"));
}

TEST(SimStr, IsAscii) {
    ssa test = "prefixВСтрокеSuffix";

    EXPECT_FALSE(test.is_ascii());
    EXPECT_TRUE(ssa{"1234567812345678124"}.is_ascii());
    EXPECT_TRUE(ssa{"asldkafjk^%&*&623216187263&^hjgaiu&&!^@*^?>"}.is_ascii());
}

TEST(SimStr, ChangeCase) {
    EXPECT_EQ(ssa{"TEST"}.uppered_only_ascii<stringa>(), "TEST");
    EXPECT_EQ(ssa{"Test"}.uppered_only_ascii<stringa>(), "TEST");
    EXPECT_EQ(ssa{"test"}.lowered_only_ascii<stringa>(), "test");
    EXPECT_EQ(ssa{"Test"}.lowered_only_ascii<stringa>(), "test");
    EXPECT_EQ(ssa{"TestПрОвЕрКа"}.uppered_only_ascii<stringa>(), "TESTПрОвЕрКа");
    EXPECT_EQ(ssa{"TestПрОвЕрКа"}.lowered_only_ascii<stringa>(), "testПрОвЕрКа");
    EXPECT_EQ(ssa{"TestПрОвЕрКа"}.uppered<stringa>(), "TESTПРОВЕРКА");
    EXPECT_EQ(ssa{"tesTПрОвЕрКа"}.lowered<stringa>(), "testпроверка");
    EXPECT_EQ(ssa{"TEST"}.uppered<stringa>(), "TEST");
    EXPECT_EQ(ssa{"test"}.lowered<stringa>(), "test");
    EXPECT_EQ(ssa{"TESTПРОВЕРКА"}.uppered<stringa>(), "TESTПРОВЕРКА");
    EXPECT_EQ(ssa{"testпроверка"}.lowered<stringa>(), "testпроверка");
    EXPECT_EQ(ssa{"tesTİiȺⱥȾⱦẞßΩФывAsd"}.lowered<stringa>(), "testiiⱥⱥⱦⱦßßωфывasd");
    EXPECT_EQ(ssa{"testⱢɱⱮɽⱤWas"}.uppered<stringa>(), "TESTⱢⱮⱮⱤⱤWAS");

    EXPECT_EQ(ssu{u"TEST"}.uppered_only_ascii<stringu>(), u"TEST");
    EXPECT_EQ(ssu{u"test"}.lowered_only_ascii<stringu>(), u"test");
    EXPECT_EQ(ssu{u"Test"}.uppered_only_ascii<stringu>(), u"TEST");
    EXPECT_EQ(ssu{u"Test"}.lowered_only_ascii<stringu>(), u"test");
    EXPECT_EQ(ssu{u"TestПрОвЕрКа"}.uppered_only_ascii<stringu>(), u"TESTПрОвЕрКа");
    EXPECT_EQ(ssu{u"TestПрОвЕрКа"}.lowered_only_ascii<stringu>(), u"testПрОвЕрКа");
    EXPECT_EQ(ssu{u"TestПрОвЕрКа"}.uppered<stringu>(), u"TESTПРОВЕРКА");
    EXPECT_EQ(ssu{u"tesTПрОвЕрКа"}.lowered<stringu>(), u"testпроверка");
    EXPECT_EQ(ssu{u"tesTİiȺⱥȾⱦẞßΩФывAsd"}.lowered<stringu>(), u"testiiⱥⱥⱦⱦßßωфывasd");
    EXPECT_EQ(ssu{u"testⱢɱⱮɽⱤWas"}.uppered<stringu>(), u"TESTⱢⱮⱮⱤⱤWAS");

    EXPECT_EQ(ssw{L"TEST"}.uppered_only_ascii<stringw>(), L"TEST");
    EXPECT_EQ(ssw{L"test"}.lowered_only_ascii<stringw>(), L"test");
    EXPECT_EQ(ssw{L"Test"}.uppered_only_ascii<stringw>(), L"TEST");
    EXPECT_EQ(ssw{L"Test"}.lowered_only_ascii<stringw>(), L"test");
    EXPECT_EQ(ssw{L"TestПрОвЕрКа"}.uppered_only_ascii<stringw>(), L"TESTПрОвЕрКа");
    EXPECT_EQ(ssw{L"TestПрОвЕрКа"}.lowered_only_ascii<stringw>(), L"testПрОвЕрКа");
    EXPECT_EQ(ssw{L"TestПрОвЕрКа"}.uppered<stringw>(), L"TESTПРОВЕРКА");
    EXPECT_EQ(ssw{L"tesTПрОвЕрКа"}.lowered<stringw>(), L"testпроверка");
    EXPECT_EQ(ssw{L"tesTİiȺⱥȾⱦẞßΩФывAsd"}.lowered<stringw>(), L"testiiⱥⱥⱦⱦßßωфывasd");
    EXPECT_EQ(ssw{L"testⱢɱⱮɽⱤWas"}.uppered<stringw>(), L"TESTⱢⱮⱮⱤⱤWAS");
}

TEST(SimStr, Replace) {
    EXPECT_EQ(ssa{"testing"}.replaced<stringa>("t", "--"), "--es--ing");
    EXPECT_EQ(ssa{"testing"}.replaced<stringa>("t", ""), "esing");
    EXPECT_EQ(stringa{ssa{"testing"}.replace_init("t", "--")}, "--es--ing");
}

TEST(SimStr, Trim) {
    EXPECT_EQ(ssa{"testing"}.trimmed(), "testing");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed(), "testing");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_left(), "testing  ");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_right(), "  \ttesting");
    EXPECT_EQ(ssa{"testing"}.trimmed("tg"), "estin");
    EXPECT_EQ(ssa{"testing"}.trimmed_left("tg"), "esting");
    EXPECT_EQ(ssa{"testing"}.trimmed_right("tg"), "testin");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_with_spaces("tg"), "estin");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_left_with_spaces("tg"), "esting  ");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_right_with_spaces("tg"), "  \ttestin");
    EXPECT_EQ(ssa{"testing"}.trimmed(ssa{"tg"}), "estin");
    EXPECT_EQ(ssa{"testing"}.trimmed_left(ssa{"tg"}), "esting");
    EXPECT_EQ(ssa{"testing"}.trimmed_right(ssa{"tg"}), "testin");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_with_spaces(ssa{"tg"}), "estin");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_left_with_spaces(ssa{"tg"}), "esting  ");
    EXPECT_EQ(ssa{"  \ttesting  "}.trimmed_right_with_spaces(ssa{"tg"}), "  \ttestin");
}

TEST(SimStr, CreateSstringEmpty) {

    stringa testa;
    EXPECT_EQ(testa.length(), 0u);
    EXPECT_EQ(testa, "");
    EXPECT_TRUE(testa.is_empty());

    stringu testu;
    EXPECT_EQ(testu.length(), 0u);
    EXPECT_EQ(testu, u"");
    EXPECT_TRUE(testu.is_empty());

    stringw testw;
    EXPECT_EQ(testw.length(), 0u);
    EXPECT_EQ(testw, L"");
    EXPECT_TRUE(testw.is_empty());
}

TEST(SimStr, CreateSstringFromLiteral) {

    stringa testa{"testФыва"};
    EXPECT_EQ(testa, "testФыва");
    EXPECT_FALSE(testa.is_empty());

    stringu testu{u"test"};
    EXPECT_EQ(testu, u"test");
    EXPECT_FALSE(testu.is_empty());

    stringw testw{L"test"};
    EXPECT_EQ(testw, L"test");
    EXPECT_FALSE(testw.is_empty());
}

TEST(SimStr, CreateSstringCopy) {

    stringa testa{"test"};
    stringa copya{testa};
    EXPECT_EQ(copya, testa);
    EXPECT_EQ(copya, "test");
    EXPECT_FALSE(testa.is_empty());
    EXPECT_FALSE(copya.is_empty());

    stringu testu{u"test"};
    stringu copyw{testu};
    EXPECT_EQ(copyw, testu);
    EXPECT_EQ(copyw, u"test");
    EXPECT_FALSE(testu.is_empty());
    EXPECT_FALSE(copyw.is_empty());

    stringw testw{L"test"};
    stringw copyu{testw};
    EXPECT_EQ(copyu, testw);
    EXPECT_EQ(copyu, L"test");
    EXPECT_FALSE(testw.is_empty());
    EXPECT_FALSE(copyu.is_empty());
}

TEST(SimStr, CreateSstringCopy1) {
    {
        Tstringa sample = "sample";
        stra ptov = sample;
        EXPECT_TRUE(sample.isConstantString());
        Tstringa copy1{sample};
        EXPECT_TRUE(copy1.isConstantString());
        EXPECT_TRUE(sample.to_str().is_same(copy1.to_str()));
        EXPECT_EQ(sample.sharedCount(), 0u);
        EXPECT_EQ(copy1.sharedCount(), 0u);
    }
    {
        Tstringa sample{2, "sample"};
        EXPECT_FALSE(sample.isConstantString());
        EXPECT_TRUE(sample.isLocalString());
        Tstringa copy1{sample};
        EXPECT_TRUE(copy1.isLocalString());
        EXPECT_FALSE(sample.to_str().is_same(copy1.to_str()));
        EXPECT_EQ(sample.sharedCount(), 0u);
        EXPECT_EQ(copy1.sharedCount(), 0u);
    }
    {
        Tstringa sample{10, "sample"};
        EXPECT_FALSE(sample.isConstantString());
        EXPECT_FALSE(sample.isLocalString());
        EXPECT_TRUE(sample.isSharedString());
        EXPECT_EQ(sample.sharedCount(), 1u);
        Tstringa copy1{sample};
        EXPECT_TRUE(copy1.isSharedString());
        EXPECT_TRUE(sample.to_str().is_same(copy1.to_str()));
        EXPECT_EQ(sample.sharedCount(), 2u);
        EXPECT_EQ(copy1.sharedCount(), 2u);
    }

    {
        lstringsa<10> sample{10, "sample"};
        EXPECT_FALSE(sample.is_empty());
        Tstringa copy1{sample};
        EXPECT_FALSE(sample.is_empty());
        EXPECT_TRUE(copy1.isSharedString());
        EXPECT_EQ(copy1.sharedCount(), 1u);
    }

    {
        lstringsa<10> sample{10, "sample"};
        EXPECT_FALSE(sample.is_empty());
        Tstringa copy1{std::move(sample)};
        EXPECT_TRUE(sample.is_empty());
        EXPECT_TRUE(copy1.isSharedString());
        EXPECT_EQ(copy1.sharedCount(), 1u);
        Tstringa new_copy = copy1;
        EXPECT_TRUE(new_copy.isSharedString());
        EXPECT_EQ(copy1.sharedCount(), 2u);
        EXPECT_EQ(new_copy.sharedCount(), 2u);
        EXPECT_EQ(copy1.c_str(), new_copy.c_str());
    }
}

TEST(SimStr, CreateSstringMove) {

    stringa testa{"test"};
    stringa copya{std::move(testa)};
    EXPECT_NE(copya, testa);
    EXPECT_EQ(copya, "test");
    EXPECT_TRUE(testa.is_empty());
    EXPECT_FALSE(copya.is_empty());

    stringu testu{u"test"};
    stringu copyu{std::move(testu)};
    EXPECT_NE(copyu, testu);
    EXPECT_EQ(copyu, u"test");
    EXPECT_TRUE(testu.is_empty());
    EXPECT_FALSE(copyu.is_empty());

    stringw testw{L"test"};
    stringw copyw{std::move(testw)};
    EXPECT_NE(copyw, testw);
    EXPECT_EQ(copyw, L"test");
    EXPECT_TRUE(testw.is_empty());
    EXPECT_FALSE(copyw.is_empty());
}

TEST(SimStr, CreateSstringChars) {

    stringa testa{4, 'a'};
    EXPECT_EQ(testa, "aaaa");

    stringu testu{4, u'a'};
    EXPECT_EQ(testu, u"aaaa");

    stringw testw{4, L'a'};
    EXPECT_EQ(testw, L"aaaa");
}

TEST(SimStr, CreateSstringsimple_string) {

    ssa sa{"test"};
    stringa testa{sa};
    EXPECT_EQ(testa, "test");

    ssu sw{u"test"};
    stringu testu{sw};
    EXPECT_EQ(testu, u"test");

    ssw su{L"test"};
    stringw testw{su};
    EXPECT_EQ(testw, L"test");
}

TEST(SimStr, CreateSstringsimple_stringRepeat) {

    ssa sa{"test"};
    stringa testa{3, sa};
    EXPECT_EQ(testa, "testtesttest");

    ssu sw{u"test"};
    stringu testu{3, sw};
    EXPECT_EQ(testu, u"testtesttest");

    ssw su{L"test"};
    stringw testw{3, su};
    EXPECT_EQ(testw, L"testtesttest");
}

TEST(SimStr, CreateSstringLong) {
    ssa sa{"test"};
    stringa testa{10, sa};
    EXPECT_EQ(testa, "testtesttesttesttesttesttesttesttesttest");
}

TEST(SimStr, CreateSstringStrExpr) {
    EXPECT_EQ(stringa{"test"_ss + 101 + e_c(3, 'a')}, "test101aaa");
    EXPECT_EQ(stringu{u"test"_ss + 1234 + e_c(3, u'a')}, u"test1234aaa");
    EXPECT_EQ(stringw{L"test"_ss + 12345 + e_c(3, L'a')}, L"test12345aaa");
}

TEST(SimStr, CreateSstringReplace) {
    EXPECT_EQ(stringa(ssa("tratata"), "t", ""), "raaa");
    EXPECT_EQ(stringa(ssa("tratata"), "t", "-"), "-ra-a-a");
    EXPECT_EQ(stringa(ssa("tratata"), "t", ">>"), ">>ra>>a>>a");

    EXPECT_EQ(stringu(ssu(u"tratata"), u"t", u""), u"raaa");
    EXPECT_EQ(stringu(ssu(u"tratata"), u"t", u"-"), u"-ra-a-a");
    EXPECT_EQ(stringu(ssu(u"tratata"), u"t", u">>"), u">>ra>>a>>a");

    EXPECT_EQ(stringw(ssw(L"tratata"), L"t", L""), L"raaa");
    EXPECT_EQ(stringw(ssw(L"tratata"), L"t", L"-"), L"-ra-a-a");
    EXPECT_EQ(stringw(ssw(L"tratata"), L"t", L">>"), L">>ra>>a>>a");
}

TEST(SimStr, AssignSstring) {
    stringa test{"test"};
    EXPECT_EQ(test = test, "test");
    EXPECT_EQ(test = "next", "next");
    EXPECT_EQ(test = ssa{"other"}, "other");
    EXPECT_EQ(test = stringa{"trtr"_ss + 10}, "trtr10");
    EXPECT_EQ(test = "trtr"_ss + 20, "trtr20");
    EXPECT_EQ(test = test(2), "tr20");
    EXPECT_EQ(test = lstringa<10>{"func"}, "func");
    EXPECT_EQ(test = lstringsa<10>{"func"}, "func");
    lstringsa<10> sample{15, "1234"};
    Tstringa t = sample;
    EXPECT_TRUE(t.isSharedString());
    EXPECT_EQ(t.sharedCount(), 1u);
    EXPECT_FALSE(sample.is_empty());
    EXPECT_EQ(sample.length(), 60u);
    EXPECT_EQ(sample, t.to_str());

    t = std::move(sample);
    EXPECT_TRUE(t.isSharedString());
    EXPECT_EQ(t.sharedCount(), 1u);
    EXPECT_TRUE(sample.is_empty());
    EXPECT_EQ(sample.symbols()[0], 0);
    EXPECT_EQ(t.length(), 60u);
}

TEST(SimStr, StrJoin) {
    EXPECT_EQ(stringa::join(std::vector<ssa>{}, "-"), "");
    EXPECT_EQ(stringa::join(std::vector<ssa>{"abc"}, "-"), "abc");
    EXPECT_EQ(stringa::join(std::vector<ssa>{"abc"}, "-", true), "abc-");
    EXPECT_EQ(stringa::join(std::vector<ssa>{"abc", "def", "ghi"}, "-"), "abc-def-ghi");
    EXPECT_EQ(stringa::join(std::vector<ssa>{"abc", "def", "ghi"}, "-", true), "abc-def-ghi-");
}

TEST(SimStr, LStringCreate) {
    lstringa<40> test;
    EXPECT_TRUE(test.is_empty());
    EXPECT_EQ(test, "");
    EXPECT_EQ(test.length(), 0u);
}

TEST(SimStr, LStringCreateLiteral) {
    lstringa<40> test{"test"};
    EXPECT_FALSE(test.is_empty());
    EXPECT_EQ(test, "test");
    EXPECT_EQ(test.length(), 4u);
}

TEST(SimStr, LStringCreatesimple_str) {
    lstringa<40> test{ssa{"test"}};
    EXPECT_FALSE(test.is_empty());
    EXPECT_EQ(test, "test");
    EXPECT_EQ(test.length(), 4u);
}

TEST(SimStr, LStringCreateCopy) {
    lstringa<40> test{"test"};
    lstringa<40> copy{test};
    EXPECT_EQ(test, copy);
    EXPECT_EQ(copy, "test");
}

TEST(SimStr, LStringCreateMove) {
    lstringa<40> test{"test"};
    lstringa<40> copy{std::move(test)};
    EXPECT_NE(test, copy);
    EXPECT_EQ(copy, "test");
    EXPECT_TRUE(test.is_empty());
    EXPECT_FALSE(copy.is_empty());
    EXPECT_EQ(test.symbols()[0], 0);
}

TEST(SimStr, LStringCreateCopyOtherSize) {
    lstringa<10> test{"test"};
    lstringa<40> copy{test};
    EXPECT_EQ(test.to_str(), copy);
    EXPECT_EQ(copy, "test");
}

TEST(SimStr, LStringCreatePad) {
    lstringa<10> test{5, 'a'};
    EXPECT_EQ(test, "aaaaa");
}

TEST(SimStr, LStringCreateExpr) {
    lstringa<40> test{"test" + e_num<u8s>(10) + "-" + 20 + e_spca<3>()};
    EXPECT_FALSE(test.is_empty());
    EXPECT_EQ(test, "test10-20   ");
}

TEST(SimStr, LStringCreateFunc) {
    EXPECT_EQ(lstringa<20>{[](auto& t) { t = "test";}}, "test");
    EXPECT_EQ(lstringa<20>{[](auto p, size_t l) {
        EXPECT_EQ(l, lstringa<20>::LocalCapacity);
        memcpy(p, "test", std::size("test") - 1);
        return std::size("test") - 1;
    }}, "test");
}

TEST(SimStr, LStringCreateSstring) {
    {
        lstringa<40> test{stringa{"test"}};
        EXPECT_FALSE(test.is_empty());
        EXPECT_EQ(test, "test");
        EXPECT_EQ(test.length(), 4u);
    }
    {
        stringa t{"test"};
        lstringa<40> test{t};
        EXPECT_FALSE(test.is_empty());
        EXPECT_FALSE(t.is_empty());
        EXPECT_EQ(test, "test");
        EXPECT_EQ(test.length(), 4u);
    }
}

TEST(SimStr, LStringAssign) {
    {
        lstringa<40> test{"test"};
        EXPECT_EQ(test, "test");
        test = "";
        EXPECT_EQ(test, "");
        test = "test";
        EXPECT_EQ(test, "test");
    }

    {
        lstringa<40> test{"test"};
        EXPECT_EQ(test, "test");
        const auto& t = test;
        test = t;
        EXPECT_EQ(test, "test");

        test = lstringa<1>{"next step"};
        EXPECT_EQ(test, "next step");

        test = test(0);
        EXPECT_EQ(test, "next step");

        test = test(1, 2);
        EXPECT_EQ(test, "ex");

        test = e_c(100, 'a');
        EXPECT_EQ(test.length(), 100u);

        lstringa<40> src{"test"};
        test = std::move(src);
        EXPECT_EQ(test, "test");
        EXPECT_TRUE(src.is_empty());

        test = e_spca<3>();
        EXPECT_EQ(test, "   ");
    }
}

TEST(SimStr, UtfConvert) {
    EXPECT_EQ(stringa{ssu{u"testпройден"}}, "testпройден");
    EXPECT_EQ(stringa{ssw{L"testпройден"}}, "testпройден");
    EXPECT_EQ(stringa{stringu{u"testпройден"}}, "testпройден");
    EXPECT_EQ(stringa{stringw{L"testпройден"}}, "testпройден");
    EXPECT_EQ(stringa{lstringu<40>{u"testпройден"}}, "testпройден");
    EXPECT_EQ(stringa{lstringw<40>{L"testпройден"}}, "testпройден");

    EXPECT_EQ(stringu{ssa{"testпройден"}}, u"testпройден");
    EXPECT_EQ(stringu{ssw{L"testпройден"}}, u"testпройден");
    EXPECT_EQ(stringu{stringa{"testпройден"}}, u"testпройден");
    EXPECT_EQ(stringu{stringw{L"testпройден"}}, u"testпройден");
    EXPECT_EQ(stringu{lstringa<40>{"testпройден"}}, u"testпройден");
    EXPECT_EQ(stringu{lstringw<40>{L"testпройден"}}, u"testпройден");

    EXPECT_EQ(stringw{ssa{"testпройден"}}, L"testпройден");
    EXPECT_EQ(stringw{ssu{u"testпройден"}}, L"testпройден");
    EXPECT_EQ(stringw{stringa{"testпройден"}}, L"testпройден");
    EXPECT_EQ(stringw{stringu{u"testпройден"}}, L"testпройден");
    EXPECT_EQ(stringw{lstringa<40>{"testпройден"}}, L"testпройден");
    EXPECT_EQ(stringw{lstringu<40>{u"testпройден"}}, L"testпройден");

    EXPECT_EQ(lstringa<10>{ssu{u"testпройден"}}, "testпройден");
    EXPECT_EQ(lstringa<10>{ssw{L"testпройден"}}, "testпройден");
    EXPECT_EQ(lstringa<10>{stringu{u"testпройден"}}, "testпройден");
    EXPECT_EQ(lstringa<10>{stringw{L"testпройден"}}, "testпройден");
    EXPECT_EQ(lstringa<10>{lstringu<40>{u"testпройден"}}, "testпройден");
    EXPECT_EQ(lstringa<10>{lstringw<40>{L"testпройден"}}, "testпройден");

    EXPECT_EQ(lstringu<10>{ssa{"testпройден"}}, u"testпройден");
    EXPECT_EQ(lstringu<10>{ssw{L"testпройден"}}, u"testпройден");
    EXPECT_EQ(lstringu<10>{stringa{"testпройден"}}, u"testпройден");
    EXPECT_EQ(lstringu<10>{stringw{L"testпройден"}}, u"testпройден");
    EXPECT_EQ(lstringu<10>{lstringa<40>{"testпройден"}}, u"testпройден");
    EXPECT_EQ(lstringu<10>{lstringw<40>{L"testпройден"}}, u"testпройден");

    EXPECT_EQ(lstringw<10>{ssa{"testпройден"}}, L"testпройден");
    EXPECT_EQ(lstringw<10>{ssu{u"testпройден"}}, L"testпройден");
    EXPECT_EQ(lstringw<10>{stringa{"testпройден"}}, L"testпройден");
    EXPECT_EQ(lstringw<10>{stringu{u"testпройден"}}, L"testпройден");
    EXPECT_EQ(lstringw<10>{lstringa<40>{"testпройден"}}, L"testпройден");
    EXPECT_EQ(lstringw<10>{lstringu<40>{u"testпройден"}}, L"testпройден");
}

TEST(SimStr, LStrChangeCase) {
    EXPECT_EQ(lstringa<20>{"Test"}.upper_only_ascii(), "TEST");
    EXPECT_EQ(lstringa<20>{"Test"}.lower_only_ascii(), "test");
    EXPECT_EQ(lstringa<20>{"TestПрОвЕрКа"}.upper_only_ascii(), "TESTПрОвЕрКа");
    EXPECT_EQ(lstringa<20>{"TestПрОвЕрКа"}.lower_only_ascii(), "testПрОвЕрКа");
    EXPECT_EQ(lstringa<20>{"TestПрОвЕрКа"}.upper(), "TESTПРОВЕРКА");
    EXPECT_EQ(lstringa<20>{"tesTПрОвЕрКа"}.lower(), "testпроверка");
    EXPECT_EQ(lstringa<20>{"tesTİiȺⱥȾⱦẞßΩФывAsd"}.lower(), "testiiⱥⱥⱦⱦßßωфывasd");
    EXPECT_EQ(lstringa<20>{"testⱢɱⱮɽⱤWas"}.upper(), "TESTⱢⱮⱮⱤⱤWAS");

    EXPECT_EQ(lstringu<20>{u"Test"}.upper_only_ascii(), u"TEST");
    EXPECT_EQ(lstringu<20>{u"Test"}.lower_only_ascii(), u"test");
    EXPECT_EQ(lstringu<20>{u"TestПрОвЕрКа"}.upper_only_ascii(), u"TESTПрОвЕрКа");
    EXPECT_EQ(lstringu<20>{u"TestПрОвЕрКа"}.lower_only_ascii(), u"testПрОвЕрКа");
    EXPECT_EQ(lstringu<20>{u"TestПрОвЕрКа"}.upper(), u"TESTПРОВЕРКА");
    EXPECT_EQ(lstringu<20>{u"tesTПрОвЕрКа"}.lower(), u"testпроверка");
    EXPECT_EQ(lstringu<20>{u"tesTİiȺⱥȾⱦẞßΩФывAsd"}.lower(), u"testiiⱥⱥⱦⱦßßωфывasd");
    EXPECT_EQ(lstringu<20>{u"testⱢɱⱮɽⱤWas"}.upper(), u"TESTⱢⱮⱮⱤⱤWAS");
}

TEST(SimStr, LStrTrim) {
    EXPECT_EQ(lstringa<20>{"testing"}.trim(), "testing");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim(), "testing");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_left(), "testing  ");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_right(), "  \ttesting");
    EXPECT_EQ(lstringa<20>{"testing"}.trim("tg"), "estin");
    EXPECT_EQ(lstringa<20>{"testing"}.trim_left("tg"), "esting");
    EXPECT_EQ(lstringa<20>{"testing"}.trim_right("tg"), "testin");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_with_spaces("tg"), "estin");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_left_with_spaces("tg"), "esting  ");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_right_with_wpaces("tg"), "  \ttestin");
    EXPECT_EQ(lstringa<20>{"testing"}.trim(ssa{"tg"}), "estin");
    EXPECT_EQ(lstringa<20>{"testing"}.trim_left(ssa{"tg"}), "esting");
    EXPECT_EQ(lstringa<20>{"testing"}.trim_right(ssa{"tg"}), "testin");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_with_spaces(ssa{"tg"}), "estin");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_left_with_spaces(ssa{"tg"}), "esting  ");
    EXPECT_EQ(lstringa<20>{"  \ttesting  "}.trim_right_with_spaces(ssa{"tg"}), "  \ttestin");
}

TEST(SimStr, LStrAppend) {
    EXPECT_EQ(lstringa<20>{"test"}.append("ing"), "testing");
    EXPECT_EQ(lstringa<20>{"test"}.append("ing"_ss + 10), "testing10");
    EXPECT_EQ(lstringa<20>{"test"}.append_in(3, "ing"), "tesing");
    EXPECT_EQ(lstringa<20>{"test"}.append_in(3, "ing"_ss + 10), "tesing10");
    EXPECT_EQ(lstringa<20>{"test"} += "ing", "testing");
    EXPECT_EQ(lstringa<20>{"test"} += "ing"_ss + 10, "testing10");
}

TEST(SimStr, LStrChange) {
    EXPECT_EQ(lstringa<20>{"test"}.insert(1, "---"), "t---est");
    EXPECT_EQ(lstringa<20>{"test"}.insert(1, eea + "---"), "t---est");
    EXPECT_EQ(lstringa<20>{"test"}.insert(100, "---"), "test---");
    EXPECT_EQ(lstringa<20>{"test"}.prepend("---"), "---test");
    EXPECT_EQ(lstringa<3>{"test"}.prepend(eea + "---"), "---test");
    EXPECT_EQ(lstringa<4>{"test"}.change(1, 2, "---"), "t---t");
    EXPECT_EQ(lstringa<20>{"test"}.change(1, 2, eea + "---"), "t---t");
    EXPECT_EQ(lstringa<0>{"test"}.change(100, 20, "---"), "test---");
    EXPECT_EQ(lstringa<1>{"test"}.remove(1, 2), "tt");
    EXPECT_EQ(lstringa<20>{""}.change(1, 5, "---"), "---");

    EXPECT_EQ(lstringu<20>{u"test"}.insert(1, u"---"), u"t---est");
    EXPECT_EQ(lstringu<20>{u"test"}.insert(1, eeu + u"---"), u"t---est");
    EXPECT_EQ(lstringu<20>{u"test"}.insert(100, u"---"), u"test---");
    EXPECT_EQ(lstringu<20>{u"test"}.prepend(u"---"), u"---test");
    EXPECT_EQ(lstringu<3>{u"test"}.prepend(eeu + u"---"), u"---test");
    EXPECT_EQ(lstringu<4>{u"test"}.change(1, 2, u"---"), u"t---t");
    EXPECT_EQ(lstringu<20>{u"test"}.change(1, 2, eeu + u"---"), u"t---t");
    EXPECT_EQ(lstringu<0>{u"test"}.change(100, 20, u"---"), u"test---");
    EXPECT_EQ(lstringu<1>{u"test"}.remove(1, 2), u"tt");
    EXPECT_EQ(lstringu<20>{u""}.change(1, 5, u"---"), u"---");
}

TEST(SimStr, ExprNum) {
    EXPECT_EQ(lstringa<20>{eea + std::numeric_limits<int16_t>::min()}, "-32768");
    EXPECT_EQ(lstringa<20>{eea + (std::numeric_limits<int16_t>::min() + 1)}, "-32767");
    EXPECT_EQ(lstringa<20>{eea + std::numeric_limits<int32_t>::min()}, "-2147483648");
}

TEST(SimStr, LStrSelfReplace) {
    {
        lstringa<40> test = "test string";
        ssa before = test;
        test.replace("aa", "asd");
        EXPECT_EQ(test, "test string");
        EXPECT_EQ(before.str, test.to_str().str);
    }
    {
        lstringa<40> test = "test string";
        ssa before = test;
        test.replace("st", "asd");
        EXPECT_EQ(test, "teasd asdring");
        EXPECT_EQ(before.str, test.to_str().str);
    }
    {
        lstringa<40> test = "test string";
        ssa before = test;
        test.replace("st", "a");
        EXPECT_EQ(test, "tea aring");
        EXPECT_EQ(before.str, test.to_str().str);
    }

    {
        lstringa<40> test = "test string";
        ssa before = test;
        test.replace("st", "--");
        EXPECT_EQ(test, "te-- --ring");
        EXPECT_EQ(before.str, test.to_str().str);
    }

    {
        lstringa<11> test = "test string1234";
        ssa before = test;
        test.replace("st", "---");
        EXPECT_EQ(test, "te--- ---ring1234");
        // buffer changed from local to allocated
        EXPECT_NE(before.str, test.to_str().str);
        test.replace("---", "s");
        EXPECT_EQ(test, "tes sring1234");
        // buffer not changed from allocated to local
        EXPECT_NE(before.str, test.to_str().str);
        test.shrink_to_fit();
        // buffer changed from allocated to local
        EXPECT_EQ(before.str, test.to_str().str);
    }

    {
        lstringa<0> buffer;
        buffer.replace_from(ssa{"test string"}, "st", "---");
        EXPECT_EQ(buffer, "te--- ---ring");
    }

    {
        lstringa<40> buffer;
        buffer.replace_from(ssa{"test string"}, "st", "--");
        EXPECT_EQ(buffer, "te-- --ring");
    }

    {
        lstringa<40> buffer;
        buffer.replace_from(ssa{"test string"}, "st", "-");
        EXPECT_EQ(buffer, "te- -ring");
    }
}

TEST(SimStr, LStrSelfFuncFill) {
    size_t count = 0, first = 0, second = 0;
    lstringa<5> test;
    test << [&](auto p, auto l) {
        if (!count) {
            first = l;
        } else {
            second = l;
        }
        count++;
        if (l >= 10) {
            memset(p, 'a', 10);
        }
        return 10;
    };
    EXPECT_EQ(count, 2);
    EXPECT_EQ(first, lstringa<5>::LocalCapacity);
    EXPECT_EQ(second, 15);
    EXPECT_EQ(test, "aaaaaaaaaa");

    count = 0, first = 0, second = 0;
    test <<= [&](auto p, auto l) {
        if (!count) {
            first = l;
        } else {
            second = l;
        }
        count++;
        if (l >= 6) {
            memset(p, 'b', 6);
        }
        return 6;
    };
    EXPECT_EQ(count, 2u);
    EXPECT_EQ(first, 5u);
    EXPECT_EQ(second, 21u);
    EXPECT_EQ(test, "aaaaaaaaaabbbbbb");
}

TEST(SimStr, SStrTrimCopy) {
    stringa sample{"test"};
    stringa result = sample.trimmed<stringa>();
    EXPECT_EQ(sample, result);
    EXPECT_TRUE(sample.to_str().is_same(result.to_str()));
    stringa yares = sample.trimmed_right("t");
    EXPECT_EQ(yares, "tes");
    EXPECT_NE(sample.symbols(), yares.symbols());

    auto ls = sample.trimmed<lstringa<20>>();
    EXPECT_EQ(ls, "test");
}

template<typename K>
void check_printf_u() {
    lstring<K, 2> text;
    if constexpr (requires {text.printf(u"%2$c%1$c aaaaaaaaaaaaaa", 'a', 'b');}) {
        text.printf(u"%2$c%1$c aaaaaaaaaaaaaa", 'a', 'b');
        EXPECT_EQ(text, u"ba aaaaaaaaaaaaaa");
    }
}

template<typename K>
void check_printf_uu() {
    lstring<K, 2> text;
    if constexpr (requires {text.printf(U"%2$c%1$c aaaaaaaaaaaaaa", 'a', 'b');}) {
        text.printf(U"%2$c%1$c aaaaaaaaaaaaaa", 'a', 'b');
        EXPECT_EQ(text, U"ba aaaaaaaaaaaaaa");
    }
}

TEST(SimStr, LStrPrintf) {
    {
        lstringa<2> text;
        text.printf("%c%c", 'a', 'b');
        EXPECT_EQ(text, "ab");
        text.printf("%s", "tested");
        EXPECT_EQ(text, "tested");
    }
    {
        lstringw<2> text;
        text.printf(L"%c%c", 'a', 'b');
        EXPECT_EQ(text, L"ab");
        text.printf(L"%ls", L"tested");
        EXPECT_EQ(text, L"tested");
    }
    {
        lstringa<40> text = "tested";
        text.printf_from(4, "%c%c", 'a', 'b');
        EXPECT_EQ(text, "testab");
        text.printf_from(1, "%s", "tested");
        EXPECT_EQ(text, "ttested");
    }

    {
        lstringa<40> text = "tested";
        text.append_printf("%c%c", 'a', 'b');
        EXPECT_EQ(text, "testedab");
        text.append_printf("%s", "tested");
        EXPECT_EQ(text, "testedabtested");
    }

    {
        lstringa<2> text;
        text.printf("%2$c%1$c aaaaaaaaaaaaaa", 'a', 'b');
        EXPECT_EQ(text, "ba aaaaaaaaaaaaaa");
    }
    {
        lstringw<2> text;
        text.printf(L"%2$c%1$c aaaaaaaaaaaaaa", L'a', L'b');
        EXPECT_EQ(text, L"ba aaaaaaaaaaaaaa");
    }

    check_printf_u<u16s>();
    check_printf_uu<u32s>();
}

TEST(SimStr, LStrFormat) {
    {
        lstringa<2> text;
        text.format("{}{}", 'a', 'b');
        EXPECT_EQ(text, "ab");
        text.format("{}", "tested");
        EXPECT_EQ(text, "tested");

        lstringa<10> test = "test";
        text.format("{}ed", test);
        text.format("{}", "tested");

        stringa stest = "test";
        text.format("{}ed", stest);
        text.format("{}", "tested");
    }
    {
        lstringa<2> text;
        text.format("{1}{0}", 'a', 'b');
        EXPECT_EQ(text, "ba");
    }
    {
        lstringw<2> text;
        text.format(L"{}{}", 'a', 'b');
        EXPECT_EQ(text, L"ab");
        text.format(L"{}", L"tested");
        EXPECT_EQ(text, L"tested");
    }
    {
        lstringa<40> text = "tested";
        text.format_from(4, "{}{}", 'a', 'b');
        EXPECT_EQ(text, "testab");
        text.format_from(1, "{}", "tested");
        EXPECT_EQ(text, "ttested");
    }

    {
        lstringa<40> text = "tested";
        text.append_formatted("{}{}", 'a', 'b');
        EXPECT_EQ(text, "testedab");
        text.append_formatted("{}", "tested");
        EXPECT_EQ(text, "testedabtested");
    }
}

TEST(SimStr, LStrJoinAndExpressions) {
    lstringa<100> buffer;
    buffer = e_join<true>(std::vector<ssa>{}, "<>");
    EXPECT_EQ(buffer, "");
    buffer = e_join<true, true>(std::vector<ssa>{}, "<>");
    EXPECT_EQ(buffer, "");

    buffer = e_join<true>(std::vector<ssa>{""}, "<>");
    EXPECT_EQ(buffer, "<>");

    buffer = e_join<true, true>(std::vector<ssa>{""}, "<>");
    EXPECT_EQ(buffer, "");

    buffer = e_join<true, true>(std::vector<ssa>{""}, "<>");
    EXPECT_EQ(buffer, "");

    buffer = e_join<false, true>(std::vector<ssa>{"", ""}, "<>");
    EXPECT_EQ(buffer, "");

    buffer = e_join<false, true>(std::vector<ssa>{"s", "", "k"}, "<>");
    EXPECT_EQ(buffer, "s<>k");

    buffer = e_join<true, true>(std::vector<ssa>{"s", "", "k"}, "<>");
    EXPECT_EQ(buffer, "s<>k<>");

    buffer = e_join<false, true>(std::vector<ssa>{"s", "v", "k"}, "<>");
    EXPECT_EQ(buffer, "s<>v<>k");

    buffer = e_join<true, true>(std::vector<ssa>{"s", "v", "k"}, "<>");
    EXPECT_EQ(buffer, "s<>v<>k<>");

    buffer = e_join(std::vector<ssa>{"asd", "fgh", "jkl"}, "<>");
    EXPECT_EQ(buffer, "asd<>fgh<>jkl");

    int k = 10;
    double d = 12.1;
    lstringa<10> test = ">" + e_repl(buffer.to_str(), "<>", "-") + k + ", " + d;
    EXPECT_EQ(test, ">asd-fgh-jkl10, 12.1");

    buffer.prepend(e_choice(test.length() > 2, eea + 99, eea + 12.1 + "asd"));
    EXPECT_EQ(buffer, "99asd<>fgh<>jkl");

    buffer.change(2, 4, test(0, 3) + "__" + 1 + ',');
    EXPECT_EQ(buffer, "99>as__1,>fgh<>jkl");
}

TEST(SimStr, LStrVFormat){
    ssa t1 = "text1", t2 = "text2", format = "{}{}{}";
    auto res = lstringa<4>{}.vformat(format, t1, t2, 4);
    EXPECT_EQ(res, "text1text24");
}

TEST(SimStr, SStrFormat) {
    EXPECT_EQ(stringa::printf("%s%i", "test", 10), "test10");
    EXPECT_EQ(stringa::format("{}{}", "test", 10), "test10");
    EXPECT_EQ(stringw::printf(L"%ls%i", L"test", 10), L"test10");
    EXPECT_EQ(stringw::format(L"{}{}", L"test", 10), L"test10");
}

#define _S(par) par, size_t(std::size(par) - 1)

TEST(SimStr, HashMap) {
    EXPECT_EQ(fnv_hash( "asdfghjkl"), fnv_hash(_S( "asdfghjkl")));
    EXPECT_EQ(fnv_hash(u"asdfghjkl"), fnv_hash(_S(u"asdfghjkl")));
    EXPECT_EQ(fnv_hash(L"asdfghjkl"), fnv_hash(_S(L"asdfghjkl")));

    EXPECT_EQ(fnv_hash( "asdfghjkl"), fnv_hash_compile(_S( "asdfghjkl")));
    EXPECT_EQ(fnv_hash(u"asdfghjkl"), fnv_hash_compile(_S(u"asdfghjkl")));
    EXPECT_EQ(fnv_hash(L"asdfghjkl"), fnv_hash_compile(_S(L"asdfghjkl")));

    EXPECT_EQ(fnv_hash_ia( "asdfGhjkl"), fnv_hash_ia(_S( "asDFghjkl")));
    EXPECT_EQ(fnv_hash_ia(u"asdfghJkl"), fnv_hash_ia(_S(u"asdFGHjkl")));
    EXPECT_EQ(fnv_hash_ia(L"Asdfghjkl"), fnv_hash_ia(_S(L"asdfghjKL")));

    EXPECT_EQ(fnv_hash_ia( "asDfghjkl"), fnv_hash_ia_compile(_S( "aSDfghjkl")));
    EXPECT_EQ(fnv_hash_ia(u"asdFghjkl"), fnv_hash_ia_compile(_S(u"asdFGhjkl")));
    EXPECT_EQ(fnv_hash_ia(L"asDfghjkl"), fnv_hash_ia_compile(_S(u"asdFGhjkl")));

    EXPECT_EQ(fnv_hash_ia( "asdfGhjkl"), unicode_traits<u8s> ::hashia(_S( "asDFghjkl")));
    EXPECT_EQ(fnv_hash_ia(u"asdfghJkl"), unicode_traits<u16s>::hashia(_S(u"asDFghjkl")));
    EXPECT_EQ(fnv_hash_ia(U"asDFghJkl"), unicode_traits<u32s>::hashia(_S(U"asdFGhjkl")));
  

    EXPECT_EQ(fnv_hash_ia(               "asdfGhjklaaaaaaaaaaaaaaaaaaasDFghjklaaaaaaaaaaaaaaaaaa"),
        unicode_traits<u8s> ::hashia(_S( "asDFghjklaaaaaaaaaaaaaaaaaaasdfGhjklaaaaaaaaaaaaaaaaaa")));

    EXPECT_EQ(unicode_traits<u8s>:: hashia(_S( "asDFghjkl")),
              unicode_traits<u8s>:: hashia(_S( "Asdfghjkl")));
    EXPECT_EQ(unicode_traits<u8s>:: hashia(_S( "asDFghjklaaaaaAAAAaaaaAAAaaaAsdfghjklAaaaAaaAAaaaAAaaAAa")),
              unicode_traits<u8s>:: hashia(_S( "AsdfghjklAaaaAaaAAaaaAAaaAAaasDFghjklaaaaaAAAAaaaaAAAaaa")));
    EXPECT_EQ(unicode_traits<u16s>::hashia(_S(u"asdFGhjkl")),
              unicode_traits<u16s>::hashia(_S(u"aSDfghjkl")));
    EXPECT_EQ(unicode_traits<u32s>::hashia(_S(U"asdfGhjkl")),
              unicode_traits<u32s>::hashia(_S(U"asDfghjkl")));

    EXPECT_EQ(unicode_traits<u8s>:: hashiu(_S( "asDFghjklРус")),
              unicode_traits<u8s>:: hashiu(_S( "AsdfghjklрУС")));
    EXPECT_EQ(unicode_traits<u16s>::hashiu(_S(u"asdFGhjklРус")),
              unicode_traits<u16s>::hashiu(_S(u"aSDfghjklрУС")));
    EXPECT_EQ(unicode_traits<u32s>::hashiu(_S(U"asdfGhjklРус")),
              unicode_traits<u32s>::hashiu(_S(U"asDfghjklрУС")));

    EXPECT_EQ(unicode_traits<u32s>::hashiu(_S(U"asdfGhjklРУсasDfghjklрУСasЯDfghjklрУСasDfghjklрУС")),
              unicode_traits<u32s>::hashiu(_S(U"asdfGhjklРусAsdfGhjklРусasяdfGhjklРусasDfghjklрУс")));
    {
        hashStrMapA<int> test = {
            {"asd"_h, 1},
            {"fgh"_h, 2},
        };

        EXPECT_EQ(test.find("aaa"), test.end());
        EXPECT_NE(test.find("asd"_h), test.end());
        EXPECT_EQ(test.find("asd")->second, 1);
    }

    {
        hashStrMapAIA<int> test = {
            {"asd"_ia, 1},
            {"fgh"_ia, 2},
            {"rusрус"_ia, 3},
        };

        EXPECT_EQ(test.find("aaa"), test.end());
        EXPECT_EQ(test.find("RUSРУС"_ia), test.end());
        EXPECT_EQ(test.find("aSd")->second, 1);
    }

    {
        hashStrMapAIA<int> test = hashStrMapAIA<int>::init_str {
            {"asd", 1},
            {"fgh", 2},
            {"rusрус", 3},
        };

        EXPECT_EQ(test.find("aaa"), test.end());
        EXPECT_EQ(test.find("RUSРУС"), test.end());
        EXPECT_EQ(test.find("aSd"_ia)->second, 1);
    }

    {
        hashStrMapAIU<int> test = {
            {"asd"_iu, 1},
            {"fgh"_iu, 2},
            {"rusрус"_iu, 3},
        };

        EXPECT_EQ(test.find("aaa"), test.end());
        EXPECT_NE(test.find("RUSРУС"), test.end());
        EXPECT_EQ(test.find("RUSРУС"_iu)->second, 3);
    }
}

TEST(SimStr, SplitterWork) {
    Splitter<u8s> splitter{"asdfQQbfjgjQQQQjfjfjf", "QQ"};
    EXPECT_FALSE(splitter.is_done());
    EXPECT_EQ(splitter.next(), "asdf");
    EXPECT_EQ(splitter.next(), "bfjgj");
    EXPECT_EQ(splitter.next(), "");
    EXPECT_EQ(splitter.next(), "jfjfjf");
    EXPECT_TRUE(splitter.is_done());
    EXPECT_EQ(splitter.next(), "");
    EXPECT_EQ(splitter.next(), "");

    Splitter<u8s> splitter1{"", "-"};
    EXPECT_FALSE(splitter1.is_done());
    EXPECT_EQ(splitter1.next(), "");
    EXPECT_TRUE(splitter1.is_done());

    Splitter<u8s> splitter2{"asd-", "-"};
    EXPECT_FALSE(splitter2.is_done());
    EXPECT_EQ(splitter2.next(), "asd");
    EXPECT_FALSE(splitter2.is_done());
    EXPECT_EQ(splitter2.next(), "");
    EXPECT_TRUE(splitter1.is_done());
}

/*
Чтобы была возможность возвращать строковое выражение из функции,
надо сильно заморочится - надо тогда чтобы все типы подвыражений
имели конструкторы копирования/перемещения, сохраняли всё в себе не по ссылке,
а копией значения (потому что ссылка по неосторожности может вести на
локальную/временную переменную). К примеру, та же e_join работает с вектором,
придётся делать копию вектора, иначе надо будет тщательно следить, что в выражении мы
не используем локальных/временных векторов. (такой случай показан в примере ниже будет).
В общем, получится сложно и чревато багами.
Поэтому лучше и проще проще сделать так - постулируем, что строковые выражения можно
посылать в функции, но нельзя копировать и возвращать из функций.
А если хочется иметь возможность оптимально возращать любой тип строки, можно
делать как в примере ниже.
*/

namespace {

// Это некая функция, возвращающая вектор строк
std::vector<stra> get_strings() {
    return {"f1", "f2", "f3"};
}

// Здесь будем запоминать адрес объекта, в который пишем результат
void* address;
void* address_copy;

// Простая тестовая структура
struct aaaa {
    int field = 10;

    // Сохранение текстового представления в строку-приёмник
    void toString(storable_str<u8s> auto& res) const {
        address = &res;
        // e_join содержит ссылку на временный локальный вектор строк
        res = "my_str: "_ss + field + '\n' + e_join(get_strings(), "\n");
    }

    // Получение текстового представления заданного типа
    template<storable_str<u8s> T>
    T toString() const {
        // Благодаря магии NRVO мы избежим копирования t, а toString применится к "наружному" инициализируемом объекту:
        // https://habr.com/ru/companies/vk/articles/666330/
        // https://pvs-studio.ru/ru/blog/terms/6516/
        T t;
        address_copy = &t;
        toString(t);
        return t;
    }
};

// тестовая структура, в которой более сложное получение текстового представления:
// надо в строках, полученных из get_strings, заменить "f" на "--"
struct bbbb {
    int field = 10;
    // Сохранение текстового представления в мутабельную строку-приёмник
    void toString(mutable_str<u8s> auto& res) const {
        address = &res;

        res = "my_str: "_ss + field + '\n';
        auto strs = get_strings();
        for (const auto& s : strs) {
            res += e_repl(s, "f", "--") + e_if(&s != &strs.back(), "\n");
        }
    }

    void toString(immutable_str<u8s> auto& res) const {
        lstringsa<100> temp_buffer;
        toString(temp_buffer);
        res = std::move(temp_buffer);
    }

    // Получение текстового представления заданного типа
    template<storable_str<u8s> T>
    T toString() const {
        T t;
        address_copy = &t;
        toString(t);
        return t;
    }
};

}

TEST(SimStr, YouMustNotCopyOrReturnStrExpr) {
    static const ssa sample = "my_str: 10\nf1\nf2\nf3";
    static const ssa sample1 = "my_str: 10\n--1\n--2\n--3";
    aaaa a;
    lstringa<200> text = "text";
    // ....
    // некие действия по использованию text
    text += "text";
    // и т.д. ....
    // Действия с text закончились, хотим переиспользовать text для других целей
    a.toString(text);           // Символы из строкового выражения размещаются в буфер переменной text
    EXPECT_EQ(&text, address);  // Проверим адрес переменной, к которой применялась toString(res)
    EXPECT_EQ(text, sample);

    // А можно так:
    // NRVO магически применит toString(res) прямо к t1 и к t2,
    // то есть символы из строкового выражения разместятся сразу в буферах переменных t1 и t2,
    // не создавая промежуточных копий
    auto t1 = a.toString<stringa>();
    EXPECT_EQ(&t1, address);        // Проверим адрес переменной, к которой применялась toString(res)
    EXPECT_EQ(&t1, address_copy);   // Проверим адрес переменной в toString()
    EXPECT_EQ(t1, sample);

    auto t2 = a.toString<lstringa<100>>();
    EXPECT_EQ(&t2, address);        // Проверим адрес переменной, к которой применялась toString(res)
    EXPECT_EQ(&t2, address_copy);   // Проверим адрес переменной в toString()
    EXPECT_EQ(t2, sample);

    bbbb b;
    // text - мутабельная строка, поэтому bbbb::toString(res) будет писать сразу в буфер b
    b.toString(text);
    EXPECT_EQ(&text, address);  // Проверим адрес переменной, к которой применялась toString(res)
    EXPECT_EQ(text, sample1);

    // stringa - иммутабельная строка, поэтому вызовется функция, готовящая строку во временном буфере,
    // а потом перемещающая готовую строку из временного буфера в stringa
    auto t3 = b.toString<stringa>();
    EXPECT_NE(&t3, address);        // toString вызывалась не для t3, а для временного буфера
    EXPECT_EQ(&t3, address_copy);   // Проверим адрес переменной в toString()
    EXPECT_EQ(t3, sample1);

    // Тут строка помещается сразу в буфер t4
    auto t4 = b.toString<lstringa<100>>();
    EXPECT_EQ(&t4, address);        // Проверим адрес переменной, к которой применялась toString(res)
    EXPECT_EQ(&t4, address_copy);   // Проверим адрес переменной в toString()
    EXPECT_EQ(t4, sample1);
}

TEST(SimStr, FindOneOf) {
    EXPECT_EQ("abcdefghij"_ss.find_first_of("ce"_ss), 2);
    EXPECT_EQ("abcdefghij"_ss.find_first_of("ce"), 2);
}

TEST(SimStr, ReplSymbols) {
    lstringa<100> res = "<" + e_repl_const_symbols("abc\"gg'uu<ldl"_ss, '\"', "&quot;", '\'', "&#39;", '<', "&lt;", 'u', "") + ">";
    EXPECT_EQ(res, "<abc&quot;gg&#39;&lt;ldl>");
}

std::vector<stringa> prepareTestStrings(size_t length, size_t delta, size_t count) {
    std::vector<stringa> result;
    result.reserve(count);

    struct expr_rand {
        using symb_type = u8s;
        size_t len;
        size_t length() const noexcept { return len; }
        char* place(char* ptr) const {
            for (size_t idx = len; idx > 0; idx--) {
                *ptr++ = char(' ' + std::rand() % 200);
            }
            return ptr;
        }
    };

    for (size_t idx = 0; idx < count; idx++) {
        result.emplace_back(expr_rand{length + std::rand() % delta});
    }
    return result;
}

TEST(SimStr, HashMapInserts) {
    auto strings = prepareTestStrings(30, 20, 10'000);
    hashStrMapA<size_t> store;

    for (size_t idx = 0; idx < strings.size(); idx++) {
        store.emplace(strings[idx], idx);
    }
    EXPECT_EQ(store.size(), strings.size());

    for (size_t idx = 0; idx < strings.size(); idx += 2) {
        store.erase(strings[idx]);
    }
    EXPECT_EQ(store.size(), strings.size() / 2);

    for (size_t idx = 0; idx < strings.size(); idx += 2) {
        store[strings[idx]] = idx;
    }
    EXPECT_EQ(store.size(), strings.size());

    for (size_t idx = 1; idx < strings.size(); idx += 2) {
        auto fnd = store.find(strings[idx]);
        EXPECT_NE(fnd, store.end());
        EXPECT_EQ(fnd->first.str, strings[idx]);
    }

    hashStrMapA<size_t> copy = store, newCopy;

    store.clear();

    for (size_t idx = 1; idx < strings.size(); idx += 2) {
        auto fnd = copy.find(strings[idx]);
        EXPECT_NE(fnd, copy.end());
        EXPECT_EQ(fnd->first.str, strings[idx]);
    }

    newCopy = copy;
    copy.clear();

    for (size_t idx = 1; idx < strings.size(); idx += 2) {
        auto fnd = newCopy.find(strings[idx]);
        EXPECT_NE(fnd, newCopy.end());
        EXPECT_EQ(fnd->first.str, strings[idx]);
    }
}

TEST(SimStr, StringData) {
    {
        lstringa<10> test = "dd";
        EXPECT_EQ(stra{test.data()}, "dd");
        static_assert(std::is_same_v<decltype(test.data()), char*>, "data() must be non const");
    }
    {
        const lstringa<10> test = "dd";
        EXPECT_EQ(stra{test.data()}, "dd");
        static_assert(std::is_same_v<decltype(test.data()), const char*>, "data() must be const");
    }
    {
        stringa test = "dd";
        EXPECT_EQ(stra{test.data()}, "dd");
        static_assert(std::is_same_v<decltype(test.data()), const char*>, "data() must be const");
    }
}

TEST(SimStr, StdStringExpr) {
    {
        lstringa<20> res = eea + "test"s;
        EXPECT_EQ(res, "test");
    }
    {
        lstringa<20> res = "test"s + eea;
        EXPECT_EQ(res, "test");
    }
    {
        lstringa<20> res = eea + "test"sv;
        EXPECT_EQ(res, "test");
    }
    {
        lstringa<20> res = "test"sv + eea;
        EXPECT_EQ(res, "test");
    }
}

static std::string checker_str = "str";
static std::string_view checker_view;

static std::string get_string_val();
static std::string& get_string_ref(){return checker_str;}
static const std::string get_string_cval();
static const std::string& get_string_cref(){return checker_str;}

template<typename K>
requires requires{new simple_str<K>(std::string{""});}
char check_lvalue_str();

template<typename K>
requires requires{new simple_str<K>(""s);}
char check_lvalue_str();

template<typename K>
requires requires{new simple_str<K>(get_string_val());}
char check_lvalue_str();

template<typename K>
requires requires{new simple_str<K>(get_string_cval());}
char check_lvalue_str();

template<typename K>
requires requires{new simple_str<K>(checker_str);}
int check_lvalue_str();

template<typename K>
requires requires{new simple_str<K>(std::string_view{""});}
char check_lvalue_view();

template<typename K>
requires requires{new simple_str<K>(checker_view);}
int check_lvalue_view();

TEST(SimStr, InitFromLValueStdStrings) {
    EXPECT_EQ(sizeof(check_lvalue_str<u8s>()), sizeof(int));
    EXPECT_EQ(sizeof(check_lvalue_view<u8s>()), sizeof(int));
    EXPECT_EQ(ssa(get_string_ref()), "str");
    EXPECT_EQ(ssa(get_string_cref()), "str");
}

} // namespace simstr::tests
