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

#include "invoker.h"


#ifdef XP_WIN
/*
 * Windows Implementation:
 * Firefox only is supported, so we rely on NPN_PluginThreadAsyncCall to do our dirty work.
 */

struct InvocationData 
{
	void (*f)(void*);
	void* a;
	HANDLE s;
};

Invoker::Invoker() : m_stopped(0) {}

Invoker::~Invoker() {}

void Invoker::Invoke(NPP plugin, void (*func)(void*), void* args)
{
	if(m_stopped)
	{
		return;
	}
	NPN_PluginThreadAsyncCall(plugin, func, args);
}

void Invoker::Stop()
{
	this->m_stopped = 1;
}

#endif

#ifdef XP_MACOSX
/*
 * Mac implementation:
 * We support Firefox and Safari, and Safari does not yet implement NPN_PluginThreadAsyncCall,
 * so we use Mach messages and a run loop on the main thread to accomplish the main thread 
 * invocations.
 */
struct MachMessage 
{
	mach_msg_header_t h;
	void (*f)(void*);
	void* a;
	sem_t* s;
};

void OnPortMessage(CFMachPortRef port, void* msg, CFIndex size, void* info)
{
	MachMessage* m = (MachMessage*)msg;
	sem_post(m->s);
	m->f(m->a);  
}

Invoker::Invoker() : m_stopped(0)
{
	
	CFMachPortContext context;
	context.version = 0;
	context.info = this;
	context.retain = NULL;
	context.release = NULL;
	context.copyDescription = NULL;
	m_machPort = CFMachPortCreate(kCFAllocatorDefault, OnPortMessage, &context, false);
	m_runLoop = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, m_machPort, 0);
	m_mainLoop = CFRunLoopGetCurrent();
	CFRunLoopAddSource(m_mainLoop, m_runLoop, kCFRunLoopCommonModes);
	sem_init(&this->m_sem, 0, 1);
	pthread_mutex_init(&this->m_mutex, NULL);
}

Invoker::~Invoker() 
{
	CFRunLoopRemoveSource(m_mainLoop, m_runLoop, kCFRunLoopCommonModes);
	CFRelease(m_runLoop);
	CFRelease(m_machPort);
	sem_destroy(&this->m_sem);
	pthread_mutex_destroy(&this->m_mutex);
}

void Invoker::Invoke(NPP plugin, void (*func)(void*), void* args)
{
	pthread_mutex_lock(&this->m_mutex);
	MachMessage m = {0};
	if(m_stopped)
	{
		goto unlockAndReturn;
	}
	if(CFEqual(m_mainLoop, CFRunLoopGetCurrent()))
	{
		func(args);
		goto unlockAndReturn;
	}
	
	
	m.h.msgh_size = sizeof(mach_msg_header_t);
	m.h.msgh_remote_port = CFMachPortGetPort(m_machPort);
	m.h.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND,
								   MACH_MSG_TYPE_MAKE_SEND_ONCE);
	m.f = func;
	m.a = args;
	m.s = &this->m_sem;
	mach_msg(&m.h, MACH_SEND_MSG, sizeof(MachMessage), 0, 0, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	sem_wait(&this->m_sem);
	
unlockAndReturn:
	pthread_mutex_unlock(&this->m_mutex);
}

void Invoker::Stop()
{
	pthread_mutex_lock(&this->m_mutex);
	OSAtomicIncrement32Barrier(&this->m_stopped);
	sem_wait(&this->m_sem);
	pthread_mutex_unlock(&this->m_mutex);
}


#endif


