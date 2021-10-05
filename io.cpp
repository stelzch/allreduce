#include "io.hpp"
#include <iostream>
#include <fstream>
#include <cassert>

using std::cerr;
using std::endl;

std::vector<double> IO::read_psllh(const std::string path) {
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

    if (num_entries != result.size()) {
        cerr << "[WARN] Header count did not match number of entries in " << path << endl;
        cerr << "Header: " << num_entries << endl;
        cerr << "Actual: " << result.size() << endl;
    }

    return result;
}
