/*****************************************************************************/
/* Page10Links.cpp                        Copyright (c) Ladislav Zezula 2009 */
/*---------------------------------------------------------------------------*/
/* Description:                                                              */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 19.10.09  1.00  Lad  The first version of Page10Links.cpp                 */
/*****************************************************************************/

#include "FileTest.h"
#include "ReparseDataHsm.h"
#include "resource.h"

//-----------------------------------------------------------------------------
// Local defines

#define MAX_TREE_ITEM_LENGTH        0x400
#define REPARSE_HEADER_LENGTH       0x08        // sizeof(REPARSE_DATA_BUFFER::ReparseTag, ReparseDataLength, Reserved)

#define ITEM_TYPE_REPARSE_TAG       0x0001
#define ITEM_TYPE_SUBSTNAME_MP      0x0002
#define ITEM_TYPE_PRINTNAME_MP      0x0003
#define ITEM_TYPE_SUBSTNAME_LNK     0x0004
#define ITEM_TYPE_PRINTNAME_LNK     0x0005

static TFlagInfo ReparseTags[] =
{
    FLAGINFO_NUMV(IO_REPARSE_TAG_MOUNT_POINT),
    FLAGINFO_NUMV(IO_REPARSE_TAG_HSM),
    FLAGINFO_NUMV(IO_REPARSE_TAG_DRIVE_EXTENDER),
    FLAGINFO_NUMV(IO_REPARSE_TAG_HSM2),
    FLAGINFO_NUMV(IO_REPARSE_TAG_SIS),
    FLAGINFO_NUMV(IO_REPARSE_TAG_WIM),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CSV),
    FLAGINFO_NUMV(IO_REPARSE_TAG_DFS),
    FLAGINFO_NUMV(IO_REPARSE_TAG_FILTER_MANAGER),
    FLAGINFO_NUMV(IO_REPARSE_TAG_SYMLINK),
    FLAGINFO_NUMV(IO_REPARSE_TAG_IIS_CACHE),
    FLAGINFO_NUMV(IO_REPARSE_TAG_DFSR),
    FLAGINFO_NUMV(IO_REPARSE_TAG_DEDUP),
    FLAGINFO_NUMV(IO_REPARSE_TAG_APPXSTRM),
    FLAGINFO_NUMV(IO_REPARSE_TAG_NFS),
    FLAGINFO_NUMV(IO_REPARSE_TAG_FILE_PLACEHOLDER),
    FLAGINFO_NUMV(IO_REPARSE_TAG_DFM),
    FLAGINFO_NUMV(IO_REPARSE_TAG_WOF),
    FLAGINFO_NUMV(IO_REPARSE_TAG_WCI),
    FLAGINFO_NUMV(IO_REPARSE_TAG_WCI_1),
    FLAGINFO_NUMV(IO_REPARSE_TAG_GLOBAL_REPARSE),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_1),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_2),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_3),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_4),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_5),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_6),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_7),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_8),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_9),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_A),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_B),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_C),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_D),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_E),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_F),
    FLAGINFO_NUMV(IO_REPARSE_TAG_CLOUD_MASK),
    FLAGINFO_NUMV(IO_REPARSE_TAG_APPEXECLINK),
    FLAGINFO_NUMV(IO_REPARSE_TAG_PROJFS),
    FLAGINFO_NUMV(IO_REPARSE_TAG_LX_SYMLINK),
    FLAGINFO_NUMV(IO_REPARSE_TAG_STORAGE_SYNC),
    FLAGINFO_NUMV(IO_REPARSE_TAG_WCI_TOMBSTONE),
    FLAGINFO_NUMV(IO_REPARSE_TAG_UNHANDLED),
    FLAGINFO_NUMV(IO_REPARSE_TAG_ONEDRIVE),
    FLAGINFO_NUMV(IO_REPARSE_TAG_PROJFS_TOMBSTONE),
    FLAGINFO_NUMV(IO_REPARSE_TAG_AF_UNIX),
    FLAGINFO_NUMV(IO_REPARSE_TAG_LX_FIFO),
    FLAGINFO_NUMV(IO_REPARSE_TAG_LX_CHR),
    FLAGINFO_NUMV(IO_REPARSE_TAG_LX_BLK),
    FLAGINFO_END()
};

static LPCWSTR AppExecLinkParts[] =
{
    L"AppPackageID",
    L"AppUserModelID",
    L"TargetPath",
};

//-----------------------------------------------------------------------------
// Support for REPARSE_DATA_BUFFER
//
// - Total data length must be REPARSE_DATA_BUFFER_HEADER_SIZE + sizeof(MountPointReparseBuffer) - sizeof(WCHAR) + filenames
// - ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE must be equal to total data length
// - There must be both NT name and DOS name. Both names will be zero-terminated
//

static ULONG GetTotalDataLength(PREPARSE_DATA_BUFFER ReparseData)
{
    ULONG TotalLength = 0;

    if(ReparseData != NULL)
        TotalLength = FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer) + ReparseData->ReparseDataLength;
    return TotalLength;
}

static int UpdateReparseDataLength(PREPARSE_DATA_BUFFER ReparseData, LPTSTR szBaseBuffer)
{
    size_t BufferOffset = (LPBYTE)szBaseBuffer - (LPBYTE)&ReparseData->MountPointReparseBuffer;

    ReparseData->ReparseDataLength = (USHORT)(BufferOffset +
                                              ReparseData->MountPointReparseBuffer.PrintNameOffset +
                                              ReparseData->MountPointReparseBuffer.PrintNameLength +
                                              sizeof(WCHAR));
    return ERROR_SUCCESS;
}

static int SetReparseDataTag(PREPARSE_DATA_BUFFER ReparseData, ULONG ReparseTag, ULONG DataLength)
{
    // Always clear everything before changing tag
    memset(ReparseData, 0, DataLength);
    ReparseData->ReparseTag = ReparseTag;

    // For a few known reparse formats, set empty values
    switch(ReparseTag)
    {
        case IO_REPARSE_TAG_MOUNT_POINT:
            ReparseData->ReparseDataLength = sizeof(ReparseData->MountPointReparseBuffer);
            ReparseData->MountPointReparseBuffer.PrintNameOffset = sizeof(WCHAR);
            break;

        case IO_REPARSE_TAG_SYMLINK:
            ReparseData->ReparseDataLength = sizeof(ReparseData->SymbolicLinkReparseBuffer);
            ReparseData->SymbolicLinkReparseBuffer.PrintNameOffset = sizeof(WCHAR);
            break;

        case IO_REPARSE_TAG_WIM:
            ReparseData->ReparseDataLength = sizeof(ReparseData->WimImageReparseBuffer);
            break;
    }

    return ERROR_SUCCESS;
}

static int SetReparseDataSubstName(PREPARSE_DATA_BUFFER ReparseData, LPTSTR szBaseBuffer, LPCTSTR szSubstName)
{
    LPTSTR szOldPrintName;
    LPTSTR szNewPrintName;
    size_t cchSubstName = _tcslen(szSubstName);

    // Fix the print name offset and size
    szOldPrintName = szBaseBuffer + ReparseData->MountPointReparseBuffer.PrintNameOffset / sizeof(WCHAR);
    szNewPrintName = szBaseBuffer + cchSubstName + 1;
    memmove(szNewPrintName, szOldPrintName, ReparseData->MountPointReparseBuffer.PrintNameLength + sizeof(WCHAR));
    ReparseData->MountPointReparseBuffer.PrintNameOffset = (USHORT)((LPBYTE)szNewPrintName - (LPBYTE)szBaseBuffer);

    // Copy the subst name
    memmove(szBaseBuffer, szSubstName, (cchSubstName + 1) * sizeof(WCHAR));
    ReparseData->MountPointReparseBuffer.SubstituteNameOffset = 0;
    ReparseData->MountPointReparseBuffer.SubstituteNameLength = (USHORT)(cchSubstName * sizeof(WCHAR));
    
    // Fix the reparse data size
    return UpdateReparseDataLength(ReparseData, szBaseBuffer);
}

