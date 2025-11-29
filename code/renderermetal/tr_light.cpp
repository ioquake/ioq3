/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.cpp - Dynamic lighting and entity lighting grid calculations
// Direct port from renderergl2/tr_light.c

#include "tr_local.h"
#include <cmath>
#include <cstring>

// Constants from GL2 renderer
#define DLIGHT_AT_RADIUS        16
// at the edge of a dlight's influence, this amount of light will be added

#define DLIGHT_MINIMUM_RADIUS   16
// never calculate a range less than this to prevent huge light numbers

// World lighting data
namespace {
    struct WorldLightingData {
        vec3_t lightGridOrigin{};
        vec3_t lightGridSize{};
        vec3_t lightGridInverseSize{};
        int lightGridBounds[3]{};
        byte* lightGridData = nullptr;
        uint16_t* lightGrid16 = nullptr;
        size_t lightGridDataSize = 0;

        // Sun direction for fallback lighting
        vec3_t sunDirection{};
        float identityLight = 1.0f;

        // Sine table for decoding light directions
        // Matches tr.sinTable in OpenGL renderer
        float sinTable[FUNCTABLE_SIZE]{};

        bool isValid() const {
            return lightGridData != nullptr &&
                   lightGridBounds[0] > 0 &&
                   lightGridBounds[1] > 0 &&
                   lightGridBounds[2] > 0;
        }

        void clear() {
            if (lightGridData) {
                free(lightGridData);
                lightGridData = nullptr;
            }
            if (lightGrid16) {
                free(lightGrid16);
                lightGrid16 = nullptr;
            }
            lightGridDataSize = 0;
            lightGridBounds[0] = lightGridBounds[1] = lightGridBounds[2] = 0;
        }
    };

    WorldLightingData g_worldLighting;
}

/*
=================
R_InitLightingSystem
Initialize sine table and other lighting constants
=================
*/
void R_InitLightingSystem() {
    // Build sine table for light direction decoding
    // Matches tr_init.c sine table generation
    for (int i = 0; i < FUNCTABLE_SIZE; i++) {
        float value = std::sin(i * M_PI * 2.0f / FUNCTABLE_SIZE);
        g_worldLighting.sinTable[i] = value;
    }

    // Default sun direction (matches tr_world.c defaults)
    VectorSet(g_worldLighting.sunDirection, 0.45f, 0.3f, 0.9f);
    VectorNormalize(g_worldLighting.sunDirection);

    // Default identityLight - will be overwritten by R_SetIdentityLight
    g_worldLighting.identityLight = 0.5f; // Default for overbrightBits=1
}

/*
=================
R_SetIdentityLight
Set identity light value based on overbright bits
Matches tr.identityLight = 1.0f / ( 1 << tr.overbrightBits ) in OpenGL2
=================
*/
void R_SetIdentityLight(int overbrightBits) {
    // Clamp overbright bits to valid range [0, 2]
    if (overbrightBits > 2) {
        overbrightBits = 2;
    } else if (overbrightBits < 0) {
        overbrightBits = 0;
    }
    
    g_worldLighting.identityLight = 1.0f / (float)(1 << overbrightBits);
    ri.Printf(PRINT_DEVELOPER, "R_SetIdentityLight: overbrightBits=%d, identityLight=%f\n",
              overbrightBits, g_worldLighting.identityLight);
}

/*
=================
R_GetIdentityLight
Return current identity light value
=================
*/
float R_GetIdentityLight(void) {
    return g_worldLighting.identityLight;
}

/*
=================
R_ShutdownLightingSystem
Clean up lighting data
=================
*/
void R_ShutdownLightingSystem() {
    g_worldLighting.clear();
}

