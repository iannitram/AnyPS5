#include "prx/libSceAgcDriver/Execution/include/VulkanDevice.hpp"
#include "prx/libSceAgcDriver/Graphics/include/Draw.hpp"
#include "Recompiler.hpp"
#include "VulkanTestDevice.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

using AgcDriver::Graphics::Require;
using ShaderRecompiler::ShaderStage;

constexpr std::uint32_t Threads = 32;
constexpr std::uint32_t Inputs = 4;
constexpr std::uint32_t Results = 16;
alignas(256) std::array<std::uint32_t, Threads * Inputs> Input{};
alignas(256) std::array<std::uint32_t, Threads * Results> Output{};

alignas(256) constexpr auto Code = [] {
    std::array<std::uint32_t, 48> code{};
    std::uint32_t cursor = 0;
    for (const auto word : {0x34020084u, 0x34060086u, 0xe0381000u, 0x80000401u, 0xbf8c3f70u}) {
        code[cursor++] = word;
    }
    constexpr std::array<std::uint32_t, 8> selectors{0u, 4u, 8u, 15u, 1u, 2u, 4u, 8u};
    for (std::uint32_t variant = 0; variant < selectors.size(); ++variant) {
        for (std::uint32_t sign = 0; sign < 2u; ++sign) {
            code[cursor++] = (sign != 0u ? 0xd7750000u : 0xd7730000u) | (selectors[variant] << 11u) |
                             (variant == 7u ? 0x8000u : 0u) | (10u + variant * 2u + sign);
            code[cursor++] = variant == 6u ? 0x03fe0b04u : 0x041a0b04u;
            if (variant == 6u) code[cursor++] = 0x12345678u;
        }
    }
    for (const auto word : {0xe0781000u, 0x80010a03u, 0xe0781010u, 0x80010e03u,
                            0xe0781020u, 0x80011203u, 0xe0781030u, 0x80011603u, 0xbf810000u}) {
        code[cursor++] = word;
    }
    return code;
}();

constexpr std::uint32_t Rows[5][4] = {
    {0x00030002u, 0x00050007u, 0x80000001u, 0x00000000u},
    {0xffff0002u, 0x0003fffeu, 0x7fff8000u, 0x00000000u},
    {0x00020002u, 0x00020002u, 0xfffe0005u, 0x00000000u},
    {0xffffffffu, 0xffffffffu, 0xffffffffu, 0x00000000u},
    {0x7fff7fffu, 0x7fff7fffu, 0x7fffffffu, 0x00000000u},
};
constexpr std::uint32_t Expected[5][16] = {
    {0x8000000fu, 0x8000000fu, 0x0000800eu, 0x0000800eu, 0x000f0000u, 0x000f0000u, 0x800f0000u, 0x800f0000u, 0x80000016u, 0x80000016u, 0x8000000bu, 0x8000000bu, 0x00001242u, 0x00001242u, 0x000f0000u, 0x000f0000u},
    {0x80017ffcu, 0x7fff7ffcu, 0x00027ffbu, 0x00007ffbu, 0x7ffc0000u, 0x7ffc0000u, 0x7ffc0000u, 0x7ffc0000u, 0x7ffc8002u, 0x7fff8002u, 0x7fff8006u, 0x7fff8006u, 0x00021230u, 0x00001230u, 0x7ffc0000u, 0x7ffc0000u},
    {0xfffe0009u, 0xfffe0009u, 0x00010002u, 0x00010002u, 0x00090000u, 0x00090000u, 0x00020000u, 0x00020000u, 0xfffe0009u, 0xfffe0009u, 0xfffe0009u, 0xfffe0009u, 0x00001238u, 0x00001238u, 0x00090000u, 0x00090000u},
    {0xfffe0000u, 0x00000000u, 0xffff0000u, 0x00010000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xfffe0000u, 0x00000000u, 0xfffe0000u, 0x00000000u, 0xfffe1235u, 0x00001235u, 0xffff0000u, 0x00000000u},
    {0xbfff0000u, 0xbfff0000u, 0x3fff8000u, 0x3fff8000u, 0x00000000u, 0x00000000u, 0x80000000u, 0x80000000u, 0xbfff0000u, 0xbfff0000u, 0xbfff0000u, 0xbfff0000u, 0x3fff1235u, 0x3fff1235u, 0x00000000u, 0xffff0000u},
};
constexpr const char* Names[16] = {
    "v_mad_u32_u16 op_sel:[0,0,0,0]",
    "v_mad_i32_i16 op_sel:[0,0,0,0]",
    "v_mad_u32_u16 op_sel:[0,0,1,0]",
    "v_mad_i32_i16 op_sel:[0,0,1,0]",
    "v_mad_u32_u16 op_sel:[0,0,0,1]",
    "v_mad_i32_i16 op_sel:[0,0,0,1]",
    "v_mad_u32_u16 op_sel:[1,1,1,1]",
    "v_mad_i32_i16 op_sel:[1,1,1,1]",
    "v_mad_u32_u16 op_sel:[1,0,0,0]",
    "v_mad_i32_i16 op_sel:[1,0,0,0]",
    "v_mad_u32_u16 op_sel:[0,1,0,0]",
    "v_mad_i32_i16 op_sel:[0,1,0,0]",
    "v_mad_u32_u16 literal 0x12345678 op_sel:[0,0,1,0]",
    "v_mad_i32_i16 literal 0x12345678 op_sel:[0,0,1,0]",
    "v_mad_u32_u16 op_sel:[0,0,0,1] clamp",
    "v_mad_i32_i16 op_sel:[0,0,0,1] clamp",
};

