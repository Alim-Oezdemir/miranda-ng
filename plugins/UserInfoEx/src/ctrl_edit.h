/*
UserinfoEx plugin for Miranda IM

Copyright:
© 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _UI_CTRL_EDIT_INCLUDE_
#define _UI_CTRL_EDIT_INCLUDE_

/**
 *
 **/
class CEditCtrl : public CBaseCtrl
{
	BYTE	_dbType;
	
	/**
	 * Private constructure is to force to use static member 'Create' 
	 * as the default way of attaching a new object to the window control.
	 *
	 * @param	 hCtrl		 - the window handle of the control to 
	 *											associate this class's instance with
	 * @param	 pszSetting - the database setting to be handled by this control
	 *
	 * @return	nothing
	 **/
	CEditCtrl(HWND hDlg, WORD idCtrl, LPCSTR pszModule, LPCSTR pszSetting);

public:

	/**
	 *
	 *
	 **/
	static FORCEINLINE CEditCtrl* GetObj(HWND hCtrl) 
		{ return (CEditCtrl*) GetUserData(hCtrl); }
	static FORCEINLINE CEditCtrl* GetObj(HWND hDlg, WORD idCtrl)
		{ return GetObj(GetDlgItem(hDlg, idCtrl)); }

	static CBaseCtrl* CreateObj(HWND hDlg, WORD idCtrl, LPCSTR pszSetting, BYTE dbType);
	static CBaseCtrl* CreateObj(HWND hDlg, WORD idCtrl, LPCSTR pszModule, LPCSTR pszSetting, BYTE dbType);

	virtual void	Release();
	virtual void	OnReset();
	virtual BOOL	OnInfoChanged(MCONTACT hContact, LPCSTR pszProto);
	virtual void	OnApply(MCONTACT hContact, LPCSTR pszProto);
	virtual void	OnChangedByUser(WORD wChangedMsg);

	void		OpenUrl();
	LRESULT LinkNotificationHandler(ENLINK* lnk);
	
};

#endif /* _UI_CTRL_EDIT_INCLUDE_ */