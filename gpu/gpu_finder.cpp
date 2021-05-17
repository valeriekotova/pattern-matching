#include "gpu_finder.h"

PatternMatchingGPU::PatternMatchingGPU(const std::vector<std::string>& patterns, const std::string &kernel_name):
    patterns_(patterns), kernel_name_(kernel_name) {

    // ChoosePlatformAndDevice();
    ChooseDefaultPlatformAndDevice();

    context_ = cl::Context({device_});
    queue_ = cl::CommandQueue(context_, device_);

    std::ifstream program_sources(kernel_name_);
    if (!program_sources.is_open())
        throw std::runtime_error("Can't open file: " + kernel_name);

    std::ostringstream ostr;
    ostr << program_sources.rdbuf();
    program_sources.close();
    std::string program_string(ostr.str());

    program_ = cl::Program(context_, program_string);
    program_.build();

    BuildPatternTable();
    BuildSignatureTables();
}

std::vector<size_t> PatternMatchingGPU::Match(const std::string& text, size_t& time) const {

    auto res = FindSmallPatterns(text);

    cl::Buffer text_buffer(context_, CL_MEM_READ_ONLY, text.size() * sizeof(std::char_traits<char>));
    queue_.enqueueWriteBuffer(text_buffer, CL_TRUE, 0, text.size() * sizeof(std::char_traits<char>), text.data());

    std::vector<cl::Event> events(maxdepth);
    const cl::NDRange global_size(text.size() / 2 + text.size() % 2);

    std::vector<cl::Buffer> answer_buffers(maxdepth);
    std::transform(answer_buffers.begin()
                   , answer_buffers.end()
                   , answer_buffers.begin()
                   , [context = context_, size = text.size()](auto& buf) {
        return cl::Buffer(context, CL_MEM_WRITE_ONLY, size * sizeof(cl_float2));
    });

    const size_t matrix_size = SignatureTables::n_;
    cl::Buffer table_buf(context_, CL_MEM_READ_ONLY, matrix_size * matrix_size * sizeof(cl_float4));

    cl::Kernel kernel_(program_, "signature_match");
    kernel_.setArg(0, text_buffer);
    kernel_.setArg(1, static_cast<cl_uint>(text.size()));

    auto start_time = std::chrono::system_clock::now();

    for(std::size_t i = 0; i < maxdepth; ++i) {

        queue_.enqueueWriteBuffer(table_buf, CL_TRUE, 0, matrix_size * matrix_size * sizeof(cl_float4), signatures_.GetData(i));

        kernel_.setArg(2, answer_buffers[i]);
        kernel_.setArg(3, table_buf);

        queue_.enqueueNDRangeKernel(kernel_,  cl::NDRange(0), global_size, cl::NullRange, nullptr, &events.at(i));
    }

    // answer[i] shows i, j - first two symbols of possible pattern, which can start from text[i]
    std::vector<cl_float2> answers(text.size());

    for(std::size_t step = 0; step < maxdepth; ++step) {

        events[step].wait();
        queue_.enqueueReadBuffer(answer_buffers[step], CL_TRUE, 0, answers.size() * sizeof(cl_float2), answers.data());

        CheckAnswers(text, answers, step, res);
    }
    auto finish_time = std::chrono::system_clock::now();
    time = (finish_time - start_time).count();

    return res;
}

void PatternMatchingGPU::CheckAnswers
    (const std::string& text, const std::vector<cl_float2>& answers, size_t step, std::vector<size_t>& res) const {
    for (size_t n = 0; n < text.size() - 2; ++n) {
        const cl_float2 coordinates = answers[n];

        //const auto i = static_cast<u_char>(static_cast<char>(coordinates.s[0]));
        //const auto j = static_cast<u_char>(static_cast<char>(coordinates.s[1]));

        const auto i = static_cast<u_char>(static_cast<char>(coordinates.s[0]));
        const auto j = static_cast<u_char>(static_cast<char>(coordinates.s[1]));

        if(i && j) {

            const std::size_t pattern_idx = Pattern_table.at(i,j)[step];
            const auto& pat = patterns_[pattern_idx];

            int k = 6;
            for (; k < pat.size() && pat[k] == text[n + k]; ++k);

            if (k == pat.size())
                ++res[pattern_idx];
        }

    }
}


std::vector<size_t> PatternMatchingGPU::FindSmallPatterns(const std::string& text) const {

    std::vector<size_t> res(patterns_.size());

    for (size_t i = 0; i < patterns_.size(); ++i) {
        auto&& pat = patterns_.at(i);
        if (pat.size() < 6) {

            auto pos = text.find(pat);
            while (pos != std::string::npos) {
                ++res[i];
                pos = text.find(pat, pos + 1);
            }
        }
    }

    return res;
}


void PatternMatchingGPU::BuildPatternTable() {

    int num = 0;
    for (const auto & p : patterns_)
    {
        if (p.size() > 5) {
            auto i = static_cast<u_char>(p[0]);
            auto j = static_cast<u_char>(p[1]);
            Pattern_table.at(i, j).push_back(num);
            maxdepth = std::max(maxdepth, Pattern_table.at(i,j).size());
        }
        ++num;
    }

    if (!maxdepth)
        throw std::invalid_argument("Count of patterns = 0");
}

