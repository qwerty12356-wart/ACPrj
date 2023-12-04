
#ifndef UNICODE
#define UNICDE
#endif


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif // !_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <thread>
#include <Uxtheme.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <algorithm> 

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Uxtheme.lib")
#define CHECK_BIT(var,pos) ((var) & (1<<(pos))) //thanks stackoverflow
#define CHECK_BYTE(var,pos) ((var >> (24-pos)) & (UINT)0xFF)
#define ID_BUTTONGS 1001
#define startpos 10
#define ID_BUTTONGETHOTKEY 1002
#define ID_EDITINTERHOUR 2001
#define ID_EDITINTERMIN 2002
#define ID_EDITINTERSEC 2003
#define ID_EDITINTERMILISEC 2004
#define ID_EDITDURMIN 2005
#define ID_EDITDURSEC 2006
#define ID_EDITDURMILISEC 2007
#define ID_DURRADIOBTTN1 2008
#define ID_DURRADIOBTTN2 2009
#define ID_DURCOMBOBOX 2010
#define ID_REPTUNTSTPRDIO 2011
#define ID_REPTFORRDO 2012
#define ID_SETHOTKEYBOX 2013
#define ID_OPTIONGRPBOX 2014
#define ID_REPEATFORUPDOWNBUDDY 2015
#define ID_RADIOBUTTONCURRENTMOUSEPOS 2016
#define ID_RADIOBUTTONCUSTOMMOUSEPOS 2017
#define ID_EDITXPOS 2018
#define ID_EDITYPOS 2019
#define ID_STARTBUTTON 2020
#define ID_STOPBUTTON 2021



struct ACSettings
{
    uint64_t clickdelay = 0;
    uint64_t holddur = 0;
    bool isholdindef = false;
    LONG xcord = 0;
    LONG ycord = 0;
    bool isexactpos = false;
    unsigned char subkeycode = NULL;
    unsigned char keycode = VK_F6;
    unsigned char mousebutton = 0;
    UINT repeatcount = 0;
    bool infrepeat = true;
};


static std::wstring GetAppFolder()
{
    wchar_t szFileName[MAX_PATH];
    DWORD len = GetModuleFileName(NULL, szFileName, MAX_PATH);
    std::wstring result(szFileName, len);
    std::wstring::size_type pos = result.find_last_of(L"\\/");
    result.resize(pos + 1);
    return result;
}


bool IsFileExist(LPCWSTR path)
{
    if (path == NULL)
    {
        return false;
    }
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}



#define GETMASK(index, size) ((((size_t)1 << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITETO(data, index, size, value) ((data) = (((data) & (~GETMASK((index), (size)))) | (((value) << (index)) & (GETMASK((index), (size))))))


bool isCapturingHotkey = false;
bool isTracking;
bool autoclickerenabled = false;
bool captureuserinput = true;
bool masterswitch = false;
std::mutex mutelock;
std::condition_variable convar;


HHOOK testHook = nullptr;
HWND hwndTT = nullptr;
HWND wndhwnd = nullptr;
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);
TTTOOLINFOW ti = { 0 };
void CreateMyTooltip(HWND);
HINSTANCE hinst;
HFONT hf = nullptr;
bool confirm = false;
void startBTracking(HWND hwnd);
UINT hotkeyarr[3] = {0,VK_F6,0};
UINT oldhotkeyarr[3] = { 0,0,0};
HHOOK kbhook = NULL; //redundant


std::wstring AutoExpandEnviromentVariable(std::wstring relpath)
{

    std::wstring tempbuff;


    DWORD strlen = ExpandEnvironmentStrings(relpath.c_str(), NULL, 0);
    tempbuff.resize(strlen);
    ExpandEnvironmentStrings(relpath.c_str(), &tempbuff[0], strlen);
    return tempbuff;
}

int GetKeycodetoArray(UINT vkkey, UINT scancode)
{
    for (unsigned int a = 0; a < sizeof(hotkeyarr) / sizeof(hotkeyarr[0]); a = a + 1)
    {
        if (vkkey == VK_LMENU || vkkey == VK_RMENU || vkkey == VK_LCONTROL || vkkey == VK_RSHIFT || vkkey == VK_RCONTROL || vkkey == VK_LSHIFT || vkkey == VK_LWIN || vkkey == VK_RWIN && a == 0)
        {
            hotkeyarr[a] = vkkey;
            return 1;
        }
        if (a == 1)
        {
            hotkeyarr[a] = vkkey;
            hotkeyarr[a + 1] = scancode; //redundant
            //std::cout << scancode << '\n';
            return 2;
            
        }
    }
    return 0;
}
void getuserinput(); // run in seprate thread
void autoclickfunc(int index, uint64_t timeinmilisecond, uint64_t holdinmilisec, int dx, int dy, bool isAbsolute, bool infinite, UINT repeat); //run in seprate thread
LRESULT CALLBACK Keyboardhook(int code, WPARAM wp, LPARAM lp)
{
    if (code < 0)
    {
        return CallNextHookEx(kbhook, code, wp, lp);
    }
    if (isCapturingHotkey)
    {
        UINT kcode = 0;
        if (CHECK_BIT(wp, 23))
        {
            switch (wp)
            {
            case VK_SHIFT:
            {
                kcode = VK_RSHIFT;
                break;
            }
            case VK_MENU:
            {
                kcode = VK_RMENU;
                break;
            }
            case VK_CONTROL:
            {

                kcode = VK_RCONTROL;
                break;
            }
            default:
            {
                kcode = wp;
                break;
            }
            break;
            }
        }
        else
        {
            switch (wp)
            {
            case VK_SHIFT:
            {
                kcode = VK_LSHIFT;
                break;
            }
            case VK_MENU:
            {
                kcode = VK_LMENU;
                break;
            }
            case VK_CONTROL:
            {
                kcode = VK_LCONTROL;
                break;
            }
            default:
            {
                kcode = wp;
                break;
            }
            break;
            }

        }
        if (GetKeycodetoArray(kcode, CHECK_BYTE(lp, 15)) == 2)
        {

            confirm = true;
            SendMessage(wndhwnd, WM_COMMAND, ID_BUTTONGETHOTKEY, FALSE);
        }

    }
    return CallNextHookEx(NULL, code, wp, lp);

}
std::wstring ConvertKeyCodeToString(uint8_t keycode)
{

    std::wstring result;
    switch (keycode)
    {
    case VK_LEFT:
    {
        result = L"LEFT";
        break;
    }
    case VK_UP:
    {
        result = L"UP";
        break;
    }
    case VK_RIGHT:
    {
        result = L"RIGHT";
        break;
    }
    case VK_DOWN:
    {
            result = L"DOWN";
            break;
    }
    case VK_LCONTROL:
    {
        result = L"LCTRL";
        break;
    }
    case VK_LSHIFT:
    {
        result = L"LSHIFT";
        break;
    }
    case VK_RSHIFT:
    {
        result = L"RSHIFT";
        break;
    }
    case VK_RCONTROL:
    {
        result = L"RCTRL";
        break;
    }
    case VK_RMENU:
    {
        result = L"RALT";
        break;
    }
    case VK_LMENU:
    {
        result = L"LALT";
        break;
    }
    case VK_LWIN: 
    {
        result = L"LWND";
        break;
    }
    case VK_RWIN:
    {
        result = L"RWIND";
        break;
    }
    case VK_APPS:
    {
        result = L"APPS";
        break;
    }
    case VK_PRIOR:
    {
        result = L"PRIOR";
        break;
    }
    case VK_NEXT:
    {
        result = L"NEXT";
        break;
    }
    case VK_END:
    {
        result = L"END";
        break;
    }
    case VK_HOME:
    {
        result = L"HOME";
        break;
    }
    case VK_INSERT:
    {
        result = L"INSERT";
        break;
    }
    case VK_DELETE:
    {
        result = L"DELETE";
        break;
    }
    case VK_DIVIDE:
    {
        result = L"DIVIDE";
        break;
    }
    case VK_NUMLOCK:
    {
        result = L"NUMLOCK";
        break;
    }
    default:
    {
        HKL kbly = LoadKeyboardLayout(L"", KLF_ACTIVATE);
        
        wchar_t bufferstring[256] = L"";
        UINT scancd = MapVirtualKeyEx((UINT)keycode, 0, kbly);
        
        GetKeyNameText(scancd << 16, bufferstring, sizeof(bufferstring) / 2);
        result = bufferstring;
        break;
    }
    }
    std::transform(result.begin(), result.end(), result.begin(), std::toupper);
    return result;

}

