/*****************************************************************************/
/* TestFile.h                             Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Definitions for file access testing application                           */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 14.07.03  1.00  Lad  The first version of FileTest.h                      */
/*****************************************************************************/

#ifndef __FILETEST_H__
#define __FILETEST_H__

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#define __TEST_MODE__               // Uncomment this to activate test mode

#pragma warning(disable: 4091)      // warning C4091: 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#define OEMRESOURCE
#include <tchar.h>
#include <stdio.h>

#define WIN32_NO_STATUS 
#include <windows.h>
#undef WIN32_NO_STATUS 
#include <windowsx.h>
#include <ShlObj.h>
#include <winioctl.h>
#include <strsafe.h>
#include <sddl.h>

#include "ntstatus.h"
#include "ntdll.h"
#include "Utils.h"
#include "AceCondition.h"
#include "AceResource.h"
#include "TFastString.h"
#include "TFlagString.h"
#include "TAceHelper.h"
#include "TAnchors.h"
#include "TToolTip.h"
#include "TDataEditor.h"
#include "WinSDK.h"

#pragma warning(disable:4995)       // cstdio(49) : warning C4995: 'gets': name was marked as #pragma deprecated
#include <vector>

//-----------------------------------------------------------------------------
// Defines

#define IsHandleValid(h)   (h != NULL && h != INVALID_HANDLE_VALUE)
#define IsHandleInvalid(h) (h == NULL || h == INVALID_HANDLE_VALUE)

#define ALIGN_INT32(Address) ((ULONG) ((Address + 3) & ~3))
#define ALIGN_INT64(Address) ((ULONG) ((Address + 7) & ~7))
#define ALIGN_EX(x, a)       (((x) + (a)-1) & ~((a)-1))

#define MAX_NT_PATH                 32767           // Maximum path name length in NT is 32767
#define MAX_FILEID_PATH             0x24            // Maximum path name length of File ID string (C:\################ or C:\################################)
#define MAX_CONTEXT_MENUS           0x08            // Maximum supported number of context menus
#define MAX_ACL_LENGTH              0xFFF8          // The biggest ACL that can possibly exist

#define OSVER_WINDOWS_NT4           0x0400
#define OSVER_WINDOWS_2000          0x0500
#define OSVER_WINDOWS_XP            0x0501
#define OSVER_WINDOWS_2003          0x0502
#define OSVER_WINDOWS_VISTA         0x0600
#define OSVER_WINDOWS_SEVEN         0x0601
#define OSVER_WINDOWS_8             0x0602
#define OSVER_WINDOWS_8_1           0x0603          // Make sure you have proper manifest to see this
#define OSVER_WINDOWS_10            0x0A00          // Make sure you have proper manifest to see this

#define WM_SHOW_HARDLINKS           (WM_USER + 0x1000)
#define WM_TIMER_BLINK              (WM_USER + 0x1001)
#define WM_TIMER_TOOLTIP            (WM_USER + 0x1002)
#define WM_TIMER_CHECK_MOUSE        (WM_USER + 0x1003)
#define WM_APC                      (WM_USER + 0x1004)
#define WM_START_WORK               (WM_USER + 0x1005)
#define WM_WORK_COMPLETE            (WM_USER + 0x1006)
#define WM_UPDATE_VIEW              (WM_USER + 0x1007)
#define WM_DEFER_ITEM_TEXT          (WM_USER + 0x1008)  // WPARAM = hItem, LPARAM = LPTSTR
#define WM_DEFER_CHANGE_INT_FLAGS   (WM_USER + 0x1009)  // WPARAM = hItem
#define WM_DEFER_CHANGE_WHOLE_ACE   (WM_USER + 0x100A)  // WPARAM = hItem, LPARAM = pAceHelper
#define WM_DEFER_CHANGE_ACE_GUID    (WM_USER + 0x100B)  // WPARAM = GUID index, LPARAM: TRUE = CreateNew
#define WM_DEFER_CHANGE_ACE_CSA     (WM_USER + 0x100C)  // WPARAM = hItem, LPARAM = BOOL bShowFlagsDialog