static int SetReparseDataPrintName(PREPARSE_DATA_BUFFER ReparseData, LPTSTR szBaseBuffer, LPCTSTR szPrintName)
{
    size_t cbPrintName = _tcslen(szPrintName) * sizeof(WCHAR);

    // Copy the print name
    memmove((LPBYTE)szBaseBuffer + ReparseData->MountPointReparseBuffer.PrintNameOffset, szPrintName, cbPrintName + sizeof(WCHAR));
    ReparseData->MountPointReparseBuffer.PrintNameLength = (USHORT)cbPrintName;
    
    // Fix the reparse data size
    return UpdateReparseDataLength(ReparseData, szBaseBuffer);
}

static void InitializeReparseData(TFileTestData * pData)
{
    PREPARSE_DATA_BUFFER ReparseData;

    if(pData->ReparseData == NULL)
    {
        // Allocate the data buffer for the reparse
        ReparseData = (PREPARSE_DATA_BUFFER)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);

        // Fill-in the IO_REPARSE_TAG_MOUNT_POINT
        if(ReparseData != NULL)
        {
            // Initialize the reparse data
            ReparseData->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
            SetReparseDataSubstName(ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, L"\\??\\C:\\Windows");
            SetReparseDataPrintName(ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, L"C:\\Windows");

            // Set the reparse data to the dialog data
            pData->ReparseData = ReparseData;
            pData->ReparseDataLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        }
    }
}

