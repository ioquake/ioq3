/*
===========================================================================
Copyright (C) 2025 Metal Renderer Implementation

Texture loading and management implementation
===========================================================================
*/

#include "tr_texture.h"

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
// Defined in tr_backend.cpp, declared in tr_local.h
extern "C" refimport_t ri;

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

qhandle_t TextureManager::registerRawImage(const char* name, const byte* rgba, int width, int height, bool mipmap) {
	if (!name || !name[0] || !rgba || width <= 0 || height <= 0) {
		return DEFAULT_TEXTURE_HANDLE;
	}

	auto existing = nameToHandle_.find(name);
	if (existing != nameToHandle_.end()) {
		return existing->second;
	}

	MTL::Texture* metalTexture = createTexture(rgba, width, height, mipmap);
	if (!metalTexture) {
		if (ri_) {
			ri_->Printf(PRINT_WARNING, "TextureManager: Failed to create raw texture '%s'\n", name);
		}
		return DEFAULT_TEXTURE_HANDLE;
	}

	Texture tex;
	tex.texture.reset(metalTexture);
	tex.name = name;
	tex.width = width;
	tex.height = height;

	qhandle_t handle = static_cast<qhandle_t>(textures_.size());
	nameToHandle_[tex.name] = handle;
	textures_.push_back(std::move(tex));

	if (ri_) {
		ri_->Printf(PRINT_DEVELOPER, "TextureManager: Registered raw '%s' (%dx%d, handle %d)\n",
		            name, width, height, handle);
	}

	return handle;
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

void TextureManager::debugListTextures() const {
	if (!ri_) {
		return;
	}

	const size_t textureCount = textures_.size();
	size_t totalBytes = 0;
	ri_->Printf(PRINT_ALL, "\n-- Metal texture list (%zu entries) --\n", textureCount);

	for (size_t i = 0; i < textureCount; ++i) {
		const Texture& tex = textures_[i];
		const size_t bytes = static_cast<size_t>(tex.width) * static_cast<size_t>(tex.height) * 4u;
		totalBytes += bytes;
		ri_->Printf(PRINT_ALL, "%4zu: %4d x %4d  %7zu KB  %s\n",
		           i,
		           tex.width,
		           tex.height,
		           bytes / 1024u,
		           tex.name.c_str());
	}

	const double totalMB = static_cast<double>(totalBytes) / (1024.0 * 1024.0);
	ri_->Printf(PRINT_ALL, "Total approx %.2f MB\n", totalMB);
}

qhandle_t TextureManager::loadImageFile(const char* name, bool mipmap) {
	// -----------------------------------------------------------------------
	// DDS path — handled separately because it carries its own pixel format
	// and may contain multiple pre-built mip levels.
	// -----------------------------------------------------------------------
	{
		// Check whether 'name' already has a .dds extension, or try appending one.
		static const char* ddsExts[] = {"", ".dds"};
		for (const char* ext : ddsExts) {
			char fullName[MAX_QPATH];
			std::snprintf(fullName, sizeof(fullName), "%s%s", name, ext);

			const char* checkExt = (ext[0] == '\0') ? strrchr(name, '.') : ext;
			if (!checkExt || strcasecmp(checkExt, ".dds") != 0) {
				continue;
			}

			byte*            ddsPic    = nullptr;
			int              ddsW      = 0;
			int              ddsH      = 0;
			int              ddsNumMip = 1;
			MTL::PixelFormat ddsFmt    = MTL::PixelFormatRGBA8Unorm;

			R_LoadDDS_Metal(fullName, &ddsPic, &ddsW, &ddsH, &ddsFmt, &ddsNumMip);

			if (!ddsPic || ddsW <= 0 || ddsH <= 0) {
				if (ddsPic) ri_->Free(ddsPic);
				continue;
			}

			MTL::Texture* metalTexture =
			    createCompressedTexture(ddsPic, ddsW, ddsH, ddsNumMip, ddsFmt);
			ri_->Free(ddsPic);

			if (!metalTexture) {
				ri_->Printf(PRINT_WARNING,
				            "TextureManager: Failed to create Metal texture for DDS '%s'\n", name);
				continue;
			}

			Texture tex;
			tex.texture.reset(metalTexture);
			tex.name   = name;
			tex.width  = ddsW;
			tex.height = ddsH;

			qhandle_t handle = static_cast<qhandle_t>(textures_.size());
			textures_.push_back(std::move(tex));
			nameToHandle_[name] = handle;

			ri_->Printf(PRINT_DEVELOPER,
			            "TextureManager: Loaded DDS '%s' (%dx%d, %d mips, handle %d)\n",
			            name, ddsW, ddsH, ddsNumMip, handle);
			return handle;
		}
	}

	// -----------------------------------------------------------------------
	// Standard image formats (TGA / JPG / PNG)
	// -----------------------------------------------------------------------
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

// -------------------------------------------------------------------------
// Helpers for compressed texture upload
// -------------------------------------------------------------------------

namespace {

// Returns the number of bytes per compressed 4×4 block for a given Metal
// pixel format, or 0 for uncompressed formats (RGBA8, etc.).
static size_t blockSizeForFormat(MTL::PixelFormat fmt)
{
	switch (fmt) {
		case MTL::PixelFormatBC1_RGBA:
		case MTL::PixelFormatBC1_RGBA_sRGB:
		case MTL::PixelFormatBC4_RUnorm:
		case MTL::PixelFormatBC4_RSnorm:
			return 8;
		case MTL::PixelFormatBC2_RGBA:
		case MTL::PixelFormatBC2_RGBA_sRGB:
		case MTL::PixelFormatBC3_RGBA:
		case MTL::PixelFormatBC3_RGBA_sRGB:
		case MTL::PixelFormatBC5_RGUnorm:
		case MTL::PixelFormatBC5_RGSnorm:
		case MTL::PixelFormatBC6H_RGBUfloat:
		case MTL::PixelFormatBC6H_RGBFloat:
		case MTL::PixelFormatBC7_RGBAUnorm:
		case MTL::PixelFormatBC7_RGBAUnorm_sRGB:
			return 16;
		default:
			return 0; // uncompressed
	}
}

// Bytes per row for a single mip level row of the given pixel format.
static size_t bytesPerRowForMip(MTL::PixelFormat fmt, int mipWidth)
{
	size_t bs = blockSizeForFormat(fmt);
	if (bs > 0) {
		// Compressed: rows are measured in 4×4 blocks.
		size_t blocksWide = (static_cast<size_t>(mipWidth) + 3) / 4;
		return blocksWide * bs;
	}
	// Uncompressed RGBA8: 4 bytes per pixel.
	return static_cast<size_t>(mipWidth) * 4;
}

// Total byte size of a single mip level.
static size_t mipLevelByteSize(MTL::PixelFormat fmt, int mipWidth, int mipHeight)
{
	size_t bs = blockSizeForFormat(fmt);
	if (bs > 0) {
		size_t blocksWide = (static_cast<size_t>(mipWidth)  + 3) / 4;
		size_t blocksHigh = (static_cast<size_t>(mipHeight) + 3) / 4;
		return blocksWide * blocksHigh * bs;
	}
	return static_cast<size_t>(mipWidth) * static_cast<size_t>(mipHeight) * 4;
}

} // anonymous namespace

MTL::Texture* TextureManager::createCompressedTexture(const byte*      data,
                                                      int              width,
                                                      int              height,
                                                      int              numMips,
                                                      MTL::PixelFormat pixelFormat)
{
	if (!device_ || !data || width <= 0 || height <= 0 || numMips < 1) {
		return nullptr;
	}

	MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
	desc->setTextureType(MTL::TextureType2D);
	desc->setPixelFormat(pixelFormat);
	desc->setWidth(static_cast<NS::UInteger>(width));
	desc->setHeight(static_cast<NS::UInteger>(height));
	desc->setMipmapLevelCount(static_cast<NS::UInteger>(numMips));
	desc->setUsage(MTL::TextureUsageShaderRead);
	desc->setStorageMode(MTL::StorageModeShared);

	MTL::Texture* texture = device_->newTexture(desc);
	desc->release();

	if (!texture) {
		return nullptr;
	}

	const byte* src    = data;
	int         mipW   = width;
	int         mipH   = height;

	for (int mip = 0; mip < numMips; ++mip) {
		size_t rowBytes  = bytesPerRowForMip(pixelFormat, mipW);
		size_t mipBytes  = mipLevelByteSize(pixelFormat, mipW, mipH);

		MTL::Region region = MTL::Region::Make2D(0, 0,
		    static_cast<NS::UInteger>(mipW),
		    static_cast<NS::UInteger>(mipH));

		texture->replaceRegion(region,
		    static_cast<NS::UInteger>(mip),
		    src,
		    rowBytes);

		src  += mipBytes;
		mipW  = (mipW  > 1) ? mipW  / 2 : 1;
		mipH  = (mipH  > 1) ? mipH  / 2 : 1;
	}

	return texture;
}
