#include "monkey/lexer.h"
#include "monkey/token.h"

#include <fmt/format.h>
#include <magic_enum/magic_enum_format.hpp>

#include <iostream>
#include <string_view>

constexpr std::string_view PROMPT = ">> ";

namespace monkey {

void start() {
    while (true) {
        fmt::print(PROMPT);

        std::string line;

        // Handle EOF (e.g., Ctrl+D) gracefully by breaking the loop
        if (!std::getline(std::cin, line)) {
            break;
        }

        auto lexer = Lexer(line);

        for (Token tok = lexer.nextToken(); tok.type != TokenType::EOF_TOKEN;
             tok = lexer.nextToken()) {
            fmt::print("Type: {}, Literal: {}\n", tok.type, tok.literal);
        }
    }
}

} // namespace monkey