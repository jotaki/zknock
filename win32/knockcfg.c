#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zknock.h"


char *pc_port2str(struct portcfg *pcfg)
{
	static char buf[8];
	sprintf(buf,"%d",pcfg->port);
	return buf;
}

char *pc_proto2str(struct portcfg *pcfg)
{
	static char buf[4];

	switch(pcfg->proto) {
		case PROTO_TCP:
			sprintf(buf,"tcp");
			break;

		case PROTO_UDP:
			sprintf(buf,"udp");
			break;

		default:	// XXX: shouldn't get here.
			sprintf(buf,"xXx");
	}
	return buf;
}

char *pc_delay2str(struct portcfg *pcfg)
{
	static char buf[12];
	snprintf(buf,11,"%d",pcfg->delay);
	return buf;
}

char *pc_cfg2str(struct portcfg *pcfg)
{
	static char buf[32];
	snprintf(buf,sizeof(buf)-1,"%d:%d:%d", pcfg->port, pcfg->proto, pcfg->delay);
	return buf;
}

struct portcfg *pc_str2cfg(struct portcfg *pcfg, char *buf)
{
	char *n, *c;

	c = strchr(buf, ':');
	if(!c) return NULL;

	*c = 0;
	pcfg->port = atoi(buf);
	buf = c+1;

	c = strchr(buf, ':');
	if(!c) return NULL;
	
	*c = 0;
	pcfg->proto = atoi(buf);
	pcfg->delay = atoi(c+1);

	return pcfg;
}

struct portcfg *pc_dup(struct portcfg *pcfg)
{
	struct portcfg *new;

	new = calloc(1,sizeof(*new));
	if(new) memcpy(new,pcfg,sizeof(*new));
	
	return new;
}

void kc_addport(struct knockcfg *kcfg, struct portcfg *pcfg)
{
	struct portcfg *p;

	p = realloc(kcfg->port, (kcfg->ports+1) * sizeof(*p));
	if(!p) return;

	kcfg->port = p;

	memcpy(&kcfg->port[kcfg->ports], pcfg, sizeof(*pcfg));
	kcfg->ports += 1;
}

void *kc_cfg2str(struct knockcfg *kcfg, const char *pswd)
{
	char *buf;
	int i, z=0;
	char *str, *p;

	buf = calloc(1, 4096);
	if(!buf) return NULL;

	for(i = 0; i < kcfg->ports; ++i) {
		z += snprintf(&buf[z], 4096-(z+1), "%d:%d:%d ",
			kcfg->port[i].port, kcfg->port[i].proto, kcfg->port[i].delay);
	}
	buf[z] = 0;

	i = strlen(kcfg->alias) + strlen("alias=\"\";");
	i += strlen(kcfg->host) + strlen("host=\"\";");
	i += z + strlen("ports=();\n");

	str = calloc(1, i+kcfg->ports+1);
	if(!str) {
		free(buf);
		return NULL;
	}

	sprintf(str,"alias=\"%s\";host=\"%s\";ports=(%s);\n", kcfg->alias, kcfg->host, buf);
	p = str_encrypt(str, pswd);
	free(str);
	free(buf);

	return p;
}

void profile_save(struct profilecfg *pcfg, const char *path)
{
	FILE *fp;
	
	fp = fopen(path, "w");
	if(fp) {
		fprintf(fp,"[%s]%s\n",pcfg->alias,(char*)pcfg->cfg);
		fclose(fp);
	}
}

struct profilecfg *profile_new(const char *alias, struct hostcfg *hcfg)
{
	struct profilecfg *prof;

	prof = calloc(1, sizeof(*prof));
	if(!prof) return NULL;

	prof->alias = xstrdup(alias);
	if(!prof->alias) {
		free(prof);
		return NULL;
	}

	prof->cfg = hcfg;

	return prof;
}

void profile_kill(struct profilecfg **profile)
{
	if(profile && *profile) {
		free((*profile)->alias);
		free((*profile)->cfg);
		free(*profile);

		*profile = NULL;
	}
}

char *xstrdup(const char *s)
{
	char *buf;

	buf = calloc(1,strlen(s)+1);
	if(buf) {
		strcpy(buf,s);
	}
	return buf;
}

static int splitvar(char **var, char **val, const char *s)
{
	char *varp, *valp, *eqp, *valp2, *p, *p2;
	int vall, varl;
	
	eqp = strchr(s, '=');
	if(!eqp) return 1;

	varl = (int) (eqp - s);
	varp = calloc(1, varl+1);
	if(!varp) return -1;

	vall = strlen(eqp+1);
	valp = calloc(1, vall+1);
	if(!valp) {
		free(varp);
		return -1;
	}

	strncpy(varp, s, varl);
	strcpy(valp, eqp+1);

	valp2 = xstrdup(valp);
	if(!valp2) {
		free(varp);
		free(valp);
		return -1;
	}

	p = valp;
	for(p2 = valp2; *p2; ++p2) {
		if(*p2 == '"') {
			if(p2 == valp2) {
				continue;
			}
			else if(p2[-1] == '\\') {
				p[-1] = '"';
				continue;
			}
			else {
				continue;
			}
		}
		*p++ = *p2;
	}
	*p = *p2;

	free(valp2);

	*var = varp;
	*val = valp;

	return 0;
}

