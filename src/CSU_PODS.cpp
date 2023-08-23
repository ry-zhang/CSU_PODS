#include "Poco/Task.h"
#include "Poco/TaskManager.h"
#include "Poco/ThreadPool.h"
#include "Poco/Process.h"
#include "Poco/Logger.h"
#include "Poco/URIStreamOpener.h"
#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/FileChannel.h"
#include "Poco/NumberParser.h"
#include "Poco/Thread.h"
#include "CompletenessValidator.h"
#include "DateChanger.h"
#include "TaskSetting.h"
#include "FileValidator.h"
#include "CommonOperation.h"
#include "DataStructures.h"

#include "PreProcessInterface.h"
#include "SPPInterface.h"
#include "DynFitInterface.h"
#include "DataEditingInterface.h"
#include "RDODInterface.h"
#include "KIPPInterface.h"

#include "IntegrationDataStore.h"
#include "TimeSystem.h"
#include "PvtStore.h"
#include "StringUtils.h"

#include "time.h"
#include "data_interface.h"
#include "FTP.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <cassert>
#include <string>
#include <windows.h>
#include <WinBase.h>
#include <cstdio>
#include <stdio.h>

#include "tinystr.h"
#include "tinyxml.h"

using namespace std;
using namespace Poco;
using namespace StringUtils;
using namespace CommonOperation;

bool TimeFormatTransform(const STimeFormat & in_sReceivetime, CTimeSystem & out_oCurObsTime);
bool TimeFormatTransform(const CTimeSystem & in_oCurObsTime, const ETimeFrame& in_eTimeFrame, STimeFormat & out_sReceivetime);

