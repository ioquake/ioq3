// tr_dsa.cpp -- Metal renderer direct state access style cache

#include "tr_dsa.h"

namespace {
constexpr std::size_t ClampSlot(std::size_t slot, std::size_t maxSlots) {
    return slot < maxSlots ? slot : maxSlots - 1;
}
}

MetalStateCache& MetalStateCache::Instance() {
    static MetalStateCache cache;
    return cache;
}

void MetalStateCache::ensureEncoder(MTL::RenderCommandEncoder* encoder) {
    if (encoder_ == encoder) {
        return;
    }

    encoder_ = encoder;
    invalidate();
}

void MetalStateCache::resetEncoder(MTL::RenderCommandEncoder* encoder) {
    encoder_ = encoder;
    invalidate();
}

void MetalStateCache::invalidate() {
    pipeline_ = nullptr;
    fragmentTextures_.fill(nullptr);
    fragmentSamplers_.fill(nullptr);
}

void MetalStateCache::bindPipeline(MTL::RenderCommandEncoder* encoder, MTL::RenderPipelineState* pipeline) {
    ensureEncoder(encoder);
    if (!encoder_) {
        pipeline_ = pipeline;
        return;
    }

    if (pipeline_ == pipeline) {
        return;
    }

    if (!pipeline) {
        pipeline_ = nullptr;
        return;
    }

    encoder_->setRenderPipelineState(pipeline);
    pipeline_ = pipeline;
}

void MetalStateCache::bindFragmentTexture(MTL::RenderCommandEncoder* encoder, std::size_t slot, MTL::Texture* texture) {
    ensureEncoder(encoder);
    if (!encoder_) {
        return;
    }

    const std::size_t idx = ClampSlot(slot, kMaxFragmentSlots);
    if (fragmentTextures_[idx] == texture) {
        return;
    }

    encoder_->setFragmentTexture(texture, idx);
    fragmentTextures_[idx] = texture;
}

void MetalStateCache::bindFragmentSampler(MTL::RenderCommandEncoder* encoder, std::size_t slot, MTL::SamplerState* sampler) {
    ensureEncoder(encoder);
    if (!encoder_) {
        return;
    }

    const std::size_t idx = ClampSlot(slot, kMaxFragmentSlots);
    if (fragmentSamplers_[idx] == sampler) {
        return;
    }

    encoder_->setFragmentSamplerState(sampler, idx);
    fragmentSamplers_[idx] = sampler;
}
