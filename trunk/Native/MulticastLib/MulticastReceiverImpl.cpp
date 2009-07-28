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

#include "MulticastReceiverImpl.h"

MulticastReceiverImpl::MulticastReceiverImpl(Logger* logger) : m_isReceiving(false), m_socket(-1), m_multicastGroup(NULL), m_logger(logger)
{
	m_threadHandles[0] = NULL;
	m_threadHandles[1] = NULL;
}

MulticastReceiverImpl::~MulticastReceiverImpl(void)
{
	if(m_isReceiving)
	{
		StopReceiving();
	}
}

int32_t MulticastReceiverImpl::StartReceiving(const char* multicastGroup, const char* multicastSource, int port, MulticastCallback* callback)
{
	if(m_isReceiving)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "already receiving\n");
		m_logger->LogError(errBuf);
		return 0;
	}
	
	char traceBuf[ERR_BUF_SZ];
	memset(traceBuf, 0, ERR_BUF_SZ);
	snprintf(traceBuf, ERR_BUF_SZ - 1, "start receiving from %s:%d", multicastGroup, port);
	m_logger->LogTrace(traceBuf);

	int32_t recvBufSz;
	int32_t reuse;
	struct timeval recvTimeout = {0};
	int rc;
	sockaddr_in_t addrLocal;

	m_callback = callback;
	m_multicastGroup = strdup(multicastGroup);
	m_multicastSource = strdup(multicastSource);

	//Create the socket
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(m_socket < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "socket create failed: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_logger->LogTrace("Socket created");

	//Set the receive timeout
	recvTimeout.tv_sec = 1;
	rc = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,(char*)&recvTimeout,sizeof(struct timeval));
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "set socket receive timeout failed: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_logger->LogTrace("Socket set timeout");

	//Set the receive buffer for this socket
	recvBufSz = MCAST_BUF_SZ * 8;
	rc = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF,(char*)&recvBufSz, sizeof(int32_t));
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		if(rc == ENOBUFS)
		{
			recvBufSz = 65535;
			rc = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF,(char*)&recvBufSz, sizeof(int32_t));
		}
		else
		{
			rc = -1;
		}
		
		if(rc < 0)
		{
			rc = LAST_SOCK_ERROR;
			char errBuf[ERR_BUF_SZ];
			memset(errBuf, 0, ERR_BUF_SZ);
			snprintf(errBuf, ERR_BUF_SZ - 1, "set socket receive buffer failed: %d\n", rc);
			m_logger->LogError(errBuf);
			goto errorEnd;
		}
	}
	m_logger->LogTrace("Socket set receive buffer");

	//Set the reuse for this socket
	reuse = 1;
	rc = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,(char*)&reuse, sizeof(int32_t));
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "set socket reuse failed: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_logger->LogTrace("Socket set reuse");
	
	//Bind the socket
	addrLocal.sin_family = AF_INET;
	addrLocal.sin_addr.s_addr = INADDR_ANY;
	addrLocal.sin_port = htons(port); 
	rc = bind(m_socket, (sockaddr*)&addrLocal, sizeof(struct sockaddr));
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "socket bind failed: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_logger->LogTrace("Socket bound");
	
	//join the multicast group
	#ifdef PLAT_WIN
	if(strlen(m_multicastSource) > 0) 
	{
		m_logger->LogTrace("Joining source specific group");
		ip_mreq_source multicastOptions;
		memset(&multicastOptions, 0, sizeof(multicastOptions));
		multicastOptions.imr_interface.s_addr = INADDR_ANY;
		multicastOptions.imr_multiaddr.s_addr = inet_addr(m_multicastGroup);
		multicastOptions.imr_sourceaddr.s_addr = inet_addr(m_multicastSource);
		rc = setsockopt(m_socket, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char*)&multicastOptions, sizeof(multicastOptions));  
		
	}
	else
	{
	#endif
	
		m_logger->LogTrace("Joining group");
		ip_mreq multicastOptions;
		memset(&multicastOptions, 0, sizeof(multicastOptions));
		multicastOptions.imr_interface.s_addr = INADDR_ANY;
		multicastOptions.imr_multiaddr.s_addr = inet_addr(m_multicastGroup);
		rc = setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&multicastOptions, sizeof(multicastOptions));  
	#ifdef PLAT_WIN
	}
	#endif
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "join multicast group failed: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_logger->LogTrace("Socket joined group");

	m_queue.Clear();
	m_isReceiving = true;
	
	#ifdef PLAT_WIN
	m_threadHandles[0] = CreateThread(NULL, 0, &MulticastReceiverThread, (LPVOID)this, 0, NULL);
	if(NULL == m_threadHandles[0])
	{
		rc = GetLastError();
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error creating thread: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	m_threadHandles[1] = CreateThread(NULL, 0, &MulticastEventNotificationThread, (LPVOID)this, 0, NULL);
	if(NULL == m_threadHandles[1])
	{
		rc = GetLastError();
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error creating thread: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	#endif /* PLAT_WIN */
	
	#ifdef PLAT_MAC
	rc = pthread_create(&m_threadHandles[0], NULL,  &MulticastReceiverThread, (void*)this);
	if(rc)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error creating thread: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	rc = pthread_create(&m_threadHandles[1], NULL,  &MulticastEventNotificationThread, (void*)this);
	if(rc)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error creating thread: %d\n", rc);
		m_logger->LogError(errBuf);
		goto errorEnd;
	}
	#endif /* PLAT_MAC */
	
	m_logger->LogTrace("Threads started");
	goto normalEnd;
errorEnd:
	m_isReceiving = false;
	free(m_multicastGroup);
	free(m_multicastSource);
	if(m_socket > -1)
	{
		SOCK_CLOSE(m_socket);
	}
	
	#ifdef PLAT_WIN
	if(m_threadHandles[0] != NULL)
	{
		CloseHandle(m_threadHandles[0]);
	}
	if(m_threadHandles[0] != NULL)
	{
		CloseHandle(m_threadHandles[1]);
	}
	#endif
normalEnd:
	m_logger->LogTrace("Done with start");
	return rc;
}

