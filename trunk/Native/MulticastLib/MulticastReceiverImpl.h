#ifndef INC_MULTICAST_RECEIVER_IMPL_H
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

#define INC_MULTICAST_RECEIVER_IMPL_H

#include "MulticastLibCommon.h"
#include "MulticastReceiver.h"
#include "RingBuffer.h"

#define MCAST_BUF_SZ 32768
#define ERR_BUF_SZ 1024
#define IO_RECV_BUFS 32

#ifdef PLAT_WIN
typedef struct _MCAST_OVERLAPPED_EX
{
	OVERLAPPED overlapped;
	WSABUF buf;
	BOOL pending;
} MCAST_OVERLAPPED_EX;
#endif
class MulticastReceiverImpl : public MulticastReceiver
{

	friend THREAD_DECL MulticastReceiverThread(void*);
	friend THREAD_DECL MulticastEventNotificationThread(void*);
	
public:
	MulticastReceiverImpl(Logger*);
	virtual ~MulticastReceiverImpl(void);

	virtual int32_t StartReceiving(const char*, const char*, int, MulticastCallback*);
	virtual int32_t StopReceiving();

private:
	bool m_isReceiving;
	char* m_multicastGroup;
	char* m_multicastSource;
	MulticastCallback* m_callback;
	socket_t m_socket;
	thread_handle_t m_threadHandles[2];
	RingBuffer m_queue;
	Logger* m_logger;
	unsigned long m_bindAddr;
};

#endif /* INC_MULTICAST_RECEIVER_IMPL_H */