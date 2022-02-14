#include "SimpleDecodeThread.h"
#include <QDir>
#include <math.h>
#include "common.h"
#include "openrtk_user.h"
#include "openrtk_inceptio.h"
#include <QProcess>

SimpleDecodeThread::SimpleDecodeThread(QObject *parent)
	: QThread(parent)
	, m_isStop(false)
	, m_show_time(false)
	, m_FileFormat(emDecodeFormat_OpenRTK330LI)
	, ins_kml_frequency(1000)
{
	ins401_decoder = new Ins401_Tool::Ins401_decoder();
	m_MountAngle = new MountAngle(this);
}

SimpleDecodeThread::~SimpleDecodeThread()
{
	if (ins401_decoder) {delete ins401_decoder; ins401_decoder = NULL;}
}

void SimpleDecodeThread::run()
{
	m_isStop = false;
	if (!m_FileName.isEmpty()) {
		m_TimeCounter.start();
		switch (m_FileFormat)
		{
		case emDecodeFormat_OpenRTK330LI:
			decode_openrtk330li();
			break;
		case emDecodeFormat_RTK330LA:
			decode_openrtk_inceptio();
			break;
		case emDecodeFormat_Ins401:
			decode_ins401();
			break;
		default:
			break;
		}
	}
	emit sgnFinished();
}

void SimpleDecodeThread::stop()
{
	m_isStop = true;
}

void SimpleDecodeThread::setFileFormat(int format)
{
	m_FileFormat = format;
}

void SimpleDecodeThread::setFileName(QString file)
{
	m_FileName = file;
	if (!m_FileName.isEmpty()) {
		makeOutPath(m_FileName);
	}
}

void SimpleDecodeThread::setShowTime(bool show)
{
	m_show_time = show;
}

void SimpleDecodeThread::setKmlFrequency(int frequency)
{
	ins_kml_frequency = frequency;
	Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
}

void SimpleDecodeThread::setDateTime(QString time)
{
	m_datatime = time;
}

QString& SimpleDecodeThread::getOutBaseName()
{
	return m_OutBaseName;
}

std::vector<stTimeSlice>& SimpleDecodeThread::get_time_slices()
{
	return m_MountAngle->get_time_slices();
}

void SimpleDecodeThread::makeOutPath(QString filename)
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

void SimpleDecodeThread::decode_openrtk330li()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		OpenRTK330LI_Tool::set_output_user_file(1);
		OpenRTK330LI_Tool::set_base_user_file_name(m_OutBaseName.toLocal8Bit().data());
		m_MountAngle->init();
		m_MountAngle->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = OpenRTK330LI_Tool::input_user_raw(read_cache[i]);
				if (ret == 1) {
					if (OpenRTK330LI_Tool::USR_OUT_INSPVAX == OpenRTK330LI_Tool::get_current_type()) {
						ins_sol_data ins_data = { 0 };
						OpenRTK330LI_Tool::user_i1_t* ins_p = OpenRTK330LI_Tool::get_ins_sol();
						ins_data.gps_week = ins_p->GPS_Week;
						ins_data.gps_millisecs = ins_p->GPS_TimeOfWeek;
						ins_data.ins_status = ins_p->ins_status;
						ins_data.ins_position_type = ins_p->ins_position_type;
						ins_data.latitude = ins_p->latitude;
						ins_data.longitude = ins_p->longitude;
						ins_data.height = ins_p->height;
						ins_data.north_velocity = ins_p->north_velocity;
						ins_data.east_velocity = ins_p->east_velocity;
						ins_data.up_velocity = ins_p->up_velocity;
						ins_data.roll = ins_p->roll;
						ins_data.pitch = ins_p->pitch;
						ins_data.heading = ins_p->heading;
						m_MountAngle->process_live_data(ins_data);
					}
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		OpenRTK330LI_Tool::write_kml_files();
		OpenRTK330LI_Tool::close_user_all_log_file();
		m_MountAngle->finish();
		fclose(file);
	}
}

