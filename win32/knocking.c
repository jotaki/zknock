#include <winsock.h>

#include <unistd.h>	// usleep

#include "zknock.h"
#include "resource.h"

static struct knockcfg *g_knockcfg = NULL;

INT_PTR CALLBACK KnockingProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
		{
			HWND pbar;

			g_knockcfg = (struct knockcfg *) lParam;
			SetWindowTitle(hwnd, "Knocking on %s", g_knockcfg->alias);
			
			pbar = GetDlgItem(hwnd, ID_KNOCK_PROGRESS);
			ProgressBar_SetRange(pbar, 0, g_knockcfg->ports+1);
			ProgressBar_SetStep(pbar, 1);

			SetTimer(hwnd, ID_TMR, 250, (TIMERPROC) &StartKnock);
		}
		break;

		case WM_CLOSE:
			g_knockcfg = NULL;
			EndDialog(hwnd, 0);
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

void StartKnock(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
	knock(g_knockcfg, hwnd);
	KillTimer(hwnd, ID_TMR);
	PostMessage(hwnd, WM_CLOSE, 0, 0);
}

void knock(struct knockcfg *kcfg, HWND hwnd)
{
	HWND pbar;
	SOCKET sd;
	struct hostent *hp;
	struct sockaddr_in addr = {0};
	int flag, i;

	SetWindowTitle(hwnd, "Knocking on %s [Resolving Host]", kcfg->alias);
	hp = gethostbyname(kcfg->host);
	if(!hp) { return; }

	pbar = GetDlgItem(hwnd, ID_KNOCK_PROGRESS);
	ProgressBar_StepIt(pbar);

	for(i = 0; i < kcfg->ports; ++i) {
		if(i > 0) {
			SetWindowTitle(hwnd,"Knocking on %s [Port %d/%d] [Waiting %dms]",
					kcfg->alias, i+1, kcfg->ports, kcfg->port[i-1].delay);
			usleep(kcfg->port[i-1].delay*1000);
		}

		SetWindowTitle(hwnd, "Knocking on %s [Port %d/%d]", kcfg->alias, i+1, kcfg->ports);
		if(kcfg->port[i].proto == PROTO_UDP) {
			sd = socket(PF_INET, SOCK_DGRAM, 0);
			if(sd < 0) continue;
		} else if(kcfg->port[i].proto == PROTO_TCP) {
			sd = socket(AF_INET, SOCK_STREAM, 0);
			if(sd < 0) continue;

			flag = 1;
			setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, flag);

			flag = 1;
			ioctlsocket(sd, FIONBIO, (u_long *) &flag);
		}

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(kcfg->port[i].port);
		memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

		if(kcfg->port[i].proto == PROTO_UDP) {
			sendto(sd, "", 1, 0, (struct sockaddr *) &addr, sizeof(addr));
		} else {
			connect(sd, (struct sockaddr *) &addr, sizeof(addr));
		}

		closesocket(sd);
		ProgressBar_StepIt(pbar);
	}
}
