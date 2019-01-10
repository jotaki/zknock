#include <stdio.h>	// vsnprintf

#include <windows.h>
#include <winuser.h>

#include "zknock.h"
#include "resource.h"

static struct runtimecfg g_runtimecfg = { NULL, 0 };

void ReportError(HWND hwnd, DWORD error)
{

	char buf[512] = "";
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 511, NULL);

	MessageBox(hwnd, buf, "Error!", MB_OK);
}

LRESULT CALLBACK ZknockProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_CREATE:
			CreateZknockWindow(hwnd, msg, wParam, lParam);
			break;

		case WM_COMMAND:
		{
			HandleZknockCommand(hwnd, msg, wParam, lParam);

			if(g_runtimecfg.modified)
				SetWindowText(hwnd, "Zknock " ZKNOCK_VERSION "*");
			else
				SetWindowText(hwnd, "Zknock " ZKNOCK_VERSION);
		}
		break;

		case WM_NOTIFY:
			switch(LOWORD(wParam)) {
				case IDC_PROFILES:
					HandleZknockProfile(hwnd, (LPNMHDR) lParam);
					break;
			}

			if(((LPNMHDR) lParam)->code == TTN_NEEDTEXT) {
				ZknockTooltips(hwnd, (LPTOOLTIPTEXT) lParam);
			}

			break;


		case WM_SIZE:
			ResizeZknockWindow(hwnd, msg, wParam, lParam);
			break;

		case WM_CLOSE:
		{
			if(g_runtimecfg.systray.enabled) {
				ShowWindow(hwnd, SW_HIDE);
				break;
			}

			if(!CheckQuit(hwnd))
				break;

			DestroyWindow(hwnd);
		}
		break;

		case IDM_SYSTRAY:
			switch(lParam) {
				case WM_LBUTTONUP:
					if(IsWindowVisible(hwnd)) {
						ShowWindow(hwnd, SW_HIDE);
					} else {
						ShowWindow(hwnd, SW_RESTORE);
					}
					break;

				case WM_RBUTTONDOWN:
				case WM_CONTEXTMENU:
					SystrayContextMenu(hwnd);
					break;
			}
			break;

		case WM_DESTROY:
			SetSystemTray(hwnd, 0);
			ClearProfiles(GetDlgItem(hwnd, IDC_PROFILES));
			PostQuitMessage(0);
			break;

		case WM_MENUCOMMAND:
		{
			HWND list;
			int len;
			char buf[256];
			struct profilecfg *profile;

			list = GetDlgItem(hwnd, IDC_PROFILES);
			len = ListView_GetItemCount(list);
			if(len < wParam) PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_QUIT, 0), 0);
			else KnockProfile(hwnd, GetProfile(list, wParam));
		}
		break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void CreateZknockWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND toolbar, statusbar, profiles;

	toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS,
			0, 0, 0, 0, hwnd, (HMENU) IDC_TOOLBAR, GetModuleHandle(NULL),
			NULL);

	statusbar = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE |
			SBARS_SIZEGRIP, 0, 0, 0, 0, hwnd, (HMENU)IDC_STATUSBAR,
			GetModuleHandle(NULL), NULL);

	profiles = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_CHILD | WS_VISIBLE | LVS_ICON | LVS_AUTOARRANGE,
			0, 0, 0, 0, hwnd, (HMENU)IDC_PROFILES, GetModuleHandle(NULL), NULL);


	InitializeZknockMenu(GetMenu(hwnd));
	InitializeZknockToolbar(toolbar);
	InitializeZknockProfileList(profiles);

	SendMessage(statusbar, SB_SETTEXT, 0, (LPARAM) "zknock - a port knocker for windows");
}

void ResizeZknockWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND toolbar, statusbar, profiles;
	RECT tbrect, sbrect, client;
	int tbheight, sbheight, pheight;

	if(wParam == SIZE_MINIMIZED && g_runtimecfg.systray.enabled) {
		ShowWindow(hwnd, SW_HIDE);
		return;
	}

	toolbar = GetDlgItem(hwnd, IDC_TOOLBAR);
	SendMessage(toolbar, TB_AUTOSIZE, 0, 0);

	GetWindowRect(toolbar, &tbrect);
	tbheight = tbrect.bottom - tbrect.top;

	statusbar = GetDlgItem(hwnd, IDC_STATUSBAR);
	SendMessage(statusbar, WM_SIZE, 0, 0);

	GetWindowRect(statusbar, &sbrect);
	sbheight = sbrect.bottom - sbrect.top;

	GetClientRect(hwnd, &client);
	pheight = client.bottom - tbheight - sbheight;

	profiles = GetDlgItem(hwnd, IDC_PROFILES);
	SetWindowPos(profiles, NULL, 0, tbheight, client.right, pheight, SWP_NOZORDER);
}

void HandleZknockCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam)) {
		case ID_HOST_ADD:
		{
			int rc;
			struct profilecfg *profile = NULL;
			HWND lv;
			LV_ITEM lvi = { LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 0 };

			rc = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDHOST),
					hwnd, AddHostProc, (LPARAM) &profile);

			if(rc == IDOK && profile) {
				lv = GetDlgItem(hwnd, IDC_PROFILES);
				lvi.iItem = ListView_GetItemCount(lv);
				lvi.pszText = profile->alias;
				lvi.lParam = (LPARAM) profile;
				lvi.iImage = 0;
				
				ListView_InsertItem(lv, &lvi);

				g_runtimecfg.modified = 1;
			}

		}
		break;

		case ID_HOST_EDIT:
			g_runtimecfg.modified = EditHost(hwnd, GetDlgItem(hwnd, IDC_PROFILES));
			break;

		case ID_HOST_REMOVE:
			g_runtimecfg.modified = RemoveHost(hwnd, GetDlgItem(hwnd, IDC_PROFILES));
			break;

		case ID_HOST_KNOCK:
			KnockProfile(hwnd, GetSelectedProfile(GetDlgItem(hwnd, IDC_PROFILES)));
			break;

		case ID_FILE_EXIT:
		{
			if(g_runtimecfg.systray.enabled && g_runtimecfg.systray.onlyexit) {
				ShowWindow(hwnd, SW_HIDE);
				break;
			}

			if(!CheckQuit(hwnd))
				break;

			DestroyWindow(hwnd);
		}
		break;

		case ID_FILE_NEW:
		{
			if(g_runtimecfg.modified) {
				if(MessageBox(hwnd, "There are changes to your configuration. "
						"Are you sure you want to start a new configuration "
						"without saving?", "Are you sure?",
						MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
					break;
				}
			}

			ClearProfiles(GetDlgItem(hwnd, IDC_PROFILES));
			UpdateConfigPath(NULL);

			g_runtimecfg.passwd.enabled = 0;

		}
		break;

		case ID_FILE_SAVE:
		{
			if(!g_runtimecfg.modified) {
				MessageBox(hwnd, "There are no changes to save", "User Error",
						MB_OK | MB_ICONSTOP);
				break;
			}

			if(!g_runtimecfg.path) {
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FILE_SAVEAS, 0), lParam);
				break;
			}

			SaveConfiguration(GetDlgItem(hwnd, IDC_PROFILES), g_runtimecfg.path);
			g_runtimecfg.modified = 0;
		}
		break;

		case ID_FILE_SAVEAS:
		{
			OPENFILENAME ofn = {0};
			char path[MAX_PATH] = "";

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = "Zknock Configuration (*.zc)\0*.zc\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = path;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrDefExt = "zc";
			ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

			if(GetSaveFileName(&ofn)) {
				SaveConfiguration(GetDlgItem(hwnd, IDC_PROFILES), path);
				UpdateConfigPath(path);
			}

		}
		break;

		case ID_FILE_OPEN:
		{
			OPENFILENAME ofn = {0};
			char path[MAX_PATH] = "";

			if(g_runtimecfg.modified) {
				if(MessageBox(hwnd, "There are changes to your configuration. "
						"Would you like to save them first?", "Save changes?",
						MB_YESNO | MB_ICONQUESTION) == IDYES) {
					SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_FILE_SAVEAS, 0), lParam);
				}
			}

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hwnd;
			ofn.lpstrFilter = "Zknock Configuration (*.zc)\0*.zc\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile = path;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrDefExt = "zc";
			ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

			if(GetOpenFileName(&ofn)) {
				ClearProfiles(GetDlgItem(hwnd, IDC_PROFILES));
				OpenConfiguration(GetDlgItem(hwnd, IDC_PROFILES), path);
				UpdateConfigPath(path);
			}
		}
		break;

		case ID_EDIT_SETTINGS:
		{
			struct zknockcfg zcfg = {0};
			int rc;

			zcfg.runtimecfg = &g_runtimecfg;

			rc = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS),
					hwnd, SettingsProc, (LPARAM) &zcfg);

			if(rc == IDOK) {
				SetSystemTray(hwnd, g_runtimecfg.systray.enabled);
				g_runtimecfg.modified = 1;
			}

		}
		break;

		case ID_QUIT:
			if(!CheckQuit(hwnd))
				break;

			DestroyWindow(hwnd);
			break;

		case ID_HELP_ABOUT:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HELP_ABOUT),
				hwnd, AboutZknockProc);
			break;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine, int nCmdShow)
{
	WSADATA wsad;
	WNDCLASSEX wc={0};
	HWND hwnd;
	MSG msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = ZknockProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOCKET));
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SOCKETSM));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = "zknockmainwindow";

	InitCommonControls();
	LoadLibrary("riched20.dll");
	WSAStartup(MAKEWORD(1,1), &wsad);

	if(!RegisterClassEx(&wc)) {
		MessageBox(NULL, "Could not register class.", "Error!",
				MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	hwnd = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			"zknockmainwindow",
			"Zknock " ZKNOCK_VERSION,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
			NULL, NULL, hInstance, NULL);
	if(!hwnd) {
		MessageBox(NULL, "CreateWindowEx() failed.", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while(GetMessage(&msg, NULL, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	return msg.wParam;
}

void HandleZknockProfile(HWND hwnd, LPNMHDR hdr)
{
	HWND lv;
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg *profile;
	int rc;

	switch(hdr->code) {
		case NM_RETURN:
		case NM_DBLCLK:
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_HOST_KNOCK, 0), 0);
			break;

		case NM_RCLICK:
			lv = GetDlgItem(hwnd, IDC_PROFILES);
			rc = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
			if(rc >= 0) {
				ProfileContextMenu(hwnd, lv);
			}
			break;

		case LVN_KEYDOWN:
			switch(((LPNMLVKEYDOWN) hdr)->wVKey) {
				case VK_DELETE:
					PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_HOST_REMOVE, 0), 0);
					break;
			}
			break;
	}
}