int main(int argc, char* argv[])
{
	AutoPtr<PatternFormatter> pPF(new PatternFormatter);
	pPF->setProperty("pattern", "%Y-%m-%d %H:%M:%S %s: %t");
	pPF->setProperty("times", "local");

	AutoPtr<FormattingChannel> pChannel(new Poco::FormattingChannel(pPF));//日志系统相关。初始化。
	pChannel->setChannel(new Poco::ConsoleChannel);

	Logger::root().setChannel(pChannel);
	Logger::root().setLevel(Poco::Message::PRIO_DEBUG);

	FileValidator* fv = new FileValidator();

	//根据时间间隔扫描观测量文件夹。
	//扫描观测量文件夹。提取需要更新的观测量文件名称中的日期数据。
	TaskSetting* ts = new TaskSetting();
	
	//配置文件所在的BIN文件夹相对于可执行文件（.exe)的相对路径
	//该路径放在RelativePathBIN.txt文件中
	string strCurrentPath = CommonOperation::GetCurrentPath();
	string strFileNameBIN = "PathSetting/RelativePathBIN.txt";

	ifstream ifFile;
	ifFile.open(strCurrentPath + strFileNameBIN);
	if (!ifFile.is_open())
	{
		cout << "BIN folder path file " << strCurrentPath + strFileNameBIN << " is not exist!" << endl;
		//system("pause");
		exit(-1);
	}
	char cText[200];
	ifFile.getline(cText, 200);
	ifFile.close();

	string strBINDir(cText);
	if (!CommonOperation::IsFolderExists(strBINDir))
	{
		cout << "BIN folder " << strCurrentPath + strBINDir << " is not exist!" << endl;
		//system("pause");
		exit(-1);
	}
	string sPreProcessConfigFile = strBINDir + "/Config/KJZ-Simu/PreProcess_global.xml";//地面预处理程序 true
	string sGBODConfigFile = strBINDir + "/Config/KJZ-Simu/GBOD_global.xml";//地面批处理程序 true

	ts->setRelativePathBIN("RelativePathBIN",strBINDir);
	ts->readConfigFile(strBINDir+"/Config/TaskSettings.xml");

	fv->readConfigFile(strBINDir + "/Config/FileValidators.xml");

	//清楚历史扫描结果
	//string sOldData = strBINDir + "/LOG/KJZ-Simu/TaskSetting.his";
	const char *name = ts->getHisFileName().c_str();
	if (remove(name) == 0) {
		printf("已删除历史扫描数据\n");
	}
	else {
		printf("无历史扫描数据\n");
	}

	bool bIsDebug = true;

	while (true)
	{

		printf("Waiting for new observation file.\n");

		if (bIsDebug)
		{
			//1秒检查一次，用于调试
			Thread::sleep(1 * 1000);
		}
		else
		{
			//每隔一段时间检查一次观测文件目录,检测间隔由配置文件指定，正式运行时使用
			Thread::sleep(ts->getSleepIntervals() * 1000);

		}

		time_t t = time(0);
		tm * lct = localtime(&t);
		printf("Check Time %4d-%02d-%02d %02d:%02d:%02d\n", lct->tm_year + 1900, lct->tm_mon + 1, lct->tm_mday, lct->tm_hour, lct->tm_min, lct->tm_sec);

		//判断当天数据是否已经处理完毕，读取已经处理完毕的日期
		ts->readHistoryFile();

		vector<string> vUpdateObsDate, vUpdateObsFiles;

		//扫描文件，看是否有新增的文件。把新增文件名称返回。
		ts->filescanner(vUpdateObsDate, vUpdateObsFiles, true);

		if (vUpdateObsDate.size() != 0)
		{
			printf("有新观测数据\n");
		}
		else
		{
			printf("无新观测数据\n");
			return 0;
		}

		for (int i = 0; i < vUpdateObsDate.size(); i++) {
			ts->saveHistoryFile(vUpdateObsDate[i]);//处理完毕的日期序列写入文件
		}

		cout << "... 预处理开始 ... " << endl;

		CPreProcessInterface* pPreProcess = new CPreProcessInterface();

		pPreProcess->ReadConfigFile(sPreProcessConfigFile, strBINDir);

		pPreProcess->Init();

		//扫描LEO轨道数据文件
		string vLEOFiles;
		ts->filescannerorbitleo(vLEOFiles);

		pPreProcess->API_ReadLEOSP3File(vLEOFiles);

		//扫描标称脉冲文件
		string vUpdateNominalImpulseFiles;
		ts->filescannernominalimpulse(vUpdateNominalImpulseFiles);

		pPreProcess->ReadImpulseFile(vUpdateNominalImpulseFiles);

		//string vUpdateImpulseFiles;
		//ts->filescannerimpulse(vUpdateImpulseFiles);

		//pPreProcess->ReadImpulseFile(vUpdateImpulseFiles);

		//读取星间星地测量数据
		for (int i = 0; i < vUpdateObsFiles.size(); i++) {
			pPreProcess->API_ReadSSTFile(vUpdateObsFiles[i]);
		}

		//上一次脉冲时间
		STimeFormat sLastImpulseEpoch;
		CTimeSystem oLastImpulseEpoch;

		//观测数据最后一个历元时刻
		STimeFormat sObsLasttime;
		CTimeSystem oObsLastEpoch;
		pPreProcess->API_GetObsTime(sObsLasttime, sLastImpulseEpoch);

		cout << "上一次脉冲时刻 " << sLastImpulseEpoch.iYear << "-" << sLastImpulseEpoch.iMonth << "-" << sLastImpulseEpoch.iDay << "T" << sLastImpulseEpoch.iHour << ":" << sLastImpulseEpoch.iMinute << ":" << sLastImpulseEpoch.dSec << endl;
		TimeFormatTransform(sLastImpulseEpoch, oLastImpulseEpoch);

		cout << "观测数据最后一个历元时刻 " << sObsLasttime.iYear << "-" << sObsLasttime.iMonth << "-" << sObsLasttime.iDay << "T" << sObsLasttime.iHour << ":" << sObsLasttime.iMinute << ":" << sObsLasttime.dSec << endl;
		TimeFormatTransform(sObsLasttime, oObsLastEpoch);

		//批处理初始时刻
		STimeFormat sInittime;
		CTimeSystem oInitEpoch;
		SCalDate sInitDate;

		int dt = oObsLastEpoch - oLastImpulseEpoch;
		bool bscanbefore = false;

		if (dt > 6 * 60 * 60)
		{
			printf("新数据跨度大于6小时，进行批处理\n");
			if (dt < 24 * 60 * 60) {
				oInitEpoch = oLastImpulseEpoch;
				//初始历元取整分钟
				sInitDate = oInitEpoch.GetCalDate(TimeFrame_UTC);
				if (sInitDate.dSec > 0) {
					sInitDate.dSec = 0;
					sInitDate.iMin = int(sInitDate.iMin + 1);
					SMJD smjd = CTimeSystem::CalDate2MJD(sInitDate);
					sInitDate = CTimeSystem::MJD2CalDate(smjd);
				}
				sInittime.iYear = sInitDate.iYear;
				sInittime.iMonth = sInitDate.iMonth;
				sInittime.iDay = sInitDate.iDay;
				sInittime.iHour = sInitDate.iHour;
				sInittime.iMinute = sInitDate.iMin;
				sInittime.dSec = sInitDate.dSec;
				strcpy_s(sInittime.cTimeFrame, "UTC");
				TimeFormatTransform(sInittime, oInitEpoch);
				cout << "批处理初始时刻 " << sInittime.iYear << "-" << sInittime.iMonth << "-" << sInittime.iDay << "T" << sInittime.iHour << ":" << sInittime.iMinute << ":" << sInittime.dSec << endl;
				bscanbefore = true;
			}
			else {
				oInitEpoch = oObsLastEpoch - 24 * 60 * 60;
				//初始历元取整分钟
				sInitDate = oInitEpoch.GetCalDate(TimeFrame_UTC);
				if (sInitDate.dSec > 0) {
					sInitDate.dSec = 0;
					sInitDate.iMin = int(sInitDate.iMin + 1);
					SMJD smjd = CTimeSystem::CalDate2MJD(sInitDate);
					sInitDate = CTimeSystem::MJD2CalDate(smjd);
				}
				sInittime.iYear = sInitDate.iYear;
				sInittime.iMonth = sInitDate.iMonth;
				sInittime.iDay = sInitDate.iDay;
				sInittime.iHour = sInitDate.iHour;
				sInittime.iMinute = sInitDate.iMin;
				sInittime.dSec = sInitDate.dSec;
				strcpy_s(sInittime.cTimeFrame, "UTC");
				TimeFormatTransform(sInittime, oInitEpoch);
				cout << "批处理初始时刻 " << sInittime.iYear << "-" << sInittime.iMonth << "-" << sInittime.iDay << "T" << sInittime.iHour << ":" << sInittime.iMinute << ":" << sInittime.dSec << endl;
				bscanbefore = true;
			}
		}
		else {
			printf("新数据跨度未大于6小时，不进行批处理\n");
			return 0;
		}

		//扫描补充之前两天内的数据
		vector<string> vAllObsDate, vAllObsFiles;
		vector<string> vOldObsDate, vOldObsFiles;

		if (bscanbefore == true) {
			ts->filescanner(vAllObsDate, vAllObsFiles, false);

			//筛选一天内的数据且不与新更新的数据重复
			for (int i = 0; i < vAllObsDate.size(); i++) {

				string strBeginYear, strBeginMonth, strBeginDay, strBeginHour, strBeginMin, strBeginSec;
				int iBeginYear, iBeginMonth, iBeginDay, iBeginHour, iBeginMin, iBeginSec;
				strBeginYear = vAllObsDate[i].substr(0, 4);
				strBeginMonth = vAllObsDate[i].substr(4, 2);
				strBeginDay = vAllObsDate[i].substr(6, 2);
				strBeginHour = vAllObsDate[i].substr(8, 2);
				strBeginMin = vAllObsDate[i].substr(10, 2);
				strBeginSec = vAllObsDate[i].substr(12, 2);

				iBeginYear = stoi(strBeginYear);
				iBeginMonth = stoi(strBeginMonth);
				iBeginDay = stoi(strBeginDay);
				iBeginHour = stoi(strBeginHour);
				iBeginMin = stoi(strBeginMin);
				iBeginSec = stoi(strBeginSec);

				STimeFormat sObsBegintime;
				sObsBegintime.iYear = iBeginYear;
				sObsBegintime.iMonth = iBeginMonth;
				sObsBegintime.iDay = iBeginDay;
				sObsBegintime.iHour = iBeginHour;
				sObsBegintime.iMinute = iBeginMin;
				sObsBegintime.dSec = iBeginSec;
				strcpy_s(sObsBegintime.cTimeFrame, "UTC");
				CTimeSystem oObsBeginEpoch;
				TimeFormatTransform(sObsBegintime, oObsBeginEpoch);

				string strEndYear, strEndMonth, strEndDay, strEndHour, strEndMin, strEndSec;
				int iEndYear, iEndMonth, iEndDay, iEndHour, iEndMin, iEndSec;
				strEndYear = vAllObsDate[i].substr(15, 4);
				strEndMonth = vAllObsDate[i].substr(19, 2);
				strEndDay = vAllObsDate[i].substr(21, 2);
				strEndHour = vAllObsDate[i].substr(23, 2);
				strEndMin = vAllObsDate[i].substr(25, 2);
				strEndSec = vAllObsDate[i].substr(27, 2);

				iEndYear = stoi(strEndYear);
				iEndMonth = stoi(strEndMonth);
				iEndDay = stoi(strEndDay);
				iEndHour = stoi(strEndHour);
				iEndMin = stoi(strEndMin);
				iEndSec = stoi(strEndSec);

				STimeFormat sObsEndtime;
				sObsEndtime.iYear = iEndYear;
				sObsEndtime.iMonth = iEndMonth;
				sObsEndtime.iDay = iEndDay;
				sObsEndtime.iHour = iEndHour;
				sObsEndtime.iMinute = iEndMin;
				sObsEndtime.dSec = iEndSec;
				strcpy_s(sObsEndtime.cTimeFrame, "UTC");
				CTimeSystem oObsEndEpoch;
				TimeFormatTransform(sObsEndtime, oObsEndEpoch);

				if (oObsLastEpoch - 24 * 60 * 60 <= oObsBeginEpoch && oObsLastEpoch - 24 * 60 * 60 <= oObsEndEpoch && oInitEpoch <= oObsBeginEpoch)
				{
					vOldObsDate.push_back(vAllObsDate[i]);
					vOldObsFiles.push_back(vAllObsFiles[i]);
				}
			}

			for (int i = 0; i < vOldObsFiles.size(); i++) {
				pPreProcess->API_ReadSSTFile(vOldObsFiles[i]);
			}
		}

		//扫描轨道初值数据文件
		string vOrbFiles;
		ts->filescannerorbit(vOrbFiles);

		pPreProcess->API_ReadOrbitSP3File(vOrbFiles);

		//string vCopyOrbFiles;
		//ifstream ifs(vOrbFiles, ios::in);
		//char ch;
		//if (vOrbFiles.size() == 0) {
		//	ts->filescannercopyorbit(vCopyOrbFiles);
		//	pPreProcess->API_ReadOrbitFile(vCopyOrbFiles);
		//}
		//else {
		//	ifs >> ch;
		//	if (ifs.eof()) {
		//		ts->filescannercopyorbit(vCopyOrbFiles);
		//		pPreProcess->API_ReadOrbitFile(vCopyOrbFiles);
		//	}
		//	else {
		//		pPreProcess->API_ReadOrbitFile(vOrbFiles);
		//	}
		//}

		//初始化轨道初值
		SOrbitStatesFormat sInitStates;
		pPreProcess->API_GetOrbitResult(sInittime, sInitStates);

		sInitStates.bJ2K = true;

		SInitStateError sInitStateError;
		sInitStateError.dSTD_P[0] = 0;//1000;
		sInitStateError.dSTD_P[1] = 0;//1000;
		sInitStateError.dSTD_P[2] = 0;//1000;
		sInitStateError.dSTD_V[0] = 0;//0.1;
		sInitStateError.dSTD_V[1] = 0;//0.1;
		sInitStateError.dSTD_V[2] = 0;//0.1;

		pPreProcess->API_SetInitStates(sInittime, sInitStates, sInitStateError);

		pPreProcess->Processing();

		pPreProcess->WriteToFile();

		pPreProcess->Flush();

		delete pPreProcess; pPreProcess = NULL;

		cout << "... 预处理结束 ... " << endl;

		cout << "... 批处理开始 ... " << endl;

		CRDODInterface* pRDOD = new CRDODInterface();

		pRDOD->ReadConfigFile(sGBODConfigFile, strBINDir);

		pRDOD->Init();

		pRDOD->API_ReadLEOSP3File(vLEOFiles);

		//string vUpdateImpulseFiles;
		//ts->filescannerimpulse(vUpdateImpulseFiles);

		pRDOD->ReadImpulseFile(vUpdateNominalImpulseFiles);

		string vUpdateNominalFiles;
		ts->filescannernominalorbit(vUpdateNominalFiles);

		pRDOD->ReadNominalOrbitFile(vUpdateNominalFiles);

		string sAfterPreProcessFile;
		ts->filescannerprep(sAfterPreProcessFile);
		//sAfterPreProcessFile = strBINDir + "/outputdata/Preprocess/DRO-KSST-1-PreProcess_536544000_536889600_60s.txt";//地面预处理程序 true
		pRDOD->API_ReadPreProcessFile(sAfterPreProcessFile);

		//定轨精度文件
		string vSorFFiles;
		ts->filescannersorf(vSorFFiles);

		if (vSorFFiles.size() == 0) {
			pRDOD->API_CreateSorFFile();
		}
		pRDOD->API_RecordSSTFile(vOldObsFiles);

		//扫描轨道数据文件
		//string vOrbFiles;
		//ts->filescannerorbit(vOrbFiles);

		//string vCopyOrbFiles;
		//ifstream ifs(vOrbFiles, ios::in);
		//char ch;

		pRDOD->API_ReadOrbitSP3File(vOrbFiles);
		//if (vOrbFiles.size() == 0) {
		//	ts->filescannercopyorbit(vCopyOrbFiles);
		//	pRDOD->API_ReadOrbitFile(vCopyOrbFiles);
		//}
		//else {
		//	ifs >> ch;
		//	if (ifs.eof()) {
		//		ts->filescannercopyorbit(vCopyOrbFiles);
		//		pRDOD->API_ReadOrbitFile(vCopyOrbFiles);
		//	}
		//	else {
		//		pRDOD->API_ReadOrbitFile(vOrbFiles);
		//	}
		//}

		////清楚历史扫描结果
		//ifs.close();
		//const char *name = vOrbFiles.c_str();
		//remove(name);

		//SOrbitStatesFormat sInitStates;
		pRDOD->API_GetOrbitResult(sInittime, sInitStates);

		sInitStates.bJ2K = true;

		//SInitStateError sInitStateError;
		sInitStateError.dSTD_P[0] = 0;//1000;
		sInitStateError.dSTD_P[1] = 0;//1000;
		sInitStateError.dSTD_P[2] = 0;//1000;
		sInitStateError.dSTD_V[0] = 0;//0.1;
		sInitStateError.dSTD_V[1] = 0;//0.1;
		sInitStateError.dSTD_V[2] = 0;//0.1;

		pRDOD->API_SetInitStates(sInittime, sInitStates, sInitStateError);

		//扫描定轨精度文件
		string vAccuFiles;
		ts->filescanneraccu(vAccuFiles);

		if (vAccuFiles.size() == 0) {
			pRDOD->API_CreateAccuFile();
		}

		//批处理
		pRDOD->Processing();

		std::cout << "批处理结束" << endl;

		pRDOD->WriteToFile();

		pRDOD->Flush();

		delete pRDOD; pRDOD = NULL;

		string strResultFile;
		ts->filescannerSP3result(strResultFile);

		//上传FTP
		//char cHostIP[] = "159.226.186.140";
		//char cUsername[] = "tcds";
		//char cPassword[] = "asdf123";
		//char cFTPFileName[] = "asdf123.sp3";
		//char *cPathName;
		//int len = strResultFile.length();
		//cPathName = (char *)malloc((len + 1) * sizeof(char));
		//strResultFile.copy(cPathName, len, 0);
		//FTP_Upload(cHostIP, cUsername, cPassword, cFTPFileName, cPathName);

		Thread::sleep(3000);
	}

	//system("pause");

	return 0;
}

