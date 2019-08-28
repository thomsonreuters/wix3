#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

enum eImportedComponentAttributes
{
    icaRunInCommit = (1 << 0),
};

// structs

struct CPI_IMPORTEDCOMPONENT
{
    WCHAR wzKey[MAX_DARWIN_KEY + 1];
    // Given that Imported Components require that someone else registered,
    // I don't think a Module makes sense here.
    WCHAR wzCLSID[CPI_MAX_GUID + 1];

    // Does it require deferred-to-commit-stage processing?
    int iAttributes;

    // No children here, like Assembly had Components

    // Referenced flags are tricky. Later if ever.

    // Not doing Role Assignments at this time.

    INSTALLSTATE isInstalled, isAction;

    CPI_APPLICATION* pApplication;

    CPI_IMPORTEDCOMPONENT* pPrev;
    CPI_IMPORTEDCOMPONENT* pNext;
};


struct CPI_IMPORTEDCOMPONENT_LIST
{
    CPI_IMPORTEDCOMPONENT* pFirst;
    CPI_IMPORTEDCOMPONENT* pLast;

    int iInstallCount;
    int iCommitCount;
    int iUninstallCount;

    // No roles here.
};

// function prototypes
void CpiImportedComponentListFree(
    __in CPI_IMPORTEDCOMPONENT_LIST* pList
    );
HRESULT CpiImportedComponentsRead(
    __in CPI_APPLICATION_LIST* pAppList,
    __inout CPI_IMPORTEDCOMPONENT_LIST* pImpCompList
    );
// What's the purpose of a VerifyInstall?
// What's the purpose of a VerifyUninstall?
HRESULT CpiImportedComponentsInstall(
    __in CPI_IMPORTEDCOMPONENT_LIST* pList,
    int iRunMode,
    __deref_out_z_opt LPWSTR* ppwzActionData,
    __inout int* piProgress
    );
HRESULT CpiImportedComponentsUninstall(
    __in CPI_IMPORTEDCOMPONENT_LIST* pList,
    int iRunMode,
    __deref_out_z_opt LPWSTR* ppwzActionData,
    __inout int* piProgress
    );