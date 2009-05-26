

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Tue May 26 10:36:39 2009
 */
/* Compiler settings for _MulticastProxyActiveX.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef ___MulticastProxyActiveX_h__
#define ___MulticastProxyActiveX_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMulticastProxy_FWD_DEFINED__
#define __IMulticastProxy_FWD_DEFINED__
typedef interface IMulticastProxy IMulticastProxy;
#endif 	/* __IMulticastProxy_FWD_DEFINED__ */


#ifndef __CMulticastProxy_FWD_DEFINED__
#define __CMulticastProxy_FWD_DEFINED__

#ifdef __cplusplus
typedef class CMulticastProxy CMulticastProxy;
#else
typedef struct CMulticastProxy CMulticastProxy;
#endif /* __cplusplus */

#endif 	/* __CMulticastProxy_FWD_DEFINED__ */


/* header files for imported files */
#include "prsht.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "exdisp.h"
#include "objsafe.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IMulticastProxy_INTERFACE_DEFINED__
#define __IMulticastProxy_INTERFACE_DEFINED__

/* interface IMulticastProxy */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMulticastProxy;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FD7B677B-6749-4574-93E9-EE2CF1458D50")
    IMulticastProxy : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartStreaming( 
            /* [in] */ BSTR multicastGroup,
            /* [in] */ USHORT multicastPort,
            /* [in] */ BSTR multicastSource,
            /* [in] */ IDispatch *target) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopStreaming( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Test( 
            /* [in] */ BSTR message) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FetchNSC( 
            /* [in] */ BSTR nscFileUrl,
            /* [retval][out] */ BSTR *nscContent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMulticastProxyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMulticastProxy * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMulticastProxy * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMulticastProxy * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMulticastProxy * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMulticastProxy * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMulticastProxy * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMulticastProxy * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StartStreaming )( 
            IMulticastProxy * This,
            /* [in] */ BSTR multicastGroup,
            /* [in] */ USHORT multicastPort,
            /* [in] */ BSTR multicastSource,
            /* [in] */ IDispatch *target);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *StopStreaming )( 
            IMulticastProxy * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Test )( 
            IMulticastProxy * This,
            /* [in] */ BSTR message);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *FetchNSC )( 
            IMulticastProxy * This,
            /* [in] */ BSTR nscFileUrl,
            /* [retval][out] */ BSTR *nscContent);
        
        END_INTERFACE
    } IMulticastProxyVtbl;

    interface IMulticastProxy
    {
        CONST_VTBL struct IMulticastProxyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMulticastProxy_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMulticastProxy_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMulticastProxy_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMulticastProxy_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMulticastProxy_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMulticastProxy_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMulticastProxy_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMulticastProxy_StartStreaming(This,multicastGroup,multicastPort,multicastSource,target)	\
    (This)->lpVtbl -> StartStreaming(This,multicastGroup,multicastPort,multicastSource,target)

#define IMulticastProxy_StopStreaming(This)	\
    (This)->lpVtbl -> StopStreaming(This)

#define IMulticastProxy_Test(This,message)	\
    (This)->lpVtbl -> Test(This,message)

#define IMulticastProxy_FetchNSC(This,nscFileUrl,nscContent)	\
    (This)->lpVtbl -> FetchNSC(This,nscFileUrl,nscContent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMulticastProxy_StartStreaming_Proxy( 
    IMulticastProxy * This,
    /* [in] */ BSTR multicastGroup,
    /* [in] */ USHORT multicastPort,
    /* [in] */ BSTR multicastSource,
    /* [in] */ IDispatch *target);


void __RPC_STUB IMulticastProxy_StartStreaming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMulticastProxy_StopStreaming_Proxy( 
    IMulticastProxy * This);


void __RPC_STUB IMulticastProxy_StopStreaming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMulticastProxy_Test_Proxy( 
    IMulticastProxy * This,
    /* [in] */ BSTR message);


void __RPC_STUB IMulticastProxy_Test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMulticastProxy_FetchNSC_Proxy( 
    IMulticastProxy * This,
    /* [in] */ BSTR nscFileUrl,
    /* [retval][out] */ BSTR *nscContent);


void __RPC_STUB IMulticastProxy_FetchNSC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMulticastProxy_INTERFACE_DEFINED__ */



#ifndef __MulticastProxyActiveX_LIBRARY_DEFINED__
#define __MulticastProxyActiveX_LIBRARY_DEFINED__

/* library MulticastProxyActiveX */
/* [helpstring][uuid][version] */ 


EXTERN_C const IID LIBID_MulticastProxyActiveX;

EXTERN_C const CLSID CLSID_CMulticastProxy;

#ifdef __cplusplus

class DECLSPEC_UUID("5CCEF05B-17B2-48F0-A577-CC019FE455D7")
CMulticastProxy;
#endif
#endif /* __MulticastProxyActiveX_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


