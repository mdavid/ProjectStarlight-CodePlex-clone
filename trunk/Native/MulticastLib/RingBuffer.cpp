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

RingBuffer::RingBuffer(void)
{	
	ATOMIC_CLR(m_pointers.val);
	ATOMIC_CLR(m_free);
	m_stopped = false;
}

RingBuffer::~RingBuffer(void)
{
}

void RingBuffer::AddPacket(RingBufferPacket* packet)
{
	rb_ptrs newPtrs, oldPtrs;
	uint32_t oldFree, newFree;

	//Try to allocate a slot to put the data in
	do {
		oldPtrs.val = m_pointers.val;
		newPtrs.val = oldPtrs.val;
		newPtrs.ht.tail = (oldPtrs.ht.tail + 1) % RING_BUFFER_SZ;

		//We've filled our buffer. Delete this packet and return.
		if(newPtrs.ht.tail == oldPtrs.ht.head) {
			printf("Buffer full, dropping packet");
			delete packet;
			return;
		}
	} while(!ATOMIC_CAS(m_pointers.val, newPtrs.val, oldPtrs.val));

	//We allocated a slot.  Put this packet in it.
	m_data[oldPtrs.ht.tail] = packet;

	do {
		oldFree = m_free;
		newFree = oldFree + 1;
	} while(!ATOMIC_CAS(m_free, newFree, oldFree));
}

int RingBuffer::TakeMultiple(RingBufferPacket** data, unsigned int count)
{
	int ret = 0;
	rb_ptrs newPtrs, oldPtrs;
	uint32_t oldFree, newFree;

	//We're stopped...let the caller know
	if(m_stopped)
	{
		return -1;
	}

	//They requested no data, let's give it to them
	if(count == 0)
	{
		return 0;
	}

	//Try to fill the return buffer
	while(ret < count) {
		//Read one packet atomically
		do {
			oldFree = m_free;
			if(oldFree == 0) {
				return ret;
			}
			newFree = oldFree - 1;
		} while(!ATOMIC_CAS(m_free, newFree, oldFree));

		do {
			oldPtrs.val = m_pointers.val;
			newPtrs.val = oldPtrs.val;

			//Test for empty buffer
			if(oldPtrs.ht.head == oldPtrs.ht.tail) {
				//It's empty, return the number of packets we have already read
				return ret;
			} else {
				//It's not empty, advance the head
				newPtrs.ht.head = (oldPtrs.ht.head + 1) % RING_BUFFER_SZ;
			}
		} while(!ATOMIC_CAS(m_pointers.val, newPtrs.val, oldPtrs.val));

		data[ret] = m_data[oldPtrs.ht.head];
		ret++;
	}
	return ret;
}

void RingBuffer::Stop()
{
	m_stopped = true;
}

void RingBuffer::Start()
{
	m_stopped = false;
}

void RingBuffer::Clear()
{
	ATOMIC_CLR(m_pointers.val);
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
