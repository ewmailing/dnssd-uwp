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
#include "DnssdService.h"
#include "DnssdServiceWatcher.h"
#include "DnssdServiceDiscovery.h"
#include "DnssdServiceResolver.h"
#include <wrl\wrappers\corewrappers.h>


namespace dnssd_uwp
{
    static bool mInitialized = false;

    DNSSD_API DnssdErrorType dnssd_initialize()
    {
        DnssdErrorType result = DNSSD_NO_ERROR;
        HRESULT hr = S_OK;

        // Initialize the Windows Runtime.
        if (!mInitialized)
        {
            HRESULT hr = ::Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
            if (!SUCCEEDED(hr))
            {
                result = DNSSD_WINDOWS_RUNTIME_INITIALIZATION_ERROR;
            }
            else
            {
                mInitialized = true;
            }
        }
        
        return result;
    }

#ifdef DNSSDUWP_USE_LEGACY
    DNSSD_API DnssdErrorType dnssd_create_service_watcher(const char* serviceName, DnssdServiceChangedCallback callback, DnssdServiceWatcherPtr *serviceWatcher)
    {
        DnssdErrorType result = DNSSD_NO_ERROR;

        *serviceWatcher = nullptr;

        auto watcher = ref new DnssdServiceWatcher(serviceName, callback);
        result = watcher->Initialize();

        if (result != DNSSD_NO_ERROR)
        {
            *serviceWatcher = nullptr;
            watcher = nullptr;
        }
        else
        {
            auto wrapper = new DnssdServiceWatcherWrapper(watcher);
            *serviceWatcher = (DnssdServiceWatcherPtr)wrapper;
        }

        return result;
    }

    DNSSD_API void dnssd_free_service_watcher(DnssdServiceWatcherPtr serviceWatcher)
    {
        if (serviceWatcher)
        {
            DnssdServiceWatcherWrapper* watcher = (DnssdServiceWatcherWrapper*)serviceWatcher;
            delete watcher;
        }
    }

    DNSSD_API DnssdErrorType dnssd_create_service(const char* serviceName, const char* port, DnssdServicePtr *service)
    {
        DnssdErrorType result = DNSSD_NO_ERROR;

        *service = nullptr;

        auto s = ref new DnssdService(serviceName, port);
        result = s->Start();

        if (result != DNSSD_NO_ERROR)
        {
            *service = nullptr;
            s = nullptr;
        }
        else
        {
            auto wrapper = new DnssdServiceWrapper(s);
            *service = (DnssdServicePtr)wrapper;
        }

        return result;
    }

    DNSSD_API void dnssd_free_service(DnssdServicePtr service)
    {
        if (service)
        {
            DnssdServiceWrapper* wrapper = (DnssdServiceWrapper*)service;
            delete wrapper;
        }
    }
#endif

	DNSSD_API DnssdErrorType dnssd_register_service(const char* service_name, const char* service_type, const char* domain, uint16_t port, DnssdRegisterCallback callback_function, void* user_data, DnssdServicePtr* out_service_ptr)
    {
        DnssdErrorType result = DNSSD_NO_ERROR;
		if( (NULL == service_type) || (NULL == service_name) )
		{
			result = DNSSD_INVALID_PARAMETER_ERROR;
			if(out_service_ptr != NULL)
			{
				*out_service_ptr = NULL;
			}
			return result;
		}

  //      *service = nullptr;

//        auto s = ref new DnssdService(service_name, service_type, port, callback_function, user_data);
        auto s = ref new DnssdService(std::string(service_name), std::string(service_type), domain, port, callback_function, user_data);
		// It appears we need to create a wrapper object to keep the ref rooted/counted, otherwise I think the object goes away.
		auto wrapper = new DnssdServiceWrapper(s);
		// The problem with having a wrapper object is that all the callbacks original inside the original class.
		// When the callbacks pass this object back through, we lose the wrapper object.
		// So this is an ugly hack to keep the wrapper object with the original object.
		// This just a pointer with no reference counting or ownership to avoid circular-references.
		s->SetWrapperPtr(wrapper);



        result = s->StartRegistration();
        if (result != DNSSD_NO_ERROR)
        {
			if(out_service_ptr != NULL)
			{
				*out_service_ptr = NULL;
			}
			s->SetWrapperPtr(nullptr);
			delete s;
			wrapper = nullptr;
            s = nullptr;
        }
        else
        {
			if(out_service_ptr != NULL)
			{
				*out_service_ptr = (DnssdServicePtr)wrapper;
			}
        }

        return result;
    }