void SimpleDecodeThread::decode_openrtk_inceptio()
{
	FILE* file = fopen(m_FileName.toLocal8Bit().data(), "rb");
	if (file) {
		int ret = 0;
		int file_size = getFileSize(file);
		int read_size = 0;
		int readcount = 0;
		char read_cache[READ_CACHE_SIZE] = { 0 };
		RTK330LA_Tool::set_output_inceptio_file(1);
		RTK330LA_Tool::set_base_inceptio_file_name(m_OutBaseName.toLocal8Bit().data());
		m_MountAngle->init();
		m_MountAngle->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = RTK330LA_Tool::input_inceptio_raw(read_cache[i]);
				if (ret == 1) {
					if (RTK330LA_Tool::INCEPTIO_OUT_INSPVA == RTK330LA_Tool::get_current_type()) {
						ins_sol_data ins_data = { 0 };
						RTK330LA_Tool::inceptio_iN_t* ins_p = RTK330LA_Tool::ger_ins_sol();
						ins_data.gps_week = ins_p->GPS_Week;
						ins_data.gps_millisecs = uint32_t(ins_p->GPS_TimeOfWeek * 1000);
						ins_data.ins_status = ins_p->insStatus;
						ins_data.ins_position_type = ins_p->insPositionType;
						ins_data.latitude = (double)ins_p->latitude *180.0 / MAX_INT;
						ins_data.longitude = (double)ins_p->longitude *180.0 / MAX_INT;
						ins_data.height = ins_p->height;
						ins_data.north_velocity = (float)ins_p->velocityNorth / 100.0;
						ins_data.east_velocity = (float)ins_p->velocityEast / 100.0;
						ins_data.up_velocity = (float)ins_p->velocityUp / 100.0;
						ins_data.roll = (float)ins_p->roll / 100.0;
						ins_data.pitch = (float)ins_p->pitch / 100.0;
						ins_data.heading = (float)ins_p->heading / 100.0;
						m_MountAngle->process_live_data(ins_data);
					}
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		RTK330LA_Tool::write_inceptio_kml_files();
		RTK330LA_Tool::close_inceptio_all_log_file();
		m_MountAngle->finish();
		fclose(file);
	}
}

void SimpleDecodeThread::decode_ins401()
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
		m_MountAngle->init();
		m_MountAngle->set_base_file_name(m_OutBaseName.toLocal8Bit().data());
		Kml_Generator::Instance()->set_kml_frequency(ins_kml_frequency);
		while (!feof(file)) {
			if (m_isStop) break;
			readcount = fread(read_cache, sizeof(char), READ_CACHE_SIZE, file);
			read_size += readcount;
			for (int i = 0; i < readcount; i++) {
				ret = ins401_decoder->input_data(read_cache[i]);
				if (ret == 1) {
					if (Ins401_Tool::em_INS_SOL == ins401_decoder->get_current_type()) {
						ins_sol_data ins_data = { 0 };
						Ins401_Tool::ins_sol_t* ins_p = ins401_decoder->get_ins_sol();
						ins_data.gps_week = ins_p->gps_week;
						ins_data.gps_millisecs = ins_p->gps_millisecs;
						ins_data.ins_status =ins_p->ins_status;
						ins_data.ins_position_type = ins_p->ins_position_type;
						ins_data.latitude = ins_p->latitude;
						ins_data.longitude = ins_p->longitude;
						ins_data.height = ins_p->height;
						ins_data.north_velocity = ins_p->north_velocity;
						ins_data.east_velocity = ins_p->east_velocity;
						ins_data.up_velocity = ins_p->up_velocity;
						ins_data.roll = ins_p->roll;
						ins_data.pitch = ins_p->pitch;
						ins_data.heading = ins_p->heading;
						m_MountAngle->process_live_data(ins_data);
					}
				}
			}
			double percent = (double)read_size / (double)file_size * 10000;
			emit sgnProgress((int)percent, m_TimeCounter.elapsed());
		}
		ins401_decoder->finish();
		m_MountAngle->finish();		
		fclose(file);
	}
}