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

#include <vector>
#include <memory>
#include <string>

#include "dnssd_uwp.h"

namespace dnssd_uwp
{
    Platform::String^ StringToPlatformString(const std::string& s);
    std::string PlatformStringToString(Platform::String^ s);
    std::string PlatformStringToString2(Platform::String^ s);
	char* CreateUTF8fromWStr(const wchar_t* input_wstr);

	// Trailing dots in names are technically correct and pedantic,
	// http://www.dns-sd.org/trailingdotsindomainnames.html
	// but the Windows APIs don't seem to handle it correctly.
	// So remove the trailing dot if provided.
	// Use copy-by-value for best performance? (Chandler Carruth, Dave Abrahams)
	std::string RemoveTrailingDotIfNecessary(std::string in_str);

	// For consistency with Bonjour, always add the trailing dot back
	// Use copy-by-value for best performance? (Chandler Carruth, Dave Abrahams)
	std::string AppendTrailingDotIfNecessary(std::string in_str);


#ifdef DNSSDUWP_USE_LEGACY
#else
	enum DnssdServiceUpdateType { ServiceAdded, ServiceUpdated, ServiceRemoved };
#endif

};