void LoadBitmapToToolbar(HWND toolbar, WORD resource)
{
	HBITMAP bmp;
	TBADDBITMAP addbmp = {0};

	bmp = (HBITMAP) LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(resource));
	if(bmp) {
		addbmp.nID = (UINT_PTR) bmp;
		SendMessage(toolbar, TB_ADDBITMAP, 1, (LPARAM) &addbmp);

		//DeleteObject(bmp);
	}
}

void LoadBitmapToMenu(HMENU menu, WORD bmpid, WORD menuid)
{
	HBITMAP bmp;
	MENUITEMINFO mii = {0};

	bmp = (HBITMAP) LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(bmpid));
	if(bmp) {
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_CHECKMARKS;
		mii.hbmpChecked = bmp;
		mii.hbmpUnchecked = bmp;
		SetMenuItemInfo(menu, menuid, FALSE, &mii);
		//DeleteObject(bmp);
	}
}

void InitializeZknockMenu(HMENU menu)
{
	int i;
	struct { WORD bmpid; WORD menuid; } menuimages[] = {
		// file menu
		{ IDB_NEW, ID_FILE_NEW },
		{ IDB_OPEN, ID_FILE_OPEN },
		{ IDB_SAVE, ID_FILE_SAVE },
		{ IDB_SAVEAS, ID_FILE_SAVEAS },
		{ IDB_EXIT, ID_FILE_EXIT },

		// host menu
		{ IDB_HOSTADD, ID_HOST_ADD },
		{ IDB_HOSTREMOVE, ID_HOST_REMOVE },
		{ IDB_HOSTEDIT, ID_HOST_EDIT },
		{ IDB_HOSTKNOCK, ID_HOST_KNOCK },

		// settings & about
		{ IDB_SETTINGS, ID_EDIT_SETTINGS },
		{ IDB_HELP, ID_HELP_ABOUT }
	};

	for(i = 0; i < sizeof(menuimages)/sizeof(menuimages[0]); ++i) {
		LoadBitmapToMenu(menu, menuimages[i].bmpid, menuimages[i].menuid);
	}
}

