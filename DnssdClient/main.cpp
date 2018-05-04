// ******************************************************************
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THE CODE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE CODE OR THE USE OR OTHER DEALINGS IN THE CODE.
// ******************************************************************

#include "stdafx.h"
#include "dnssd_uwp.h"
#include "DnssdClient.h"
#include "WindowsVersionHelper.h"
#include <iostream>
#include <string>
#include <conio.h>
#include <assert.h>
#include <memory>

#define USING_APP_MANIFEST
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace std;
using namespace dnssd_uwp;

CRITICAL_SECTION gCriticalSection;

static const std::string gServiceName = "_daap._tcp"; // This is misnamed
static const std::string gServiceType = "_daap._tcp";
static const std::string gServicePort = "3689";
//static const char gResolveServiceNameUTF8[] = "Eric Wing\xe2\x80\x99 Music";
//static const char gResolveServiceNameUTF8[] = "Al\xe2\x80\x99 Music";
static const std::wstring gResolveServiceName = L"Al’s Music";

//(uint16_t)std::stoi(port
uint16_t gNetworkPort = 3689;

std::unique_ptr<DnssdClient> gDnssdClient;


wchar_t* BlurrrPlatformWindows_CreateWINfromUTF8String(const char* input_str)
{
	if(NULL == input_str)
	{
		return NULL;
	}
    int str_len = (int)strlen(input_str);
    int num_chars = MultiByteToWideChar(CP_UTF8, 0, input_str, str_len, NULL, 0);
    wchar_t* output_winstr = (wchar_t*)malloc((num_chars + 1) * sizeof(wchar_t));
    if(output_winstr)
    {
        MultiByteToWideChar(CP_UTF8, 0, input_str, str_len, output_winstr, num_chars);
        output_winstr[num_chars] = L'\0';
    }
    return output_winstr;
}

//char* BlurrrPlatformWindows_ConvertWINtoUTF8Str(const wchar_t* input_wstr)
char* BlurrrPlatformWindows_CreateUTF8fromWINString(const wchar_t* input_wstr)
{
	if(NULL == input_wstr)
	{
		return NULL;
	}
    int wstr_len = (int)wcslen(input_wstr);
    int num_chars = WideCharToMultiByte(CP_UTF8, 0, input_wstr, wstr_len, NULL, 0, NULL, NULL);
    char* output_utf8str = (char*)malloc((num_chars + 1) * sizeof(char));
    if(output_utf8str)
    {
        WideCharToMultiByte(CP_UTF8, 0, input_wstr, wstr_len, output_utf8str, num_chars, NULL, NULL);
        output_utf8str[num_chars] = '\0';
    }
    return output_utf8str;
}

void dnssdServiceChangedCallback(const DnssdServiceWatcherPtr serviceWatcher, DnssdServiceUpdateType update, DnssdServiceInfoPtr info)
{
    EnterCriticalSection(&gCriticalSection);
    string portName = "In";

    switch (update)
    {
    case ServiceAdded:
        wcout << L"*** dnssd service added ***" << endl;
        break;

    case ServiceUpdated:
        wcout << L"*** dnssd service updated ***" << endl;
        break;

    case ServiceRemoved:
        wcout << L"*** dnssd service removed ***" << endl;
        break;
    }

    if (info != nullptr)
    {
        auto cp = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        wprintf(L"name: %S\n", info->instanceName);
  //      wprintf(L"host: %S\n", info->host);
        wprintf(L"port: %S\n", info->port);
        wprintf(L"  id: %S\n", info->id);
        SetConsoleOutputCP(cp);
    }
    wcout << endl;
    LeaveCriticalSection(&gCriticalSection);
}

