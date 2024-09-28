#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <wchar.h>

#include "../Shared/common.h"
#include "logging.h"
#include "kernelreader.h"
#include "output.h"
#include "cache.h"

#pragma comment (lib, "wintrust.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "crypt32.lib")


bool KernelReaderThreadStopFlag = FALSE;
HANDLE kernel_pipe = NULL;


void KernelReaderStopAll() {
    KernelReaderThreadStopFlag = TRUE;

    // Send some stuff so the ReadFile() in the reader thread returns
    DWORD dwWritten;
    BOOL success = WriteFile(kernel_pipe, "", 0, &dwWritten, NULL);
}


void CheckForNewProcess(wchar_t* line) {
    // Check if "observe:1" exists
    wchar_t* observe_str = wcsstr(line, L"observe:");
    if (!observe_str) {
        return;
    }
    
    // something like
    // "type:kernel;time:133711655617407173;callback:create_process;krn_pid:5564;pid:4240;name:\\Device\\HarddiskVolume2\\Windows\\System32\\notepad.exe;ppid:5564;parent_name:\\Device\\HarddiskVolume2\\Windows\\explorer.exe;observe:1"
    // find "observe:<int>" and pid from "pid:<int>"
    int observe_value = 0;
    swscanf_s(observe_str, L"observe:%d", &observe_value);
    if (observe_value == 1) {
        // Now extract the pid
        wchar_t* pid_str = wcsstr(line, L";pid:");
        if (pid_str) {
            int pid = 0;
            swscanf_s(pid_str, L";pid:%d", &pid);
            LOG_A(LOG_WARNING, "observe: %d, pid: %d\n", observe_value, pid);
            g_cache.getObject(pid); // FIXME this actually creates the process 
        }
    }
}


DWORD WINAPI KernelReaderProcessingThread(LPVOID param) {
    char buffer[DATA_BUFFER_SIZE] = { 0 };
    char* buf_ptr = buffer; // buf_ptr and rest_len are synchronized
    int rest_len = 0;
    DWORD bytesRead;

    while (!KernelReaderThreadStopFlag) {
        kernel_pipe = CreateNamedPipe(
            KERNEL_PIPE_NAME,                 // Pipe name to create
            PIPE_ACCESS_INBOUND,       // Whether the pipe is supposed to receive or send data (can be both)
            PIPE_TYPE_MESSAGE,        // Pipe mode (whether or not the pipe is waiting for data)
            PIPE_UNLIMITED_INSTANCES, // Maximum number of instances from 1 to PIPE_UNLIMITED_INSTANCES
            PIPE_BUFFER_SIZE,             // Number of bytes for output buffer
            PIPE_BUFFER_SIZE,             // Number of bytes for input buffer
            0,                        // Pipe timeout 
            NULL                      // Security attributes (anonymous connection or may be needs credentials. )
        );
        if (kernel_pipe == INVALID_HANDLE_VALUE) {
            LOG_A(LOG_ERROR, "KernelReader: Error creating named pipe: %ld", GetLastError());
            return 1;
        }

        LOG_A(LOG_INFO, "KernelReader: Waiting for client (Kernel Driver) to connect...");

        // Wait for the client to connect
        BOOL result = ConnectNamedPipe(kernel_pipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!result) {
            LOG_A(LOG_ERROR, "KernelReader: Error connecting the named pipe: %ld", GetLastError());
            CloseHandle(kernel_pipe);
            continue;
        }

        LOG_A(LOG_INFO, "KernelReader: Kernel connected");

        while (!KernelReaderThreadStopFlag) {
            // Read data from the pipe
            if (ReadFile(kernel_pipe, buf_ptr, sizeof(buffer) - rest_len, &bytesRead, NULL)) {
                int full_len = rest_len + bytesRead; // full len including the previous shit, if any
                wchar_t* p = (wchar_t*)buffer; // pointer to the string we will print. points to buffer
                // which always contains the beginning of a string
                int last_potential_str_start = 0;
                for (int i = 0; i < full_len; i += 2) { // 2-byte increments because wide string
                    if (buffer[i] == 0 && buffer[i + 1] == 0) { // check manually for \x00\x00
                        do_output(std::wstring(p));
                        CheckForNewProcess(p);

                        //wprintf(L"KRN: %s\n", p); // found \x00\x00, print the previous string
                        i += 2; // skip \x00\x00
                        last_potential_str_start = i; // remember the last zero byte we found
                        p = (wchar_t*)&buffer[i]; // init p with (potential) next string
                    }
                }
                if (last_potential_str_start == 0) {
                    LOG_A(LOG_ERROR, "KernelReader: No 0x00 0x00 byte found, errornous input?");
                }

                if (last_potential_str_start != full_len) {
                    // we didnt print until end of the buffer. so there's something left
                    rest_len = full_len - last_potential_str_start; // how much is left
                    memcpy(&buffer[0], &buffer[last_potential_str_start], rest_len); // copy that to the beginning of the buffer
                    buf_ptr = &buffer[rest_len]; // point read buffer to after our rest
                }
                else {
                    // printf till the end of the read data. 
                    // always reset
                    buf_ptr = &buffer[0];
                    rest_len = 0;
                }
            }
            else {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    LOG_A(LOG_INFO, "KernelReader: Kernel disconnected from pipe");
                    break;
                }
                else {
                    LOG_A(LOG_ERROR, "KernelReader: Error reading from kernel pipe: %ld", GetLastError());
                    break;
                }
            }
        }

        // Close the pipe
        CloseHandle(kernel_pipe);
    }
    LOG_A(LOG_INFO, "KernelReader: Thread Finished");

}


void InitializeKernelReader(std::vector<HANDLE>& threads) {
    const wchar_t* data = L"";
    LOG_A(LOG_INFO, "!KernelReader: Start thread");
    HANDLE thread = CreateThread(NULL, 0, KernelReaderProcessingThread, (LPVOID)data, 0, NULL);
    if (thread == NULL) {
        LOG_A(LOG_ERROR, "KernelReader: Failed to create thread for trace session logreader");
        return;
    }
    threads.push_back(thread);
}
