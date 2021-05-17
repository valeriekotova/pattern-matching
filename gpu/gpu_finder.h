#pragma once

#include "Matrix/Matrix.h"

#ifdef __APPLE__
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include <sstream>
#include <fstream>

class PatternMatchingGPU final {

private:

    cl::Platform platform_;
    cl::Context context_;
    cl::Device device_;
    cl::CommandQueue queue_;
    cl::Program program_;

    const std::string kernel_name_;

private:

    const std::vector<std::string> patterns_;

    linal::Matrix<std::vector<size_t>> Pattern_table = linal::Matrix<std::vector<size_t>>(256,256);
    size_t maxdepth = 0;

    struct SignatureTables final {
        static constexpr unsigned n_ = 256; // number of rows and columns in matrix

        std::vector<linal::Matrix<cl_float4>> tables_;

        const cl_float4* GetData(size_t i) const {return tables_[i].data();};

    } signatures_;

private:

    void ChoosePlatformAndDevice(); //choose by user in console
    void ChooseDefaultPlatformAndDevice(); //choose first suited platform and device

    void BuildPatternTable();
    void BuildSignatureTables();

    std::vector<size_t> FindSmallPatterns(const std::string& text) const;

public:

    explicit PatternMatchingGPU(const std::vector<std::string>& patterns, const std::string& kernel_name = "match.cl");

    std::vector<size_t> Match(const std::string& text, size_t& time) const;
    void CheckAnswers(const std::string& text, const std::vector<cl_float2>& answers, size_t step, std::vector<size_t>& res) const;

};