bool TimeFormatTransform(const STimeFormat & in_sReceivetime, CTimeSystem & out_oCurObsTime)
{
	SCalDate sCurEpoch;
	sCurEpoch.iYear = in_sReceivetime.iYear;
	sCurEpoch.iMonth = in_sReceivetime.iMonth;
	sCurEpoch.iDay = in_sReceivetime.iDay;
	sCurEpoch.iHour = in_sReceivetime.iHour;
	sCurEpoch.iMin = in_sReceivetime.iMinute;
	sCurEpoch.dSec = in_sReceivetime.dSec;

	if (abs(sCurEpoch.dSec - 60.00000) < 1e-9)
	{
		sCurEpoch.dSec = int(sCurEpoch.dSec + 1e-9);

		SMJD smjd = CTimeSystem::CalDate2MJD(sCurEpoch);
		sCurEpoch = CTimeSystem::MJD2CalDate(smjd);
	}

	ETimeFrame eTimeFrame;
	string stmp = asString(in_sReceivetime.cTimeFrame);
	if (stmp.compare("GPS") == 0)
		eTimeFrame = TimeFrame_GPS;
	else if (stmp.compare("UTC") == 0)
		eTimeFrame = TimeFrame_UTC;
	else
	{
		cerr << "TimeSystem is wrong!" << endl;
		return false;
	}

	out_oCurObsTime.SetCalDate(sCurEpoch, eTimeFrame);

	return true;
}

bool TimeFormatTransform(const CTimeSystem & in_oCurObsTime, const ETimeFrame& in_eTimeFrame, STimeFormat & out_sReceivetime)
{
	SCalDate sDate = in_oCurObsTime.GetCalDate(in_eTimeFrame);
	out_sReceivetime.iYear = sDate.iYear;
	out_sReceivetime.iMonth = sDate.iMonth;
	out_sReceivetime.iDay = sDate.iDay;
	out_sReceivetime.iHour = sDate.iHour;
	out_sReceivetime.iMinute = sDate.iMin;
	out_sReceivetime.dSec = sDate.dSec;
	if (in_eTimeFrame == TimeFrame_GPS)
	{
		strcpy_s(out_sReceivetime.cTimeFrame, "GPS");
	}
	else if (in_eTimeFrame == TimeFrame_UTC)
	{
		strcpy_s(out_sReceivetime.cTimeFrame, "UTC");
	}
	else
	{
		cerr << "TimeSystem is wrong!" << endl;
		return false;
	}
	return true;
}