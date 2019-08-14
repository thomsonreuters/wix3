// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

// private structs

// Similar to CPI_ASSEMBLY_ATTRIBUTES
struct CPI_IMPORTED_COMPONENT_ATTRIBUTES
{
    int iActionType;
    int iActionCost;
    LPWSTR pwzKey;
    WCHAR wzClsId[CPI_MAX_GUID + 1];
    LPWSTR pwzAppID;
    LPWSTR pwzPartID;
    // Attributes?
};

// prototypes for private helper functions

static HRESULT ReadImportedComponentAttributes(
    __deref_in LPWSTR* ppwzData,
    __out CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    );
static void FreeImportedComponentAttributes(
    __in CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    );
static HRESULT RegisterImportedComponent(
    __in CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    );
static HRESULT UnregisterImportedComponent(
    __in CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    );

// function definitions

HRESULT CpiConfigureImportedComponents(
    __deref_in LPWSTR* ppwzData,
    __in HANDLE hRollbackFile
    )
{
    HRESULT hr = E_NOTIMPL;

    CPI_IMPORTED_COMPONENT_ATTRIBUTES attrs;
    ::ZeroMemory(&attrs, sizeof(attrs));

    // initialize
    // TODO: Write a function that initializes globals for tracking.

    // read action text
    hr = CpiActionStartMessage(ppwzData, FALSE);
    ExitOnFailure(hr, "Failed to send action start message");

    // get count
    int iCnt = 0;
    hr = WcaReadIntegerFromCaData(ppwzData, &iCnt);
    ExitOnFailure(hr, "Failed to read count");

    // write count to rollback file
    hr = CpiWriteIntegerToRollbackFile(hRollbackFile, iCnt);
    ExitOnFailure(hr, "Failed to write count to rollback file");

    for (int i = 0; i < iCnt; ++i)
    {
        // read attributes from CustomActionData
        hr = ReadImportedComponentAttributes(ppwzData, &attrs);
        ExitOnFailure(hr, "Failed to read imported component attributes");

        // write key to rollback file
        hr = CpiWriteKeyToRollbackFile(hRollbackFile, attrs.pwzKey);
        ExitOnFailure(hr, "Failed to write key to rollback file");

        // action
        switch (attrs.iActionType)
        {
        case atCreate:
            hr = RegisterImportedComponent(&attrs);
            ExitOnFailure1(hr, "Failed to register imorted comonent, key: %S", attrs.pwzKey);
            break;
        case atRemove:
            hr = UnregisterImportedComponent(&attrs);
            ExitOnFailure1(hr, "Failed to unregister imported component, key: %S", attrs.pwzKey);
            break;
        default:
            hr = S_OK;
            break;
        }

        if (S_FALSE == hr)
        {
            ExitFunction(); // aborted by user
        }
    }

LExit:
    // clean up
    FreeImportedComponentAttributes(&attrs);

    // TODO: Uninitialize?

    return hr;
}

static HRESULT ReadImportedComponentAttributes(
    __deref_in LPWSTR* ppwzData,
    __out CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    )
{
    HRESULT hr = S_OK;

    LPWSTR pwzData = NULL;

    // read attributes
    hr = WcaReadIntegerFromCaData(ppwzData, &pAttrs->iActionType);
    ExitOnFailure(hr, "Failed to read action type");
    hr = WcaReadIntegerFromCaData(ppwzData, &pAttrs->iActionCost);
    ExitOnFailure(hr, "Failed to read action cost");
    hr = WcaReadStringFromCaData(ppwzData, &pAttrs->pwzKey);
    ExitOnFailure(hr, "Failed to read key");
    // Read clsid
    hr = WcaReadStringFromCaData(ppwzData, &pwzData);
    ExitOnFailure(hr, "Failed to read clsid");
    hr = StringCchCopyW(pAttrs->wzClsId, countof(pAttrs->wzClsId), pwzData);
    ExitOnFailure(hr, "Failed to copy clsid");
    hr = WcaReadStringFromCaData(ppwzData, &pAttrs->pwzAppID);
    ExitOnFailure(hr, "Failed to read application id");
    hr = WcaReadStringFromCaData(ppwzData, &pAttrs->pwzPartID);
    ExitOnFailure(hr, "Failed to read partition id");

    hr = S_OK;

LExit:
    // clean up
    ReleaseStr(pwzData);

    return hr;
}

