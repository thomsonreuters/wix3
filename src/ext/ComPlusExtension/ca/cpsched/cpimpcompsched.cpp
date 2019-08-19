// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"


// sql queries

// private structs

// property definitions

// prototypes for private helper functions

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
#error resume coding here.
    }

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

