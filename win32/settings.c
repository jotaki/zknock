#include "zknock.h"
#include "resource.h"

INT_PTR CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct zknockcfg *zcfg, **zc;

	if(msg != WM_INITDIALOG) {
		zc = GetSettings(hwnd);
		if(zc) zcfg = zc[0];
	}

	switch(msg) {
		case WM_INITDIALOG:
		{
			zc = InitSettings((struct zknockcfg *) lParam);
			if(!zc) break;

			SetSettings(hwnd, zc);
			zcfg = zc[0];

			CheckDlgButton(hwnd, ID_CB_SYSTRAY, zcfg->runtimecfg->systray.enabled);
			CheckDlgButton(hwnd, ID_CB_STEXITONLY, zcfg->runtimecfg->systray.onlyexit);
			CheckDlgButton(hwnd, ID_CB_VKNOCK, zcfg->runtimecfg->verbose.enabled);
			CheckDlgButton(hwnd, ID_CB_AUTOCLOSE, zcfg->runtimecfg->verbose.close);
			CheckDlgButton(hwnd, ID_CB_REQPASSWD, zcfg->runtimecfg->passwd.enabled);

			EnableWindow(GetDlgItem(hwnd, ID_CB_STEXITONLY), zcfg->runtimecfg->systray.enabled);
			EnableWindow(GetDlgItem(hwnd, ID_CB_AUTOCLOSE), zcfg->runtimecfg->verbose.enabled);
			EnableWindow(GetDlgItem(hwnd, ID_TXT_PASSWD), zcfg->runtimecfg->passwd.enabled);
			EnableWindow(GetDlgItem(hwnd, ID_TXT_VPASSWD), zcfg->runtimecfg->passwd.enabled);
		}
		break;

		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			FiniSettings(hwnd);
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
				{
					SaveSettings(hwnd);
				}
				case IDCANCEL:
				{
					FiniSettings(hwnd);
					EndDialog(hwnd, LOWORD(wParam));
				}
				break;

				case ID_CB_SYSTRAY:
				{
					BOOL checked = !!IsDlgButtonChecked(hwnd, ID_CB_SYSTRAY);

					EnableWindow(GetDlgItem(hwnd, ID_CB_STEXITONLY), checked);
					zcfg->runtimecfg->systray.enabled = checked;
				}
				break;

				case ID_CB_STEXITONLY:
				{
					BOOL checked = !!IsDlgButtonChecked(hwnd, ID_CB_STEXITONLY);

					zcfg->runtimecfg->systray.onlyexit = checked;
				}
				break;

				case ID_CB_VKNOCK:
				{
					BOOL checked = !!IsDlgButtonChecked(hwnd, ID_CB_VKNOCK);

					EnableWindow(GetDlgItem(hwnd, ID_CB_AUTOCLOSE), checked);
					zcfg->runtimecfg->verbose.enabled = checked;
				}
				break;

				case ID_CB_AUTOCLOSE:
				{
					BOOL checked = !!IsDlgButtonChecked(hwnd, ID_CB_AUTOCLOSE);
					zcfg->runtimecfg->verbose.close = checked;
				}
				break;

				case ID_CB_REQPASSWD:
				{
					BOOL checked = !!IsDlgButtonChecked(hwnd, ID_CB_REQPASSWD);

					EnableWindow(GetDlgItem(hwnd, ID_TXT_PASSWD), checked);
					EnableWindow(GetDlgItem(hwnd, ID_TXT_VPASSWD), checked);
					zcfg->runtimecfg->passwd.enabled = checked;
				}
				break;

			}
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

struct zknockcfg **InitSettings(struct zknockcfg *zcfg)
{
	struct zknockcfg **zcfgp, *new;

	zcfgp = (struct zknockcfg **) calloc(2, sizeof(struct zknockcfg *));
	if(!zcfgp) return NULL;

	new = calloc(1, sizeof(*new));
	if(!new) {
		free(zcfgp);
		return NULL;
	}

	memcpy(new, zcfg, sizeof(*zcfg));

	new->runtimecfg = calloc(1, sizeof(struct runtimecfg));
	if(!new->runtimecfg) {
		free(new);
		free(zcfgp);
		return NULL;
	}

	memcpy(new->runtimecfg, zcfg->runtimecfg, sizeof(struct runtimecfg));

	/* new->runtimecfg->path = xstrdup(zcfg->runtimecfg->path);
	 * if(!new->runtimecfg->path) {
	 * 	free(new->runtimecfg);
	 * 	free(new);
	 * 	free(zcfgp);
	 * 	return NULL;
	 * } */

	zcfgp[0] = new;
	zcfgp[1] = zcfg;

	return zcfgp;
}

struct zknockcfg **GetSettings(HWND hwnd)
{
	return (struct zknockcfg **) GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

void SetSettings(HWND hwnd, struct zknockcfg **zcfg)
{
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) zcfg);
}

void SaveSettings(HWND hwnd)
{
	struct zknockcfg **zc;

	zc = GetSettings(hwnd);
	if(!zc) return;

	memcpy(zc[1]->runtimecfg, zc[0]->runtimecfg, sizeof(struct runtimecfg));
}

void FiniSettings(HWND hwnd)
{
	struct zknockcfg **zc;

	zc = GetSettings(hwnd);
	if(!zc) return;

	free(zc[0]->runtimecfg);
	free(zc[0]);
	free(zc);

	SetSettings(hwnd, NULL);
}
