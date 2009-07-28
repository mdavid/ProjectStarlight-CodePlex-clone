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

#include "multicastcallback.h"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void b64chunk(const unsigned char* in, char* out, int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
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
	
	m_invoker->Invoke(m_navigator, &NPNInvokeMulticastCallback::DoRealReportPacketRead, new NPNInvokeMulticastCallbackArgs(packetData, b64sz, m_target, &ADD_PACKET_METHOD, m_logger, m_navigator));
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