void InitializeZknockToolbar(HWND toolbar)
{
	int i;
	TBBUTTON buttons[] = {
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },

		{ 0, ID_FILE_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 1, ID_FILE_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 2, ID_FILE_SAVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },

		{ 3, ID_HOST_KNOCK, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 4, ID_HOST_ADD, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 5, ID_HOST_EDIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 6, ID_HOST_REMOVE, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },

		{ 7, ID_EDIT_SETTINGS, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 8, ID_HELP_ABOUT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
		{ 0, 0, TBSTATE_ENABLED, TBSTYLE_SEP, 0 },

		{ 9, ID_FILE_EXIT, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0 },
	};
	WORD btnimages[] = {
		IDB_NEW, IDB_OPEN, IDB_SAVE,
		IDB_HOSTKNOCK, IDB_HOSTADD, IDB_HOSTEDIT, IDB_HOSTREMOVE,
		IDB_SETTINGS, IDB_HELP,
		IDB_EXIT,
	};

	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

	for(i = 0; i < sizeof(btnimages)/sizeof(btnimages[0]); ++i) {
		LoadBitmapToToolbar(toolbar, btnimages[i]);
	}

	SendMessage(toolbar,TB_ADDBUTTONS,sizeof(buttons)/sizeof(TBBUTTON),(LPARAM)&buttons);
}

void InitializeZknockProfileList(HWND list)
{
	HIMAGELIST imglist;
	HBITMAP bmp;

	bmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_PROFILE));
	if(!bmp) return;

	imglist = ImageList_Create(32,32,ILC_COLOR32,1,1);
	ImageList_Add(imglist, bmp, NULL);

	ListView_SetImageList(list, imglist, LVSIL_NORMAL);
}

void ZknockTooltips(HWND hwnd, LPTOOLTIPTEXT tip)
{
	tip->hinst = NULL;

	switch(tip->hdr.idFrom) {
		case ID_FILE_NEW:
			tip->lpszText = "New configuration";
			break;

		case ID_FILE_OPEN:
			tip->lpszText = "Open configuration";
			break;

		case ID_FILE_SAVE:
			tip->lpszText = "Save configuration";
			break;

		case ID_HOST_ADD:
			tip->lpszText = "Add host";
			break;

		case ID_HOST_REMOVE:
			tip->lpszText = "Remove host";
			break;

		case ID_HOST_EDIT:
			tip->lpszText = "Edit host";
			break;

		case ID_HOST_KNOCK:
			tip->lpszText = "Knock host";
			break;

		case ID_EDIT_SETTINGS:
			tip->lpszText = "Configure Zknock";
			break;

		case ID_HELP_ABOUT:
			tip->lpszText = "About Zknock";
			break;

		case ID_FILE_EXIT:
			tip->lpszText = "Exit Zknock";
			break;

		default:
			break;
	}
}

int EditHost(HWND hwnd, HWND profiles)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg *profile;
	int rc;

	lvi.iItem = ListView_GetNextItem(profiles, -1, LVNI_SELECTED);
	if(lvi.iItem < 0) {
		MessageBox(hwnd, "You must select a host to edit.", "User Error", MB_OK | MB_ICONSTOP);
		return 0;
	}

	ListView_GetItem(profiles, &lvi);
	profile = (struct profilecfg *) lvi.lParam;
	rc = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADDHOST),
			hwnd, AddHostProc, (LPARAM) &profile);

	if(rc == IDOK) {
		free((void *) lvi.lParam);
		lvi.mask |= LVIF_TEXT;
		lvi.pszText = profile->alias;
		lvi.lParam = (LPARAM) profile;
		ListView_SetItem(profiles, &lvi);

		return 1;
	}
	return 0;
}

int RemoveHost(HWND hwnd, HWND profiles)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg *profile;
	char *buf;
	int allocated = 1, len;

	lvi.iItem = ListView_GetNextItem(profiles, -1, LVNI_SELECTED);
	if(lvi.iItem < 0) {
		MessageBox(hwnd, "You must select a host to remove.", "User Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	ListView_GetItem(profiles, &lvi);

	profile = (struct profilecfg *) lvi.lParam;
	buf = (char *)GlobalAlloc(GPTR, 34 + strlen(profile->alias));
	if(buf) wsprintf(buf, "Are you sure you want to remove %s?", profile->alias);
	else {
		buf = "Are you sure you want to remove that host?";
		allocated = 0;
	}

	if(MessageBox(hwnd, buf, buf, MB_YESNO | MB_ICONQUESTION) == IDYES) {
		ListView_DeleteItem(profiles, lvi.iItem);
		profile_kill(&profile);
	}
	
	if(allocated)
		GlobalFree(buf);

	return 1;
}

void SaveConfiguration(HWND profiles, const char *path)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg **pcfg;
	int len, i;

	len = ListView_GetItemCount(profiles);
	pcfg = (struct profilecfg **) calloc(sizeof(struct profilecfg *), len+1);
	if(!pcfg) {
		MessageBox(profiles, "Out of Memory", "Out of Memory", MB_OK | MB_ICONERROR);
		return;
	}

	for(i = 0; i < len; ++i) {
		lvi.iItem = i;
		ListView_GetItem(profiles, &lvi);
		pcfg[i] = (struct profilecfg *) lvi.lParam;
	}
	pcfg[i] = NULL;

	profiles_save(pcfg, len, path);
	free(pcfg);
}