void PatternMatchingGPU::BuildSignatureTables() {

    std::vector<linal::Matrix<cl_float4>> tables(maxdepth);
    for (int k = 0; k < maxdepth; ++k)
        tables[k].resize(SignatureTables::n_, SignatureTables::n_);

    for (size_t i = 0; i < SignatureTables::n_; ++i) {
        for (size_t j = 0; j < SignatureTables::n_; ++j) {
            auto&& patterns = Pattern_table.at(i, j);

            for (int k = 0; k < maxdepth; ++k) {

                if (patterns.size() > k) {

                    const size_t n = patterns[k];
                    auto&& pat = patterns_[n];

                    if (pat.size() > 5) {
                        tables[k].at(i, j).x = pat.at(2);
                        tables[k].at(i, j).y = pat.at(3);
                        tables[k].at(i, j).z = pat.at(4);
                        tables[k].at(i, j).w = pat.at(5);
                    }
                }
            }
        }
    }

    signatures_.tables_ = tables;
}


/*void PatternMatchingGPU::BuildSignatureTables() {

    Signature_tables.resize(maxdepth);

    for (size_t i = 0; i < SignatureTable::n_; ++i) {
        for (size_t j = 0; j < SignatureTable::n_; ++j) {
            auto&& patterns = Pattern_table.at(i, j);

            for (int k = 0; k < maxdepth; ++k) {

                if (patterns.size() > k) {

                    const size_t n = patterns[k];
                    auto&& pat = patterns_[n];

                    if (pat.size() > 5) {
                        Signature_tables[k].at(i, j).x = pat.at(2);
                        Signature_tables[k].at(i, j).y = pat.at(3);
                        Signature_tables[k].at(i, j).z = pat.at(4);
                        Signature_tables[k].at(i, j).w = pat.at(5);
                    }
                }
            }
        }
    }

}*/



void PatternMatchingGPU::ChoosePlatformAndDevice() {

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty())
        throw std::invalid_argument("No platforms found");

    int size = platforms.size();

    //Get all devices for each platform
    std::vector<std::vector<cl::Device>> all_devices(size);
    for (int i = 0; i < size; ++i)
        platforms[i].getDevices(CL_DEVICE_TYPE_GPU, &all_devices[i]);

    //if only one platform available
    if (size == 1) {
        std::cout << "You have one platform: " << platforms[0].getInfo<CL_PLATFORM_NAME>() << std::endl;

        if (all_devices[0].empty())
            throw std::invalid_argument("No devices found!");

        platform_ = platforms[0];

        int devices_count = all_devices[0].size();
        if (devices_count == 1) {
            std::cout << "You have one device: " << all_devices[0][0].getInfo<CL_DEVICE_NAME>() << std::endl;
            device_ = all_devices[0][0];
            return;
        }

        std::cout << "You have " << devices_count << " devices available.\nChoose one (write number)" << std::endl;
        for (int i = 0; i < devices_count; ++i)
            std::cout << "[" << i << "]: " << all_devices[0][i].getInfo<CL_DEVICE_NAME>() << std::endl;

        int number = 0;
        std::cin >> number;
        while (number < 0 && number > devices_count) {
            std::cout << "Wrong number. Try again!" << std::endl;
            std::cin >> number;
        }

        device_ = all_devices[0][number];

        return;
    }

    std::cout << "You have " << size << " platforms available.\nChoose one (write number)" << std::endl;
    for (int i = 0; i < size; ++i) {
        std::cout << "[" << i << "]: " << platforms[i].getInfo<CL_PLATFORM_NAME>() << std::endl;
        std::cout << "\tDevices available on this platform: " << std::endl;
        for (int j = 0; j < all_devices[i].size(); ++j)
            std::cout << "\t[" << j << "]: " << all_devices[i][j].getInfo<CL_DEVICE_NAME>() << std::endl;

        std::cout << "-------------------------------------\n";
    }

    int number = 0;
    std::cin >> number;
    while (number < 0 && number > size) {
        std::cout << "Wrong number. Try again!" << std::endl;
        std::cin >> number;
    }
    platform_ = platforms[number];

    int devices_count = all_devices[number].size();
    if (!devices_count)
        throw std::invalid_argument("No devices found!");

    std::cout << "You have " << devices_count << " devices available.\nChoose one (write number)" << std::endl;
    for (int i = 0; i < devices_count; ++i)
        std::cout << "[" << i << "]: " << all_devices[number][i].getInfo<CL_DEVICE_NAME>() << std::endl;

    int N = 0;
    std::cin >> N;
    while (N < 0 && N > devices_count) {
        std::cout << "Wrong number. Try again!" << std::endl;
        std::cin >> N;
    }

    device_ = all_devices[number][N];
}

void PatternMatchingGPU::ChooseDefaultPlatformAndDevice() {

    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty())
        throw std::invalid_argument("No platforms found");


    for (auto &platform : platforms) {

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (!devices.empty()) {
            platform_ = platform;
            device_ = devices[0];
            return;
        }
    }

    throw std::invalid_argument("No devices found");
}