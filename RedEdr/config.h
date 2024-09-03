#pragma once
#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <tchar.h>


class Config {
public:
	LPCWSTR targetExeName = L"powershell.exe";
	BOOL log_unload = FALSE;
	std::wstring sessionName = L"RedEdrEtw";

	LPCWSTR driverName = L"RedEdr";
	LPCWSTR driverPath = L"C:\\RedEdr\\RedEdrDriver\\RedEdrDriver.sys";

	LPCSTR inject_dll_path = "C:\\RedEdr\\RedEdrDll.dll";
	bool do_etw = false;
	bool do_mplog = false;
	bool do_kernelcallback = false;
	bool do_dllinjection = false;
	bool debug_dllreader = false;
	bool web_output = false;
	//bool do_dllinjection_nochildren = false;

	bool etw_standard = true;
	bool etw_secaudit = true;
	bool etw_defender = false;

	bool do_udllinjection = false;
};

extern Config g_config;
