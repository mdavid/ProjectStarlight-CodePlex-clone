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

#include "domlogger.h"

void DOMElementLogger::LogTrace(const char * data)
{
	m_invoker->Invoke(m_navigator, &DOMElementLogger::DoRealLog, (void*)new DOMElementLoggerCallbackArgs(data, "TRACE: ", m_navigator));
}

void DOMElementLogger::LogError(const char * data)
{
	m_invoker->Invoke(m_navigator, &DOMElementLogger::DoRealLog, (void*)new DOMElementLoggerCallbackArgs(data, "ERROR: ", m_navigator));
}

void DOMElementLogger::DoRealLog(void* vargs)
{
	DOMElementLoggerCallbackArgs* args = (DOMElementLoggerCallbackArgs*)vargs;
	
	NPObject* window;
	NPVariant document;
	NPVariant docElementId;
	STRINGZ_TO_NPVARIANT(LOG_DOM_ELEMENT, docElementId);
	NPVariant logElement;
	
	if(NPN_GetValue(args->m_navigator, NPNVWindowNPObject, &window) == NPERR_NO_ERROR)
	{
		NPIdentifier documentPropId = NPN_GetStringIdentifier("document");
		bool rc = NPN_GetProperty(args->m_navigator, window, documentPropId, &document);
		if(rc && NPVARIANT_IS_OBJECT(document))
		{
			NPIdentifier getElementMethodId = NPN_GetStringIdentifier("getElementById");
			rc = NPN_Invoke(args->m_navigator, NPVARIANT_TO_OBJECT(document), getElementMethodId, &docElementId, 1, &logElement);
			if(rc && NPVARIANT_IS_OBJECT(logElement))
			{
				//Get the existing inner HTML
				NPVariant innerHTMLVariant;
				NPIdentifier innerHTMLPropId = NPN_GetStringIdentifier("innerHTML");
				rc = NPN_GetProperty(args->m_navigator, NPVARIANT_TO_OBJECT(logElement), innerHTMLPropId, &innerHTMLVariant);
				
				//Write the log message into a buffer
				char buffer[JS_BUF_SZ];
				memset(buffer, 0, JS_BUF_SZ);
				snprintf(buffer, JS_BUF_SZ - 1, "%s: %s <BR>", args->m_type, args->m_message);
				int szMessage = strlen(buffer);
				
				//Convert the old value to a NPString
				NPString innerHTMLString;
				if(rc && NPVARIANT_IS_STRING(innerHTMLVariant))
				{
					innerHTMLString = NPVARIANT_TO_STRING(innerHTMLVariant);
				}
				else
				{
					NPSTR_LEN(innerHTMLString) = 0;
				}
				
				//Create a new string with the old data + the new data
				char* newInnerHTML = (char*) NPN_MemAlloc(szMessage + NPSTR_LEN(innerHTMLString));
				memcpy(newInnerHTML, NPSTR_CHARS(innerHTMLString), NPSTR_LEN(innerHTMLString));
				memcpy(newInnerHTML + NPSTR_LEN(innerHTMLString), buffer, szMessage);
				
				//Create a variant and set the property to it
				NPVariant newInnerHTMLVariant;
				STRINGN_TO_NPVARIANT(newInnerHTML, szMessage + NPSTR_LEN(innerHTMLString), newInnerHTMLVariant);
				NPN_SetProperty(args->m_navigator, NPVARIANT_TO_OBJECT(logElement), innerHTMLPropId, &newInnerHTMLVariant);
				
				//Release the old value
				NPN_ReleaseVariantValue(&innerHTMLVariant);
				NPN_ReleaseVariantValue(&logElement);
			}
			NPN_ReleaseVariantValue(&document);
		}
		NPN_ReleaseObject(window);
	}
	delete args;
}
