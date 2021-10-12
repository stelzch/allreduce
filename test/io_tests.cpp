#include <string>
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <iostream>
#include "io.hpp"

using std::string;
using std::filesystem::path;

// Assume test binary is always launched from build directory
string get_file_path(string relative_path) {
    const auto cwd = std::filesystem::current_path();
    path file_path = cwd / path("test/data") / relative_path;
    EXPECT_TRUE(std::filesystem::exists(file_path))
        << "Could not find test file " << file_path;
    return file_path.string();
}

TEST(IOTests, PsllhLoading) {
    auto data = IO::read_psllh(get_file_path("example.psllh"));

    EXPECT_EQ(data.size(), 3);
    EXPECT_EQ(data[0], 1.0);
    EXPECT_EQ(data[2], 3.0);
}

TEST(IOTests, BinPsllhComparison) {
    auto text_variant = IO::read_psllh(get_file_path("fusob.psllh"));
    auto binary_variant = IO::read_binpsllh(get_file_path("fusob.binpsllh"));

    EXPECT_EQ(text_variant.size(), binary_variant.size());
    
    for (size_t i = 0; i < text_variant.size(); i++) {
        // The numbers stored as text have 6 decimals, thus we expect the error
        // to be less than 1e-6
        EXPECT_NEAR(text_variant[i], binary_variant[i],
                1e-4);
        break;
    }
    
}
