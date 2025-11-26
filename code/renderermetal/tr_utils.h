/*
===========================================================================
Copyright (C) 2025 Metal Renderer Implementation

Metal renderer utilities and RAII wrappers
===========================================================================
*/

#ifndef TR_UTILS_H
#define TR_UTILS_H

//=============================================================================
// Standard library helpers
//=============================================================================

#include <algorithm>
#include <cctype>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#ifdef __cplusplus
}
#endif

inline std::string MetalNormalizeShaderName(const char *rawName) {
	if (!rawName) {
		return std::string();
	}

	char strippedName[MAX_QPATH];
	COM_StripExtension(rawName, strippedName, sizeof(strippedName));

	std::string key(strippedName);
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) -> char {
		if (ch == '\\') {
			return '/';
		}
		return static_cast<char>(std::tolower(ch));
	});
	return key;
}

//=============================================================================
// RAII Smart Pointer for Metal Objects
//=============================================================================

template<typename T>
class MetalPtr {
	T* ptr_ = nullptr;

public:
	MetalPtr() = default;
	explicit MetalPtr(T* p) : ptr_(p) {}
	
	~MetalPtr() {
		if (ptr_) {
			ptr_->release();
		}
	}

	// Move-only semantics
	MetalPtr(MetalPtr&& other) noexcept : ptr_(other.ptr_) {
		other.ptr_ = nullptr;
	}

	MetalPtr& operator=(MetalPtr&& other) noexcept {
		if (this != &other) {
			reset();
			ptr_ = other.ptr_;
			other.ptr_ = nullptr;
		}
		return *this;
	}

	// Deleted copy
	MetalPtr(const MetalPtr&) = delete;
	MetalPtr& operator=(const MetalPtr&) = delete;

	T* get() const { return ptr_; }
	T* operator->() const { return ptr_; }
	T& operator*() const { return *ptr_; }
	explicit operator bool() const { return ptr_ != nullptr; }

	T* release() {
		T* tmp = ptr_;
		ptr_ = nullptr;
		return tmp;
	}

	void reset(T* p = nullptr) {
		if (ptr_) {
			ptr_->release();
		}
		ptr_ = p;
	}
};

#endif // TR_UTILS_H
