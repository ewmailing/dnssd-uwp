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

#pragma once

#include "dnssd_uwp.h"
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace dnssd_uwp
{
    class DnssdClient 
    {
    public:
        DnssdClient();
        ~DnssdClient();

        DnssdErrorType InitializeDnssd();
#ifdef DNSSDUWP_USE_LEGACY
        DnssdErrorType InitializeDnssdServiceWatcher(const std::string& serviceName, const std::string& port, DnssdServiceChangedCallback callback);
        DnssdErrorType InitializeDnssdService(const std::string& serviceName, const std::string& port);
#endif
		DnssdErrorType RegisterDnssdService(const std::string& service_name, const std::string& service_type, const char* domain, uint16_t network_port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data);
		void UnregisterDnssdService();
		DnssdErrorType StartDiscovery(const std::string& service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback_function, void* user_data);
		void StopDiscovery();
		DnssdErrorType StartResolve(const std::string& service_name, const std::string& service_type, const char* domain, DnssdServiceResolverChangedCallback callback_function, void* user_data);
		void StopResolve();

	private:
        // Dnssd DLL function pointers
        DnssdInitializeFunc             mDnssdInitFunc;
#ifdef DNSSDUWP_USE_LEGACY
        DnssdCreateServiceWatcherFunc   mDnssdCreateServiceWatcherFunc;
        DnssdFreeServiceWatcherFunc     mDnssdFreeServiceWatcherFunc;
        DnssdCreateServiceFunc          mDnssdCreateServiceFunc;
        DnssdFreeServiceFunc            mDnssdFreeServiceFunc;
#endif

        DnssdRegisterServiceFunc        mDnssdRegisterServiceFunc;
        DnssdUnregisterServiceFunc        mDnssdUnregisterServiceFunc;
		DnssdStartDiscoveryFunc mDnssdStartDiscoveryFunc;
		DnssdStopDiscoveryFunc mDnssdStopDiscoveryFunc;
		DnssdStartResolveFunc mDnssdStartResolveFunc;
		DnssdStopResolveFunc mDnssdStopResolveFunc;

        // dnssd service
        DnssdServicePtr mDnssdServicePtr;

#ifdef DNSSDUWP_USE_LEGACY
        // dnssd service watcher
        DnssdServiceWatcherPtr mDnssdServiceWatcherPtr;
#endif

		DnssdServiceDiscoveryPtr mDnssdServiceDiscoveryPtr;
		DnssdServiceResolverPtr mDnssdServiceResolverPtr;

        // dnssd DLL Handle
        HINSTANCE mDllHandle;
    };
};

