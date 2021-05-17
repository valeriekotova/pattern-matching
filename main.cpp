#include <iostream>
#include "gpu/gpu_finder.h"
#include "cpu/cpu_finder.h"

std::string ReadString(std::istream& in) {

    long size = 0;
    in >> size;
    if (!size)
        return {};

    std::string str;
    str.resize(size);
    in.ignore(1);
    in.read(str.data(), size);
    in.ignore();

    return str;
}

int main() {

    try {
        std::istream& in = std::cin;
/*      std::ifstream in("tests//my_test.txt");
        if (!in.is_open())
            throw std::runtime_error("Can't open file");*/

        const std::string text = ReadString(in);

        size_t num_of_pat = 0;
        in >> num_of_pat;

        std::vector<std::string> patterns;
        patterns.reserve(num_of_pat);


        for (int i = 0; i < num_of_pat; ++i) {
            const std::string pat = ReadString(in);
            patterns.push_back(pat);
        }

        PatternMatchingGPU Finder(patterns);
        size_t time = 0;

        auto result = Finder.Match(text, time);
        for (int i = 0; i < result.size(); ++i)
            std::cout << i + 1 << " " << result[i] << std::endl;

    } catch (std::exception& e) {
        std::cerr<<e.what()<<std::endl;
        exit(1);
    }

    return 0;
}