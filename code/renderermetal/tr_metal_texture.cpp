/*
===========================================================================
Copyright (C) 2025 Metal Renderer Implementation

Texture loading and management implementation
===========================================================================
*/

#include "tr_metal_texture.h"

extern "C" {
	#include "../renderercommon/tr_common.h"
}


#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>

namespace {
int CaseInsensitiveCompare(const char* lhs, const char* rhs) {
	if (lhs == rhs) {
		return 0;
	}
	if (!lhs) {
		return -1;
	}
	if (!rhs) {
		return 1;
	}

	while (*lhs && *rhs) {
		int diff = std::tolower(static_cast<unsigned char>(*lhs)) -
		           std::tolower(static_cast<unsigned char>(*rhs));
		if (diff != 0) {
			return diff;
		}
		++lhs;
		++rhs;
	}

	return std::tolower(static_cast<unsigned char>(*lhs)) -
	       std::tolower(static_cast<unsigned char>(*rhs));
}

struct ShaderRemap {
	const char* shaderName;
	const char* fallbackTexture;
};

constexpr ShaderRemap kShaderRemaps[] = {
	{"menuback", "textures/sfx/logo512"},
	{"menubacknologo", "gfx/colors/black"},
	{"menubackragepro", "textures/sfx/logo512"},
	{"console", "gfx/misc/console01"},
};

const char* RemapShaderName(const char* name) {
	if (!name || !name[0]) {
		return nullptr;
	}

	for (const auto& remap : kShaderRemaps) {
		if (CaseInsensitiveCompare(name, remap.shaderName) == 0) {
			return remap.fallbackTexture;
		}
	}

	return nullptr;
}
} // namespace

// Global refimport_t required by renderercommon image loaders
// This will be set by TextureManager constructor
refimport_t ri;

TextureManager::TextureManager(MTL::Device* device, refimport_t* rimp)
	: device_(device), ri_(rimp)
{
	// Set global ri for renderercommon image loaders
	if (rimp) {
		ri = *rimp;
	}
	
	// Reserve handle 0 for default white texture
	Texture defaultTex;
	defaultTex.name = "*default";
	defaultTex.width = 1;
	defaultTex.height = 1;
	
	// Create 1x1 white texture
	byte white[4] = {255, 255, 255, 255};
	defaultTex.texture.reset(createTexture(white, 1, 1, false));
	
	textures_.push_back(std::move(defaultTex));
}

qhandle_t TextureManager::registerShader(const char* name, bool mipmap) {
	if (!name || !name[0]) {
		return DEFAULT_TEXTURE_HANDLE;
	}

	// Handle default white texture explicitly
	if (strcmp(name, "white") == 0 || strcmp(name, "*white") == 0) {
		return DEFAULT_TEXTURE_HANDLE;
	}

	// Check cache
	auto it = nameToHandle_.find(name);
	if (it != nameToHandle_.end()) {
		return it->second;
	}

	// Remap known multi-stage shaders to a representative texture so menus work
	const char* loadName = name;
	if (const char* remapped = RemapShaderName(name)) {
		loadName = remapped;
	}
	// Load new texture
	return loadImageFile(loadName, mipmap);
}

MTL::Texture* TextureManager::getTexture(qhandle_t handle) const {
	if (handle < 0 || static_cast<size_t>(handle) >= textures_.size()) {
		return textures_[DEFAULT_TEXTURE_HANDLE].texture.get();
	}
	return textures_[handle].texture.get();
}

void TextureManager::clear() {
	textures_.clear();
	nameToHandle_.clear();
	
	// Recreate default texture
	Texture defaultTex;
	defaultTex.name = "*default";
	defaultTex.width = 1;
	defaultTex.height = 1;
	byte white[4] = {255, 255, 255, 255};
	defaultTex.texture.reset(createTexture(white, 1, 1, false));
	textures_.push_back(std::move(defaultTex));
}