void OnRegisterCallback(DnssdServicePtr service_ptr, const char* service_name, const char* service_type, const char* domain, uint16_t network_port, DnssdErrorType error_code, void* user_data)
{
	    EnterCriticalSection(&gCriticalSection);
//		        auto cp = GetConsoleOutputCP();
//        SetConsoleOutputCP(CP_UTF8);

	wchar_t* w_service_name = BlurrrPlatformWindows_CreateWINfromUTF8String(service_name);
	wchar_t* w_service_type = BlurrrPlatformWindows_CreateWINfromUTF8String(service_type);
	wchar_t* w_domain = BlurrrPlatformWindows_CreateWINfromUTF8String(domain);


	wcout << L"*** dnssd OnRegisterCallback ***" << endl;
	wcout << L"service_name: " << w_service_name << endl;
	wcout << L"service_type: " << w_service_type << endl;
	wcout << L"domain: " << w_domain << endl;
	wcout << L"network_port: " << network_port << endl;
	wcout << L"error_code: " << error_code << endl;
//	         SetConsoleOutputCP(cp);

	OutputDebugStringW(w_service_name);
	OutputDebugStringW(L"\n");
	OutputDebugStringW(w_service_type);
	OutputDebugStringW(L"\n");
	
	free(w_domain);
	free(w_service_type);
	free(w_service_name);

	LeaveCriticalSection(&gCriticalSection);


}


void OnDiscoveryCallback(DnssdServiceDiscoveryPtr service_discovery, const char* service_name, const char* service_type, const char* domain, uint32_t flags, DnssdErrorType error_code, void* user_data)
{
	    EnterCriticalSection(&gCriticalSection);
	//	        auto cp = GetConsoleOutputCP();
//        SetConsoleOutputCP(CP_UTF8);
		
	wchar_t* w_service_name = BlurrrPlatformWindows_CreateWINfromUTF8String(service_name);
	wchar_t* w_service_type = BlurrrPlatformWindows_CreateWINfromUTF8String(service_type);
	wchar_t* w_domain = BlurrrPlatformWindows_CreateWINfromUTF8String(domain);

	wcout << L"*** dnssd OnDiscoveryCallback ***" << endl;
	wcout << L"service_name: " << w_service_name << endl;
	wcout << L"service_type: " << w_service_type << endl;
	wcout << L"domain: " << w_domain << endl;

	wcout << L"flags: " << flags << endl;
	  //      SetConsoleOutputCP(cp);
	OutputDebugStringW(w_service_name);
	OutputDebugStringW(L"\n");
	OutputDebugStringW(w_service_type);
	OutputDebugStringW(L"\n");
	free(w_domain);
	free(w_service_type);
	free(w_service_name);

	LeaveCriticalSection(&gCriticalSection);
}

