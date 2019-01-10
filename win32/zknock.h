#ifndef ZKNOCK_H
# define ZKNOCK_H

# include <windows.h>
# include <commctrl.h>
# include <stdarg.h>

# define	ZKNOCK_VERSION		"0.1"
# define	DEFAULT_DELAY		250

# define	IDC_TOOLBAR		0x100
# define	IDC_STATUSBAR		0x101
# define	IDC_PROFILES		0x102
# define	ID_TB_SYSTRAY		0x103
# define	ID_SYSTRAY		0x104
# define	ID_QUIT			0x105
# define	ID_TMR			0x106

# define	IDM_SYSTRAY		(WM_APP + 1)

/* zknock.c */
struct runtimecfg {
	char *path;
	int modified;

	struct {
		int enabled;
		char *buf;
	} passwd;

	struct {
		int enabled;
		int onlyexit;
	} systray;

	struct {
		int enabled;
		int close;
	} verbose;
};

void ReportError(HWND hwnd, DWORD error);

LRESULT CALLBACK ZknockProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CreateZknockWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ResizeZknockWindow(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void HandleZknockCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void HandleZknockProfile(HWND hwnd, LPNMHDR hdr);

void LoadBitmapToToolbar(HWND toolbar, WORD resource);
void LoadBitmapToMenu(HMENU menu, WORD bmpid, WORD menuid);

void InitializeZknockMenu(HMENU menu);
void InitializeZknockToolbar(HWND toolbar);
void InitializeZknockProfileList(HWND list);

void ZknockTooltips(HWND hwnd, LPTOOLTIPTEXT hdr);
int EditHost(HWND hwnd, HWND profiles);
int RemoveHost(HWND hwnd, HWND profiles);

void SaveConfiguration(HWND profiles, const char *path);
void OpenConfiguration(HWND profileslist, const char *path);

void UpdateConfigPath(const char *path);
void ClearProfiles(HWND profiles);

void ProfileContextMenu(HWND hwnd, HWND profiles);

void SetSystemTray(HWND hwnd, int enabled);
void SystrayContextMenu(HWND hwnd);

struct profilecfg **GetProfilesList(HWND list, int *lenp);
struct profilecfg *GetProfile(HWND list, int idx);
struct profilecfg *GetSelectedProfile(HWND list);

void KnockProfile(HWND hwnd, struct profilecfg *profile);

void vSetWindowTitle(HWND hwnd, const char *fmt, va_list ap);
void SetWindowTitle(HWND hwnd, const char *fmt, ...);

int CheckQuit(HWND hwnd);

/* about.c */
INT_PTR CALLBACK AboutZknockProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* addhost.c */
struct knockcfg;

INT_PTR CALLBACK AddHostProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void FreeListViewItems(HWND lv);
void PopulatePortsList(HWND lv, struct knockcfg *kcfg);

/* knockcfg.c */
enum {
	PROTO_TCP = 0,
	PROTO_UDP,
};

struct portcfg {
	int proto;
	int port;
	int delay;
};

struct knockcfg {
	char alias[512];
	char host[512];
	int ports;
	struct portcfg *port;
};

struct hostcfg {
	char secure;
	char buf[];
};

struct profilecfg {
	char *alias;
	struct hostcfg *cfg;
};

# define	ZKNOCK_SIG_SIZE		4
# define	ZKNOCK_SIGNATURE	"\x1fZNK"
# define	PROFILE_SIG_SIZE	3
# define	PROFILE_SIGNATURE	"\xb0\x0bs"

struct zknockhdr {
	char sig[ZKNOCK_SIG_SIZE];	/* ZKNOCK_SIGNATURE */
	char secure;			/* (1|0) */
	int nprofiles;
} __attribute__ ((packed));

struct profilehdr {
	char sig[PROFILE_SIG_SIZE];	/* PROFILE_SIGNATURE */
	int alias_len;
	int cfg_len;
} __attribute__ ((packed));


char *pc_port2str(struct portcfg *pcfg);
char *pc_proto2str(struct portcfg *pcfg);
char *pc_delay2str(struct portcfg *pcfg);
char *pc_cfg2str(struct portcfg *pcfg);
struct portcfg *pc_str2cfg(struct portcfg *pcfg, char *buf);
struct portcfg *pc_dup(struct portcfg *pcfg);
void kc_addport(struct knockcfg *kcfg, struct portcfg *pcfg);
void kc_savefile(struct knockcfg *kcfg, const char *path);
void *kc_cfg2str(struct knockcfg *kcfg, const char *pswd);
void profile_save(struct profilecfg *pcfg, const char *path);
char *xstrdup(const char *s);
struct profilecfg *profile_new(const char *alias, struct hostcfg *hcfg);
void profile_kill(struct profilecfg **profile);
struct knockcfg *kc_parse(char *buf);
struct portcfg *kc_addports(struct knockcfg *kcfg, char *buf);
void kc_kill(struct knockcfg **kcfg);
int profiles_save(struct profilecfg **profiles, int nprofiles, const char *path);
struct profilecfg **profiles_load(const char *path);

/* encrypt.c */
char *str_encrypt(const char *str, const char *pswd);
struct knockcfg *profile_decrypt(struct profilecfg *profile, const char *pswd);

/* knocking.c */
INT_PTR CALLBACK KnockingProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK StartKnock(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime);

void knock(struct knockcfg *kcfg, HWND hwnd);

# define ProgressBar_SetRange(pbar, min, max)	PostMessage(pbar, PBM_SETRANGE, 0, MAKELPARAM(min, max))
# define ProgressBar_SetStep(pbar, step)	PostMessage(pbar, PBM_SETSTEP, (WPARAM) step, 0)
# define ProgressBar_StepIt(pbar)		SendMessage(pbar, PBM_STEPIT, 0, 0)

/* settings.c */
INT_PTR CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct zknockcfg {
	struct runtimecfg *runtimecfg;
};

struct zknockcfg **InitSettings(struct zknockcfg *zcfg);
struct zknockcfg **GetSettings(HWND hwnd);
void SetSettings(HWND hwnd, struct zknockcfg **zcfg);
void SaveSettings(HWND hwnd);
void FiniSettings(HWND hwnd);

#endif /* !ZKNOCK_H */