void OpenConfiguration(HWND profileslist, const char *path)
{
	struct profilecfg **profiles = NULL;
	LV_ITEM lvi = { LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE, 0 };
	int i;

	profiles = profiles_load(path);
	if(!profiles) {
		MessageBox(profileslist, "Failed to load configuration", "Loading Error", MB_OK | MB_ICONERROR);
		return;
	}

	for(i = 0; profiles[i]; ++i) {
		lvi.iItem = ListView_GetItemCount(profileslist);
		lvi.pszText = profiles[i]->alias;
		lvi.lParam = (LPARAM) profiles[i];
		lvi.iImage = 0;

		ListView_InsertItem(profileslist, &lvi);
	}

	free(profiles);
}

void UpdateConfigPath(const char *path)
{
	if(g_runtimecfg.path) {
		free(g_runtimecfg.path);
		g_runtimecfg.path = NULL;
	}

	if(path) {
		g_runtimecfg.path = xstrdup(path);
	}

	g_runtimecfg.modified = 0;
}

void ClearProfiles(HWND profiles)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg *profile;
	int i, len;

	len = ListView_GetItemCount(profiles);
	for(i = len-1; i >= 0; --i) {
		lvi.iItem = i;
		ListView_GetItem(profiles, &lvi);

		profile = (struct profilecfg *) lvi.lParam;
		profile_kill(&profile);

		ListView_DeleteItem(profiles, i);
	}
}

void ProfileContextMenu(HWND hwnd, HWND profiles)
{
	HMENU popup;
	POINT mouse;
	struct {
		UINT bmpid;
		UINT menuid;
		const char *str;
	} menu[] = {
		{ IDB_HOSTKNOCK, ID_HOST_KNOCK, "K&nock" },
		{ 0, 0, NULL },
		{ IDB_HOSTEDIT, ID_HOST_EDIT, "&Edit" },
		{ IDB_HOSTREMOVE, ID_HOST_REMOVE, "&Remove" },
	};
	int i;

	if(!GetCursorPos(&mouse)) {
		MessageBox(hwnd, "Failure tracking mouse position", "Error", MB_OK|MB_ICONSTOP);
		return;
	}

	popup = CreatePopupMenu();
	for(i = 0; i < sizeof(menu)/sizeof(menu[0]); ++i) {
		if(menu[i].str) {
			InsertMenu(popup, i, MF_BYPOSITION | MF_STRING, menu[i].menuid, menu[i].str);
			LoadBitmapToMenu(popup, menu[i].bmpid, menu[i].menuid);
		} else {
			InsertMenu(popup, i, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		}
	}

	SetForegroundWindow(hwnd);
	TrackPopupMenu(popup, TPM_TOPALIGN | TPM_LEFTALIGN, mouse.x, mouse.y, 0, hwnd, NULL);
}

void SetSystemTray(HWND hwnd, int enabled)
{
	static NOTIFYICONDATA nid = {0};

	if(!nid.cbSize) {
		nid.cbSize = sizeof(nid);
		nid.hWnd = hwnd;
		nid.uID = ID_SYSTRAY;
		nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
		nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SOCKET));
		nid.uCallbackMessage = IDM_SYSTRAY;
		wsprintf(nid.szTip, "Zknock " ZKNOCK_VERSION);
	}

	if(enabled) {
		Shell_NotifyIcon(NIM_ADD, &nid);
	} else {
		Shell_NotifyIcon(NIM_DELETE, &nid);
	}

	g_runtimecfg.systray.enabled = enabled;
}

