#include "zknock.h"
#include "resource.h"

INT_PTR CALLBACK AboutZknockProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
		{
			HWND text = GetDlgItem(hwnd, ID_ABOUT_LABEL);
			SendMessage(text, WM_SETTEXT, 0, (LPARAM)
				"\tZknock is a port knocking utility for Windows.\n\n" \
				"Port knocking is a \"security through obscurity\" method; " \
				"requiring a client to first attempt connections to "
				"a certain sequence of ports using some protocol before " \
				"the server will grant access to some service or services.\n\n\t"
				"Zknock version " ZKNOCK_VERSION " written by Joseph Kinsella");

		}
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK:
					EndDialog(hwnd, 0);
					break;
			}
			break;

		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;

		default:
			return FALSE;
	}
	return TRUE;
}
