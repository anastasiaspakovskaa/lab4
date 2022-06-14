#include <iostream>
#include <fstream>
#include <Windows.h>
#include <vector>
#include <string>
#include <list>
#include"wstr.h"

void replace(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty())
        return;
    std::string::size_type start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
int main(int argc, char* argv[]) {
    std::cout << "Enter file name: ";
    std::string filename;
    std::cin >> filename;

    std::cout << "Enter number of records: ";
    int recordsNumber;
    std::cin >> recordsNumber;

    std::ofstream file(filename, std::ofstream::binary);
    file.close();

    std::cout << "Enter number of processes: ";
    int processesNumber;
    std::cin >> processesNumber;
    if (processesNumber < 1) {
        std::cerr << "wrong number of threads\n";
        return 0;
    }

    std::vector<HANDLE> startEvents(processesNumber);
    std::vector<HANDLE> finishEvents(processesNumber);

    std::string finishAllEventName = "finishAll";

    HANDLE finishAll = CreateEvent(NULL,
        TRUE, 
        FALSE,
        convertToWideString(finishAllEventName).c_str());

    std::vector<STARTUPINFO> si(processesNumber);
    std::vector<PROCESS_INFORMATION> procInfo(processesNumber);

    std::string startEventName;
    std::string finishEventName;
    std::string cmdLine;

    const std::string readSemaphore = "readSemaphore";
    const std::string writeSemaphore = "writeSemaphore";
    const std::string mutexName = "mutex";

    HANDLE readSem = CreateSemaphore(NULL, 0, recordsNumber,
        convertToWideString(readSemaphore).c_str());
    
    HANDLE writeSem = CreateSemaphore(NULL, 
        recordsNumber, 
        recordsNumber, 
        convertToWideString(writeSemaphore).c_str());
    
    HANDLE mutex = CreateMutex(NULL,
        FALSE,
        convertToWideString(mutexName).c_str());


    for (int i = 0; i < processesNumber; ++i) {

        startEventName = "startEvent" + std::to_string(i);
        startEvents[i] = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            convertToWideString(startEventName).c_str()
        );


        finishEventName = "finishEvent" + std::to_string(i);

        finishEvents[i] = CreateEvent(
            NULL,
            FALSE,
            FALSE,
            convertToWideString(finishEventName).c_str()
        );
       
        ZeroMemory(&si[i], sizeof(STARTUPINFO));
        si[i].cb = sizeof(STARTUPINFO);

        std::string path = argv[0];
        replace(path, "Receiver.exe", "");

        cmdLine = path + "Sender.exe " +
            filename + " " +
            startEventName + " " +
            readSemaphore + " " +
            writeSemaphore + " " +
            mutexName + " " +
            finishAllEventName + " " +
            finishEventName;

        if (!CreateProcess(
            NULL, 
            (LPWSTR)convertToWideString(cmdLine).c_str(),
            NULL, 
            NULL, 
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL, NULL,
            &si[i],
            &procInfo[i])) {

            std::cerr << "error creating process";
            std::cerr << GetLastError();
            return 0;
        }
        else {
            std::cout << "start process ext\n";
        }
    }

    WaitForMultipleObjects(processesNumber, 
        &startEvents[0],
        TRUE,
        INFINITE);

    std::cout << "Enter r to read or any other string to exit\n";
    std::string input = "r";
    std::string message;
    std::ifstream fin{};
    std::string cachedFile;

    while (true) {
        std::cout << "enter next action: ";
        std::cin >> input;
        if (input != "r") {
            break;
        }

        WaitForSingleObject(readSem, INFINITE);
        WaitForSingleObject(mutex, INFINITE);

        fin.open(filename, std::ifstream::binary);
        std::getline(fin, message);
        std::cout << "new message: " << message << '\n';

        while (std::getline(fin, message)) {
            cachedFile += message + '\n';
        }
        fin.close();
        file.open(filename, std::ofstream::binary | std::ofstream::trunc);
        file << cachedFile;
        file.close();
        ReleaseMutex(mutex);
        ReleaseSemaphore(writeSem, 1, NULL);
        cachedFile = "";
    }

    SetEvent(finishAll);
    WaitForMultipleObjects(processesNumber, &finishEvents[0], TRUE, INFINITE);

    CloseHandle(readSem);
    CloseHandle(writeSem);
    CloseHandle(mutex);
    CloseHandle(finishAll);

    for (int i = 0; i < processesNumber; ++i) {
        CloseHandle(finishEvents[i]);
        CloseHandle(startEvents[i]);
        CloseHandle(procInfo[i].hThread);
        CloseHandle(procInfo[i].hProcess);
    }
}