static bool ReparseTargetIsDirectory(PREPARSE_DATA_BUFFER ReparseData)
{
    FILE_BASIC_INFORMATION BasicInfo;
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    LPTSTR szBaseBuffer = 0;
    bool bResult = false;

    // Determine the pointer to the target name
    switch(ReparseData->ReparseTag)
    {
        case IO_REPARSE_TAG_MOUNT_POINT:
            szBaseBuffer = ReparseData->MountPointReparseBuffer.PathBuffer;
            break;

        case IO_REPARSE_TAG_SYMLINK:
            szBaseBuffer = ReparseData->SymbolicLinkReparseBuffer.PathBuffer;
            break;
    }

    // Only for mount point/symbolic link
    if(szBaseBuffer != NULL)
    {
        InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        FileName.MaximumLength =
        FileName.Length = ReparseData->MountPointReparseBuffer.SubstituteNameLength;
        FileName.Buffer = szBaseBuffer + ReparseData->MountPointReparseBuffer.SubstituteNameOffset / sizeof(WCHAR);
        Status = NtQueryAttributesFile(&ObjAttr, &BasicInfo);
        bResult = (NT_SUCCESS(Status) && (BasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    }

    return bResult;
}


//-----------------------------------------------------------------------------
// Helper functions

static void BinaryToString(LPBYTE pbData, ULONG cbData, LPTSTR szBuffer, size_t cchBuffer)
{
    LPTSTR szSaveBuffer = szBuffer;
    LPTSTR szBufferEnd = szBuffer + cchBuffer;
    LPBYTE pbDataEnd = pbData + cbData;

    while((szBuffer + 3) <= szBufferEnd && pbData < pbDataEnd)
    {
        // Put space there, if any
        if(szBuffer > szSaveBuffer)
            *szBuffer++ = _T(' ');

        // Printf the hexa digit
        *szBuffer++ = HexaAlphabetLower[pbData[0] >> 0x04];
        *szBuffer++ = HexaAlphabetLower[pbData[0] & 0x0F];
        pbData++;
    }

    // Terminate the buffer with zero
    szBuffer[0] = 0;
}

static LPTSTR FormatReparseTag(ULONG ReparseTag, LPTSTR szBuffer, size_t cchBuffer)
{
    LPCSTR szReparseTag = "unknown";

    // Try to find the reparse tag
    for(size_t i = 0; i < _countof(ReparseTags); i++)
    {
        if(ReparseTags[i].dwValue == ReparseTag)
        {
            szReparseTag = ReparseTags[i].szFlagText;
            break;
        }
    }

    // Format the string
    StringCchPrintf(szBuffer, cchBuffer, _T("%08X (%s)"), ReparseTag, TWideString(szReparseTag).GetStr());
    return szBuffer;
}

static HTREEITEM TreeView_InsertString(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szItemText,
    LPARAM lParam = 0)
{
    TVINSERTSTRUCT tvi;

    // Insert the item
    ZeroMemory(&tvi, sizeof(TVITEM));
    tvi.hParent = hParent;
    tvi.hInsertAfter = TVI_LAST;
    tvi.item.mask = TVIF_TEXT | TVIF_PARAM;
    tvi.item.pszText = (LPTSTR)szItemText;
    tvi.item.lParam = lParam;
    return TreeView_InsertItem(hWndTree, &tvi);
}

static HTREEITEM TreeView_InsertNameAndValue(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szValueName,
    LPCTSTR szValueText,
    LPARAM lParam = 0)
{
    TCHAR szItemText[MAX_TREE_ITEM_LENGTH];

    // Prepare the item text
    StringCchPrintf(szItemText, _countof(szItemText), _T("%s: %s"), szValueName, szValueText);
    return TreeView_InsertString(hWndTree, hParent, szItemText, lParam);
}

static HTREEITEM TreeView_InsertInteger(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szValueName,
    LPCTSTR szFormat,
    ULONG IntValue)
{
    TCHAR szValueText[0x20];

    // Format the value
    StringCchPrintf(szValueText, _countof(szValueText), szFormat, IntValue);
    return TreeView_InsertNameAndValue(hWndTree, hParent, szValueName, szValueText);
}

static HTREEITEM TreeView_InsertGuid(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szValueName,
    GUID & Guid,
    LPARAM lParam = 0)
{
    TCHAR szValueText[0x40];

    // Format the value
    GuidToString(&Guid, szValueText, _countof(szValueText));
    return TreeView_InsertNameAndValue(hWndTree, hParent, szValueName, szValueText, lParam);
}

static HTREEITEM TreeView_InsertBinary(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szValueName,
    PVOID pvData,
    ULONG cbData,
    LPARAM lParam = 0)
{
    HTREEITEM hNewItem = NULL;
    LPTSTR szValueText;
    size_t cchLength;

    // Limit the maximum length
    if(cbData > (MAXIMUM_REPARSE_DATA_BUFFER_SIZE - 8))
        cbData = (MAXIMUM_REPARSE_DATA_BUFFER_SIZE - 8);
    cchLength = (cbData * 3) + 1;

    // Allocate buffer for the item text
    szValueText = new TCHAR[cchLength];
    if(szValueText != NULL)
    {
        // Format the value
        BinaryToString((LPBYTE)pvData, cbData, szValueText, cchLength);

        // Apply to the tree item
        hNewItem = TreeView_InsertNameAndValue(hWndTree, hParent, szValueName, szValueText, lParam);
        delete [] szValueText;
    }

    return hNewItem;
}

static HTREEITEM TreeView_InsertStrOffs(
    HWND hWndTree,
    HTREEITEM hParent,
    LPCTSTR szValueName,
    LPVOID BeginOfStruct,
    USHORT StringOffset,
    USHORT StringLength,
    LPARAM lParam = 0)
{
    WCHAR szValueText[MAX_TREE_ITEM_LENGTH];
    size_t cchStringLength = (StringLength / sizeof(WCHAR));

    // Put the prefix
    StringCchPrintf(szValueText, MAX_TREE_ITEM_LENGTH, _T("0x%02X \""), StringOffset);
    StringCchCatN(szValueText, MAX_TREE_ITEM_LENGTH, (LPWSTR)((LPBYTE)BeginOfStruct + StringOffset), cchStringLength);
    StringCchCat(szValueText, MAX_TREE_ITEM_LENGTH, _T("\""));

    // Insert as tree item
    return TreeView_InsertNameAndValue(hWndTree, hParent, szValueName, szValueText, lParam);
}

static void TreeView_InsertSubstName(
    HWND hWndTree,
    HTREEITEM hParent,
    PREPARSE_DATA_BUFFER ReparseData,
    LPTSTR szBaseBuffer,
    LPARAM lParam = 0)
{
    TreeView_InsertStrOffs(hWndTree, hParent, _T("SubstituteNameOffset"),
                                              szBaseBuffer,
                                              ReparseData->MountPointReparseBuffer.SubstituteNameOffset,
                                              ReparseData->MountPointReparseBuffer.SubstituteNameLength,
                                              lParam);
    TreeView_InsertInteger(hWndTree, hParent, _T("SubstituteNameLength"),
                                              _T("0x%02X"),
                                              ReparseData->MountPointReparseBuffer.SubstituteNameLength);
}

static void TreeView_InsertPrintName(
    HWND hWndTree,
    HTREEITEM hParent,
    PREPARSE_DATA_BUFFER ReparseData,
    LPTSTR szBaseBuffer,
    LPARAM lParam = 0)
{
    TreeView_InsertStrOffs(hWndTree, hParent, _T("PrintNameOffset"),
                                              szBaseBuffer,
                                              ReparseData->MountPointReparseBuffer.PrintNameOffset,
                                              ReparseData->MountPointReparseBuffer.PrintNameLength,
                                              lParam);
    TreeView_InsertInteger(hWndTree, hParent, _T("PrintNameLength"),
                                              _T("0x%02X"),
                                              ReparseData->MountPointReparseBuffer.PrintNameLength);
}

static bool TreeView_EditString(HWND hWndEdit, LPTSTR szBaseBuffer, USHORT NameOffset, USHORT NameLength)
{
    LPTSTR szString;

    // We need to allocate the string
    szString = (LPTSTR)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, NameLength + sizeof(WCHAR));
    if(szString != NULL)
    {
        memcpy(szString, (LPBYTE)szBaseBuffer + NameOffset, NameLength);
        SetWindowText(hWndEdit, szString);
        HeapFree(g_hHeap, 0, szString);
        return true;
    }

    return false;
}

LPTSTR GetFullHardLinkName(PFILE_LINK_ENTRY_INFORMATION pLinkInfo, LPTSTR szFileName)
{
    PFILE_NAME_INFORMATION pNameInfo = NULL;
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    LPTSTR szHardLinkName = NULL;
    LPTSTR szTemp;
    HANDLE hVolume = NULL;
    HANDLE hParent;
    TCHAR szVolumeName[MAX_PATH];
    ULONG BackSlashCount = 0;
    ULONG BackSlashMax = 1;
    ULONG CharIndex = 0;
    ULONG NameLength;

    // Extract volume name from the file name
    if(szFileName[0] == _T('\\'))
        BackSlashMax = 3;
    while(BackSlashCount < BackSlashMax)
    {
        if(szFileName[CharIndex] == _T('\\'))
            BackSlashCount++;
        szVolumeName[CharIndex] = szFileName[CharIndex];
        CharIndex++;
    }
    szVolumeName[CharIndex] = 0;

    // Open the volume
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FileNameToUnicodeString(&FileName, szVolumeName);
    if(NT_SUCCESS(Status))
    {
        Status = NtOpenFile(&hVolume,
                             SYNCHRONIZE,
                            &ObjAttr,
                            &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_SYNCHRONOUS_IO_ALERT);
        FreeFileNameString(&FileName);
    }
    
    if(NT_SUCCESS(Status))
    {
        // Now open the target directory by ID
        InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, hVolume, NULL);
        FileName.MaximumLength = sizeof(LARGE_INTEGER);
        FileName.Length = sizeof(LARGE_INTEGER);
        FileName.Buffer = (PWSTR)&pLinkInfo->ParentFileId;
        Status = NtOpenFile(&hParent,
                             FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                            &ObjAttr,
                            &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN_BY_FILE_ID | FILE_SYNCHRONOUS_IO_ALERT);

        // Now query name of the directory
        if(NT_SUCCESS(Status))
        {
            pNameInfo = (PFILE_NAME_INFORMATION)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, 0x1000);
            if(pNameInfo != NULL)
            {
                Status = NtQueryInformationFile(hParent,
                                               &IoStatus,
                                                pNameInfo,
                                                0x1000,
                                                FileNameInformation);
                if(NT_SUCCESS(Status))
                {
                    NameLength = (ULONG)_tcslen(szVolumeName) * sizeof(WCHAR) +
                                 pNameInfo->FileNameLength +
                                 (pLinkInfo->FileNameLength + 1) * sizeof(WCHAR);
                    szHardLinkName = szTemp = (LPTSTR)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, NameLength);
                    if(szHardLinkName != NULL)
                    {
                        // Copy the volume name
                        StringCchCopy(szTemp, NameLength / sizeof(WCHAR), szVolumeName);
                        szTemp = _tcsrchr(szTemp, _T('\\'));

                        // Copy the directory name
                        memcpy(szTemp, pNameInfo->FileName, pNameInfo->FileNameLength);
                        szTemp += pNameInfo->FileNameLength / sizeof(WCHAR);

                        // Append backslash
                        if(pNameInfo->FileNameLength > 2)
                            *szTemp++ = _T('\\');

                        // Copy the link name
                        memcpy(szTemp, pLinkInfo->FileName, (pLinkInfo->FileNameLength * sizeof(WCHAR)));
                        szTemp[pLinkInfo->FileNameLength] = 0;
                    }
                }

                HeapFree(g_hHeap, 0, pNameInfo);
            }
            NtClose(hParent);
        }
        NtClose(hVolume);
    }

    return szHardLinkName;
}

static void TreeView_InsertHsmData(HWND hWndChild, HTREEITEM hParent, PHSM_DATA HsmData, ULONG BitmapMask)
{
    ULONG BitmapBit = 1;

    // Insert the base data
    TreeView_InsertInteger(hWndChild, hParent, _T("Magic"), _T("0x%08X"), HsmData->Magic);
    TreeView_InsertInteger(hWndChild, hParent, _T("Crc32"), _T("0x%08X"), HsmData->Crc32);
    TreeView_InsertInteger(hWndChild, hParent, _T("Length"), _T("0x%08X"), HsmData->Length);
    TreeView_InsertInteger(hWndChild, hParent, _T("Flags"), _T("0x%04X"), HsmData->Flags);
    TreeView_InsertInteger(hWndChild, hParent, _T("NumberOfElements"), _T("0x%04X"), HsmData->NumberOfElements);

    // Insert the page for elements
    hParent = TreeView_InsertString(hWndChild, hParent, _T("Elements"));

    // Insert each element
    for (USHORT i = 0; i < HsmData->NumberOfElements; i++, BitmapBit <<= 1)
    {
        PHSM_ELEMENT_INFO pElementInfo = &HsmData->ElementInfos[i];
        HTREEITEM hBitmap;
        HTREEITEM hItem;
        LPBYTE ElementData;
        TCHAR szElement[0x10];

        // Insert the "[x]" part
        StringCchPrintf(szElement, _countof(szElement), _T("[%u]"), i);
        hItem = TreeView_InsertString(hWndChild, hParent, szElement);

        // Insert the element info
        TreeView_InsertInteger(hWndChild, hItem, _T("Type"), _T("%04X"), pElementInfo->Type);
        TreeView_InsertInteger(hWndChild, hItem, _T("Length"), _T("%04X"), pElementInfo->Length);
        TreeView_InsertInteger(hWndChild, hItem, _T("Offset"), _T("%08X"), pElementInfo->Offset);

        // Insert element data
        if(pElementInfo->Offset && pElementInfo->Length)
        {
            // Retrieve the element data
            ElementData = HsmGetElementData(HsmData, i);

            // Can the data be a bitmap?
            if(BitmapMask & BitmapBit)
            {
                hBitmap = TreeView_InsertString(hWndChild, hItem, _T("HSM_BITMAP"));
                TreeView_InsertHsmData(hWndChild, hBitmap, (PHSM_DATA)ElementData, 0);
            }
            else
            {
                TreeView_InsertBinary(hWndChild, hItem, _T("Data"), ElementData, pElementInfo->Length);
            }
        }
    }
}

static NTSTATUS TreeView_InsertHsm(HWND hWndChild, HTREEITEM hParent, PREPARSE_DATA_BUFFER ReparseData, ULONG ReparseDataLength)
{
    PREPARSE_DATA_BUFFER HsmReparseData = NULL;
    HTREEITEM hItem;
    NTSTATUS Status;

    // Decompress the reparse data
    Status = HsmUncompressData(ReparseData, ReparseDataLength, &HsmReparseData);
    if(NT_SUCCESS(Status))
    {
        hItem = TreeView_InsertString(hWndChild, hParent, _T("HSM_REPARSE_DATA"));
        if(hItem != NULL)
        {
            // Insert the flags and length
            TreeView_InsertInteger(hWndChild, hItem, _T("Flags"), _T("0x%04X"), HsmReparseData->HsmReparseBufferRaw.Flags);
            TreeView_InsertInteger(hWndChild, hItem, _T("Length"), _T("0x%04X"), HsmReparseData->HsmReparseBufferRaw.Length);
            
            // Insert the FileData substructure
            hItem = TreeView_InsertString(hWndChild, hItem, _T("HSM_DATA"));

            // Get the pointer to the HSM_DATA. If validated OK, insert the substructure
            Status = HsmValidateReparseData(HsmReparseData);
            if(NT_SUCCESS(Status))
            {
                PHSM_REPARSE_DATA HsmRpData = (PHSM_REPARSE_DATA)(&HsmReparseData->HsmReparseBufferRaw);
                PHSM_DATA HsmData = &HsmRpData->FileData;

                TreeView_InsertHsmData(hWndChild, hItem, HsmData, 0);
                Status = STATUS_SUCCESS;
            }
            else
            {
                TreeView_InsertBinary(hWndChild, hItem, _T("RawData (invalid)"), ReparseData->HsmReparseBufferRaw.RawData, HsmReparseData->HsmReparseBufferRaw.Length);
                Status = STATUS_SUCCESS;
            }
        }

        // Free the uncompressed reparse data buffer
        if(HsmReparseData != NULL && HsmReparseData != ReparseData)
        {
            HeapFree(g_hHeap, 0, HsmReparseData);
        }
    }

    return Status;
}


//-----------------------------------------------------------------------------
// Message handlers

static TAnchors * pAnchors = NULL;

static int OnInitDialog(HWND hDlg, LPARAM lParam)
{
    PROPSHEETPAGE * pPage = (PROPSHEETPAGE *)lParam;
    TFileTestData * pData = (TFileTestData *)pPage->lParam;
    LPCTSTR szReparseName;
    HWND hWndChild;

    // Apply the data to the dialog
    SetDialogData(hDlg, pPage->lParam);

    // Initialize the reparse data
    InitializeReparseData(pData);

    // Configure dialog resizing
    if(pData->bEnableResizing)
    {
        pAnchors = new TAnchors(hDlg);
        pAnchors->AddAnchor(hDlg, IDC_SYMLINK_FRAME, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_SYMLINK_TARGET, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_SYMLINK_QUERY, akLeft | akTop);
        pAnchors->AddAnchor(hDlg, IDC_SYMLINK_CREATE, akLeftCenter | akTop);
        pAnchors->AddAnchor(hDlg, IDC_SYMLINK_DELETE, akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_HARDLINK_FRAME, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_HARDLINK_LIST, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_NEW_HARDLINK, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_HARDLINK_CREATE, akLeft | akTop);
        pAnchors->AddAnchor(hDlg, IDC_HARDLINK_QUERY, akLeftCenter | akTop);
        pAnchors->AddAnchor(hDlg, IDC_HARDLINK_DELETE, akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE_FRAME, akAll);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE, akLeft | akTop | akRight);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE_DATA, akAll);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE_CREATE, akLeft | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE_QUERY, akLeftCenter | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_REPARSE_DELETE, akRight | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_RESULT_FRAME, akLeft | akRight | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_ERROR_CODE_TITLE, akLeft | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_ERROR_CODE, akLeft | akRight | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_INFORMATION_TITLE, akLeft | akBottom);
        pAnchors->AddAnchor(hDlg, IDC_INFORMATION, akLeft | akRight | akBottom);
    }

    // We need this in order to create symbolic link
    EnablePrivilege(_T("SeCreateSymbolicLinkPrivilege"));

    // Congigure the right-arrow image 
    AttachIconToEdit(hDlg, GetDlgItem(hDlg, IDC_SYMLINK_TARGET), MAKEINTRESOURCE(IDI_RIGHT_ARROW));

    // Default number of characters for combo box's edit field is 46.
    // We have to increase it.
    hWndChild = GetDlgItem(hDlg, IDC_HARDLINK_LIST);
    ComboBox_LimitText(hWndChild, MAX_PATH);

    // Pre-fill the dialog
    SetDlgItemText(hDlg, IDC_SYMLINK, _T("\\??\\C:"));

    SetDlgItemText(hDlg, IDC_HARDLINK_LIST, _T("C:\\TestFile.bin"));
    SetDlgItemText(hDlg, IDC_NEW_HARDLINK, _T("C:\\TestFile_HardLink.bin"));

    szReparseName = (pData->IsDefaultFileName1) ? _T("C:\\Windows_Reparse") : pData->szFileName1;
    SetDlgItemText(hDlg, IDC_REPARSE, szReparseName);

    // Initiate updating of the view
    PostMessage(hDlg, WM_UPDATE_VIEW, 0, 0);
    return TRUE;
}                                                                         

static int OnSetActive(HWND /* hDlg */)
{
    return FALSE;
}

static int OnKillActive(HWND /* hDlg */)
{
    return FALSE;
}

