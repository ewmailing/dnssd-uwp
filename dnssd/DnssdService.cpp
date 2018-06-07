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

#ifdef DNSSDUWP_USE_LEGACY

DnssdService::DnssdService(const std::string& name, const std::string& port)
{
    mServiceName = StringToPlatformString(name);
    mPort = StringToPlatformString(port);
}
#endif

DnssdService::DnssdService(const char* service_name, const char* service_type, const char* domain, const char* host_name, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdRegisterCallback callback_function, void* user_data)
	: mTxtRecordVector(txt_record_length),
	mTxtRecordLength(txt_record_length)
{
	if((NULL == service_name) || (service_name[0] == '\0'))
	{
		wchar_t computer_name_buffer[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD buf_size = MAX_COMPUTERNAME_LENGTH;
		BOOL ret_flag = GetComputerNameW(computer_name_buffer, &buf_size);
		if(ret_flag)
		{
			char* default_service_name = CreateUTF8fromWStr(computer_name_buffer);
			mServiceNameStr = std::string(default_service_name);
			mServiceName = StringToPlatformString(default_service_name);
			free(default_service_name);
		}
		else
		{
			mServiceNameStr = std::string("My Service");
			mServiceName = StringToPlatformString("My Service");
		}
	}
	else
	{
		mServiceNameStr = std::string(service_name);
		mServiceName = StringToPlatformString(service_name);
	}



	mServiceTypeStr = RemoveTrailingDotIfNecessary(service_type);
	mServiceType = StringToPlatformString(mServiceTypeStr);

	if(domain != NULL)
	{
		mDomain = StringToPlatformString(RemoveTrailingDotIfNecessary(domain));
	}
	else
	{
		mDomain = L"local";
	}

	if((host_name != NULL) && (host_name[0] != '\0'))
	{
		// documentation says:
		// The constructor will fail if the hostName parameter is null or contains an empty string.
		// I have no idea if it will fail under other circumstances like malformed IP-address.
		// I also have no idea if this blocks. May need to move into Start() if it does.
		try
		{
			Platform::String^ platform_string_host_name = StringToPlatformString(host_name);
			HostName^ host = ref new HostName(platform_string_host_name);
			mHostName = host;
		}
		catch(...)
		{
			mHostName = nullptr;
		}
	}
	else
	{
		mHostName = nullptr;
	}

	mNetworkPort = port; // Do we want network or host order? Bonjour DNSService* does network, but I think everything else does host.
	mRegisterCallback = callback_function;
	mUserData = user_data;

//    mPort = StringToPlatformString(std::to_string(ntohs(port))); // Does Windows assume host order?
    mPort = StringToPlatformString(std::to_string(port)); // Does Windows assume host order?

	// My attempt to initialize an initial capacity in the constructor caused that many blank elements to be inserted.
	mTxtRecordVector.clear();
	for(size_t i=0; i<txt_record_length; i++)
	{
		mTxtRecordVector.push_back(txt_record[i]);
	}
}

DnssdService::~DnssdService()
{
    DnssdService::Stop();
}

#ifdef DNSSDUWP_USE_LEGACY
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
#endif

DnssdErrorType DnssdService::StartRegistration()
{

    if (mService != nullptr)
    {
        return DNSSD_SERVICE_ALREADY_EXISTS_ERROR;
    }

	HostName^ hostName = mHostName;

	if(nullptr == mHostName)
	{
		auto hostNames = NetworkInformation::GetHostNames();

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
	}

    if (hostName == nullptr)
    {
        return DNSSD_LOCAL_HOSTNAME_NOT_FOUND_ERROR;
    }
	else
	{
		mHostName = hostName;
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
		//mService->TextAttributes->Insert("MaxUsers", "22");
		//mService->TextAttributes->Insert("SomeKey", "SomeValue");

		// DNS TXT record max size is supposed to be 255 characters, however, I don't think it includes the leading byte indicators.
		// I'm using 512 as an upper bound in case the user gives a malformed string. This will avoid getting trapped in an infinite loop.
		for(size_t i=0; ((i<mTxtRecordLength) && (i<512)); )
		{
			// this does count the = sign as part of the byte count
			size_t next_str_len = (size_t)mTxtRecordVector[i];
			i++;
			char key_buffer[256];
			char value_buffer[256];
			memset(key_buffer, 0, 256);
			memset(value_buffer, 0, 256);
			bool is_key_state = true;
			// The equal sign does not count as part of the key=value length
			size_t j=0;
			size_t k=0;
			for(j=0; ((j<next_str_len) && (j<257)); j++)
			{
				char current_char = mTxtRecordVector[i];
				i++;
				if(current_char == '=')
				{
					is_key_state = false;
					k=i+1;
					key_buffer[j] = '\0';
					j++; // because we are breaking out, j won't get incremented, but we need to continue the count in the following loop so we need to manually increment here.
					break;
				}
				key_buffer[j] = current_char;
			}
			// Make sure we got an = character. If not, skip this or try using ""?
			if(is_key_state)
			{
				// if we are here, the key_buffer was not NULL terminated.
				key_buffer[j] = '\0';
				mService->TextAttributes->Insert(StringToPlatformString(key_buffer), "");
				continue; // go to next loop
			}

			// Using j here is not a bug. We are continuing on the character count because the combined key=value makes up the entire string length.
			for(k=0; ((j<next_str_len) && (j<257)); j++, k++)
			{
				char current_char = mTxtRecordVector[i];
				i++;
				value_buffer[k] = current_char;
			}
			value_buffer[k] = '\0';
			mService->TextAttributes->Insert(StringToPlatformString(key_buffer), StringToPlatformString(value_buffer));
		}


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
					// Microsoft bug?: DnssdServiceInstanceName is supposed to return just the service name
					// Docs: Instance portion of DNS-SD service instance name.(e.g. "myservice" in "myservice._http._tcp.local")
					// https://msdn.microsoft.com/en-us/library/windows/desktop/mt805679(v=vs.85).aspx
					// But I'm getting the fullname: "myservice._http._tcp.local"
					service_name_adjusted = PlatformStringToString(mService->DnssdServiceInstanceName);
					// So remove everything after the first dot.
					std::string::size_type idx;

					idx = service_name_adjusted.find('.');
					if(idx != std::string::npos)
					{
						service_name_adjusted = service_name_adjusted.substr(0, idx);
					}
					else
					{
						// No dot found
					}

				}

				register_callback(wrapper_ptr, service_name_adjusted.c_str(), AppendTrailingDotIfNecessary(service_type).c_str(), AppendTrailingDotIfNecessary(domain).c_str(), network_port, result, register_callback_user_data);
			}
			//return result;
		}
		catch (Platform::Exception^ ex)
		{
			result =  DNSSD_SERVICE_INITIALIZATION_ERROR;
			if(NULL != register_callback)
			{
				register_callback(wrapper_ptr, service_name.c_str(), AppendTrailingDotIfNecessary(service_type).c_str(), AppendTrailingDotIfNecessary(domain).c_str(), network_port, result, register_callback_user_data);
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


