#include <simstr/sstring.h>
#include <filesystem>
#include <fstream>
#include <array>
#include <algorithm>
#include <map>

using namespace simstr;

struct result_info;

using out_t = lstringa<0>;
using results_vector = std::vector<result_info>;

bool extract_cpu_info(ssa text, ssa& res) {
    // Найдём, где начинается "Run on ", потом где за ним начинается "\n---"
    // Find where "Run on" begins, then where after it "\n---" begins
    size_t start = text.find("Run on "), end = text.find("\n---", start);
    // Если что-то не нашлось - ошибка
    // If something was not found - an error
    if (start == str::npos || end == str::npos) {
        return false;
    }
    res = text.from_to(start, end);
    return true;
}

struct result_info {
    stringa text_;
    stringa platform_;
    ssa current_text_ = text_.to_str();
    ssa cpu_info_;

    result_info(stringa text, stringa platform) : text_(std::move(text)), platform_(std::move(platform)) {
        if (!extract_cpu_info(current_text_, cpu_info_)) {
            std::cerr << "Not found cpu info for platform " << platform_;
            throw std::runtime_error{"Not found cpu info"};
        }
        // Текущее положение поставим сразу за cpuinfo и откинем завершающие переводы строк
        // We will put the current position immediately after cpuinfo and discard the final line feeds
        current_text_ = current_text_(cpu_info_.end() - current_text_.begin() + 1).trimmed_right("\n");
    }
};

template<typename T>
T path_to_str(const std::filesystem::path& path) {
    auto utf = path.u8string();
    return T{ssa{(const u8s*)utf.c_str(), utf.size()}};
}

std::filesystem::path str_to_path(ssa path) {
    return path.to_sv();
}

