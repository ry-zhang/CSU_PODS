#ifndef _MAIN_CONFIG_FILE_H_
#define _MAIN_CONFIG_FILE_H_

#include <string>
#include <windows.h>
#include <atlstr.h>

using namespace std;

struct SProcessControl 
{
	bool GBODSwitch;

	SProcessControl()
	{
		GBODSwitch = false;
	}
};


 // 积分器参数
struct SMainConfig{
	string sObservationDate;              // 年月日
	string sConfigFilePath;               // 配置文件所在目录
	string sLogFileName;                  // 日志文件

	SProcessControl sProcessControl;

	SMainConfig()
	{
		sObservationDate = "20120102";
		sConfigFilePath = "";
		sLogFileName = "";
	}
};

// 配置文件函数声明


bool ReadMAINConfigFile(const string& in_sXMLFile, const string& in_sRelativePathBIN, SMainConfig& out_sMAINConfig);

bool SetDateInfo(const string& in_sConfigFile, const SMainConfig& in_sMAINConfig);
#endif  // _MAIN_CONFIG_FILE_H_