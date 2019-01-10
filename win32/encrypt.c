#include "zknock.h"
#include "resource.h"

char *str_encrypt(const char *str, const char *pswd)
{
	DWORD len, size;
	BOOL rc;
	struct hostcfg *buf;

	len = strlen(str)+1;
	rc = CryptBinaryToString(str, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &size);
	if(!rc) return NULL;

	buf = calloc(1,size+1);
	if(!buf) return NULL;

	buf->secure = '0';

	rc = CryptBinaryToString(str, len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf->buf, &size);
	if(!rc) {
		free(buf);
		return NULL;
	}

	return (char *) buf;
}

struct knockcfg *profile_decrypt(struct profilecfg *profile, const char *pswd)
{
	DWORD len, size;
	BOOL rc;
	char *buf;
	struct knockcfg *kcfg;

	len = strlen((char*)profile->cfg->buf);
	rc = CryptStringToBinary(profile->cfg->buf, len, CRYPT_STRING_BASE64, NULL, &size, NULL, NULL);
	if(!rc) return NULL;

	buf = calloc(1, size+1);
	if(!buf) return NULL;

	rc = CryptStringToBinary(profile->cfg->buf, len, CRYPT_STRING_BASE64, buf, &size, NULL, NULL);
	if(!rc) {
		free(buf);
		return NULL;
	}

	kcfg = kc_parse(buf);
	free(buf);

	return kcfg;
}