stringa get_file_content(stra filePath) {
    std::ifstream file(str_to_path(filePath), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Can not open file " << filePath << std::endl;
        throw std::runtime_error{"Can not open file"};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    // Такой тип удобен для передачи потом в stringa
    // This type is convenient for later passing to stringa
    lstringsa<0> result;
    file.read(result.set_size(size), size);
    result.replace("\r\n", "\n");
    return std::move(result);
}

results_vector get_results_infos() {
    // Отберём в директории results все файлы с названиями, заканчивающимися на ".txt" и отсортируем их по имени
    // Select all files in the results directory with names ending in ".txt" and sort them by name
    const ssa suffix = ".txt", dirForResults = "results/";
    std::vector<stringa> fileNames;
    for (const auto& f: std::filesystem::directory_iterator{str_to_path(dirForResults)}) {
        if (f.is_regular_file()) {
            auto fileName = path_to_str<lstringa<64>>(f.path().filename());
          #ifdef _WIN32
            if (fileName.ends_with_ia(suffix)) {
          #else
            if (fileName.ends_with(suffix)) {
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
            ssa fileName = f;
            // В начале имени файла может идти число и дефис, для сортировки, уберём их
            // At the beginning of the file name there can be a number and a hyphen, for sorting, remove them
            if (auto delimiter = fileName.find('-'); delimiter + 1 > 1) {
                if (fileName(0, delimiter).to_int<unsigned, false, 10, false, false>().ec == IntConvertResult::Success) {
                    fileName.remove_prefix(delimiter + 1);
                }
            }
            results.emplace_back(get_file_content(lstringa<128>{dirForResults + f}), fileName(0, -suffix.length()));
        }
    }
    return results;
}

void write_header(out_t& out) {
    out += get_file_content("header.txt");
}

void write_platforms_cpu(out_t& out, const results_vector& results) {
    lstringa<1024> script = "<script>const platform_names=[";
    out += R"=(<div>Group tests by platforms in charts: <input type="checkbox" id="gbp" checked onchange="switchGrouping()"/></div><div class="test_platforms"><h3>Test configurations:</h3><ul>)=";
    unsigned counter = 0;
    for (const auto& r : results) {
        size_t rp = r.cpu_info_.find('(') + 1, lp = r.cpu_info_.find(')', rp);
        ssa shortCpuInfo = r.cpu_info_.from_to(rp, lp);
        ssa cpuInfo = r.cpu_info_(lp + 2);
        out += e_subst(R"--(
<li><span class="platform">{}</span><span class="tooltip">{}<span class="tooltiptext">{}</span></span>&nbsp;Include in charts: <input type="checkbox" id="pl{}" checked onchange="buildCharts()"/></li>)--",
            r.platform_, shortCpuInfo, cpuInfo, counter++);
        script += "'" + e_repl(r.platform_.to_str(), "'", "\\'") + "'" + e_choice(&r == &results.back(), "]", ",");
    }
    out += "\n</ul></div></div>\n" + script + ";</script>\n";
}

auto repl_html_symbols(ssa text) {
    return e_repl_const_symbols(text, '\"', "&quot;", '<', "&lt;", '\'', "&#39;", '&', "&amp;");
}

void write_benchset_header(out_t& out, const results_vector& results, ssa benchsetName, unsigned benchSetId) {
    size_t hash = fnv_hash(benchsetName.symbols(), benchsetName.length());
    static std::unordered_map<size_t, int> ids;
    auto [id, _] = ids.try_emplace(hash, 0);
    if (id->second) {
        std::cout << "Duplicate hash for " << benchsetName << "\n";
    }
    int ii = id->second++;

    size_t width = 40 / results.size();
    out += "\n\n<div class=\"benchset\" id=\"bs"_ss + benchSetId + "\"><h4><a id=\"bs" + hash + ii + "\" href=\"#bs" +
        hash + ii + "\">#</a>&nbsp;" + repl_html_symbols(benchsetName) +
        "</h4>\n<table><thead><tr><th>Benchmark name</th><th width=\"5%\">Comment</th>";
    for (const auto& r : results) {
        out += "<th width=\""_ss + width + "%\">" + r.platform_ + "</th>";
    }
    out += "</tr></thead><tbody>";
}

void write_benchset_footer(out_t& out, ssa script) {
    out += "\n</tbody></table></div><script>" + script + "}\n]}</script>";
}

ssa extract_name_result(ssa line, ssa& result) {
    size_t ns = line.find(" ns ");
    bool inNs = true;
    if (ns == str::npos) {
        ns = line.find("ERROR OCCURRED: 'not implemented'");
        if (ns == str::npos) {
            std::cerr << "Not found ' ns ' in line " << line << std::endl;
            throw std::runtime_error{"bad line"};
        }
        inNs = false;
    }
    line.len = ns;
    if (inNs) {
        size_t end = line.find_last(' ');
        result = line(end + 1);
        if (auto rp = line.find("/repeats"); rp != str::npos) {
            line.len = rp;
        } else {
            line.len = end;
        }
    } else {
        result = "Not impl";
    }
    return line.trimmed_right();
}

ssa extract_comment(ssa commentsText, ssa benchmarkName) {
    size_t idx = commentsText.find_end(lstringa<120>{"- " + benchmarkName + "\n"});
    if (idx != str::npos) {
        if (commentsText[idx] != '\n' && commentsText(idx, 2) != "- ") {
            return commentsText.from_to(idx, commentsText.find("\n\n", idx));
        }
    }
    return stra::empty_str;
}

void write_one_result(out_t& out, ssa result, ssa line, auto& script_text, bool last) {
    out += "<td class=\"benchmarkresult\">" + result + "</td>";
    script_text += e_choice(result[0] == 'N', "NaN", result) + e_choice(last, "]", ",");
}

std::pair<ssa, size_t> extract_source_for_benchmark(ssa benchName, ssa sourceText) {
    static hashStrMapA<std::pair<stringa, size_t>> textes;
    static auto lines_pos = [&]() {
        std::map<size_t, size_t> l = {{0, 0}};
        size_t line = 0;
        sourceText.for_all_finded([&](size_t pos){
            l.emplace(pos, ++line);
        }, "\n");
        return l;
    }();

    size_t delim = benchName.find_last('/');
    if (delim != str::npos && benchName(delim + 1).to_int<unsigned, false, 10, false, false>().ec == IntConvertResult::Success) {
        benchName.len = delim;
    }
    auto [it, not_exist] = textes.try_emplace(benchName, stringa{}, 0);
    if (not_exist) {
        // Ищем имя функции для этого бенчмарка
        // Looking for the name of the function for this benchmark
        size_t start = sourceText.find(lstringa<128>{"->Name(\"" + e_repl(benchName, "\"", "\\\"") + "\")"});
        if (start == str::npos) {
            std::cerr << "Can not found benchmark function name for " << benchName << std::endl;
            return {stra::empty_str, 0};
        }
        start = sourceText(0, start).find('(', sourceText.find_last('\n', start - 1));
        if (start == str::npos) {
            std::cerr << "Can not found benchmark function name for " << benchName << std::endl;
            return {stra::empty_str, 0};
        }
        start++;
        size_t end = sourceText.find_first_of(")<,", start);
        ssa funcName = sourceText.from_to(start, end);
        auto [func_it, not_exist] = textes.try_emplace(funcName, stringa{}, 0);
        if (not_exist) {
            // Теперь ищем саму эту функцию
            // Now we look for this function itself
            start = sourceText.find(lstringa<128>{funcName + "(benchmark::State"});
            if (start == str::npos) {
                std::cerr << "Can not found source function " << funcName << " for benchmark " << benchName << std::endl;
                return {stra::empty_str, 0};
            }
            start = sourceText.find_last('\n', start);
            size_t templ_start = sourceText.find_last('\n', start) + 1;
            if (sourceText(templ_start).starts_with("template")) {
                start = templ_start;
            } else {
                start++;
            }
            size_t end = -1;
            // Проверим, возможно там есть переход на другую функцию через //>
            // Let's check, maybe there is a transition to another function via //>
            ssa prevLine = sourceText.from_to(sourceText.find_last('\n', start - 1) + 1, start);
            if (prevLine.starts_with("//> ")) {
                prevLine.remove_prefix(4);
                start = sourceText.find(prevLine);
                if (start == str::npos) {
                    std::cerr << "Not found link " << prevLine;
                    throw std::runtime_error{"Not found link"};
                }
                size_t beginLine = sourceText.find_last('\n', start);
                ssa indent = sourceText.from_to(beginLine, start);
                end = sourceText.find(lstringa<40>{indent + "}\n"}, start + prevLine.length());
                if (end == str::npos) {
                    std::cerr << "Not found end of " << prevLine;
                    throw std::runtime_error{"Not found end of func"};
                }
                lstringa<2048> text{sourceText.from_to(beginLine, end + indent.length() + 1), indent, "\n"};
                func_it->second.first = repl_html_symbols(text(1));
            } else {
                end = sourceText.find("\n}\n", start);
                func_it->second.first = repl_html_symbols(sourceText.from_to(start, end + 2));
            }
            func_it->second.second = lines_pos.lower_bound(start)->second;
        }
        it->second = func_it->second;
    }
    return {it->second.first, it->second.second};
}

void write_benchmarks(out_t& out, const results_vector& results, ssa sourceText, ssa commentsText) {
    std::vector<Splitter<u8s>> splitters;
    splitters.reserve(results.size());

    for (const auto& r : results) {
        splitters.emplace_back(r.current_text_.splitter("\n"));
    }
    bool needFooter = false, needCommaForTests = false;
    lstringa<1024> script_text;
    unsigned benchSetId = 0;
    while(!splitters[0].is_done()) {
        ssa line = splitters[0].next(), benchName, result;
        if (auto rm = line.find("_mean"); rm != str::npos) {
            benchName = extract_name_result(line, result)(0, rm);
            auto [source, line_num] = extract_source_for_benchmark(benchName, sourceText);
            auto comment = extract_comment(commentsText, benchName);
            // Нужно вывести название бенча и коммент
            // Need to display title and comment
            out += "\n<tr><td class=\"benchmarkname\"><span class=\"tooltip\"><a target=\"blank\" href=\"https://github.com/orefkov/simstr/blob/main/bench/bench_str.cpp#L"_ss +
                line_num + "\">" + repl_html_symbols(benchName) +
                "</a><span class=\"tooltiptext code\">" +
                source + "</span></span></td><td>" +
                e_if(!comment.is_empty(), "<span class=\"tooltip info\">&nbsp;>>&nbsp;<span class=\"tooltiptext\">" + comment + "</span></span>") +
                "</td>";
            script_text += e_if(needCommaForTests, "},") + "\n{name:'" + e_repl(benchName.to_str(), "'", "\\'") + "',data:[";
            write_one_result(out, result, line, script_text, result.size() == 1);

            for (unsigned idx = 1; idx < results.size(); idx++) {
                if (splitters[idx].is_done()) {
                    std::cerr << "Not expected end of file for " << results[idx].platform_ << std::endl;
                    throw std::runtime_error{"Not expected end of file"};
                }
                line = splitters[idx].next();
                ssa rbench_name = extract_name_result(line, result)(0, rm);
                if (rbench_name != benchName) {
                    while (line.find("_mean") == str::npos && !splitters[idx].is_done()) {
                        line = splitters[idx].next();
                    }
                    rbench_name = extract_name_result(line, result)(0, rm);
                    if (rbench_name != benchName) {
                        std::cerr << "In results for " << results[idx].platform_ << " benchmark '" << rbench_name
                                  << "' does not match with other results" << std::endl;
                        result = stra::empty_str;
                    }
                }
                write_one_result(out, result, line, script_text, idx == results.size() - 1);
            }
            out += "</tr>";
            needCommaForTests = true;
            continue;
        } else if (line.starts_with("--") && !line.ends_with("---")) {
            // Начинается новый набор бенчмарков
            // Starts a new set of benchmarks
            if (needFooter) {
                write_benchset_footer(out, script_text);
            }
            benchName = extract_name_result(line, result);
            // Из названия набора надо удалить начальные и конечные ---
            // From the name of the set we need to remove the beginning and end ---
            benchName = benchName.from_to(benchName.find(' ') + 1, benchName.find_last(' ')).trimmed();
            write_benchset_header(out, results, benchName, ++benchSetId);
            needFooter = true;
            needCommaForTests = false;
            script_text = "bench_sets['" + e_repl(benchName.to_str(), "'", "\\'") + "'] = {id:'bs" + benchSetId +"', tests:[";
        }
        // Эти строки надо пропустить во всех файлах
        // These lines must be skipped in all files
        for (unsigned idx = 1; idx < results.size(); idx++) {
            if (splitters[idx].is_done()) {
                std::cerr << "Not expected end of file for " << results[idx].platform_ << std::endl;
                throw std::runtime_error{"Not expected end of file"};
            }
            line = splitters[idx].next();
            if (line.starts_with("std::unordered_map<std::string, size_t> emplace & find std::string_view;_cv")) {
                int t = 1;
            }
        }
    }
    if (needFooter) {
        write_benchset_footer(out, script_text);
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