#include <WinDNS.h>
void OnResolveCallback(DnssdServiceResolverPtr service_resolver, const char* service_name, const char* service_type, const char* domain, const char* full_name, const char* host_target, uint16_t port, const char* txt_record, size_t txt_record_length, DnssdErrorType error_code, void* user_data)
{
	    EnterCriticalSection(&gCriticalSection);
	//	        auto cp = GetConsoleOutputCP();
//        SetConsoleOutputCP(CP_UTF8);
		
	wchar_t* w_service_name = BlurrrPlatformWindows_CreateWINfromUTF8String(service_name);
	wchar_t* w_service_type = BlurrrPlatformWindows_CreateWINfromUTF8String(service_type);
	wchar_t* w_domain = BlurrrPlatformWindows_CreateWINfromUTF8String(domain);
	wchar_t* w_full_name = BlurrrPlatformWindows_CreateWINfromUTF8String(full_name);
	wchar_t* w_host_target = BlurrrPlatformWindows_CreateWINfromUTF8String(host_target);


	wcout << L"*** dnssd OnResolveCallback ***" << endl;
	wcout << L"service_name: " << w_service_name << endl;
	wcout << L"service_type: " << w_service_type << endl;
	wcout << L"domain: " << w_domain << endl;
	wcout << L"full_name: " << w_full_name << endl;
	wcout << L"host_target: " << w_host_target << endl;
	wcout << L"port: " << port << endl;
	if(txt_record == NULL)
	{
		wcout << L"txt_record: NULL" << endl;
	}
	else
	{
		wchar_t* w_txt_record = BlurrrPlatformWindows_CreateWINfromUTF8String(txt_record);
		wcout << L"txt_record: " << w_txt_record << endl;
		free(w_txt_record);
	}
	wcout << L"txt_record_length: " << txt_record_length << endl;

//	DNS_STATUS status = DnsQuery_W(L"MyServiceName._daap._tcp.local.", DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, NULL, NULL);
	DNS_STATUS status = DnsQuery_W(w_full_name, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, NULL, NULL);
//	DNS_STATUS status = DnsQuery_W(w_full_name, DNS_TYPE_A, DNS_QUERY_STANDARD, NULL, NULL, NULL);
	wcout << L"status: " << status << endl;


	free(w_host_target);
	free(w_full_name);
	free(w_domain);
	free(w_service_type);
	free(w_service_name);

	LeaveCriticalSection(&gCriticalSection);
}

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    case CTRL_LOGOFF_EVENT:
        gDnssdClient.reset();
        gDnssdClient = nullptr;
        return(TRUE);

    case CTRL_BREAK_EVENT:
        return FALSE;

    default:
        return FALSE;
    }
}
#include <io.h> 
#include <fcntl.h>
int main()
{
_setmode(_fileno(stdout), _O_U16TEXT);
    DnssdErrorType result = DNSSD_NO_ERROR;

    gDnssdClient = std::unique_ptr<DnssdClient>(new DnssdClient());

    // add a handler to clean up DnssdClient for various console exit scenarios
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    wcout << L"Press any key to exit..." << endl << endl;

    // Initialize the dsssd api
#ifdef USING_APP_MANIFEST
    if(windows10orGreaterWithManifest())
    {
        gDnssdClient->InitializeDnssd();
    }
#else
    if (windows10orGreater())
    {
        gDnssdClient->InitializeDnssd();
    }
#endif

    if(result != DNSSD_NO_ERROR)
    {
        wcout << L"Unable to initialize dnssd" << endl;
        goto cleanup;
    }
  
    InitializeCriticalSection(&gCriticalSection);
#if 1
//    result = gDnssdClient->InitializeDnssdServiceWatcher(gServiceName, gServicePort, dnssdServiceChangedCallback);
    result = gDnssdClient->StartDiscovery(gServiceType, "", OnDiscoveryCallback, NULL);
	if (result != DNSSD_NO_ERROR)
    {
        wcout << L"Unable to initialize dnssd service watcher" << endl;
        goto cleanup;
    }
#endif
#if 1
//    result = gDnssdClient->InitializeDnssdService(gServiceName, gServicePort);
    result = gDnssdClient->RegisterDnssdService("MyServiceName", gServiceName, NULL, gNetworkPort, OnRegisterCallback, NULL);
    if (result != DNSSD_NO_ERROR)
    {
        wcout << L"Unable to initialize dnssd service" << endl;
        goto cleanup;
    }
#endif // 0

#if 0
	result = gDnssdClient->StartResolve("MyServiceName", gServiceType, NULL, OnResolveCallback, NULL);
//	result = gDnssdClient->StartResolve(gResolveServiceNameUTF8, gServiceType, NULL, OnResolveCallback, NULL);
	if (result != DNSSD_NO_ERROR)
    {
        wcout << L"Unable to initialize dnssd service watcher" << endl;
        goto cleanup;
    }
#endif

cleanup:
    // process dnssd callbacks until user presses a key on keyboard
    char c = _getch();
//		gDnssdClient->StopDiscovery();

#if 1
//	result = gDnssdClient->StartResolve("MyServiceName", gServiceType, NULL, OnResolveCallback, NULL);
//	result = gDnssdClient->StartResolve("My Test Service", gServiceType, NULL, OnResolveCallback, NULL);
	char* utf8_str = BlurrrPlatformWindows_CreateUTF8fromWINString(gResolveServiceName.c_str());
	result = gDnssdClient->StartResolve(utf8_str, gServiceType, NULL, OnResolveCallback, NULL);
	if (result != DNSSD_NO_ERROR)
    {
        wcout << L"Unable to initialize dnssd service watcher" << endl;
        goto cleanup;
    }
	free(utf8_str);
#endif

    c = _getch();

	gDnssdClient->UnregisterDnssdService();

    c = _getch();

    gDnssdClient.reset();
    gDnssdClient = nullptr;
    DeleteCriticalSection(&gCriticalSection);

    return 0;
}