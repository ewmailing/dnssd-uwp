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

#include "dnssd_uwp.h"
#include "DnssdClient.h"
#include <string>
using namespace dnssd_uwp;
using namespace std;

DnssdClient::DnssdClient()
{
    mDnssdInitFunc = nullptr;
#ifdef DNSSDUWP_USE_LEGACY

    mDnssdCreateServiceWatcherFunc = nullptr;
    mDnssdFreeServiceWatcherFunc = nullptr;
    mDnssdCreateServiceFunc = nullptr;
    mDnssdFreeServiceFunc = nullptr;
    mDnssdServiceWatcherPtr = nullptr;
#endif
    mDnssdServicePtr = nullptr;
	mDnssdServiceDiscoveryPtr = nullptr;

	mDllHandle = NULL;

	mDnssdRegisterServiceFunc = nullptr;
	mDnssdUnregisterServiceFunc = nullptr;
	mDnssdStartDiscoveryFunc = nullptr;
	mDnssdStopDiscoveryFunc = nullptr;

}

DnssdClient::~DnssdClient()
{
#ifdef DNSSDUWP_USE_LEGACY
	if (mDnssdFreeServiceFunc && mDnssdServicePtr)
    {
        mDnssdFreeServiceFunc(mDnssdServicePtr);
    }

    if (mDnssdFreeServiceWatcherFunc && mDnssdServiceWatcherPtr)
    {
        mDnssdFreeServiceWatcherFunc(mDnssdServiceWatcherPtr);
    }
#endif
	if (mDnssdUnregisterServiceFunc && mDnssdServicePtr)
    {
        mDnssdUnregisterServiceFunc(mDnssdServicePtr);
    }
	if(mDnssdStopDiscoveryFunc && mDnssdServiceDiscoveryPtr)
	{
		mDnssdStopDiscoveryFunc(mDnssdServiceDiscoveryPtr);
	}
	if(mDnssdStopResolveFunc && mDnssdServiceResolverPtr)
	{
		mDnssdStopResolveFunc(mDnssdServiceResolverPtr);
	}

    //Free the library:
    if (mDllHandle)
    {
        FreeLibrary(mDllHandle);
    }
}

DnssdErrorType DnssdClient::InitializeDnssd()
{
    DnssdErrorType result = DNSSD_NO_ERROR;

    if (mDllHandle == NULL)
    {
        mDllHandle = LoadLibrary(L"Dnssd_uwp.dll");
        if (NULL == mDllHandle)
        {
            result = DNSSD_DLL_MISSING_ERROR;
            goto cleanup;
        }
    }

    // GetDnssd DLL function pointers. Error checking needs to be added!
    //Get pointer to the DnssdInitializeFunc function using GetProcAddress:  
    mDnssdInitFunc = reinterpret_cast<DnssdInitializeFunc>(::GetProcAddress(mDllHandle, "dnssd_initialize"));

#ifdef DNSSDUWP_USE_LEGACY

    //Get pointer to the DnssdFreeServiceWatcherFunc function using GetProcAddress:  
    mDnssdFreeServiceWatcherFunc = reinterpret_cast<DnssdFreeServiceWatcherFunc>(::GetProcAddress(mDllHandle, "dnssd_free_service_watcher"));

    //Get pointer to the DnssdCreateServiceWatcherFunc function using GetProcAddress:  
    mDnssdCreateServiceWatcherFunc = reinterpret_cast<DnssdCreateServiceWatcherFunc>(::GetProcAddress(mDllHandle, "dnssd_create_service_watcher"));

    //Get pointer to the DnssdFreeServiceFunc function using GetProcAddress:  
    mDnssdFreeServiceFunc = reinterpret_cast<DnssdFreeServiceFunc>(::GetProcAddress(mDllHandle, "dnssd_free_service"));

    //Get pointer to the DnssdCreateServiceFunc function using GetProcAddress:  
    mDnssdCreateServiceFunc = reinterpret_cast<DnssdCreateServiceFunc>(::GetProcAddress(mDllHandle, "dnssd_create_service"));
#endif

    mDnssdRegisterServiceFunc = reinterpret_cast<DnssdRegisterServiceFunc>(::GetProcAddress(mDllHandle, "dnssd_register_service"));
    mDnssdUnregisterServiceFunc = reinterpret_cast<DnssdUnregisterServiceFunc>(::GetProcAddress(mDllHandle, "dnssd_unregister_service"));
    mDnssdStartDiscoveryFunc = reinterpret_cast<DnssdStartDiscoveryFunc>(::GetProcAddress(mDllHandle, "dnssd_start_discovery"));
	mDnssdStopDiscoveryFunc = reinterpret_cast<DnssdStopDiscoveryFunc>(::GetProcAddress(mDllHandle, "dnssd_stop_discovery"));

	mDnssdStartResolveFunc = reinterpret_cast<DnssdStartResolveFunc>(::GetProcAddress(mDllHandle, "dnssd_start_resolve"));
	mDnssdStopResolveFunc = reinterpret_cast<DnssdStopResolveFunc>(::GetProcAddress(mDllHandle, "dnssd_stop_resolve"));

	// initialize dnssd interface
    result = mDnssdInitFunc();
    if (result != DNSSD_NO_ERROR)
    {
        goto cleanup;
    }

cleanup:
    return result;
}