static void FreeImportedComponentAttributes(
    __in CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    )
{
    ReleaseStr(pAttrs->pwzKey);
    ReleaseStr(pAttrs->pwzAppID);
    ReleaseStr(pAttrs->pwzPartID);
}

// Lots of this code was inpired by RegisterNativeAssembly() in cpasmexec.cpp
static HRESULT RegisterImportedComponent(
    __in CPI_IMPORTED_COMPONENT_ATTRIBUTES* pAttrs
    )
{
    HRESULT hr = S_OK;

    ICOMAdminCatalog* piCatalog = NULL;
    ICOMAdminCatalog2* piCatalog2 = NULL;
    BSTR bstrGlobPartID = NULL;

    BSTR bstrPartID = NULL;
    BSTR bstrAppID = NULL;
    BSTR bstrClsID = NULL;

    // progress message
    hr = CpiActionDataMessage(1, pAttrs->wzClsId);
    ExitOnFailure(hr, "Failed to send progress messages");

    if (S_FALSE == hr)
    {
        ExitFunction(); // aborted by user
    }

    // log
    WcaLog(LOGMSG_VERBOSE, "Registering imported component, key: %S", pAttrs->pwzKey);

    // Unlike assemblies, there is no differences in native or managed.

    // Create BSTRs for parameters
    if (pAttrs->pwzPartID && *pAttrs->pwzPartID)
    {
        bstrPartID = ::SysAllocString(pAttrs->pwzPartID);
        ExitOnNull(bstrPartID, hr, E_OUTOFMEMORY, "Failed to allocate BSTR for partition id");
    }

    bstrAppID = ::SysAllocString(pAttrs->pwzAppID);
    ExitOnNull(bstrAppID, hr, E_OUTOFMEMORY, "Failed to allocate BSTR for application id");

    bstrClsID = ::SysAllocString(pAttrs->wzClsId);
    ExitOnNull(bstrClsID, hr, E_OUTOFMEMORY, "Failed to allocate BSTR for class id");

    // get catalog
    hr = CpiGetAdminCatalog(&piCatalog);
    ExitOnFailure(hr, "Failed to get COM+ admin catalog");

    // get ICOMAdminCatalog2 interface
    hr = piCatalog->QueryInterface(IID_ICOMAdminCatalog2, (void**)&piCatalog2);

    // COM+ 1.5 or later
    if (E_NOINTERFACE != hr)
    {
        ExitOnFailure(hr, "Failed to get IID_ICOMAdminCatalog2 interface");

        // partition id
        if (!bstrPartID)
        {
            // get global partition id
            hr = piCatalog2->get_GlobalPartitionID(&bstrGlobPartID);
            ExitOnFailure(hr, "Failed to get global partition id");
        }

        // set current partition
        hr = piCatalog2->put_CurrentPartition(bstrPartID ? bstrPartID : bstrGlobPartID);
        ExitOnFailure(hr, "Failed to set current partition");
    }

    // COM+ pre 1.5
    else
    {
        // this version of COM+ does not support partitions, make sure a partition was not specified
        if (bstrPartID)
        {
            ExitOnFailure(hr = E_FAIL, "Partitions are not supported by this version of COM+");
        }
    }

    hr = piCatalog->ImportComponent(bstrAppID, bstrClsID);
    if (COMADMIN_E_OBJECTERRORS == hr)
    {
        CpiLogCatalogErrorInfo();
    }

    ExitOnFailure(hr, "Failed to import components");

    hr = S_OK;

LExit:
    // clean up
    ReleaseObject(piCatalog);
    ReleaseObject(piCatalog2);
    ReleaseBSTR(bstrGlobPartID);

    ReleaseBSTR(bstrPartID);
    ReleaseBSTR(bstrAppID);
    ReleaseBSTR(bstrClsID);

    return hr;
}
