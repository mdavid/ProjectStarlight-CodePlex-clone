/**
* --- BEGIN LICENSE HEADER ---
* 
* This license governs use of the accompanying software. If you use the software, you
* accept this license. If you do not accept the license, do not use the software.
*
* 1. Definitions
* The terms "reproduce," "reproduction," "derivative works," and "distribution" have the
* same meaning here as under U.S. copyright law.
* A "contribution" is the original software, or any additions or changes to the software.
* A "contributor" is any person that distributes its contribution under this license.
* "Licensed patents" are a contributor's patent claims that read directly on its contribution.
* 
* 2. Grant of Rights
* (A) Copyright Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free copyright license to reproduce its contribution, prepare derivative works of its contribution, and distribute its contribution or any derivative works that you create.
* (B) Patent Grant- Subject to the terms of this license, including the license conditions and limitations in section 3, each contributor grants you a non-exclusive, worldwide, royalty-free license under its licensed patents to make, have made, use, sell, offer for sale, import, and/or otherwise dispose of its contribution in the software or derivative works of the contribution in the software.
* 
* 3. Conditions and Limitations
* (A) No Trademark License- This license does not grant you rights to use any contributors' name, logo, or trademarks.
* (B) If you bring a patent claim against any contributor over patents that you claim are infringed by the software, your patent license from such contributor to the software ends automatically.
* (C) If you distribute any portion of the software, you must retain all copyright, patent, trademark, and attribution notices that are present in the software.
* (D) If you distribute any portion of the software in source code form, you may do so only under this license by including a complete copy of this license with your distribution. If you distribute any portion of the software in compiled or object code form, you may only do so under a license that complies with this license.
* (E) The software is licensed "as-is." You bear the risk of using it. The contributors give no express warranties, guarantees or conditions. You may have additional consumer rights under your local laws which this license cannot change. To the extent permitted under your local laws, the contributors exclude the implied warranties of merchantability, fitness for a particular purpose and non-infringement.
*
* --- END LICENSE HEADER ---
*/

//////////////////////////////////////////////////
//
// StarlightPlugin class implementation
//


#include "plugin.h"
#include "scriptobjectutil.h"

StarlightPlugin::StarlightPlugin(NPP npp) :
  m_navigator(npp),
  m_scriptableObject(NULL)
{
	m_invoker = new Invoker();
	m_logger = new DOMElementLogger(npp, m_invoker);
	m_callback = new NPNInvokeMulticastCallback(npp, m_logger, m_invoker);
	m_receiver = CreateMulticastReceiver(m_logger);
	
	m_scriptableObject = CreateScriptable(this, m_navigator);
  
}

StarlightPlugin::~StarlightPlugin()
{
	m_invoker->Stop();
	ReleaseScriptable(m_scriptableObject);
	
	delete m_receiver;
	delete m_callback;
	delete m_logger;
	delete m_invoker;
}



NPObject* StarlightPlugin::GetScriptableObject()
{
	return m_scriptableObject;
}