#define STATUS_INVALID_DATA_FORMAT  0xC1110001
#define STATUS_AUTO_CALCULATED      0xC1110002
#define STATUS_CANNOT_EDIT_THIS     0xC1110003
#define STATUS_FILE_ID_CONVERSION   0xC1110004
#define STATUS_COPIED_TO_CLIPBOARD  0xC1110005

#define SEVERITY_PENDING            2

#define APC_TYPE_NONE               0
#define APC_TYPE_READ_WRITE         1
#define APC_TYPE_LOCK_UNLOCK        2
#define APC_TYPE_FSCTL              3
#define APC_TYPE_IOCTL              4

#define COPY_FILE_USE_READ_WRITE    0x01000000      // Manual file copy - use ReadFile + WriteFile
#define COPY_FILE_SKIP_IO_ERRORS    0x02000000      // On I/O errors, replace loaded data with zeros if failed
#define COPY_FILE_LOG_IO_ERRORS     0x04000000      // Log the I/O errors to FileTest.log
#define COPY_FILE_PER_SECTOR        0x08000000      // Copy sector-per-sector (512 bytes)

#ifndef SECTOR_SIZE
#define SECTOR_SIZE                 0x200           // Sector size for disk drives
#endif

#define INTEGRITY_LEVEL_NONE        0xFFFFFFFF
#define INVALID_PAGE_INDEX          0xFFFFFFFF
#define INVALID_ITEM_INDEX          (size_t)(-1)

#define FILETEST_DATA_MAGIC         0x54534554454C4946ULL

//-----------------------------------------------------------------------------
// Structures

// Common structure for context menus
struct TContextMenu
{
    LPCTSTR szMenuName;                     // Name of the menu (or ID)
    HMENU hMenu;                            // Pre-loaded HMENU
};

// Common structure for data blob 
struct TDataBlob
{
    DWORD SetLength(SIZE_T NewLength);
    void Free();

    LPBYTE pbData;                          // Pointer to the data
    SIZE_T cbData;                          // Current length of the data
    SIZE_T cbDataMax;                       // Maximum length of the data
};

// Common structure for APCs. Keep its size 8-byte aligned
struct TApcEntry
{
    // Common APC entry members
    IO_STATUS_BLOCK IoStatus;               // IO_STATUS_BLOCK for the entry
    LARGE_INTEGER ByteOffset;               // Starting offset of the operation
    OVERLAPPED Overlapped;                  // Overlapped structure for the Win32 APIs
    LIST_ENTRY Entry;                       // Links to other APC entries
    HANDLE hEvent;                          // When signalled, triggers this APC
    HWND hWndPage;                          // Page that initiated the APC
    ULONG ApcType;                          // Common member for determining type of the APC
    ULONG UserParam;                        // Any user-defined 32-bit value (e.g. FSCTL code)
    ULONG BufferLength;                     // Length of data buffer (following the TApcEntry structure)
    ULONG bAsynchronousCompletion:1;        // If truem the request returned pending status and will be completed asynchronously
    ULONG bIncrementPosition:1;             // If true, the file position will be incremented when complete
    ULONG bHasIoStatus:1;                   // If true, the IO_STATUS_BLOCK is valid (otherwise it's OVERLAPED)
};

struct TOpenPacket
{
    TOpenPacket()
    {
        memset(this, 0, sizeof(TOpenPacket));
    }

    ~TOpenPacket()
    {
        Free();
    }

    void Free()
    {
        if(pvFileEa != NULL)
            HeapFree(g_hHeap, 0, pvFileEa);
        pvFileEa = NULL;
    }

