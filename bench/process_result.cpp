#include <simstr/sstring.h>
#include <filesystem>
#include <fstream>
#include <array>
#include <algorithm>

using namespace simstr;

struct result_info;

using out_t = lstringa<0>;
using results_vector = std::vector<result_info>;

bool extract_cpu_info(ssa text, ssa& res) {
    // Найдём, где начинается "Run on ", потом где за ним начинается "\n---"
    size_t start = text.find("Run on "), end = text.find("\n---", start);
    // Если что-то не нашлось - ошибка
    if (start == str::npos || end == str::npos) {
        return false;
    }
    res = text.from_to(start, end);
    return true;
}

struct result_info {
    stringa text_;
    stringa platform_;
    ssa current_text_{text_};
    ssa cpu_info_;

    result_info(stringa text, stringa platform) : text_(std::move(text)), platform_(std::move(platform)) {
        if (!extract_cpu_info(current_text_, cpu_info_)) {
            std::cerr << "Not found cpu info for platform " << platform_;
            throw std::runtime_error{"Not found cpu info"};
        }
        // Текущее положение поставим сразу за cpuinfo и откинем завершающие переводы строк
        current_text_ = current_text_(cpu_info_.end() - current_text_.begin() + 1).trimmed_right("\n");
    }
};

template<typename T>
T path_to_str(const std::filesystem::path& path) {
    auto utf = path.u8string();
    return T{ssa{(const u8s*)utf.c_str(), utf.size()}};
}

stringa get_file_content(stra filePath) {
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Can not open file " << filePath << std::endl;
        throw std::runtime_error{"Can not open file"};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // Такой тип удобен для передачи потом в stringa
    lstringsa<0> result;
    file.read(result.set_size(size), size);
    result.replace("\r\n", "\n");
    return result;
}

results_vector get_results_infos() {
    // Отберём в текущей директории все файлы с названиями, начинающимся на "result-" и заканчивающимися
    // на ".txt" и отсортируем их по имени
    const ssa prefix = "result-", suffix = ".txt";
    std::vector<stringa> fileNames;
    for (const auto& f : std::filesystem::directory_iterator{"."}) {
        if (f.is_regular_file()) {
            auto fileName = path_to_str<lstringa<64>>(f.path().filename());
          #ifdef _WIN32
            if (fileName.starts_with_ia(prefix) && fileName.ends_with_ia(suffix)) {
          #else
            if (fileName.starts_with(prefix) && fileName.ends_with(suffix)) {
          #endif
                fileNames.emplace_back(fileName);
            }
        }
    }
    
    results_vector results;
    
    if (fileNames.size()) {
        std::sort(fileNames.begin(), fileNames.end());
        results.reserve(fileNames.size());
        for (const auto& f : fileNames) {
            results.emplace_back(get_file_content(f), f(prefix.length(), -suffix.length()));
        }
    }
    return results;
}

void write_header(out_t& out) {
    out += get_file_content("header.txt");
}

void write_platforms_cpu(out_t& out, const results_vector& results) {
    out += R"(<div class="test_platforms"><h3>Test configurations:</h3><ul>)";
    for (const auto& r : results) {
        size_t rp = r.cpu_info_.find('(') + 1, lp = r.cpu_info_.find(')', rp);
        ssa shortCpuInfo = r.cpu_info_.from_to(rp, lp);
        ssa cpuInfo = r.cpu_info_(lp + 2);
        out.append_formatted(R"--(
<li><span class="platform">{}</span><span class="tooltip">{}<span class="tooltiptext">{}</span></span></li>)--",
            r.platform_, shortCpuInfo, cpuInfo);
    }
    out += "\n</ul></div>\n";
}

auto repl_html_symbols(ssa text) {
    return e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;");
}

void write_benchset_header(out_t& out, const results_vector& results, ssa benchsetName) {
    size_t width = 40 / results.size();
    out += "\n\n<div class=\"benchset\"><h4>" + repl_html_symbols(benchsetName) + "</h4>\n<table><thead><tr><th>Benchmark name</th><th width=\"5%\">Comment</th>";
    for (const auto& r : results) {
        out += "<th width=\""_ss + width + "%\">" + r.platform_ + "</th>";
    }
    out += "</tr></thead><tbody>";
}

void write_benchset_footer(out_t& out) {
    out += "\n</tbody></table></div>";
}

ssa extract_name_result(ssa line, ssa& result) {
    size_t ns = line.find(" ns ");
    if (ns == str::npos) {
        std::cerr << "Not found ' ns ' in line " << line << std::endl;
        throw std::runtime_error{"bad line"};
    }
    line.len = ns;
    size_t end = line.find_last(' ');
    result = line(end + 1);
    return line(0, end).trimmed_right();
}

ssa extract_comment(ssa commentsText, ssa benchmarkName) {
    size_t idx = commentsText.find_end(lstringa<120>{"- " + benchmarkName + "\n"});
    if (idx != str::npos) {
        if (commentsText[idx] != '\n' && commentsText(idx, 2) != "- ") {
            return commentsText(idx, commentsText.find("\n\n", idx) - idx);
        }
    }
    return stra::empty_str;
}

void write_one_result(out_t& out, ssa result, ssa line) {
    out += "<td class=\"benchmarkresult\">" + result + "</td>";
}

