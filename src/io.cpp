#include "io.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

using std::cerr;
using std::endl;

std::vector<double> IO::read_psllh(const std::string path) {
    assert(std::filesystem::exists(path));
    std::ifstream file(path);

    unsigned int num_entries = 0;
    file >> num_entries;
    assert(num_entries > 0);

    std::vector<double> result;
    result.reserve(num_entries);

    while(!file.eof()) {
        double num;
        file >> num;
        if (file) {
            // Only insert if there were no errors
            result.push_back(num);
        }
    }

    assert(result.size() == num_entries);
    if (num_entries != result.size()) {
        cerr << "[WARN] Header count did not match number of entries in " << path << endl;
        cerr << "Header: " << num_entries << endl;
        cerr << "Actual: " << result.size() << endl;
    }

    return result;
}

std::vector<double> IO::read_binpsllh(const std::string path) {
    assert(std::filesystem::exists(path));
    std::ifstream file(path);

    uint64_t num_entries = 0;
    file.read(reinterpret_cast<char *>(&num_entries), sizeof(num_entries));

    std::vector<double> result;
    result.resize(num_entries);

    file.read(reinterpret_cast<char *>(&result[0]), sizeof(double) * (num_entries));
    assert(result.size() == num_entries);

    return result;
}
