// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "DevicesManager.h"

CMainDlg::CMainDlg()
{
	ZeroMemory(&notifyIconData, sizeof notifyIconData);
	m_hDialogBrush = CreateSolidBrush(RGB(255, 255, 255));
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	
	ZeroMemory(&notifyIconData, sizeof notifyIconData);
	// center the dialog on the screen
	CenterWindow();




	// set icons
	const PAUDIODEVICE defaultDevice=devicesManager->GetDefaultDevice();
	if(defaultDevice)
	{
		SetIcon(defaultDevice->largeIcon, TRUE);
		SetIcon(defaultDevice->smallIcon, FALSE);
	}
	else
	{
		::MessageBox(NULL,L"Cannot determine the default device",L"Error",MB_OK|MB_ICONERROR);
		return FALSE;
	}

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	
	listView.Attach(GetDlgItem(IDC_DEVICES_LIST));
	listView.SetExtendedListViewStyle(listView.GetExtendedListViewStyle()|LVS_EX_CHECKBOXES|
		LVS_EX_AUTOSIZECOLUMNS|LVS_EX_DOUBLEBUFFER);	
	listView.AddColumn(L"Device",0,0);
	const std::vector<AUDIODEVICE>* audioDevices=devicesManager->GetAudioDevices();
	CImageList imgList;
	imgList.Create(16,16,ILC_COLOR32|ILC_ORIGINALSIZE|ILC_MASK,(int)audioDevices->size(),1);
	for(auto it=audioDevices->cbegin();it!=audioDevices->cend();++it)
	{
		imgList.AddIcon((*it).largeIcon);
	}

	listView.SetImageList(imgList,LVSIL_SMALL);
	DWORD lpcValues=0,lpcMaxValueNameLen=0;
	LONG regOpResult=::RegQueryInfoKey(deviceSettingsKey,NULL,NULL,NULL,NULL,NULL,NULL,&lpcValues,&lpcMaxValueNameLen,NULL,NULL,NULL);	
	WCHAR *keyName=new WCHAR[lpcMaxValueNameLen+1];
	const std::vector<AUDIODEVICE> *devices=devicesManager->GetAudioDevices();
	int count=0;
	popupMenu.LoadMenu(IDR_POPUP_MENU);
	CMenuHandle menu = popupMenu.GetSubMenu(0);
	menu.SetMenuDefaultItem(ID_POPUPMENU_OPTIONS,FALSE);
	for(auto it=devices->cbegin();it!=devices->cend();++it)
	{
		listView.AddItem(count,0,(*it).deviceName,count);
		listView.SetItemData(count,(DWORD_PTR)&(*it));
		listView.SetColumnWidth(count,LVSCW_AUTOSIZE);
		DWORD keyType;
		regOpResult=::RegQueryValueEx(deviceSettingsKey,(*it).deviceId,NULL,&keyType,NULL,NULL);
		if(regOpResult==ERROR_SUCCESS)
		{
			listView.SetCheckState(count,TRUE);
		}
		size_t menuItemLen=wcslen((*it).deviceName)+10;
		WCHAR *menuName=new WCHAR[menuItemLen];
		swprintf(menuName,menuItemLen,L"&%d %s",count+1,(*it).deviceName);
		menu.AppendMenu(MF_ENABLED|MF_STRING,WM_USER+count+1,menuName);
		delete[] menuName;
		count++;
	}

	delete[] keyName;

	
//	this->SetWindowLong();
	return bHandled=FALSE;
}
LRESULT CMainDlg::OnSpecificDeviceSelected(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled)
{
	WORD index=wID-WM_USER-1;
	const std::vector<AUDIODEVICE> *devices=devicesManager->GetAudioDevices();
	if(index<0 || index>=devices->size())
	{
		bHandled=FALSE;
		return 0;
	}
	AUDIODEVICE device=devices->at(index);
	devicesManager->SetDefaultDevice(device.deviceId);
	UpdateApplicationIcon();
	bHandled=TRUE;
	return 0;
}

HBRUSH CMainDlg::OnCtlColorDlg(CDCHandle dc, CWindow wnd)
{
	return m_hDialogBrush;
}

HBRUSH CMainDlg::OnCtlColorStatic(CDCHandle dc, CStatic wndStatic)
{
	return m_hDialogBrush;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(notifyIconData.cbSize)
    {
		Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
        ZeroMemory(&notifyIconData, sizeof notifyIconData);
    }


	DeleteObject(m_hDialogBrush);

	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);


	return 0;
}
void CMainDlg::SetDevicesManager(CDevicesManager* devicesManager)
{
	this->devicesManager=devicesManager;
}

