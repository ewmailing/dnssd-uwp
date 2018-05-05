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

#include "DnssdServiceResolver.h"
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

    DnssdServiceResolver::DnssdServiceResolver(const char* service_name, const char* service_type, const char* domain, DnssdServiceResolverChangedCallback callback, void* user_data)
        : mDnssdServiceChangedCallback(callback),
		mUserData(user_data)
        , mRunning(false)
        , mIsIniting(true )
		, mWrapperPtr(nullptr)
    {
		mServiceName = StringToPlatformString(service_name);
		mServiceType = StringToPlatformString(service_type);
		if(domain != NULL)
		{
			mDomain = StringToPlatformString(domain);
		}
    }

    DnssdServiceResolver::~DnssdServiceResolver()
    {
        if (mServiceWatcher)
        {
			while(mIsIniting)
			{
			}
			mServiceWatcher->Added -= mDelegateAdded;
            mServiceWatcher->Removed -= mDelegateRemoved;
            mServiceWatcher->Updated -= mDelegateUpdated;
            mServiceWatcher->EnumerationCompleted -= mDelegateCompleted;
            mServiceWatcher->Stopped -= mDelegateStopped;

            mRunning = false;
            mServiceWatcher->Stop();
            mServiceWatcher = nullptr;
			mWrapperPtr = nullptr; // you are expected to explictly delete the wrapper which will cause this instance to be released
        }
    }

    DnssdErrorType DnssdServiceResolver::Initialize()
    {
        auto task = create_task(create_async([this]
        {
            /// <summary>
            /// All of the properties that will be returned when a DNS-SD instance has been found. 
            /// </summary>

            Vector<Platform::String^>^ propertyKeys = ref new Vector<Platform::String^>();
			propertyKeys->Append(L"System.Devices.Dnssd.HostName");
            propertyKeys->Append(L"System.Devices.Dnssd.InstanceName");
            propertyKeys->Append(L"System.Devices.Dnssd.ServiceName");
            propertyKeys->Append(L"System.Devices.Dnssd.Domain");
            propertyKeys->Append(L"System.Devices.Dnssd.FullName");
			propertyKeys->Append(L"System.Devices.IpAddress");
			propertyKeys->Append(L"System.Devices.Dnssd.PortNumber");
			propertyKeys->Append(L"System.Devices.Dnssd.TextAttributes");

            Platform::String^ aqsQueryString;

			if(mDomain != nullptr)
			{

				aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54} AND " +
					L"System.Devices.Dnssd.Domain:=\"" + mDomain + "\""
					+ " AND System.Devices.Dnssd.InstanceName:=\"" + mServiceName 
	//                L"System.Devices.Dnssd.Domain:=\"local\" AND System.Devices.Dnssd.InstanceName:=\"Al’s Music" 
					+ L"\" AND System.Devices.Dnssd.ServiceName:=\"" + mServiceType  
					+ L"\"";
			}
			else
			{
				aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54} AND " +
					L"System.Devices.Dnssd.InstanceName:=\"" + mServiceName 
	//                L"System.Devices.Dnssd.Domain:=\"local\" AND System.Devices.Dnssd.InstanceName:=\"Al’s Music" 
					+ L"\" AND System.Devices.Dnssd.ServiceName:=\"" + mServiceType  
					+ L"\"";
			}
/*
			            aqsQueryString = L"System.Devices.AepService.ProtocolId:={4526e8c1-8aac-4153-9b16-55e86ada0e54} AND "
				+ "System.Devices.Dnssd.InstanceName:=\"" + mServiceName 
//				+ "System.Devices.Dnssd.FullName:=\"" + mServiceName + L"." + mServiceType 
				+ "\"";
*/
            mServiceWatcher = DeviceInformation::CreateWatcher(aqsQueryString, propertyKeys, DeviceInformationKind::AssociationEndpointService);

            // wire up event handlers
            mDelegateAdded = mServiceWatcher->Added += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformation ^>(this, &DnssdServiceResolver::OnServiceAdded);
mDelegateRemoved = mServiceWatcher->Removed += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>(this, &DnssdServiceResolver::OnServiceRemoved);
mDelegateUpdated = mServiceWatcher->Updated += ref new TypedEventHandler<DeviceWatcher ^, DeviceInformationUpdate ^>(this, &DnssdServiceResolver::OnServiceUpdated);
mDelegateCompleted = mServiceWatcher->EnumerationCompleted += ref new Windows::Foundation::TypedEventHandler<DeviceWatcher ^, Platform::Object ^>(this, &DnssdServiceResolver::OnServiceEnumerationCompleted);
mDelegateStopped = mServiceWatcher->Stopped += ref new Windows::Foundation::TypedEventHandler<DeviceWatcher ^, Platform::Object ^>(this, &DnssdServiceResolver::OnServiceEnumerationStopped);

            // start watching for dnssd services
            mServiceWatcher->Start();
            mRunning = true;
            auto status = mServiceWatcher->Status;

//			return 0;

        }));

		std::string service_type = PlatformStringToString(mServiceType);
		DnssdServiceResolverChangedCallback resolve_callback = mDnssdServiceChangedCallback;
		void* user_data = mUserData;
		DnssdServiceResolverWrapper* wrapper_ptr = mWrapperPtr;
		std::string domain;
		bool is_domain_null = true;
		if(mDomain)
		{
			domain = PlatformStringToString(mDomain);
			is_domain_null = false;
		}

			// I don't think I am doing the task<> parameter correctly.
		task.then([this, wrapper_ptr, service_type, domain, is_domain_null, resolve_callback, user_data](auto task_result)
//		task.then([task, this, service_type, resolve_callback, user_data](auto task_result)
		{

			try
			{
				// wait for port enumeration to complete
//				task.get(); // will throw any exceptions from above task
				task_result.get(); // will throw any exceptions from above task
//				return DNSSD_NO_ERROR;
				mIsIniting = false;
			}
			catch (Platform::Exception^ ex)
			{
				const char* domain_c_str = NULL;
				if(is_domain_null)
				{
					domain_c_str = domain.c_str();
				}

				resolve_callback(
					(DnssdServiceResolverPtr)wrapper_ptr,
					NULL, // service_name
					service_type.c_str(),
					domain_c_str,
					NULL, // full_name
					NULL, // host target
					0, //port
					NULL, // txtRecord
					0, // txt length
					DNSSD_SERVICEWATCHER_INITIALIZATION_ERROR, 
					user_data
				);

				mIsIniting = false;

//				return DNSSD_SERVICEWATCHER_INITIALIZATION_ERROR;
			}
		}
		);
		return DNSSD_NO_ERROR;

    }

    void DnssdServiceResolver::UpdateDnssdService(DnssdServiceUpdateType type, Windows::Foundation::Collections::IMapView<Platform::String^, Platform::Object^>^ props, Platform::String^ serviceId)
    {
		// FIXME: I think we need to iterate through each IpAddress and do a callback for each. (e.g. user needs both IPv4 and IPv6)
        auto box = safe_cast<Platform::IBoxArray<Platform::String^>^>(props->Lookup("System.Devices.IpAddress"));
		Platform::String^ host;
		if(box->Value->Length > 0 )
		{
			host = box->Value->get(0);
		}
		else
		{
			OutputDebugStringA("box has no IpAddress value");
		}
        Platform::String^ port_plstr = props->Lookup("System.Devices.Dnssd.PortNumber")->ToString();
        Platform::String^ full_name = props->Lookup("System.Devices.Dnssd.FullName")->ToString();
        Platform::String^ name = props->Lookup("System.Devices.Dnssd.InstanceName")->ToString();
        Platform::String^ service_type = props->Lookup("System.Devices.Dnssd.ServiceName")->ToString();
		Platform::String^ domain = props->Lookup("System.Devices.Dnssd.Domain")->ToString();

		// Documentation is wrong. It is not an IMap but an IBoxArray
//		auto text_attributes_collection = safe_cast<IMap<Platform::String^, Platform::String^>^>(props->Lookup("System.Devices.Dnssd.TextAttributes"));
		auto text_attributes_collection = safe_cast<Platform::IBoxArray<Platform::String^>^>(props->Lookup("System.Devices.Dnssd.TextAttributes"));

	
		std::vector<char> combined_txt_record;
		// It looks like Bonjour is separating multiple entries with the number of bytes of the following string.
		// https://developer.apple.com/library/mac/qa/qa1306/_index.html

		for(unsigned int i=0; i<text_attributes_collection->Value->Length; i++)
		{
			Platform::String^ platform_string = text_attributes_collection->Value->get(i);
			std::string kv_string = PlatformStringToString(platform_string);
			std::vector<char> kv_vector = std::vector<char>(kv_string.begin(), kv_string.end());
			combined_txt_record.push_back(static_cast<char>(kv_string.length()));
			combined_txt_record.insert( std::end(combined_txt_record), std::begin(kv_vector), std::end(kv_vector) );
//			std::string byte_string = std::to_string(kv_string.length());
//			combined_txt_record += (byte_string + kv_string);

		}
#if 0
		// Go through all txt records in the collection and combine them into a txt record format
		for(const auto& kv : text_attributes_collection)
		{
//			std::wcout << kv.first << " has value " << kv.second << std::endl;
			std::string key_string = PlatformStringToString(kv->Key);
			std::string value_string = PlatformStringToString(kv->Value);
			std::string kv_string = key_string + "=" + value_string;
			std::string byte_string = std::to_string(kv_string.length());

			combined_txt_record += (byte_string + kv_string);
		}
#endif


		bool change_due_to_new_ip_address = false;
		bool change_due_to_txt_record = false;

        auto it = mServices.find(serviceId);
        if (it != mServices.end()) // service was previously found. Update the info and report change if necessary
        {
            auto info = it->second;
			if(box->Value->Length != info->mIpAddresses.size())
			{
				info->mChanged = true;
				change_due_to_new_ip_address = true;
			}
			else if(box->Value->Length > 0 )
			{
				for(unsigned int i=0; i<box->Value->Length; i++)
				{
					Platform::String^ platform_ip_address = box->Value->get(i);
					std::string stdstr_ip_address = PlatformStringToString(platform_ip_address);
					auto it = info->mIpAddresses.find(stdstr_ip_address);
					if(it == info->mIpAddresses.end())
					{
						info->mChanged = true;
						change_due_to_new_ip_address = true;
						break;
					}
				}
			}

			// Need to check txt record change independently of IP address change
			if(info->mCombinedTxtRecord != combined_txt_record)
			{
				info->mChanged = true;
				// set the new txt record
				info->mCombinedTxtRecord = combined_txt_record;
				change_due_to_txt_record = true;
			}

            info->mType = DnssdServiceUpdateType::ServiceUpdated;

            if(info->mChanged)
            {
				// This is messy. I originally only fired callbacks if there was a new IP address, and avoiding callbacks on existing IP addresses.
				// But now with txt record changes, callbacks are needed for everything.
				// To simplify things, I'm disabling the old code and now calling back on everything.


#if 0
				// If there is a change, it means we have an IP address change.
				// We need to post callbacks for each new IP address.
				// Then we should remove the old saved IP addresses and save the new IP addresses.
				if(box->Value->Length > 0 )
				{
					std::set<std::string> set_of_new_ipaddresses;

					// TODO: This is redundant with the above algorithm that detects if there is a change. Should optimize this so it doesn't redo work.
					for(unsigned int i=0; i<box->Value->Length; i++)
					{
						Platform::String^ platform_ip_address = box->Value->get(i);
						std::string stdstr_ip_address = PlatformStringToString(platform_ip_address);
						auto it = info->mIpAddresses.find(stdstr_ip_address);
						if(it == info->mIpAddresses.end())
						{
							// report the updated service if this is a new IP address
			                OnDnssdServiceUpdated(info, stdstr_ip_address);
							set_of_new_ipaddresses.insert(stdstr_ip_address);
						}
					}

					// Now clear the old map.
					info->mIpAddresses.clear();
					// Save the new ipaddreses
					info->mIpAddresses = set_of_new_ipaddresses;
				}
				// else, we have no IP addresses which implies the service went away. We should not do a callback for that case.
#else

				if(box->Value->Length > 0 )
				{
					std::set<std::string> set_of_new_ipaddresses;

					// TODO: This is redundant with the above algorithm that detects if there is a change. Should optimize this so it doesn't redo work.
					for(unsigned int i=0; i<box->Value->Length; i++)
					{
						Platform::String^ platform_ip_address = box->Value->get(i);
						std::string stdstr_ip_address = PlatformStringToString(platform_ip_address);
		                OnDnssdServiceUpdated(info, stdstr_ip_address);
						set_of_new_ipaddresses.insert(stdstr_ip_address);
					}

					// Now clear the old map.
					info->mIpAddresses.clear();
					// Save the new ipaddreses
					info->mIpAddresses = set_of_new_ipaddresses;
				}
				// else, we have no IP addresses which implies the service went away. We should not do a callback for that case.


#endif

            }
        }
        else // add it to the service map
        {
            DnssdServiceResolverInstance^ info = ref new DnssdServiceResolverInstance;
            info->mId = serviceId;
			info->mHostTarget = host;
//            info->mPort = port;
            info->mInstanceName = name;
            info->mServiceType = service_type;
            info->mDomain = domain;
			info->mFullName = full_name;
            info->mType = DnssdServiceUpdateType::ServiceAdded;
			info->mCombinedTxtRecord = combined_txt_record;
			mServices[serviceId] = info;

			std::string port_stdstring = PlatformStringToString(port_plstr);
			info->networkPort = std::stoi(port_stdstring);

			if(box->Value->Length > 0 )
			{
				std::set<std::string> set_of_new_ipaddresses;
				for(unsigned int i=0; i<box->Value->Length; i++)
				{
					Platform::String^ platform_ip_address = box->Value->get(i);
					std::string stdstr_ip_address = PlatformStringToString(platform_ip_address);
					info->mIpAddresses.insert(stdstr_ip_address);
		            OnDnssdServiceUpdated(info, stdstr_ip_address);
				}
			}
			// else, we have no IP addresses which implies the service went away. We should not do a callback for that case.
        }
    }

    void DnssdServiceResolver::OnDnssdServiceUpdated(DnssdServiceResolverInstance^ info, const std::string& ip_address)
    {

		std::string service_name = PlatformStringToString(info->mInstanceName);
        std::string service_type = PlatformStringToString(info->mServiceType);
        std::string domain = PlatformStringToString(info->mDomain);
        std::string full_name = PlatformStringToString(info->mFullName);
//		std::string host_target = PlatformStringToString(info->mHostTarget);
//		std::string txt_record = PlatformStringToString(info->mTxtRecord);
		uint16_t port = info->networkPort;



        if (mDnssdServiceChangedCallback != nullptr)
        {
			// Create a buffer long enough to hold our string with converted escaped characters.
			size_t str_len = full_name.length();
			size_t tmp_buf_len = full_name.length() * 8 + 1;
			const char* original_str = full_name.c_str(); 
			char* dns_full_name = (char*)calloc(tmp_buf_len, sizeof(char));
			for(size_t i=0, j=0; i<str_len; i++)
			{
				if( ( (original_str[i] >= 0x30) && (original_str[i] <= 0x39) ) // digits
					|| ( (original_str[i] >= 0x41) && (original_str[i] <= 0x5a) ) // lowercase
					|| ( (original_str[i] >= 0x61) && (original_str[i] <= 0x7a) ) // uppercase
					|| ( (original_str[i] == 0x2d) ) // -
					|| ( (original_str[i] == 0x5f) ) // _
					|| ( (original_str[i] == 0x2e) ) // .
				)
				{
					dns_full_name[j] = original_str[i];
					j++;
				}
//				else if((original_str[i] <= 0x7f))
				else
				{
					char tmp_buffer[33];
					dns_full_name[j] = '\\';
					j++;
					dns_full_name[j] = 'x';
					j++;

					sprintf_s(tmp_buffer, 33, "%02x", (original_str[i] & 0xFF));
					{
						size_t k=0;
						while(tmp_buffer[k] != '\0')
						{
							dns_full_name[j] = tmp_buffer[k];
							j++;
							k++;
						}
					}
					dns_full_name[j] = '\0';
				}
				/*
				else
				{
					dns_full_name[j] = original_str[i];
				}
				*/
			}

			const char* txt_record = NULL;
			uint16_t txt_record_length = 0;
			if(info->mCombinedTxtRecord.size() > 0)
			{
				txt_record_length = (uint16_t)info->mCombinedTxtRecord.size();
				txt_record = info->mCombinedTxtRecord.data();
			}

			//	    typedef void(*DnssdServiceResolverChangedCallback) (DnssdServiceResolverPtr service_resolver, const char* full_name, const char* host_target, uint16_t port, const char* txt_record, uint16_t txt_record_length, DnssdErrorType error_code, void* user_data);

			mDnssdServiceChangedCallback(
				(DnssdServiceResolverPtr)mWrapperPtr, 
//				full_name.c_str(),
				service_name.c_str(),
				service_type.c_str(),
				domain.c_str(),
				dns_full_name,
				ip_address.c_str(),
				port,
				txt_record,
				txt_record_length,
				DNSSD_NO_ERROR,
				mUserData
			);
			free(dns_full_name);

        }
    }

    void DnssdServiceResolver::OnServiceAdded(DeviceWatcher^ sender, DeviceInformation^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceAdded, args->Properties, args->Id);
    }

    void DnssdServiceResolver::OnServiceUpdated(DeviceWatcher^ sender, DeviceInformationUpdate^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceUpdated, args->Properties, args->Id);
    }


    void DnssdServiceResolver::OnServiceRemoved(DeviceWatcher^ sender, DeviceInformationUpdate^ args)
    {
        UpdateDnssdService(DnssdServiceUpdateType::ServiceUpdated, args->Properties, args->Id);
    }

    void DnssdServiceResolver::OnServiceEnumerationCompleted(DeviceWatcher^ sender, Platform::Object^ args)
    {
        // stop the service scanning. Service scanning will be restarted when OnServiceEnumerationStopped event is received
        mServiceWatcher->Stop();
    }

    void DnssdServiceResolver::OnServiceEnumerationStopped(Windows::Devices::Enumeration::DeviceWatcher^ sender, Platform::Object^ args)
    {
        // check if we are shutting down
        if (!mRunning)
        {
            return;
        }

        std::vector<Platform::String^> removedServices;

        // iterate through the services list and remove any service that is marked for removal

		for (auto it = mServices.begin(); it != mServices.end(); ++it)
        {
            auto service = it->second;
            if (service->mType == DnssdServiceUpdateType::ServiceRemoved)
            {
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
    }
}


