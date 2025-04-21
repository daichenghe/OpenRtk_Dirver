#pragma once
#include <stdint.h>

void decode_openrtk330li_interface(char* filename);

void decode_rtk330la_interface(char* filename);

void decode_ins401_interface(char* filename, char* is_parse_dr);

void decode_beidou_interface(char* filename);

void decode_npos122_interface(char* filename);

void decode_ins401c_interface(char* filename);

void decode_rtk350la_interface(char* filename, bool pruned = false);