static int OnBeginLabelEdit(HWND hDlg, LPNMTVDISPINFO pNMDispInfo)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData = GetDialogData(hDlg);
    TCHAR szItemText[0x200];
    HWND hTreeView = GetDlgItem(hDlg, IDC_REPARSE_DATA);
    HWND hWndEdit;
    BOOL bStartEditing = FALSE;

    // Retrieve the edit control
    hWndEdit = TreeView_GetEditControl(hTreeView);
    if(hWndEdit != NULL)
    {
        // Get the reparse data
        ReparseData = pData->ReparseData;

        // Perform the item-specific editing
        switch(pNMDispInfo->item.lParam)
        {
            case ITEM_TYPE_REPARSE_TAG:
                StringCchPrintf(szItemText, _countof(szItemText), _T("%08X"), ReparseData->ReparseTag);
                SetWindowText(hWndEdit, szItemText);
                bStartEditing = TRUE;
                break;

            case ITEM_TYPE_SUBSTNAME_MP:
                bStartEditing = TreeView_EditString(hWndEdit,
                                                    ReparseData->MountPointReparseBuffer.PathBuffer,
                                                    ReparseData->MountPointReparseBuffer.SubstituteNameOffset,
                                                    ReparseData->MountPointReparseBuffer.SubstituteNameLength);

            case ITEM_TYPE_PRINTNAME_MP:
                bStartEditing = TreeView_EditString(hWndEdit,
                                                    ReparseData->MountPointReparseBuffer.PathBuffer,
                                                    ReparseData->MountPointReparseBuffer.PrintNameOffset,
                                                    ReparseData->MountPointReparseBuffer.PrintNameLength);
                break;

            case ITEM_TYPE_SUBSTNAME_LNK:
                bStartEditing = TreeView_EditString(hWndEdit,
                                                    ReparseData->SymbolicLinkReparseBuffer.PathBuffer,
                                                    ReparseData->SymbolicLinkReparseBuffer.SubstituteNameOffset,
                                                    ReparseData->SymbolicLinkReparseBuffer.SubstituteNameLength);
                break;

            case ITEM_TYPE_PRINTNAME_LNK:
                bStartEditing = TreeView_EditString(hWndEdit,
                                                    ReparseData->SymbolicLinkReparseBuffer.PathBuffer,
                                                    ReparseData->SymbolicLinkReparseBuffer.PrintNameOffset,
                                                    ReparseData->SymbolicLinkReparseBuffer.PrintNameLength);
                break;
        }
    }

    // By default, the item is not editable
    if(bStartEditing == FALSE)
        SetResultInfo(hDlg, RSI_NTSTATUS, STATUS_CANNOT_EDIT_THIS);

    // Setup the close dialog and start editing (or not)
    DisableCloseDialog(hDlg, bStartEditing);
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, bStartEditing ? FALSE : TRUE);
    return TRUE;
}

static int OnEndLabelEdit(HWND hDlg, NMTVDISPINFO * pNMDispInfo)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData = GetDialogData(hDlg);
    DWORD IntValue32 = 0;
    DWORD dwErrCode = ERROR_NOT_SUPPORTED;

    // Only do something when the test has been changed
    if(pNMDispInfo->item.pszText)
    {
        // Get the pointer to the reparse data
        ReparseData = pData->ReparseData;

        // Perform the item-specific editing
        switch(pNMDispInfo->item.lParam)
        {
            case ITEM_TYPE_REPARSE_TAG:
                dwErrCode = Text2Hex32(pNMDispInfo->item.pszText, &IntValue32);
                if(dwErrCode == ERROR_SUCCESS && IntValue32 != ReparseData->ReparseTag)
                    dwErrCode = SetReparseDataTag(ReparseData, IntValue32, pData->ReparseDataLength);
                break;

            case ITEM_TYPE_SUBSTNAME_MP:
                if(ReparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                    dwErrCode = SetReparseDataSubstName(ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, pNMDispInfo->item.pszText);
                break;

            case ITEM_TYPE_PRINTNAME_MP:
                if(ReparseData->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                    dwErrCode = SetReparseDataPrintName(ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, pNMDispInfo->item.pszText);
                break;

            case ITEM_TYPE_SUBSTNAME_LNK:
                if(ReparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK)
                    dwErrCode = SetReparseDataSubstName(ReparseData, ReparseData->SymbolicLinkReparseBuffer.PathBuffer, pNMDispInfo->item.pszText);
                break;

            case ITEM_TYPE_PRINTNAME_LNK:
                if(ReparseData->ReparseTag == IO_REPARSE_TAG_SYMLINK)
                    dwErrCode = SetReparseDataPrintName(ReparseData, ReparseData->SymbolicLinkReparseBuffer.PathBuffer, pNMDispInfo->item.pszText);
                break;
        }

        // If we are going to accept changes, we need to update the view
        if(dwErrCode == ERROR_SUCCESS)
        {
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
            PostMessage(hDlg, WM_UPDATE_VIEW, 0, 0);
        }
    }

    DisableCloseDialog(hDlg, FALSE);
    return TRUE;
}

static int OnDoubleClick(HWND hDlg, LPNMHDR pNMHDR)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData;
    LPARAM lParam;
    ULONG ReparseTag;

    // Was it the tree view?
    if(pNMHDR->idFrom == IDC_REPARSE_DATA)
    {
        // Retrieve the item param
        lParam = TreeView_GetItemParam(pNMHDR->hwndFrom, TreeView_GetSelection(pNMHDR->hwndFrom));
            
        // If reparse tag, we can give the user a choice
        if(lParam == ITEM_TYPE_REPARSE_TAG)
        {
            // Retrieve the reparse data
            pData = GetDialogData(hDlg);
            ReparseData = pData->ReparseData;
            ReparseTag = ReparseData->ReparseTag;

            // Let the user choose a new reparse tag
            if(FlagsDialog(hDlg, IDS_CHOOSE_REPARSE_TAG, ReparseTags, ReparseTag) == IDOK)
            {
                SetReparseDataTag(ReparseData, ReparseTag, pData->ReparseDataLength);
                PostMessage(hDlg, WM_UPDATE_VIEW, 0, 0);
            }
        }
    }

    return TRUE;
}

static int OnSymlinkCreate(HWND hDlg)
{
    TFileTestData * pData = GetDialogData(hDlg);
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING SymlinkName;
    UNICODE_STRING TargetName;
    NTSTATUS Status;
    TCHAR szSymlinkName[MAX_PATH];
    TCHAR szTargetName[MAX_PATH];

    // Get the name of the symbolic link
    GetDlgItemText(hDlg, IDC_SYMLINK, szSymlinkName, _countof(szSymlinkName));
    GetDlgItemText(hDlg, IDC_SYMLINK_TARGET, szTargetName, _countof(szTargetName));

    // Query the symbolic link
    InitializeObjectAttributes(&ObjAttr,
                               &SymlinkName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    RtlInitUnicodeString(&SymlinkName, szSymlinkName);
    RtlInitUnicodeString(&TargetName, szTargetName);

    // Create the symbolic link
    Status = NtCreateSymbolicLinkObject(&pData->hSymLink,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjAttr,
                                        &TargetName);
    if(NT_SUCCESS(Status))
    {
        EnableDlgItems(hDlg, TRUE, IDC_SYMLINK_DELETE, 0);
    }

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_NOINFO, Status);
    return TRUE;
}

static int OnSymlinkQuery(HWND hDlg)
{
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING SymlinkName;
    UNICODE_STRING TargetName;
    NTSTATUS Status;
    HANDLE Handle;
    TCHAR szSymlinkName[MAX_PATH];
    ULONG Length = 0;

    // Get the name of the symbolic link
    GetDlgItemText(hDlg, IDC_SYMLINK, szSymlinkName, _countof(szSymlinkName));

    // Query the symbolic link
    InitializeObjectAttributes(&ObjAttr, &SymlinkName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    RtlInitUnicodeString(&SymlinkName, szSymlinkName);
    Status = NtOpenSymbolicLinkObject(&Handle, SYMBOLIC_LINK_QUERY, &ObjAttr);
    if(NT_SUCCESS(Status))
    {
        TargetName.MaximumLength = 0x1000;
        TargetName.Length = 0;
        TargetName.Buffer = (PWSTR)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, TargetName.MaximumLength + 2);
        if(TargetName.Buffer != NULL)
        {
            Status = NtQuerySymbolicLinkObject(Handle, &TargetName, &Length);
            if(NT_SUCCESS(Status))
            {
                SetDlgItemText(hDlg, IDC_SYMLINK_TARGET, TargetName.Buffer);
            }

            HeapFree(g_hHeap, 0, TargetName.Buffer);
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        NtClose(Handle);
    }

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_INFO_INT32, Status, Length);
    return TRUE;
}

