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
#include "scriptableobject.h"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void b64chunk(const unsigned char* in, char* out, int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}



static NPObject *
AllocateScriptablePluginObject(NPP npp, NPClass *aClass)
{
  return new StarlightScriptable(npp);
}

DECLARE_NPOBJECT_CLASS_WITH_BASE(StarlightScriptable,
                                 AllocateScriptablePluginObject);


bool
StarlightScriptable::HasMethod(NPIdentifier name)
{
	if(name == START_STREAMING_METHOD
		|| name == STOP_STREAMING_METHOD
		|| name == FETCH_NSC_METHOD
		|| name == TEST_METHOD)
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool
StarlightScriptable::Invoke(NPIdentifier name, const NPVariant* args,
                               uint32_t argCount, NPVariant *result)
{
	if (name == START_STREAMING_METHOD) 
	{
		if(argCount != 3)
		{
			return false;
		}
		NPString multicastGroup = NPVARIANT_TO_STRING(args[0]);
		int32_t port;
		if(NPVARIANT_IS_INT32(args[1]))
		{
			port = NPVARIANT_TO_INT32(args[1]);
		}
		else
		{
			port = (int)NPVARIANT_TO_DOUBLE(args[1]);
		}
		NPObject* target = NPVARIANT_TO_OBJECT(args[2]);
		return StartStreaming(multicastGroup, port, target, result);
	}
	else if(name == STOP_STREAMING_METHOD)
	{
		return StopStreaming(result);
	}
	else if(name == TEST_METHOD)
	{
		if(argCount != 1)
		{
			return false;
		}
		NPString message = NPVARIANT_TO_STRING(args[0]);
		return Test(message, result);
	}
	else if(name == FETCH_NSC_METHOD)
	{
		if(argCount != 1)
		{
			return false;
		}
		NPString url = NPVARIANT_TO_STRING(args[0]);
		return FetchNSC(url, result);
	}
	else
	{
		return false;
	}
}

bool 
StarlightScriptable::StartStreaming(NPString multicastGroup, int32_t port, NPObject* target, NPVariant* result)
{
	m_callback->Init(target);

	char* multicastGroupBuffer = new char[NPSTR_LEN(multicastGroup) + 1];
	memset(multicastGroupBuffer, 0, NPSTR_LEN(multicastGroup) + 1);
	memcpy(multicastGroupBuffer, NPSTR_CHARS(multicastGroup), NPSTR_LEN(multicastGroup));

	int32_t rc = m_receiver->StartReceiving(multicastGroupBuffer, port, m_callback);

	delete[] multicastGroupBuffer;
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

bool 
StarlightScriptable::StopStreaming(NPVariant* result)
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

bool 
StarlightScriptable::Test(NPString message, NPVariant* result)
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

	NPN_GetValue(navigator, NPNVWindowNPObject, &window);
	NPN_Evaluate(navigator, window, &s, &ret);
	NPN_ReleaseObject(window);
	VOID_TO_NPVARIANT(*result);
	return true;
}

bool 
StarlightScriptable::FetchNSC(NPString url, NPVariant* result)
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

StarlightPlugin::StarlightPlugin(NPP pNPInstance) :
  m_navigator(pNPInstance),
  m_scriptableObject(NULL)
{
  m_scriptableObject = NPN_CreateObject(m_navigator,
                       GET_NPOBJECT_CLASS(StarlightScriptable));
  NPN_RetainObject(m_scriptableObject);
  
}

StarlightPlugin::~StarlightPlugin()
{
  NPN_ReleaseObject(m_scriptableObject);
}



NPObject* StarlightPlugin::GetScriptableObject()
{
  return m_scriptableObject;
}

void NPNInvokeMulticastCallback::Init(NPObject* target)
{
	Release();
	NPN_RetainObject(target);
	m_target = target;
}

void NPNInvokeMulticastCallback::Release()
{
	if(m_target != NULL)
	{
		NPN_ReleaseObject(m_target);
		m_target = NULL;
	}
}

void NPNInvokeMulticastCallback::ReportPacketRead(int count, MulticastCallbackData* data)
{
	if(NULL == m_target)
	{
		return;
	}

	int totalPacketSz = sizeof(int);
	for(int i = 0; i < count; i++)
	{
		totalPacketSz += data[i].szData;
		totalPacketSz += sizeof(int);
	}

	unsigned char* allData = new unsigned char[totalPacketSz];
	int allDataIdx = sizeof(int);
	memcpy(allData, &count, sizeof(int));
	for(int i = 0; i < count; i++)
	{
		memcpy(allData + allDataIdx, &data[i].szData, sizeof(int));
		allDataIdx += sizeof(int);
		memcpy(allData + allDataIdx, data[i].data, data[i].szData);
		allDataIdx += data[i].szData;

	}

	int b64sz = ((totalPacketSz / 3) + (totalPacketSz % 3 > 0 ? 1 : 0)) * 4;
	char* packetData = new char[b64sz];
	int srcIdx = 0;
	int destIdx = 0;
	while(srcIdx < totalPacketSz) 
	{
		int len = totalPacketSz - srcIdx;
		if(len > 3) {
			len = 3;
		}
		b64chunk(allData + srcIdx, packetData + destIdx, len);
		srcIdx += len;
		destIdx += 4;
	}

	delete[] allData;

	m_async->CallAsync(m_navigator, &NPNInvokeMulticastCallback::DoRealReportPacketRead, new NPNInvokeMulticastCallbackArgs(packetData, b64sz, m_target, &ADD_PACKET_METHOD, m_logger, m_navigator));
}

void NPNInvokeMulticastCallback::DoRealReportPacketRead(void* vargs)
{
	NPNInvokeMulticastCallbackArgs* args = (NPNInvokeMulticastCallbackArgs*)vargs;
	NPVariant argAry;
	NPVariant result;
	STRINGN_TO_NPVARIANT(args->m_data, args->m_szData, argAry);
	if(!NPN_Invoke(args->m_navigator, args->m_target, *(args->m_method), &argAry, 1, &result))
	{
		char buf[ERR_BUF_SZ];
		memset(buf, 0, ERR_BUF_SZ);
		snprintf(buf, ERR_BUF_SZ, "Invoke failed.");
		args->m_logger->LogError(buf);
	}
	else
	{
		NPN_ReleaseVariantValue(&result);
	}
	delete args;
}

void DOMElementLogger::LogTrace(const char * data)
{
	m_async->CallAsync(m_navigator, &DOMElementLogger::DoRealLog, (void*)new DOMElementLoggerCallbackArgs(data, "TRACE: ", m_navigator));
}

void DOMElementLogger::LogError(const char * data)
{
	m_async->CallAsync(m_navigator, &DOMElementLogger::DoRealLog, (void*)new DOMElementLoggerCallbackArgs(data, "ERROR: ", m_navigator));
}

void DOMElementLogger::DoRealLog(void* vargs)
{
	DOMElementLoggerCallbackArgs* args = (DOMElementLoggerCallbackArgs*)vargs;

	NPObject* window;
	NPVariant document;
	NPVariant docElementId;
	STRINGZ_TO_NPVARIANT(LOG_DOM_ELEMENT, docElementId);
	NPVariant logElement;
	
	if(NPN_GetValue(args->m_navigator, NPNVWindowNPObject, &window) == NPERR_NO_ERROR)
	{
		NPIdentifier documentPropId = NPN_GetStringIdentifier("document");
		bool rc = NPN_GetProperty(args->m_navigator, window, documentPropId, &document);
		if(rc && NPVARIANT_IS_OBJECT(document))
		{
			NPIdentifier getElementMethodId = NPN_GetStringIdentifier("getElementById");
			rc = NPN_Invoke(args->m_navigator, NPVARIANT_TO_OBJECT(document), getElementMethodId, &docElementId, 1, &logElement);
			if(rc && NPVARIANT_IS_OBJECT(logElement))
			{
				//Get the existing inner HTML
				NPVariant innerHTMLVariant;
				NPIdentifier innerHTMLPropId = NPN_GetStringIdentifier("innerHTML");
				rc = NPN_GetProperty(args->m_navigator, NPVARIANT_TO_OBJECT(logElement), innerHTMLPropId, &innerHTMLVariant);
		
				//Write the log message into a buffer
				char buffer[JS_BUF_SZ];
				memset(buffer, 0, JS_BUF_SZ);
				snprintf(buffer, JS_BUF_SZ - 1, "%s: %s <BR>", args->m_type, args->m_message);
				int szMessage = strlen(buffer);
		
				//Convert the old value to a NPString
				NPString innerHTMLString;
				if(rc && NPVARIANT_IS_STRING(innerHTMLVariant))
				{
					innerHTMLString = NPVARIANT_TO_STRING(innerHTMLVariant);
				}
				else
				{
					NPSTR_LEN(innerHTMLString) = 0;
				}
		
				//Create a new string with the old data + the new data
				char* newInnerHTML = (char*) NPN_MemAlloc(szMessage + NPSTR_LEN(innerHTMLString));
				memcpy(newInnerHTML, NPSTR_CHARS(innerHTMLString), NPSTR_LEN(innerHTMLString));
				memcpy(newInnerHTML + NPSTR_LEN(innerHTMLString), buffer, szMessage);
		
				//Create a variant and set the property to it
				NPVariant newInnerHTMLVariant;
				STRINGN_TO_NPVARIANT(newInnerHTML, szMessage + NPSTR_LEN(innerHTMLString), newInnerHTMLVariant);
				NPN_SetProperty(args->m_navigator, NPVARIANT_TO_OBJECT(logElement), innerHTMLPropId, &newInnerHTMLVariant);
		
				//Release the old value
				NPN_ReleaseVariantValue(&innerHTMLVariant);
				NPN_ReleaseVariantValue(&logElement);
			}
			NPN_ReleaseVariantValue(&document);
		}
		NPN_ReleaseObject(window);
	}
	delete args;
}

#ifdef XP_WIN
AsyncCaller::AsyncCaller() {}

AsyncCaller::~AsyncCaller() {}

void AsyncCaller::CallAsync(NPP plugin, void (*func)(void*), void* args)
{
	NPN_PluginThreadAsyncCall(plugin, func, args);
}

#endif

#ifdef XP_MACOSX

struct MachMessage 
{
	mach_msg_header_t h;
	void (*f)(void*);
	void* a;
	NPP n;
};

void OnPortMessage(CFMachPortRef port, void* msg, CFIndex size, void* info)
{
	MachMessage* m = (MachMessage*)msg;
	m->f(m->a);  
}

AsyncCaller::AsyncCaller() 
{
	
	CFMachPortContext context;
	context.version = 0;
	context.info = this;
	context.retain = NULL;
	context.release = NULL;
	context.copyDescription = NULL;
	m_machPort = CFMachPortCreate(kCFAllocatorDefault, OnPortMessage, &context, false);
	m_runLoop = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_machPort, 0);
	m_mainLoop = CFRunLoopGetCurrent();
	CFRunLoopAddSource(m_mainLoop, m_runLoop, kCFRunLoopCommonModes);
	
}

AsyncCaller::~AsyncCaller() 
{
	CFRunLoopRemoveSource(m_mainLoop, m_runLoop, kCFRunLoopCommonModes);
	CFRelease(m_runLoop);
	CFRelease(m_machPort);
}

void AsyncCaller::CallAsync(NPP plugin, void (*func)(void*), void* args)
{
	if(CFEqual(m_mainLoop, CFRunLoopGetCurrent()))
	{
		func(args);
		return;
	}
	
	MachMessage m = {0};
	m.h.msgh_size = sizeof(mach_msg_header_t);
	m.h.msgh_remote_port = CFMachPortGetPort(m_machPort);
	m.h.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
									MACH_MSG_TYPE_MAKE_SEND_ONCE);
	m.f = func;
	m.a = args;
	m.n = plugin;
	mach_msg(&m.h, MACH_SEND_MSG, sizeof(MachMessage), 0, 0, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	
}

#endif