void SystrayContextMenu(HWND hwnd)
{
	HMENU menu;
	HBITMAP bmp;
	MENUITEMINFO item = {0};
	MENUINFO info = {0};
	POINT mouse;
	struct profilecfg **profiles;
	int len, i = 0;

	if(!GetCursorPos(&mouse)) {
		MessageBox(hwnd, "Failure tracking mouse position", "Error", MB_OK|MB_ICONSTOP);
		return;
	}

	profiles = GetProfilesList(GetDlgItem(hwnd, IDC_PROFILES), &len);
	if(!profiles) return;

	bmp = (HBITMAP) LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_PROFILESM));
	if(!bmp) {
		free(profiles);
		return;
	}

	menu = CreatePopupMenu();

	if(len > 0) {
		item.cbSize = sizeof(item);
		item.fMask = MIIM_CHECKMARKS | MIIM_TYPE;
		item.fType = MFT_STRING;
		item.hbmpChecked = bmp;
		item.hbmpUnchecked = bmp;

		for(i = 0; i < len; ++i) {
			item.dwTypeData = profiles[i]->alias;
			item.cch = strlen(profiles[i]->alias);

			InsertMenuItem(menu, i, TRUE, &item);
		}
	} else {
		item.cbSize = sizeof(item);
		item.fMask = MIIM_TYPE | MIIM_STATE;
		item.fType = MFT_STRING;
		item.fState = MFS_GRAYED;
		item.dwTypeData = "No Profiles";
		item.cch = strlen(item.dwTypeData);

		InsertMenuItem(menu, i++, TRUE, &item);
	}

	InsertMenu(menu, i++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
	InsertMenu(menu, i++, MF_BYPOSITION | MF_STRING, ID_QUIT, "E&xit");
	LoadBitmapToMenu(menu, IDB_EXIT, ID_QUIT);

	free(profiles);

	info.cbSize = sizeof(info);
	info.fMask = MIM_STYLE;
	info.dwStyle = MNS_NOTIFYBYPOS;
	SetMenuInfo(menu, &info);

	SetForegroundWindow(hwnd);
	TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_CENTERALIGN, mouse.x, mouse.y, 0, hwnd, NULL);
}

struct profilecfg **GetProfilesList(HWND list, int *lenp)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	struct profilecfg **profiles;
	int len, i;

	len = ListView_GetItemCount(list);
	profiles = (struct profilecfg **) calloc(sizeof(struct profilecfg *), len+1);
	if(!profiles) {
		MessageBox(list, "Out of Memory (%x)", "Out of memory", MB_OK|MB_ICONERROR);
		return NULL;
	}

	for(i = 0; i < len; ++i) {
		lvi.iItem = i;
		ListView_GetItem(list, &lvi);
		profiles[i] = (struct profilecfg *) lvi.lParam;
	}

	profiles[i] = NULL;
	if(lenp) *lenp = len;

	return profiles;
}

struct profilecfg *GetProfile(HWND list, int idx)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	
	lvi.iItem = idx;
	ListView_GetItem(list, &lvi);

	return (struct profilecfg *) lvi.lParam;
}

struct profilecfg *GetSelectedProfile(HWND list)
{
	int i;

	i = ListView_GetNextItem(list, -1, LVNI_SELECTED);
	if(i < 0) return NULL;

	return GetProfile(list, i);
}

void KnockProfile(HWND hwnd, struct profilecfg *profile)
{
	struct knockcfg *kcfg;
	int rc;

	if(!profile) return;

	kcfg = profile_decrypt(profile, "");
	if(!kcfg) return;

	rc = DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_KNOCKING),
			hwnd, KnockingProc, (LPARAM) kcfg);

	kc_kill(&kcfg);
}

void vSetWindowTitle(HWND hwnd, const char *fmt, va_list ap)
{
	va_list cpy;
	char *buf;
	int len;

	va_copy(cpy, ap);
	len = vsnprintf(NULL, 0, fmt, ap);
	va_end(cpy);

	if(len < 0) return;

	buf = calloc(1, ++len);
	if(!buf) return;

	va_copy(cpy, ap);
	vsnprintf(buf, len, fmt, cpy);
	va_end(cpy);

	SetWindowText(hwnd, buf);
	free(buf);
}

void SetWindowTitle(HWND hwnd, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vSetWindowTitle(hwnd, fmt, ap);
	va_end(ap);
}

int CheckQuit(HWND hwnd)
{
	if(!g_runtimecfg.modified)
		return 1;

	if(MessageBox(hwnd, "You have made changes to your configuration. "
			"Are you sure you want to exit?", "Quit without saving?",
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDNO) {
		return 0;
	}

	return 1;
}
