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

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DNSSD_EXPORT)
#define DNSSD_API extern __declspec(dllexport)
#else
#define DNSSD_API extern __declspec(dllimport)
#endif

#include <stdint.h>

#ifdef DNSSDUWP_USE_LEGACY
    enum DnssdServiceUpdateType { ServiceAdded, ServiceUpdated, ServiceRemoved };
#endif
    enum DnssdErrorType {
        DNSSD_NO_ERROR = 0,                         // no error
        DNSSD_WINDOWS_RUNTIME_INITIALIZATION_ERROR, // unable to initialize Windows Runtime
        DNSSD_WINDOWS_VERSION_ERROR,                // version of Windows does not support Windows::Networking::ServiceDiscovery::Dnssd api
        DNSSD_SERVICEWATCHER_INITIALIZATION_ERROR,  // error initializing dnssd service watcher
        DNSSD_LOCAL_HOSTNAME_NOT_FOUND_ERROR,       // dns service was not able to find a local hostname
        DNSSD_SERVICE_ALREADY_EXISTS_ERROR,         // dns service has already been started
        DNSSD_SERVICE_INITIALIZATION_ERROR,         // error starting dnssd service 
        DNSSD_INVALID_SERVICE_NAME_ERROR,           // Invalid service name during registration
        DNSSD_SERVICE_SERVER_ERROR,                 // Dnssd server error during registration
        DNSSD_SERVICE_SECURITY_ERROR,               // Dnssd security error during registration
        DNSSD_INVALID_PARAMETER_ERROR,
        DNSSD_MEMORY_ERROR,
        DNSSD_DLL_MISSING_ERROR,                    // dnssd dll not found
        DNSSD_UNSPECIFIED_ERROR
    };
    typedef enum DnssdErrorType DnssdErrorType;

    typedef void* DnssdServicePtr;
    typedef void* DnssdServiceDiscoveryPtr;
    typedef void* DnssdServiceResolverPtr;

#ifdef DNSSDUWP_USE_LEGACY
    typedef void* DnssdServiceWatcherPtr;
    // dnssd service info
    typedef struct 
    {
        const char* id;
        const char* instanceName;
        const char* host;
        const char* port;
    } DnssdServiceInfo;

    typedef DnssdServiceInfo* DnssdServiceInfoPtr;
#endif
    // dnssd functions
    typedef DnssdErrorType(__cdecl *DnssdInitializeFunc)(void);
    DNSSD_API DnssdErrorType __cdecl dnssd_initialize(void);

	typedef DnssdErrorType(__cdecl *DnssdUninitializeFunc)(void);
    DNSSD_API void __cdecl dnssd_uninitialize(void);

#ifdef DNSSDUWP_USE_LEGACY
    // dnssd service watcher functions

    // dnssd service watcher changed callback
    typedef void(*DnssdServiceChangedCallback) (const DnssdServiceWatcherPtr portWatcher, DnssdServiceUpdateType update, DnssdServiceInfoPtr info);

    // dnssd service watcher create function
    typedef  DnssdErrorType(__cdecl *DnssdCreateServiceWatcherFunc)(const char* serviceName, DnssdServiceChangedCallback callback, DnssdServiceWatcherPtr *serviceWatcher);
    DNSSD_API DnssdErrorType __cdecl dnssd_create_service_watcher(const char* serviceName, DnssdServiceChangedCallback callback, DnssdServiceWatcherPtr * serviceWatcher);

    typedef void(__cdecl *DnssdFreeServiceWatcherFunc)(DnssdServiceWatcherPtr serviceWatcher);
    DNSSD_API void __cdecl dnssd_free_service_watcher(DnssdServiceWatcherPtr serviceWatcher);

    // dnssd service create function
    typedef  DnssdErrorType(__cdecl *DnssdCreateServiceFunc)(const char* serviceName, const char* port, DnssdServicePtr *service);
    DNSSD_API DnssdErrorType __cdecl dnssd_create_service(const char* serviceName, const char* port, DnssdServicePtr *service);
    typedef void(__cdecl *DnssdFreeServiceFunc)(DnssdServicePtr service);
    DNSSD_API void __cdecl dnssd_free_service(DnssdServicePtr service);
#endif


    // dnssd register callback
	typedef void(*DnssdRegisterCallback) (DnssdServicePtr service_ptr, const char* service_name, const char* service_type, const char* domain, uint16_t network_port, DnssdErrorType error_code, void* user_data);
    typedef  DnssdErrorType(__cdecl *DnssdRegisterServiceFunc)(const char* service_name, const char* service_type, const char* domain, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data, DnssdServicePtr* out_service_ptr);
	DNSSD_API DnssdErrorType __cdecl dnssd_register_service(const char* service_name, const char* service_type, const char* domain, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data, DnssdServicePtr* out_service_ptr);

    typedef  DnssdErrorType(__cdecl *DnssdUnregisterServiceFunc)(DnssdServicePtr service_ptr);
    DNSSD_API void __cdecl dnssd_unregister_service(DnssdServicePtr service);

	
    // dnssd service discovery changed callback
    typedef void(*DnssdServiceDiscoveryChangedCallback) (DnssdServiceDiscoveryPtr service_discovery, const char* service_name, const char* domain, const char* service_type, uint32_t flags, DnssdErrorType error_code, void* user_data);
	typedef  DnssdErrorType(__cdecl *DnssdStartDiscoveryFunc)(const char* service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback_function, void* user_data, DnssdServiceDiscoveryPtr* out_discovery_ptr);
	DNSSD_API DnssdErrorType __cdecl dnssd_start_discovery(const char* service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback_function, void* user_data, DnssdServiceDiscoveryPtr* out_discovery_ptr);

	typedef void(__cdecl *DnssdStopDiscoveryFunc)(DnssdServiceDiscoveryPtr discovery_ptr);
	DNSSD_API void __cdecl dnssd_stop_discovery(DnssdServiceDiscoveryPtr discovery_ptr);


	// dnssd service resolve changed callback
    typedef void(*DnssdServiceResolverChangedCallback) (DnssdServiceResolverPtr service_resolver, const char* service_name, const char* service_type, const char* domain, const char* full_name, const char* host_target, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdErrorType error_code, void* user_data);
	typedef  DnssdErrorType(__cdecl *DnssdStartResolveFunc)(const char* service_name, const char* service_type, const char* domain, DnssdServiceResolverChangedCallback callback_function, void* user_data, DnssdServiceResolverPtr* out_resolver_ptr);
	DNSSD_API DnssdErrorType __cdecl dnssd_start_resolve(const char* service_name, const char* service_type, const char* domain, DnssdServiceResolverChangedCallback callback_function, void* user_data, DnssdServiceResolverPtr* out_resolver_ptr);

	typedef void(__cdecl *DnssdStopResolveFunc)(DnssdServiceResolverPtr resolver_ptr);
	DNSSD_API void __cdecl dnssd_stop_resolve(DnssdServiceResolverPtr resolver_ptr);


#ifdef __cplusplus
}
#endif
