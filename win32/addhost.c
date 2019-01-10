#include "zknock.h"
#include "resource.h"

INT_PTR CALLBACK AddHostProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static struct profilecfg **profile = NULL;

	switch(msg) {
		case WM_INITDIALOG:
		{
			LV_COLUMN header[] = {
				{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_CENTER, 80, "Port", 0, 0 },
				{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_CENTER, 120, "Protocol", 0, 0 },
				{ LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_CENTER, 60, "Delay", 0, 0 },
			};
			HWND h;
			struct knockcfg *kcfg;
			char *title;
			int i;

			CheckDlgButton(hwnd, ID_CB_REQPASSWD, 1);
			CheckDlgButton(hwnd, ID_RAD_TCP, 1);

			h = GetDlgItem(hwnd, ID_LV_PORTS);
			for(i = 0; i < sizeof(header)/sizeof(header[0]); ++i) {
				ListView_InsertColumn(h, i, &header[i]);
			}

			SendMessage(h, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
			SetDlgItemInt(hwnd, ID_TXT_DELAY, DEFAULT_DELAY, FALSE);

			profile = (struct profilecfg **) lParam;
			if(*profile) {
				if((*profile)->cfg->secure == '0') {
					CheckDlgButton(hwnd, ID_CB_REQPASSWD, 0);

					EnableWindow(GetDlgItem(hwnd, ID_TXT_PASSWD), FALSE);
					EnableWindow(GetDlgItem(hwnd, ID_TXT_VPASSWD), FALSE);
				}

				kcfg = profile_decrypt(*profile, "");
				if(!kcfg) break;

				title = GlobalAlloc(GPTR, 13 + strlen(kcfg->alias));
				if(title) {
					wsprintf(title,"Configuring %s", kcfg->alias);
					SetWindowText(hwnd, title);
					GlobalFree(title);
				}
				SetDlgItemText(hwnd, ID_TXT_ALIAS, kcfg->alias);
				SetDlgItemText(hwnd, ID_TXT_HOST, kcfg->host);
				PopulatePortsList(GetDlgItem(hwnd, ID_LV_PORTS), kcfg);
				kc_kill(&kcfg);
			}

		}
		break;

		case WM_CLOSE:
			FreeListViewItems(GetDlgItem(hwnd, ID_LV_PORTS));
			EndDialog(hwnd, IDCANCEL);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_CB_REQPASSWD:
				{
					UINT checked;
					BOOL enabled = FALSE;

					checked = IsDlgButtonChecked(hwnd, ID_CB_REQPASSWD);
					if(checked) enabled = TRUE;

					EnableWindow(GetDlgItem(hwnd, ID_TXT_PASSWD), enabled);
					EnableWindow(GetDlgItem(hwnd, ID_TXT_VPASSWD), enabled);
				}
				break;

				case IDOK:
				{
					HWND lv;
					struct knockcfg kc = {0};
					struct hostcfg *hcfg;
					int i, len;
					char pass1[512], pass2[512];

					GetDlgItemText(hwnd, ID_TXT_PASSWD, pass1, sizeof(pass1)-1);
					GetDlgItemText(hwnd, ID_TXT_VPASSWD, pass2, sizeof(pass2)-1);
					GetDlgItemText(hwnd, ID_TXT_ALIAS, kc.alias, sizeof(kc.alias)-1);
					GetDlgItemText(hwnd, ID_TXT_HOST, kc.host, sizeof(kc.host)-1);
					
					if(IsDlgButtonChecked(hwnd, ID_CB_REQPASSWD)) {
						len = strlen(pass1);
						if(!len) {
							MessageBox(hwnd,
								"You must specify a password.",
								"User Error",
								MB_OK | MB_ICONSTOP);
							break;
						}

						if(strcmp(pass1,pass2)) {
							MessageBox(hwnd,
								"Password did not match.",
								"User Error",
								MB_OK | MB_ICONSTOP);
							break;
						}
					}
					
					len = GetWindowTextLength(GetDlgItem(hwnd, ID_RE_EXPERT));
					if(len <= 0) {
						len = strlen(kc.host);
						if(!len) {
							MessageBox(hwnd,
								"You must specify a host.",
								"User Error",
								MB_OK | MB_ICONSTOP);
							break;
						}

						len = strlen(kc.alias);
						if(!len) {
							strncpy(kc.alias, kc.host, sizeof(kc.alias)-1);
						}

						lv = GetDlgItem(hwnd, ID_LV_PORTS);
						len = ListView_GetItemCount(lv);
						if(len <= 0) {
							MessageBox(hwnd,
								"You must specify the ports.",
								"User Error",
								MB_OK | MB_ICONSTOP);
							break;
						}

						for(i = 0; i < len; ++i) {
							LV_ITEM lvi = { LVIF_PARAM, 0 };
							struct portcfg *pcfg;

							lvi.iItem = i;

							ListView_GetItem(lv, &lvi);
							pcfg = (struct portcfg *) lvi.lParam;
							if(pcfg) {
								kc_addport(&kc, pcfg);
								free(pcfg);

								lvi.lParam = 0;
								ListView_SetItem(lv, &lvi);
							}
						}

					}

					hcfg = kc_cfg2str(&kc, pass1);

					if(!hcfg) {
						MessageBox(hwnd, "Failed to create hostcfg", "Error!",
							MB_OK | MB_ICONSTOP);

						free(kc.port);
						EndDialog(hwnd, IDCANCEL);
					}

					*profile = profile_new(kc.alias, hcfg);

					if(!*profile) {
						MessageBox(hwnd, "Failed to create profile.", "Error",
							MB_OK | MB_ICONSTOP);

						EndDialog(hwnd, IDCANCEL);
						break;
					}

					profile = NULL;

					free(kc.port);
					kc.ports = 0;

					EndDialog(hwnd, IDOK);
				}
				break;

				case IDCANCEL:
					FreeListViewItems(GetDlgItem(hwnd, ID_LV_PORTS));
					EndDialog(hwnd, IDCANCEL);
					break;

				case ID_BTN_ADD:
				{
					HWND lv;
					LV_ITEM lvi = { LVIF_TEXT | LVIF_PARAM, 0 };
					struct portcfg portcfg = {0};
					BOOL ret;

					lv = GetDlgItem(hwnd, ID_LV_PORTS);

					portcfg.port = GetDlgItemInt(hwnd, ID_TXT_PORT, &ret, FALSE);
					if(!ret) {
						MessageBox(hwnd, "Port must be a number", "User Input Error",
								MB_OK | MB_ICONEXCLAMATION);
						break;
					}

					if(portcfg.port <= 0 || portcfg.port >= 65536) {
						MessageBox(hwnd, "Port is out of range", "User Input Error",
								MB_OK | MB_ICONEXCLAMATION);
						break;
					}

					portcfg.delay = GetDlgItemInt(hwnd, ID_TXT_DELAY, &ret, FALSE);
					if(!ret) {
						MessageBox(hwnd, "Delay must be an integer", "User Input Error",
								MB_OK | MB_ICONEXCLAMATION);
						break;
					}

					if(IsDlgButtonChecked(hwnd, ID_RAD_UDP)) {
						portcfg.proto = PROTO_UDP;
					}

					lvi.iItem = ListView_GetItemCount(lv);

					lvi.pszText = pc_port2str(&portcfg);
					lvi.lParam = (LPARAM) pc_dup(&portcfg);
					ListView_InsertItem(lv, &lvi);

					lvi.mask &= ~LVIF_PARAM;
					lvi.pszText = pc_proto2str(&portcfg);
					lvi.iSubItem = 1;
					ListView_SetItem(lv, &lvi);

					lvi.pszText = pc_delay2str(&portcfg);
					lvi.iSubItem = 2;
					ListView_SetItem(lv, &lvi);

					SetDlgItemText(hwnd, ID_TXT_PORT, "");
					SetFocus(GetDlgItem(hwnd, ID_TXT_PORT));
				}
				break;

				case ID_BTN_REMOVE:
				{
					LV_ITEM lvi = { LVIF_PARAM, 0 };
					HWND lv;

					lv = GetDlgItem(hwnd, ID_LV_PORTS);
					lvi.iItem = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
					if(lvi.iItem < 0) break;

					ListView_GetItem(lv, &lvi);
					ListView_DeleteItem(lv, lvi.iItem);
					free((void *) lvi.lParam);
				}
				break;
			}
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

void FreeListViewItems(HWND lv)
{
	LV_ITEM lvi = { LVIF_PARAM, 0 };
	DWORD i, len;

	len = ListView_GetItemCount(lv);
	for(i = 0; i < len; ++i) {
		lvi.iItem = i;
		TreeView_GetItem(lv, &lvi);

		free((void *) lvi.lParam);
	}
}

void PopulatePortsList(HWND lv, struct knockcfg *kcfg)
{
	LV_ITEM lvi = { LVIF_TEXT | LVIF_PARAM, 0 };
	struct portcfg *pcfg;
	int i;

	for(i = 0; i < kcfg->ports; ++i) {
		pcfg = pc_dup(&kcfg->port[i]);
		if(!pcfg) continue;

		lvi.mask |= LVIF_PARAM;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.lParam = (LPARAM) pcfg;
		lvi.pszText = pc_port2str(pcfg);
		ListView_InsertItem(lv, &lvi);

		lvi.mask &= ~LVIF_PARAM;
		lvi.pszText = pc_proto2str(pcfg);
		lvi.iSubItem = 1;
		ListView_SetItem(lv, &lvi);

		lvi.pszText = pc_delay2str(pcfg);
		lvi.iSubItem = 2;
		ListView_SetItem(lv, &lvi);
	}
}