bool GetSettings(std::wstring settingpath, ACSettings* acsptr)
{
    


    std::wstring buff = AutoExpandEnviromentVariable(settingpath);
    ACSettings acs;
    acsptr->clickdelay = GetPrivateProfileInt(L"ACSettings", L"ClickDelay", 100, buff.c_str());
    acsptr->holddur = GetPrivateProfileInt(L"ACSettings", L"HoldDuration", 10, buff.c_str());
    acsptr->infrepeat = GetPrivateProfileInt(L"ACSettings", L"InfiniteRepeat", 1, buff.c_str());
    acsptr->isexactpos = GetPrivateProfileInt(L"ACSettings", L"ExactPosition", 0, buff.c_str());
    acsptr->xcord = GetPrivateProfileInt(L"ACSettings", L"Xcoordinate", 0, buff.c_str());
    acsptr->ycord = GetPrivateProfileInt(L"ACSettings", L"YCoordinate", 0, buff.c_str());
    acsptr->isholdindef = GetPrivateProfileInt(L"ACSettings", L"HoldIndefinetly", 0, buff.c_str());
    acsptr->subkeycode = GetPrivateProfileInt(L"ACSettings", L"subkeycode", 0, buff.c_str());
    acsptr->keycode = GetPrivateProfileInt(L"ACSettings", L"keycode", VK_F6, buff.c_str());
    acsptr->repeatcount = GetPrivateProfileInt(L"ACSettings", L"RepeatCount", 1, buff.c_str());
    acsptr->mousebutton = GetPrivateProfileInt(L"ACSettings", L"MouseButton", 0, buff.c_str());
    return 1;
}

