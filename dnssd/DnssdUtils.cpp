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

//#include "DnssdServiceWatcher.h"
#include <algorithm>
#include <cvt/wstring>
#include <codecvt>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace dnssd_uwp
{
    Platform::String^ StringToPlatformString1(const std::string& s)
    {
        std::wstring w(s.begin(), s.end());
        Platform::String^ p = ref new Platform::String(w.c_str());
        return p;
    }

	wchar_t* CStrToWStr(const char* input_str)
	{
		if(NULL == input_str)
		{
			return NULL;
		}
		int str_len = (int)strlen(input_str);
		int num_chars = MultiByteToWideChar(CP_UTF8, 0, input_str, str_len, NULL, 0);
		wchar_t* output_winstr = (wchar_t*)malloc((num_chars + 1) * sizeof(wchar_t));
		if(output_winstr)
		{
			MultiByteToWideChar(CP_UTF8, 0, input_str, str_len, output_winstr, num_chars);
			output_winstr[num_chars] = L'\0';
		}
		return output_winstr;
	}
	Platform::String^ StringToPlatformString(const std::string& s)
    {
		wchar_t* w_str = CStrToWStr(s.c_str());

//        std::wstring w(s.begin(), s.end());
        Platform::String^ p = ref new Platform::String(w_str);
		free(w_str);
        return p;
    }


    std::string PlatformStringToString(Platform::String^ s)
    {
        int bufferSize = WideCharToMultiByte(CP_UTF8, 0, s->Data(), -1, nullptr, 0, NULL, NULL);
        auto utf8 = std::make_unique<char[]>(bufferSize);
        if (0 == WideCharToMultiByte(CP_UTF8, 0, s->Data(), -1, utf8.get(), bufferSize, NULL, NULL))
            throw std::exception("Can't convert string to UTF8");

        return std::string(utf8.get());
    }

    std::string PlatformStringToString2(Platform::String^ s)
    {
        stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
        std::string stringUtf8 = convert.to_bytes(s->Data());
        return stringUtf8;
    }

	char* CreateUTF8fromWStr(const wchar_t* input_wstr)
	{
		if(NULL == input_wstr)
		{
			return NULL;
		}
		int wstr_len = (int)wcslen(input_wstr);
		int num_chars = WideCharToMultiByte(CP_UTF8, 0, input_wstr, wstr_len, NULL, 0, NULL, NULL);
		char* output_utf8str = (char*)malloc((num_chars + 1) * sizeof(char));
		if(output_utf8str)
		{
			WideCharToMultiByte(CP_UTF8, 0, input_wstr, wstr_len, output_utf8str, num_chars, NULL, NULL);
			output_utf8str[num_chars] = '\0';
		}
		return output_utf8str;
	}



	// Trailing dots in names are technically correct and pedantic,
	// http://www.dns-sd.org/trailingdotsindomainnames.html
	// but the Windows APIs don't seem to handle it correctly.
	// So remove the trailing dot if provided.
	// Use copy-by-value for best performance? (Chandler Carruth, Dave Abrahams)
	std::string RemoveTrailingDotIfNecessary(std::string in_str)
	{
		std::string ret_str = in_str;

		if('.' == ret_str.back())
		{
			ret_str.pop_back();
		}
		return ret_str;
	}

	// For consistency with Bonjour, always add the trailing dot back
	// Use copy-by-value for best performance? (Chandler Carruth, Dave Abrahams)
	std::string AppendTrailingDotIfNecessary(std::string in_str)
	{
		if('.' != in_str.back())
		{
			return (in_str + ".");
		}
		else
		{
			return in_str;
		}
	}



}