	DNSSD_API void dnssd_unregister_service(DnssdServicePtr wrapper_ptr)
	{
		if(nullptr != wrapper_ptr)
		{
			DnssdServiceWrapper* wrapper = (DnssdServiceWrapper*)wrapper_ptr;
			delete wrapper;
		}
	}

	
	DNSSD_API DnssdErrorType dnssd_start_discovery(const char* service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback_function, void* user_data, DnssdServiceDiscoveryPtr* out_discovery_ptr)
    {
		DnssdErrorType result = DNSSD_NO_ERROR;

		if(NULL == service_type)
		{
			result = DNSSD_INVALID_PARAMETER_ERROR;
			if(out_discovery_ptr != NULL)
			{
				*out_discovery_ptr = NULL;
			}
			return result;
		}
     //   *serviceWatcher = nullptr;

		auto watcher = ref new DnssdServiceDiscovery(service_type, domain, callback_function, user_data);
		// It appears we need to create a wrapper object to keep the ref rooted/counted, otherwise I think the object goes away.
		auto wrapper = new DnssdServiceDiscoveryWrapper(watcher);
		// The problem with having a wrapper object is that all the callbacks original inside the original class.
		// When the callbacks pass this object back through, we lose the wrapper object.
		// So this is an ugly hack to keep the wrapper object with the original object.
		// This just a pointer with no reference counting or ownership to avoid circular-references.
		watcher->SetWrapperPtr(wrapper);


		result = watcher->Initialize();

        if (result != DNSSD_NO_ERROR)
        {
			if(out_discovery_ptr != NULL)
			{
				*out_discovery_ptr = NULL;
			}
			watcher->SetWrapperPtr(nullptr);
			delete wrapper;
			wrapper = nullptr;
            watcher = nullptr;
        }
        else
        {
			if(out_discovery_ptr != NULL)
			{
				*out_discovery_ptr = (DnssdServiceDiscoveryPtr)wrapper;
			}
        }

        return result;
    }

	DNSSD_API void dnssd_stop_discovery(DnssdServiceDiscoveryPtr wrapper_ptr)
	{
		if(nullptr != wrapper_ptr)
		{
			DnssdServiceDiscoveryWrapper* wrapper = (DnssdServiceDiscoveryWrapper*)wrapper_ptr;
			delete wrapper;
		}
	}

	
	DNSSD_API DnssdErrorType dnssd_start_resolve(const char* service_name, const char* service_type, const char* domain, DnssdServiceResolverChangedCallback callback_function, void* user_data, DnssdServiceResolverPtr* out_resolve_ptr)
    {
		DnssdErrorType result = DNSSD_NO_ERROR;
		if( (NULL == service_type) || (NULL == service_name) )
		{
			result = DNSSD_INVALID_PARAMETER_ERROR;
			if(out_resolve_ptr != NULL)
			{
				*out_resolve_ptr = NULL;
			}
			return result;
		}
     //   *serviceWatcher = nullptr;

		auto watcher = ref new DnssdServiceResolver(service_name, service_type, domain, callback_function, user_data);
		// It appears we need to create a wrapper object to keep the ref rooted/counted, otherwise I think the object goes away.
		auto wrapper = new DnssdServiceResolverWrapper(watcher);
		// The problem with having a wrapper object is that all the callbacks original inside the original class.
		// When the callbacks pass this object back through, we lose the wrapper object.
		// So this is an ugly hack to keep the wrapper object with the original object.
		// This just a pointer with no reference counting or ownership to avoid circular-references.
		watcher->SetWrapperPtr(wrapper);


		result = watcher->Initialize();

        if (result != DNSSD_NO_ERROR)
        {
			if(out_resolve_ptr != NULL)
			{
				*out_resolve_ptr = NULL;
			}
			watcher->SetWrapperPtr(nullptr);
			delete wrapper;
			wrapper = nullptr;
            watcher = nullptr;
        }
        else
        {
			if(out_resolve_ptr != NULL)
			{
				*out_resolve_ptr = (DnssdServiceResolverPtr)wrapper;
			}
        }

        return result;
    }

	DNSSD_API void dnssd_stop_resolve(DnssdServiceResolverPtr wrapper_ptr)
	{
		if(nullptr != wrapper_ptr)
		{
			DnssdServiceResolverWrapper* wrapper = (DnssdServiceResolverWrapper*)wrapper_ptr;
			delete wrapper;
		}
	}
}

