#pragma once
#include <vector>
#include <string>

namespace IO {
    std::vector<double> read_psllh(const std::string path);
    std::vector<double> read_binpsllh(const std::string path);
}