int32_t MulticastReceiverImpl::StopReceiving()
{
	m_logger->LogTrace("Stopping");
	struct ip_mreq multicastOptions; 
	int rc;

	m_isReceiving = false;
	m_queue.Stop();
	#ifdef PLAT_WIN
	WaitForMultipleObjects(2, m_threadHandles, TRUE, INFINITE);
	CloseHandle(m_threadHandles[0]);
	CloseHandle(m_threadHandles[1]);
	#endif
	#ifdef PLAT_MAC
	rc = pthread_join(m_threadHandles[0], NULL);
	if(rc)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error joining thread: %d\n", rc);
		m_logger->LogError(errBuf);
	}
	rc = pthread_join(m_threadHandles[1], NULL);
	if(rc)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "error joining thread: %d\n", rc);
		m_logger->LogError(errBuf);
	}
	#endif
	
	m_threadHandles[0] = NULL;
	m_threadHandles[1] = NULL;
	multicastOptions.imr_interface.s_addr = INADDR_ANY;
	multicastOptions.imr_multiaddr.s_addr = inet_addr(m_multicastGroup);
	rc = setsockopt(m_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&multicastOptions, sizeof(multicastOptions));  
	if(rc < 0)
	{
		rc = LAST_SOCK_ERROR;
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "leave multicast group failed: %d\n", rc);
		m_logger->LogError(errBuf);
	}

	SOCK_CLOSE(m_socket);
	free(m_multicastGroup);
	free(m_multicastSource);
	m_logger->LogTrace("Done with stop");
	return rc;
}


THREAD_DECL MulticastReceiverThread(void* iValue)
{
	int rc;
	MulticastReceiverImpl* receiver = (MulticastReceiverImpl*)iValue;
	RingBuffer& queue = receiver->m_queue;
	while(receiver->m_isReceiving)
	{
		unsigned char buffer[MCAST_BUF_SZ];
		rc = recv(receiver->m_socket, (char*)&buffer, MCAST_BUF_SZ, 0);
		if(rc > -1)
		{
			queue.AddPacket(new RingBufferPacket((unsigned char*)buffer, (unsigned int)rc));
		}
		else
		{
			rc = LAST_SOCK_ERROR;
			if(rc != ETIMEDOUT && rc != EWOULDBLOCK)
			{
				char errBuf[ERR_BUF_SZ];
				memset(errBuf, 0, ERR_BUF_SZ);
				snprintf(errBuf, ERR_BUF_SZ - 1, "recv failed: %d\n", rc);
				receiver->m_logger->LogError(errBuf);
			}
		}
	}
	return 0;
}

THREAD_DECL MulticastEventNotificationThread(void* iValue)
{
	MulticastReceiverImpl* receiver = (MulticastReceiverImpl*)iValue;
	RingBuffer& queue = receiver->m_queue;
	RingBufferPacket* packets[RING_BUFFER_SZ];
	int count;
	while((count = queue.TakeMultiple(packets, RING_BUFFER_SZ)) != -1)
	{
		if(count > 0)
		{
			MulticastCallbackData* callbackData = new MulticastCallbackData[count];
			for(int i = 0; i < count; i++)
			{
				callbackData[i].data = packets[i]->GetData();
				callbackData[i].szData = packets[i]->GetLength();
			}
			receiver->m_callback->ReportPacketRead(count, callbackData);

			delete[] callbackData;
			for(int i = 0; i < count; i++)
			{
				delete packets[i];
			}
		}
	}
	return 0;
}

MulticastReceiver* CreateMulticastReceiver(Logger* logger)
{
	return new MulticastReceiverImpl(logger);
}
