/*
===========================================================================
Copyright (C) 2025 Metal Renderer Implementation

Texture loading and management for 2D/3D rendering
===========================================================================
*/

#ifndef TR_TEXTURE_H
#define TR_TEXTURE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"
#ifdef __cplusplus
}
#endif
#include "tr_utils.h"

#include <Metal/Metal.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class TextureManager {
public:
	explicit TextureManager(MTL::Device* device, refimport_t* rimp);
	~TextureManager() = default;

	// Register a texture/shader by name (main API)
	qhandle_t registerShader(const char* name, bool mipmap);
	
	// Get texture for rendering
	MTL::Texture* getTexture(qhandle_t handle) const;
	
	// Clear all textures
	void clear();

private:
	struct Texture {
		MetalPtr<MTL::Texture> texture;
		std::string name;
		int width = 0;
		int height = 0;
	};

	// Load image from file using renderercommon loaders
	qhandle_t loadImageFile(const char* name, bool mipmap);
	
	// Create Metal texture from pixel data
	MTL::Texture* createTexture(const byte* data, int width, int height, bool mipmap);

	MTL::Device* device_;
	refimport_t* ri_;  // Store pointer to engine imports
	std::vector<Texture> textures_;
	std::unordered_map<std::string, qhandle_t> nameToHandle_;
	
	// Default white texture (handle 0)
	static constexpr qhandle_t DEFAULT_TEXTURE_HANDLE = 0;
};

#endif // TR_TEXTURE_H
