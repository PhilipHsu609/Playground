#include "monkey/repl.h"

#include <fmt/core.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    if (pw == nullptr) {
        fmt::print(stderr, "Failed to get user information for UID: {}\n", uid);
        return 1;
    }

    fmt::println("Hello {}! This is the Monkey programming language!", pw->pw_name);
    fmt::println("Feel free to type in commands");

    monkey::start();

    return 0;
}
