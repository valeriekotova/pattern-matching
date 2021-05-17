#include "gpu/gpu_finder.h"
#include "cpu/cpu_finder.h"
#include <set>
#include <random>

#include <filesystem>
std::vector<std::string> GetAllTestFileNames(const std::string& dirname);


//info for generating tests
struct TestGenInfo final {
    std::string name;
    size_t size;
    char min_value, max_value; // for string
    std::vector<size_t> size_of_pat;
    char min_p, max_p; //for patterns
};
void TestGenerator(const std::vector<TestGenInfo>& files);

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

int main () {

    try {
        auto filenames = GetAllTestFileNames("tests");

        for (const auto &filename : filenames) {

            std::ifstream in(filename);
            if (!in.is_open())
                throw std::runtime_error("Can't open file: " + filename);

            const std::string text = ReadString(in);

            size_t num_of_pat = 0;
            in >> num_of_pat;

            std::vector<std::string> patterns;
            patterns.reserve(num_of_pat);

            for (int i = 0; i < num_of_pat; ++i) {
                const std::string pat = ReadString(in);
                patterns.push_back(pat);
            }

            size_t cpu_time = 0;
            PatternMatchingCPU cpu(patterns);
            auto cpu_result = cpu.GetCounts(text, cpu_time);

            size_t gpu_time = 0;
            PatternMatchingGPU gpu(patterns);
            auto gpu_result = gpu.Match(text, gpu_time);

            assert(cpu_result.size() == gpu_result.size());

            bool res = true;
            for (size_t i = 0; i < cpu_result.size(); ++i) {
                if (cpu_result[i] != gpu_result[i]) {
                    std::cerr<<"Wrong answer in test: "<<filename<<std::endl;
                    for (int j = 0; j < cpu_result.size(); ++j)
                        std::cout << j << " cpu: " << cpu_result[j] << " | " << "gpu: " << gpu_result[j] << std::endl;

                    res = false;
                    i = cpu_result.size();
                    std::cout<<"\n";

                }
            }

            if (res) {
                std::cout << "-------------Test: " << filename << " ----------\n";
                std::cout << "size of test: " << text.size() << std::endl;
                std::cout << "CPU time: " << cpu_time << std::endl;
                std::cout << "GPU time: " << gpu_time << "\n" << std::endl;
            }

            in.close();
        }
    } catch (std::exception& e) {
        std::cerr<<e.what()<<std::endl;
        exit(1);
    }


    //TestGenInfo info1 = {"tests//test1.txt", 2000, 'a', 'd', {4,5,7,8}, 'a', 'd'};
    //TestGenInfo info2 = {"tests//test2.txt", 1000, 'a', 'b', {3,7,8,9,10}, 'a', 'b'};
    //TestGenInfo info3 = {"tests//test100000.txt", 100000, 'a', 'e', {3,4,5,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8},'a', 'e'};
    //std::vector<size_t> sz(2000, 6);
    //TestGenInfo info4 = {"tests//testbig2.txt", 2000000, 'a', 't', sz, 'a', 't'};
    //TestGenerator({info3});

    return 0;
}

std::vector<std::string> GetAllTestFileNames(const std::string& dirname) {

    std::vector<std::string> filenames;

    std::filesystem::directory_iterator p(dirname);

    for (const auto & it : p) {
        if (!std::filesystem::is_regular_file(it.status()))
            continue;

        filenames.push_back(it.path().string());
    }
    return filenames;
}

void TestGenerator(const std::vector<TestGenInfo>& files) {

    for (const auto& file : files) {
        unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
        std::default_random_engine eng(seed);
        std::uniform_int_distribution<int> distr1(file.min_value, file.max_value);
        std::uniform_int_distribution<int> distr2(file.min_p, file.max_p);

        std::ofstream ostr(file.name);
        if (!ostr.is_open()) {
            std::cerr << "Cant open input file!" << std::endl;
            exit(0);
        }
        ostr<<file.size<<" "<<std::endl;
        for (int i = 0; i < file.size; ++i) {
            char c = distr1(eng);
            ostr<<c;
        }
        ostr<<"\n"<<file.size_of_pat.size()<<"\n";

        for (auto&& s : file.size_of_pat) {
            ostr<<s<<" ";
            for (int i = 0; i < s; ++i) {
                char c = distr2(eng);
                ostr<<c;
            }
            ostr<<"\n";
        }

        ostr.close();
    }
};