// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// sql queries

LPCWSTR vcsImportedComponentQuery =
    L"SELECT `ImportedComponent`, `Component_`, `Application_`, `CLSID` FROM `ComPlusImportedComponent`";
enum eImportedComponentQuery { icqImportedComponent = 1, icqComponent, icqApplication, icqCLSID };

// private structs

// property definitions

// prototypes for private helper functions

static HRESULT AddImportedComponentToActionData(
    __in CPI_IMPORTEDCOMPONENT* pItm,
    BOOL fInstall,
    int iActionType,
    int iActionCost,
    __deref_out_z_opt LPWSTR* ppwzActionData
    );
static void ImportedComponentFree(
    __in CPI_IMPORTEDCOMPONENT* pItm
    );
static HRESULT ImportedComponentsRead(
    __in CPI_APPLICATION_LIST* pAppList,
    __inout CPI_IMPORTEDCOMPONENT_LIST* pImpCompList
    );
// function definitions

void CpiImportedComponentListFree(
    __in CPI_IMPORTEDCOMPONENT_LIST* pList
    )
{
    CPI_IMPORTEDCOMPONENT* pItm = pList->pFirst;

    while (pItm)
    {
        CPI_IMPORTEDCOMPONENT* pDelete = pItm;
        pItm = pItm->pNext;
        ImportedComponentFree(pDelete);
    }
}

HRESULT CpiImportedComponentsRead(
    __in CPI_APPLICATION_LIST* pAppList,
    __inout CPI_IMPORTEDCOMPONENT_LIST* pImpCompList
    )
{
    HRESULT hr = S_OK;

    hr = ImportedComponentsRead(pAppList, pImpCompList);
    ExitOnFailure(hr, "Failed to read ComPlusImportedComponent table");

    // Any sorting?

    hr = S_OK;

LExit:
    return hr;
}

HRESULT CpiImportedComponentsInstall(
    __in CPI_IMPORTEDCOMPONENT_LIST* pList,
    int iRunMode,
    __deref_inout_z LPWSTR* ppwzActionData,
    __inout_opt int* piProgress
    )
{
    HRESULT hr = S_OK;

    int iActionType = 0;
    int iCount = 0;

    // add action text
    hr = CpiAddActionTextToActionData(L"RegisterComPlusImportedComponents", ppwzActionData);
    ExitOnFailure(hr, "Failed to add action text to custom action data");

    // imported component count
    switch (iRunMode)
    {
    case rmDeferred:
        iCount = pList->iInstallCount - pList->iCommitCount;
        break;
    case rmCommit:
        iCount = pList->iCommitCount;
        break;
    case rmRollback:
        iCount = pList->iInstallCount;
        break;
    }

    // add imported component count to action data
    hr = WcaWriteIntegerToCaData(iCount, ppwzActionData);
    ExitOnFailure(hr, "Failed to add count to custom action data");

    // add imported components to custom action data in forward order
    for (CPI_IMPORTEDCOMPONENT* pItm = pList->pFirst; pItm; pItm = pItm->pNext)
    {
        if (!WcaIsInstalling(pItm->isInstalled, pItm->isAction))
        {
            continue;
        }

        // The purpose of aaRunInCommit's is explained in the manual
        // for the ComPlusAssembly element. Not repeating it here yet.

        //action type
        if (rmRollback == iRunMode)
        {
            if (CpiIsInstalled(pItm->isInstalled))
            {
                iActionType = atNoOp;
            }
            else
            {
                iActionType = atRemove;
            }
        }
        else
        {
            iActionType = atCreate;
        }

        hr = AddImportedComponentToActionData(pItm, TRUE, iActionType, COST_IMPORTED_COMPONENT_CREATE, ppwzActionData);
        ExitOnFailure1(hr, "Failed to add imported component to custom action data, key: %S", pItm->wzKey);
    }

    // Add progress tics
    if (piProgress)
    {
        *piProgress += COST_IMPORTED_COMPONENT_CREATE * iCount;
    }

    hr = S_OK;

LExit:
    return hr;
}

HRESULT CpiImportedComponentsUninstall(
    __in CPI_IMPORTEDCOMPONENT_LIST* /*pList*/,
    int /*iRunMode*/,
    __deref_out_z_opt LPWSTR* /*ppwzActionData*/,
    __inout int* /*piProgress*/
    )
{
    HRESULT hr = S_OK;

    // TODO: Make this actually work, but for now...
    hr = E_NOTIMPL;

//LExit:
    return hr;
}