    LARGE_INTEGER AllocationSize;
    PFILE_FULL_EA_INFORMATION pvFileEa;
    LPCSTR szId;                            // Const string with an ID, such as "RelativeFile" or "MainFile"
    ULONG dwDesiredAccess;                  // Desired access
    ULONG dwOA_Attributes;                  // OBJECT_ATTRIBUTES::Attributes
    ULONG dwFlagsAndAttributes;
    ULONG dwShareAccess;
    ULONG dwCreateDisposition1;             // For CreateFile
    ULONG dwCreateDisposition2;             // For NtCreateFile
    ULONG dwCreateOptions;
    ULONG cbFileEa;                         // Length of data in pvFileEa
};

// Structure used by main dialog to hold all its data
struct TWindowData
{
    HWND hDlg;                              // Handle to ourself
    HWND hWndPage;                          // HWND of the current page

    CRITICAL_SECTION ApcLock;               // A critical section protecting APC data
    LIST_ENTRY ApcList;                     // List of queued APC calls
    HANDLE hApcThread;                      // Handle to a thread sending messages
    HANDLE hAlertEvent;                     // Handle to a watcher event
    DWORD dwAlertReason;                    // A reason why the watcher was stopped
    int nApcCount;                          // Number of queued APCs

    UINT_PTR CheckMouseTimer;               // Timer for checking mouse
    RECT ScreenRect;                        // Size of the screen
    RECT DialogRect;                        // Size of the dialog
    bool bDialogBiggerThanScreen;           // true = the main dialog is bigger than the screen

    HANDLE hThread;                         // Thread that moves the dialog
    int nStartX;                            // The dialog's starting X position
    int nStartY;                            // The dialog's starting Y position
    int nEndY;                              // The dialog's final Y position
    int nAddY;                              // The dialog's movement direction
};

struct TFileTestData : public TWindowData
{
    ULONGLONG     MagicHeader;              // FILETEST_DATA_MAGIC ('FILETEST')
    LPTSTR        szDirName;                // Directory name
    LPTSTR        szFileName1;              // First file name
    LPTSTR        szFileName2;              // Second file name
    LPTSTR        szTemplate;               // Template file for CreateFileW
    LPTSTR        szSectionName;            // Section name for NtCreateSection
    HANDLE        hTransaction;             // Handle to the current transaction (NULL if none)
    HANDLE        hDirectory;               // Handle to the open directory
    HANDLE        hFile;                    // Handle to the open file
    HANDLE        hSection;                 // Section handle
    HANDLE        hSymLink;                 // Handle to the symbolic link

    TOpenPacket * pOP;                      // Current open packed (for LoadDialog / SaveDialog)
    TOpenPacket   RelaFile;                 // Set of flags for NtCreateFile/CreateFileW for the relative file
    TOpenPacket   OpenFile;                 // Set of flags for NtCreateFile/CreateFileW
    ULONG         IsDefaultFileName1:1;     // TRUE: The file name was created as default
    ULONG         UseRelativeFile:1;        // TRUE: Uset the relative file name (even if the name is empty)

    LARGE_INTEGER SectionOffset;            // Section Offset for NtCreateSection
    LARGE_INTEGER MaximumSize;              // Maximum size of section for NtCreateSection
    PVOID         pvSectionMappedView;      // BaseAddress
    ULONG         dwSectDesiredAccess;      // DesiredAccess
    ULONG         dwSectPageProtection;     // PageProtection
    ULONG         dwSectAllocAttributes;    // AllocationAttributes
    size_t        cbSectCommitSize;
    size_t        cbSectViewSize;
    ULONG         dwSectAllocType;          // AllocationType for NtMapViewOfSection
    ULONG         dwSectWin32Protect;       // Win32Protect for NtMapViewOfSection
    UINT          FillPattern;              // Fill data pattern

    ULONG         dwCopyFileFlags;          // For file copying
    ULONG         dwMoveFileFlags;          // For MoveFileEx
    ULONG         dwOplockLevel;            // For requesting Win7 oplock
    BOOL          bTransactionActive;
    BOOL          bUseTransaction;
    BOOL          bSectionViewMapped;

    ULONG         SecurityInformation;      // Combination of security information flags
    ULONG         InitialPage;              // Initial page for the main dialog

