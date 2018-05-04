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
#include "Dnssdutils.h"
#include <ppltasks.h>
#include <stdlib.h>

using namespace dnssd_uwp;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Networking::ServiceDiscovery::Dnssd;

DnssdService::DnssdService(const std::string& name, const std::string& port)
{
    mServiceName = StringToPlatformString(name);
    mPort = StringToPlatformString(port);
}

DnssdService::DnssdService(const std::string& service_name, const std::string& service_type, const char* domain, uint16_t port, DnssdRegisterCallback callback_function, void* user_data)
{
	mServiceNameStr = service_name;
	mServiceTypeStr = service_type;

	mServiceName = StringToPlatformString(service_name);
	mServiceType = StringToPlatformString(service_type);
	if(domain != NULL)
	{
		mDomain = StringToPlatformString(domain);
	}
	else
	{
		mDomain = L"local";
	}
		


	mNetworkPort = port; // Do we want network or host order? Bonjour DNSService* does network, but I think everything else does host.
	mRegisterCallback = callback_function;
	mUserData = user_data;

//    mPort = StringToPlatformString(std::to_string(ntohs(port))); // Does Windows assume host order?
    mPort = StringToPlatformString(std::to_string(port)); // Does Windows assume host order?
}

DnssdService::~DnssdService()
{
    DnssdService::Stop();
}

DnssdErrorType DnssdService::Start()
{
    DnssdErrorType result = DNSSD_NO_ERROR;

    if (mService != nullptr)
    {
        return DNSSD_SERVICE_ALREADY_EXISTS_ERROR;
    }

    auto hostNames = NetworkInformation::GetHostNames();
    HostName^ hostName = nullptr;

    // find first HostName of Type == HostNameType.DomainName && RawName contains "local"
    for (unsigned int i = 0; i < hostNames->Size; ++i)
    {
        HostName^ n = hostNames->GetAt(i);
        if (n->Type == HostNameType::DomainName)
        {
            std::wstring temp(n->RawName->Data());
            auto found = temp.find(L"local");
            if (found != std::string::npos)
            {
                hostName = n;
                break;
            }
        }
    }

    if (hostName == nullptr)
    {
        return DNSSD_LOCAL_HOSTNAME_NOT_FOUND_ERROR;
    }

    auto task = create_task(create_async([this, hostName]
    {
        mSocket = ref new StreamSocketListener();
        mSocketToken = mSocket->ConnectionReceived += ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs ^>(this, &DnssdService::OnConnect);
        create_task(mSocket->BindServiceNameAsync(mPort)).get();
        unsigned short port = static_cast<unsigned short>(_wtoi(mSocket->Information->LocalPort->Data()));
        mService = ref new DnssdServiceInstance(L"dnssd." + mServiceName + L".local", hostName, port);


        return create_task(mService->RegisterStreamSocketListenerAsync(mSocket));
    }));

    try
    {
        // wait for dnssd service to start
        DnssdRegistrationResult^ reg = task.get(); // will also rethrow any exceptions from above task
        auto ip = reg->IPAddress; // this always seems to be NULL
        auto status = reg->Status;
        bool hasInstanceChanged = reg->HasInstanceNameChanged;

        if (status != DnssdRegistrationStatus::Success)
        {
            switch (status)
            {
                case DnssdRegistrationStatus::InvalidServiceName:
                    result = DNSSD_INVALID_SERVICE_NAME_ERROR;
                    break;
                case DnssdRegistrationStatus::SecurityError:
                    result = DNSSD_SERVICE_SECURITY_ERROR;
                    break;
                case DnssdRegistrationStatus::ServerError:
                    result = DNSSD_SERVICE_SERVER_ERROR;
                    break;
                default:
                    result = DNSSD_SERVICE_INITIALIZATION_ERROR;
                    break;
            }

        }
        return result;
    }
    catch (Platform::Exception^ ex)
    {
        result =  DNSSD_SERVICE_INITIALIZATION_ERROR;
    }

    return result;
}