/*
=================
R_LoadLightGrid
Load light grid data from BSP
Matches R_LoadLightGrid from renderergl2/tr_world.c
=================
*/
void R_LoadLightGrid(const byte* gridData, int gridDataSize,
                     const uint16_t* grid16Data, int grid16DataSize,
                     const vec3_t gridOrigin, const vec3_t gridSize,
                     const int gridBounds[3]) {
    if (!gridData || gridDataSize <= 0) {
        ri.Printf(PRINT_DEVELOPER, "R_LoadLightGrid: No light grid data\n");
        g_worldLighting.clear();
        return;
    }

    // Free existing data
    g_worldLighting.clear();

    // Copy grid bounds and dimensions
    VectorCopy(gridOrigin, g_worldLighting.lightGridOrigin);
    VectorCopy(gridSize, g_worldLighting.lightGridSize);

    // Calculate inverse size for fast position->grid conversion
    g_worldLighting.lightGridInverseSize[0] = 1.0f / gridSize[0];
    g_worldLighting.lightGridInverseSize[1] = 1.0f / gridSize[1];
    g_worldLighting.lightGridInverseSize[2] = 1.0f / gridSize[2];

    g_worldLighting.lightGridBounds[0] = gridBounds[0];
    g_worldLighting.lightGridBounds[1] = gridBounds[1];
    g_worldLighting.lightGridBounds[2] = gridBounds[2];

    // Allocate and copy 8-bit grid data
    g_worldLighting.lightGridDataSize = gridDataSize;
    g_worldLighting.lightGridData = (byte*)malloc(gridDataSize);
    if (!g_worldLighting.lightGridData) {
        ri.Error(ERR_FATAL, "R_LoadLightGrid: Failed to allocate %d bytes", gridDataSize);
        return;
    }
    memcpy(g_worldLighting.lightGridData, gridData, gridDataSize);

    // Allocate and copy 16-bit grid data if present
    if (grid16Data && grid16DataSize > 0) {
        g_worldLighting.lightGrid16 = (uint16_t*)malloc(grid16DataSize * sizeof(uint16_t));
        if (!g_worldLighting.lightGrid16) {
            ri.Error(ERR_FATAL, "R_LoadLightGrid: Failed to allocate %d bytes for 16-bit data",
                     grid16DataSize * (int)sizeof(uint16_t));
            return;
        }
        memcpy(g_worldLighting.lightGrid16, grid16Data, grid16DataSize * sizeof(uint16_t));
    }

    ri.Printf(PRINT_DEVELOPER, "R_LoadLightGrid: Loaded %dx%dx%d grid (%d bytes)\n",
              gridBounds[0], gridBounds[1], gridBounds[2], gridDataSize);
}

