// tr_dsa.h -- Metal renderer state caching helpers mirroring GL DSA intent
#pragma once

#include <Metal/Metal.hpp>
#include <array>
#include <cstddef>

class MetalStateCache {
public:
    static MetalStateCache& Instance();

    void resetEncoder(MTL::RenderCommandEncoder* encoder);
    void invalidate();

    void bindPipeline(MTL::RenderCommandEncoder* encoder, MTL::RenderPipelineState* pipeline);
    void bindFragmentTexture(MTL::RenderCommandEncoder* encoder, std::size_t slot, MTL::Texture* texture);
    void bindFragmentSampler(MTL::RenderCommandEncoder* encoder, std::size_t slot, MTL::SamplerState* sampler);

private:
    static constexpr std::size_t kMaxFragmentSlots = 32;

    MTL::RenderCommandEncoder* encoder_ = nullptr;
    MTL::RenderPipelineState* pipeline_ = nullptr;
    std::array<MTL::Texture*, kMaxFragmentSlots> fragmentTextures_{};
    std::array<MTL::SamplerState*, kMaxFragmentSlots> fragmentSamplers_{};

    MetalStateCache() = default;
    void ensureEncoder(MTL::RenderCommandEncoder* encoder);
};