bool StarlightPlugin::StartStreaming(NPString multicastGroup, int32_t port, NPString multicastSource, NPObject* target, NPVariant* result)
{
	m_callback->Init(target);
	
	char* multicastGroupBuffer = new char[NPSTR_LEN(multicastGroup) + 1];
	memset(multicastGroupBuffer, 0, NPSTR_LEN(multicastGroup) + 1);
	memcpy(multicastGroupBuffer, NPSTR_CHARS(multicastGroup), NPSTR_LEN(multicastGroup));
	
	char* multicastSourceBuffer = new char[NPSTR_LEN(multicastSource) + 1];
	memset(multicastSourceBuffer, 0, NPSTR_LEN(multicastSource) + 1);
	memcpy(multicastSourceBuffer, NPSTR_CHARS(multicastSource), NPSTR_LEN(multicastSource));
	
	int32_t rc = m_receiver->StartReceiving(multicastGroupBuffer, multicastSourceBuffer, port, m_callback);
	
	delete[] multicastGroupBuffer;
	delete[] multicastSourceBuffer;
	VOID_TO_NPVARIANT(*result);
	if(rc != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool StarlightPlugin::StopStreaming(NPVariant* result)
{
	int32_t rc = m_receiver->StopReceiving();
	m_callback->Release();
	VOID_TO_NPVARIANT(*result);
	if(rc != 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool StarlightPlugin::Test(NPString message, NPVariant* result)
{
	NPVariant ret;
	NPObject* window;
	NPString s;
	char* messageBuffer = new char[NPSTR_LEN(message) + 1];
	memset(messageBuffer, 0, NPSTR_LEN(message) + 1);
	memcpy(messageBuffer, NPSTR_CHARS(message), NPSTR_LEN(message));
	char buffer[JS_BUF_SZ];
	memset(buffer, 0, JS_BUF_SZ);
	int charcount = snprintf(buffer, JS_BUF_SZ - 1, "alert('%s');", messageBuffer);
	delete[] messageBuffer;
	
	NPSTR_CHARS(s) = buffer;
	NPSTR_LEN(s) = charcount;
	
	NPN_GetValue(m_navigator, NPNVWindowNPObject, &window);
	NPN_Evaluate(m_navigator, window, &s, &ret);
	NPN_ReleaseObject(window);
	VOID_TO_NPVARIANT(*result);
	return true;
}

bool StarlightPlugin::FetchNSC(NPString url, NPVariant* result)
{
	
	//This doesn't make use of the NPAPI functions
	//to get URLs since I don't want to deal with fetching the resource 
	//asynchronously at the moment.
	char* urlBuffer = new char[NPSTR_LEN(url) + 1];
	memset(urlBuffer, 0, NPSTR_LEN(url) + 1);
	memcpy(urlBuffer, NPSTR_CHARS(url), NPSTR_LEN(url));
	
	char buffer[NSC_BUF_SZ];
	
#ifdef XP_WIN
	//Crack the URL
	URL_COMPONENTSA urlParts;
	ZeroMemory(&urlParts, sizeof(urlParts));
	urlParts.dwUrlPathLength = 1;
	urlParts.dwStructSize = sizeof(urlParts);
	if(!InternetCrackUrlA(urlBuffer, 0, 0, &urlParts))
	{
		delete[] urlBuffer;
		return PR_FALSE;
	}
	
	
	//Make sure the URL ends with .nsc so that people can't use us to
	//fetch arbitrary files from the internet.
	if(urlParts.dwUrlPathLength < 4)
	{
		delete[] urlBuffer;
		return PR_FALSE;
	}
	if(memcmp(urlParts.lpszUrlPath + urlParts.dwUrlPathLength - 4, ".nsc", (4 * sizeof(char))) != 0)
	{
		delete[] urlBuffer;
		return PR_FALSE;
	}
	
	char* headers = "Accept-Charset: ISO-8859-1\r\nAccept-Encoding: identity\r\n";
	
	//Open the internet handle
	HINTERNET internetHandle = InternetOpenA("Starlight Multicast Proxy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	
	//Open the requested URL
	HINTERNET urlHandle = InternetOpenUrlA(
										   internetHandle, 
										   urlBuffer, 
										   headers, 
										   -1, 
										   INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS,
										   NULL);
	
	if(urlHandle == NULL)
	{
		delete[] urlBuffer;
		return PR_FALSE;
	}
	
	DWORD bytesRead = 0;
	DWORD idx = 0;
	while(InternetReadFile(urlHandle, buffer + idx, NSC_BUF_SZ - idx, &bytesRead))
	{
		
		if(bytesRead == 0)
		{
			break;
		}
		else
		{	
			idx += bytesRead;
		}
	}
	
	//Cleanup
	InternetCloseHandle(urlHandle);
	InternetCloseHandle(internetHandle);
#endif /* XP_WIN */
	
#ifdef XP_MACOSX
	CFStringRef cfurlString = CFStringCreateWithCString(kCFAllocatorDefault, urlBuffer, kCFStringEncodingUTF8);
	CFURLRef cfurl = CFURLCreateWithString(kCFAllocatorDefault, cfurlString, NULL);
	CFStringRef path = CFURLCopyPath(cfurl);
	if(!CFStringHasSuffix(path, CFSTR(".nsc")))
	{
		CFRelease(path);
		CFRelease(cfurl);
		CFRelease(cfurlString);
		return false;
	}
	
	CFHTTPMessageRef request = CFHTTPMessageCreateRequest(kCFAllocatorDefault, CFSTR("GET"), cfurl, kCFHTTPVersion1_0);
	CFHTTPMessageSetHeaderFieldValue(request, CFSTR("Accept-Charset"), CFSTR("ISO-8859-1"));
	CFHTTPMessageSetHeaderFieldValue(request, CFSTR("Accept-Encoding"), CFSTR("identity"));
	CFReadStreamRef stream = CFReadStreamCreateForHTTPRequest(kCFAllocatorDefault, request);
	if(!CFReadStreamOpen(stream))
	{
		CFRelease(path);
		CFRelease(cfurl);
		CFRelease(cfurlString);
		CFRelease(request);
		CFRelease(stream);
		return false;
	}
	
	int bytesRead = 0;
	int idx = 0;
	while((bytesRead = CFReadStreamRead(stream, (UInt8*)buffer + idx, NSC_BUF_SZ - idx)) > -1)
	{
		
		if(bytesRead == 0)
		{
			break;
		}
		else
		{	
			idx += bytesRead;
		}
	}
	
	
	CFRelease(path);
	CFRelease(cfurl);
	CFRelease(cfurlString);
	CFRelease(request);
	CFRelease(stream);
#endif /* XP_MACOSX */
	
	char* retBuf = (char*)NPN_MemAlloc(idx);
	memcpy(retBuf, buffer, idx);
	while(idx >0 && retBuf[idx - 1] == 0)
	{
		idx--;
	}
	STRINGN_TO_NPVARIANT(retBuf, idx, *result);
	delete[] urlBuffer;
	return true;
}








