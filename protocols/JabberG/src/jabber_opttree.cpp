/*

Jabber Protocol Plugin for Miranda NG

Copyright (c) 2002-04  Santithorn Bunchua
Copyright (c) 2005-12  George Hazan
Copyright (c) 2007     Maxim Mluhov
Copyright (c) 2007     Victor Pavlychko
Copyright (c) 2012-18 Miranda NG team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "stdafx.h"
#include "jabber_opttree.h"

enum { IMG_GROUP, IMG_CHECK, IMG_NOCHECK, IMG_RCHECK, IMG_NORCHECK, IMG_GRPOPEN, IMG_GRPCLOSED };

CCtrlTreeOpts::CCtrlTreeOpts(CDlgBase* dlg, int ctrlId):
	CCtrlTreeView(dlg, ctrlId),
	m_options(5)
{
}

CCtrlTreeOpts::~CCtrlTreeOpts()
{
	for (auto &it : m_options)
		delete it;
}

void CCtrlTreeOpts::AddOption(wchar_t *szOption, CMOption<BYTE> &option)
{
	m_options.insert(new COptionsItem(szOption, option), m_options.getCount());
}

BOOL CCtrlTreeOpts::OnNotify(int idCtrl, NMHDR *pnmh)
{
	switch (pnmh->code) {
	case TVN_KEYDOWN:
		{
			LPNMTVKEYDOWN lpnmtvkd = (LPNMTVKEYDOWN)pnmh;
			HTREEITEM hti;
			if ((lpnmtvkd->wVKey == VK_SPACE) && (hti = GetSelection()))
				ProcessItemClick(hti);
		}
		break;

	case NM_CLICK:
		{
			TVHITTESTINFO hti;
			hti.pt.x = (short)LOWORD(GetMessagePos());
			hti.pt.y = (short)HIWORD(GetMessagePos());
			ScreenToClient(pnmh->hwndFrom, &hti.pt);
			if (HitTest(&hti))
				if (hti.flags & TVHT_ONITEMICON)
					ProcessItemClick(hti.hItem);
		}
		break;

	case TVN_ITEMEXPANDED:
		{
			LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)pnmh;
			TVITEM tvi;
			tvi.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			tvi.hItem = lpnmtv->itemNew.hItem;
			tvi.iImage = tvi.iSelectedImage =
				(lpnmtv->itemNew.state & TVIS_EXPANDED) ? IMG_GRPOPEN : IMG_GRPCLOSED;
			SendMessage(pnmh->hwndFrom, TVM_SETITEM, 0, (LPARAM)&tvi);
		}
		break;
	}

	return CCtrlTreeView::OnNotify(idCtrl, pnmh);
}

void CCtrlTreeOpts::OnInit()
{
	CCtrlTreeView::OnInit();

	wchar_t itemName[1024];
	HIMAGELIST hImgLst;

	SelectItem(nullptr);
	DeleteAllItems();

	hImgLst = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR | ILC_COLOR32 | ILC_MASK, 5, 1);
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_MIRANDA));
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_TICK));	// check on
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_NOTICK));	// check off
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_TICK));	// radio on
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_NOTICK));	// radio on
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_GROUPOPEN));
	ImageList_AddIcon_Icolib(hImgLst, Skin_LoadIcon(SKINICON_OTHER_GROUPSHUT));
	SetImageList(hImgLst, TVSIL_NORMAL);

	/* build options tree. based on code from IcoLib */
	for (auto &it : m_options) {
		wchar_t *sectionName;
		int sectionLevel = 0;

		HTREEITEM hSection = nullptr;
		mir_wstrcpy(itemName, it->m_szOptionName);
		sectionName = itemName;

		while (sectionName) {
			// allow multi-level tree
			wchar_t *pItemName = sectionName;
			HTREEITEM hItem;

			if (sectionName = wcschr(sectionName, '/')) {
				// one level deeper
				*sectionName = 0;
				sectionName++;
			}

			hItem = FindNamedItem(hSection, pItemName);
			if (!sectionName || !hItem) {
				if (!hItem) {
					TVINSERTSTRUCT tvis = {};
					tvis.hParent = hSection;
					tvis.hInsertAfter = TVI_LAST;//TVI_SORT;
					tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tvis.item.pszText = pItemName;
					tvis.item.state = tvis.item.stateMask = TVIS_EXPANDED;
					if (sectionName) {
						tvis.item.lParam = -1;
						tvis.item.state |= TVIS_BOLD;
						tvis.item.stateMask |= TVIS_BOLD;
						tvis.item.iImage = tvis.item.iSelectedImage = IMG_GRPOPEN;
					}
					else {
						tvis.item.lParam = m_options.indexOf(&it);

						BYTE val = *it->m_option;

						if (it->m_groupId == OPTTREE_CHECK)
							tvis.item.iImage = tvis.item.iSelectedImage = val ? IMG_CHECK : IMG_NOCHECK;
						else
							tvis.item.iImage = tvis.item.iSelectedImage = val ? IMG_RCHECK : IMG_NORCHECK;
					}
					hItem = InsertItem(&tvis);
					if (!sectionName)
						it->m_hItem = hItem;
				}
			}
			sectionLevel++;
			hSection = hItem;
		}
	}

	TranslateTree();
	ShowWindow(m_hwnd, SW_SHOW);
	SelectItem(FindNamedItem(nullptr, nullptr));
}