ssa extract_source_for_benchmark(ssa benchName, ssa sourceText) {
    static hashStrMapA<stringa> textes;

    size_t delim = benchName.find_last('/');
    if (delim != str::npos && std::get<1>(benchName(delim + 1).to_int<unsigned, false, 10, false, false>()) == IntConvertResult::Success) {
        benchName.len = delim;
    }
    auto [it, not_exist] = textes.try_emplace(benchName);
    if (not_exist) {
        // Ищем имя функции для этого бенчмарка
        size_t start = sourceText.find(lstringa<128>{"->Name(\"" + e_repl(benchName, "\"", "\\\"") + "\")"});
        if (start == str::npos) {
            std::cerr << "Can not found benchmark function name for " << benchName << std::endl;
            return stra::empty_str;
        }
        start = sourceText(0, start).find('(', sourceText.find_last('\n', start - 1));
        if (start == str::npos) {
            std::cerr << "Can not found benchmark function name for " << benchName << std::endl;
            return stra::empty_str;
        }
        start++;
        size_t end = sourceText.find_first_of(")<,", start);
        ssa funcName = sourceText.from_to(start, end);
        auto [func_it, not_exist] = textes.try_emplace(funcName);
        if (not_exist) {
            // Теперь ищем саму эту функцию
            start = sourceText.find(lstringa<128>{funcName + "(benchmark::State"});
            if (start == str::npos) {
                std::cerr << "Can not found source function " << funcName << " for benchmark " << benchName << std::endl;
                return stra::empty_str;
            }
            start = sourceText.find_last('\n', start);
            size_t templ_start = sourceText.find_last('\n', start) + 1;
            if (sourceText(templ_start).starts_with("template")) {
                start = templ_start;
            } else {
                start++;
            }
            end = sourceText.find("\n}", start);
            func_it->second = repl_html_symbols(sourceText.from_to(start, end + 2));
        }
        it->second = func_it->second;
    }
    return it->second;
}

void write_benchmarks(out_t& out, const results_vector& results, ssa sourceText, ssa commentsText) {
    int state = 0;
    std::vector<Splitter<u8s>> splitters;
    splitters.reserve(results.size());

    for (const auto& r : results) {
        splitters.emplace_back(r.current_text_.splitter("\n"));
    }
    while(!splitters[0].isDone()) {
        ssa line = splitters[0].next(), benchName, result;
        
        if (line.starts_with("--") && !line.ends_with("---")) {
            // Начинается новый набор бенчмарков
            if (state != 0) {
                write_benchset_footer(out);
            }
            benchName = extract_name_result(line, result);
            write_benchset_header(out, results, benchName.from_to(benchName.find(' ') + 1, benchName.find_last(' ')).trimmed());
            state = 1;
        }
        if (state == 2) {
            benchName = extract_name_result(line, result);
            auto source = extract_source_for_benchmark(benchName, sourceText);
            auto comment = extract_comment(commentsText, benchName);
            // Нужно вывести название бенча и коммент
            out += "\n<tr><td class=\"benchmarkname\"><span class=\"tooltip\">" +
                repl_html_symbols(benchName) +
                "<span class=\"tooltiptext code\">" +
                source + "</span></span></td><td>" +
                e_choice(comment.is_empty(), eea, "<span class=\"tooltip info\">&nbsp;>>&nbsp;<span class=\"tooltiptext\">" + comment + "</span></span>") +
                "</td>";
            write_one_result(out, result, line);
        }
        for (unsigned idx = 1; idx < results.size(); idx++) {
            if (splitters[idx].isDone()) {
                std::cerr << "Not expected end of file for " << results[idx].platform_ << std::endl;
                throw std::runtime_error{"Not expected end of file"};
            }
            line = splitters[idx].next();
            if (state == 2) {
                ssa rbench_name = extract_name_result(line, result);
                if (rbench_name != benchName) {
                    std::cerr << "In results for " << results[idx].platform_ << " benchmark '" << rbench_name << "' does not match with other results" << std::endl;
                    result = stra::empty_str;
                }
                write_one_result(out, result, line);
            }
        }
        if (state == 2) {
            // Нужно закончить строку
            out += "</tr>";
        }
        if (state == 1) {
            state = 2;
        }
    }
    if (state) {
        write_benchset_footer(out);
    }
}

out_t create_page_text() {
    auto sourceText = get_file_content("bench_str.cpp");
    auto commentsText = get_file_content("comments.txt");

    auto resultInfos = get_results_infos();
    out_t out;
    out.reserve_no_preserve(64 * 1024);
    write_header(out);
    write_platforms_cpu(out, resultInfos);
    write_benchmarks(out, resultInfos, sourceText, commentsText);
    out += "</body></html>";
    return out;
}

int main() {
    int res = 0;

    try {
        auto pageText = create_page_text();
        std::ofstream fout{"results.html", std::ios::binary | std::ios::trunc};
        if (!fout.is_open()) {
            throw std::runtime_error{"Can not open file results.html for write results"};
        }
        fout.write(pageText.c_str(), pageText.size());
    } catch (const std::exception& err) {
        std::cerr << "Catch exception: " << err.what() << "\nProgram exited\n";
        res = -1;
    } catch (...) {
        std::cerr << "Catch unknown exception.\nProgram exited\n";
        res = -2;
    }
    return res;
}
