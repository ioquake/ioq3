#ifndef HUMBLENET_H
#define HUMBLENET_H


#include <stddef.h>
#include <stdint.h>

#ifdef HUMBLENET_STATIC
	#define HUMBLENET_DLL_EXPORT
#else
	#ifdef WIN32
		#ifdef HUMBLENET_DLL_BUILD
			#define HUMBLENET_DLL_EXPORT __declspec(dllexport)
		#else
			#define HUMBLENET_DLL_EXPORT __declspec(dllimport)
		#endif
	#else // GCC
		#if defined(__GNUC__) && __GNUC__ >= 4
		# define HUMBLENET_DLL_EXPORT __attribute__ ((visibility("default")))
		#else
		# define HUMBLENET_DLL_EXPORT
		#endif
	#endif
#endif

#if defined(WIN32)
	#define HUMBLENET_CALL __cdecl
#elif defined(__GNUC__)
	#if defined(__LP64__)
		#define HUMBLENET_CALL
	#else
		#define HUMBLENET_CALL __attribute__((cdecl))
	#endif
#else
	#define HUMBLENET_CALL
#endif

#define HUMBLENET_API HUMBLENET_DLL_EXPORT

#define HUMBLENET_MAJOR_VERSION 1
#define HUMBLENET_MINOR_VERSION 0
#define HUMBLENET_PATCHLEVEL 0

/**
 * Create a comparable version number value
 *
 * e.g.  (1,2,3) -> (0x010203)
 */
#define HUMBLENET_VERSIONNUM(X, Y, Z) \
	(((X) << 16) | ((Y) << 8) | (Z))

/**
 * this is the version that we are currently compiling against
 */
#define HUMBLENET_COMPILEDVERSION \
	HUMBLENET_VERSIONNUM(HUMBLENET_MAJOR_VERSION, HUMBLENET_MINOR_VERSION, HUMBLENET_PATCHLEVEL)

/**
 * check if compiled with HumbleNet version of at least X.Y.Z
 */
#define HUMBLENET_VERSION_ATLEAST(X, Y, Z) \
	(HUMBLENET_COMPILEDVERSION >= HUMBLENET_VERSIONNUM(X, Y, Z))

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ha_bool;
typedef uint32_t PeerId;

/*
 * Get the version of the humblenet library we are linked to
 */
HUMBLENET_API uint32_t HUMBLENET_CALL humblenet_version();

/*
 * Initialize the core humblenet library
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_init();

/*
 * Shutdown the entire humblenet library
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_shutdown();

/*
 * Allow network polling to occur. This is already thread safe and must NOT be within a lock/unlock block
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_poll(int ms);

/*
 * Get error string
 * Return value will stay valid until next call to humblenet_set_error
 * or humblenet_clear_error() is called
 */
HUMBLENET_API const char * HUMBLENET_CALL humblenet_get_error();

/*
 * Set error string
 * Parameter is copied to private storage
 * Must not be NULL
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_set_error(const char *error);

/*
 * Clear error string
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_clear_error();

/*
 * Set the value of a hint
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_set_hint(const char* name, const char* value);

/*
 * Get the value of a hint
 */
HUMBLENET_API const char* HUMBLENET_CALL humblenet_get_hint(const char* name);

/*
 * If using the loader this will set the loader path.
 * If using the loading returns 1 if the library loads or 0 if it fails to load.
 *    Use humblenet_get_error() to see why loading failed.
 * If not using the loader, simply returns 1
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_loader_init(const char* libpath);

#ifdef __cplusplus
}
#endif


#endif /*HUMBLENET_H */
