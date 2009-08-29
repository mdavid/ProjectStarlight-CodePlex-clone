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

#ifndef INC_RINGBUFFER_H
#define INC_RINGBUFFER_H

#include "MulticastLibCommon.h"

/*
 * This class is a simple thread safe ring-buffer that will overwrite old entries as it wraps around.
 */

#define RING_BUFFER_SZ 256
#define STOP_HANDLE 0
#define READY_HANDLE 1

#ifdef PLAT_WIN
#define ATOMIC_CAS(target, newval, oldval) (InterlockedCompareExchange((volatile LONG*)&target, newval, oldval) == oldval)
#define ATOMIC_CLR(target) InterlockedExchange((volatile LONG*)&target, 0)
#define ATOMIC_ALIGN __declspec(align(32)) volatile
typedef unsigned short uint16_t; 
typedef unsigned long uint32_t;
#endif

#ifdef PLAT_MAC
#define ATOMIC_CAS(target, newval, oldval) OSAtomicCompareAndSwap32((int32_t)oldval, (int32_t)newval, (int32_t*)&target)
#define ATOMIC_CLR(target) (target = 0)
#define ATOMIC_ALIGN __attribute__ ((aligned (32)))
#endif

struct head_and_tail {
	uint16_t head;
	uint16_t tail;
};

union rb_ptrs {
	head_and_tail ht;
	uint32_t val;
};

class RingBufferPacket
{
public:
	RingBufferPacket(const unsigned char* data, unsigned int length);
	~RingBufferPacket();
	const unsigned char* GetData();
	unsigned int GetLength();
private:
	unsigned char* m_data;
	unsigned int m_length;
};

class RingBuffer
{
public:
	RingBuffer(void);
	~RingBuffer(void);
	void AddPacket(RingBufferPacket*);
	int TakeMultiple(RingBufferPacket*[], unsigned int);
	void Stop();
	void Clear();

private:
	RingBufferPacket* m_data[RING_BUFFER_SZ];
	ATOMIC_ALIGN rb_ptrs m_pointers;
	bool m_stopped;
};

#endif /* INC_RINGBUFFER_H */