DnssdErrorType DnssdService::StartRegistration()
{

    if (mService != nullptr)
    {
        return DNSSD_SERVICE_ALREADY_EXISTS_ERROR;
    }

    auto hostNames = NetworkInformation::GetHostNames();
    HostName^ hostName = nullptr;

    // find first HostName of Type == HostNameType.DomainName && RawName contains "local"
	std::wstring w_domain = std::wstring(mDomain->Data());
    for (unsigned int i = 0; i < hostNames->Size; ++i)
    {
        HostName^ n = hostNames->GetAt(i);
        if (n->Type == HostNameType::DomainName)
        {
            std::wstring temp(n->RawName->Data());
//            auto found = temp.find(L"local");
            auto found = temp.find(w_domain);
            if (found != std::string::npos)
            {
                hostName = n;
                break;
            }
        }
    }

    if (hostName == nullptr)
    {
        return DNSSD_LOCAL_HOSTNAME_NOT_FOUND_ERROR;
    }
#if 1
    auto task = create_task(create_async([this, hostName]
    {
        mSocket = ref new StreamSocketListener();
        mSocketToken = mSocket->ConnectionReceived += ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs ^>(this, &DnssdService::OnConnect);
//        create_task(mSocket->BindServiceNameAsync(mPort)).get();
        create_task(mSocket->BindServiceNameAsync("")).get();
//        create_task(mSocket->BindServiceNameAsync(std::to_string(mNetworkPort))).get();
//        unsigned short port = static_cast<unsigned short>(_wtoi(mSocket->Information->LocalPort->Data()));
//        mService = ref new DnssdServiceInstance(L"dnssd." + mServiceType + L".local", hostName, port);
		// which to use?
		// original code uses .local
		// slides use .local. https://view.officeapps.live.com/op/view.aspx?src=http%3a%2f%2fvideo.ch9.ms%2fsessions%2fbuild%2f2015%2f3-79.Build_2015_DNS_SD_API.pptx
//        mService = ref new DnssdServiceInstance(mServiceName + L"." + mServiceType + L".local", hostName, mNetworkPort);
        mService = ref new DnssdServiceInstance(mServiceName + L"." + mServiceType + L"." + mDomain, hostName, mNetworkPort);

		
//		IMap<Platform::String^, Platform::String^>^ txt_attributes = ref new IMap<Platform::String^, Platform::String^>^();
//		txt_attributes->Insert("MaxUsers", "20");
		//txt_attributes->Insert("key2", "2.0f");
		mService->TextAttributes->Insert("MaxUsers", "22");
		mService->TextAttributes->Insert("SomeKey", "SomeValue");

        return create_task(mService->RegisterStreamSocketListenerAsync(mSocket));
    }));

	// To conform with the other implementations of Zeroconf, we shouldn't block in this setup function.
	// Instead, there should be a user callback that fires async.
	// As I understand it, we must call task.get() to trigger any exceptions that might get thrown.
	// The original code called this, and the operation blocked.
	// So try to make this async.
	DnssdRegisterCallback register_callback = mRegisterCallback;
	void* register_callback_user_data = mUserData;
	uint16_t network_port = mNetworkPort;
	std::string service_name = mServiceNameStr;
	std::string service_type = mServiceTypeStr;
	DnssdServiceWrapper* wrapper_ptr = mWrapperPtr;
	std::string domain = PlatformStringToString(mDomain);


	// I don't think I am doing the task<> parameter correctly.
//	task.then([task, this, service_name, service_type, network_port, register_callback, register_callback_user_data](concurrency::task<DnssdRegistrationResult ^> task_result)
	task.then([this, wrapper_ptr, service_name, service_type, domain, network_port, register_callback, register_callback_user_data](concurrency::task<DnssdRegistrationResult ^> task_result)
	{
		DnssdErrorType result = DNSSD_NO_ERROR;

		try
		{
			// wait for dnssd service to start
//			DnssdRegistrationResult^ reg = task.get(); // will also rethrow any exceptions from above task
			DnssdRegistrationResult^ reg = task_result.get(); // will also rethrow any exceptions from above task
			auto ip = reg->IPAddress; // this always seems to be NULL
			auto status = reg->Status;
			bool hasInstanceChanged = reg->HasInstanceNameChanged;
//			auto string_data = reg->ToString(); // causes exception

			if (status != DnssdRegistrationStatus::Success)
			{
				switch (status)
				{
					case DnssdRegistrationStatus::InvalidServiceName:
						result = DNSSD_INVALID_SERVICE_NAME_ERROR;
						break;
					case DnssdRegistrationStatus::SecurityError:
						result = DNSSD_SERVICE_SECURITY_ERROR;
						break;
					case DnssdRegistrationStatus::ServerError:
						result = DNSSD_SERVICE_SERVER_ERROR;
						break;
					default:
						result = DNSSD_SERVICE_INITIALIZATION_ERROR;
						break;
				}

			}

			if(NULL != register_callback)
			{
				std::string service_name_adjusted = service_name;
				if(hasInstanceChanged)
				{
					service_name_adjusted = PlatformStringToString(mService->DnssdServiceInstanceName);
				}

				register_callback(wrapper_ptr, service_name_adjusted.c_str(), service_type.c_str(), domain.c_str(), network_port, result, register_callback_user_data);
			}
			//return result;
		}
		catch (Platform::Exception^ ex)
		{
			result =  DNSSD_SERVICE_INITIALIZATION_ERROR;
			if(NULL != register_callback)
			{
				register_callback(wrapper_ptr, service_name.c_str(), service_type.c_str(), domain.c_str(), network_port, result, register_callback_user_data);
			}
		}


	}
	);
#else
	        mService = ref new DnssdServiceInstance(mServiceName + L"." + mServiceType + L".local", hostName, mNetworkPort);
#endif

    return DNSSD_NO_ERROR;
}

void DnssdService::Stop()
{
    if (mSocket != nullptr)
    {
        mSocket->ConnectionReceived -= mSocketToken;
        delete mSocket;
        mSocket = nullptr;
    }
	delete mService;
    mService = nullptr;
}

void DnssdService::OnConnect(StreamSocketListener^ sender, StreamSocketListenerConnectionReceivedEventArgs ^ args)
{


}


