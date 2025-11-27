#pragma once

#include <Metal/Metal.hpp>

#include "tr_capabilities.h"

extern "C" {
#include "../renderercommon/tr_public.h"
}

void Metal_InitExtensions(MTL::Device* device, glconfig_t* config, refimport_t* ri);
