#include "tr_local.h"
#include "ref_import.h"

static void* q3_stbi_malloc(size_t size)
{
    return ri.Malloc((int)size);
}

static void q3_stbi_free(void* p)
{
    ri.Free(p);
}

static void* q3_stbi_realloc(void* p, size_t old_size, size_t new_size)
{
    if (p == NULL)
    {
        return q3_stbi_malloc(new_size);
    }

    void* p_new;
    if (old_size < new_size)
    {
        p_new = q3_stbi_malloc(new_size);
        memcpy(p_new, p, old_size);
        q3_stbi_free(p);
    }
    else
    {
        p_new = p;
    }

    return p_new;
}

#define STBI_MALLOC q3_stbi_malloc
#define STBI_FREE q3_stbi_free
#define STBI_REALLOC_SIZED q3_stbi_realloc
#define STB_IMAGE_IMPLEMENTATION
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4459)
#endif
#include "stb_image.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif