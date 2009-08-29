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

#ifndef INC_MCASTLIBCOMMON_H
#define INC_MCASTLIBCOMMON_H

#ifdef PLAT_WIN
	#ifndef _WIN32_WINNT		                  
	#define _WIN32_WINNT 0x0501	
	#endif						

	#define WIN32_LEAN_AND_MEAN	

	#include <stdlib.h>
	#include <string.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
	#include <Windows.h>

	typedef SOCKET socket_t;
	typedef SOCKADDR_IN sockaddr_in_t;
	typedef HANDLE thread_handle_t;
	typedef int int32_t;

	#define THREAD_DECL DWORD WINAPI
	#define LAST_SOCK_ERROR WSAGetLastError()
	#define SOCK_CLOSE(S) closesocket(S)
	#define ETIMEDOUT WSAETIMEDOUT
	#define ENOBUFS WSAENOBUFS
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define snprintf _snprintf_s
	#define strdup _strdup 

	#define SLEEP_MILLI(millis) Sleep(millis) 
#endif /* PLAT_WIN */

#ifdef PLAT_MAC
	#include <pthread.h>
	#include <memory.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <stdio.h>
	#include <sys/time.h>
	#include <errno.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <libkern/OSAtomic.h>

	typedef int socket_t;
	typedef sockaddr_in sockaddr_in_t;
	typedef pthread_t thread_handle_t;

	#define THREAD_DECL void*
	#define LAST_SOCK_ERROR errno
	#define SOCK_CLOSE(S) close(S)

	#define SLEEP_MILLI(millis) usleep(millis * 1000) 
#endif /* PLAT_MAC */

#endif /* INC_MCASTLIBCOMMON_H */