struct knockcfg *kc_parse(char *buf)
{
	struct knockcfg *cfg;
	char *str, *p, *var, *val;
	int iq = 0;

	cfg = calloc(1, sizeof(*cfg));
	if(!cfg) return NULL;

	str = buf;
	for(p = str; *p; ++p) {
		if(*p == '"') {
			if(p == str || p[-1] != '\\') {
				iq = !iq;
				continue;
			}
		} else if(*p == ';' && !iq) {
			if(p != str) {
				*p = 0;
				if(!splitvar(&var, &val, str)) {
					if(!strcmp(var, "alias")) {
						strncpy(cfg->alias, val, sizeof(cfg->alias)-1);
					} else if(!strcmp(var, "host")) {
						strncpy(cfg->host, val, sizeof(cfg->host)-1);
					} else if(!strcmp(var, "ports")) {
						kc_addports(cfg, val);
					}
					free(var);
					free(val);
				}
				*p = ';';
			}
			str = p+1;
		}

	}
	return cfg;
}

struct portcfg *kc_addports(struct knockcfg *kcfg, char *buf)
{
	char *st, *ed, *p;
	struct portcfg pcfg = {0};

	st = strchr(buf, '(');
	ed = strchr(buf, ')');

	if(!st || !ed) return NULL;

	++st;
	while(st) {
		ed = strchr(st,' ');
		if(!ed) {
			ed = strchr(st,')');
			if(!ed) break;
		}
		*ed++ = 0;

		if(pc_str2cfg(&pcfg, st))
			kc_addport(kcfg, &pcfg);

		st = ed;
	}

	return kcfg->port;
}

void kc_kill(struct knockcfg **kcfg)
{
	free((*kcfg)->port);
	free(*kcfg);

	*kcfg = NULL;
}

int profiles_save(struct profilecfg **profiles, int nprofiles, const char *path)
{
	FILE *fp;
	struct zknockhdr hdr = { ZKNOCK_SIGNATURE, '0', '\n' };
	struct profilehdr profhdr = { PROFILE_SIGNATURE, 0, 0 };
	int i, j, tmp;
	char *buf;

	fp = fopen(path, "wb");
	if(!fp) return 1;

	hdr.nprofiles = nprofiles;
	fwrite(&hdr, sizeof(hdr), 1, fp);

	for(i = 0; i < nprofiles; ++i) {
		buf = (char *) profiles[i]->cfg;
		profhdr.alias_len = strlen(profiles[i]->alias);
		profhdr.cfg_len = strlen(buf);

		fwrite(&profhdr, sizeof(profhdr), 1, fp);

		// just make it look binary.
		for(j = 0; j < profhdr.alias_len; ++j) {
			tmp = profiles[i]->alias[j];
			if(tmp != 42) tmp ^= 42;
			fwrite(&tmp, 1, 1, fp);
		}

		for(j = 0; j < profhdr.cfg_len; ++j) {
			tmp = buf[j];
			if(tmp != 42) tmp ^= 42;
			fwrite(&tmp, 1, 1, fp);
		}
	}

	fclose(fp);
	return 0;
}

struct profilecfg **profiles_load(const char *path)
{
	struct zknockhdr hdr = {0};
	struct profilehdr profhdr;
	struct profilecfg *profile, **profiles;
	FILE *fp;
	char *p;
	int i = 0;

	fp = fopen(path, "rb");
	if(!fp) return NULL;

	fread(&hdr, sizeof(hdr), 1, fp);
	if(memcmp(ZKNOCK_SIGNATURE, hdr.sig, ZKNOCK_SIG_SIZE)) {
		fclose(fp);
		return NULL;
	}

	profiles = (struct profilecfg **) calloc(sizeof(struct profilecfg *), hdr.nprofiles+1);
	if(!profiles) {
		fclose(fp);
		return NULL;
	}

	while(fread(&profhdr, 1, sizeof(profhdr), fp) == sizeof(profhdr)) {
		if(memcmp(PROFILE_SIGNATURE, profhdr.sig, PROFILE_SIG_SIZE)) break;

		profile = calloc(1, sizeof(*profile));
		if(!profile) break;

		profile->alias = calloc(1, profhdr.alias_len+1);
		if(!profile->alias) {
			free(profile);
			break;
		}

		profile->cfg = calloc(1, profhdr.cfg_len+1);
		if(!profile->cfg) {
			free(profile->alias);
			free(profile);
			break;
		}

		fread(profile->alias, 1, profhdr.alias_len, fp);
		fread(profile->cfg, 1, profhdr.cfg_len, fp);

		for(p = profile->alias; *p; ++p) {
			if(*p != 42) *p ^= 42;
		}

		for(p = (char *) profile->cfg; *p; ++p) {
			if(*p != 42) *p ^= 42;
		}

		profiles[i++] = profile;
	}

	profiles[i] = NULL;
	fclose(fp);

	return profiles;
}
