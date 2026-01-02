#include "logger.hpp"
#include <fstream>

static const char* LOG_FILE = "program.log";

void log_line(const std::string& msg)
{
    std::ofstream f(LOG_FILE, std::ios::app);
    f << msg << std::endl;
}