void Fill(std::uint32_t tid, std::uint32_t* words) {
    const auto& row = Rows[tid % std::size(Rows)];
    std::copy(std::begin(row), std::end(row), words);
}

std::array<std::uint32_t, 4> BufferDescriptor(const void* data, std::uint32_t bytes) {
    const auto address = reinterpret_cast<std::uintptr_t>(data);
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>((address >> 32u) & 0xffffu), bytes, 0x31016facu};
}

std::string Hex(std::uint32_t value) {
    char text[16];
    std::snprintf(text, sizeof(text), "0x%08x", value);
    return text;
}

void Expect(std::uint32_t tid, std::uint32_t actual, std::uint32_t expected, const char* name) {
    Require(actual == expected, std::string("vop3 wide integer16 mad op sel: lane ") + std::to_string(tid) + " " + name + " is " + Hex(actual) + ", expected " + Hex(expected));
}

void Run(AgcDriver::VulkanDevice& device) {
    for (std::uint32_t tid = 0; tid < Threads; ++tid) Fill(tid, &Input[tid * Inputs]);
    Output.fill(0xdeadbeefu);
    std::vector<std::uint32_t> userData(8, 0u);
    const auto input = BufferDescriptor(Input.data(), static_cast<std::uint32_t>(Input.size() * 4u));
    const auto output = BufferDescriptor(Output.data(), static_cast<std::uint32_t>(Output.size() * 4u));
    std::copy(input.begin(), input.end(), userData.begin());
    std::copy(output.begin(), output.end(), userData.begin() + 4);
    const std::span<const std::uint32_t> code(Code);
    const std::array<ShaderRecompiler::MemoryRegion, 1> memory{{{reinterpret_cast<std::uintptr_t>(code.data()), std::as_bytes(code)}}};
    const ShaderRecompiler::ShaderComputeStageInfo compute{{Threads, 1, 1}, 0u, {false, false, false}, false, 1};
    ShaderRecompiler::RecompileRequest request{
        {ShaderStage::Compute, reinterpret_cast<std::uintptr_t>(code.data()), code, 0, {}},
        {32, 0, userData, compute, std::nullopt, std::nullopt, memory},
        device.Target(),
        {0, 0, 0, 128}
    };
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    device.Dispatch(result, 1, 1, 1, {}, reinterpret_cast<std::uintptr_t>(code.data()));
    device.WaitIdle();
}

void Check() {
    for (std::uint32_t tid = 0; tid < Threads; ++tid) {
        const std::uint32_t* out = &Output[tid * Results];
        for (std::uint32_t i = 0; i < 16; ++i) {
            Expect(tid, out[i], Expected[tid % std::size(Rows)][i], Names[i]);
        }
    }
}

}

int main() {
    try {
        const auto device = OpenVulkanTestDevice();
        if (!device) return VulkanTestSkipped;
        Run(*device);
        Check();
        std::puts("vop3 wide integer16 mad op sel tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
