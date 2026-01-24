#include "../include/simstr/strexpr.h"
#include <gtest/gtest.h>
#include <list>

using namespace std::literals;

namespace simstr::tests {

TEST(StrExpr, Empty) {
    std::string testa = eea;
    EXPECT_EQ(testa, "");

    std::u8string testb = eeb;
    EXPECT_EQ(testb, u8"");

    std::u16string testu = eeu;
    EXPECT_EQ(testu, u"");

    std::u32string testuu = eeuu;
    EXPECT_EQ(testuu, U"");

    std::wstring testw = eew;
    EXPECT_EQ(testw, L"");
}

TEST(StrExpr, OpPlusChar) {
    std::string testa = "test"_ss + 'a';
    EXPECT_EQ(testa, "testa");
    testa += +testa + u8'b';
    EXPECT_EQ(testa, "testatestab");

    std::u8string testb = u8"test"_ss + u8'a';
    EXPECT_EQ(testb, u8"testa");
#ifdef _WIN32
    std::wstring testw = L"test"_ss + u'a' + L'b';
    EXPECT_EQ(testw, L"testab");

    std::u16string testu = +u"test"sv + L'a';
    EXPECT_EQ(testu, u"testa");
#else
    std::wstring testw = L"test"_ss + U'a' + U'b';
    EXPECT_EQ(testw, L"testab");

    std::u32string testu = U"test"_ss + L'a';
    EXPECT_EQ(testu, U"testa");
#endif
}

TEST(StrExpr, Spaces) {
    std::string testa = "abc" + e_spca<10>() + "cde";
    EXPECT_EQ(testa, "abc          cde");

    std::u16string testu = u"abc" + e_c(10, u'_') + u"cde";
    EXPECT_EQ(testu, u"abc__________cde");
}

TEST(StrExpr, Repeat) {
    std::string testa = "abc";
    testa = e_repeat(+testa + " " + 10 + "s.", 3);
    EXPECT_EQ(testa, "abc 10s.abc 10s.abc 10s.");
    std::wstring testw = L"abc";
    testw += L" " + e_repeat(+testw + L" " + 10 + L"s.", 3);
    EXPECT_EQ(testw, L"abc abc 10s.abc 10s.abc 10s.");
}

TEST(StrExpr, OpPlusDifferentTypes) {
    std::string testa = "test"_ss + u8"test";
    EXPECT_EQ(testa, "testtest");

    std::u8string testb = "test"_ss + u8"test";
    EXPECT_EQ(testb, u8"testtest");
#ifdef _WIN32
    std::wstring testw = L"test"_ss + u"test";
    EXPECT_EQ(testw, L"testtest");

    std::u16string testu = +u"test"sv + L"test";
    EXPECT_EQ(testu, u"testtest");
#else
    std::wstring testw = L"test"_ss + U"test";
    EXPECT_EQ(testw, L"testtest");

    std::u32string testu = U"test"_ss + L"test";
    EXPECT_EQ(testu, U"testtest");
#endif
}

TEST(StrExpr, AddNumber) {
    std::string testa = "test"_ss + 10;
    EXPECT_EQ(testa, "test10");

    std::u8string testb = u8"test"_ss + 10;
    EXPECT_EQ(testb, u8"test10");

    std::u16string testu = u"test"_ss + 10;
    EXPECT_EQ(testu, u"test10");

    std::u32string testuu = U"test"_ss + 10;
    EXPECT_EQ(testuu, U"test10");

    std::wstring testw = L"test"_ss + 10;
    EXPECT_EQ(testw, L"test10");
}
TEST(StrExpr, Choice) {
    std::string testa = "t = " + e_choice(true, "test", "t") + " " + 10 + e_if(true, " from "_ss + 20);
    EXPECT_EQ(testa, "t = test 10 from 20");
    testa = "t = " + e_choice(false, "test", "t") + " " + 10 + e_if(false, " from "_ss + 20);
    EXPECT_EQ(testa, "t = t 10");
}

TEST(StrExpr, Fill) {
    std::string testa = "t = " + e_fill_left(+"test"s, 10);
    EXPECT_EQ(testa, "t =       test");
    testa = "t = " + e_fill_right(+"test"s, 10, '-');
    EXPECT_EQ(testa, "t = test------");

    std::u16string testu = u"t = " + e_fill_left(e_repl(u"test"sv, u"t", u"--"), 10);
    EXPECT_EQ(testu, u"t =     --es--");
}

TEST(StrExpr, Join) {
    std::vector<ssa> lst = {"abc", "def", "ghi"};
    std::string testa = "/" + e_join(lst, "-") + "/";
    EXPECT_EQ(testa, "/abc-def-ghi/");

    std::vector<std::u16string_view> ulst = {u"abc", u"def", u"ghi"};
    std::u16string testu = u"/" + e_join(ulst, u"-") + u"/";
    EXPECT_EQ(testu, u"/abc-def-ghi/");

    std::list<std::wstring> wlst = {L"abc", L"def", L"ghi"};
    std::wstring testw = L"/" + e_join(wlst, L"-") + L"/" + e_join(wlst, L"++");
    EXPECT_EQ(testw, L"/abc-def-ghi/abc++def++ghi");
}

TEST(StrExpr, Replace) {
    std::string testa = e_repl("test"_ss, "t"sv, "-|-"s) + 10;
    EXPECT_EQ(testa, "-|-es-|-10");
    testa = e_repl("aaaaaaaaaaaaaaaa"_ss, "a", "bb");
    EXPECT_EQ(testa, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

    testa = e_repl("aaaaaaaaaaaaaaaa"_ss, "a", "") + "-";
    EXPECT_EQ(testa, "-");

    testa = e_repl("aaaaaaaaaaaaaaaaaad"_ss, "a", "bb");
    EXPECT_EQ(testa, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbd");
    testa = e_repl(testa, "bb", "a");
    EXPECT_EQ(testa, "aaaaaaaaaaaaaaaaaad");

    std::u8string testb = e_repl(u8"test"sv, u8"t", u8"-|-") + 10;
    EXPECT_EQ(testb, u8"-|-es-|-10");

    std::u16string testu = e_repl(u"test"sv, u"t", u"-|-") + 10;
    EXPECT_EQ(testu, u"-|-es-|-10");

    std::u32string testuu = e_repl(U"test"sv, U"t", U"-|-") + 10;
    EXPECT_EQ(testuu, U"-|-es-|-10");

    std::wstring testw = e_repl(L"test"s, L"t", L"-|-") + 10;
    EXPECT_EQ(testw, L"-|-es-|-10");
}

size_t find_pos_str(const std::string& src, std::string_view name) {
    return src.find("\n- " + std::string(name) + " -\n");
}

size_t find_pos_exp(const std::string&  src, ssa name) {
    return src.find("\n- " + name + " -\n");
}

TEST(StrExpr, FindConcatThree) {
    std::string src = "sdfsdf\n- testtest -\nsfrgdgfsg";
    EXPECT_EQ(find_pos_str(src, "testtest"sv), 6);
    EXPECT_EQ(find_pos_exp(src, "testtest"sv), 6);
}

std::string buildTypeNameStr(std::string_view type_name, size_t prec, size_t scale) {
    std::string res{type_name};
    if (prec) {
        res += "(" + std::to_string(prec);
        if (scale) {
            res += "," + std::to_string(scale);
        }
        res += ")";
    }
    return res;
}

std::string buildTypeNameExp(std::string_view type_name, size_t prec, size_t scale) {
    if (prec) {
        return type_name + "("_ss + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return std::string{type_name};
}

TEST(StrExpr, MultiConcat) {
    EXPECT_EQ(buildTypeNameStr("integer", 0, 0), "integer");
    EXPECT_EQ(buildTypeNameStr("numeric", 10, 2), "numeric(10,2)");
    EXPECT_EQ(buildTypeNameExp("integer", 0, 0), "integer");
    EXPECT_EQ(buildTypeNameExp("numeric", 10, 2), "numeric(10,2)");
}

int split_and_calc_total_str(std::string_view numbers, std::string_view delimiter) {
    int total = 0;
    for (size_t start = 0; start < numbers.length(); ) {
        int delim = numbers.find(delimiter, start);
        if (delim == std::string::npos) {
            delim = numbers.size();
        }
        std::string part{numbers.substr(start, delim - start)};
        total += std::strtol(part.c_str(), nullptr, 0);
        start = delim + delimiter.length();
    }
    return total;
}

int split_and_calc_total_sim(ssa numbers, ssa delimiter) {
    int total = 0;
    for (auto splitter = numbers.splitter(delimiter); !splitter.is_done();) {
        total += splitter.next().as_int<int>();
    }
    return total;
}

TEST(StrExpr, SplitCalcTotal) {
    const char NUMBER_LIST[] = "1-!- 2-!-   3-!- 4 -!- 5-!- 6  -!- 7-!-  -8-!- 0xaF-!-   15-!- 010"; // 218
    const char delim[] = "-!-";
    EXPECT_EQ(split_and_calc_total_str(NUMBER_LIST, delim), 218);
    EXPECT_EQ(split_and_calc_total_sim(NUMBER_LIST, delim), 218);
}

TEST(StrExpr, StdToSimplestr) {
    std::string test = "   sdfsg   ";
    std::string_view res = ssa{test}.trimmed();
    EXPECT_EQ(res, "sdfsg");

    size_t fnd = test.find(" " + std::string{res});
    EXPECT_EQ(fnd, 2);
}

TEST(StrExpr, StrChange) {
    {
        std::string str = "test";
        EXPECT_EQ(str::change(str, 1, 2, "new val"_ss + 10), "tnew val10t");
    }
    {
        std::string str = "test";
        EXPECT_EQ(str::change(str, 0, 0, "new val"_ss + 10), "new val10test");
    }
    {
        std::string str = "test";
        EXPECT_EQ(str::change(str, 100, 100, u8"new val"_ss + 10), "testnew val10");
    }
    {
        std::string str = "test";
        EXPECT_EQ(str |= u8"new val"_ss + 10, "testnew val10");
    }
    {
        std::string str = "test";
        EXPECT_EQ(str ^= u8"new val"_ss + 10, "new val10test");
    }
}

TEST(StrExpr, StrReplace) {
    {
        std::string src = "-aaaaaaaaaaaaaaaa--";
        EXPECT_EQ(str::replace(src, "a", "aa"), "-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa--");
        EXPECT_EQ(str::replace(src, "aa", "b"), "-bbbbbbbbbbbbbbbb--");
        EXPECT_EQ(str::replace(src, "b", ""_ss), "---");
    }
    {
        std::string src = "-aaaaaaaaaaaaaaaaaa--";
        EXPECT_EQ(str::replace(src, "a", "aa"), "-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa--");
    }
    {
        std::string src = "-aaaaaaaaaaaaaaaa--";
        EXPECT_EQ(str::replace(src, "a", "a"_ss + "a"), "-aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa--");
        EXPECT_EQ(str::replace(src, "aa", eea + "b"), "-bbbbbbbbbbbbbbbb--");
        EXPECT_EQ(str::replace(src, "b", eea), "---");
    }
    {
        std::u16string src = u"-aaaaaaaaaaaaaaaa--";
        EXPECT_EQ(str::replace(src, u"a", u"vv", 5, 3), u"-aaaavvvvvvaaaaaaaaa--");
    }
    {
        std::u16string src = u"-aaaaaaaaaaaaaaaa--";
        EXPECT_EQ(str::replace(src, u"a", u"vv"_ss + 10, 5, 3), u"-aaaavv10vv10vv10aaaaaaaaa--");
    }
}

} // namespace simstr::tests