qhandle_t TextureManager::loadImageFile(const char* name, bool mipmap) {
	// Try different extensions
	static const char* extensions[] = {"", ".tga", ".jpg", ".png", ".jpeg"};
	
	byte* pic = nullptr;
	int width = 0;
	int height = 0;
	
	for (const char* ext : extensions) {
		char fullName[MAX_QPATH];
		std::snprintf(fullName, sizeof(fullName), "%s%s", name, ext);
		
		// Dispatch to correct loader based on extension
		// Note: If ext is empty, we don't know the type, so we might skip or try all?
		// But usually 'name' doesn't have extension if we are here.
		// If 'name' has extension and ext is empty, we should check name's extension.
		
		if (ext[0] == '\0') {
			// If trying without extension, check if name has one
			const char* existingExt = strrchr(name, '.');
			if (existingExt) {
				if (strcasecmp(existingExt, ".tga") == 0) R_LoadTGA(fullName, &pic, &width, &height);
				else if (strcasecmp(existingExt, ".jpg") == 0 || strcasecmp(existingExt, ".jpeg") == 0) R_LoadJPG(fullName, &pic, &width, &height);
				else if (strcasecmp(existingExt, ".png") == 0) R_LoadPNG(fullName, &pic, &width, &height);
			}
		} else {
			if (strcasecmp(ext, ".tga") == 0) {
				R_LoadTGA(fullName, &pic, &width, &height);
			} else if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
				R_LoadJPG(fullName, &pic, &width, &height);
			} else if (strcasecmp(ext, ".png") == 0) {
				R_LoadPNG(fullName, &pic, &width, &height);
			}
		}
		
		if (pic) break;
	}
	
	if (!pic || width <= 0 || height <= 0) {
		if (pic) {
			ri_->Free(pic);
		}
		ri_->Printf(PRINT_WARNING, "TextureManager: Failed to load '%s'\n", name);
		return DEFAULT_TEXTURE_HANDLE;
	}

	// Create Metal texture
	MTL::Texture* metalTexture = createTexture(pic, width, height, mipmap);
	ri_->Free(pic);
	
	if (!metalTexture) {
		ri_->Printf(PRINT_WARNING, "TextureManager: Failed to create Metal texture for '%s'\n", name);
		return DEFAULT_TEXTURE_HANDLE;
	}

	// Store texture
	Texture tex;
	tex.texture.reset(metalTexture);
	tex.name = name;
	tex.width = width;
	tex.height = height;
	
	qhandle_t handle = static_cast<qhandle_t>(textures_.size());
	textures_.push_back(std::move(tex));
	nameToHandle_[name] = handle;
	
	ri_->Printf(PRINT_DEVELOPER, "TextureManager: Loaded '%s' (%dx%d, handle %d)\n",
	          name, width, height, handle);
	
	return handle;
}

MTL::Texture* TextureManager::createTexture(const byte* data, int width, int height, bool mipmap) {
	if (!device_ || !data || width <= 0 || height <= 0) {
		return nullptr;
	}

	// Create texture descriptor
	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::texture2DDescriptor(
		MTL::PixelFormatRGBA8Unorm, width, height, mipmap);
	
	desc->setUsage(MTL::TextureUsageShaderRead);
	desc->setStorageMode(MTL::StorageModeShared);  // Shared for fast CPU→GPU upload
	
	MTL::Texture* texture = device_->newTexture(desc);
	desc->release();
	
	if (!texture) {
		return nullptr;
	}

	// Upload pixel data
	MTL::Region region = MTL::Region::Make2D(0, 0, width, height);
	texture->replaceRegion(region, 0, data, width * 4);
	
	// Generate mipmaps if requested
	if (mipmap && texture->mipmapLevelCount() > 1) {
		MTL::CommandBuffer* cmdBuf = device_->newCommandQueue()->commandBuffer();
		MTL::BlitCommandEncoder* blitEncoder = cmdBuf->blitCommandEncoder();
		blitEncoder->generateMipmaps(texture);
		blitEncoder->endEncoding();
		cmdBuf->commit();
		cmdBuf->waitUntilCompleted();
		
		blitEncoder->release();
		cmdBuf->release();
	}
	
	return texture;
}
