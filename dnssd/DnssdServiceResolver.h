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
#include <set>

#include "dnssd_uwp.h"
#ifdef DNSSDUWP_USE_LEGACY
#else
#include "DnssdUtils.h"
#endif

namespace dnssd_uwp
{
	class DnssdServiceResolverWrapper;

    ref class DnssdServiceResolver;

    // C++ dsssd service changed callback
//    typedef std::function<void(DnssdServiceResolver^ watcher, DnssdServiceUpdateType update, DnssdServiceInfoPtr info)> DnssdServiceChangedCallbackType;

    // WinRT Delegate
 //   delegate void DnssdServiceUpdateHandler(DnssdServiceResolver^ sender, DnssdServiceUpdateType update, DnssdServiceInfoPtr info);

    ref class DnssdServiceResolverInstance sealed
    {
    public:
        DnssdServiceResolverInstance()
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
        Platform::String^ mFullName;
        Platform::String^ mId;
        DnssdServiceUpdateType mType;
		uint16_t networkPort;
		Platform::String^ mHostTarget;
//		Platform::String^ mTxtRecord;
		std::vector<char> mCombinedTxtRecord;

		std::set<std::string> mIpAddresses;

        bool mChanged;
    };

    ref class DnssdServiceResolver
    {
    public:
        virtual ~DnssdServiceResolver();


    internal:
        DnssdErrorType Initialize();

        void RemoveDnssdServiceChangedCallback() {
            mDnssdServiceChangedCallback = nullptr;
        };

        //event DnssdServiceUpdateHandler^ mPortUpdateEventHander;
        
        // needs to be internal as DnssdServiceChangedCallbackType is not a WinRT type
        void SetDnssdServiceChangedCallback(const DnssdServiceResolverChangedCallback callback) {
            mDnssdServiceChangedCallback = callback;
        };
       
        // Constructor needs to be internal as this is an unsealed ref base class
        DnssdServiceResolver(const char* service_name, const char* service_type, const char* domain, DnssdServiceResolverChangedCallback callback, void* user_data);

		void SetWrapperPtr(DnssdServiceResolverWrapper* wrapper_ptr)
		{
			mWrapperPtr = wrapper_ptr;
		}
		DnssdServiceResolverWrapper* GetWrapperPtr()
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
        void OnDnssdServiceUpdated(DnssdServiceResolverInstance^ info, const std::string& ip_address);

        Windows::Devices::Enumeration::DeviceWatcher^ mServiceWatcher;

        DnssdServiceResolverChangedCallback mDnssdServiceChangedCallback;
		void* mUserData;

        std::map<Platform::String^, DnssdServiceResolverInstance^> mServices;
        Platform::String^ mServiceName;
        Platform::String^ mServiceType;
        Platform::String^ mDomain;



		bool mRunning;

		DnssdServiceResolverWrapper* mWrapperPtr;
    };


    class DnssdServiceResolverWrapper
    {
    public:
        DnssdServiceResolverWrapper(DnssdServiceResolver ^ watcher)
            : mWatcher(watcher)
        {
        }

		virtual ~DnssdServiceResolverWrapper()
		{
			mWatcher = nullptr;
		}

        DnssdServiceResolver^ GetWatcher() {
            return mWatcher;
        }

    private:
        DnssdServiceResolver^ mWatcher;
    };
};




