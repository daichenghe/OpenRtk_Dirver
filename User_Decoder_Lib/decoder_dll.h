#pragma once
#include <stdint.h>
#ifdef WIN32
#ifdef USERDECODERLIB_EXPORTS
#define USERDECODERLIB_API __declspec(dllexport)
#else
#define USERDECODERLIB_API __declspec(dllimport)
#endif
#else
#define USERDECODERLIB_API __attribute__ ((visibility("default")))
#endif

extern "C" USERDECODERLIB_API void decode_openrtk_user(char* filename);

extern "C" USERDECODERLIB_API void decode_openrtk_inceptio(char* filename);

extern "C" USERDECODERLIB_API void decode_ins401(char* filename, char* is_parse_dr);

extern "C" USERDECODERLIB_API void decode_beidou(char* filename, uint32_t kml_date);