static int OnSymlinkDelete(HWND hDlg)
{
    TFileTestData * pData = GetDialogData(hDlg);
    NTSTATUS Status;

    Status = NtClose(pData->hSymLink);
    pData->hSymLink = NULL;

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_NOINFO, Status);
    EnableDlgItems(hDlg, FALSE, IDC_SYMLINK_DELETE, 0);
    return TRUE;
}

static void OnShowHardlinks(HWND hDlg)
{
    HWND hCombo = GetDlgItem(hDlg, IDC_HARDLINK_LIST);

    ComboBox_ShowDropdown(hCombo, TRUE);
}

static void OnUpdateView(HWND hDlg)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData = GetDialogData(hDlg);
    HTREEITEM hItem = TVI_ROOT;
    TCHAR szItemText[MAX_PATH];
    HWND hWndChild = GetDlgItem(hDlg, IDC_REPARSE_DATA);

    // Free all items in the tree view
    TreeView_DeleteAllItems(hWndChild);
    ReparseData = pData->ReparseData;

    // Insert the three common fields from the REPARSE_DATA
    TreeView_InsertNameAndValue(hWndChild, hItem, _T("Tag"), FormatReparseTag(ReparseData->ReparseTag, szItemText, _countof(szItemText)), ITEM_TYPE_REPARSE_TAG);
    TreeView_InsertInteger(hWndChild, hItem, _T("ReparseDataLength"), _T("0x%02X"), ReparseData->ReparseDataLength);
    TreeView_InsertInteger(hWndChild, hItem, _T("Reserved"), _T("0x%02X"), ReparseData->Reserved);

    // Insert the sub-structure
    switch(ReparseData->ReparseTag)
    {
        case IO_REPARSE_TAG_MOUNT_POINT:

            hItem = TreeView_InsertString(hWndChild, hItem, _T("MountPointReparseBuffer"));
            if(hItem != NULL)
            {
                TreeView_InsertSubstName(hWndChild, hItem, ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, ITEM_TYPE_SUBSTNAME_MP);
                TreeView_InsertPrintName(hWndChild, hItem, ReparseData, ReparseData->MountPointReparseBuffer.PathBuffer, ITEM_TYPE_PRINTNAME_MP);
            }
            break;

        case IO_REPARSE_TAG_SYMLINK:

            hItem = TreeView_InsertString(hWndChild, hItem, _T("SymbolicLinkReparseBuffer"));
            if(hItem != NULL)
            {
                TreeView_InsertSubstName(hWndChild, hItem, ReparseData, ReparseData->SymbolicLinkReparseBuffer.PathBuffer, ITEM_TYPE_SUBSTNAME_LNK);
                TreeView_InsertPrintName(hWndChild, hItem, ReparseData, ReparseData->SymbolicLinkReparseBuffer.PathBuffer, ITEM_TYPE_PRINTNAME_LNK);
                TreeView_InsertInteger  (hWndChild, hItem, _T("Flags"), _T("0x%04X"), ReparseData->SymbolicLinkReparseBuffer.Flags);
            }
            break;

        case IO_REPARSE_TAG_WIM:
            hItem = TreeView_InsertString(hWndChild, hItem, _T("WimImageReparseBuffer"));
            if(hItem != NULL)
            {
                TreeView_InsertGuid(hWndChild, hItem, _T("ImageGuid"),
                                                         ReparseData->WimImageReparseBuffer.ImageGuid);
                TreeView_InsertBinary(hWndChild, hItem, _T("ImagePathHash"),
                                                         ReparseData->WimImageReparseBuffer.ImagePathHash,
                                                         sizeof(ReparseData->WimImageReparseBuffer.ImagePathHash));
            }
            break;

        case IO_REPARSE_TAG_WOF:
            hItem = TreeView_InsertString(hWndChild, hItem, _T("WofReparsePointBuffer"));
            if(hItem != NULL)
            {
                TreeView_InsertInteger(hWndChild, hItem, _T("Wof.Version"),  _T("0x%04X"), ReparseData->WofReparseBuffer.Wof_Version);
                TreeView_InsertInteger(hWndChild, hItem, _T("Wof.Provider"), _T("0x%04X"), ReparseData->WofReparseBuffer.Wof_Provider);
                TreeView_InsertInteger(hWndChild, hItem, _T("Version"),      _T("0x%04X"), ReparseData->WofReparseBuffer.FileInfo_Version);
                TreeView_InsertInteger(hWndChild, hItem, _T("Algorithm"),    _T("0x%04X"), ReparseData->WofReparseBuffer.FileInfo_Algorithm);
            }
            break;

        case IO_REPARSE_TAG_APPEXECLINK:
            hItem = TreeView_InsertString(hWndChild, hItem, _T("AppExecLinkReparseBuffer"));
            if(hItem != NULL)
            {
                // Insert the string count
                TreeView_InsertInteger(hWndChild, hItem, _T("StringCount"), _T("0x%04X"), ReparseData->AppExecLinkReparseBuffer.StringCount);

                // Insert the strings
                if(ReparseData->AppExecLinkReparseBuffer.StringCount != 0)
                {
                    LPWSTR szString = (LPWSTR)ReparseData->AppExecLinkReparseBuffer.StringList;
                    WCHAR szValue[0x300];

                    for(ULONG i = 0; i < ReparseData->AppExecLinkReparseBuffer.StringCount; i++)
                    {
                        StringCchPrintf(szValue, _countof(szValue), _T("%s: %s"), AppExecLinkParts[i], szString);
                        TreeView_InsertString(hWndChild, hItem, szValue);
                        szString += wcslen(szString) + 1;
                    }
                }
            }
            break;

        case IO_REPARSE_TAG_WCI:
            hItem = TreeView_InsertString(hWndChild, hItem, _T("WcifsReparseBuffer"));
            if(hItem != NULL)
            {
                TreeView_InsertInteger(hWndChild, hItem, _T("Version"), _T("0x%08X"), ReparseData->WcifsReparseBuffer.Version);
                TreeView_InsertInteger(hWndChild, hItem, _T("Reserved"), _T("0x%08X"), ReparseData->WcifsReparseBuffer.Reserved);
                TreeView_InsertGuid(hWndChild, hItem, _T("LookupGuid"), ReparseData->WcifsReparseBuffer.LookupGuid);
                TreeView_InsertInteger(hWndChild, hItem, _T("WciNameLength"), _T("0x%04X"), ReparseData->WcifsReparseBuffer.WciNameLength);
                TreeView_InsertStrOffs(hWndChild, hItem, _T("WciName"), ReparseData, FIELD_OFFSET(REPARSE_DATA_BUFFER, WcifsReparseBuffer.WciName), ReparseData->WcifsReparseBuffer.WciNameLength, NULL);
            }
            break;

        default:
            if((ReparseData->ReparseTag & ~IO_REPARSE_TAG_CLOUD_MASK) == IO_REPARSE_TAG_CLOUD)
            {
                // Attemnpt to unpack and format the HSM data. If fails, fall back to the generic reparse buffer
                if(NT_SUCCESS(TreeView_InsertHsm(hWndChild, hItem, ReparseData, pData->ReparseDataValid)))
                    break;
            }

            hItem = TreeView_InsertString(hWndChild, hItem, _T("GenericReparseBuffer"));
            if(hItem != NULL)
            {
                if(ReparseData->ReparseDataLength != 0)
                {
                    TreeView_InsertBinary(hWndChild, hItem, _T("DataBuffer"), ReparseData->GenericReparseBuffer.DataBuffer, ReparseData->ReparseDataLength);
                }
                else
                {
                    TreeView_InsertString(hWndChild, hItem, _T("DataBuffer: (no data)"));
                }
            }
            break;
    }

    // Expand the tree view
    TreeView_Expand(hWndChild, hItem, TVE_EXPAND);
}

