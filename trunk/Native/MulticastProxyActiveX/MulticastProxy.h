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

#pragma once
#include "resource.h"       // main symbols
#include "MulticastReceiver.h"
#include "MulticastCallback.h"
#include "Logger.h"

#define ERR_BUF_SZ 1024
#define NSC_BUF_SZ 32768
#define LOG_DOM_ELEMENT L"starlight_multicast_proxy_log"

// IMulticastProxy
[
	object,
	uuid("FD7B677B-6749-4574-93E9-EE2CF1458D50"),
	dual,	helpstring("IMulticastProxy Interface"),
	pointer_default(unique)
]
__interface IMulticastProxy : IDispatch
{
	[id(1), helpstring("method StartStreaming")] HRESULT StartStreaming([in] BSTR multicastGroup, [in] USHORT multicastPort, [in] BSTR multicastSource, [in] IDispatch* target);
	[id(2), helpstring("method StopStreaming")] HRESULT StopStreaming(void);
	[id(3), helpstring("method Test")] HRESULT Test([in] BSTR message);
	[id(4), helpstring("method FetchNSC")] HRESULT FetchNSC([in] BSTR nscFileUrl, [out,retval] BSTR* nscContent);
};


//Helper classes/functions

static OLECHAR* NAME_ADD_PACKET = OLESTR("AddPacketBase64");

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void b64chunk(const unsigned char* in, char* out, int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

//A class which implements the Logger interface by
//writing to a DOM Element named LOG_DOM_ELEMENT.
class DOMElementLogger : public Logger
{
public:
	DOMElementLogger() : m_element(NULL)
	{
	}
	virtual ~DOMElementLogger()
	{
		if(m_element != NULL)
		{
			m_element->Release();
		}
	}
	void SetElement(IHTMLElement* element)
	{
		element->AddRef();
		m_element = element;
	}
	virtual void LogTrace(const char * data)
	{
		if(m_element != NULL)
		{
			CComBSTR newText; 
			newText.Empty();
			m_element->get_innerHTML(&newText);
			newText.Append("TRACE: ");
			newText.Append(data);
			newText.Append("<BR>");
			m_element->put_innerHTML(newText);
		}
	}

	virtual void LogError(const char * data)
	{
		if(m_element != NULL)
		{
			CComBSTR newText; 
			newText.Empty();
			m_element->get_innerHTML(&newText);
			newText.Append("ERROR: ");
			newText.Append(data);
			newText.Append("<BR>");
			m_element->put_innerHTML(newText);
		}
	}



private:
	IHTMLElement* m_element;
};


//A class that dispatches data via calls to an IDispatch
//hosted in the browser.
class DispatchInvokeMulticastCallback  : public MulticastCallback
{
public:
	DispatchInvokeMulticastCallback(Logger* logger)
	{
		m_target = NULL;
		m_logger = logger;
	}

	virtual ~DispatchInvokeMulticastCallback()
	{
		Release();
	}

	void Init(IDispatch* target)
	{
		Release();
		m_target = target;
		m_target->AddRef();

		HRESULT result = m_target->GetIDsOfNames(IID_NULL, &NAME_ADD_PACKET, 1, LOCALE_SYSTEM_DEFAULT, &m_dispId);
		if(FAILED(result))
		{
			char buf[ERR_BUF_SZ];
			ZeroMemory(buf, ERR_BUF_SZ);
			_snprintf_s(buf, ERR_BUF_SZ, "GetIDsOfNames: %d", HRESULT_CODE(result));
			m_logger->LogError(buf);
		}
	}

	void Release()
	{
		if(m_target != NULL)
		{
			m_target->Release();
			m_target = NULL;
		}
	}

	virtual void ReportPacketRead(int count, MulticastCallbackData* data)
	{
		if(NULL == m_target)
		{
			return;
		}

		HRESULT result;
		
		DISPPARAMS args;
		VARIANTARG arg;
		args.rgvarg = &arg;
		args.cArgs = 1;
		args.cNamedArgs = 0;
		char outBuf[5] = {0, 0, 0, 0, 0};

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


		int b64sz = ((totalPacketSz / 3) + (totalPacketSz % 3 > 0 ? 1 : 0)) * 4 + 1;
		char* b64Data = new char[b64sz];
		b64Data[b64sz - 1] = 0;
		int srcIdx = 0;
		int destIdx = 0;
		while(srcIdx < totalPacketSz) 
		{
			int len = totalPacketSz - srcIdx;
			if(len > 3) {
				len = 3;
			}
			b64chunk(allData + srcIdx, b64Data + destIdx, len);
			srcIdx += len;
			destIdx += 4;
		}

		delete[] allData;

		CComBSTR packetData(b64Data);

		delete[] b64Data;

		arg.bstrVal = packetData;
		arg.vt = VT_BSTR;
		
		
		result = m_target->Invoke(m_dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, NULL, NULL, NULL);
		if(FAILED(result))
		{
			char buf[ERR_BUF_SZ];
			ZeroMemory(buf, ERR_BUF_SZ);
			_snprintf_s(buf, ERR_BUF_SZ, "Invoke: %d", HRESULT_CODE(result));
			m_logger->LogError(buf);
		}
		
	}
private:
	IDispatch* m_target;
	DISPID m_dispId;
	Logger* m_logger;
};



// CMulticastProxy

[
	coclass,
	default(IMulticastProxy),
	threading(free),
	vi_progid("Starlight.MulticastProxy"),
	progid("Starlight.MulticastProxy.1"),
	version(1.0),
	uuid("5CCEF05B-17B2-48F0-A577-CC019FE455D7"),
	helpstring("Starlight Multicast Proxy Class")
]
class ATL_NO_VTABLE CMulticastProxy :
	public IObjectWithSiteImpl<CMulticastProxy>,
	public IObjectSafetyImpl<CMulticastProxy, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
	public IMulticastProxy
{
public:
	CMulticastProxy()
	{
		m_logger = new DOMElementLogger();
		m_callback = new DispatchInvokeMulticastCallback(m_logger);
		m_receiver = CreateMulticastReceiver(m_logger);
	}

	~CMulticastProxy()
	{
		delete m_receiver;
		delete m_callback;
		delete m_logger;
	}

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(StartStreaming)(BSTR multicastGroup, USHORT multicastPort, BSTR multicastSource, IDispatch* target);
	STDMETHOD(StopStreaming)(void);
	STDMETHOD(Test)(BSTR message);
	STDMETHOD(SetSite)(IUnknown*);

private:
	DOMElementLogger* m_logger;
	MulticastReceiver* m_receiver;
	DispatchInvokeMulticastCallback* m_callback;
public:
	STDMETHOD(FetchNSC)(BSTR nscFileUrl, BSTR* nscContent);
};

