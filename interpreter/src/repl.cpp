#include "monkey/repl.h"
#include "monkey/eval.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"

#include <fmt/ostream.h>
#include <magic_enum/magic_enum_format.hpp>

#include <iostream>
#include <string_view>

namespace {

constexpr std::string_view MONKEY_FACT = R"(            __,__
   .--.  .-"     "-.  .--.
  / .. \/  .-. .-.  \/ .. \
 | |  '|  /   Y   \  |'  | |
 | \   \  \ 0 | 0 /  /   / |
  \ '- ,\.-"""""""-./, -' /
   ''-' /_   ^ ^   _\ '-''
       |  \._   _./  |
       \   \ '~' /   /
        '._ '-=-' _.'
           '-----'
)";

constexpr std::string_view PROMPT = ">> ";

void printParserErrors(const std::vector<std::string> &errors, std::ostream &output) {
    fmt::print(output, "{}", MONKEY_FACT);
    fmt::print(output, "Woops! We ran into some monkey business here!\n");
    fmt::print(output, "parser has {} errors:\n", errors.size());
    for (const auto &error : errors) {
        fmt::print(output, "\t{}\n", error);
    }
}

} // namespace

namespace monkey {

void start(std::istream &input, std::ostream &output) {
    while (true) {
        fmt::print(PROMPT);

        std::string line;

        // Handle EOF (e.g., Ctrl+D) gracefully by breaking the loop
        if (!std::getline(input, line)) {
            break;
        }

        auto parser = Parser(Lexer(line));
        auto program = parser.parseProgram();

        if (!parser.errors().empty()) {
            printParserErrors(parser.errors(), output);
            continue;
        }

        Object result = eval(*program);
        fmt::print(output, "{}\n", inspect(result));
    }
}

} // namespace monkey