static int OnHardlinkCreate(HWND hDlg)
{
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK IoStatus = { 0 };
    UNICODE_STRING FileName;
    NTSTATUS Status = STATUS_SUCCESS;
    LPTSTR szPlainName;
    HANDLE hDirectory = NULL;
    HANDLE hFile = NULL;
    TCHAR szHardlinkName[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    TCHAR szDirectory[MAX_PATH];
    BYTE LinkInfoBuff[0x500];

    // Get the name of the symbolic link
    GetDlgItemText(hDlg, IDC_HARDLINK_LIST, szFileName, _countof(szFileName));
    GetDlgItemText(hDlg, IDC_NEW_HARDLINK, szHardlinkName, _countof(szHardlinkName));

    // Get the plain file name of the hardlink
    szPlainName = _tcsrchr(szHardlinkName, _T('\\'));
    if(szPlainName != NULL)
    {
        // Extract directory name from the hardlink path
        StringCchCopyN(szDirectory, _countof(szDirectory), szHardlinkName, (szPlainName - szHardlinkName + 1));
        szDirectory[szPlainName - szHardlinkName + 1] = 0;

        // Open the directory
        InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = FileNameToUnicodeString(&FileName, szDirectory);
        if(NT_SUCCESS(Status))
        {
            Status = NtOpenFile(&hDirectory,
                                 SYNCHRONIZE,
                                &ObjAttr,
                                &IoStatus,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 FILE_SYNCHRONOUS_IO_ALERT);
            FreeFileNameString(&FileName);
        }
    }

    if(NT_SUCCESS(Status))
    {
        // Open the file in order to set symlink
        InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = FileNameToUnicodeString(&FileName, szFileName);
        if(NT_SUCCESS(Status))
        {
            Status = NtOpenFile(&hFile, 
                                 MAXIMUM_ALLOWED,
                                &ObjAttr,
                                &IoStatus,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 0);
            FreeFileNameString(&FileName);
        }

        // Set the new hardlink to the file
        if(NT_SUCCESS(Status))
        {
            PFILE_LINK_INFORMATION pLinkInfo = (PFILE_LINK_INFORMATION)LinkInfoBuff;

            // Get the plain name of the directory
            szPlainName = GetPlainName(szHardlinkName);

            // Prepare the structure for creating hardlink
            pLinkInfo->ReplaceIfExists = FALSE;
            pLinkInfo->RootDirectory   = hDirectory;
            pLinkInfo->FileNameLength  = (ULONG)(_tcslen(szPlainName) * sizeof(TCHAR));
            StringCchCopy(pLinkInfo->FileName,
                         (sizeof(LinkInfoBuff) - sizeof(FILE_LINK_INFORMATION)) / sizeof(WCHAR),
                          szPlainName);

            // Create the hard link
            Status = NtSetInformationFile(hFile,
                                         &IoStatus,
                                          pLinkInfo,
                                          sizeof(FILE_LINK_INFORMATION) + pLinkInfo->FileNameLength,
                                          FileLinkInformation);
            NtClose(hFile);
        }
    }

    if(hDirectory != NULL)
        NtClose(hDirectory);
    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_INFORMATION, Status, &IoStatus);
    return TRUE;
}


static int OnHardlinkQuery(HWND hDlg)
{
    PFILE_LINK_ENTRY_INFORMATION pLinkInfo = NULL;
    PFILE_LINKS_INFORMATION pLinksInfo = NULL;
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK IoStatus = { 0 };
    UNICODE_STRING FileName;
    NTSTATUS Status = STATUS_SUCCESS;
    LPTSTR szHardLinkName;
    HANDLE hFile = NULL;
    TCHAR szFileName[MAX_PATH];
    ULONG HardLinkCount = 0;
    ULONG Length = 0x100;
    HWND hCombo = GetDlgItem(hDlg, IDC_HARDLINK_LIST);

    // Get the name of the symbolic link
    GetDlgItemText(hDlg, IDC_HARDLINK_LIST, szFileName, _countof(szFileName));

    // Open the hardlink as if it was file
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FileNameToUnicodeString(&FileName, szFileName);
    if(NT_SUCCESS(Status))
    {
        Status = NtOpenFile(&hFile, 
                             FILE_READ_ATTRIBUTES,
                            &ObjAttr,
                            &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             0);
        FreeFileNameString(&FileName);
    }

    // Query the file name
    if(NT_SUCCESS(Status))
    {
        for(;;)
        {
            // Allocate a chink of memory, that (we hope) will be enough
            pLinksInfo = (PFILE_LINKS_INFORMATION)HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, Length);
            if(pLinksInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            // Query all hardlinks
            Status = NtQueryInformationFile(hFile,
                                           &IoStatus,
                                            pLinksInfo,
                                            Length,
                                            FileHardLinkInformation);
            if(Status != STATUS_INFO_LENGTH_MISMATCH && Status != STATUS_BUFFER_OVERFLOW)
                break;

            // Free the buffer and double the length
            HeapFree(g_hHeap, 0, pLinksInfo);
            Length *= 2;
        }

        if(NT_SUCCESS(Status))
        {
            ComboBox_ResetContent(hCombo);
            if(pLinksInfo->EntriesReturned != 0)
            {
                pLinkInfo = &pLinksInfo->Entry;
                for(;;)
                {
                    szHardLinkName = GetFullHardLinkName(pLinkInfo, szFileName);
                    if(szHardLinkName != NULL)
                    {
                        // First hardlink will also be added to the combo box main field
                        if(HardLinkCount == 0)
                            SetWindowText(hCombo, szHardLinkName);

                        // Add the hardlink to the list
                        ComboBox_InsertString(hCombo, -1, szHardLinkName);
                        HeapFree(g_hHeap, 0, szHardLinkName);
                        HardLinkCount++;
                    }

                    if(pLinkInfo->NextEntryOffset == 0)
                        break;
                    pLinkInfo = (PFILE_LINK_ENTRY_INFORMATION)((LPBYTE)pLinkInfo + pLinkInfo->NextEntryOffset);
                }
            }
        }
        
        // Free the buffer and close file handle
        if(pLinksInfo != NULL)
            HeapFree(g_hHeap, 0, pLinksInfo);
        NtClose(hFile);
    }

    // Open the drop list to show all links
    if(HardLinkCount != 0)
        PostMessage(hDlg, WM_SHOW_HARDLINKS, 0, 0);
    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_INFORMATION, Status, &IoStatus);
    return TRUE;
}

static int OnHardlinkDelete(HWND hDlg)
{
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING FileName;
    NTSTATUS Status = STATUS_SUCCESS;
    TCHAR szHardlinkName[MAX_PATH];

    // Get the name of the symbolic link
    GetDlgItemText(hDlg, IDC_HARDLINK_LIST, szHardlinkName, _countof(szHardlinkName));

    // Open the symlink and delete it
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FileNameToUnicodeString(&FileName, szHardlinkName);
    if(NT_SUCCESS(Status))
    {
        Status = NtDeleteFile(&ObjAttr);
        FreeFileNameString(&FileName);
    }

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_NOINFO, Status);
    return TRUE;
}