#ifdef DNSSDUWP_USE_LEGACY
DnssdErrorType DnssdClient::InitializeDnssdServiceWatcher(const std::string& serviceName, const std::string& port, DnssdServiceChangedCallback callback)
{
    // create a dns service watcher
    DnssdErrorType result = mDnssdCreateServiceWatcherFunc(serviceName.c_str(), callback, &mDnssdServiceWatcherPtr);
    return result;
}

DnssdErrorType DnssdClient::InitializeDnssdService(const std::string& serviceName, const std::string& port)
{
    // create a dns service 
    DnssdErrorType result = mDnssdCreateServiceFunc(serviceName.c_str(), port.c_str(), &mDnssdServicePtr);
    return result;
}
#endif

DnssdErrorType DnssdClient::RegisterDnssdService(const std::string& service_name, const std::string& service_type, const char* domain, uint16_t network_port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data )
{
    // create a dns service 
//    DnssdErrorType result = mDnssdCreateServiceFunc(serviceName.c_str(), port.c_str(), &mDnssdServicePtr);
    DnssdErrorType result = DNSSD_NO_ERROR;
	result = mDnssdRegisterServiceFunc(service_name.c_str(), service_type.c_str(), domain, network_port, txt_record, txt_record_length, callback_function, user_data, &mDnssdServicePtr);

    return result;
}

void DnssdClient::UnregisterDnssdService()
{
    // create a dns service 
//    DnssdErrorType result = mDnssdCreateServiceFunc(serviceName.c_str(), port.c_str(), &mDnssdServicePtr);
	mDnssdUnregisterServiceFunc(mDnssdServicePtr);
	mDnssdServicePtr = nullptr;

}

DnssdErrorType DnssdClient::StartDiscovery(const std::string& service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback_function, void* user_data)
{
    // create a dns service 
//    DnssdErrorType result = mDnssdCreateServiceFunc(serviceName.c_str(), port.c_str(), &mDnssdServicePtr);

	DnssdErrorType result = mDnssdStartDiscoveryFunc(service_type.c_str(), domain, callback_function, user_data, &mDnssdServiceDiscoveryPtr);
    return result;
}

void DnssdClient::StopDiscovery()
{
	mDnssdStopDiscoveryFunc(mDnssdServiceDiscoveryPtr);
	mDnssdServiceDiscoveryPtr = nullptr;
}


DnssdErrorType DnssdClient::StartResolve(const std::string& service_name, const std::string& service_type, const char* domain, DnssdServiceResolverChangedCallback callback_function, void* user_data)
{
    // create a dns service 
//    DnssdErrorType result = mDnssdCreateServiceFunc(serviceName.c_str(), port.c_str(), &mDnssdServicePtr);

	DnssdErrorType result = mDnssdStartResolveFunc(service_name.c_str(), service_type.c_str(), domain, callback_function, user_data, &mDnssdServiceResolverPtr);
    return result;
}

void DnssdClient::StopResolve()
{
	mDnssdStopResolveFunc(mDnssdServiceResolverPtr);
	mDnssdServiceResolverPtr = nullptr;
}