    UINT_PTR      BlinkTimer;               // If nonzero, this is ID of the blink timer
    HWND          hWndBlink;                // It not NULL, this is the handle of blink window
    BOOL          bEnableResizing;          // TRUE if the dialog is allowed to be resized

    TDataBlob     RdWrData;                 // Buffer for ReadFile / WriteFile
    TDataBlob     NtInfoData;               // Buffer for NtQueryInformationFile/NtSetInformationFile
    TDataBlob     InData;                   // Input buffer for DeviceIoControlFile / NtDeviceIoControlFile / NtfsControlFile
    TDataBlob     OutData;                  // Output buffer for DeviceIoControlFile / NtDeviceIoControlFile / NtfsControlFile

    PREPARSE_DATA_BUFFER ReparseData;       // Buffer for reparse points
    ULONG         ReparseDataLength;        // Total length of reparse data buffer
    ULONG         ReparseDataValid;         // Available length of reparse data buffer

    TCHAR         szBuffer1[MAX_NT_PATH];   // A buffer for directory name
    TCHAR         szBuffer2[MAX_NT_PATH];   // A buffer for the first file name
    TCHAR         szBuffer3[MAX_NT_PATH];   // A buffer for the second file name
    TCHAR         szBuffer4[MAX_NT_PATH];   // A buffer for template file for CreateFileW
    TCHAR         szBuffer5[MAX_NT_PATH];   // A buffer for section name for NtCreateSection
};

#define GetDialogData(hDlg) ((TFileTestData *)GetWindowLongPtr(hDlg, DWLP_USER))
#define SetDialogData(hDlg,pData) SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pData)

//-----------------------------------------------------------------------------
// Data types and strctures for NtQueryInformationFile and NtSetInformationFile

#define TYPE_NONE             0
#define TYPE_BOOLEAN          1
#define TYPE_UINT8            2
#define TYPE_UINT16           3
#define TYPE_UINT32           4
#define TYPE_UINT64           5
#define TYPE_STRING           6
#define TYPE_WSTRING          7
#define TYPE_ARRAY8_FIXED     8             // Array of bytes, fixed length
#define TYPE_ARRAY8_VARIABLE  9             // Array of bytes, variable length
#define TYPE_HANDLE          10             // Handle
#define TYPE_POINTER         11             // pointer
#define TYPE_FILETIME        12             // FILETIME
#define TYPE_CNAME_L8B       13             // Array of CHARs, var length, length is 8-bit value in bytes
#define TYPE_WNAME_L32B      14             // Array of WCHARs, var length, length is 32-bit value in bytes
#define TYPE_WNAME_L32W      15             // Array of WCHARs, var length, length is 32-bit value in WCHARs
#define TYPE_VNAME_FBDI      16             // ShortName in FILE_BOTH_DIRECTORY_INFORMATION
#define TYPE_VNAME_FIBD      17             // ShortName in FILE_ID_BOTH_DIRECTORY_INFORMATION
#define TYPE_VNAME_FIEBD     18             // ShortName in FILE_ID_EXTD_BOTH_DIRECTORY_INFORMATION
#define TYPE_FILEID64        19             // 8-byte file ID
#define TYPE_FILEID128       20             // 16-byte file ID
#define TYPE_DIR_HANDLE      21             // Directory handle for certain file operations
#define TYPE_FLAG8           22             // An 8-bit flag value
#define TYPE_FLAG16          23             // A 16-bit flag value
#define TYPE_FLAG32          24             // A 32-bit flag value
#define TYPE_GUID            25             // A GUID value
#define TYPE_SID             26             // A SID value
#define TYPE_STRUCT         100             // Sub-structure, nMemberSize must be sizeof(structure) !!!
#define TYPE_CHAINED_STRUCT 101             // Chain of structures first 32-bit number is "NextEntryOffset"
#define TYPE_ARRAY_PROCESS  102             // Array of process IDs, variable length, length is 32-bit number
#define TYPE_PADDING       1000             // Padding for the next member. The "nMemberSize" contains alignment