static int OnReparseCreate(HWND hDlg)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData = GetDialogData(hDlg);
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK IoStatus = { 0 };
    UNICODE_STRING FileName;
    NTSTATUS Status;
    HANDLE hReparse = NULL;
    TCHAR szReparseName[MAX_PATH];
    ULONG CreateOptions = FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_REPARSE_POINT;

    // Get the name of the reparse point
    GetDlgItemText(hDlg, IDC_REPARSE, szReparseName, _countof(szReparseName));
    ReparseData = pData->ReparseData;

    // Prepare the name of the reparse point
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FileNameToUnicodeString(&FileName, szReparseName);
    if(NT_SUCCESS(Status))
    {
        // Get the name of the target. If it doesn't exist, we create new
        if(ReparseTargetIsDirectory(ReparseData))
            CreateOptions |= FILE_DIRECTORY_FILE;

        // Create/open the reparse point
        Status = NtCreateFile(&hReparse,
                               GENERIC_WRITE | SYNCHRONIZE,
                              &ObjAttr,
                              &IoStatus,
                               NULL,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_OPEN_IF,
                               CreateOptions,
                               NULL,
                               0);
        if(NT_SUCCESS(Status))
        {
            // Fire the IOCTL
            Status = NtFsControlFile(hReparse,
                                     NULL,
                                     NULL,
                                     NULL,
                                    &IoStatus,
                                     FSCTL_SET_REPARSE_POINT,
                                     ReparseData,
                                     GetTotalDataLength(ReparseData),
                                     NULL,
                                     0);
            // Close the handle
            NtClose(hReparse);
        }

        FreeFileNameString(&FileName);
    }

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_INFORMATION, Status, &IoStatus);
    return TRUE;
}

static int OnReparseQuery(HWND hDlg)
{
    PREPARSE_DATA_BUFFER ReparseData;
    TFileTestData * pData = GetDialogData(hDlg);
    OBJECT_ATTRIBUTES ObjAttr;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    HANDLE hFile = NULL;
    TCHAR szReparseName[MAX_PATH];

    // Get the name of the reparse point
    GetDlgItemText(hDlg, IDC_REPARSE, szReparseName, _countof(szReparseName));
    ReparseData = pData->ReparseData;

    // Open the reparse point
    InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = FileNameToUnicodeString(&FileName, szReparseName);
    if(NT_SUCCESS(Status))
    {
        // Open the reparse point
        Status = NtOpenFile(&hFile,
                             FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                            &ObjAttr,
                            &IoStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_ALERT);
        if(NT_SUCCESS(Status))
        {
            // Reset the data of the reparse point, so we don't have any artifacts from the past
            memset(pData->ReparseData, 0, pData->ReparseDataLength);
            pData->ReparseDataValid = 0;

            // Query the reparse point
            Status = NtFsControlFile(hFile,
                                     NULL,
                                     NULL,
                                     NULL,
                                    &IoStatus,
                                     FSCTL_GET_REPARSE_POINT,
                                     NULL,
                                     0,
                                     pData->ReparseData,
                                     pData->ReparseDataLength);
            if(NT_SUCCESS(Status))
            {
                // Update the view
                pData->ReparseDataValid = (ULONG)IoStatus.Information;
                PostMessage(hDlg, WM_UPDATE_VIEW, Status, 0);
            }

            NtClose(hFile);
        }

        FreeFileNameString(&FileName);
    }

    // Show the result to the dialog
    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_INFORMATION, Status, &IoStatus);
    return TRUE;
}

static int OnReparseDelete(HWND hDlg)
{
    OBJECT_ATTRIBUTES ObjAttr;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    TCHAR szReparseName[MAX_PATH];

    // Get the name of the reparse point
    GetDlgItemText(hDlg, IDC_REPARSE, szReparseName, _countof(szReparseName));

    // Open the reparse point
    Status = FileNameToUnicodeString(&FileName, szReparseName);
    if(NT_SUCCESS(Status))
    {
        InitializeObjectAttributes(&ObjAttr, &FileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
        Status = NtDeleteReparsePoint(&ObjAttr);
        FreeFileNameString(&FileName);
    }

    SetResultInfo(hDlg, RSI_NTSTATUS | RSI_NOINFO, Status);
    return TRUE;
}

static int OnCommand(HWND hDlg, UINT nNotify, UINT nIDCtrl)
{
    if(nNotify == BN_CLICKED || nNotify == 1)
    {
        switch(nIDCtrl)
        {
            case ID_EDIT_LABEL:
                return TreeView_EditLabel_ID(hDlg, IDC_REPARSE_DATA);

            case IDC_SYMLINK_CREATE:
                return OnSymlinkCreate(hDlg);

            case IDC_SYMLINK_QUERY:
                return OnSymlinkQuery(hDlg);

            case IDC_SYMLINK_DELETE:
                return OnSymlinkDelete(hDlg);

            case IDC_HARDLINK_CREATE:
                return OnHardlinkCreate(hDlg);

            case IDC_HARDLINK_QUERY:
                return OnHardlinkQuery(hDlg);

            case IDC_HARDLINK_DELETE:
                return OnHardlinkDelete(hDlg);

            case IDC_REPARSE_CREATE:
                return OnReparseCreate(hDlg);

            case IDC_REPARSE_QUERY:
                return OnReparseQuery(hDlg);

            case IDC_REPARSE_DELETE:
                return OnReparseDelete(hDlg);
        }
    }

    return FALSE;
}

static int OnNotify(HWND hDlg, LPNMHDR pNMHDR)
{
    switch(pNMHDR->code)
    {
        case PSN_SETACTIVE:
            return OnSetActive(hDlg);

        case PSN_KILLACTIVE:
            return OnKillActive(hDlg);

        case TVN_BEGINLABELEDIT:
            return OnBeginLabelEdit(hDlg, (LPNMTVDISPINFO)pNMHDR);

        case TVN_ENDLABELEDIT:
            return OnEndLabelEdit(hDlg, (LPNMTVDISPINFO)pNMHDR);

        case TVN_KEYDOWN:
            return OnTVKeyDown_CopyToClipboard(hDlg, (LPNMTVKEYDOWN)pNMHDR);

        case NM_DBLCLK:
            return OnDoubleClick(hDlg, pNMHDR);
    }
    return FALSE;
}

static void OnDestroy(HWND hDlg)
{
    TFileTestData * pData = GetDialogData(hDlg);

    if(pData != NULL)
    {
        if(pAnchors != NULL)
            delete pAnchors;
        pAnchors = NULL;

        if(pData->ReparseData != NULL)
            HeapFree(g_hHeap, 0, pData->ReparseData);
        pData->ReparseData = NULL;
    }
}

//-----------------------------------------------------------------------------
// Public functions

INT_PTR CALLBACK PageProc10(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog(hDlg, lParam);

        case WM_SIZE:
        case WM_GETMINMAXINFO:
            return pAnchors->OnMessage(uMsg, wParam, lParam);

        case WM_SHOW_HARDLINKS:
            OnShowHardlinks(hDlg);
            return FALSE;

        case WM_UPDATE_VIEW:
            OnUpdateView(hDlg);
            return TRUE;        

        case WM_COMMAND:
            return OnCommand(hDlg, HIWORD(wParam), LOWORD(wParam));

        case WM_NOTIFY:
            return OnNotify(hDlg, (NMHDR *)lParam);

        case WM_DESTROY:
            OnDestroy(hDlg);
            return FALSE;
    }
    return FALSE;
}
