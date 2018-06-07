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

#include "DnssdServiceDiscovery.h"
#include "DnssdUtils.h"
#include <algorithm>
#include <vector>
#include <collection.h>
#include <cvt/wstring>
#include <codecvt>
#include <ppltasks.h>

using namespace Platform::Collections;
using namespace Windows::Networking::ServiceDiscovery::Dnssd;
using namespace Windows::Networking;
using namespace Windows::Networking::Sockets;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;
using namespace concurrency;

namespace dnssd_uwp
{

    DnssdServiceDiscovery::DnssdServiceDiscovery(const char* service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback, void* user_data)
        : mDnssdServiceChangedCallback(callback),
		mUserData(user_data)
        , mRunning(false)
		, mWrapperPtr(nullptr)
    {
        mServiceType = StringToPlatformString(RemoveTrailingDotIfNecessary(service_type));
		if( (NULL == domain) || (0==strcmp(domain, "")) )
		{
			mDomain = nullptr;
		}
		else
		{
			mDomain = StringToPlatformString(RemoveTrailingDotIfNecessary(domain));
		}
    }

    DnssdServiceDiscovery::~DnssdServiceDiscovery()
    {
        if (mServiceWatcher)
        {
			mServiceWatcher->Added -= mDelegateAdded;
            mServiceWatcher->Removed -= mDelegateRemoved;
            mServiceWatcher->Updated -= mDelegateUpdated;
            mServiceWatcher->EnumerationCompleted -= mDelegateCompleted;
            mServiceWatcher->Stopped -= mDelegateStopped;

            mRunning = false;
			// I think I get an exception if I call Stop() twice.
			if(DeviceWatcherStatus::Stopped != mServiceWatcher->Status)
			{
				mServiceWatcher->Stop();
			}
			// I'm getting a mysterious exception from another thread in device enumeration
			// (testing time-out and freeing resolve in callback)
			// I think if this destructor finishes while in the middle of enumeration, this triggers the crash.
			// So wait/block until enumeration is completely stopped.
			while(DeviceWatcherStatus::Stopped != mServiceWatcher->Status)
			{
			}
			 

            mServiceWatcher = nullptr;
			mWrapperPtr = nullptr; // you are expected to explictly delete the wrapper which will cause this instance to be released
        }
    }