void WriteSettings(std::wstring path, ACSettings* acsptr)
{
    HANDLE hfile = nullptr;
    std::wstring buff = AutoExpandEnviromentVariable(path);
    if (!IsFileExist(path.c_str()))
    {
        hfile = CreateFile(buff.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        CloseHandle(hfile);
        int awadw = GetLastError();
    }
    WritePrivateProfileString(L"ACSettings", L"ClickDelay", std::to_wstring(acsptr->clickdelay).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"HoldDuration", std::to_wstring(acsptr->holddur).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"InfiniteRepeat", std::to_wstring(acsptr->infrepeat).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"ExactPosition", std::to_wstring(acsptr->isexactpos).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"Xcoordinate", std::to_wstring(acsptr->xcord).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"YCoordinate", std::to_wstring(acsptr->ycord).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"HoldIndefinetly", std::to_wstring(acsptr->isholdindef).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"subkeycode", std::to_wstring(acsptr->subkeycode).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"keycode", std::to_wstring(acsptr->keycode).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"RepeatCount", std::to_wstring(acsptr->repeatcount).c_str(), buff.c_str());
    WritePrivateProfileString(L"ACSettings", L"MouseButton", std::to_wstring(acsptr->mousebutton).c_str(), buff.c_str());
    int aa = GetLastError();

}


ACSettings acstt;

int WINAPI wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,_In_ int nCmdShow)
{
    
    /*
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    */
    
    SetEnvironmentVariable(L"curdir", GetAppFolder().c_str());
    masterswitch = true;
    std::thread(getuserinput).detach();
    
    
    MSG  msg;
    WNDCLASS wc = { 0 };
    wc.lpszClassName = L"ACPrj";
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpfnWndProc = WndProc2;
    wc.hCursor = LoadCursor(0, IDC_ARROW);

    RegisterClass(&wc);
    HWND mainwnd = CreateWindowEx(WS_EX_TOPMOST , wc.lpszClassName, L"Stopped - ACprj",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        100, 100, 501, 508, NULL, NULL,hInstance, 0);
    
    hinst = hInstance;
    
    InitCommonControls();
    while (GetMessage(&msg, NULL, 0, 0)) {
        
        if (!IsDialogMessage(mainwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    masterswitch = false;
    
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!hf)
    {
        hf = CreateFontW(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Helvetica");
    }
    wndhwnd = hwnd;
    
    switch (msg)
    {
    
    
    case WM_CREATE:
    {
        HWND intervalGroupBox = CreateWindowW(L"Button", L"Click inerval", BS_GROUPBOX | WS_CHILD | WS_VISIBLE , 10, 10, 466, 53, hwnd, NULL, hinst, NULL);
        SendMessageW(intervalGroupBox, WM_SETFONT, (WPARAM)hf, TRUE);

        HWND EditHour = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 20, 33, 49, 20, hwnd, (HMENU)ID_EDITINTERHOUR, hinst, NULL);
        SetWindowTheme(EditHour, L"Explorer", NULL);
        SendMessageW(EditHour, WM_SETFONT, (WPARAM)hf, TRUE);

        HWND TextHour = CreateWindowW(L"Static", L"Hours", WS_CHILD | WS_VISIBLE, 64, 25, 34, 16, intervalGroupBox, NULL, hinst, NULL);
        SendMessageW(TextHour, WM_SETFONT, (WPARAM)hf, TRUE);

        HWND EditMinute = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 118, 33, 49, 20, hwnd, (HMENU)ID_EDITINTERMIN, hinst, NULL);
        SetWindowTheme(EditMinute, L"Explorer", NULL);
        SendMessageW(EditMinute, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND TextMinute = CreateWindowW(L"Static", L"Minutes", WS_CHILD | WS_VISIBLE, 162, 25, 46, 16, intervalGroupBox, NULL, hinst, NULL);
        SendMessage(TextMinute, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND EditSecond = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 223, 33, 49, 20, hwnd, (HMENU)ID_EDITINTERSEC, hinst, NULL);
        SetWindowTheme(EditSecond, L"Explorer", NULL);
        SendMessageW(EditSecond, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND TextSecond = CreateWindow(L"Static", L"Seconds", WS_CHILD | WS_VISIBLE, 272, 25, 51, 16, intervalGroupBox, NULL, hinst, NULL);
        SendMessageW(TextSecond, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND Editmilisecond = CreateWindowW(L"Edit", L"100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 343, 33, 49, 20, hwnd, (HMENU)ID_EDITINTERMILISEC, hinst, NULL);
        SetWindowTheme(Editmilisecond, L"Explorer", NULL);

        SendMessageW(Editmilisecond, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND Textmilisecond  = CreateWindowW(L"Static", L"Miliseconds", WS_CHILD | WS_VISIBLE, 387, 25, 69, 16, intervalGroupBox, NULL, hinst, NULL);
        SendMessageW(Textmilisecond, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddurationGroupbox = CreateWindowW(L"Button", L"Hold Duration", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 73, 466, 76, hwnd, NULL, hinst, NULL);
        SendMessageW(holddurationGroupbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddurradiobttn1 = CreateWindowW(L"Button", L"Hold for:", BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE|WS_GROUP | WS_TABSTOP, 20,73+ 24, 63, 16, hwnd, (HMENU)ID_DURRADIOBTTN1, hinst, NULL);
        SendMessageW(holddurradiobttn1, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddurradiobttn2  = CreateWindowW(L"Button", L"Hold indefinetly", BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 20, 73+50, 105, 16, hwnd, (HMENU)ID_DURRADIOBTTN2, hinst, NULL);
        SendMessageW(holddurradiobttn2, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddureditminbox = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 88, 73+23, 49, 20, hwnd, (HMENU)ID_EDITDURMIN, hinst, NULL);
        SendMessageW(holddureditminbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddurmintex = CreateWindowW(L"Static", L"Minutes", WS_CHILD | WS_VISIBLE, 132, 25, 46, 16, holddurationGroupbox, NULL, hinst, NULL);
        SendMessageW(holddurmintex, WM_SETFONT, (WPARAM)hf, TRUE);
        SetWindowTheme(holddurradiobttn1, L"Explorer", NULL);
        SetWindowTheme(holddurradiobttn2, L"Explorer", NULL);
        SetWindowTheme(holddureditminbox, L"Explorer", NULL);
        HWND holddureditsecbox = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 198, 73+23, 49, 20, hwnd, (HMENU)ID_EDITDURSEC, hinst, NULL);
        SendMessageW(holddurradiobttn1, BM_SETCHECK, BST_CHECKED, TRUE);
        SendMessageW(holddureditsecbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddursectex = CreateWindowW(L"Static", L"Seconds", WS_CHILD | WS_VISIBLE, 242, 25, 51, 16, holddurationGroupbox, NULL, hinst, NULL);
        SendMessageW(holddursectex, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddureditmilisecbox = CreateWindowW(L"Edit", L"100", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 313,73+ 23, 49, 20, hwnd, (HMENU)ID_EDITDURMILISEC, hinst, NULL);
        SendMessageW(holddureditmilisecbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND holddurmilisectex = CreateWindowW(L"Static", L"Miliseconds", WS_CHILD | WS_VISIBLE, 357, 25, 69, 16, holddurationGroupbox, NULL, hinst, NULL);
        SendMessageW(holddurmilisectex, WM_SETFONT, (WPARAM)hf, TRUE);
        SetWindowTheme(holddureditsecbox, L"Explorer", NULL);
        SetWindowTheme(holddureditmilisecbox, L"Explorer", NULL);
        HWND Optionsgroupbox = CreateWindowW(L"Button", L"Options", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 159, 174, 150, hwnd, (HMENU)ID_OPTIONGRPBOX, hinst, NULL);
        SendMessage(Optionsgroupbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND mousebutton = CreateWindowW(L"Static", L"Mouse Button:", WS_CHILD | WS_VISIBLE, 10, 27, 85, 16, Optionsgroupbox, NULL, hinst, NULL);
        SendMessageW(mousebutton, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND combobox = CreateWindowW(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 110, 159+23, 64, 16, hwnd, (HMENU)ID_DURCOMBOBOX, hinst, NULL);
        SendMessageW(combobox, WM_SETFONT, (WPARAM)hf, TRUE);
        SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)L"Left");
        SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)L"Right");
        SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)L"Middle");
        SendMessageW(combobox, CB_SETCURSEL, NULL, NULL);
        
        HWND reptfrdo = CreateWindowW(L"Button", L"Repeat for:", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 20, 159+54, 78, 16, hwnd, (HMENU)ID_REPTFORRDO, hinst, NULL);
        SendMessageW(reptfrdo, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND rpptuntstpdrdo = CreateWindowW(L"Button", L"Repeat until stopped", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP, 20,159+ 75,136, 16, hwnd, (HMENU)ID_REPTUNTSTPRDIO, hinst, NULL);
        SendMessageW(rpptuntstpdrdo, WM_SETFONT, (WPARAM)hf, TRUE);
        SendMessageW(rpptuntstpdrdo, BM_SETCHECK, BST_CHECKED, TRUE);
        HWND reptfrupdownbuddy = CreateWindowW(L"Edit", L"1", ES_NUMBER | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 103, 159+52, 64, 20, hwnd, (HMENU)ID_REPEATFORUPDOWNBUDDY, hinst, NULL);
        HWND reptfrupdownbox = CreateWindowW(UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS | UDS_SETBUDDYINT | UDS_HOTTRACK , 0, 0, 0, 0, hwnd, NULL, hinst, NULL);
        SendMessageW(reptfrupdownbox, UDM_SETRANGE, 0, MAKELPARAM(16595, 1));
        SendMessageW(reptfrupdownbuddy, WM_SETFONT, (WPARAM)hf, TRUE);
        SendMessageW(reptfrupdownbox, UDM_SETBUDDY, (WPARAM)reptfrupdownbuddy, 0);
        SetWindowTheme(reptfrupdownbox, L"Explorer", NULL);
        HWND clickposGroupbox = CreateWindowW(L"Button", L"Click position", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 194, 159, 282, 105, hwnd, NULL, hinst, NULL);
        SendMessageW(clickposGroupbox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND SetHotkey = CreateWindowW(L"Button", L"Get hotkey", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 20, 265, 70, 20, hwnd,(HMENU) ID_BUTTONGETHOTKEY, hinst, NULL);
        SendMessageW(SetHotkey, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND sethotkeybox = CreateWindowW(L"Edit", L"F6", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY , 85, 106, 82, 20, Optionsgroupbox, (HMENU)ID_SETHOTKEYBOX, hinst, NULL);
        SendMessageW(sethotkeybox, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND RadiobuttonCustomMousePos = CreateWindowW(L"Button", L"", WS_GROUP | WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_TABSTOP,194+ 10, 159+23, 16, 20, hwnd, (HMENU)ID_RADIOBUTTONCUSTOMMOUSEPOS, hinst, NULL);
        SendMessageW(RadiobuttonCustomMousePos, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND GetScreenCordsButton = CreateWindowW(L"Button", L"Get screen cords", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 204+16 + 5,156+23, 105, 25, hwnd, (HMENU)ID_BUTTONGS, hinst, NULL);
        SendMessageW(GetScreenCordsButton, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND RadiobuttonCUrrentMousePos = CreateWindowW(L"Button", L"Current Mouse Position", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP, 204,159+23+20+5, 157, 20, hwnd, (HMENU)ID_RADIOBUTTONCURRENTMOUSEPOS, hinst, NULL);
        SendMessageW(RadiobuttonCUrrentMousePos, WM_SETFONT, (WPARAM)hf, TRUE);
        SendMessageW(RadiobuttonCUrrentMousePos, BM_SETCHECK, (WPARAM)BST_CHECKED, TRUE);
        HWND TextXCord = CreateWindowW(L"Static", L"X:", WS_VISIBLE | WS_CHILD, 141, 23, 10, 16, clickposGroupbox, NULL, hinst, NULL);
        SendMessageW(TextXCord, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND EditXCord = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 346, 180, 49, 20, hwnd, (HMENU)ID_EDITXPOS, hinst, NULL);
        SendMessageW(EditXCord, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND TextYCord = CreateWindowW(L"Static", L"Y:", WS_VISIBLE | WS_CHILD, 206, 23, 11, 16, clickposGroupbox, NULL, hinst, NULL);
        SendMessageW(TextYCord, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND EditYCord = CreateWindowW(L"Edit", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | WS_TABSTOP, 412, 180, 49, 20, hwnd, (HMENU)ID_EDITYPOS, hinst, NULL);
        SendMessageW(EditYCord, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND StartButton = CreateWindowW(L"Button", L"Start", WS_VISIBLE | WS_CHILD, 10, 319, 228, 140, hwnd, (HMENU)ID_STARTBUTTON, hinst, NULL);
        SendMessageW(StartButton, WM_SETFONT, (WPARAM)hf, TRUE);
        HWND StopButton = CreateWindowW(L"Button", L"Stop", WS_VISIBLE | WS_CHILD, 10 + 228+ 10, 319, 228, 140, hwnd, (HMENU)ID_STOPBUTTON, hinst, NULL);
        SendMessageW(StopButton, WM_SETFONT, (WPARAM)hf, TRUE);
        CreateMyTooltip(hwnd);
        
        

        
        
        
        GetSettings(L"%curdir%\\ACSettings.ini", &acstt);
        uint64_t temp = acstt.clickdelay;
        ULONG hours = temp / 3600000;
        temp -= hours * 3600000;
        ULONG mins = temp / 60000;
        temp -= mins * 60000;
        ULONG seconds = temp / 1000;
        temp -= seconds * 1000;
        ULONG milisec = temp;

       SetWindowText(Editmilisecond, std::to_wstring(milisec).c_str());
       SetWindowText(EditSecond, std::to_wstring(seconds).c_str());
       SetWindowText(EditMinute, std::to_wstring(mins).c_str());
       SetWindowText(EditHour, std::to_wstring(hours).c_str());

       temp = acstt.holddur;
       hours = temp / 3600000;
       temp -= hours * 3600000;
       mins = temp / 60000;
       temp -= mins * 60000;
       seconds = temp / 1000;
       temp -= seconds * 1000;
       milisec = temp;

       SetWindowText(holddureditmilisecbox, std::to_wstring(milisec).c_str());
       SetWindowText(holddureditsecbox, std::to_wstring(seconds).c_str());
       SetWindowText(holddureditminbox, std::to_wstring(mins).c_str());

       if (acstt.isholdindef)
       {
           SendMessage(holddurradiobttn2, BM_SETCHECK, BST_CHECKED, NULL);
           SendMessage(holddurradiobttn1, BM_SETCHECK, BST_UNCHECKED, NULL);
       }
       else
       {
           SendMessage(holddurradiobttn1, BM_SETCHECK, BST_CHECKED, NULL);
           SendMessage(holddurradiobttn2, BM_SETCHECK, BST_UNCHECKED, NULL);
       }
       if (acstt.isexactpos)
       {
           SendMessage(RadiobuttonCustomMousePos, BM_SETCHECK, BST_CHECKED, NULL);
           SendMessage(RadiobuttonCUrrentMousePos, BM_SETCHECK, BST_UNCHECKED, NULL);
       }

       if (!acstt.infrepeat)
       {
           SendMessage(rpptuntstpdrdo, BM_SETCHECK, BST_CHECKED, NULL);
           SendMessage(rpptuntstpdrdo, BM_SETCHECK, BST_UNCHECKED, NULL);
       }
       SetWindowText(EditXCord, std::to_wstring(acstt.xcord).c_str());
       SetWindowText(EditYCord, std::to_wstring(acstt.ycord).c_str());
       SendMessage(combobox, CB_SETCURSEL, (WPARAM)(acstt.mousebutton % 3), NULL);
       SetWindowText(reptfrupdownbuddy, std::to_wstring(acstt.repeatcount).c_str());
       if (!acstt.infrepeat)
       {
           SendMessage(reptfrdo, BM_SETCHECK, BST_CHECKED, NULL);
       }
       hotkeyarr[0] = acstt.subkeycode;
       hotkeyarr[1] = acstt.keycode;
       std::wstring result;
       if (hotkeyarr[0])
       {
           result = ConvertKeyCodeToString(hotkeyarr[0]) + std::wstring(L"+");
       }
       if (hotkeyarr[1])
       {
           result = result + ConvertKeyCodeToString(hotkeyarr[1]);
       }
       SetWindowText(sethotkeybox, result.c_str());
       captureuserinput = true;
       break;
    }
    case WM_COMMAND:
    {
        switch (wParam)
        {
        case ID_STOPBUTTON:
        {
            if (isCapturingHotkey)
            {

                SendMessage(hwnd, WM_COMMAND, ID_BUTTONGETHOTKEY, TRUE);
                if (!captureuserinput)
                {
                    
                    captureuserinput = true;
                    convar.notify_one();
                }
            }
            if (autoclickerenabled)
            {
                autoclickerenabled = false;
                INPUT mousedeinput;
                mousedeinput.type = INPUT_MOUSE;
                mousedeinput.mi.time = 0;
                mousedeinput.mi.mouseData = NULL;
                mousedeinput.mi.dwExtraInfo = 0;

                if (GetAsyncKeyState(VK_LBUTTON))
                {
                    mousedeinput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &mousedeinput, sizeof(mousedeinput));
                }
                if (GetAsyncKeyState(VK_RBUTTON))
                {
                    mousedeinput.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
                    SendInput(1, &mousedeinput, sizeof(mousedeinput));
                }
                if (GetAsyncKeyState(VK_MBUTTON))
                {
                    mousedeinput.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
                    SendInput(1, &mousedeinput, sizeof(mousedeinput));
                }
                SetWindowText(hwnd, L"Stopped - ACprj");
            }
            if (isTracking)
            {
                isTracking = false;
                SendMessage(hwndTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
            }
            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), TRUE);
            EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), TRUE);
            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGETHOTKEY), TRUE);
            break;
        }
        case ID_BUTTONGETHOTKEY:
        {
            
            if (!isCapturingHotkey)
            {
                EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), FALSE);
                EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), FALSE);
                HWND optiongroupbox = GetDlgItem(hwnd, ID_OPTIONGRPBOX);
                HWND button = GetDlgItem(hwnd, ID_BUTTONGETHOTKEY);
                HWND sethotkeybox = GetDlgItem(optiongroupbox, ID_SETHOTKEYBOX);
                SetWindowTextW(button, L"Cancel");
                for (int i = 0; i < 3; ++i)
                    oldhotkeyarr[i] = hotkeyarr[i];
               isCapturingHotkey = true;
               SetWindowTextW(sethotkeybox, L"Please key");
               //hotkeyarr[0] = 0;
               
               captureuserinput = false;
               SetWindowsHook(WH_KEYBOARD, Keyboardhook);
               convar.notify_one();
            }
            else
            {
                if (confirm)
                {
                    EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), TRUE);
                    EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), TRUE);
                    HWND optiongroupbox = GetDlgItem(hwnd, ID_OPTIONGRPBOX);
                    HWND sethotkeybttn = GetDlgItem(hwnd, ID_BUTTONGETHOTKEY);
                    HWND sethotkeybox = GetDlgItem(optiongroupbox, ID_SETHOTKEYBOX);
                    SetWindowTextW(sethotkeybttn, L"Get Hotkey");
                    std::wstring result;
                 
                    if (hotkeyarr[0])
                    {
                        result = ConvertKeyCodeToString(hotkeyarr[0]) + std::wstring(L"+");
                    }
                    if (hotkeyarr[1])
                    {
                        result = result + ConvertKeyCodeToString(hotkeyarr[1]);
                    }
                    isCapturingHotkey = false;
                    captureuserinput = true;
                    convar.notify_one();
                    SetWindowTextW(sethotkeybox, result.c_str());
                    SetWindowTextW(sethotkeybttn, L"Get Hotkey");
                    confirm = false;
                    UnhookWindowsHook(WH_KEYBOARD, Keyboardhook);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), TRUE);
                    EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), TRUE);
                    HWND optiongroupbox = GetDlgItem(hwnd, ID_OPTIONGRPBOX);
                    HWND sethotkeybttn = GetDlgItem(hwnd, ID_BUTTONGETHOTKEY);
                    HWND sethotkeybox = GetDlgItem(optiongroupbox, ID_SETHOTKEYBOX);
                    for (int i = 0; i < 3; ++i)
                        hotkeyarr[i] = oldhotkeyarr[i];
                    SetWindowTextW(sethotkeybttn, L"Get Hotkey");
                    std::wstring result;
                    if (hotkeyarr[0])
                    {
                        result = ConvertKeyCodeToString(hotkeyarr[0]) + std::wstring(L"+");
                    }
                    if (hotkeyarr[1])
                    {
                        result = result + ConvertKeyCodeToString(hotkeyarr[1]);
                    }
                    SetWindowTextW(sethotkeybox, result.c_str());
                    SetWindowTextW(sethotkeybttn, L"Get Hotkey");
                    isCapturingHotkey = false;
                    
                    captureuserinput = false;
                    convar.notify_one();
                    UnhookWindowsHook(WH_KEYBOARD, Keyboardhook);
                }
            }
            
            SetFocus(hwnd);
            break;
        }
        case ID_BUTTONGS:
        {
            if (!isTracking)
            {
                EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), FALSE);
                EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), FALSE);
                EnableWindow(GetDlgItem(hwnd, ID_BUTTONGETHOTKEY), FALSE);
                ShowWindow(hwnd, SW_MINIMIZE);
                isTracking = true;
                SendMessage(hwndTT, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
                std::thread t(startBTracking, hwnd);
                t.detach();
                
                captureuserinput = false;
                convar.notify_one();

            }
            else
            {
                isTracking = false;
                SendMessage(hwndTT, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);


            }
            break;
        }
        case ID_STARTBUTTON:
        {
            if (!autoclickerenabled)
            {
                SetWindowText(hwnd, L"Running - ACprj");
                EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), FALSE);
                EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), FALSE);
                EnableWindow(GetDlgItem(hwnd, ID_BUTTONGETHOTKEY), FALSE);
                bool temp1 = false;
                bool temp2 = true;
                
                std::wstring temp2string;
                UINT temp2int = 0;
                int tempdx = 0;
                int tempdy = 0;
                if (SendMessage(GetDlgItem(hwnd, ID_RADIOBUTTONCUSTOMMOUSEPOS), BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    /*
                    std::wstring buffe;
                    GetWindowText(GetDlgItem(hwnd, ID_EDITXPOS), &buffe[0], GetWindowTextLength(GetDlgItem(hwnd, ID_EDITXPOS)) + 1);
                    tempdx = _wtoi(buffe.c_str());
                    
                    GetWindowText(GetDlgItem(hwnd, ID_EDITYPOS), buffe.data(), GetWindowTextLength(GetDlgItem(hwnd, ID_EDITXPOS)) + 1);
                    tempdy = _wtoi(buffe.c_str());
                    */
                    std::wstring buff;
                    UINT len = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITXPOS)) + 1;
                    buff.resize(len);
                    GetWindowText(GetDlgItem(hwnd, ID_EDITXPOS), &buff[0], len);
                    tempdx = _wtoi(buff.c_str());

                    len = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITYPOS)) + 1;
                    buff.resize(len);
                    GetWindowText(GetDlgItem(hwnd, ID_EDITYPOS), &buff[0], len);
                    tempdy = _wtoi(buff.c_str());

                    std::wstring buffe;
                    int templen = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITXPOS)) + 1;
                    buffe.resize(templen);
                    GetWindowText(GetDlgItem(hwnd, ID_EDITXPOS), &buffe[0], templen);
                     templen = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITYPOS)) + 1;
                    buffe.resize(templen);
                    GetWindowText(GetDlgItem(hwnd, ID_EDITYPOS), &buffe[0], templen);
                    temp1 = true;
                }
                int mouseindex = SendMessage(GetDlgItem(hwnd, ID_DURCOMBOBOX), CB_GETCURSEL, 0, 0);
                if (SendMessage(GetDlgItem(hwnd, ID_REPTFORRDO), BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    temp2 = false;
                    int templen = GetWindowTextLength(GetDlgItem(hwnd, ID_REPTFORRDO)) + 1;
                    temp2string.resize(templen);
                    GetWindowText(GetDlgItem(hwnd, ID_REPEATFORUPDOWNBUDDY), &temp2string[0], templen);
                    temp2int = _wtoi(temp2string.c_str());
                }




                if (SendMessage(GetDlgItem(hwnd, ID_DURRADIOBTTN2), BM_GETCHECK, 0, 0) == BST_CHECKED)
                {
                    INPUT mouseinput;
                    mouseinput.type = INPUT_MOUSE;
                    if (temp1)
                    {
                        SetCursorPos(tempdx, tempdy);
                    }
                    switch (mouseindex)
                    {
                    case 0:
                    {
                        mouseinput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                        mouseinput.mi.dwExtraInfo = NULL;
                        mouseinput.mi.mouseData = 0;
                        mouseinput.mi.time = 0;
                        
                        break;
                    }
                    case 1:
                    {
                        mouseinput.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
                        mouseinput.mi.dwExtraInfo = NULL;
                        mouseinput.mi.mouseData = 0;
                        mouseinput.mi.time = 0;
                        
                        break;
                    }
                    case 2:
                    {
                        mouseinput.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
                        mouseinput.mi.dwExtraInfo = NULL;
                        mouseinput.mi.mouseData = 0;
                        mouseinput.mi.time = 0;
                        
                        break;
                    }
                    
                    break;
                    }
                    SendInput(1, &mouseinput, sizeof(mouseinput));
                    autoclickerenabled = true;
                }
                else
                {
                    
                    uint64_t timeinmilisec = 0;
                    uint64_t holddurinmilisec = 0;
                    
                    for (unsigned int a = 2001; a < 2005; a++)
                    {
                        std::wstring buffer;
                        int templen = GetWindowTextLength(GetDlgItem(hwnd, a)) + 1;
                        buffer.resize(templen);
                        GetWindowText(GetDlgItem(hwnd, a), &buffer[0], templen);
                        
                        if (a == 2001) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)60 * (uint64_t)60 * (uint64_t)1000;
                        if (a == 2002) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()))  * (uint64_t)60 * (uint64_t)1000;
                        if (a==2003) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)1000;
                        if (a==2004) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
                    }
                    for (unsigned int a = 2005; a < 2008; a++)
                    {
                        std::wstring buffer;
                        int templen = GetWindowTextLength(GetDlgItem(hwnd, a)) + 1;
                        buffer.resize(templen);
                        GetWindowText(GetDlgItem(hwnd, a), &buffer[0], templen);
                        //holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
                        if (a == 2005) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)60 * (uint64_t)1000;
                        if (a== 2006) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)1000;
                        if (a== 2007) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
                    }

                    if (timeinmilisec + holddurinmilisec == 0)
                    {
                        if (MessageBox(hwnd, L"You are about to run autoclicker at high speed which can lead to high CPU usage. Continue?", L"Warning!", MB_YESNO) == IDYES)
                        {
                            autoclickerenabled = true;
                            std::thread(autoclickfunc, mouseindex,timeinmilisec, holddurinmilisec, tempdx, tempdy, temp1, temp2, temp2int).detach();
                        }
                        else
                        {
                            autoclickerenabled = false;
                            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), TRUE);
                            EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), TRUE);
                            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGETHOTKEY), TRUE);
                            break;
                        }
                    }
                    else
                    {
                        autoclickerenabled = true;
                        std::thread(autoclickfunc, mouseindex, timeinmilisec, holddurinmilisec, tempdx, tempdy, temp1, temp2, temp2int).detach();
                    }
                    
                }
               
                
            }
            

            break;
        }
        default:
            break;
        }
        break;
    }
    case WM_DESTROY:
    {
        masterswitch = false;
        PostQuitMessage(0);
        break;
    }

    case WM_CLOSE:
    {
        masterswitch = false;
        UnhookWindowsHook(WH_KEYBOARD, Keyboardhook);
        uint64_t timeinmilisec = 0;
        uint64_t holddurinmilisec = 0;

        for (unsigned int a = 2001; a < 2005; a++)
        {
            std::wstring buffer;
            int templen = GetWindowTextLength(GetDlgItem(hwnd, a)) + 1;
            buffer.resize(templen);
            GetWindowText(GetDlgItem(hwnd, a), &buffer[0], templen);
            //should have used += here
            if (a == 2001) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)60 * (uint64_t)60 * (uint64_t)1000;
            if (a == 2002) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)60 * (uint64_t)1000;
            if (a == 2003) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)1000;
            if (a == 2004) timeinmilisec = timeinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
        }
        for (unsigned int a = 2005; a < 2008; a++)
        {
            std::wstring buffer;
            int templen = GetWindowTextLength(GetDlgItem(hwnd, a)) + 1;
            buffer.resize(templen);
            GetWindowText(GetDlgItem(hwnd, a), &buffer[0], templen);
            //holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
            if (a == 2005) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)60 * (uint64_t)1000;
            if (a == 2006) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str())) * (uint64_t)1000;
            if (a == 2007) holddurinmilisec = holddurinmilisec + (uint64_t)std::abs(_wtoi64(buffer.c_str()));
        }
        if ((SendMessage(GetDlgItem(hwnd, ID_RADIOBUTTONCUSTOMMOUSEPOS), BM_GETCHECK, 0, 0) == BST_CHECKED))
        {
            acstt.isexactpos = true;

        }
        else
        {
            acstt.isexactpos = false;
        }

        if (SendMessage(GetDlgItem(hwnd, ID_DURRADIOBTTN2), BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            acstt.isholdindef = true;
        }
        else
        {
            acstt.isholdindef = false;
        }

        if (SendMessage(GetDlgItem(hwnd, ID_REPTFORRDO), BM_GETCHECK, NULL, NULL) != BST_CHECKED)
        {
            acstt.infrepeat = true;
        }
        else
        {
            acstt.infrepeat = false;
        }
        acstt.mousebutton = SendMessage(GetDlgItem(hwnd, ID_DURCOMBOBOX), CB_GETCURSEL, 0, 0);
        acstt.holddur = holddurinmilisec;
        acstt.clickdelay = timeinmilisec;
        acstt.keycode = hotkeyarr[1];
        acstt.subkeycode = hotkeyarr[0];
        std::wstring buffe;
        int templen = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITXPOS)) + 1;
        buffe.resize(templen);
        GetWindowText(GetDlgItem(hwnd, ID_EDITXPOS), &buffe[0], templen);
        acstt.xcord = _wtoi(buffe.c_str());
        templen = GetWindowTextLength(GetDlgItem(hwnd, ID_EDITYPOS)) + 1;
        buffe.resize(templen);
        GetWindowText(GetDlgItem(hwnd, ID_EDITYPOS), &buffe[0], templen);
        acstt.ycord = _wtoi(buffe.c_str());
        templen = GetWindowTextLength(GetDlgItem(hwnd, ID_REPEATFORUPDOWNBUDDY)) + 1;
        buffe.resize(templen);
        GetWindowText(GetDlgItem(hwnd, ID_REPEATFORUPDOWNBUDDY), &buffe[0], templen);
        acstt.repeatcount = _wtoi(buffe.c_str());


        WriteSettings(L"%curdir%\\ACSettings.ini", &acstt);


        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    }

   
    return DefWindowProc(hwnd, msg, wParam, lParam);
}



