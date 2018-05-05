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
#ifdef DNSSDUWP_USE_LEGACY
#else
#include "DnssdUtils.h"
#endif
#include <string>

namespace dnssd_uwp
{
	class DnssdServiceWrapper;

    ref class DnssdService sealed
    {
    public:
        virtual ~DnssdService();

    internal:
#ifdef DNSSDUWP_USE_LEGACY
		DnssdService(const std::string& name, const std::string& port);
        DnssdErrorType Start();
#endif
        DnssdService(const std::string& service_name, const std::string& service_type, const char* domain, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data);
        DnssdErrorType StartRegistration();
        void Stop();

		void SetWrapperPtr(DnssdServiceWrapper* wrapper_ptr)
		{
			mWrapperPtr = wrapper_ptr;
		}

    private:
        void OnConnect(Windows::Networking::Sockets::StreamSocketListener^ sender, Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs ^ args);
        Platform::String^ mServiceName;
        Platform::String^ mServiceType;
        Platform::String^ mDomain;
        Platform::String^ mPort;
        Windows::Networking::ServiceDiscovery::Dnssd::DnssdServiceInstance^ mService;
        Windows::Networking::Sockets::StreamSocketListener^ mSocket;
        Windows::Foundation::EventRegistrationToken mSocketToken;

		std::string mServiceNameStr;
		std::string mServiceTypeStr;
		DnssdRegisterCallback mRegisterCallback;
		void* mUserData;
		uint16_t mNetworkPort;
		std::vector<char> mTxtRecordVector;
		uint16_t mTxtRecordLength;

		DnssdServiceWrapper* mWrapperPtr;

    };

    class DnssdServiceWrapper
    {
    public:
        DnssdServiceWrapper(DnssdService ^ service)
            : mService(service)
        {
        }

		virtual ~DnssdServiceWrapper()
		{
			mService = nullptr;
		}

        DnssdService^ GetService() {
            return mService;
        }

    private:
        DnssdService^ mService;
    };
};

