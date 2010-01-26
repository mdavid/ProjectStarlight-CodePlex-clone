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
	//check if we are already receiving
	if(m_isReceiving)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "already receiving\n");
		m_logger->LogError(errBuf);
		return 0;
	}
	
	//Start the ring buffer
	m_queue.Start();

	//Log
	char traceBuf[ERR_BUF_SZ];
	memset(traceBuf, 0, ERR_BUF_SZ);
	snprintf(traceBuf, ERR_BUF_SZ - 1, "start receiving from %s:%d", multicastGroup, port);
	m_logger->LogTrace(traceBuf);

	//Init variables
	int32_t recvBufSz;
	int32_t reuse;
	struct timeval recvTimeout = {0};
	int rc;
	sockaddr_in_t addrLocal;

	//copy our input in
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
	m_bindAddr = INADDR_ANY;

	//If on windows, determine the address to bind to.  Needed
	//for the multi-adapter case, since using INADDR_ANY seems 
	//to pick a random adapter to use for IGMP, and hence no multicast
	//packets are received.
	#ifdef PLAT_WIN

	//Find the best interface to use
	DWORD ifIdx = -1;

	//If we have a multicast source IP, find the best interface for that
	if(strlen(m_multicastSource) > 0) 
	{
		GetBestInterface(inet_addr(m_multicastSource), &ifIdx);
	} 
	else //Find the best interface for the default route.
	{
		GetBestInterface(inet_addr("0.0.0.0"), &ifIdx);
	}

	//Find the IP address for the adapter we found earlier
	DWORD szAdapterInfo = 0;
	if(GetAdaptersInfo(NULL, &szAdapterInfo) == ERROR_BUFFER_OVERFLOW) 
	{
		m_logger->LogTrace("Allocated adapter buffer"); 
		PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR, szAdapterInfo);
		rc = GetAdaptersInfo(pAdapterInfo, &szAdapterInfo);
		if(rc == 0) 
		{
			for(PIP_ADAPTER_INFO pCurrentAdapter = pAdapterInfo; pCurrentAdapter; pCurrentAdapter = pCurrentAdapter->Next)
			{
				m_logger->LogTrace("Checking adapter"); 
				if(pCurrentAdapter->Index == ifIdx)
				{
					m_logger->LogTrace("Found adapter"); 
					if(pCurrentAdapter->IpAddressList.IpAddress.String > 0)
					{
						char msgBuf[ERR_BUF_SZ];
						memset(msgBuf, 0, ERR_BUF_SZ);
						snprintf(msgBuf, ERR_BUF_SZ - 1, "binding to: %s(%d)\n", pCurrentAdapter->IpAddressList.IpAddress.String, inet_addr(pCurrentAdapter->IpAddressList.IpAddress.String));
						m_logger->LogTrace(msgBuf);
						m_bindAddr = addrLocal.sin_addr.s_addr = inet_addr(pCurrentAdapter->IpAddressList.IpAddress.String);
						break;
					}
				}
			}
		}
		else
		{
			char errBuf[ERR_BUF_SZ];
			memset(errBuf, 0, ERR_BUF_SZ);
			snprintf(errBuf, ERR_BUF_SZ - 1, "list adapters failed: %d\n", rc);
			m_logger->LogError(errBuf);
		}

		GlobalFree((HGLOBAL)pAdapterInfo);
	}
	#endif /* PLAT_WIN */

	//Bind the socket
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
	//If on windows, try to use source specific multicast
	if(strlen(m_multicastSource) > 0) 
	{
		m_logger->LogTrace("Joining source specific group");
		ip_mreq_source multicastOptions;
		memset(&multicastOptions, 0, sizeof(multicastOptions));
		multicastOptions.imr_interface.s_addr = addrLocal.sin_addr.s_addr;
		multicastOptions.imr_multiaddr.s_addr = inet_addr(m_multicastGroup);
		multicastOptions.imr_sourceaddr.s_addr = inet_addr(m_multicastSource);
		rc = setsockopt(m_socket, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char*)&multicastOptions, sizeof(multicastOptions));  
		
	}
	else
	{
	#endif /* PLAT_WIN */
		//If mac or no source address, join the group with no source specified.
		m_logger->LogTrace("Joining group");
		ip_mreq multicastOptions;
		memset(&multicastOptions, 0, sizeof(multicastOptions));
		multicastOptions.imr_interface.s_addr = addrLocal.sin_addr.s_addr;
		multicastOptions.imr_multiaddr.s_addr = inet_addr(m_multicastGroup);
		rc = setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&multicastOptions, sizeof(multicastOptions));  
	#ifdef PLAT_WIN
	}
	#endif /* PLAT_WIN */

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
	
	//Start the receiver threads
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
	if(!m_isReceiving)
	{
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "already stopped\n");
		m_logger->LogError(errBuf);
		return 0;
	}
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
	multicastOptions.imr_interface.s_addr = m_bindAddr;
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

	#ifdef PLAT_WIN
	//On windows use overlapped I/O and completion ports
	//for efficient receiving.  Seems to be needed to not drop
	//packets on Vista/Win7
	HANDLE hCompletion;
	MCAST_OVERLAPPED_EX overlapped[IO_RECV_BUFS];
	DWORD bytesIn = 0;
	DWORD flags = 0;
	ULONG uniqueKey = 0;
	MCAST_OVERLAPPED_EX* pending;

	//Initialize the IO Completion port
	hCompletion = CreateIoCompletionPort((HANDLE)receiver->m_socket, NULL, NULL, 1);
	if(!hCompletion)
	{
		rc = GetLastError();
		char errBuf[ERR_BUF_SZ];
		memset(errBuf, 0, ERR_BUF_SZ);
		snprintf(errBuf, ERR_BUF_SZ - 1, "create completion port failed: %d\n", rc);
		receiver->m_logger->LogError(errBuf);
		goto errorEnd;
	}

	//Set up overlapped IO
	for(int i = 0; i < IO_RECV_BUFS; i++)
	{
		memset(&overlapped[i].overlapped,  0, sizeof(OVERLAPPED));
		overlapped[i].buf.len = MCAST_BUF_SZ;
		overlapped[i].buf.buf = new char[MCAST_BUF_SZ];
		WSARecv(receiver->m_socket, &overlapped[i].buf, 1, NULL, &flags, &overlapped[i].overlapped, NULL);
	}
	#endif /* PLAT_WIN */

	while(receiver->m_isReceiving)
	{
		unsigned char buffer[MCAST_BUF_SZ];

		#ifdef PLAT_WIN
		flags = 0;
		bytesIn = 0;
		if(GetQueuedCompletionStatus(hCompletion, &bytesIn, &uniqueKey, (OVERLAPPED **)&pending, 1000) != 0)
		{
			rc = WSAGetOverlappedResult(receiver->m_socket, (OVERLAPPED *)pending, &bytesIn, FALSE, &flags);
			if(rc)
			{
				queue.AddPacket(new RingBufferPacket((unsigned char*)pending->buf.buf, (unsigned int)bytesIn));
			}
			else
			{
				rc = LAST_SOCK_ERROR;
				char errBuf[ERR_BUF_SZ];
				memset(errBuf, 0, ERR_BUF_SZ);
				snprintf(errBuf, ERR_BUF_SZ - 1, "recv failed: %d\n", rc);
				receiver->m_logger->LogError(errBuf);
			}
			memset(&pending->overlapped, 0, sizeof(OVERLAPPED));
			flags = 0;
			WSARecv(receiver->m_socket, &pending->buf, 1, NULL, &flags, &pending->overlapped, NULL);
		}
		#endif /* PLAT_WIN */

		#ifdef PLAT_MAC
		//Use simple recv loop for Mac.
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
		#endif /* PLAT_MAC */
		
	}

#ifdef PLAT_WIN
	for(int i = 0; i < IO_RECV_BUFS; i++)
	{
		delete[] overlapped[i].buf.buf;
	}
#endif

errorEnd:
#ifdef PLAT_WIN
	CloseHandle(hCompletion);
#endif
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
		} else {
			SLEEP_MILLI(1);
		}
	}
	return 0;
}

MulticastReceiver* CreateMulticastReceiver(Logger* logger)
{
	return new MulticastReceiverImpl(logger);
}
