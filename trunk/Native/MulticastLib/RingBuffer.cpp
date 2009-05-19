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

RingBuffer::RingBuffer(void) : m_head(0), m_tail(0)
{
	#ifdef PLAT_MAC
	pthread_mutex_init(&m_mutex, NULL);
	pthread_cond_init(&m_event, NULL);
	m_stop = false;
	m_dataReady = false;
	#endif
	
	#ifdef PLAT_WIN
	InitializeCriticalSection(&m_crit);
	m_handles[READY_HANDLE] = CreateEvent(NULL, 0, FALSE, NULL);
	m_handles[STOP_HANDLE] = CreateEvent(NULL, TRUE, FALSE, NULL);
	#endif
}

RingBuffer::~RingBuffer(void)
{
	#ifdef PLAT_MAC
	pthread_mutex_destroy(&m_mutex);
	#endif
	
	#ifdef PLAT_WIN
	DeleteCriticalSection(&m_crit);
	CloseHandle(m_handles[READY_HANDLE]);
	CloseHandle(m_handles[STOP_HANDLE]);
	#endif
}

void RingBuffer::AddPacket(RingBufferPacket* packet)
{
	#ifdef PLAT_MAC
	pthread_mutex_lock(&m_mutex);
	#endif
	
	#ifdef PLAT_WIN
	EnterCriticalSection(&m_crit);
	#endif
	
	m_data[m_tail] = packet;

	m_tail = (m_tail + 1) % RING_BUFFER_SZ;
	if(m_tail == m_head)
	{
		delete m_data[m_head];
		m_head = (m_head + 1) % RING_BUFFER_SZ;
	}
	
	
	#ifdef PLAT_WIN
	SetEvent(m_handles[READY_HANDLE]);
	LeaveCriticalSection(&m_crit);
	#endif
	
	#ifdef PLAT_MAC
	m_dataReady = true;
	pthread_cond_signal(&m_event);
	pthread_mutex_unlock(&m_mutex);
	#endif
	
}

int RingBuffer::TakeMultiple(RingBufferPacket** data, unsigned int count)
{
	int ret = 0;
	if(count == 0)
	{
		return 0;
	}

	#ifdef PLAT_MAC
	pthread_mutex_lock(&m_mutex);
	while(!m_dataReady && !m_stop) 
	{
		pthread_cond_wait(&m_event, &m_mutex);
	}
	if(m_stop)
	{
		ret = -1;
	}
	else if(m_dataReady)
	{
		while(ret < count && m_head != m_tail)
		{
			data[ret] = m_data[m_head];
			m_head = (m_head + 1) % RING_BUFFER_SZ;
			ret++;
		}
		m_dataReady = false;
	}
	
	pthread_mutex_unlock(&m_mutex);
	return ret;
	#endif
	
	#ifdef PLAT_WIN
	switch(WaitForMultipleObjects(2, m_handles, FALSE, INFINITE))
	{
	case STOP_HANDLE:
		return -1;
	case READY_HANDLE:
		EnterCriticalSection(&m_crit);
		while(ret < count && m_head != m_tail)
		{
			data[ret] = m_data[m_head];
			m_head = (m_head + 1) % RING_BUFFER_SZ;
			ret++;
		}
		LeaveCriticalSection(&m_crit);
		return ret;
	default:
		return -1;
	}
	#endif
}

void RingBuffer::Stop()
{
	#ifdef PLAT_MAC
	pthread_mutex_lock(&m_mutex);
	m_stop = true;
	pthread_cond_signal(&m_event);
	pthread_mutex_unlock(&m_mutex);
	#endif
	
	#ifdef PLAT_WIN
	SetEvent(m_handles[STOP_HANDLE]);
	#endif
}

void RingBuffer::Clear()
{
	#ifdef PLAT_MAC
	pthread_mutex_lock(&m_mutex);
	#endif
	
	#ifdef PLAT_WIN
	EnterCriticalSection(&m_crit);
	#endif
	
	while(m_head != m_tail)
	{
		delete m_data[m_head];
		m_head = (m_head + 1) % RING_BUFFER_SZ;
	}
	
	#ifdef PLAT_WIN
	SetEvent(m_handles[READY_HANDLE]);
	LeaveCriticalSection(&m_crit);
	#endif
	
	#ifdef PLAT_MAC
	pthread_mutex_unlock(&m_mutex);
	#endif
}


RingBufferPacket::RingBufferPacket(const unsigned char* data, unsigned int length)
{
	m_data = new unsigned char[length];
	memcpy(m_data, data, length);
	m_length = length;
}

RingBufferPacket::~RingBufferPacket()
{
	delete[] m_data;
}

const unsigned char* RingBufferPacket::GetData()
{
	return m_data;
}

unsigned int RingBufferPacket::GetLength()
{
	return m_length;
}
