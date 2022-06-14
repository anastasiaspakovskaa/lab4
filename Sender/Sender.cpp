#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <string>
//#include "D:\unik\C++\OS\lab4\Receiver\wstr.h"
std::wstring convertToWideString(const std::string& str) {
	const char* c_str = str.c_str();
	wchar_t* wstr = new wchar_t[str.length() + 1];
	mbstowcs(wstr, c_str, str.size() + 1);
	std::wstring wide(wstr);
	delete[] wstr;
	return wide;
}
//  cmd line: Sender.exe file startEvent readSem writeSem mutex finishAllEvent finishEvent
int main(int argc, char* argv[]) {

	if (argc != 8) {
		std::cerr << "wrong number of arguments\n";
		return 0;
	}
	std::cout << "start process in\n";

	HANDLE startEvent = OpenEvent(EVENT_ALL_ACCESS,
		FALSE,
		convertToWideString(argv[2]).c_str());

	HANDLE readSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, convertToWideString(argv[3]).c_str());
	HANDLE writeSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, convertToWideString(argv[4]).c_str());

	HANDLE mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, convertToWideString(argv[5]).c_str());

	HANDLE finishAllEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, convertToWideString(argv[6]).c_str());
	HANDLE finishEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, convertToWideString(argv[7]).c_str());

	if (!startEvent || !readSem || !writeSem || !mutex) {
		std::cerr << "cannot open objects with such names\n";
		return 0;
	}

	std::string filename = argv[1];
	std::ofstream fout{};

	SetEvent(startEvent);

	std::string message;
	HANDLE waitArr[] = { writeSem, finishAllEvent };

	while (true) {
		std::cout << "enter message(or f to finish this process): ";
		std::getline(std::cin, message);
		if (message == "f") {
			break;
		}
		if (WaitForMultipleObjects(2, waitArr, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
			break;
		}

		WaitForSingleObject(mutex, INFINITE);
		fout.open(filename, std::ofstream::binary | std::ofstream::app);
		fout << message << '\n';
		fout.close();

		ReleaseMutex(mutex);
		ReleaseSemaphore(readSem, 1, NULL);
		SetEvent(finishEvent);
	}
}