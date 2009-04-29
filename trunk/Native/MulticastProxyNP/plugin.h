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

#ifndef INC_PLUGIN_H
#define INC_PLUGIN_H

#ifdef XP_WIN
#include <Windows.h>
#include <Wininet.h>
#include <stdio.h>
#include "npapi.h"
#include "npruntime.h"
#define NPSTR_CHARS(S) (S.utf8characters)
#define NPSTR_LEN(S) (S.utf8length)
#define snprintf _snprintf_s
#define strdup _strdup 
#endif

#ifdef XP_MACOSX
#include <CoreServices/CoreServices.h>
#include <mach/port.h>
#include <mach/message.h>
#include <Webkit/npapi.h>
#include <Webkit/npruntime.h>
#define NPSTR_CHARS(S) (S.UTF8Characters)
#define NPSTR_LEN(S) (S.UTF8Length)
#endif

#include "scriptableobject.h"
#include "../MulticastLib/MulticastReceiver.h"
#include "../MulticastLib/MulticastCallback.h"
#include "../MulticastLib/Logger.h"

#define LOG_DOM_ELEMENT "starlight_multicast_proxy_log"
#define JS_BUF_SZ 4096
#define NSC_BUF_SZ 32768
#define ERR_BUF_SZ 1024

class AsyncCaller 
{
public:
	AsyncCaller();
	~AsyncCaller();
	void CallAsync(NPP plugin, void (*func)(void*), void* args);
private:
	#ifdef XP_MACOSX
	CFMachPortRef m_machPort;
	CFRunLoopSourceRef m_runLoop;
	CFRunLoopRef m_mainLoop;
	#endif
};

class StarlightPlugin
{
private:
  NPP m_navigator;
  NPObject* m_scriptableObject;

public:
  StarlightPlugin(NPP pNPInstance);
  ~StarlightPlugin();

  NPObject* GetScriptableObject();
};

class DOMElementLoggerCallbackArgs
{
public:
	DOMElementLoggerCallbackArgs(const char* message, const char* type, NPP navigator) : m_navigator(navigator)
	{
		m_message = strdup(message);
		m_type = strdup(type);
	}

	~DOMElementLoggerCallbackArgs()
	{
		free(m_message);
		free(m_type);
	}

	NPP m_navigator;
	char* m_message;
	char* m_type;
};



//A class which implements the Logger interface by
//writing to a DOM Element named LOG_DOM_ELEMENT.
class DOMElementLogger : public Logger
{
public:
	DOMElementLogger(NPP navigator, AsyncCaller* async) : m_navigator(navigator), m_async(async)
	{
	}
	virtual ~DOMElementLogger()
	{
	}

	void LogTrace(const char*);
	void LogError(const char*);
	static void DoRealLog(void*);
private:
	NPP m_navigator;
	AsyncCaller* m_async;
};

class NPNInvokeMulticastCallbackArgs
{
public:
	NPNInvokeMulticastCallbackArgs(char* data, int szData, NPObject* target, NPIdentifier* method, Logger* logger, NPP navigator) : m_navigator(navigator), m_target(target), m_logger(logger), m_szData(szData), m_method(method), m_data(data)
	{
	}

	~NPNInvokeMulticastCallbackArgs()
	{
		delete[] m_data;
	}

	NPP m_navigator;
	Logger* m_logger;
	NPObject* m_target;
	int m_szData;
	char* m_data;
	NPIdentifier* m_method;
};


//A class that dispatches data via calls to an NPObject
//hosted in the browser.
class NPNInvokeMulticastCallback : public MulticastCallback
{
public:
	NPNInvokeMulticastCallback(NPP navigator, Logger* logger, AsyncCaller* async) : m_logger(logger), m_navigator(navigator), m_async(async)
	{
		m_target = NULL;
		ADD_PACKET_METHOD = NPN_GetStringIdentifier("AddPacketBase64");
	}

	virtual ~NPNInvokeMulticastCallback()
	{
		Release();
	}

	virtual void ReportPacketRead(int, MulticastCallbackData*);
	void Init(NPObject*);
	void Release();
	static void DoRealReportPacketRead(void*);
private:
	NPIdentifier ADD_PACKET_METHOD;
	NPP m_navigator;
	NPObject* m_target;
	Logger* m_logger;
	AsyncCaller* m_async;
};


class StarlightScriptable : public ScriptablePluginObjectBase
{
public:
  StarlightScriptable(NPP npp)
    : ScriptablePluginObjectBase(npp)
  {
      m_async = new AsyncCaller();
	  m_logger = new DOMElementLogger(npp, m_async);
	  m_callback = new NPNInvokeMulticastCallback(npp, m_logger, m_async);
	  m_receiver = CreateMulticastReceiver(m_logger);

	  START_STREAMING_METHOD = NPN_GetStringIdentifier("StartStreaming");
	  STOP_STREAMING_METHOD = NPN_GetStringIdentifier("StopStreaming");
	  FETCH_NSC_METHOD = NPN_GetStringIdentifier("FetchNSC");
	  TEST_METHOD = NPN_GetStringIdentifier("Test");
  }

  virtual ~StarlightScriptable()
  {
	  delete m_receiver;
	  delete m_callback;
	  delete m_logger;
	  delete m_async;
  }

  virtual bool HasMethod(NPIdentifier name);
  virtual bool Invoke(NPIdentifier name, const NPVariant *args,
                      uint32_t argCount, NPVariant *result);
  bool StartStreaming(NPString multicastGroup, int32_t port, NPObject* target, NPVariant *result);
  bool StopStreaming(NPVariant *result);
  bool Test(NPString message, NPVariant *result);
  bool FetchNSC(NPString url, NPVariant *result);
private:
  AsyncCaller* m_async;
  MulticastReceiver* m_receiver;
  Logger* m_logger;
  NPNInvokeMulticastCallback* m_callback;
  NPIdentifier START_STREAMING_METHOD;
  NPIdentifier STOP_STREAMING_METHOD;
  NPIdentifier FETCH_NSC_METHOD;
  NPIdentifier TEST_METHOD;
};

#endif // INC_PLUGIN_H
