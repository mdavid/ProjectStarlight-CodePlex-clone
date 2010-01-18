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

#include "starlightscriptable.h"



StarlightScriptable::StarlightScriptable(NPP npp) : ScriptablePluginObjectBase(npp)
{
	m_plugin = NULL;
	START_STREAMING_METHOD = NPN_GetStringIdentifier("StartStreaming");
	STOP_STREAMING_METHOD = NPN_GetStringIdentifier("StopStreaming");
	FETCH_NSC_METHOD = NPN_GetStringIdentifier("FetchNSC");
	TEST_METHOD = NPN_GetStringIdentifier("Test");
}

StarlightScriptable::~StarlightScriptable()
{
}

bool StarlightScriptable::HasMethod(NPIdentifier name)
{
	if(name == START_STREAMING_METHOD
	   || name == STOP_STREAMING_METHOD
	   || name == FETCH_NSC_METHOD
	   || name == TEST_METHOD)
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

bool StarlightScriptable::Invoke(NPIdentifier name, const NPVariant* args,
							uint32_t argCount, NPVariant *result)
{
	if (m_plugin == NULL)
	{
		return false;
	}
	
	if (name == START_STREAMING_METHOD) 
	{
		if(argCount != 4)
		{
			return false;
		}
		if(!NPVARIANT_IS_STRING(args[0]))
		{
			return false;
		}

		if(!(NPVARIANT_IS_INT32(args[1]) || NPVARIANT_IS_DOUBLE(args[1])))
		{
			return false;
		}

		if(!(NPVARIANT_IS_STRING(args[2]) || NPVARIANT_IS_NULL(args[2])))
		{
			return false;
		}

		if(!NPVARIANT_IS_OBJECT(args[3])) 
		{
			return false;
		}
		NPString multicastGroup = NPVARIANT_TO_STRING(args[0]);
		int32_t port;
		if(NPVARIANT_IS_INT32(args[1]))
		{
			port = NPVARIANT_TO_INT32(args[1]);
		}
		else
		{
			port = (int)NPVARIANT_TO_DOUBLE(args[1]);
		}
		NPString multicastSource;
		if(NPVARIANT_IS_STRING(args[2])) 
		{
			multicastSource = NPVARIANT_TO_STRING(args[2]);
		}
		else 
		{
			#ifdef XP_WIN
			multicastSource.utf8length = 0;
			#endif
			#ifdef XP_MACOSX
			multicastSource.UTF8Length = 0;
			#endif
			
		}
		NPObject* target = NPVARIANT_TO_OBJECT(args[3]);
		return m_plugin->StartStreaming(multicastGroup, port, multicastSource, target, result);
	}
	else if(name == STOP_STREAMING_METHOD)
	{
		return m_plugin->StopStreaming(result);
	}
	else if(name == TEST_METHOD)
	{
		if(argCount != 1)
		{
			return false;
		}
		NPString message = NPVARIANT_TO_STRING(args[0]);
		return m_plugin->Test(message, result);
	}
	else if(name == FETCH_NSC_METHOD)
	{
		if(argCount != 1)
		{
			return false;
		}
		NPString url = NPVARIANT_TO_STRING(args[0]);
		return m_plugin->FetchNSC(url, result);
	}
	else
	{
		return false;
	}
}

void StarlightScriptable::Attach(StarlightPlugin* plugin)
{
	m_plugin = plugin;
}

void StarlightScriptable::Detach() 
{
	m_plugin = NULL;
}