void CCtrlTreeOpts::OnDestroy()
{
	ImageList_Destroy(GetImageList(TVSIL_NORMAL));
}

void CCtrlTreeOpts::OnApply()
{
	CCtrlTreeView::OnApply();

	for (auto &it : m_options) {
		TVITEMEX tvi;
		GetItem(it->m_hItem, &tvi);
		*it->m_option = ((tvi.iImage == IMG_CHECK) || (tvi.iImage == IMG_RCHECK)) ? 1 : 0;
	}
}

void CCtrlTreeOpts::ProcessItemClick(HTREEITEM hti)
{
	TVITEMEX tvi;
	GetItem(hti, &tvi);
	switch (tvi.iImage) {
	case IMG_GRPOPEN:
		tvi.iImage = tvi.iSelectedImage = IMG_GRPCLOSED;
		Expand(tvi.hItem, TVE_COLLAPSE);
		break;

	case IMG_GRPCLOSED:
		tvi.iImage = tvi.iSelectedImage = IMG_GRPOPEN;
		Expand(tvi.hItem, TVE_EXPAND);
		break;

	case IMG_CHECK:
		tvi.iImage = tvi.iSelectedImage = IMG_NOCHECK;
		SendMessage(::GetParent(::GetParent(m_hwnd)), PSM_CHANGED, 0, 0);
		break;

	case IMG_NOCHECK:
		tvi.iImage = tvi.iSelectedImage = IMG_CHECK;
		SendMessage(::GetParent(::GetParent(m_hwnd)), PSM_CHANGED, 0, 0);
		break;

	case IMG_NORCHECK:
		for (auto &it : m_options) {
			if (it->m_groupId == m_options[tvi.lParam]->m_groupId) {
				TVITEMEX tvi_tmp;
				tvi_tmp.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvi_tmp.hItem = it->m_hItem;
				tvi_tmp.iImage = tvi_tmp.iSelectedImage = IMG_NORCHECK;
				SetItem(&tvi_tmp);
			}
		}
		tvi.iImage = tvi.iSelectedImage = IMG_RCHECK;
		SendMessage(::GetParent(::GetParent(m_hwnd)), PSM_CHANGED, 0, 0);
		break;
	}

	SetItem(&tvi);
}

CCtrlTreeOpts::COptionsItem::COptionsItem(wchar_t *szOption, CMOption<BYTE> &option) :
m_option(&option), m_groupId(OPTTREE_CHECK), m_hItem(nullptr)
{
	m_szOptionName = mir_wstrdup(szOption);
}

CCtrlTreeOpts::COptionsItem::~COptionsItem()
{
	mir_free(m_szOptionName);
}
