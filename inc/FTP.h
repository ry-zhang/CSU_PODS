#pragma once
#ifndef FTP_TEST_H
#define FTP_TEST_H

#include <stdio.h>
#include <Windows.h>
#include <WinInet.h>
#pragma comment(lib, "WinInet.lib")

BOOL Ftp_SaveToFile(char *pszFileName, BYTE *pData, DWORD dwDataSize)
{
	HANDLE hFile = ::CreateFileA(pszFileName, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
		return FALSE;

	DWORD dwRet = 0;
	WriteFile(hFile, pData, dwDataSize, &dwRet, NULL);
	CloseHandle(hFile);
	return TRUE;
}

BOOL FTP_Download(char *szHostName, char *szUserName, char *szPassword, char *szUrlPath, char *SavePath)
{
	HINTERNET hInternet, hConnect, hFTPFile = NULL;
	BYTE *pDownloadData = NULL;
	DWORD dwDownloadDataSize = 0;
	BYTE *pBuf = NULL;
	DWORD dwBytesReturn = 0;
	DWORD dwOffset = 0;
	BOOL bRet = FALSE;

	hInternet = InternetOpen("WinInet Ftp", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	hConnect = InternetConnect(hInternet, szHostName, INTERNET_INVALID_PORT_NUMBER,
		szUserName, szPassword, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

	hFTPFile = FtpOpenFile(hConnect, szUrlPath, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, NULL);
	dwDownloadDataSize = FtpGetFileSize(hFTPFile, NULL);
	pDownloadData = new BYTE[dwDownloadDataSize];

	RtlZeroMemory(pDownloadData, dwDownloadDataSize);
	pBuf = new BYTE[4096];
	RtlZeroMemory(pBuf, 4096);
	do
	{
		bRet = InternetReadFile(hFTPFile, pBuf, 4096, &dwBytesReturn);
		if (FALSE == bRet)
			break;
		RtlCopyMemory((pDownloadData + dwOffset), pBuf, dwBytesReturn);
		dwOffset = dwOffset + dwBytesReturn;
	} while (dwDownloadDataSize > dwOffset);

	Ftp_SaveToFile(SavePath, pDownloadData, dwDownloadDataSize);
	delete[]pDownloadData;
	pDownloadData = NULL;
	return TRUE;
}


BOOL FTP_Upload(char *szHostName, char *szUserName, char *szPassword, char *szUrlPath, char *FilePath)
{
	HINTERNET hInternet, hConnect, hFTPFile = NULL;
	DWORD dwBytesReturn = 0;
	DWORD UploadDataSize = 0;
	BYTE *pUploadData = NULL;
	DWORD dwRet, bRet = 0;

	hInternet = ::InternetOpen("WinInet Ftp Upload V1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	hConnect = ::InternetConnect(hInternet, szHostName, INTERNET_INVALID_PORT_NUMBER, szUserName, szPassword,
		INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
	hFTPFile = ::FtpOpenFile(hConnect, szUrlPath, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, NULL);
	HANDLE hFile = ::CreateFileA(FilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ |
		FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		cout << "打开文件失败" << endl;
		return FALSE;
	}
	//cout << "打开文件成功" << endl;

	UploadDataSize = ::GetFileSize(hFile, NULL);
	pUploadData = new BYTE[UploadDataSize];
	::ReadFile(hFile, pUploadData, UploadDataSize, &dwRet, NULL);
	UploadDataSize = dwRet;

	bRet = ::InternetWriteFile(hFTPFile, pUploadData, UploadDataSize, &dwBytesReturn);
	
	InternetCloseHandle(hInternet); //关闭FTP
	if (FALSE == bRet)
	{
		cout << "上传文件失败" << endl;
		CloseHandle(hFile);
		delete[]pUploadData;
		return FALSE;
	}
	CloseHandle(hFile); //关闭文件，
	delete[]pUploadData;
	return TRUE;
}


//BOOL FTP_Upload(char *szHostName, char *szUserName, char *szPassword, const char *szUrlPath, const char *FilePath)
//{
//	HINTERNET hInternet, hConnect, hFTPFile = NULL;
//	DWORD dwBytesReturn = 0;
//	DWORD UploadDataSize = 0;
//	BYTE *pUploadData = NULL;
//	DWORD dwRet, bRet = 0;
//
//	hInternet = ::InternetOpen("WinInet Ftp Upload V1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
//	hConnect = ::InternetConnect(hInternet, szHostName, INTERNET_INVALID_PORT_NUMBER, szUserName, szPassword,
//		INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);
//	hFTPFile = ::FtpOpenFile(hConnect, szUrlPath, GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, NULL);
//	HANDLE hFile = ::CreateFile(FilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ |
//		FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
//	if (INVALID_HANDLE_VALUE == hFile)
//		return FALSE;
//
//	UploadDataSize = ::GetFileSize(hFile, NULL);
//	pUploadData = new BYTE[UploadDataSize];
//	::ReadFile(hFile, pUploadData, UploadDataSize, &dwRet, NULL);
//	UploadDataSize = dwRet;
//
//	bRet = ::InternetWriteFile(hFTPFile, pUploadData, UploadDataSize, &dwBytesReturn);
//	if (FALSE == bRet)
//	{
//		delete[]pUploadData;
//		return FALSE;
//	}
//	delete[]pUploadData;
//	return TRUE;
//}

#endif