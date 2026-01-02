#include "cli.hpp"
#include "shared.hpp"

#include <iostream>
#include <string>

void cli_loop()
{
    std::string cmd;
    while (std::getline(std::cin, cmd)) {
        if (cmd.rfind("set", 0) == 0) {
            long long v = std::stoll(cmd.substr(4));
            shared->counter.store(v);
        }
    }
}