    DnssdErrorType DnssdServiceDiscovery::Initialize()
    {
        auto task = create_task(create_async([this]
        {
            /// <summary>
            /// All of the properties that will be returned when a DNS-SD instance has been found. 
            /// </summary>

            Vector<Platform::String^>^ propertyKeys = ref new Vector<Platform::String^>();
 //          propertyKeys->Append(L"System.Devices.Dnssd.HostName");
            propertyKeys->Append(L"System.Devices.Dnssd.ServiceName");
            propertyKeys->Append(L"System.Devices.Dnssd.InstanceName");
            propertyKeys->Append(L"System.Devices.Dnssd.Domain");
 //           propertyKeys->Append(L"System.Devices.IpAddress");
 //           propertyKeys->Append(L"System.Devices.Dnssd.PortNumber");

            Platform::String^ aqsQueryString;
			// I'm reserving the possibility that a NULL domain could eventually become a wildcard for multiple domains, e.g. wide-area-Bonjour.
			if(mDomain != nullptr)
			{
				aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54} AND "
					+ L"System.Devices.Dnssd.Domain:=\"" + mDomain + L"\""
					+ L" AND System.Devices.Dnssd.ServiceName:=\"" + mServiceType + L"\"";
			}
			else
			{
				aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54}"
					+ L" AND System.Devices.Dnssd.ServiceName:=\"" + mServiceType + L"\"";
			}
//                "System.Devices.Dnssd.Domain:=\"local\" AND System.Devices.Dnssd.ServiceName:=\"" + mServiceType + "\"";
 //           aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54} AND " +
 //               "System.Devices.Dnssd.Domain:=\"local\" AND System.Devices.Dnssd.ServiceName:=\"" + L"_daap._tcp" + "\"";

            mServiceWatcher = DeviceInformation::CreateWatcher(aqsQueryString, propertyKeys, DeviceInformationKind::AssociationEndpointService);

            // wire up event handlers
			mDelegateAdded = mServiceWatcher->Added += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformation ^>(this, &DnssdServiceDiscovery::OnServiceAdded);
			mDelegateRemoved = mServiceWatcher->Removed += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>(this, &DnssdServiceDiscovery::OnServiceRemoved);
			mDelegateUpdated = mServiceWatcher->Updated += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>(this, &DnssdServiceDiscovery::OnServiceUpdated);
			mDelegateCompleted = mServiceWatcher->EnumerationCompleted += ref new Windows::Foundation::TypedEventHandler<DeviceWatcher ^, Platform::Object ^>(this, &DnssdServiceDiscovery::OnServiceEnumerationCompleted);
			mDelegateStopped = mServiceWatcher->Stopped += ref new Windows::Foundation::TypedEventHandler<DeviceWatcher ^, Platform::Object ^>(this, &DnssdServiceDiscovery::OnServiceEnumerationStopped);

            // start watching for dnssd services
            mServiceWatcher->Start();
            mRunning = true;
            auto status = mServiceWatcher->Status;
//			return 0;
        }));

		 std::string service_type = PlatformStringToString(mServiceType);
 		 std::string domain;
		 bool is_domain_null = false;
		 if(mDomain != nullptr)
		 {
			domain = PlatformStringToString(mDomain);
			is_domain_null = false;
		 }
		 else
		 {
			is_domain_null = true;
		 }

		DnssdServiceDiscoveryChangedCallback discovery_callback = mDnssdServiceChangedCallback;
		void* user_data = mUserData;
		DnssdServiceDiscoveryWrapper* wrapper_ptr = mWrapperPtr;

			// I don't think I am doing the task<> parameter correctly.
		task.then([wrapper_ptr, service_type, domain, is_domain_null, discovery_callback, user_data](auto task_result)
//		task.then([task, this, service_type, discovery_callback, user_data](auto task_result)
		{
			const char* domain_c_str = NULL;
			if(is_domain_null)
			{
				// WARNING: If this code or the MS backend ever changes so that NULL for the domain searches things other than local,
				// then hardcoding this to local. will not work and we must figure out what that domain is.
				domain_c_str = "local.";
			}
			else
			{
				domain_c_str = AppendTrailingDotIfNecessary(domain).c_str();
			}

			try
			{
				// wait for port enumeration to complete
//				task.get(); // will throw any exceptions from above task
				task_result.get(); // will throw any exceptions from above task
//				return DNSSD_NO_ERROR;

			}
			catch (Platform::Exception^ ex)
			{
				discovery_callback(
					(DnssdServiceDiscoveryPtr)wrapper_ptr, 
					NULL,
					AppendTrailingDotIfNecessary(service_type).c_str(),
					domain_c_str, // domain
					0,
					DNSSD_SERVICEWATCHER_INITIALIZATION_ERROR, 
					user_data
				);

//				return DNSSD_SERVICEWATCHER_INITIALIZATION_ERROR;
			}
		}
		);
		return DNSSD_NO_ERROR;

    }

	void DnssdServiceDiscovery::Stop()
	{
		// grrr. I think I hit a deadlock in a shutdown condition.
//		mLock.lock();
		mRunning = false;
//		mLock.unlock();
	}


    void DnssdServiceDiscovery::UpdateDnssdService(DnssdServiceUpdateType type, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::Object^>^ props, Platform::String^ serviceId)
    {
		mLock.lock();

//        auto box = safe_cast<Platform::IBoxArray<Platform::String^>^>(props->Lookup("System.Devices.IpAddress"));
//        Platform::String^ host = box->Value->get(0);
//        Platform::String^ port = props->Lookup("System.Devices.Dnssd.PortNumber")->ToString();
        Platform::String^ name = props->Lookup("System.Devices.Dnssd.InstanceName")->ToString();
        Platform::String^ service_type = props->Lookup("System.Devices.Dnssd.ServiceName")->ToString();
        Platform::String^ domain = props->Lookup("System.Devices.Dnssd.Domain")->ToString();

        auto it = mServices.find(serviceId);
        if (it != mServices.end()) // service was previously found. Update the info and report change if necessary
        {
            auto info = it->second;

			// NOTE: The original code worked differently and merged discovery+resolve into a single function.
			// Additionally, the Win API has separate add, remove, update callbacks.
			// I don't think we should ever get an update callback that is meaningful.
			// However, I'm nervous about disabling the update callback because the documentation strongly suggests I should have all 3 
			// and it is unclear under what circumstances an update callback will fire. 
            if (info->mInstanceName != name)
            {
                info->mInstanceName = name;
                info->mChanged = true;
            }
            info->mType = DnssdServiceUpdateType::ServiceUpdated;

            if (info->mChanged)
            {
                // report the updated service
                OnDnssdServiceUpdated(info);
            }
        }
        else // add it to the service map
        {
            DnssdServiceDiscoveryInstance^ info = ref new DnssdServiceDiscoveryInstance;
            info->mId = serviceId;
 //           info->mHost = host;
//            info->mPort = port;
            info->mInstanceName = name;
            info->mServiceType = service_type;
            info->mDomain = domain;
            info->mType = DnssdServiceUpdateType::ServiceAdded;
            mServices[serviceId] = info;

            // report the new service
            OnDnssdServiceUpdated(info);
        }

		mLock.unlock();

    }

	// don't lock because all the callers already locked
    void DnssdServiceDiscovery::OnDnssdServiceUpdated(DnssdServiceDiscoveryInstance^ info)
    {

		std::string instanceName = PlatformStringToString(info->mInstanceName);
        std::string service_type = AppendTrailingDotIfNecessary(PlatformStringToString(info->mServiceType));
		std::string domain;
		const char* domain_c_str = NULL;
		if(info->mDomain != nullptr)
		{
			domain = AppendTrailingDotIfNecessary(PlatformStringToString(info->mDomain));
		}
		else
		{
			// WARNING: If this code or the MS backend ever changes so that NULL for the domain searches things other than local,
			// then hardcoding this to local. will not work and we must figure out what that domain is.
			domain = "local.";
		}
		domain_c_str = domain.c_str();



#if DNSSDUWP_USE_BATCHED_UPDATES
		DnssdDiscoveryCallbackInfo callback_info;
        callback_info.serviceName = instanceName;
        callback_info.serviceType = service_type;
		callback_info.updateType =  info->mType;
		callback_info.userData = mUserData;

#endif

        if(mRunning && mDnssdServiceChangedCallback != nullptr)
        {
//            mDnssdServiceChangedCallback(&wrapper, info->mType, &serviceInfo);
#if DNSSDUWP_USE_BATCHED_UPDATES
			mPendingServicesToCallback.push_back(callback_info);
#else
			uint32_t flags = ComputeFlag(info->mType, false);
			mDnssdServiceChangedCallback(
				(DnssdServiceDiscoveryPtr)mWrapperPtr, 
				instanceName.c_str(),
				service_type.c_str(),
				domain_c_str,
				flags, 
				DNSSD_NO_ERROR, // TODO: error type?
				mUserData
			);
#endif
        }
    }

    void DnssdServiceDiscovery::OnServiceAdded(DeviceWatcher^ sender, DeviceInformation^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceAdded, args->Properties, args->Id);
    }

    void DnssdServiceDiscovery::OnServiceUpdated(DeviceWatcher^ sender, DeviceInformationUpdate^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceUpdated, args->Properties, args->Id);
    }

    void DnssdServiceDiscovery::OnServiceRemoved(DeviceWatcher^ sender, DeviceInformationUpdate^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceRemoved, args->Properties, args->Id);
    }

    void DnssdServiceDiscovery::OnServiceEnumerationCompleted(DeviceWatcher^ sender, Platform::Object^ args)
    {
        // stop the service scanning. Service scanning will be restarted when OnServiceEnumerationStopped event is received
        mServiceWatcher->Stop();
    }

    void DnssdServiceDiscovery::OnServiceEnumerationStopped(Windows::Devices::Enumeration::DeviceWatcher^ sender, Platform::Object^ args)
    {
		if(mIsMarkedForDeletion)
		{
			try
			{
				DnssdServiceDiscoveryWrapper* wrapper_ptr = mWrapperPtr;
				mWrapperPtr = nullptr;
				delete wrapper_ptr;
			}
			catch(...)
			{
				OutputDebugStringW(L"Caught exception deleting DnssdServiceDiscoveryWrapper");
			}
			return;
		}

        // check if we are shutting down
        if (!mRunning)
        {
            return;
        }

		mLock.lock();

        std::vector<Platform::String^> removedServices;

        // iterate through the services list and remove any service that is marked for removal

		for (auto it = mServices.begin(); it != mServices.end(); ++it)
        {
            auto service = it->second;
            if (service->mType == DnssdServiceUpdateType::ServiceRemoved)
            {
                // report to the client the removed service
                OnDnssdServiceUpdated(service);
                removedServices.push_back(it->first);
            }
            else // prepare the service for the next search
            {
                // for each scan we mark each service as removed. 
                // If the scan finds the service again we will update its state accordingly
                service->mType = DnssdServiceUpdateType::ServiceRemoved;
                service->mChanged = false;
            }
        }

#if DNSSDUWP_USE_BATCHED_UPDATES
		if (mDnssdServiceChangedCallback != nullptr)
		{
			size_t number_of_items = mPendingServicesToCallback.size();
			size_t i = 0;

			if(number_of_items > 0)
			{
				for(i=0; i<number_of_items-1; i++)
				{
					// set the more data is coming flag
					auto &callback_info = mPendingServicesToCallback[i];
					uint32_t flags = ComputeFlag(info->mType, false);
					mDnssdServiceChangedCallback(
						(DnssdServiceDiscoveryPtr)this, 
						callback_info.serviceName.c_str(),
						AppendTrailingDotIfNecessary(callback_info.serviceType).c_str(), 
						AppendTrailingDotIfNecessary(callback_info.mDomain).c_str(), 
						flags, 
						callback_info.errorType, 
						mUserData
					);

				}
				// last element
//				for(; i<number_of_items; i++)
				{
					auto &callback_info = mPendingServicesToCallback[i];
					uint32_t flags = ComputeFlag(info->mType, false);
					mDnssdServiceChangedCallback(
						(DnssdServiceDiscoveryPtr)mWrapperPtr, 
						callback_info.serviceName.c_str(),
						AppendTrailingDotIfNecessary(callback_info.serviceType).c_str(), 
						AppendTrailingDotIfNecessary(callback_info.mDomain).c_str(), 
						flags, 
						callback_info.errorType, 
						mUserData
					);
				}
			}

        }
		mPendingServicesToCallback.clear();
#endif

        // remove stale services from the services map
        std::for_each(begin(removedServices), end(removedServices), [&](Platform::String^ id)
        {
            auto service = mServices.find(id);
            if (service != mServices.end())
            {
                mServices.erase(service);
            }
        });

        // restart the service scan
        mServiceWatcher->Start();

		mLock.unlock();
    }

	uint32_t DnssdServiceDiscovery::ComputeFlag(DnssdServiceUpdateType updateType, bool more_coming)
	{
		uint32_t flag_mask = 0;

		if(DnssdServiceUpdateType::ServiceAdded == updateType)
		{
			flag_mask |= DNSSD_FLAGS_ADD;
		}
		else if(DnssdServiceUpdateType::ServiceUpdated == updateType)
		{
			// I don't know what to do with this value since we are only supposed to get add or remove.
			// Make an add for now.
			OutputDebugStringW(L"WARNING: Unexpected 'update' to service (not an add or remove). Making an add.");
			flag_mask |= DNSSD_FLAGS_ADD;
		}
		// We don't give updated events for discovery because a service is either added or removed. There is no other data.
		else
		{
		}

		if(more_coming)
		{
			flag_mask |= 0x2; // DNSSD_FLAGS_MORECOMING
		}
		return flag_mask;
	}

}


