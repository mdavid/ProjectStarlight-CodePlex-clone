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

#include "stdafx.h"
#include "MulticastProxy.h"


// CMulticastProxy


STDMETHODIMP CMulticastProxy::StartStreaming(BSTR multicastGroup, USHORT multicastPort, BSTR multicastSource, IDispatch* target)
{
	USES_CONVERSION;

	char* cstrMulticastGroup = W2A(multicastGroup);
	char* cstrMulticastSource = W2A(multicastSource);
	m_callback->Init(target);
	DWORD rc = m_receiver->StartReceiving(cstrMulticastGroup, cstrMulticastSource, multicastPort, m_callback);
	if(rc != 0)
	{
		return HRESULT_FROM_WIN32(rc);
	}
	else
	{
		return S_OK;
	}
}

STDMETHODIMP CMulticastProxy::StopStreaming(void)
{
	DWORD rc = m_receiver->StopReceiving();
	m_callback->Release();
	if(rc != 0)
	{
		return HRESULT_FROM_WIN32(rc);
	}
	else
	{
		return S_OK;
	}
}

STDMETHODIMP CMulticastProxy::Test(BSTR message)
{
	//Pop up a message box to test that this control is working correctly.
	MessageBox(NULL, message, L"Test Message", MB_OK);
	return S_OK;
}

STDMETHODIMP CMulticastProxy::FetchNSC(BSTR nscFileUrl, BSTR* nscContent)
{
	HRESULT ret = S_OK;
	CComBSTR content;

	//Crack the URL
	URL_COMPONENTS urlParts;
	ZeroMemory(&urlParts, sizeof(urlParts));
	urlParts.dwUrlPathLength = 1;
	urlParts.dwExtraInfoLength = 1;
	urlParts.dwStructSize = sizeof(urlParts);
	if(!InternetCrackUrlW(nscFileUrl, 0, 0, &urlParts))
	{
		ret = HRESULT_FROM_WIN32(GetLastError());
		goto endFetchNSC;
	}

	//Make sure the URL ends with .nsc so that people can't use us to
	//fetch arbitrary files from the internet.
	if(urlParts.dwUrlPathLength < 4)
	{
		ret = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
		goto endFetchNSC;
	}
	if(memcmp(urlParts.lpszUrlPath + urlParts.dwUrlPathLength - 4, L".nsc", (4 * sizeof(wchar_t))) != 0)
	{
		ret = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
		goto endFetchNSC;
	}
	

	wchar_t* headers = L"Accept-Charset: ISO-8859-1\r\nAccept-Encoding: identity\r\n";

	//Open the internet handle
	HINTERNET internetHandle = InternetOpenW(L"Starlight Multicast Proxy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	//Open the requested URL
	HINTERNET urlHandle = InternetOpenUrlW(
		internetHandle, 
		nscFileUrl, 
		headers, 
		-1, 
		INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS,
		NULL);

	if(urlHandle == NULL)
	{
		ret = HRESULT_FROM_WIN32(GetLastError());
		InternetCloseHandle(internetHandle);
		goto endFetchNSC;
	}
	
	char buffer[NSC_BUF_SZ];
	
	DWORD bytesRead = 0;
	while(InternetReadFile(urlHandle, buffer, NSC_BUF_SZ - 1, &bytesRead))
	{

		if(bytesRead == 0)
		{
			break;
		}
		else
		{	
			buffer[bytesRead] = 0;
			content.Append((char*)buffer);
		}
	}

	if(bytesRead == 0) {
		ret = HRESULT_FROM_WIN32(GetLastError());
	}

	//Set the returned content
	*nscContent = content.Detach();

	//Cleanup
	InternetCloseHandle(urlHandle);
	InternetCloseHandle(internetHandle);

endFetchNSC:
	return ret;
}

STDMETHODIMP CMulticastProxy::SetSite(IUnknown *pUnkSite)
{
	//If the site is known try to find a log target element in it
	//to hand to our logger.
	if(pUnkSite != NULL)
	{
		IServiceProvider *serviceProvider = NULL;
		IWebBrowser2 *browser = NULL;
		IHTMLDocument3 *doc = NULL;
		IDispatch *docDisp = NULL;
		IHTMLElement *elem = NULL;

		CComBSTR elementName(LOG_DOM_ELEMENT);
		HRESULT hr;

		hr = pUnkSite->QueryInterface(IID_IServiceProvider, (void**)&serviceProvider);
		if(!SUCCEEDED(hr))
		{
			goto cleanup;
		}

		hr = serviceProvider->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, (void**)&browser);
		if(!SUCCEEDED(hr))
		{
			goto cleanup;
		}
		
		hr = browser->get_Document(&docDisp);
		if(!SUCCEEDED(hr))
		{
			goto cleanup;
		}
		hr = docDisp->QueryInterface(IID_IHTMLDocument3, (void**)&doc);
		if(!SUCCEEDED(hr))
		{
			goto cleanup;
		}

		hr = doc->getElementById(elementName, &elem);
		if(!SUCCEEDED(hr))
		{
			goto cleanup;
		}

		if(elem != NULL)
		{
			m_logger->SetElement(elem);
		}
cleanup:

		if(elem != NULL) elem->Release();
		if(doc != NULL) doc->Release();
		if(docDisp != NULL) docDisp->Release();
		if(browser != NULL) browser->Release();
		if(serviceProvider != NULL) serviceProvider->Release();
	}
	return IObjectWithSiteImpl::SetSite(pUnkSite);
}