static HRESULT ImportedComponentsRead(
    __in CPI_APPLICATION_LIST* pAppList,
    __inout CPI_IMPORTEDCOMPONENT_LIST* pImpCompList
    )
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;

    PMSIHANDLE hView, hRec;

    CPI_IMPORTEDCOMPONENT* pItm = NULL;
    LPWSTR pwzData = NULL;
    LPWSTR pwzComponent = NULL;

    // loop through all imported components
    hr = WcaOpenExecuteView(vcsImportedComponentQuery, &hView);
    ExitOnFailure(hr, "Failed to execute view on ComPlusImportedComponent table");

    while (S_OK == (hr = WcaFetchRecord(hView, &hRec)))
    {
        // get component
        hr = WcaGetRecordString(hRec, icqComponent, &pwzComponent);
        ExitOnFailure(hr, "Failed to get component");

        // Think long and hard before validating Architecture

        // create entry
        pItm = (CPI_IMPORTEDCOMPONENT*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CPI_IMPORTEDCOMPONENT));
        if (!pItm)
        {
            ExitFunction1(hr = E_OUTOFMEMORY);
        }

        // get component install state
        er = ::MsiGetComponentStateW(WcaGetInstallHandle(), pwzComponent, &pItm->isInstalled, &pItm->isAction);
        ExitOnFailure(hr = HRESULT_FROM_WIN32(er), "Failed to get component state");

        // get key
        hr = WcaGetRecordString(hRec, icqImportedComponent, &pwzData);
        ExitOnFailure(hr, "Failed to get key");
        StringCchCopy(pItm->wzKey, countof(pItm->wzKey), pwzData);

        // no attributes

        // get imported component clsid
        hr = WcaGetRecordFormattedString(hRec, icqCLSID, &pwzData);
        ExitOnFailure(hr, "Failed to get imported component CLSID");
        StringCchCopy(pItm->wzCLSID, countof(pItm->wzCLSID), pwzData);

        // I don't think the alternate way to get an assembly name needs an equivalent

        // get application
        hr = WcaGetRecordString(hRec, icqApplication, &pwzData);
        ExitOnFailure(hr, "Failed to get application");

        if (pwzData && *pwzData)
        {
            hr = CpiApplicationFindByKey(pAppList, pwzData, &pItm->pApplication);
            if (S_FALSE == hr)
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            }
            ExitOnFailure1(hr, "Failed to find application, key: %S", pwzData);
        }

        // No paths

        // No sub-parts

        if (WcaIsInstalling(pItm->isInstalled, pItm->isAction))
        {
            ++pImpCompList->iInstallCount;
            // TODO: If/when runInCommit, check and increment.
        }
        if (WcaIsUninstalling(pItm->isInstalled, pItm->isAction) ||
            WcaIsReInstalling(pItm->isInstalled, pItm->isAction))
        {
            ++pImpCompList->iUninstallCount;
        }
        // No role assignments

        // I'm not sure why we're twiddling the application reference count,
        // but Assemblies did so.
        if (pItm->pApplication)
        {
            if (WcaIsInstalling(pItm->isInstalled, pItm->isAction))
            {
                CpiApplicationAddReferenceInstall(pItm->pApplication);
            }
            if (WcaIsUninstalling(pItm->isInstalled, pItm->isAction) ||
                WcaIsReInstalling(pItm->isInstalled, pItm->isAction))
            {
                CpiApplicationAddReferenceUninstall(pItm->pApplication);
            }
        }

        // add entry
        if (pImpCompList->pLast)
        {
            pImpCompList->pLast->pNext = pItm;
            pItm->pPrev = pImpCompList->pLast;
        }
        else
        {
            pImpCompList->pFirst = pItm;
        }
        pImpCompList->pLast = pItm;
        pItm = NULL;
    }

    if (E_NOMOREITEMS == hr)
    {
        hr = S_OK;
    }

LExit:
    // clean up
    if (pItm)
    {
        ImportedComponentFree(pItm);
    }

    ReleaseStr(pwzData);
    ReleaseStr(pwzComponent);

    return hr;
}

static HRESULT AddImportedComponentToActionData(
    __in CPI_IMPORTEDCOMPONENT* pItm,
    BOOL /*fInstall*/,
    int iActionType,
    int iActionCost,
    __deref_out_z_opt LPWSTR* ppwzActionData
    )
{
    HRESULT hr = S_OK;

    // add action information to custom action data
    hr = WcaWriteIntegerToCaData(iActionType, ppwzActionData);
    ExitOnFailure(hr, "Failed to add action type to custom action data");
    hr = WcaWriteIntegerToCaData(iActionCost, ppwzActionData);
    ExitOnFailure(hr, "Failed to add action cost to custom action data");

    // add imported component information to custom action data
    hr = WcaWriteStringToCaData(pItm->wzKey, ppwzActionData);
    ExitOnFailure(hr, "Failed to add imported component key to custom action data");
    hr = WcaWriteStringToCaData(pItm->wzCLSID, ppwzActionData);
    ExitOnFailure(hr, "Failed to add imported component clsid to custom action data");

    // add application information to custom action data
    hr = WcaWriteStringToCaData(pItm->pApplication ? pItm->pApplication->wzID : L"", ppwzActionData);
    ExitOnFailure(hr, "Failed to add application id to custom action data");

    // add partition information to custom action data
    LPCWSTR pwzPartID = pItm->pApplication && pItm->pApplication->pPartition ? pItm->pApplication->pPartition->wzID : L"";
    hr = WcaWriteStringToCaData(pwzPartID, ppwzActionData);
    ExitOnFailure(hr, "Failed to add partition id to custom action data");

    // Assemblies have sub components. We don't.

    hr = S_OK;

LExit:
    return hr;
}

static void ImportedComponentFree(
    __in CPI_IMPORTEDCOMPONENT* pItm
    )
{
    // We kept it fairly clean!

    // The Application is not owned by the Imported Component.

    ::HeapFree(::GetProcessHeap(), 0, pItm);
}

