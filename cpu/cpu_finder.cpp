#include "cpu_finder.h"

size_t PatternMatchingCPU::find(const std::string& text, const std::string& pattern) const {

    if ( (pattern.size() > text.size()) || (pattern.empty()) )
        return 0;

    size_t ans = 0, i = 0;
    for (i = text.find(pattern, i); i != std::string::npos; i = text.find(pattern, i + 1))
        ans++;

    return ans;
}

std::vector<size_t> PatternMatchingCPU::GetCounts(const std::string& text, size_t& time) const {

    auto start = std::chrono::system_clock::now();

    std::vector<size_t> res;
    res.reserve(patterns_.size());

    for(const auto& pattern : patterns_)
        res.emplace_back(find(text, pattern));

    auto finish = std::chrono::system_clock::now();
    time = (finish - start).count();

    return res;
}