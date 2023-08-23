
#include <set>
#include "MainConfigFile.h"
#include "StringUtils.h"
#include "CommonOperation.h"

#include "tinystr.h"
#include "tinyxml.h"

using namespace StringUtils;

bool ReadMAINConfigFile(const string& in_sXMLFile,
	const string& in_sRelativePathBIN,
	SMainConfig& out_sMAINConfig)
{
	TiXmlDocument *pDocument = new TiXmlDocument(in_sXMLFile.c_str());
	if (!pDocument->LoadFile())
	{
		cerr << "Config File Error: maybe XML file " + in_sXMLFile + "is not exist!" << endl;
		return false;
	}

	/*
	**获得根元素
	*/
	TiXmlElement *pRootElement = pDocument->FirstChildElement("MAIN");
	if (asString(pRootElement->Value()) != "MAIN")
	{
		cerr << "Config File Error: maybe MAIN element is not exist!" << endl;
		assert(false);
		return false;
	}

	set<string> setFilePath;

	//节点：ObservationDate
	TiXmlElement *pXmlElement = pRootElement->FirstChildElement("ObservationDate");
	if (!pXmlElement)
	{
		cerr << "Config File Error: ObservationDate element is error!" << endl;
		return false;
	}
	out_sMAINConfig.sObservationDate = pXmlElement->FirstChild()->Value();

	//节点：ConfigFilePath
	pXmlElement = pRootElement->FirstChildElement("ConfigFilePath");
	if (!pXmlElement)
	{
		cerr << "Config File Error: ConfigFilePath element is error!" << endl;
		return false;
	}
	out_sMAINConfig.sConfigFilePath = pXmlElement->FirstChild()->Value();
	StringUtils::change(out_sMAINConfig.sConfigFilePath, "RelativePathBIN", in_sRelativePathBIN);
	setFilePath.insert(out_sMAINConfig.sConfigFilePath);

	//节点：LogFile
	pXmlElement = pRootElement->FirstChildElement("LogFile");
	if (!pXmlElement)
	{
		cerr << "Config File Error: LogFile element is error!" << endl;
		return false;
	}
	out_sMAINConfig.sLogFileName = pXmlElement->FirstChild()->Value();
	StringUtils::change(out_sMAINConfig.sLogFileName, "RelativePathBIN", in_sRelativePathBIN);
	setFilePath.insert(out_sMAINConfig.sLogFileName);

	/************************************************************************/
	/*    过程控制部分                                                      */
	/************************************************************************/

	//节点：GBODSwitch
	pXmlElement = pRootElement->FirstChildElement("GBODSwitch");
	if (!pXmlElement)
	{
		cerr << "Config File Error: GBODSwitch element is error!" << endl;
		return false;
	}
	string stmp = pXmlElement->FirstChild()->Value();
	if (lowerCase(stmp).compare("true") == 0)
		out_sMAINConfig.sProcessControl.GBODSwitch = true;
	else
		out_sMAINConfig.sProcessControl.GBODSwitch = false;

	CommonOperation::CreatFolders(setFilePath);

	return true;
}

bool SetDateInfo(const string& in_sConfigFile, const SMainConfig& in_sMAINConfig/*, const string& in_sRootElements*/)
{
	TiXmlDocument* pDocument = new TiXmlDocument();
	pDocument->LoadFile(in_sConfigFile.c_str());

	TiXmlElement *pRootElement = pDocument->FirstChildElement();
	if (!pRootElement)
	{
		//cerr << "Config File Error: maybe" +/*in_sRootElements*/+ " is not exist!" << endl;
		//assert(false);
		//return false;
	}

	if (pRootElement)
	{
		TiXmlElement *pElement = pRootElement->FirstChildElement("Configure_00");
		if (!pElement)
		{
			cerr << "Config File Error: maybe child element Configure_00 is not exist!" << endl;
			assert(false);
			return false;
		}

		string sName = asString(pElement->FirstChildElement("ConfigName")->FirstChild()->Value());
		if (sName != "ObsDate")
		{
			cerr << "Config File Error: maybe child element ConfigName is not exist!" << endl;
			assert(false);
			return false;
		}

		TiXmlNode* pNode = pElement->FirstChildElement("ConfigFile")->FirstChild();
		if (!pNode)
		{
			cerr << "Config File Error: maybe child element ConfigFile is not exist!" << endl;
			assert(false);
			return false;
		}

		pNode->SetValue(in_sMAINConfig.sObservationDate.c_str());
	}

	pDocument->SaveFile();

	return true;
}