/*
===========================================================================
Copyright (C) 2025 Metal Renderer Implementation

Metal renderer utilities and RAII wrappers
===========================================================================
*/

#ifndef TR_METAL_UTILS_H
#define TR_METAL_UTILS_H

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

#endif // TR_METAL_UTILS_H
