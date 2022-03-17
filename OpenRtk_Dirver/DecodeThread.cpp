#include "DecodeThread.h"
#include <QDir>
#include <QProcess>
#include <math.h>
#include "common.h"
#include "rtcm.h"
#include "gnss_math.h"
#include "openrtk_user.h"
#include "openrtk_inceptio.h"
#include "mixed_raw.h"
#include "imu_raw.h"
#include "beidou.h"

DecodeThread::DecodeThread(QObject *parent)
	: QThread(parent)
	, m_isStop(false)
	, m_show_time(false)
	, m_FileFormat(emDecodeFormat_OpenRTK330LI)
	, ins_kml_frequency(1000)
{
	m_static_point_ecp = true;
	ins401_decoder = new Ins401_Tool::Ins401_decoder();
	e2e_deocder = new E2E::E2E_protocol();
	npos122_decoder = new NPOS122_Tool::NPOS122_decoder();
	m_StaticAnalysis = new StaticAnalysis(this);
}

DecodeThread::~DecodeThread()
{
	if (ins401_decoder) {delete ins401_decoder; ins401_decoder = NULL;}
}

void DecodeThread::run()
{
	m_isStop = false;
	if (!m_FileName.isEmpty()) {
		m_TimeCounter.start();
		makeOutPath(m_FileName);
		switch (m_FileFormat)
		{
		case emDecodeFormat_OpenRTK330LI:
			decode_openrtk_user();
			break;
		case emDecodeFormat_RTK330LA:
			decode_openrtk_inceptio();
			break;
		case emDecodeFormat_Mixed_Raw:
			decode_mixed_raw();
			break;
		case emDecodeFormat_Imu:
			decode_imu();
			break;
		case emDecodeFormat_Ins401:
			decode_ins401();
			break;
		case emDecodeFormat_E2E_Protocol:
			decode_e2e_protocol();
			break;
		case emDecodeFormat_RTCM_EPVT:
			decode_rtcm_epvt();
			break;
		case emDecodeFormat_Convbin:
			decode_rtcm_convbin();
			break;
		case emDecodeFormat_RTCM_2_HEX:
			decode_rtcm_2_hex();
			break;
		case emDecodeFormat_Beidou:
			decode_beidou();
			break;
		case emDecodeFormat_NPOS112:
			decode_npos112();
			break;
		case emDecodeFormat_ST_RTCM:
			decode_st_rtcm();
			break;
		default:
			break;
		}
	}
	emit sgnFinished();
}

void DecodeThread::stop()
{
	m_isStop = true;
}

void DecodeThread::setFileFormat(int format)
{
	m_FileFormat = format;
}

void DecodeThread::setFileName(QString file)
{
	m_FileName = file;
}

void DecodeThread::setShowTime(bool show)
{
	m_show_time = show;
}

void DecodeThread::setKmlFrequency(int frequency)
{
	ins_kml_frequency = frequency;
	Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
}

void DecodeThread::setDateTimeStr(QString time)
{
	m_datatime_str = time;
}

void DecodeThread::setDateTime(QDateTime time)
{
	m_datatime = time;
}

void DecodeThread::setMIFileSwitch(bool write)
{
	ins401_decoder->set_MI_file_switch(write);
}

void DecodeThread::makeOutPath(QString filename)
{
	QFileInfo outDir(filename);
	if (outDir.isFile()) {
		QString basename = outDir.baseName();
		QString absoluteDir = outDir.absoluteDir().absolutePath();
		QDir outPath = absoluteDir + QDir::separator() + basename + "_d";
		if (!outPath.exists()) {
			outPath.mkpath(outPath.absolutePath());
		}
		m_OutBaseName = outPath.absolutePath() + QDir::separator() + basename;
	}
}

void DecodeThread::decode_openrtk_user()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		OpenRTK330LI_Tool::set_output_user_file(1);
		OpenRTK330LI_Tool::set_save_bin(1);
		OpenRTK330LI_Tool::set_base_user_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = OpenRTK330LI_Tool::input_user_raw(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		OpenRTK330LI_Tool::write_kml_files();
		OpenRTK330LI_Tool::close_user_all_log_file();
		fclose(file);
	}
}

