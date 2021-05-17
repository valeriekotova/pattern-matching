#pragma once

#include <string>
#include <vector>
#include <chrono>

class PatternMatchingCPU final {
public:
    PatternMatchingCPU(const std::vector<std::string>& patterns) : patterns_(patterns) {}

    std::vector<size_t> GetCounts(const std::string& text, size_t& time) const;
private:

    size_t find(const std::string& text, const std::string& pattern) const;

    std::vector<std::string> patterns_;
};