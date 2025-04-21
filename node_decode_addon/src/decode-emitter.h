#include <napi.h>
#include "ins401.h"
#include "rtk330la_decoder.h"
#include "SplitByTime.h"
#include <vector>

typedef struct {
  uint32_t slice;
  uint32_t time;
  double angle;
  double speed;
}TurnInfo_t;

typedef struct {
  uint32_t slice;
  uint32_t time;
  double distance;
  double speed;
}Distance_t;

typedef struct
{
	float roll;
	float pitch;
	float heading;
}stAngle;

class decodeEmitter : public Napi::ObjectWrap<decodeEmitter> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  decodeEmitter(const Napi::CallbackInfo& info);
  ~decodeEmitter();

 private:
  static Napi::FunctionReference constructor;

  Napi::Value SetExePath(const Napi::CallbackInfo& info);
  Napi::Value SetMinDistanceLimit(const Napi::CallbackInfo& info);  
  Napi::Value StartStep(const Napi::CallbackInfo& info);
  Napi::Value EndStep(const Napi::CallbackInfo& info);
  Napi::Value ContinueStep(const Napi::CallbackInfo& info);
  Napi::Value InitIns401(const Napi::CallbackInfo& info);
  Napi::Value InputIns401Buffer(const Napi::CallbackInfo& info);
  Napi::Value DecodeIns401(const Napi::CallbackInfo& info);
  Napi::Value InitRtk330la(const Napi::CallbackInfo& info);
  Napi::Value InputRtk330laBuffer(const Napi::CallbackInfo& info);
  Napi::Value DecodeRtk330la(const Napi::CallbackInfo& info);
  Napi::Value DecodeRtk350la(const Napi::CallbackInfo& info);
  Napi::Value SplitPostInsByTime(const Napi::CallbackInfo& info);
  Napi::Value CalcRoll(const Napi::CallbackInfo& info);
  Napi::Value CalcPitchHeading(const Napi::CallbackInfo& info);

  Ins401_Tool::Ins401_decoder* m_Ins401_decoder;
  RTK330LA_Tool::Rtk330la_decoder* m_Rtk330la_decoder;
  SplitByTime* m_SplitByTime;
  std::vector<stAngle> angle_list;
  std::vector<stAngle> roll_list;
  int m_step;
  bool m_step_running;
  std::string m_ExePath;
};
