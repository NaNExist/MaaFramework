#include "MinicapBase.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"

#include <array>
#include <format>

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapBase::parse(const json::value& config)
{
    return binary_->parse(config) && library_->parse(config);
}

// x86_64的prebuilt里面的library是32位的, 用不了
// arm64-v8会卡住, 不知道原因
static constexpr std::array<std::string_view, 3> kArchList = { /* "x86_64", */ "x86", /* "arm64-v8a", */ "armeabi-v7a",
                                                               "armeabi" };
static constexpr std::array<int, 16> kSdkList = { 31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14 };

bool MinicapBase::init(int w, int h, std::function<std::string(const std::string&)> path_of_bin,
                       std::function<std::string(const std::string&, int)> path_of_lib, const std::string& force_temp)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    if (!binary_->init(force_temp) || !library_->init("minicap.so")) {
        return false;
    }

    auto archs = binary_->abilist();
    auto sdk = binary_->sdk();

    if (!archs || !sdk) {
        return false;
    }

    auto arch_iter = ranges::find_first_of(*archs, kArchList);
    if (arch_iter == archs->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;

    auto sdk_iter = ranges::find_if(kSdkList, [sdk](int s) { return s <= sdk.value(); });
    if (sdk_iter == kSdkList.end()) {
        return false;
    }
    int fit_sdk = *sdk_iter;

    auto bin = path_of_bin(target_arch);
    auto lib = path_of_lib(target_arch, fit_sdk);

    if (!binary_->push(bin) || !library_->push(lib)) {
        return false;
    }

    if (!binary_->chmod() || !library_->chmod()) {
        return false;
    }

    set_wh(w, h);
    return true;
}

MAA_CTRL_UNIT_NS_END