/*
=================
R_SetupEntityLightingGrid
Calculate lighting for entity from light grid
Direct port from renderergl2/tr_light.c:129-279
=================
*/
static void R_SetupEntityLightingGrid(trRefEntity_t* ent) {
    vec3_t lightOrigin;
    int pos[3];
    int i, j;
    byte* gridData;
    float frac[3];
    int gridStep[3];
    vec3_t direction;
    float totalFactor;

    if (!g_worldLighting.isValid()) {
        // No light grid - use defaults
        ent->ambientLight[0] = ent->ambientLight[1] =
            ent->ambientLight[2] = g_worldLighting.identityLight * 150.0f;
        ent->directedLight[0] = ent->directedLight[1] =
            ent->directedLight[2] = g_worldLighting.identityLight * 150.0f;
        VectorCopy(g_worldLighting.sunDirection, ent->lightDir);
        return;
    }

    // Use lighting origin if specified, otherwise use entity origin
    if (ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    // Convert world position to grid position
    VectorSubtract(lightOrigin, g_worldLighting.lightGridOrigin, lightOrigin);
    for (i = 0; i < 3; i++) {
        float v = lightOrigin[i] * g_worldLighting.lightGridInverseSize[i];
        pos[i] = floor(v);
        frac[i] = v - pos[i];
        if (pos[i] < 0) {
            pos[i] = 0;
        } else if (pos[i] > g_worldLighting.lightGridBounds[i] - 1) {
            pos[i] = g_worldLighting.lightGridBounds[i] - 1;
        }
    }

    VectorClear(ent->ambientLight);
    VectorClear(ent->directedLight);
    VectorClear(direction);

    // Calculate grid data pointer
    // Each grid point is 8 bytes: 3 ambient RGB + 3 directed RGB + 2 direction angles
    gridStep[0] = 8;
    gridStep[1] = 8 * g_worldLighting.lightGridBounds[0];
    gridStep[2] = 8 * g_worldLighting.lightGridBounds[0] * g_worldLighting.lightGridBounds[1];
    gridData = g_worldLighting.lightGridData + pos[0] * gridStep[0]
        + pos[1] * gridStep[1] + pos[2] * gridStep[2];

    // Trilinear interpolation of 8 surrounding grid points
    totalFactor = 0;
    for (i = 0; i < 8; i++) {
        float factor;
        byte* data;
        int lat, lng;
        vec3_t normal;

        factor = 1.0;
        data = gridData;

        // Calculate interpolation factor for this corner
        for (j = 0; j < 3; j++) {
            if (i & (1 << j)) {
                if (pos[j] + 1 > g_worldLighting.lightGridBounds[j] - 1) {
                    break; // ignore values outside lightgrid
                }
                factor *= frac[j];
                data += gridStep[j];
            } else {
                factor *= (1.0f - frac[j]);
            }
        }

        if (j != 3) {
            continue;
        }

        // Check if this grid sample is valid (not in solid)
        if (g_worldLighting.lightGrid16) {
            uint16_t* data16 = g_worldLighting.lightGrid16 +
                (int)(data - g_worldLighting.lightGridData) / 8 * 6;
            if (!(data16[0] + data16[1] + data16[2] + data16[3] + data16[4] + data16[5])) {
                continue; // ignore samples in walls
            }
        } else {
            if (!(data[0] + data[1] + data[2] + data[3] + data[4] + data[5])) {
                continue; // ignore samples in walls
            }
        }

        totalFactor += factor;

        // Accumulate ambient and directed light
        if (g_worldLighting.lightGrid16) {
            uint16_t* data16 = g_worldLighting.lightGrid16 +
                (int)(data - g_worldLighting.lightGridData) / 8 * 6;

            ent->ambientLight[0] += factor * data16[0] / 257.0f;
            ent->ambientLight[1] += factor * data16[1] / 257.0f;
            ent->ambientLight[2] += factor * data16[2] / 257.0f;

            ent->directedLight[0] += factor * data16[3] / 257.0f;
            ent->directedLight[1] += factor * data16[4] / 257.0f;
            ent->directedLight[2] += factor * data16[5] / 257.0f;
        } else {
            ent->ambientLight[0] += factor * data[0];
            ent->ambientLight[1] += factor * data[1];
            ent->ambientLight[2] += factor * data[2];

            ent->directedLight[0] += factor * data[3];
            ent->directedLight[1] += factor * data[4];
            ent->directedLight[2] += factor * data[5];
        }

        // Decode light direction from lat/long angles
        lat = data[7];
        lng = data[6];
        lat *= (FUNCTABLE_SIZE / 256);
        lng *= (FUNCTABLE_SIZE / 256);

        // Decode X as cos(lat) * sin(long)
        // Decode Y as sin(lat) * sin(long)
        // Decode Z as cos(long)
        normal[0] = g_worldLighting.sinTable[(lat + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK] *
                    g_worldLighting.sinTable[lng];
        normal[1] = g_worldLighting.sinTable[lat] * g_worldLighting.sinTable[lng];
        normal[2] = g_worldLighting.sinTable[(lng + (FUNCTABLE_SIZE / 4)) & FUNCTABLE_MASK];

        VectorMA(direction, factor, normal, direction);
    }

    // Normalize if we didn't get perfect coverage
    if (totalFactor > 0 && totalFactor < 0.99f) {
        totalFactor = 1.0f / totalFactor;
        VectorScale(ent->ambientLight, totalFactor, ent->ambientLight);
        VectorScale(ent->directedLight, totalFactor, ent->directedLight);
    }

    // Apply console variable scaling
    if (r_ambientScale && r_ambientScale->value != 1.0f) {
        VectorScale(ent->ambientLight, r_ambientScale->value, ent->ambientLight);
    }
    if (r_directedScale && r_directedScale->value != 1.0f) {
        VectorScale(ent->directedLight, r_directedScale->value, ent->directedLight);
    }

    VectorNormalize2(direction, ent->lightDir);
}

/*
===============
LogLight
Debug logging for entity lighting
===============
*/
static void LogLight(trRefEntity_t* ent) {
    if (!(ent->e.renderfx & RF_FIRST_PERSON)) {
        return;
    }

    int max1 = ent->ambientLight[0];
    if (ent->ambientLight[1] > max1) {
        max1 = ent->ambientLight[1];
    } else if (ent->ambientLight[2] > max1) {
        max1 = ent->ambientLight[2];
    }

    int max2 = ent->directedLight[0];
    if (ent->directedLight[1] > max2) {
        max2 = ent->directedLight[1];
    } else if (ent->directedLight[2] > max2) {
        max2 = ent->directedLight[2];
    }

    ri.Printf(PRINT_ALL, "amb:%i  dir:%i\n", max1, max2);
}

/*
=================
R_SetupEntityLighting
Calculates all the lighting values that will be used by the renderer
Applies light grid + dynamic lights
Direct port from renderergl2/tr_light.c:319-440
=================
*/
void R_SetupEntityLighting(const refdef_t* refdef, trRefEntity_t* ent,
                           const dlight_t* dlights, int numDlights) {
    int i;
    float power;
    vec3_t dir;
    float d;
    vec3_t lightDir;
    vec3_t lightOrigin;

    // Skip if already calculated
    if (ent->lightingCalculated) {
        return;
    }
    ent->lightingCalculated = qtrue;

    // Use lighting origin if specified
    if (ent->e.renderfx & RF_LIGHTING_ORIGIN) {
        VectorCopy(ent->e.lightingOrigin, lightOrigin);
    } else {
        VectorCopy(ent->e.origin, lightOrigin);
    }

    // Sample from light grid if available
    if (!(refdef->rdflags & RDF_NOWORLDMODEL) && g_worldLighting.isValid()) {
        R_SetupEntityLightingGrid(ent);
    } else {
        // No world model - use identity lighting
        ent->ambientLight[0] = ent->ambientLight[1] =
            ent->ambientLight[2] = g_worldLighting.identityLight * 150.0f;
        ent->directedLight[0] = ent->directedLight[1] =
            ent->directedLight[2] = g_worldLighting.identityLight * 150.0f;
        VectorCopy(g_worldLighting.sunDirection, ent->lightDir);
    }

    // Bonus minimum light for all entities
    ent->ambientLight[0] += g_worldLighting.identityLight * 32.0f;
    ent->ambientLight[1] += g_worldLighting.identityLight * 32.0f;
    ent->ambientLight[2] += g_worldLighting.identityLight * 32.0f;

    // Modify the light by dynamic lights
    d = VectorLength(ent->directedLight);
    VectorScale(ent->lightDir, d, lightDir);

    for (i = 0; i < numDlights; i++) {
        const dlight_t* dl = &dlights[i];
        VectorSubtract(dl->origin, lightOrigin, dir);
        d = VectorNormalize(dir);

        power = DLIGHT_AT_RADIUS * (dl->radius * dl->radius);
        if (d < DLIGHT_MINIMUM_RADIUS) {
            d = DLIGHT_MINIMUM_RADIUS;
        }
        d = power / (d * d);

        VectorMA(ent->directedLight, d, dl->color, ent->directedLight);
        VectorMA(lightDir, d, dir, lightDir);
    }

    // Clamp ambient light
    {
        float r = ent->ambientLight[0];
        float g = ent->ambientLight[1];
        float b = ent->ambientLight[2];
        float max = MAX(MAX(r, g), b);

        if (max > 255.0f) {
            max = 255.0f / max;
            ent->ambientLight[0] *= max;
            ent->ambientLight[1] *= max;
            ent->ambientLight[2] *= max;
        }
    }

    // Clamp directed light
    {
        float r = ent->directedLight[0];
        float g = ent->directedLight[1];
        float b = ent->directedLight[2];
        float max = MAX(MAX(r, g), b);

        if (max > 255.0f) {
            max = 255.0f / max;
            ent->directedLight[0] *= max;
            ent->directedLight[1] *= max;
            ent->directedLight[2] *= max;
        }
    }

    if (r_debugLight && r_debugLight->integer) {
        LogLight(ent);
    }

    // Save out the byte packet version
    ((byte*)&ent->ambientLightInt)[0] = ri.ftol(ent->ambientLight[0]);
    ((byte*)&ent->ambientLightInt)[1] = ri.ftol(ent->ambientLight[1]);
    ((byte*)&ent->ambientLightInt)[2] = ri.ftol(ent->ambientLight[2]);
    ((byte*)&ent->ambientLightInt)[3] = 0xff;

    // Transform the direction to local space
    VectorNormalize(lightDir);
    ent->modelLightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
    ent->modelLightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
    ent->modelLightDir[2] = DotProduct(lightDir, ent->e.axis[2]);

    VectorCopy(lightDir, ent->lightDir);
}

/*
=================
R_LightForPoint
Sample light grid at a specific point
Direct port from renderergl2/tr_light.c:447-462
=================
*/
int R_LightForPoint(vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir) {
    if (!g_worldLighting.isValid()) {
        return qfalse;
    }

    trRefEntity_t ent;
    memset(&ent, 0, sizeof(ent));
    VectorCopy(point, ent.e.origin);
    R_SetupEntityLightingGrid(&ent);
    VectorCopy(ent.ambientLight, ambientLight);
    VectorCopy(ent.directedLight, directedLight);
    VectorCopy(ent.lightDir, lightDir);

    return qtrue;
}
