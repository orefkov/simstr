/*
 * ver. 1.6.7
 * (c) Проект "SimStr", Александр Орефков orefkov@gmail.com
 * Бенчмарки
 * (c) Project "SimStr", Aleksandr Orefkov orefkov@gmail.com
 * Benchmarks
 */

#include "bench.h"
#include <sstream>
#include <string>
#include <charconv>
#include <iomanip>
#include <fmt/format.h>
#include <fmt/compile.h>

using namespace simstr;
using namespace std::literals;

#ifdef EMSCRIPTEN
#include <emscripten.h>
bool do_sync = false;
static void DoTeardown(const benchmark::State& state) {
    // Это нужно чтобы интерфейс браузера обновился
    // This is necessary for the browser interface to refresh.
    if (!do_sync) {
        emscripten_sleep(1);
    }
}
#undef BENCHMARK
#define BENCHMARK(...)                                                \
  BENCHMARK_PRIVATE_DECLARE(_benchmark_) =                            \
      (::benchmark::internal::RegisterBenchmarkInternal(              \
          ::benchmark::internal::make_unique<                         \
              ::benchmark::internal::FunctionBenchmark>(#__VA_ARGS__, \
        __VA_ARGS__)))->Teardown(DoTeardown)

#undef BENCHMARK_CAPTURE
#define BENCHMARK_CAPTURE(func, test_case_name, ...)     \
  BENCHMARK_PRIVATE_DECLARE(_benchmark_) =               \
      (::benchmark::internal::RegisterBenchmarkInternal( \
          ::benchmark::internal::make_unique<            \
              ::benchmark::internal::FunctionBenchmark>( \
              #func "/" #test_case_name,                 \
              [](::benchmark::State& st) { func(st, __VA_ARGS__); })))->Teardown(DoTeardown)

#endif

#define TEST_TEXT "Test text"
#define LONG_TEXT "123456789012345678901234567890"
#define TEXT_16 "abbaabbaabbaabba"

#define CHECK_RESULT

void __(benchmark::State& state) { for (auto _: state) {} }

void ConcatStdToStd(benchmark::State& state) {
    std::string s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            std::string str = s1 + std::to_string(i) + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToStd(benchmark::State& state) {
    std::string s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            std::string str = +s1 + i + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSim(benchmark::State& state) {
    stra s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            stringa str = s1 + i + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimConcat(benchmark::State& state) {
    stra s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            stringa str = e_concat("", s1, i, " end");
            benchmark::DoNotOptimize(str);
        }
    }
}

BENCHMARK(__)->Name("-----  Concatenate string + Number + \"Literal\" ---------")->Repetitions(1);
BENCHMARK(ConcatStdToStd)       ->Name("Concat std::string and number by std to std::string");
BENCHMARK(ConcatSimToStd)       ->Name("Concat std::string and number by StrExpr to std::string");
BENCHMARK(ConcatSimToSim)       ->Name("Concat stringa and number by StrExpr to simstr::stringa");
BENCHMARK(ConcatSimToSimConcat) ->Name("Concat stringa and number by e_concat to simstr::stringa");

void ConcatStdToFmtHex(benchmark::State& state) {
    // We use a short string so that the longest result is 15 characters and fits in the std::string SSO buffer.
    std::string s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // It is not worked for char8_t, char16_t, char32_t :(
            std::string str = s1 + std::format("{:#x}", i) + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatAllFmtToHex(benchmark::State& state) {
    // We use a short string so that the longest result is 15 characters and fits in the std::string SSO buffer.
    std::string s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // It is not worked for char8_t, char16_t, char32_t :(
            std::string str = std::format("{}{:#x} end", s1, i);
            benchmark::DoNotOptimize(str);
        }
    }
}
void ConcatStdToCharsHex(benchmark::State& state) {
    // We use a short string so that the longest result is 15 characters and fits in the std::string SSO buffer.
    std::string s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // it worked only for char :(
            char buf[40];
            size_t len = std::to_chars(buf, buf + std::size(buf), i, 16).ptr - buf;
            std::string str = s1 + "0x" + std::string(buf, len) + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToStdHex(benchmark::State& state) {
    // We use a short string so that the longest result is 15 characters and fits in the std::string SSO buffer.
    std::string s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // Can work for all types of symbols
            std::string str = +s1 + i / 1_f16 + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimHex(benchmark::State& state) {
    // stringa SSO buffer is 23, but we use a short string to compare under the same conditions
    stra s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // Can work for all types of symbols
            stringa str = s1 + i / 1_f16 + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimHexC(benchmark::State& state) {
    // stringa SSO buffer is 23, but we use a short string to compare under the same conditions
    stra s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            // Can work for all types of symbols
            stringa str = e_concat("", s1, i / 1_f16, " end");
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimHexS(benchmark::State& state) {
    // stringa SSO buffer is 23, but we use a short string to compare under the same conditions
    stra s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            stringa str = e_subst("{}{} end", s1, i / 1_f16);
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimHexVS(benchmark::State& state) {
    // stringa SSO buffer is 23, but we use a short string to compare under the same conditions
    stra s1 = "art ";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            stringa str = e_vsubst("{}{} end"_ss, s1, i / 1_f16);
            benchmark::DoNotOptimize(str);
        }
    }
}

BENCHMARK(__)->Name("-----  Concatenate string + Hex Number + \"Literal\" ---------")->Repetitions(1);
BENCHMARK(ConcatStdToFmtHex)    ->Name("Concat std::string and format hex number and literal to std::string");
BENCHMARK(ConcatAllFmtToHex)    ->Name("std::format std::string and hex number by literal to std::string");
BENCHMARK(ConcatStdToCharsHex)  ->Name("Concat std::string and std::to_chars and string to std::string");
BENCHMARK(ConcatSimToStdHex)    ->Name("Concat std::string and hex number and literal by StrExpr to std::string");
BENCHMARK(ConcatSimToSimHex)    ->Name("Concat stringa and hex number and literal by StrExpr to simstr::stringa");
BENCHMARK(ConcatSimToSimHexC)   ->Name("Concat stringa and hex number and literal by e_concat to simstr::stringa");
BENCHMARK(ConcatSimToSimHexS)   ->Name("Subst stringa and hex number by e_subst literal to simstr::stringa");
BENCHMARK(ConcatSimToSimHexVS)  ->Name("Subst stringa and hex number by e_vsubst stra to simstr::stringa");

void ConcatSimToSimOct(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            stringa str = "abcdefghihklmopqr "_ss + i / 0x8a010_fmt + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void SubstSimToSimOct(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            stringa str = e_subst("abcdefghihklmopqr {} end", i / 0x8a010_fmt);
            benchmark::DoNotOptimize(str);
        }
    }
}

void VSubstSimToSimOct(benchmark::State& state) {
    ssa pattern = "abcdefghihklmopqr {} end";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            stringa str = e_vsubst(pattern, i / 0x8a010_fmt);
            benchmark::DoNotOptimize(str);
        }
    }
}

void FormatStdToStdOct(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            std::string str = std::format("abcdefghihklmopqr {:#010o} end", i);
            benchmark::DoNotOptimize(str);
        }
    }
}

void VFormatStdToStdOct(benchmark::State& state) {
    std::string_view pattern = "abcdefghihklmopqr {:#010o} end";
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            std::string str = std::vformat(pattern, std::make_format_args(i));
            benchmark::DoNotOptimize(str);
        }
    }
}

void StreamStdToStdOct(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            std::stringstream strm;
            strm << "abcdefghihklmopqr 0" << std::oct << std::setw(9) << std::setfill('0') << i << " end";
            std::string str = strm.str();
            benchmark::DoNotOptimize(str);
        }
    }
}

void FmtFormatComp(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            std::string str = fmt::format(FMT_COMPILE("abcdefghihklmopqr {:#010o} end"), i);
            benchmark::DoNotOptimize(str);
        }
    }
}

void FmtFormat(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned i = 1; i <= 100'000; i *= 10) {
            std::string str = fmt::format("abcdefghihklmopqr {:#010o} end", i);
            benchmark::DoNotOptimize(str);
        }
    }

}

BENCHMARK(__)->Name("---- format/vformat and subst/vsubst octal number to 32 symbols result ----")->Repetitions(1);
BENCHMARK(ConcatSimToSimOct)    ->Name("\"abcdefghihklmopqr \"_ss + i / 0x8a010_fmt + \" end\"");
BENCHMARK(SubstSimToSimOct)     ->Name("e_subst(\"abcdefghihklmopqr {} end\", i / 0x8a010_fmt)");
BENCHMARK(VSubstSimToSimOct)    ->Name("e_vsubst(pattern, i / 0x8a010_fmt)");
BENCHMARK(FmtFormatComp)        ->Name("fmt::format(FMT_COMPILE(\"abcdefghihklmopqr {:#010o} end\"), i)");
BENCHMARK(FmtFormat)            ->Name("fmt::format(\"abcdefghihklmopqr {:#010o} end\", i)");
BENCHMARK(FormatStdToStdOct)    ->Name("std::format(\"abcdefghihklmopqr {:#010o} end\", i)");
BENCHMARK(VFormatStdToStdOct)   ->Name("std::vformat(pattern, std::make_format_args(i))");
BENCHMARK(StreamStdToStdOct)    ->Name("strm << \"abcdefghihklmopqr 0\" << std::oct << std::setw(9) << std::setfill('0') << i << \" end\"");