void DecodeThread::decode_openrtk_inceptio()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int ret = 0;
		size_t file_size = getFileSize(file);
		size_t read_size = 0;
		size_t readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		RTK330LA_Tool::set_output_inceptio_file(1);
		RTK330LA_Tool::set_base_inceptio_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		m_StaticAnalysis->init();
		m_StaticAnalysis->set_out_base_name(m_OutBaseName);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (size_t i = 0; i < readcount; i++) {
				ret = RTK330LA_Tool::input_inceptio_raw(read_cache[i]);
				if (m_static_point_ecp &&ret == 1) {
					if (RTK330LA_Tool::INCEPTIO_OUT_GNSS ==  RTK330LA_Tool::get_current_type()) {
						m_StaticAnalysis->append_gnss_sol_rtk330la(RTK330LA_Tool::get_gnss_sol());
					}
					else if (RTK330LA_Tool::INCEPTIO_OUT_SCALED2 == RTK330LA_Tool::get_current_type()) {
						m_StaticAnalysis->append_imu_rtk330la(RTK330LA_Tool::get_imu_raw());
					}
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		RTK330LA_Tool::write_inceptio_kml_files();
		RTK330LA_Tool::close_inceptio_all_log_file();
		if (m_static_point_ecp) {
			m_StaticAnalysis->summary();
		}		
		fclose(file);
	}
}

void DecodeThread::decode_mixed_raw()
{
	if (is_aceinna_decoding()) return;
	set_aceinna_decoding(1);
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		set_output_aceinna_file(1);
		set_aceinna_file_basename(m_OutBaseName.toLocal8Bit().data());
		open_aceinna_log_file();
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				input_aceinna_format_raw(read_cache[i],NULL,NULL);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		close_aceinna_all_file();
		fclose(file);
	}
	set_aceinna_decoding(0);
}

void DecodeThread::decode_imu()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		char out_msg[1024] = { 0 };
		set_base_imu_file_name(m_OutBaseName.toLocal8Bit().data());
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				input_imu_raw(read_cache[i], out_msg);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		close_imu_all_log_file();
		fclose(file);
	}
}

void DecodeThread::decode_ins401()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file && ins401_decoder) {
		int ret = 0;
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		ins401_decoder->init();
		ins401_decoder->set_show_format_time(m_show_time);
		ins401_decoder->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		m_StaticAnalysis->init();
		m_StaticAnalysis->set_out_base_name(m_OutBaseName);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = ins401_decoder->input_data(read_cache[i]);
				if (m_static_point_ecp && ret == 1) {
					if (Ins401_Tool::em_GNSS_SOL == ins401_decoder->get_current_type()) {
						m_StaticAnalysis->append_gnss_sol_ins401(ins401_decoder->get_gnss_sol());
					}
					else if (Ins401_Tool::em_RAW_IMU == ins401_decoder->get_current_type()) {
						m_StaticAnalysis->append_imu_ins401(ins401_decoder->get_imu_raw());
					}
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		ins401_decoder->finish();
		if (m_static_point_ecp) {
			//m_Ins401_Analysis->summary();
			m_StaticAnalysis->summary();
		}
		fclose(file);
	}
}

void DecodeThread::decode_e2e_protocol() {
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		e2e_deocder->init();
		e2e_deocder->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				e2e_deocder->input_data(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		e2e_deocder->finish();
		fclose(file);
	}
}

void DecodeThread::decode_rtcm_epvt() {
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		gnss_rtcm_t rtcm = { 0 };
		rtcm_t* rcv0 = rtcm.rcv;
		type_999_t* teseo = &rcv0->teseo;
		double blh[3] = { 0.0 };
		double last_epvt_time = 0.0;
		int week = 0;
		char file_name[256] = { 0 };
		sprintf(file_name, "%s_spp.csv", m_OutBaseName.toLocal8Bit().data());
		FILE* spp_file = fopen(file_name, "w");
		if (!spp_file) return;
		fprintf(spp_file, "GPS_Week(),GPS_TimeofWeek(s),positionMode(),latitude(deg),longitude(deg),height(m),numberOfSVs(),hdop(),vdop(),pdop(),diffage(),velocityNorth(m/s),velocityEast(m/s),velocityUp(m/s),latitude_std(m),longitude_std(m),height_std(m)\n");
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				//e2e_deocder->input_data(read_cache[i]);
				int ret = input_rtcm3(read_cache[i],0, &rtcm);
				if (ret != 1) {
					continue;
				}
				/*total_obs_num(&rtcm.obs[0], 0);*/
				if (norm(rtcm.obs[0].pos, 3) < 0.01)
				{
					continue;
				}
				time2gpst(rcv0->time, &week);
				ecef2pos(teseo->pos, blh);
				fprintf(spp_file, "%d,%11.4f,%3d,%14.9f,%14.9f,%10.4f,%3d,%5.1f,%5.1f,%5.1f,%5.1f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f,%10.4f\n",
					week, teseo->time - week * SECONDS_IN_WEEK, 1, blh[0] * R2D, blh[1] * R2D, blh[2],
					teseo->nsat_use, (float)teseo->hdop*0.1, (float)teseo->vdop*0.1, (float)teseo->pdop*0.1, teseo->age,
					teseo->vel_enu[1], teseo->vel_enu[0], teseo->vel_enu[2], teseo->std_neu[0], teseo->std_neu[1], teseo->std_neu[2]);

				last_epvt_time = teseo->time;
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		fclose(spp_file);
		fclose(file);
	}
}

