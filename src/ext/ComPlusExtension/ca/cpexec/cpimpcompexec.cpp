// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.

#include "precomp.h"

// function definitions

HRESULT CpiConfigureImportedComponents(
    __deref_in LPWSTR* ppwzData,
    __in HANDLE hRollbackFile
    )
{
    HRESULT hr = E_NOTIMPL;

    LPWSTR pwzData = NULL;

    WCHAR wzCLSID[CPI_MAX_GUID + 1];
    ::ZeroMemory(wzCLSID, sizeof(wzCLSID));

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
        // Read clsid
        hr = WcaReadStringFromCaData(ppwzData, &pwzData);
        ExitOnFailure(hr, "Failed to read clsid");
        StringCchCopyW(wzCLSID, countof(wzCLSID), pwzData);

        // Write Guid? to rollback file
        hr = CpiWriteKeyToRollbackFile(hRollbackFile, wzCLSID);
        ExitOnFailure(hr, "Failed to write key/clsid to rollback file");

        // action

        switch (iActionType)
        {
#error pick up here
        }
    }

LExit:
    // clean up
    ReleaseStr(pwzData);

    return hr;
}