#define MEMBER_SIZE_SPECIAL (UINT)-1        // For variable length data items

struct TStructMember
{
    LPCTSTR szMemberName;                   // Name of the member
    UINT    nDataType;                      // Data type
    UINT    nMemberSize;                    // Size (in bytes) of the structure member
    PBYTE   pbStructPtr;                    // Pointer to the begin of the structure
    union
    {
        TStructMember * pSubItems;          // Subitems, if this is structure too
        TFlagInfo * pFlags;                 // Flags, if this is a flag array
        PBYTE pbDataPtr;                    // If this describes data item, pointer to binary data
    };
};

struct TInfoData
{
    int                    InfoClass;       // Value for NtSetInformationFile
    LPCTSTR                szInfoClass;     // Text for the value
    LPCTSTR                szStructName;    // Name of the input/output structure 
    TStructMember        * pStructMembers;  // Description of the data structure
                                            // (NULL = not implemented)
    BOOL                   bIsChain;        // if TRUE, it is a chain of structures
                                            // (with ULONG NextEntryOffset as first member)
    BOOL                   bIsEditable;     // If TRUE, the structure is editable
                                            // and able to send to NtSetInfo
};

#define FILE_INFO_READONLY(classname, structname, ischain)   \
    {(int)classname, _T(#classname), _T(#structname), classname##Members, ischain, FALSE}

#define FILE_INFO_EDITABLE(classname, structname, ischain)   \
    {(int)classname, _T(#classname), _T(#structname), classname##Members, ischain, TRUE}

#define FILE_INFO_NOTIMPL(classname, structname, ischain)   \
    {(int)classname, _T(#classname), NULL, NULL, FALSE, FALSE}

//-----------------------------------------------------------------------------
// Prototypes for transaction APIs

typedef HANDLE (WINAPI * CREATETRANSACTION)(
    LPSECURITY_ATTRIBUTES lpTransactionAttributes,
    LPGUID TransactionGuid,
    DWORD CreateOptions,
    DWORD IsolationLevel,
    DWORD IsolationFlags,
    DWORD dwMilliseconds,
    LPWSTR Description
    );

typedef BOOL (WINAPI * SETCURRENTTRANSACTION)(
    HANDLE hTransaction
    );
    
typedef BOOL (WINAPI * COMMITTRANSACTION)(
    HANDLE hTransaction
    );

typedef BOOL (WINAPI * ROLLBACKTRANSACTION)(
    HANDLE hTransaction
    );

typedef HANDLE (WINAPI * RTLGETCURRENTTRANSACTION)(
    );

typedef VOID (WINAPI * RTLSETCURRENTTRANSACTION)(
    HANDLE TransactionHandle
    );

typedef BOOL (WINAPI * CREATEDIRTRANSACTED)(
    LPCTSTR lpTemplateDirectory,
    LPCTSTR lpNewDirectory,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    HANDLE hTransaction
    );

typedef HANDLE (WINAPI * CREATEFILETRANSACTED)(
    LPCTSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile,
    HANDLE hTransaction,
    PUSHORT pusMiniVersion, 
    PVOID  lpExtendedParameter);

typedef BOOL (WINAPI * CREATEHARDLINK)(
    LPCTSTR lpFileName,
    LPCTSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes);

//-----------------------------------------------------------------------------
// Global variables

extern TContextMenu g_ContextMenus[MAX_CONTEXT_MENUS];
extern TToolTip g_Tooltip;
extern LPCTSTR HexaAlphabetUpper;
extern DWORD g_dwWinVer;
extern DWORD g_dwWinBuild;
extern TCHAR g_szInitialDirectory[MAX_PATH];

extern RTLGETCURRENTTRANSACTION pfnRtlGetCurrentTransaction;
extern RTLSETCURRENTTRANSACTION pfnRtlSetCurrentTransaction;
extern CREATETRANSACTION        pfnCreateTransaction;
extern COMMITTRANSACTION        pfnCommitTransaction;
extern ROLLBACKTRANSACTION      pfnRollbackTransaction;
extern CREATEDIRTRANSACTED      pfnCreateDirectoryTransacted;
extern CREATEFILETRANSACTED     pfnCreateFileTransacted;
extern CREATEHARDLINK           pfnCreateHardLink;
extern ADDMANDATORYACE          pfnAddMandatoryAce;

//-----------------------------------------------------------------------------
// Flag values, global to the entire project

extern TFlagInfo AccessMaskValues[];
extern TFlagInfo FileAttributesValues[];
extern TFlagInfo CreateOptionsValues[];
extern TFlagInfo ShareAccessValues[];

//-----------------------------------------------------------------------------
// NTSTATUS conversion

LPCTSTR NtStatus2Text(NTSTATUS Status);

//-----------------------------------------------------------------------------
// Utilities (in Utils.cpp)

DWORD StrToInt(LPCTSTR ptr, LPTSTR * szEnd, int nRadix);

LPCTSTR SkipHexaPrefix(LPCTSTR szString);
DWORD Text2Bool(LPCTSTR szText, bool * pValue);
DWORD Text2Hex32(LPCTSTR szText, PDWORD pValue);
DWORD DlgText2Hex32(HWND hDlg, UINT nIDCtrl, PDWORD pValue);
void  Hex2TextXX(ULONGLONG Value, LPTSTR szBuffer, size_t nSize);
void  Hex2Text32(LPTSTR szBuffer, DWORD Value);
void  Hex2DlgText32(HWND hDlg, UINT nIDCtrl, DWORD Value);

DWORD Text2HexPtr(LPCTSTR szText, PDWORD_PTR pValue);
DWORD DlgText2HexPtr(HWND hDlg, UINT nIDCtrl, PDWORD_PTR pValue);
void  Hex2TextPtr(LPTSTR szBuffer, DWORD_PTR Value);
void  Hex2DlgTextPtr(HWND hDlg, UINT nIDCtrl, DWORD_PTR Value);

DWORD Text2Hex64(LPCTSTR szText, PLONGLONG pValue);
DWORD DlgText2Hex64(HWND hDlg, UINT nIDCtrl, PLONGLONG pValue);
void Hex2Text64(LPTSTR szBuffer, LONGLONG Value);
void Hex2DlgText64(HWND hDlg, UINT nIDCtrl, LONGLONG Value);

LPTSTR FindDirectoryPathPart(LPTSTR szFullPath);
LPTSTR FindNextPathSeparator(LPTSTR szPathPart);

ULONG GetEaEntrySize(PFILE_FULL_EA_INFORMATION EaInfo);

void TreeView_SetItemText(HWND hWndTree, HTREEITEM hItem, LPCTSTR szText);
DWORD TreeView_GetChildCount(HWND hWndTree, HTREEITEM hItem);
LPARAM TreeView_GetItemParam(HWND hWndTree, HTREEITEM hItem);
void TreeView_SetItemParam(HWND hWndTree, HTREEITEM hItem, LPARAM lParam);
HTREEITEM TreeView_SetTreeItem(HWND hWndTree, HTREEITEM hItem, LPCTSTR szText, LPARAM lParam);
BOOL TreeView_EditLabel_ID(HWND hDlg, UINT nID);
HTREEITEM InsertTreeItem(HWND hWndTree, HTREEITEM hParent, HTREEITEM hInsertAfter, LPCTSTR szText, PVOID pParam);
HTREEITEM InsertTreeItem(HWND hWndTree, HTREEITEM hParent, LPCTSTR szText, PVOID pParam);
HTREEITEM InsertTreeItem(HWND hWndTree, HTREEITEM hParent, LPCTSTR szText, LPARAM lParam = 0);
HTREEITEM InsertTreeItem(HWND hWndTree, HTREEITEM hParent, LPARAM lParam, UINT nID, ...);

void TreeView_DeleteChildren(HWND hWndTree, HTREEITEM hParent);
void TreeView_CopyToClipboard(HWND hWndTree);
int OnTVKeyDown_CopyToClipboard(HWND hDlg, LPNMTVKEYDOWN pNMTVKeyDown);

HANDLE OpenCurrentToken(DWORD dwDesiredAccess);
BOOL GetTokenElevation(PBOOL pbElevated);
BOOL GetTokenVirtualizationEnabled(PBOOL pbEnabled);
BOOL SetTokenVirtualizationEnabled(BOOL bEnabled);

HWND AttachIconToEdit(HWND hDlg, HWND hWndChild, LPTSTR szIDIcon);

void ResolveDynamicLoadedAPIs();

BOOLEAN  IsNativeName(LPCWSTR szFileName);

NTSTATUS FileNameToUnicodeString(PUNICODE_STRING FileName, LPCTSTR szFileName);
void     FreeFileNameString(PUNICODE_STRING FileName);

NTSTATUS ConvertToNtName(HWND hDlg, UINT nIDEdit);
int      ConvertToWin32Name(HWND hDlg, UINT nIDEdit);

LPTSTR FlagsToString(TFlagInfo * pFlags, LPTSTR szBuffer, size_t cchBuffer, DWORD dwBitMask, bool bNewLineSeparated = false);
LPTSTR NamedValueToString(TFlagInfo * pFlags, LPTSTR szBuffer, size_t cchBuffer, LPCTSTR szFormat, DWORD dwBitMask);
LPTSTR NamedValueToString(TFlagInfo * pFlags, LPTSTR szBuffer, size_t cchBuffer, UINT nIDFormat, DWORD dwBitMask);
LPTSTR GuidValueToString(LPTSTR szBuffer, size_t cchBuffer, LPCTSTR szFormat, LPGUID PtrGuid);

void FileIDToString(TFileTestData * pData, ULONGLONG FileId, LPTSTR szBuffer);
void ObjectIDToString(PBYTE pbObjId, LPCTSTR szFileName, LPTSTR szObjectID);
DWORD StringToFileID(LPCTSTR szFileOrObjId, LPTSTR szVolume, PVOID pvFileObjId, PDWORD pLength);

void SidToString(PSID pvSid, LPTSTR szString, size_t cchString, bool bAddUserName);

HMENU FindContextMenu(UINT nIDMenu);
int ExecuteContextMenu(HWND hWndParent, HMENU hMenu, LPARAM lParam);
int ExecuteContextMenuForDlgItem(HWND hWndParent, HMENU hMenu, UINT nIDCtrl);

NTSTATUS NtDeleteReparsePoint(HANDLE ObjectHandle);
NTSTATUS NtDeleteReparsePoint(POBJECT_ATTRIBUTES PtrObjectAttributes);

ULONG RtlComputeCrc32(ULONG InitialCrc, PVOID Buffer, ULONG Length);

//BOOL WINAPI MyAddMandatoryAce(PACL pAcl, DWORD dwAceRevision, DWORD dwAceFlags, DWORD MandatoryPolicy, PSID pLabelSid);

void EnableRedraw(HWND hWnd, BOOL bEnableRedraw = TRUE);

//-----------------------------------------------------------------------------
// Common function to set result of an operation

// Supported flags
#define RSI_LAST_ERROR  0x00000001              // IDC_ERROR_CODE  -> DWORD dwErrCode (with blinking icon)
#define RSI_NTSTATUS    0x00000002              // IDC_ERROR_CODE  -> NTSTATUS Status (with blinking icon)
#define RSI_HANDLE      0x00000004              // IDC_HANDLE      -> HANDLE hHandle;
#define RSI_NOINFO      0x00000008              // IDC_INFORMATION -> Set empty
#define RSI_INFORMATION 0x00000010              // IDC_INFORMATION -> PIO_STATUS_BLOCK IoStatus
#define RSI_INFO_INT32  0x00000020              // IDC_INFORMATION -> DWORD Information
#define RSI_NTCREATE    0x00000040              // IDC_INFORMATION -> PIO_STATUS_BLOCK IoStatus
#define RSI_READ        0x00000080              // IDC_INFORMATION -> DWORD BytesRead
#define RSI_WRITTEN     0x00000100              // IDC_INFORMATION -> DWORD BytesWritten
#define RSI_FILESIZE    0x00000200              // IDC_INFORMATION -> PLARGE_INTEGER FileSize
#define RSI_FILEPOS     0x00000400              // IDC_INFORMATION -> PLARGE_INTEGER FilePos

void SetResultInfo(HWND hDlg, DWORD dwFlags, ...);

//-----------------------------------------------------------------------------
// Conversion of FILETIME to text and back

LPTSTR FileTimeToText(
    LPTSTR szBuffer,
    LPTSTR szEndChar,
    PFILETIME pFt,
    BOOL bTextForEdit);

NTSTATUS TextToFileTime(
    LPCTSTR szText,
    PFILETIME pFt);

BOOL GetSupportedDateTimeFormats(
    LPCTSTR szDateFormatPrefix,
    LPCTSTR szTimeFormatPrefix,
    LPTSTR szBuffer,
    int nMaxChars);

//-----------------------------------------------------------------------------
// Dialogs

INT_PTR HelpAboutDialog(HWND hWndParent);
INT_PTR HelpCommandLineDialog(HWND hWndParent);
INT_PTR FlagsDialog(HWND hWndParent, UINT nIDTitle, TFlagInfo * pFlags, DWORD & dwBitMask);
INT_PTR FlagsDialog_OnControl(HWND hWndParent, UINT nIDTitle, TFlagInfo * pFlags, UINT nIDCtrl);

INT_PTR EaEditorDialog(HWND hWndParent, PFILE_FULL_EA_INFORMATION * pEaInfo);
INT_PTR PrivilegesDialog(HWND hWndParent);
INT_PTR ObjectIDActionDialog(HWND hWndParent);
INT_PTR ObjectGuidHelpDialog(HWND hWndParent);
INT_PTR FileAttributesDialog(HWND hWndParent, PFILE_BASIC_INFORMATION pBasicInfo);
INT_PTR NtAttributesDialog(HWND hWndParent, PFILE_BASIC_INFORMATION pBasicInfo);
INT_PTR CopyFileDialog(HWND hWndParent, TFileTestData * pData);

TApcEntry * CreateApcEntry(TWindowData * pData, UINT ApcType, size_t cbExtraSize = 0);
bool InsertApcEntry(TWindowData * pData, TApcEntry * pApc);
void FreeApcEntry(TApcEntry * pApc);

int NtUseFileId(HWND hDlg, LPCTSTR szFileId);
void DisableCloseDialog(HWND hDlg, BOOL bDisable);
INT_PTR FileTestDialog(HWND hWndParent, TFileTestData * pData);

//-----------------------------------------------------------------------------
// Extended attributes dialog (shared functions)

void ExtendedAttributesToListView(HWND hDlg, PFILE_FULL_EA_INFORMATION pFileEa);
DWORD ListViewToExtendedAttributes(HWND hDlg, TOpenPacket & OpenPacket);
INT_PTR CALLBACK ExtendedAttributesEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR ExtendedAtributesEditorDialog(HWND hWndParent, TOpenPacket * pOP);
INT_PTR DataEditorDialog(HWND hWndParent, LPVOID BaseAddress, size_t ViewSize);

//-----------------------------------------------------------------------------
// Message handlers for each page

INT_PTR CALLBACK PageProc00(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc01(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc02(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc03(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc04(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc05(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc06(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc08(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc09(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc10(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc11(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PageProc12(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// Debugging functions

#ifdef __TEST_MODE__
void DebugCode_TEST();
void DebugCode_SecurityDescriptor(LPCTSTR szPath);
#endif  // __TEST_MODE__

#endif // __FILETEST_H__