void DecodeThread::decode_rtcm_convbin()
{
	
	QString exeFile = "convbin.exe";
	QFileInfo outDir(m_FileName);
	QString command_cd = QString("cd /d %1 \n").arg(outDir.absoluteDir().absolutePath());
	QString command = QString("%1 -r rtcm3 -v 3.04 -tr %2  -od -os -oi -ot -ol -f 7 \"%3\" -p %4").arg(QDir::currentPath()+QDir::separator()+ exeFile, m_datatime_str, m_FileName, m_OutBaseName+"_out.csv");
	
	QFile bat_file("run_conbin.bat");
	if (!bat_file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		return;
	}
	bat_file.write(command_cd.toLocal8Bit());
	bat_file.write(command.toLocal8Bit());
	bat_file.close();
	system("run_conbin.bat");
}

void DecodeThread::decode_rtcm_2_hex()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		gnss_rtcm_t rtcm = { 0 };
		rtcm_t* rcv0 = rtcm.rcv;
		char file_name[256] = { 0 };
		sprintf(file_name, "%s_byte.txt", m_OutBaseName.toLocal8Bit().data());
		FILE* byte_file = fopen(file_name, "w");
		rcv0->time.time =  m_datatime.toTime_t();
		//set_approximate_time(2022, 17, rcv0);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				//e2e_deocder->input_data(read_cache[i]);
				int ret = input_rtcm3(read_cache[i], 0, &rtcm);
				if (is_complete_rtcm()) {
					//rcv0->buff, rcv0->len + 3;
					int size = rcv0->len + 3;
					fprintf(byte_file,"%lld,", rcv0->time.time, size);
					for (int i = 0; i < size; i++) {
						fprintf(byte_file, " %d", rcv0->buff[i]);
					}
					fprintf(byte_file,"\n");
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		fclose(byte_file);
		fclose(file);
	}
}

void DecodeThread::decode_beidou()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		char dirname[256] = { 0 };
		int ret = 0;
		size_t file_size = getFileSize(file);
		size_t read_size = 0;
		size_t readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		beidou_Tool::set_output_beidou_file(1);
		beidou_Tool::set_base_beidou_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (size_t i = 0; i < readcount; i++) {
				ret = beidou_Tool::input_beidou_raw(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		beidou_Tool::write_beidou_kml_files();
		beidou_Tool::close_beidou_all_log_file();
		fclose(file);
	}
}

void DecodeThread::decode_npos112()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file && npos122_decoder) {
		int8_t ret = 0;
		int8_t ret_nmea = 0;
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		size_t readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		npos122_decoder->init();
		npos122_decoder->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (size_t i = 0; i < readcount; i++) {
				ret_nmea = npos122_decoder->parse_nmea(read_cache[i]);
				ret = npos122_decoder->input_data(read_cache[i]);
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		npos122_decoder->finish();
		fclose(file);
	}
}

void DecodeThread::decode_st_rtcm()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	FILE* newfile = fopen((m_FileName + "_new.bin").toLocal8Bit().data(), "wb");
	if (file && newfile) {
		int8_t ret = 0;
		int8_t ret_nmea = 0;
		int64_t file_size = getFileSize(file);
		int64_t read_size = 0;
		size_t readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		rtcm_t rtcm = { 0 };
		obs_t obs = { 0 };
		nav_t nav = { 0 };
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (size_t i = 0; i < readcount; i++) {
				ret = input_rtcm3_data(&rtcm, read_cache[i], &obs, &nav);
				if (is_complete_rtcm()) {
					int size = rtcm.len + 3;
					fwrite(rtcm.buff, 1, size, newfile);
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		fclose(file);
		fclose(newfile);
	}
}
