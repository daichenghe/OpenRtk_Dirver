#include "common.h"
#include "openrtk_user.h"
#include "openrtk_inceptio.h"
#include "ins401.h"
#include "decoder_dll.h"
#include <string.h>
#include "ins_save_parse.h"

USERDECODERLIB_API void decode_openrtk_user(char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file) {
		char dirname[256] = { 0 };
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		set_output_user_file(1);
		createDirByFilePath(filename, dirname);
		set_base_user_file_name(dirname);
		while (!feof(file)) {
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = input_user_raw(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 100;
			printf("Process : %4.1f %%\r", percent);
		}
		write_kml_files();
		close_user_all_log_file();
		fclose(file);
		printf("\nfinished\r\n");
	}
}

USERDECODERLIB_API void decode_openrtk_inceptio(char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file) {
		char dirname[256] = { 0 };
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		set_output_inceptio_file(1);
		createDirByFilePath(filename, dirname);
		set_base_inceptio_file_name(dirname);
		while (!feof(file)) {
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = input_inceptio_raw(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 100;
			printf("Process : %4.1f %%\r", percent);
		}
		write_inceptio_kml_files();
		close_inceptio_all_log_file();
		fclose(file);
		printf("\nfinished\r\n");
	}
}

USERDECODERLIB_API void decode_ins401(char* filename, char* is_parse_dr, uint32_t inskml_rate)
{
	Ins401::Ins401_decoder* ins401_decoder = new Ins401::Ins401_decoder();
	Kml_Generator::Instance()->inskml_rate = inskml_rate;
	FILE* file = fopen(filename, "rb");
	if( (strcmp(_strlwr(is_parse_dr), "false") == 0) && (strstr(_strlwr(filename), "ins_save") != NULL ) )
	{
		return;
	}
	if (file && ins401_decoder) {
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		char dirname[256] = { 0 };
		createDirByFilePath(filename, dirname);
		ins401_decoder->init();
		ins401_decoder->set_base_file_name(dirname);
		while (!feof(file)) {
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = ins401_decoder->input_data(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 100;
			printf("Process : %4.1f %%\r", percent);
		}
		if(strstr(filename, "ins_save") != NULL)
		{
			ins401_decoder->ins_save_finish();
		}
		else
		{
			ins401_decoder->finish();
		}
		fclose(file);
		printf("\nfinished\r\n");
	}
}

Ins401::Ins401_decoder* ins401_decoder = new Ins401::Ins401_decoder();
// Kml_Generator::Instance()->inskml_rate = 10;

USERDECODERLIB_API void decode_ins401_data(uint8_t* data, uint32_t len)
{
	if (ins401_decoder) 
	{
		int ret = 0;
		ins401_decoder->init();
		// ins401_decoder->set_base_file_name(dirname);
		for (int i = 0; i < len; i++) {
			ret = ins401_decoder->input_data(data[i]);
		}

	}
}
