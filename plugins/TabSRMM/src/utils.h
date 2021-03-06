/////////////////////////////////////////////////////////////////////////////////////////
// Miranda NG: the free IM client for Microsoft* Windows*
//
// Copyright (c) 2012-18 Miranda NG team,
// Copyright (c) 2000-09 Miranda ICQ/IM project,
// all portions of this codebase are copyrighted to the people
// listed in contributors.txt.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// you should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// part of tabSRMM messaging plugin for Miranda.
//
// (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
//
// utility functions for TabSRMM

#ifndef __UTILS_H
#define __UTILS_H

#define RTF_CTABLE_DEFSIZE 8

#define RTF_DEFAULT_HEADER L"{\\rtf1\\ansi\\deff0\\pard\\li%u\\fi-%u\\ri%u\\tx%u"

#define CNT_KEYNAME "CNTW_Def"
#define CNT_BASEKEYNAME "CNTW_"

struct TRTFColorTable
{
	__forceinline TRTFColorTable(const wchar_t *wszName, COLORREF _clr) :
		clr(_clr)
	{
		mir_wstrncpy(szName, wszName, _countof(szName));
	}

	wchar_t  szName[10];
	COLORREF clr;
};

class Utils {

public:
	static wchar_t* GetPreviewWithEllipsis(wchar_t *szText, size_t iMaxLen);
	static wchar_t* FilterEventMarkers(wchar_t *wszText);
	static char*    FilterEventMarkers(char *szText);
	static void     DoubleAmpersands(wchar_t *pszText, size_t len);
	static void     RTF_CTableInit();
	static void     RTF_ColorAdd(const wchar_t *tszColname);
	static int      ReadContainerSettingsFromDB(const MCONTACT hContact, TContainerSettings *cs, const char *szKey = nullptr);
	static int      WriteContainerSettingsToDB(const MCONTACT hContact, TContainerSettings *cs, const char *szKey = nullptr);
	static void     SettingsToContainer(TContainerData *pContainer);
	static void     ContainerToSettings(TContainerData *pContainer);
	static void     ReadPrivateContainerSettings(TContainerData *pContainer, bool fForce = false);
	static void     SaveContainerSettings(TContainerData *pContainer, const char *szSetting);

	static DWORD    CALLBACK StreamOut(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG * pcb);

	static void     addMenuItem(const HMENU& m, MENUITEMINFO& mii, HICON hIcon, const wchar_t *szText, UINT uID, UINT pos);
	static void     enableDlgControl(const HWND hwnd, UINT id, bool fEnable = true);
	static void     showDlgControl(const HWND hwnd, UINT id, int showCmd);
	static void     setAvatarContact(HWND hWnd, MCONTACT hContact);
	static void     getIconSize(HICON hIcon, int& sizeX, int& sizeY);

	static bool     extractResource(const HMODULE h, const UINT uID, const wchar_t *tszName, const wchar_t *tszPath, const wchar_t *tszFilename, bool fForceOverwrite);
	static void     scaleAvatarHeightLimited(const HBITMAP hBm, double& dNewWidth, double& dNewHeight, const LONG maxHeight);

	static AVATARCACHEENTRY*  loadAvatarFromAVS(const MCONTACT hContact);

	static void     sanitizeFilename(wchar_t *tszFilename);
	static void     ensureTralingBackslash(wchar_t *szPathname);

	static void     sendContactMessage(MCONTACT hContact, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static HMODULE  loadSystemLibrary(const wchar_t* szFilename);

	static LRESULT  CALLBACK PopupDlgProcError(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LPTSTR   extractURLFromRichEdit(const ENLINK* _e, const HWND hwndRich);

	static size_t   CopyToClipBoard(const wchar_t *str, const HWND hwndOwner);

	static void     AddToFileList(wchar_t ***pppFiles, int *totalCount, LPCTSTR szFilename);

	//////////////////////////////////////////////////////////////////////////////////////
	// safe mir_strlen function - do not overflow the given buffer length
	// if the buffer does not contain a valid (zero-terminated) string, it
	// will return 0.
	//
	// careful: maxlen must be given in element counts!!

	template<typename T> static size_t safe_strlen(const T* src, const size_t maxlen = 0)
	{
		size_t s = 0;

		while (s < maxlen && *(src++))
			s++;

		return (s >= maxlen && *src != 0) ? 0 : s;
	}

public:
	static OBJLIST<TRTFColorTable> rtf_clrs;
};

__forceinline LRESULT _dlgReturn(HWND hWnd, LRESULT result)
{
	SetWindowLongPtr(hWnd, DWLP_MSGRESULT, result);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// implements a warning dialog with a "do not show this again" check
// box

class CWarning {

public:
	/*
	 * the warning IDs
	 */
	enum {
		WARN_RELNOTES = 0,
		WARN_ICONPACK_VERSION = 1,
		WARN_EDITUSERNOTES = 2,
		WARN_ICONPACKMISSING = 3,
		WARN_AEROPEEK_SKIN = 4,
		WARN_SENDFILE = 5,
		WARN_HPP_APICHECK = 6,
		WARN_NO_SENDLATER = 7,
		WARN_CLOSEWINDOW = 8,
		WARN_OPTION_CLOSE = 9,
		WARN_THEME_OVERWRITE = 10,
		WARN_LAST = 11
	};

	// the flags(low word is reserved for default windows flags like MB_OK etc.
	enum {
		CWF_UNTRANSLATED = 0x00010000, // do not translate the msg (useful for some error messages)
		CWF_NOALLOWHIDE = 0x00020000  // critical message, hide the "do not show this again" check box
	};

	CWarning(const wchar_t* tszTitle, const wchar_t* tszText, const UINT uId, const DWORD dwFlags);
	~CWarning();

public:
	// static function to construct and show the dialog, returns the user's choice
	static LRESULT show(const int uId, DWORD dwFlags = 0, const wchar_t* tszTxt = nullptr);
	static void destroyAll();
	LRESULT ShowDialog() const;

private:
	ptrW  m_szTitle, m_szText;
	UINT  m_uId;
	HFONT m_hFontCaption;
	DWORD m_dwFlags;
	HWND  m_hwnd;
	bool  m_fIsModal;

	INT_PTR CALLBACK dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK	stubDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static __int64 getMask(); // get bit mask for disabled message classes

private:
	static MWindowList hWindowList;
};

#endif /* __UTILS_H */