void ConcatStdToStdS(benchmark::State& state) {
    std::string s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            std::string str = s1 + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToStdS(benchmark::State& state) {
    std::string s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            std::string str = +s1 + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

void ConcatSimToSimS(benchmark::State& state) {
    stra s1 = "start ";
    for (auto _: state) {
        for (int i = 1; i <= 100'000; i *= 10) {
            benchmark::DoNotOptimize(s1);
            stringa str = s1 + " end";
            benchmark::DoNotOptimize(str);
        }
    }
}

BENCHMARK(__)->Name("-----  Concatenate string + \"Literal\" ---------")->Repetitions(1);
BENCHMARK(ConcatStdToStdS)    ->Name("Concat std::string by std to std::string");
BENCHMARK(ConcatSimToStdS)    ->Name("Concat std::string by StrExpr to std::string");
BENCHMARK(ConcatSimToSimS)    ->Name("Concat stringa by StrExpr to stringa");

size_t find_pos_str(std::string_view src, std::string_view name) {
    // before C++26 we can not concatenate string and string_view...
    return src.find("\n- "s + std::string{name} + " -\n");
}

size_t find_pos_exp(ssa src, ssa name) {
    return src.find(std::string{"\n- " + name + " -\n"});
}

size_t find_pos_sim(ssa src, ssa name) {
    return src.find(lstringa<200>{"\n- " + name + " -\n"});
}

//> size_t find_pos_str(std::string_view src, std::string_view name) {
void FindConcatThreeStr(benchmark::State& state) {
    std::string_view src = "sdfsdf\n- testtesttesttesttesttesttestte -\nsfrgdgfsg";
    std::string_view fnd = "testtesttesttesttesttesttestte";
    for (auto _: state) {
        benchmark::DoNotOptimize(src);
        benchmark::DoNotOptimize(fnd);
        size_t pos = find_pos_str(src, fnd);
        benchmark::DoNotOptimize(pos);
        #ifdef CHECK_RESULT
        if (pos != 6) {
            state.SkipWithError("fail");
        }
        #endif
    }
}

//> size_t find_pos_exp(ssa src, ssa name) {
void FindConcatThreeExp(benchmark::State& state) {
    std::string_view src = "sdfsdf\n- testtesttesttesttesttesttestte -\nsfrgdgfsg";
    std::string_view fnd = "testtesttesttesttesttesttestte";
    for (auto _: state) {
        benchmark::DoNotOptimize(src);
        benchmark::DoNotOptimize(fnd);
        size_t pos = find_pos_exp(src, fnd);
        benchmark::DoNotOptimize(pos);
        #ifdef CHECK_RESULT
        if (pos != 6) {
            state.SkipWithError("fail");
        }
        #endif
    }
}

//> size_t find_pos_sim(ssa src, ssa name) {
void FindConcatThreeSim(benchmark::State& state) {
    ssa src = "sdfsdf\n- testtesttesttesttesttesttestte -\nsfrgdgfsg";
    ssa fnd = "testtesttesttesttesttesttestte";
    for (auto _: state) {
        benchmark::DoNotOptimize(src);
        benchmark::DoNotOptimize(fnd);
        size_t pos = find_pos_sim(src, fnd);
        benchmark::DoNotOptimize(pos);
        #ifdef CHECK_RESULT
        if (pos != 6) {
            state.SkipWithError("fail");
        }
        #endif
    }
}

BENCHMARK(__)->Name("-----  Find three concatenated string in string_view -----")->Repetitions(1);
BENCHMARK(FindConcatThreeStr)->Name("Find concat three std::string");
BENCHMARK(FindConcatThreeExp)->Name("Find concat three strexpr");
BENCHMARK(FindConcatThreeSim)->Name("Find concat three simstr");

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

std::string buildTypeNameExp(ssa type_name, size_t prec, size_t scale) {
    if (prec) {
        return type_name + "(" + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return type_name;
}

stringa buildTypeNameSim(ssa type_name, size_t prec, size_t scale) {
    if (prec) {
        return type_name + "(" + prec + e_if(scale, ","_ss + scale) + ")";
    }
    return type_name;
}

//> std::string buildTypeNameStr(std::string_view type_name, size_t prec, size_t scale) {
void BuildTypeNameStr(benchmark::State& state) {
    std::string_view type_name = "numeric";
    size_t prec = state.range(0), scale = prec / 2;
    for (auto _: state) {
        benchmark::DoNotOptimize(type_name);
        benchmark::DoNotOptimize(prec);
        benchmark::DoNotOptimize(scale);
        std::string res = buildTypeNameStr(type_name, prec, scale);
        benchmark::DoNotOptimize(res);
    }
}

//> std::string buildTypeNameExp(ssa type_name, size_t prec, size_t scale) {
void BuildTypeNameExp(benchmark::State& state) {
    std::string_view type_name = "numeric";
    size_t prec = state.range(0), scale = prec / 2;
    for (auto _: state) {
        benchmark::DoNotOptimize(type_name);
        benchmark::DoNotOptimize(prec);
        benchmark::DoNotOptimize(scale);
        std::string res = buildTypeNameExp(type_name, prec, scale);
        benchmark::DoNotOptimize(res);
    }
}

//> stringa buildTypeNameSim(ssa type_name, size_t prec, size_t scale) {
void BuildTypeNameSim(benchmark::State& state) {
    ssa type_name = "numeric";
    size_t prec = state.range(0), scale = prec / 2;
    for (auto _: state) {
        benchmark::DoNotOptimize(type_name);
        benchmark::DoNotOptimize(prec);
        benchmark::DoNotOptimize(scale);
        stringa res = buildTypeNameSim(type_name, prec, scale);
        benchmark::DoNotOptimize(res);
    }
}

BENCHMARK(__)->Name("-----  Build Type Name ---------")->Repetitions(1);
BENCHMARK(BuildTypeNameStr)   ->Name("BuildTypeNameStr 0")->Arg(0);
BENCHMARK(BuildTypeNameExp)   ->Name("BuildTypeNameExp 0")->Arg(0);
BENCHMARK(BuildTypeNameSim)   ->Name("BuildTypeNameSim 0")->Arg(0);
BENCHMARK(BuildTypeNameStr)   ->Name("BuildTypeNameStr 10")->Arg(10);
BENCHMARK(BuildTypeNameExp)   ->Name("BuildTypeNameExp 10")->Arg(10);
BENCHMARK(BuildTypeNameSim)   ->Name("BuildTypeNameSim 10")->Arg(10);

std::string make_str_str(std::string_view from, std::string_view pattern, std::string_view repl) {
    auto str_replace = [](std::string_view from, std::string_view pattern, std::string_view repl) {
        std::string result;
        for (size_t offset = 0; ;) {
            size_t pos = from.find(pattern, offset);
            if (pos == std::string::npos) {
                result += from.substr(offset);
                break;
            }
            result += from.substr(offset, pos - offset);
            result += repl;
            offset = pos + pattern.length();
        }
        return result;
    };
    return "<" + str_replace(from, pattern, repl) + ">";
}

std::string make_str_exp(std::string_view from, std::string_view pattern, std::string_view repl) {
    return "<" + e_repl(from, pattern, repl) + ">";
}

//> std::string make_str_str(std::string_view from, std::string_view pattern, std::string_view repl) {
void ReplaceStr(benchmark::State& state) {
    std::string_view from = "testitestitestitesti", what = "te", repl = "--+--";

    for (auto _: state) {
        benchmark::DoNotOptimize(from);
        benchmark::DoNotOptimize(what);
        benchmark::DoNotOptimize(repl);
        std::string res = make_str_str(from, what, repl);
        benchmark::DoNotOptimize(res);
    }
}

//> std::string make_str_exp(std::string_view from, std::string_view pattern, std::string_view repl) {
void ReplaceExp(benchmark::State& state) {
    std::string_view from = "testitestitestitesti", what = "te", repl = "--+--";

    for (auto _: state) {
        benchmark::DoNotOptimize(from);
        benchmark::DoNotOptimize(what);
        benchmark::DoNotOptimize(repl);
        std::string res = make_str_exp(from, what, repl);
        benchmark::DoNotOptimize(res);
    }
}

BENCHMARK(__)->Name("-----  Replace string by copy -----")->Repetitions(1);
BENCHMARK(ReplaceStr)->Name("Concat with replace str");
BENCHMARK(ReplaceExp)->Name("Concat with replace exp");

template<typename T>
void CreateEmpty(benchmark::State& state) {
    for (auto _: state) {
        T empty_string;
        benchmark::DoNotOptimize(empty_string);
    }
}
BENCHMARK(__)->Name("-----  Create Empty Str ---------")->Repetitions(1);
BENCHMARK(CreateEmpty<std::string>)         ->Name("std::string e;");
BENCHMARK(CreateEmpty<std::string_view>)    ->Name("std::string_view e;");
BENCHMARK(CreateEmpty<ssa>)                 ->Name("ssa e;");
BENCHMARK(CreateEmpty<stringa>)             ->Name("stringa e;");
BENCHMARK(CreateEmpty<lstringa<20>>)        ->Name("lstringa<20> e;");
BENCHMARK(CreateEmpty<lstringa<40>>)        ->Name("lstringa<40> e;");

template<typename T>
void CreateShortLiteral(benchmark::State& state) {
    for (auto _: state) {
        T empty_string = TEST_TEXT;
        benchmark::DoNotOptimize(empty_string);
    }
}
BENCHMARK(__)->Name("-----  Create Str from short literal (9 symbols) --------")->Repetitions(1);
BENCHMARK(CreateShortLiteral<std::string>)      ->Name("std::string e      = \"Test text\";");
BENCHMARK(CreateShortLiteral<std::string_view>) ->Name("std::string_view e = \"Test text\";");
BENCHMARK(CreateShortLiteral<ssa>)              ->Name("ssa e              = \"Test text\";");
BENCHMARK(CreateShortLiteral<stringa>)          ->Name("stringa e          = \"Test text\";");
BENCHMARK(CreateShortLiteral<lstringa<20>>)     ->Name("lstringa<20> e     = \"Test text\";");
BENCHMARK(CreateShortLiteral<lstringa<40>>)     ->Name("lstringa<40> e     = \"Test text\";");

template<typename T>
void CreateLongLiteral(benchmark::State& state) {
    for (auto _: state) {
        T empty_string{LONG_TEXT};
        benchmark::DoNotOptimize(empty_string);
    }
}
BENCHMARK(__)->Name("-----  Create Str from long literal (30 symbols) ---------")->Repetitions(1);
BENCHMARK(CreateLongLiteral<std::string>)        ->Name("std::string e      = \"123456789012345678901234567890\";");
BENCHMARK(CreateLongLiteral<std::string_view>)   ->Name("std::string_view e = \"123456789012345678901234567890\";");
BENCHMARK(CreateLongLiteral<ssa>)                ->Name("ssa e              = \"123456789012345678901234567890\";");
BENCHMARK(CreateLongLiteral<stringa>)            ->Name("stringa e          = \"123456789012345678901234567890\";");
BENCHMARK(CreateLongLiteral<lstringa<20>>)       ->Name("lstringa<20> e     = \"123456789012345678901234567890\";");
BENCHMARK(CreateLongLiteral<lstringa<40>>)       ->Name("lstringa<40> e     = \"123456789012345678901234567890\";");

template<typename T>
void CopyShortString(benchmark::State& state) {
    T x{TEST_TEXT};
    for (auto _: state) {
        T copy{x};
        benchmark::DoNotOptimize(copy);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(__)->Name("-----  Create copy of Str with 9 symbols ---------")->Repetitions(1);
BENCHMARK(CopyShortString<std::string>)      ->Name("std::string e      = \"Test text\"; auto c{e};");
BENCHMARK(CopyShortString<std::string_view>) ->Name("std::string_view e = \"Test text\"; auto c{e};");
BENCHMARK(CopyShortString<ssa>)              ->Name("ssa e              = \"Test text\"; auto c{e};");
BENCHMARK(CopyShortString<stringa>)          ->Name("stringa e          = \"Test text\"; auto c{e};");
BENCHMARK(CopyShortString<lstringa<20>>)     ->Name("lstringa<20> e     = \"Test text\"; auto c{e};");
BENCHMARK(CopyShortString<lstringa<40>>)     ->Name("lstringa<40> e     = \"Test text\"; auto c{e};");

template<typename T>
void CopyLongString(benchmark::State& state) {
    T x = LONG_TEXT;
    for (auto _: state) {
        T copy{x};
        benchmark::DoNotOptimize(copy);
    }
}

BENCHMARK(__)->Name("-----  Create copy of Str with 30 symbols ---------")->Repetitions(1);
BENCHMARK(CopyLongString<std::string>)      ->Name("std::string e      = \"123456789012345678901234567890\"; auto c{e};");
BENCHMARK(CopyLongString<std::string_view>) ->Name("std::string_view e = \"123456789012345678901234567890\"; auto c{e};");
BENCHMARK(CopyLongString<ssa>)              ->Name("ssa e              = \"123456789012345678901234567890\"; auto c{e};");
BENCHMARK(CopyLongString<stringa>)          ->Name("stringa e          = \"123456789012345678901234567890\"; auto c{e};");
BENCHMARK(CopyLongString<lstringa<20>>)     ->Name("lstringa<20> e     = \"123456789012345678901234567890\"; auto c{e};");
BENCHMARK(CopyLongString<lstringa<40>>)     ->Name("lstringa<40> e     = \"123456789012345678901234567890\"; auto c{e};");

template<typename T>
void Find(benchmark::State& state) {
    T x{LONG_TEXT LONG_TEXT LONG_TEXT TEST_TEXT};
    for (auto _: state) {
        int i = (int)x.find(TEST_TEXT);
    #ifdef CHECK_RESULT
        if (i != 90) {
            state.SkipWithError("not find?");
            break;
        }
    #endif
        benchmark::DoNotOptimize(i);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(__)->Name("-----  Find 9 symbols text in end of 99 symbols text ---------")->Repetitions(1);
BENCHMARK(Find<std::string>)        ->Name("std::string::find;");
BENCHMARK(Find<std::string_view>)   ->Name("std::string_view::find;");
BENCHMARK(Find<ssa>)                ->Name("ssa::find;");
BENCHMARK(Find<stringa>)            ->Name("stringa::find;");
BENCHMARK(Find<lstringa<20>>)       ->Name("lstringa<20>::find;");
BENCHMARK(Find<lstringa<40>>)       ->Name("lstringa<40>::find;");

template<typename T>
void CopyDynString(benchmark::State& state) {
    T x(state.range(0), 'a');
    for (auto _: state) {
        T copy{x};
        benchmark::DoNotOptimize(copy);
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(__)->Name("-------  Copy not literal Str with N symbols ---------")->Repetitions(1);
BENCHMARK(CopyDynString<std::string>)   ->Name("std::string copy{str_with_len_N};")->Arg(15)->Arg(16)->Arg(23)->Arg(24)->RangeMultiplier(2)->Range(32, 4096);
BENCHMARK(CopyDynString<stringa>)       ->Name("stringa copy{str_with_len_N};")->Arg(15)->Arg(16)->Arg(23)->Arg(24)->RangeMultiplier(2)->Range(32, 4096);
BENCHMARK(CopyDynString<lstringa<16>>)  ->Name("lstringa<16> copy{str_with_len_N};")->Arg(15)->Arg(16)->Arg(23)->Arg(24)->RangeMultiplier(2)->Range(32, 4096);
BENCHMARK(CopyDynString<lstringa<512>>) ->Name("lstringa<512> copy{str_with_len_N};")->Arg(15)->Arg(16)->Arg(23)->Arg(24)->RangeMultiplier(2)->Range(32, 4096);

void ToIntStr10(benchmark::State& state, const std::string& s, int c) {
    for (auto _: state) {
        int res = std::strtol(s.c_str(), nullptr, 10);
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

void ToIntStr16(benchmark::State& state, const std::string& s, int c) {
    for (auto _: state) {
        int res = std::strtol(s.c_str(), nullptr, 16);
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

void ToIntStr0(benchmark::State& state, const std::string& s, int c) {
    for (auto _: state) {
        int res = std::strtol(s.c_str(), nullptr, 0);
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

void ToIntFromChars10(benchmark::State& state, const std::string_view& s, int c) {
    for (auto _: state) {
        int res = 0;
        std::from_chars(s.data(), s.data() + s.size(), res, 10);
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

void ToIntFromChars16(benchmark::State& state, const std::string_view& s, int c) {
    for (auto _: state) {
        int res = 0;
        std::from_chars(s.data(), s.data() + s.size(), res, 16);
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

template<typename T>
void ToIntSimStr10(benchmark::State& state, T t, int c) {
    for (auto _: state) {
        int res = t. template to_int<int, true, 10, false, false>().value;
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
        benchmark::DoNotOptimize(t);
    }
}

template<typename T>
void ToIntSimStr16(benchmark::State& state, T t, int c) {
    for (auto _: state) {
        int res = t. template to_int<int, true, 16, false, false>().value;
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
        benchmark::DoNotOptimize(t);
    }
}

template<typename T>
void ToIntSimStr0(benchmark::State& state, T t, int c) {
    for (auto _: state) {
        int res = t. template to_int<int>().value;
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
        benchmark::DoNotOptimize(t);
    }
}

void ToIntNoOverflow(benchmark::State& state, ssa t, int c) {
    for (auto _: state) {
        int res = t.to_int<int, false>().value;
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(__)->Name("-----  Convert to int '1234567'  ---------")->Repetitions(1);
BENCHMARK_CAPTURE(ToIntStr10, , std::string{"123456789"}, 123456789)                ->Name("std::string s = \"123456789\"; int res = std::strtol(s.c_str(), 0, 10);");
BENCHMARK_CAPTURE(ToIntFromChars10, , std::string_view{"123456789"}, 123456789)     ->Name("std::string_view s = \"123456789\"; std::from_chars(s.data(), s.data() + s.size(), res, 10);");
BENCHMARK_CAPTURE(ToIntSimStr10, , stringa{"123456789"}, 123456789)                 ->Name("stringa s = \"123456789\"; int res = s.to_int<int, true, 10, false>");
BENCHMARK_CAPTURE(ToIntSimStr10, , ssa{"123456789"}, 123456789)                     ->Name("ssa s = \"123456789\"; int res = s.to_int<int, true, 10, false>");
BENCHMARK_CAPTURE(ToIntSimStr10, , lstringa<20>{"123456789"}, 123456789)            ->Name("lstringa<20> s = \"123456789\"; int res = s.to_int<int, true, 10, false>");
BENCHMARK(__)->Name("-----  Convert to unsigned 'abcDef'  ---------")->Repetitions(1);
BENCHMARK_CAPTURE(ToIntStr16, , std::string{"abcDef"}, 0xabcDef)                    ->Name("std::string s = \"abcDef\"; int res = std::strtol(s.c_str(), 0, 16);");
BENCHMARK_CAPTURE(ToIntFromChars16, , std::string_view{"abcDef"}, 0xabcDef)         ->Name("std::string_view s = \"abcDef\"; std::from_chars(s.data(), s.data() + s.size(), res, 16);");
BENCHMARK_CAPTURE(ToIntSimStr16, , stringa{"abcDef"}, 0xabcDef)                     ->Name("stringa s = \"abcDef\"; int res = s.to_int<int, true, 16, false>");
BENCHMARK_CAPTURE(ToIntSimStr16, , ssa{"abcDef"}, 0xabcDef)                         ->Name("ssa s = \"abcDef\"; int res = s.to_int<int, true, 16, false>");
BENCHMARK_CAPTURE(ToIntSimStr16, >, lstringa<20>{"abcDef"}, 0xabcDef)               ->Name("lstringa<20> s = \"abcDef\"; int res = s.to_int<int, true, 16, false>");
BENCHMARK(__)->Name("-----  Convert to int '    1234567'  ---------")->Repetitions(1);
BENCHMARK_CAPTURE(ToIntStr0, , std::string{"    123456789"}, 123456789)             ->Name("std::string s = \"    123456789\"; int res = std::strtol(s.c_str(), 0, 0);");
BENCHMARK_CAPTURE(ToIntSimStr0, , stringa{"    123456789"}, 123456789)              ->Name("stringa s = \"    123456789\"; int res = s.to_int<int>; // Check overflow");
BENCHMARK_CAPTURE(ToIntNoOverflow, , ssa{"    123456789"}, 123456789)               ->Name("ssa s = \"    123456789\"; int res = s.to_int<int, false>; // No check overflow");

void ToDoubleStr(benchmark::State& state, const std::string& s, double c) {
    for (auto _: state) {
        char* ptr = nullptr;
        double res = std::strtod(s.c_str(), &ptr);
        if (ptr == s.c_str()) {
            state.SkipWithError("not equal");
        }
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

void ToDoubleFromChars(benchmark::State& state, const std::string_view& s, double c) {
    for (auto _: state) {
        double res = 0;
        if (std::from_chars(s.data(), s.data() + s.size(), res).ec != std::errc{}) {
            state.SkipWithError("not equal");
        }
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
    }
}

template<typename T>
void ToDoubleSimStr(benchmark::State& state, T t, double c) {
    for (auto _: state) {
        auto r = t.template to_double<false>();
        if (!r) {
            state.SkipWithError("not equal");
        }
        double res = *r;
    #ifdef CHECK_RESULT
        if (res != c) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(res);
        benchmark::DoNotOptimize(t);
    }
}

BENCHMARK(__)->Name("-----  Convert to double '1234.567e10'  ---------")->Repetitions(1);
BENCHMARK_CAPTURE(ToDoubleStr, , std::string{"1234.567e10"}, 1234.567e10)                ->Name("std::string s = \"1234.567e10\"; double res = std::strtod(s.c_str(), nullptr);");
BENCHMARK_CAPTURE(ToDoubleFromChars, , std::string_view{"1234.567e10"}, 1234.567e10)     ->Name("std::string_view s = \"1234.567e10\"; std::from_chars(s.data(), s.data() + s.size(), res);");
BENCHMARK_CAPTURE(ToDoubleSimStr, , ssa{"1234.567e10"}, 1234.567e10)                     ->Name("ssa s = \"1234.567e10\"; double res = *s.to_double()");

void AppendStreamConstLiteral(benchmark::State& state) {
    for (auto _: state) {
        std::string result;
        std::stringstream str;
        for (size_t c = 0; c < 64; c++) {
            str << TEXT_16;
        }
        result = str.str();
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(str);
    }
}

void AppendStdStringConstLiteral(benchmark::State& state) {
    for (auto _: state) {
        std::string result;
        for (size_t c = 0; c < 64; c++) {
            result += TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
    }
}

template<unsigned N>
void AppendLstringConstLiteral(benchmark::State& state) {
    for (auto _: state) {
        lstringa<N> result;
        for (size_t c = 0; c < 64; c++) {
            result += TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.length() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(__)->Name("-- Append const literal of 16 bytes 64 times, 1024 total length --")->Repetitions(1);
BENCHMARK(AppendStreamConstLiteral)       ->Name("std::stringstream str; ... str << \"abbaabbaabbaabba\";");
BENCHMARK(AppendStdStringConstLiteral)    ->Name("std::string str; ... str += \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringConstLiteral<8>)   ->Name("lstringa<8> str; ... str += \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringConstLiteral<128>) ->Name("lstringa<128> str; ... str += \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringConstLiteral<512>) ->Name("lstringa<512> str; ... str += \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringConstLiteral<1024>)->Name("lstringa<1024> str; ... str += \"abbaabbaabbaabba\";");

void AppendStreamStrConstLiteral(benchmark::State& state) {
    std::string s1 = TEXT_16;
    for (auto _: state) {
        std::string result;
        std::stringstream s;
        for (size_t c = 0; c < 32; c++) {
            s << s1 << TEXT_16;
        }
        result = s.str();
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(s1);
    }
}

void AppendStdStrStrConstLiteral(benchmark::State& state) {
    std::string p1 = TEXT_16;
    for (auto _: state) {
        std::string result;
        for (size_t c = 0; c < 32; c++) {
            result += p1 + TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(p1);
    }
}

template<unsigned N>
void AppendLstringStrConstLiteral(benchmark::State& state) {
    stringa p1 = TEXT_16;
    for (auto _: state) {
        lstringa<N> result;
        for (size_t c = 0; c < 32; c++) {
            result += p1 + TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.length() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(p1);
    }
}

BENCHMARK(__)->Name("-- Append string of 16 bytes and const literal of 16 bytes 32 times, 1024 total length --")->Repetitions(1);
BENCHMARK(AppendStreamStrConstLiteral)         ->Name("std::stringstream str; ... str << str_var << \"abbaabbaabbaabba\";");
BENCHMARK(AppendStdStrStrConstLiteral)         ->Name("std::string str; ... str += str_var + \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringStrConstLiteral<8>)     ->Name("lstringa<8> str; ... str += str_var + \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringStrConstLiteral<128>)   ->Name("lstringa<128> str; ... str += str_var + \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringStrConstLiteral<512>)   ->Name("lstringa<512> str; ... str += str_var + \"abbaabbaabbaabba\";");
BENCHMARK(AppendLstringStrConstLiteral<1024>)  ->Name("lstringa<1024> str; ... str += str_var + \"abbaabbaabbaabba\";");


void AppendStreamStrConstLiteralBig(benchmark::State& state) {
    std::string s1 = TEXT_16;
    for (auto _: state) {
        std::string result;
        std::stringstream s;
        for (size_t c = 0; c < 2048; c++) {
            s << s1 << TEXT_16;
        }
        result = s.str();
    #ifdef CHECK_RESULT
        if (result.size() != 1024 * 64) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(s1);
    }
}

void AppendStdStrStrConstLiteralBig(benchmark::State& state) {
    std::string p1 = TEXT_16;
    for (auto _: state) {
        std::string result;
        for (size_t c = 0; c < 2048; c++) {
            result += p1 + TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.size() != 1024 * 64) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(p1);
    }
}

template<unsigned N>
void AppendLstringStrConstLiteralBig(benchmark::State& state) {
    stringa p1 = TEXT_16;
    for (auto _: state) {
        lstringa<N> result;
        for (size_t c = 0; c < 2048; c++) {
            result += p1 + TEXT_16;
        }
    #ifdef CHECK_RESULT
        if (result.length() != 1024 * 64) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(p1);
    }
}

BENCHMARK(__)->Name("-- Append string of 16 bytes and const literal of 16 bytes 2048 times, 65536 total length --")->Repetitions(1);
BENCHMARK(AppendStreamStrConstLiteralBig)             ->Name("std::stringstream str; ... str << str_var << \"abbaabbaabbaabba\"; 2048 times");
BENCHMARK(AppendStdStrStrConstLiteralBig)             ->Name("std::string str; ... str += str_var + \"abbaabbaabbaabba\"; 2048 times");
BENCHMARK(AppendLstringStrConstLiteralBig<8>)         ->Name("lstringa<8> str; ... str += str_var + \"abbaabbaabbaabba\"; 2048 times");
BENCHMARK(AppendLstringStrConstLiteralBig<128>)       ->Name("lstringa<128> str; ... str += str_var + \"abbaabbaabbaabba\"; 2048 times");
BENCHMARK(AppendLstringStrConstLiteralBig<512>)       ->Name("lstringa<512> str; ... str += str_var + \"abbaabbaabbaabba\"; 2048 times");
BENCHMARK(AppendLstringStrConstLiteralBig<1024>)      ->Name("lstringa<1024> str; ... str += str_var + \"abbaabbaabbaabba\"; 2048 times");

void AppendStream2String(benchmark::State& state) {
    std::string s1 = TEXT_16;
    std::string s2 = TEXT_16;
    for (auto _: state) {
        std::string result;
        std::stringstream s;
        for (size_t c = 0; c < 32; c++) {
            s << s1 << s2;
        }
        result = s.str();
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(s1);
        benchmark::DoNotOptimize(s2);
    }
}

void AppendStdStr2String(benchmark::State& state) {
    std::string s1 = TEXT_16;
    std::string s2 = TEXT_16;

    for (auto _: state) {
        std::string result;
        for (size_t c = 0; c < 32; c++) {
            result += s1 + s2;
        }
    #ifdef CHECK_RESULT
        if (result.size() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(s1);
        benchmark::DoNotOptimize(s2);
    }
}

template<unsigned N>
void AppendLstring2String(benchmark::State& state) {
    stra s1 = TEXT_16;
    stra s2 = TEXT_16;
    for (auto _: state) {
        lstringa<N> result;
        for (size_t c = 0; c < 32; c++) {
            result += s1 + s2;
        }
    #ifdef CHECK_RESULT
        if (result.length() != 1024) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(s1);
        benchmark::DoNotOptimize(s2);
    }
}

BENCHMARK(__)->Name("-- Append 2 string of 16 bytes 32 times, 1024 total length --")->Repetitions(1);
BENCHMARK(AppendStream2String)          ->Name("std::stringstream str; ... str << str_var1 << str_var2;");
BENCHMARK(AppendStdStr2String)          ->Name("std::string str; ... str += str_var1 + str_var2;");
BENCHMARK(AppendLstring2String<8>)      ->Name("lstringa<16> str; ... str += str_var1 + str_var2;");
BENCHMARK(AppendLstring2String<128>)    ->Name("lstringa<128> str; ... str += str_var1 + str_var2;");
BENCHMARK(AppendLstring2String<512>)    ->Name("lstringa<512> str; ... str += str_var1 + str_var2;");
BENCHMARK(AppendLstring2String<1024>)   ->Name("lstringa<1024> str; ... str += str_var1 + str_var2;");

void AppendStreamStrNumStr(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            std::stringstream t;
            t << "test = " << k << " times";
            std::string result = t.str();
        #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
        #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

void AppendStdStringStrNumStr(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            std::string result = "test = " + std::to_string(k) + " times";
        #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
        #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

void AppendSprintfStrNumStr(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            char buf[100];
            std::sprintf(buf, "test = %u times", k);
            std::string result = buf;
        #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
        #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

void AppendFormatStrNumStr(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            std::string result = std::format("test = {} times", k);
        #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
        #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

template<typename T>
void AppendSimStrStrNumStrF(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            T result;
            result.format("test = {} times", k);
    #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
    #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

template<typename T>
void AppendSimStrStrNumStr(benchmark::State& state) {
    for (auto _: state) {
        for (unsigned k = 1; k <= 1'000'000'000; k *= 10) {
            T result = "test = "_ss + k + " times";
    #ifdef CHECK_RESULT
            if (!result.starts_with("test = ") || !result.ends_with(" times")) {
                state.SkipWithError("not equal");
                break;
            }
    #endif
            benchmark::DoNotOptimize(result);
            benchmark::DoNotOptimize(k);
        }
    }
}

BENCHMARK(__)->Name("-- Append text, number, text --")->Repetitions(1);
BENCHMARK(AppendStreamStrNumStr)               ->Name("std::stringstream str; str << \"test = \" << k << \" times\";");
BENCHMARK(AppendStdStringStrNumStr)            ->Name("std::string str = \"test = \" + std::to_string(k) + \" times\";");
BENCHMARK(AppendSprintfStrNumStr)              ->Name("char buf[100]; sprintf(buf, \"test = %u times\", k); std::string str = buf;");
BENCHMARK(AppendFormatStrNumStr)               ->Name("std::string str = std::format(\"test = {} times\", k);");
BENCHMARK(AppendSimStrStrNumStrF<lstringa<8>>) ->Name("lstringa<8> str; str.format(\"test = {} times\", k);");
BENCHMARK(AppendSimStrStrNumStrF<lstringa<32>>)->Name("lstringa<32> str; str.format(\"test = {} times\", k);");
BENCHMARK(AppendSimStrStrNumStr<lstringa<8>>)  ->Name("lstringa<8> str = \"test = \" + k + \" times\";");
BENCHMARK(AppendSimStrStrNumStr<lstringa<32>>) ->Name("lstringa<32> str = \"test = \" + k + \" times\";");
BENCHMARK(AppendSimStrStrNumStr<stringa>)      ->Name("stringa str = \"test = \" + k + \" times\";");

const char NUMBER_LIST[] = "1-!- 2-!-   3-!- 4 -!- 5-!- 6  -!- 7-!-  -8-!- 0xaF-!-   15-!- 010"; // 218

void SplitConvertIntStdString(benchmark::State& state) {
    std::string numbers = NUMBER_LIST;
    for (auto _: state) {
        int total = 0;
        for (size_t start = 0; start < numbers.length(); ) {
            int delim = numbers.find("-!-", start);
            if (delim == std::string::npos) {
                delim = numbers.size();
            }
            std::string part = numbers.substr(start, delim - start);
            total += std::strtol(part.c_str(), nullptr, 0);
            start = delim + 3;
        }
    #ifdef CHECK_RESULT
        if (total != 218) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(total);
        benchmark::DoNotOptimize(numbers);
    }
}

void SplitConvertIntSimStr(benchmark::State& state) {
    stra numbers = NUMBER_LIST;
    for (auto _: state) {
        int total = 0;
        for (auto splitter = numbers.splitter("-!-"); !splitter.is_done();) {
            total += splitter.next().as_int<int>();
        }
    #ifdef CHECK_RESULT
        if (total != 218) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(total);
        benchmark::DoNotOptimize(numbers);
    }
}
void SplitConvertIntSplitf(benchmark::State& state) {
    stra numbers = NUMBER_LIST;
    for (auto _: state) {
        int total = 0;
        numbers.splitf<void>("-!-", [&](ssa& part){total += part.as_int<int>();});
    #ifdef CHECK_RESULT
        if (total != 218) {
            state.SkipWithError("not equal");
            break;
        }
    #endif
        benchmark::DoNotOptimize(total);
        benchmark::DoNotOptimize(numbers);
    }
}

BENCHMARK(__)->Name("-- Split text and convert to int --")->Repetitions(1);
BENCHMARK(SplitConvertIntStdString) ->Name("std::string::find + substr + std::strtol");
BENCHMARK(SplitConvertIntSimStr)    ->Name("ssa::splitter + ssa::as_int");
BENCHMARK(SplitConvertIntSplitf)    ->Name("ssa::splitf + functor");

void ReplaceSymbolsStdStringNaiveWrong(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        ;
    std::vector<std::pair<u8s, std::string_view>> repl = {
        {'&', "&amp;"},
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"}
    };

    auto repl_all = [](std::string& str, char s, std::string_view repl) {
        size_t start_pos = 0;
        while((start_pos = str.find(s, start_pos)) != std::string::npos) {
            str.replace(start_pos, 1, repl);
            start_pos += repl.length();
        }
    };
    for (auto _: state) {
        std::string result{source};
        for (const auto& r : repl) {
            repl_all(result, r.first, r.second);
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

void ReplaceSymbolsStdStringNaiveRight(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        ;
    std::vector<std::pair<u8s, std::string_view>> repl = {
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"},
        {'&', "&amp;"},
    };

    for (auto _: state) {
        std::string result{source};
        std::string pattern;
        for (const auto& r : repl) {
            pattern += r.first;
        }
        size_t start_pos = 0;
        while((start_pos = result.find_first_of(pattern, start_pos)) != std::string::npos) {
            size_t idx = pattern.find(result[start_pos]);
            result.replace(start_pos, 1, repl[idx].second);
            start_pos += repl[idx].second.length();
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

void ReplaceSymbolsStdString(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        ;

    const std::string_view repl_from = "-<>'\"&";
    const std::string_view repl_to[] = {"", "&lt;", "&gt;", "&#39;", "&quot;", "&amp;"};

    for (auto _: state) {
        std::string result;

        for (size_t start = 0; start < source.size();) {
            size_t idx = source.find_first_of(repl_from, start);
            if (idx == std::string::npos) {
                result += source.substr(start);
                break;
            }
            if (idx > start) {
                result += source.substr(start, idx - start);
            }
            size_t what = repl_from.find(source[idx]);
            result += repl_to[what];

            start = idx + 1;
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

template<bool UseVector>
void ReplaceSymbolsDynPatternSimStr(benchmark::State& state) {
    stra source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        ;

    std::vector<std::pair<u8s, ssa>> repl = {
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"},
        {'&', "&amp;"},
    };

    for (auto _: state) {
        stringa result = expr_replace_symbols<u8s, UseVector>{source, repl};

#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

template<bool UseVector>
void ReplaceSymbolsCons2PatternSimStr(benchmark::State& state) {
    stra source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        " ksjd-fksjd \"dkjfs-jkhdf dfj ' kdkd \"dkfdkfkdjf"
        ;

    for (auto _: state) {
        stringa result = e_repl_const_symbols<UseVector>(source,
            '-', "",
            '<', "&lt;",
            '>', "&gt;",
            '\'', "&#39;",
            '\"', "&quot;",
            '&', "&amp;"
        );

#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
            " ksjdfksjd &quot;dkjfsjkhdf dfj &#39; kdkd &quot;dkfdkfkdjf"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

BENCHMARK(__)->Name("-- Replace symbols in text ~400 symbols --")->Repetitions(1);
BENCHMARK(ReplaceSymbolsStdStringNaiveWrong)        ->Name("Naive (and wrong) replace symbols with std::string find + replace");
BENCHMARK(ReplaceSymbolsStdStringNaiveRight)        ->Name("replace symbols with std::string find_first_of + replace");
BENCHMARK(ReplaceSymbolsStdString)                  ->Name("replace symbols with std::string_view find_first_of + copy");
BENCHMARK(ReplaceSymbolsDynPatternSimStr<false>)    ->Name("replace runtime symbols with string expressions and without remembering all search results");
BENCHMARK(ReplaceSymbolsDynPatternSimStr<true>)     ->Name("replace runtime symbols with simstr and memorization of all search results");
BENCHMARK(ReplaceSymbolsCons2PatternSimStr<false>)  ->Name("replace const symbols with string expressions and without remembering all search results");
BENCHMARK(ReplaceSymbolsCons2PatternSimStr<true>)   ->Name("replace const symbols with string expressions and memorization of all search results");


void ShortReplaceSymbolsStdStringNaiveWrong(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        ;
    std::vector<std::pair<u8s, std::string_view>> repl = {
        {'&', "&amp;"},
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"}
    };

    auto repl_all = [](std::string& str, char s, std::string_view repl) {
        size_t start_pos = 0;
        while((start_pos = str.find(s, start_pos)) != std::string::npos) {
            str.replace(start_pos, 1, repl);
            start_pos += repl.length();
        }
    };
    for (auto _: state) {
        std::string result{source};
        for (const auto& r : repl) {
            repl_all(result, r.first, r.second);
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

void ShortReplaceSymbolsStdStringNaiveRight(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        ;
    std::vector<std::pair<u8s, std::string_view>> repl = {
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"},
        {'&', "&amp;"},
    };

    for (auto _: state) {
        std::string result{source};
        std::string pattern;
        for (const auto& r : repl) {
            pattern += r.first;
        }
        size_t start_pos = 0;
        while((start_pos = result.find_first_of(pattern, start_pos)) != std::string::npos) {
            size_t idx = pattern.find(result[start_pos]);
            result.replace(start_pos, 1, repl[idx].second);
            start_pos += repl[idx].second.length();
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

void ShortReplaceSymbolsStdString(benchmark::State& state) {
    std::string_view source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        ;

    const std::string_view repl_from = "-<>'\"&";
    const std::string_view repl_to[] = {"", "&lt;", "&gt;", "&#39;", "&quot;", "&amp;"};

    for (auto _: state) {
        std::string result;

        for (size_t start = 0; start < source.size();) {
            size_t idx = source.find_first_of(repl_from, start);
            if (idx == std::string::npos) {
                result += source.substr(start);
                break;
            }
            if (idx > start) {
                result += source.substr(start, idx - start);
            }
            size_t what = repl_from.find_first_of(source[idx]);
            result += repl_to[what];

            start = idx + 1;
        }
#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

template<bool UseVector>
void ShortReplaceSymbolsDynPatternSimStr(benchmark::State& state) {
    stra source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        ;

    std::vector<std::pair<u8s, ssa>> repl = {
        {'-', ""},
        {'<', "&lt;"},
        {'>', "&gt;"},
        {'\'', "&#39;"},
        {'\"', "&quot;"},
        {'&', "&amp;"},
    };

    for (auto _: state) {
        stringa result = expr_replace_symbols<u8s, UseVector>{source, repl};

#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(repl);
    }
}

template<bool UseVector>
void ShortReplaceSymbolsCons2PatternSimStr(benchmark::State& state) {
    stra source =
        "abcdefg124 < jhsfjsh sjdfsh jfhjd && jdjdj >"
        ;

    for (auto _: state) {
        stringa result = e_repl_const_symbols<UseVector>(source,
            '-', "",
            '<', "&lt;",
            '>', "&gt;",
            '\'', "&#39;",
            '\"', "&quot;",
            '&', "&amp;"
        );

#ifdef CHECK_RESULT
        if (result
            !=
            "abcdefg124 &lt; jhsfjsh sjdfsh jfhjd &amp;&amp; jdjdj &gt;"
        ) {
            state.SkipWithError("not equal");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

BENCHMARK(__)->Name("-- Replace symbols in text ~40 symbols --")->Repetitions(1);
BENCHMARK(ShortReplaceSymbolsStdStringNaiveWrong)        ->Name("Short Naive (and wrong) replace symbols with std::string find + replace");
BENCHMARK(ShortReplaceSymbolsStdStringNaiveRight)        ->Name("Short replace symbols with std::string find_first_of + replace");
BENCHMARK(ShortReplaceSymbolsStdString)                  ->Name("Short replace symbols with std::string_view find_first_of + copy");
BENCHMARK(ShortReplaceSymbolsDynPatternSimStr<false>)    ->Name("Short replace runtime symbols with string expressions and without remembering all search results");
BENCHMARK(ShortReplaceSymbolsDynPatternSimStr<true>)     ->Name("Short replace runtime symbols with simstr and memorization of all search results");
BENCHMARK(ShortReplaceSymbolsCons2PatternSimStr<false>)  ->Name("Short replace const symbols with string expressions and without remembering all search results");
BENCHMARK(ShortReplaceSymbolsCons2PatternSimStr<true>)   ->Name("Short replace const symbols with string expressions and memorization of all search results");

template<size_t Long>
void ReplaceAllLongerStdString(benchmark::State& state) {
    std::string_view source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    std::string_view sample = "aaaaaaaaaaaaaaaaaaa----baaaaaaaa--------aaaaabaaaaaaaaaaaaaaaaaaaaa----a";

    std::string_view pattern = "bb";
    std::string_view repl = "----";

    std::string big_source, big_sample;
    for (int i = 0; i < Long; i++) {
        big_source += source;
        big_sample += sample;
    }

    for (auto _: state) {
        std::string result{big_source};
        size_t start_pos = 0;
        while((start_pos = result.find(pattern, start_pos)) != std::string::npos) {
            result.replace(start_pos, pattern.length(), repl);
            start_pos += repl.length();
        }
#ifdef CHECK_RESULT
        if (result != big_sample) {
            state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(pattern);
        benchmark::DoNotOptimize(repl);
    }
}

template<size_t N, size_t Count>
void ReplaceAllLongerSimString(benchmark::State& state) {
    ssa source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    ssa sample = "aaaaaaaaaaaaaaaaaaa----baaaaaaaa--------aaaaabaaaaaaaaaaaaaaaaaaaaa----a";

    lstringa<2048> big_source{Count, source}, big_sample{Count, sample};

    for (auto _: state) {
        lstringa<N> result = big_source;
        result.replace("bb", "----");

#ifdef CHECK_RESULT
        if (result.to_str() != big_sample) {
            std::cout << result.length() << ": " << result << "\n\n" << big_sample.length() << ": " << big_sample << "\n\n";
            state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

template<size_t Count>
void ReplaceAllLongerSimStringExpr(benchmark::State& state) {
    ssa source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    ssa sample = "aaaaaaaaaaaaaaaaaaa----baaaaaaaa--------aaaaabaaaaaaaaaaaaaaaaaaaaa----a";

    lstringa<2048> big_source{Count, source}, big_sample{Count, sample};

    for (auto _: state) {
        stringa result = e_repl(big_source.to_str(), "bb", "----");

#ifdef CHECK_RESULT
        if (result.to_str() != big_sample) {
            std::cout << result.length() << ": " << result << "\n\n" << big_sample.length() << ": " << big_sample << "\n\n";
            state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

template<size_t Long>
void ReplaceAllEqualStdString(benchmark::State& state) {
    std::string_view source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    std::string_view sample = "aaaaaaaaaaaaaaaaaaa--baaaaaaaa----aaaaabaaaaaaaaaaaaaaaaaaaaa--a";
    std::string_view pattern = "bb";
    std::string_view repl = "--";

    std::string big_source, big_sample;
    for (int i = 0; i < Long; i++) {
        big_source += source;
        big_sample += sample;
    }

    for (auto _: state) {
        std::string result{big_source};
        size_t start_pos = 0;
        while((start_pos = result.find(pattern, start_pos)) != std::string::npos) {
            result.replace(start_pos, pattern.length(), repl);
            start_pos += repl.length();
        }
#ifdef CHECK_RESULT
        if (result != big_sample) {
            state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(pattern);
        benchmark::DoNotOptimize(repl);
    }
}

template<size_t N, size_t Long>
void ReplaceAllEqualSimString(benchmark::State& state) {
    ssa source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    ssa sample = "aaaaaaaaaaaaaaaaaaa--baaaaaaaa----aaaaabaaaaaaaaaaaaaaaaaaaaa--a";
    lstringa<2048> big_source{Long, source}, big_sample{Long, sample};

    for (auto _: state) {
        lstringa<N> result = big_source;
        result.replace("bb", "--");

        #ifdef CHECK_RESULT
    if (result.to_str() != big_sample) {
        std::cout << result.length() << ": " << result << "\n\n" << big_sample.length() << ": " << big_sample << "\n\n";
        state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
    }
}

template<size_t Count>
void ReplaceAllEqualSimStringExpr(benchmark::State& state) {
    ssa source = "aaaaaaaaaaaaaaaaaaabbbaaaaaaaabbbbaaaaabaaaaaaaaaaaaaaaaaaaaabba";
    ssa sample = "aaaaaaaaaaaaaaaaaaa--baaaaaaaa----aaaaabaaaaaaaaaaaaaaaaaaaaa--a";
    ssa pattern = "bb";
    ssa repl = "--";

    lstringa<2048> big_source{Count, source}, big_sample{Count, sample};

    for (auto _: state) {
        stringa result = e_repl(big_source.to_str(), "bb", "--");

#ifdef CHECK_RESULT
        if (result.to_str() != big_sample) {
            std::cout << result.length() << ": " << result << "\n\n" << big_sample.length() << ": " << big_sample << "\n\n";
            state.SkipWithError("error in replace");
            break;
        }
#endif
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(source);
        benchmark::DoNotOptimize(pattern);
        benchmark::DoNotOptimize(repl);
    }
}

BENCHMARK(__)->Name("-----  Replace All Str To Longer Size ---------")->Repetitions(1);
BENCHMARK(ReplaceAllLongerStdString<1>)         ->Name("replace bb to ---- in std::string|64");
BENCHMARK(ReplaceAllLongerStdString<4>)         ->Name("replace bb to ---- in std::string|256");
BENCHMARK(ReplaceAllLongerStdString<8>)         ->Name("replace bb to ---- in std::string|512");
BENCHMARK(ReplaceAllLongerStdString<16>)        ->Name("replace bb to ---- in std::string|1024");
BENCHMARK(ReplaceAllLongerStdString<32>)        ->Name("replace bb to ---- in std::string|2048");
BENCHMARK(ReplaceAllLongerSimString<8, 1>)      ->Name("replace bb to ---- in lstringa<8>|64");
BENCHMARK(ReplaceAllLongerSimString<8, 4>)      ->Name("replace bb to ---- in lstringa<8>|256");
BENCHMARK(ReplaceAllLongerSimString<8, 8>)      ->Name("replace bb to ---- in lstringa<8>|512");
BENCHMARK(ReplaceAllLongerSimString<8, 16>)     ->Name("replace bb to ---- in lstringa<8>|1024");
BENCHMARK(ReplaceAllLongerSimString<8, 32>)     ->Name("replace bb to ---- in lstringa<8>|2048");
BENCHMARK(ReplaceAllLongerSimStringExpr<1>)     ->Name("replace bb to ---- by init stringa|64");
BENCHMARK(ReplaceAllLongerSimStringExpr<4>)     ->Name("replace bb to ---- by init stringa|256");
BENCHMARK(ReplaceAllLongerSimStringExpr<8>)     ->Name("replace bb to ---- by init stringa|512");
BENCHMARK(ReplaceAllLongerSimStringExpr<16>)    ->Name("replace bb to ---- by init stringa|1024");
BENCHMARK(ReplaceAllLongerSimStringExpr<32>)    ->Name("replace bb to ---- by init stringa|2048");

BENCHMARK(__)->Name("-----  Replace All Str To Same Size ---------")->Repetitions(1);
BENCHMARK(ReplaceAllEqualStdString<1>)          ->Name("replace bb to -- in std::string|64");
BENCHMARK(ReplaceAllEqualStdString<4>)          ->Name("replace bb to -- in std::string|256");
BENCHMARK(ReplaceAllEqualStdString<8>)          ->Name("replace bb to -- in std::string|512");
BENCHMARK(ReplaceAllEqualStdString<16>)         ->Name("replace bb to -- in std::string|1024");
BENCHMARK(ReplaceAllEqualStdString<32>)         ->Name("replace bb to -- in std::string|2048");
BENCHMARK(ReplaceAllEqualSimString<8, 1>)       ->Name("replace bb to -- in lstringa<8>|64");
BENCHMARK(ReplaceAllEqualSimString<8, 4>)       ->Name("replace bb to -- in lstringa<8>|256");
BENCHMARK(ReplaceAllEqualSimString<8, 8>)       ->Name("replace bb to -- in lstringa<8>|512");
BENCHMARK(ReplaceAllEqualSimString<8, 16>)      ->Name("replace bb to -- in lstringa<8>|1024");
BENCHMARK(ReplaceAllEqualSimString<8, 32>)      ->Name("replace bb to -- in lstringa<8>|2048");
BENCHMARK(ReplaceAllEqualSimStringExpr<1>)      ->Name("replace bb to -- by init stringa|64");
BENCHMARK(ReplaceAllEqualSimStringExpr<4>)      ->Name("replace bb to -- by init stringa|256");
BENCHMARK(ReplaceAllEqualSimStringExpr<8>)      ->Name("replace bb to -- by init stringa|512");
BENCHMARK(ReplaceAllEqualSimStringExpr<16>)     ->Name("replace bb to -- by init stringa|1024");
BENCHMARK(ReplaceAllEqualSimStringExpr<32>)     ->Name("replace bb to -- by init stringa|2048");

std::vector<stringa> prepareTestStrings(size_t length, size_t delta, size_t count) {
    std::vector<stringa> result;
    result.reserve(count);

    struct expr_rand {
        using symb_type = u8s;
        size_t len;
        size_t length() const noexcept {
            return len;
        }
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

std::vector<stringa> bs_sim = prepareTestStrings(30, 20, 10'000);

std::vector<std::string> prepareTestStdStrings() {
    std::vector<std::string> result;
    result.reserve(bs_sim.size());
    for (const auto& s: bs_sim) {
        result.emplace_back(s.to_string());
    }
    return result;
}

std::vector<std::string> bs_std = prepareTestStdStrings();

void HashMapSimStr(benchmark::State& state) {
    for (auto _: state) {
        hashStrMapA<size_t> store;
        for (size_t idx = 0; idx < bs_sim.size(); idx++) {
            store.try_emplace(bs_sim[idx], idx);
        }
#ifdef CHECK_RESULT
        if (store.size() != bs_sim.size()) {
            state.SkipWithError("bad inserts");
        }
#endif
        for (size_t idx = 0; idx < bs_sim.size(); idx++) {
            auto find = store.find(bs_sim[idx]);
            size_t res = find->second;
#ifdef CHECK_RESULT
            if (res != idx) {
                state.SkipWithError("bad find");
            }
#endif
            benchmark::DoNotOptimize(res);
        }
    }
}

void HashMapStdStr(benchmark::State& state) {
    for (auto _: state) {
        std::unordered_map<std::string, size_t> store;
        for (size_t idx = 0; idx < bs_std.size(); idx++) {
            store.try_emplace(bs_std[idx], idx);
        }
#ifdef CHECK_RESULT
        if (store.size() != bs_std.size()) {
            state.SkipWithError("bad inserts");
        }
#endif
        for (size_t idx = 0; idx < bs_std.size(); idx++) {
            auto find = store.find(bs_std[idx]);
            size_t res = find->second;
#ifdef CHECK_RESULT
            if (res != idx) {
                state.SkipWithError("bad find");
            }
#endif
            benchmark::DoNotOptimize(res);
        }
    }
}

void HashMapSimSsa(benchmark::State& state) {
    for (auto _: state) {
        hashStrMapA<size_t> store;
        for (size_t idx = 0; idx < bs_sim.size(); idx++) {
            store.emplace(bs_sim[idx], idx);
        }
#ifdef CHECK_RESULT
        if (store.size() != bs_sim.size()) {
            state.SkipWithError("bad inserts");
        }
#endif
        for (size_t idx = 0; idx < bs_sim.size(); idx++) {
            ssa key = bs_sim[idx];
            auto find = store.find(key);
            size_t res = find->second;
#ifdef CHECK_RESULT
            if (res != idx) {
                state.SkipWithError("bad find");
            }
#endif
            benchmark::DoNotOptimize(res);
        }
    }
}

void HashMapStdStrView(benchmark::State& state) {
    for (auto _: state) {
        std::unordered_map<std::string, size_t> store;
        for (size_t idx = 0; idx < bs_std.size(); idx++) {
            store.emplace(bs_std[idx], idx);
        }
#ifdef CHECK_RESULT
        if (store.size() != bs_std.size()) {
            state.SkipWithError("bad inserts");
        }
#endif
        for (size_t idx = 0; idx < bs_std.size(); idx++) {
            std::string_view key = bs_std[idx];
            auto find = store.find(std::string{key});
            size_t res = find->second;
#ifdef CHECK_RESULT
            if (res != idx) {
                state.SkipWithError("bad find");
            }
#endif
            benchmark::DoNotOptimize(res);
        }
    }
}

BENCHMARK(__)->Name("-----  Hash Map insert and find ---------")->Repetitions(1);
BENCHMARK(HashMapSimStr)->Name("hashStrMapA<size_t> emplace & find stringa;");
BENCHMARK(HashMapStdStr)->Name("std::unordered_map<std::string, size_t> emplace & find std::string;");
BENCHMARK(HashMapSimSsa)->Name("hashStrMapA<size_t> emplace & find ssa;");
BENCHMARK(HashMapStdStrView)->Name("std::unordered_map<std::string, size_t> emplace & find std::string_view;");

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Набор тестов, имитирующий довольно типовой сценарий, похож на то, что встречалось в работе
// По имеющимся данным о неких функциях и их параметрах - построить полное имя функции
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class Types {
    int2,
    int4,
    int8,
    bytea,
    text,
    char_,
    varchar,
    boolean,
    last,
};

const ssa type_names[] = {
    "int2",
    "int4",
    "int8",
    "bytea",
    "text",
    "char",
    "varchar",
    "boolean"
};

const std::string_view type_names_sv[] = {
    "int2",
    "int4",
    "int8",
    "bytea",
    "text",
    "char",
    "varchar",
    "boolean"
};

constexpr bool is_power_of_two_or_zero(uint32_t value) {
    return !(value & (value - 1));
}

struct type_set {
    uint32_t value;
    void to_simstr(mutable_str<u8s> auto& str) const {
        if (!value) {
            str += "{?}";
        } else if (value == 0xFFFFFFFF) {
            str += "any";
        } else {
            if (!is_power_of_two_or_zero(value)) {
                str += "{";
            }
            bool add_comma = false;
            for (unsigned idx = 0; idx < (unsigned)Types::last; idx++) {
                if (value & (1 << idx)) {
                    str += e_if(add_comma, ", ") + type_names[idx];
                    add_comma = true;
                }
            }
            if (!is_power_of_two_or_zero(value)) {
                str += "}";
            }
        }
    }
    auto get_simstr() const {
        lstringa<128> str;
        if (!value) {
            str = "{?}";
        } else if (value == 0xFFFFFFFF) {
            str = "any";
        } else {
            if (!is_power_of_two_or_zero(value)) {
                str = "{";
            }
            bool add_comma = false;
            for (unsigned idx = 0; idx < (unsigned)Types::last; idx++) {
                if (value & (1 << idx)) {
                    str += e_if(add_comma, ", ") + type_names[idx];
                    add_comma = true;
                }
            }
            if (!is_power_of_two_or_zero(value)) {
                str += "}";
            }
        }
        return str;
    }
    void to_stdstr(std::string& str) const {
        if (!value) {
            str += "{?}";
        } else if (value == 0xFFFFFFFF) {
            str += "any";
        } else {
            if (!is_power_of_two_or_zero(value)) {
                str += "{";
            }
            bool add_comma = false;
            for (unsigned idx = 0; idx < (unsigned)Types::last; idx++) {
                if (value & (1 << idx)) {
                    if (add_comma) {
                        str += ", ";
                    }
                    str += type_names_sv[idx];
                    add_comma = true;
                }
            }
            if (!is_power_of_two_or_zero(value)) {
                str += "}";
            }
        }
    }
    void to_stream(std::ostream& str) const {
        if (!value) {
            str << "{?}";
        } else if (value == 0xFFFFFFFF) {
            str << "any";
        } else {
            if (!is_power_of_two_or_zero(value)) {
                str << "{";
            }
            bool add_comma = false;
            for (unsigned idx = 0; idx < (unsigned)Types::last; idx++) {
                if (value & (1 << idx)) {
                    if (add_comma) {
                        str << ", ";
                    }
                    str << type_names_sv[idx];
                    add_comma = true;
                }
            }
            if (!is_power_of_two_or_zero(value)) {
                str << "}";
            }
        }
    }
};

struct param {
    type_set allowed_types;
    bool optional;
};

struct function {
    stringa name;
    std::string std_name;
    std::vector<param> params;
    Types ret_type;
    bool has_ret_type_resolver;
    bool unlim_params;
    // Построение полного имени с помощью simstr строковых объектов и выражений
    stringa build_full_name() const {
        lstringa<512> str = e_choice(has_ret_type_resolver, "any", type_names[(unsigned)ret_type]) + " " + name + "(";

        bool add_comma = false;

        for (const auto& param : params) {
            str += e_if(add_comma, ", ") + e_if(param.optional, "[");
            param.allowed_types.to_simstr(str);
            if (param.optional) {
                str += "]";
            }
            add_comma = true;
        }
        return str + e_if(unlim_params, e_if(add_comma, ", ") + "...") + ")";
    }
    stringa build_full_name1() const {
        lstringa<512> str = e_choice(has_ret_type_resolver, "any", type_names[(unsigned)ret_type]) + " " + name + "(";

        bool add_comma = false;

        for (const auto& param : params) {
            str += e_if(add_comma, ", ") + e_if(param.optional, "[") + param.allowed_types.get_simstr() + e_if(param.optional, "]");
            add_comma = true;
        }
        return str + e_if(unlim_params, e_if(add_comma, ", ") + "...") + ")";
    }
    // Построение полного имени с помощью std::string
    std::string build_full_name_std() const {
        std::string str{has_ret_type_resolver ? "any"sv : type_names_sv[(unsigned)ret_type]};
        str += " ";
        str += std_name;
        str += "(";

        bool add_comma = false;

        for (const auto& param : params) {
            if (add_comma) {
                str += ", ";
            }
            if (param.optional) {
                str += "[";
            }
            param.allowed_types.to_stdstr(str);
            if (param.optional) {
                str += "]";
            }
            add_comma = true;
        }
        if (unlim_params) {
            if (add_comma) {
                str += ", ";
            }
            str += "...";
        }
        str += ")";
        //std::cout << "Len=" << str.length() << ", Cap=" << str.capacity() << "\n";
        return str;
    }
    // Построение полного имени с помощью std::string
    std::string build_full_name_std1() const {
        std::string str{has_ret_type_resolver ? "any"sv : type_names_sv[(unsigned)ret_type]};
        str += " " + std_name + "(";

        bool add_comma = false;

        for (const auto& param : params) {
            if (add_comma) {
                str += ", ";
            }
            if (param.optional) {
                str += "[";
            }
            param.allowed_types.to_stdstr(str);
            if (param.optional) {
                str += "]";
            }
            add_comma = true;
        }
        if (unlim_params) {
            if (add_comma) {
                str += ", ";
            }
            str += "...";
        }
        str += ")";
        //std::cout << "Len=" << str.length() << ", Cap=" << str.capacity() << "\n";
        return str;
    }
    // Построение полного имени с помощью std::ostringstream
    std::string build_full_name_stream() const {
        std::ostringstream str;
        if (has_ret_type_resolver) {
            str << "any";
        } else {
            str << type_names_sv[(unsigned)ret_type];
        }
        str << " " << std_name << "(";

        bool add_comma = false;

        for (const auto& param : params) {
            if (add_comma) {
                str << ", ";
            }
            if (param.optional) {
                str << "[";
            }
            param.allowed_types.to_stream(str);
            if (param.optional) {
                str << "]";
            }
            add_comma = true;
        }
        if (unlim_params) {
            if (add_comma) {
                str << ", ";
            }
            str << "...";
        }
        str << ")";
        return str.str();
    }
};
// Тестовые варианты функций
const struct {
    function f;
    std::string_view check;
} functions[] = {
    {{"func1", "func1", {}, Types::boolean, true, true}, "any func1(...)"},
    {{"function2", "function2", {{4, false}, {12, true}}, Types::int2, false, false}, "int2 function2(int8, [{int8, bytea}])"},
    {{"func3", "func3", {{16, false}, {7, false}, {32, true}}, Types::char_, false, true}, "char func3(text, {int2, int4, int8}, [char], ...)"},
    {{"function4", "function4", {{10, false}, {64, false}, {31, true}}, Types::char_, true, false}, "any function4({int4, bytea}, varchar, [{int2, int4, int8, bytea, text}])"},
    {{"f5", "f5", {{10, false}, {64, false}, {0xFFFFFFFF, false}}, Types::text, false, true}, "text f5({int4, bytea}, varchar, any, ...)"},
};

// Замеры скорости выполнения разных вариантов

//> stringa build_full_name() const {
void BuildFuncNameSimStr(benchmark::State& state) {
    for (auto _: state) {
        for (const auto& f : functions) {
            stringa res = f.f.build_full_name();
            benchmark::DoNotOptimize(res);
            #ifdef CHECK_RESULT
                if (res != ssa{f.check}) {
                    std::cout << res << "\n";
                    state.SkipWithError("not equal");
                    break;
                }
            #endif
        }
    }
}

//> stringa build_full_name1() const {
void BuildFuncNameSimStr1(benchmark::State& state) {
    for (auto _: state) {
        for (const auto& f : functions) {
            stringa res = f.f.build_full_name1();
            benchmark::DoNotOptimize(res);
            #ifdef CHECK_RESULT
                if (res != ssa{f.check}) {
                    std::cout << res << "\n";
                    state.SkipWithError("not equal");
                    break;
                }
            #endif
        }
    }
}

//> std::string build_full_name_std() const {
void BuildFuncNameStdStr(benchmark::State& state) {
    for (auto _: state) {
        for (const auto& f : functions) {
            std::string res = f.f.build_full_name_std();
            benchmark::DoNotOptimize(res);
            #ifdef CHECK_RESULT
                if (res != f.check) {
                    std::cout << res << "\n";
                    state.SkipWithError("not equal");
                    break;
                }
            #endif
        }
    }
}

//> std::string build_full_name_std1() const {
void BuildFuncNameStdStr1(benchmark::State& state) {
    for (auto _: state) {
        for (const auto& f : functions) {
            std::string res = f.f.build_full_name_std1();
            benchmark::DoNotOptimize(res);
            #ifdef CHECK_RESULT
                if (res != f.check) {
                    std::cout << res << "\n";
                    state.SkipWithError("not equal");
                    break;
                }
            #endif
        }
    }
}

//> std::string build_full_name_stream() const {
void BuildFuncNameStream(benchmark::State& state) {
    for (auto _: state) {
        for (const auto& f : functions) {
            std::string res = f.f.build_full_name_stream();
            benchmark::DoNotOptimize(res);
            #ifdef CHECK_RESULT
                if (res != f.check) {
                    std::cout << res << "\n";
                    state.SkipWithError("not equal");
                    break;
                }
            #endif
        }
    }
}

BENCHMARK(__)->Name("-----  Build Full Func Name ---------")->Repetitions(1);
BENCHMARK(BuildFuncNameStdStr)         ->Name("Build func full name std::string;");
BENCHMARK(BuildFuncNameStdStr1)        ->Name("Build func full name std::string 1;");
BENCHMARK(BuildFuncNameStream)         ->Name("Build func full name std::stream;");
BENCHMARK(BuildFuncNameSimStr)         ->Name("Build func full name stringa;");
BENCHMARK(BuildFuncNameSimStr1)        ->Name("Build func full name stringa 1;");

int main(int argc, char** argv) {
    std::cout << "Benchmarks of simstr, version " SIMSTR_VERSION << "\n"
        << "Sources: https://github.com/orefkov/simstr\n"
        << "Results: https://orefkov.github.io/simstr/results.html\n" << std::endl;

#ifdef EMSCRIPTEN
    EM_ASM({ console.log(navigator.userAgent); });
    if (argc == 2 && stra{argv[1]} == "-sync") {
        do_sync = true;
        argc = 1;
    }
#endif

    char arg1[] = "--benchmark_repetitions=4", arg2[] = "--benchmark_report_aggregates_only=true";
    char* my_params[] = {argv[0], arg1, arg2};
    if (argc < 2) {
        argc = 3;
        argv = my_params;
    }
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
#ifdef EMSCRIPTEN
    std::cout << "Done!\n";
    EM_ASM({ on_done(); });
#endif

    return 0;
}
