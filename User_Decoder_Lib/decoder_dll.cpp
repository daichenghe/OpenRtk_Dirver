#include "decoder_dll.h"
#include "decode_interface.h"
#include <stdint.h>

USERDECODERLIB_API void decode_openrtk_user(char* filename)
{
	decode_openrtk330li_interface(filename);
}

USERDECODERLIB_API void decode_openrtk_inceptio(char* filename)
{
	decode_rtk330la_interface(filename);
}

USERDECODERLIB_API void decode_ins401(char* filename, char* is_parse_dr)
{
	decode_ins401_interface(filename, is_parse_dr);
}

USERDECODERLIB_API void decode_beidou(char* filename, uint32_t kml_date)
{
	decode_beidou_interface(filename, kml_date);
}


