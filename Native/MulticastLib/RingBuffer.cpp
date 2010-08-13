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

#include "RingBuffer.h"

RingBuffer::RingBuffer(void) : m_head(0), m_tail(0), m_stopped(FALSE)
{	
	ATOMIC_CLR(m_fillCount);
	for(int i = 0; i < RING_BUFFER_SZ; i++)
	{
		m_data[i].data = new unsigned char[MCAST_BUF_SZ];
		m_data[i].szData = 0;
	}
	ATOMIC_BARRIER();
}

RingBuffer::~RingBuffer(void)
{
	ATOMIC_BARRIER();
	for(int i = 0; i < RING_BUFFER_SZ; i++)
	{
		delete[] m_data[i].data;
	}
}

void RingBuffer::AddPacket(const unsigned char* data, unsigned int szData)
{
	ATOMIC_ALIGN uint32_t oldFillCount;
	ATOMIC_ALIGN uint32_t newFillCount;

	//This should be safe on x86.
	if(m_fillCount == RING_BUFFER_SZ) 
	{
		return;
	}

	//Add the packet and move the tail pointer.  
	memcpy(m_data[m_tail].data, data, szData);
	m_data[m_tail].szData = szData;
	m_tail = (m_tail + 1) % RING_BUFFER_SZ;

	//Increase our fill count so the reader sees the packet
	do {
		oldFillCount = m_fillCount;
		newFillCount = oldFillCount + 1;
	} while(!ATOMIC_CAS(m_fillCount, newFillCount, oldFillCount));
}

int RingBuffer::TakeMultiple(MulticastCallback* callback)
{
	ATOMIC_ALIGN uint32_t oldFillCount;
	ATOMIC_ALIGN uint32_t newFillCount;
	uint32_t toFetch;

	//We're stopped...let the caller know
	if(m_stopped)
	{
		return -1;
	}

	//Fetch and update a value of fill count.
	do {
		oldFillCount = m_fillCount;
		toFetch = RING_BUFFER_SZ - m_head;

		//Don't fetch across the buffer loop boundary
		//so we can present the callback with a contiguous 
		//block of memory.
		if(oldFillCount < toFetch)
		{
			toFetch = oldFillCount;
		}
		newFillCount = oldFillCount - toFetch;
	} while(!ATOMIC_CAS(m_fillCount, newFillCount, oldFillCount));

	callback->ReportPacketRead(toFetch, m_data + m_head);

	m_head = (m_head + toFetch) % RING_BUFFER_SZ;
	
	return toFetch;
}

void RingBuffer::Stop()
{
	m_stopped = true;
	ATOMIC_BARRIER();
}

void RingBuffer::Start()
{
	m_stopped = false;
	ATOMIC_BARRIER();
}

void RingBuffer::Clear()
{
	ATOMIC_CLR(m_fillCount);
	m_head = 0;
	m_tail = 0;
	ATOMIC_BARRIER();
}

