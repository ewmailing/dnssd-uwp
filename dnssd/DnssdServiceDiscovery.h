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

#include <string>
#include <functional>
#include <map>
#include <vector>
#include <mutex>

#include "dnssd_uwp.h"

#ifdef DNSSDUWP_USE_LEGACY
#else
#include "DnssdUtils.h"
#endif

// I tried to use batched updates since we have something called OnServiceEnumerationCompleted
// which implied to me that we could send a 'more coming' flag like on some other platforms.
// But when I tried it, I seem to get an initial burst of events, and then therer is a 15-30 second wait before the completed callback happens.
// The point of the flag is to allow those who want to avoid updating the GUI too much if there is still data to come so you don't waste time redrawing.
// But I don't think the 15+ second delay is worth it.
// But just in case, here is #define to turn it on
// Set to 1
#define DNSSDUWP_USE_BATCHED_UPDATES 0

namespace dnssd_uwp
{
#if DNSSDUWP_USE_BATCHED_UPDATES
	struct DnssdDiscoveryCallbackInfo;
#endif
	class DnssdServiceDiscoveryWrapper;

    ref class DnssdServiceDiscovery;

    // C++ dsssd service changed callback
//    typedef std::function<void(DnssdServiceDiscovery^ watcher, DnssdServiceUpdateType update, DnssdServiceInfoPtr info)> DnssdServiceChangedCallbackType;

    // WinRT Delegate
 //   delegate void DnssdServiceUpdateHandler(DnssdServiceDiscovery^ sender, DnssdServiceUpdateType update, DnssdServiceInfoPtr info);

    ref class DnssdServiceDiscoveryInstance sealed
    {
    public:
        DnssdServiceDiscoveryInstance()
        {
            mChanged = false;
            mType = DnssdServiceUpdateType::ServiceAdded;
        }

    internal:
//        Platform::String^ mHost;
//        Platform::String^ mPort;
        Platform::String^ mInstanceName;
        Platform::String^ mServiceType;
        Platform::String^ mDomain;
        Platform::String^ mId;
        DnssdServiceUpdateType mType;
        bool mChanged;
    };

    ref class DnssdServiceDiscovery
    {
    public:
        virtual ~DnssdServiceDiscovery();


    internal:
        DnssdErrorType Initialize();
        void Stop();

        void RemoveDnssdServiceChangedCallback() {
            mDnssdServiceChangedCallback = nullptr;
        };

        //event DnssdServiceUpdateHandler^ mPortUpdateEventHander;
        
        // needs to be internal as DnssdServiceChangedCallbackType is not a WinRT type
        void SetDnssdServiceChangedCallback(const DnssdServiceDiscoveryChangedCallback callback) {
            mDnssdServiceChangedCallback = callback;
        };
       
        // Constructor needs to be internal as this is an unsealed ref base class
        DnssdServiceDiscovery(const char* service_type, const char* domain, DnssdServiceDiscoveryChangedCallback callback, void* user_data);

		void SetWrapperPtr(DnssdServiceDiscoveryWrapper* wrapper_ptr)
		{
			mWrapperPtr = wrapper_ptr;
		}
		DnssdServiceDiscoveryWrapper* GetWrapperPtr()
		{
			return mWrapperPtr;
		}

    private:
        void OnServiceAdded(Windows::Devices::Enumeration::DeviceWatcher^ sender, Windows::Devices::Enumeration::DeviceInformation^ args);
        void OnServiceRemoved(Windows::Devices::Enumeration::DeviceWatcher^ sender, Windows::Devices::Enumeration::DeviceInformationUpdate^ args);
        void OnServiceUpdated(Windows::Devices::Enumeration::DeviceWatcher^ sender, Windows::Devices::Enumeration::DeviceInformationUpdate^ args);
        void OnServiceEnumerationCompleted(Windows::Devices::Enumeration::DeviceWatcher^ sender, Platform::Object^ args);
        void OnServiceEnumerationStopped(Windows::Devices::Enumeration::DeviceWatcher^ sender, Platform::Object^ args);
        void UpdateDnssdService(DnssdServiceUpdateType type, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::Object^>^ props, Platform::String^ serviceId);
        void OnDnssdServiceUpdated(DnssdServiceDiscoveryInstance^ info);
		/*
		Windows::Foundation::TypedEventHandler<Windows::Devices::Enumeration::DeviceWatcher ^, Windows::Devices::Enumeration::DeviceInformation ^>^ mDelegateAdded;
		Windows::Foundation::TypedEventHandler<Windows::Devices::Enumeration::DeviceWatcher ^, Windows::Devices::Enumeration::DeviceInformationUpdate ^>^ mDelegateRemoved;
		Windows::Foundation::TypedEventHandler<Windows::Devices::Enumeration::DeviceWatcher ^, Windows::Devices::Enumeration::DeviceInformationUpdate ^>^ mDelegateUpdated;
		Windows::Foundation::TypedEventHandler<Windows::Devices::Enumeration::DeviceWatcher ^, Platform::Object ^>^ mDelegateComplated;
		Windows::Foundation::TypedEventHandler<Windows::Devices::Enumeration::DeviceWatcher ^, Platform::Object ^>^ mDelegateStopped;
		*/
		Windows::Foundation::EventRegistrationToken mDelegateAdded;
		Windows::Foundation::EventRegistrationToken mDelegateRemoved;
		Windows::Foundation::EventRegistrationToken mDelegateUpdated;
		Windows::Foundation::EventRegistrationToken mDelegateCompleted;
		Windows::Foundation::EventRegistrationToken mDelegateStopped;

		// helper for callback flags: 1 is added, 0 is removed
		uint32_t ComputeFlag(DnssdServiceUpdateType updateType, bool more_coming = false);


        Windows::Devices::Enumeration::DeviceWatcher^ mServiceWatcher;

        DnssdServiceDiscoveryChangedCallback mDnssdServiceChangedCallback;
		void* mUserData;

        std::map<Platform::String^, DnssdServiceDiscoveryInstance^> mServices;
        Platform::String^ mServiceType;
        Platform::String^ mDomain;

#if DNSSDUWP_USE_BATCHED_UPDATES
        std::vector<DnssdDiscoveryCallbackInfo> mPendingServicesToCallback;
#endif

		std::mutex mLock;
		bool mRunning;

		DnssdServiceDiscoveryWrapper* mWrapperPtr;
    };

#if DNSSDUWP_USE_BATCHED_UPDATES
	struct DnssdDiscoveryCallbackInfo
	{
//		const char* service_name, const char* service_type, uint32_t flags, DnssdErrorType error_code, void* user_data
		std::string serviceName;
		std::string serviceType;
		DnssdServiceUpdateType updateType;
		DnssdErrorType errorType;
		void* userData;
	};
#endif

    class DnssdServiceDiscoveryWrapper
    {
    public:
        DnssdServiceDiscoveryWrapper(DnssdServiceDiscovery ^ watcher)
            : mWatcher(watcher)
        {
        }

		virtual ~DnssdServiceDiscoveryWrapper()
		{
			mWatcher->Stop();
			mWatcher = nullptr;
		}

        DnssdServiceDiscovery^ GetWatcher() {
            return mWatcher;
        }

    private:
        DnssdServiceDiscovery^ mWatcher;
    };
};




