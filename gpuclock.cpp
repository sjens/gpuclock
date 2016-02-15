// Compile with "g++ -Wl,-subsystem,windows -static-libgcc -static-libstdc++ gpuclock.cpp -o gpuclock.exe"
// -static-libgcc -static-libstdc++ includes libraries.

#define WINVER 0x0500
#define KEY_ONE 0x02
#define KEY_ZERO 0x0B
#define KEY_SCR_LCK 0x46
#include <windows.h>
#include <Tlhelp32.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <Tchar.h>
#include <process.h>
using namespace std;
std::ifstream input("processes.txt");

// do something after 5 minutes of user inactivity
static const unsigned int idle_milliseconds = 60*5*1000;

void change_clock(int key_code) {
    // Create a generic keyboard event structure
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;

    // Press the "Ctrl" key
    ip.ki.wScan = 0x1D;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &ip, sizeof(INPUT));

    // Press the "Alt" key
    ip.ki.wScan = 0x38;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &ip, sizeof(INPUT));

    // Press the key supplied by the key code
    ip.ki.wScan = key_code;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &ip, sizeof(INPUT));

    // Release the key supplied by the key code
    ip.ki.wScan = key_code;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Alt" key
    ip.ki.wScan = 0x38;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Ctrl" key
    ip.ki.wScan = 0x1D;
    ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));
}

int list_proc(vector<string> proc) {
    PROCESSENTRY32 pEntry;
    HANDLE pSnap;

    // Process error handling
    pSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(pSnap == INVALID_HANDLE_VALUE)
        return 0;
    if(!Process32First(pSnap, &pEntry))
        return 0;

    // Iterate through open processes
    while(Process32Next(pSnap, &pEntry)) {
        for(vector<string>::const_iterator i = proc.begin(); i != proc.end(); i++) {
            string current_proc = pEntry.szExeFile;
            if(!current_proc.find(*i)) {
                //cout << current_proc + " Match \n";
                CloseHandle(pSnap);
                return 1;
            }
        }
    }

    CloseHandle(pSnap);
    return 2;
}

static long FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_POWERBROADCAST) {
        //Do something
        if(wParam == PBT_APMRESUMEAUTOMATIC) {
            Sleep(1000);
            change_clock(KEY_ZERO);
        }
        return TRUE;
    } else {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

void Thread(void* pParams) {
    WNDCLASS wc = {0};

    // Set up and register window class
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = _T("GPUClock");
    RegisterClass(&wc);
    HWND hWin = CreateWindow(_T("GPUClock"), _T(""), 0, 0, 0, 0, 0, NULL, NULL, NULL, 0);

    BOOL bRet;
    MSG msg;
    while((bRet = GetMessage( &msg, hWin, 0, 0 )) != 0) {
        if(bRet == -1) {
            // handle the error and possibly exit
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    vector<string> proc;
    int list_proc_return;
    int recent_clock;
    LASTINPUTINFO last_input;
    last_input.cbSize = sizeof(LASTINPUTINFO);
    int screen_off = 0;
    int current_tick;

    // Load text from file to vector
    for(string line; getline(input, line);) {
        proc.push_back(line);
    }

    _beginthread( Thread, 0, NULL );

    while(1) {
        list_proc_return = list_proc(proc);
        GetLastInputInfo(&last_input);
        current_tick = GetTickCount();
        if (list_proc_return == 1 && recent_clock != list_proc_return) {
            change_clock(KEY_ONE);
            //change_clock(KEY_SCR_LCK);
            recent_clock = 1;
            //cout << "OC ";
        } else if (list_proc_return == 2 && recent_clock != list_proc_return
            || screen_off == 1 && last_input.dwTime + 500 > current_tick) {
            change_clock(KEY_ZERO);
            //change_clock(KEY_SCR_LCK);
            recent_clock = 2;
            screen_off = 0;
            //cout << "Normal ";
        /*} else if (last_input.dwTime + idle_milliseconds < current_tick && screen_off == 0) {
            //cout << "Screen off ";
            screen_off = 1;*/
        }
        Sleep(10000);
    }
    return 0;
}