BOOL CMainDlg::ShowTrayIcon()
{
	if(!notifyIconData.cbSize)
	{
		notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
		notifyIconData.hWnd = m_hWnd;
		notifyIconData.uID = 1;
		notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		notifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;
		const PAUDIODEVICE defaultDevice=devicesManager->GetDefaultDevice();
		if(defaultDevice)
		{
			//apparently the icon that appears usually on the desktop is the large one
			//the small one seems to be somewhat different (maybe it shows up under different circumstances)
			notifyIconData.hIcon = defaultDevice->largeIcon;
		}
		//AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
		TCHAR sWindowText[MAX_PATH];
		GetWindowText(sWindowText,MAX_PATH);
		_tcscpy_s(notifyIconData.szTip, sWindowText);
		return Shell_NotifyIcon(NIM_ADD, &notifyIconData);
	}
	else
	{
		return Shell_NotifyIcon(NIM_MODIFY, &notifyIconData);
	}
	
}

LRESULT CMainDlg::OnSysCommand(UINT nCommand, CPoint point)
{
	switch(nCommand)
    {
    case SC_MINIMIZE:
        ShowWindow(SW_HIDE);
        break;        
	default:
		SetMsgHandled(FALSE);
    }
    return 0;
}

LRESULT CMainDlg::OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam)
{
    ATLASSERT(wParam == 1);
    switch(lParam)
    {
    case WM_LBUTTONDBLCLK:
        SendMessage(WM_COMMAND, ID_POPUPMENU_OPTIONS);
        break;
    case WM_RBUTTONUP:
        {
            SetForegroundWindow(m_hWnd);
			CMenuHandle menu = popupMenu.GetSubMenu(0);		
			CPoint Position;
            ATLVERIFY(GetCursorPos(&Position));
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, Position.x, Position.y, m_hWnd);
        }
        break;
    }
	return 0;
}

LRESULT CMainDlg::OnScRestore(UINT, INT, HWND)
{
    ShowWindow(SW_SHOW);
    BringWindowToTop();
    return 0;
}
LRESULT CMainDlg::OnScClose(UINT, INT, HWND)
{
    PostMessage(WM_COMMAND, IDCLOSE);
    return 0;
}
LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnBtnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}


LRESULT CMainDlg::OnBnClickedHideButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);
	return 0;
}


LRESULT CMainDlg::OnBnClickedSwitchButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{	
	std::vector<LPWSTR> ids;
	int count=listView.GetItemCount();
	for(int i=0;i<count;i++)
	{
		if(listView.GetCheckState(i))
		{
			PAUDIODEVICE device=(PAUDIODEVICE)listView.GetItemData(i);
			LPWSTR id=device->deviceId;
			ids.push_back(id);
		}
	}
	HRESULT hr=devicesManager->SwitchDevices(&ids);
	if(SUCCEEDED(hr))
	{
		UpdateApplicationIcon();
	}
	return 0;
}

void CMainDlg::UpdateApplicationIcon()
{
	const PAUDIODEVICE defaultDevice=devicesManager->GetDefaultDevice();
	if(defaultDevice)
	{
		SetIcon(defaultDevice->largeIcon, TRUE);
		SetIcon(defaultDevice->smallIcon, FALSE);
		notifyIconData.hIcon = defaultDevice->largeIcon;
		ShowTrayIcon();
	}
}

void CMainDlg::SetDeviceSettingsKey(HKEY key)
{
	deviceSettingsKey=key;
}

LRESULT CMainDlg::OnItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
	LPNMLISTVIEW pNMListView=(LPNMLISTVIEW)pnmh;
	if (pNMListView->uOldState == 0 && pNMListView->uNewState == 0)
	{
        return 0;    // No change
	}
	BOOL bPrevState = (BOOL)(((pNMListView->uOldState & 
                LVIS_STATEIMAGEMASK)>>12)-1);  
    if (bPrevState < 0)    // On startup there's no previous state 
	{
        bPrevState = 0; // so assign as false (unchecked)
	}
	// New check box state
    BOOL bChecked = 
            (BOOL)(((pNMListView->uNewState & LVIS_STATEIMAGEMASK)>>12)-1);   
    if (bChecked < 0) // On non-checkbox notifications assume false
	{
        bChecked = 0; 
	}
	if (bPrevState == bChecked) // No change in check box
	{
        return 0;
	}

	PAUDIODEVICE device=(PAUDIODEVICE)listView.GetItemData(pNMListView->iItem);

	if(bChecked)
	{
		LONG result=::RegSetValueEx(deviceSettingsKey,device->deviceId,NULL,REG_NONE,NULL,0);
		if(result!=ERROR_SUCCESS)
		{
			::MessageBox(m_hWnd,L"Cannot save key in the registry",L"Error",MB_OK|MB_ICONERROR);
		}
	}
	else
	{
		LONG result=::RegDeleteKeyValue(deviceSettingsKey,NULL,device->deviceId);
		if(result!=ERROR_SUCCESS)
		{
			::MessageBox(m_hWnd,L"Cannot save key in the registry",L"Error",MB_OK|MB_ICONERROR);
		}
	}

	bHandled=true;
	return 0;
}