void CreateMyTooltip(HWND hwnd)
{

    hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 50, 50, 0, 0, hwnd, NULL, hinst, NULL);


    ti.cbSize = sizeof(TTTOOLINFO);
    ti.uFlags = TTF_SUBCLASS | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd = hwnd;
    ti.lpszText = const_cast<LPWSTR>(L"Temp");
    GetClientRect(hwnd, &ti.rect);

    if (!SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti))
        MessageBox(0, TEXT("Failed: TTM_ADDTOOL"), 0, 0);
}
void startBTracking(HWND hwnd)
{
    int oldX = 0;
    int oldY = 0;
    SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, (LPARAM)231);
    while (isTracking && masterswitch)
    {
        POINT pt;
        GetCursorPos(&pt);
        if (oldX == pt.x && oldY == pt.y)
        {

        }
        else
        {
            
            
            std::wstring wstrresult = L"Press ESC or left mouse button to select";
            wstrresult = wstrresult + std::wstring(L" ") + std::to_wstring(pt.x) +std::wstring(L", ") + std::to_wstring(pt.y);
            ti.lpszText = const_cast<wchar_t*>(wstrresult.c_str());
            SendMessage(hwndTT, TTM_SETTOOLINFO, 0, (LPARAM)(LPTTTOOLINFO)&ti);
            SendMessage(hwndTT, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x + 10, pt.y + 10));
            oldX = pt.x;
            oldY = pt.y;
        }
        
        Sleep(10);

        if ((GetAsyncKeyState(VK_ESCAPE) & (1 << 15)) != 0 || (GetAsyncKeyState(VK_LBUTTON) & (1 << 15)) != 0)
        {
            isTracking = false;
            ShowWindow(hwnd, SW_SHOWNORMAL);
            SetForegroundWindow(hwnd);
            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGS), TRUE);
            EnableWindow(GetDlgItem(hwnd, ID_STARTBUTTON), TRUE);
            EnableWindow(GetDlgItem(hwnd, ID_BUTTONGETHOTKEY), TRUE);
            SetWindowTextW(GetDlgItem(hwnd, ID_EDITXPOS), std::to_wstring(pt.x).c_str());
            SetWindowTextW(GetDlgItem(hwnd, ID_EDITYPOS), std::to_wstring(pt.y).c_str());
            std::unique_lock<std::mutex> lock(mutelock);
            captureuserinput = true;
            convar.notify_one();

        }
        if (!isTracking)
        {
            SendMessage(hwndTT, TTM_TRACKACTIVATE, 0, (LPARAM)&ti);
        }
    }
}
void getuserinput()
{
    while (masterswitch)
    {
        
        std::unique_lock<std::mutex> lock(mutelock);
        while (!captureuserinput) convar.wait(lock, []() { return captureuserinput; });

       
        //wtf was this for
        //This is the small delay that prevent activation of autoclicker right after the hotkey is set
        if (hotkeyarr[0])
        {
            while ((GetAsyncKeyState(hotkeyarr[0]) & (1 << 15)) != 0 && (GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        else
        {
            while ((GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        
        //this is stupid
        /*while (captureuserinput)
        {
            
            if (hotkeyarr[0])
            {
                if ((GetAsyncKeyState(hotkeyarr[0]) & (1 << 15)) != 0 && (GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
                {
                    if (!autoclickerenabled)
                    {
                        SendMessage(wndhwnd, WM_COMMAND, ID_STARTBUTTON, TRUE);
                        break;
                    }
                    else
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, TRUE);
                        break;
                    }
                }
            }
            else
            {
                if ((GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
                {
                    if (!autoclickerenabled)
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STARTBUTTON, TRUE);
                        break;
                    }
                    else
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, TRUE);
                        break;
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        */

        if (hotkeyarr[0])
        {
            while (captureuserinput) {
                if ((GetAsyncKeyState(hotkeyarr[0]) & (1 << 15)) != 0 && (GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
                {
                    if (!autoclickerenabled)
                    {
                        SendMessage(wndhwnd, WM_COMMAND, ID_STARTBUTTON, TRUE);
                        break;
                    }
                    else
                    {

                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, TRUE);
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        else
        {
            while (captureuserinput) {
                if ((GetAsyncKeyState(hotkeyarr[1]) & (1 << 15)) != 0)
                {
                    if (!autoclickerenabled)
                    {

                        SendMessage(wndhwnd, WM_COMMAND, ID_STARTBUTTON, TRUE);
                        break;
                    }
                    else
                    {

                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, TRUE);
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }


    }
}


void autoclickfunc(int index,  uint64_t timeinmilisecond, uint64_t holdinmilisec, int dx, int dy, bool isAbsolute, bool infinite, UINT repeat)
{
    int tempint = 0;
    if (!infinite)
    {
       tempint = repeat;
    }

    switch (index)
    {
    case 0:
    { 
        if (isAbsolute)
    {
        while (autoclickerenabled && masterswitch)
        {
            SetCursorPos(dx, dy);
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, NULL, NULL);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, NULL, NULL);
            
            if (!infinite && tempint > 0)
            {
                tempint = tempint - 1;
                if (tempint <= 0)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                    //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                }
            }
            else if (!infinite)
            {

                SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
        }
    }
    else
    {
        while (autoclickerenabled && masterswitch)
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, NULL, NULL);
            std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, NULL, NULL);
            //std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
            if (!infinite && tempint > 0)
            {
                tempint = tempint - 1;
                if (tempint <= 0)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                    //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                }
            }
            else if (!infinite)
            {

                SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
        }
    }
    break;
    }
    case 1:
    {
        if (isAbsolute)
        {
            while (autoclickerenabled && masterswitch)
            {
                SetCursorPos(dx, dy);
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, NULL, NULL);
                std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, NULL, NULL);
                //std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
                if (!infinite && tempint > 0)
                {
                    tempint = tempint - 1;
                    if (tempint <= 0)
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                        //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                    }
                }
                else if (!infinite)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
            }
        }
        else
        {
            while (autoclickerenabled && masterswitch)
            {
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, NULL, NULL);
                std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, NULL, NULL);
                //std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
                if (!infinite && tempint > 0)
                {
                    tempint = tempint - 1;
                    if (tempint <= 0)
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                        //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                    }
                }
                else if (!infinite)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
            }
        }
        break;
    }
    case 2:
    {
        if (isAbsolute)
        {
            while (autoclickerenabled && masterswitch)
            {
                SetCursorPos(dx, dy);
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, NULL, NULL);
                std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, NULL, NULL);
                //std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
                if (!infinite && tempint > 0)
                {
                    tempint = tempint - 1;
                    if (tempint <= 0)
                    {
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                        //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                    }
                }
                else if (!infinite)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
            }
        }
        else
        {
            while (autoclickerenabled && masterswitch)
            {
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, NULL, NULL);
                std::this_thread::sleep_for(std::chrono::milliseconds(holdinmilisec));
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, NULL, NULL);
                std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
                if (!infinite && tempint > 0)
                {
                    tempint = tempint - 1;
                    if (tempint <= 0)
                    {
                        
                        SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                        //SetWindowText(wndhwnd, L"Stopped - ACPrj");
                        
                    }
                }
                else if (!infinite)
                {
                    
                    SendMessage(wndhwnd, WM_COMMAND, ID_STOPBUTTON, NULL);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(timeinmilisecond));
            }
        }
    }
    break;
    }
}
