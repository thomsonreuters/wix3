// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// sql queries

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
        // Process only items that need action during install

        // Assemblies had a check with WcaIsInstalling() Not sure if that deserves replication.
        // I think it might, because the Application seems to have the ability to be in a
        // different component from the assembly and this could get really really complex.
        // Check out the code in AssembliesRead().

        // The purpose of aaRunInCommit's is explained in the manual
        // for the ComPlusAssembly element. Not repeating it here yet.

        //action type
        if (rmRollback == iRunMode)
        {
            // Bizarro check on ItemState to do here
            iActionType = atRemove;
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

