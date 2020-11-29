typedef int                     BOOL;
typedef unsigned char           BYTE;
typedef unsigned int            DWORD;
typedef const unsigned short *  LPCWSTR;
typedef LPCWSTR                 LPCTSTR;
typedef unsigned short          WCHAR;
typedef WCHAR                   TCHAR;
typedef const char *            LPCSTR;
typedef int                     SOCKET;
typedef void *                  HMODULE;
typedef void *                  LPVOID;

#define TRUE            1
#define FALSE           0
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1

#define WAIT_OBJECT_0   0
#define WAIT_ABANDONED  0x00000080
#define WAIT_TIMEOUT    0x00000102

#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>


#include <stdio.h>
#include <inttypes.h>

#include "core/tsdump_def.h"
#include "utils/arib_proginfo.h"
#include "core/module_api.h"

#include "IBonDriver2.h"

typedef struct {
	HMODULE hdll;
	pCreateBonDriver_t *pCreateBonDriver;
	IBonDriver *pBon;
	IBonDriver2 *pBon2;
	DWORD n_rem;
} bondriver_stat_t;

static const char *bon_dll_name = NULL;
static int sp_num = -1;
static int ch_num = -1;
static int reg_hook = 0;

static int hook_postconfig()
{
	if (bon_dll_name == NULL) {
		return 1;
	}

	if (!reg_hook) {
		output_message(MSG_ERROR, "generatorフックの登録に失敗しました");
		return 0;
	}

	if (ch_num < 0) {
		output_message(MSG_ERROR, "チャンネルが指定されていないか、または不正です");
		return 0;
	}
	if (sp_num < 0) {
		output_message(MSG_ERROR, "チューナー空間が指定されていないか、または不正です");
		return 0;
	}

	return 1;
}

static void hook_stream_generator(void *param, unsigned char **buf, int *size)
{
	DWORD n_recv;
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;

	/* tsをチューナーから取得 */
	if (!pstat->pBon2->GetTsStream(buf, &n_recv, &pstat->n_rem)) {
		*size = 0;
		*buf = NULL;
		return;
	}
	*size = n_recv;
}

static int hook_stream_generator_wait(void *param, int timeout_ms)
{
	DWORD ret;
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;

	if (pstat->n_rem > 0) {
		return 1;
	}
	if (timeout_ms > 0) {
		ret = pstat->pBon2->WaitTsStream(timeout_ms);
		if (ret == WAIT_OBJECT_0) {
			return 1;
		}
	}
	return 0;
}

static int hook_stream_generator_open(void **param, ch_info_t *chinfo)
{
	bondriver_stat_t *pstat, stat;
	ch_info_t ci;

	stat.hdll = dlopen(bon_dll_name, RTLD_LAZY);
	if (stat.hdll == NULL) {
		output_message(MSG_SYSERROR, "BonDriverをロードできませんでした(LoadLibrary): %s", bon_dll_name);
		return 0;
	}

	stat.pCreateBonDriver = (pCreateBonDriver_t*)dlsym(stat.hdll, "CreateBonDriver");
	if (stat.pCreateBonDriver == NULL) {
		dlclose(stat.hdll);
		output_message(MSG_SYSERROR, "CreateBonDriver()のポインタを取得できませんでした(GetProcAddress): %s", bon_dll_name);
		return 0;
	}

	stat.pBon = stat.pCreateBonDriver();
	if (stat.pBon == NULL) {
		dlclose(stat.hdll);
		output_message(MSG_ERROR, "CreateBonDriver()に失敗しました: %s", bon_dll_name);
		return 0;
	}

	stat.pBon2 = (IBonDriver2 *)(stat.pBon);

	if (! stat.pBon2->OpenTuner()) {
		dlclose(stat.hdll);
		output_message(MSG_ERROR, "OpenTuner()に失敗しました");
		return 0;
	}

	ci.ch_str = (TSDCHAR *)stat.pBon2->EnumChannelName(sp_num, ch_num);
	ci.sp_str = (TSDCHAR *)stat.pBon2->EnumTuningSpace(sp_num);
	ci.tuner_name = (TSDCHAR *)stat.pBon2->GetTunerName();
	ci.ch_num = ch_num;
	ci.sp_num = sp_num;

	if (!ci.tuner_name) {
		ci.tuner_name = "NullTuner";
	}
	if (!ci.ch_str) {
		ci.ch_str = "Null";
	}
	if (!ci.sp_str) {
		ci.sp_str = "Null";
	}

	/* これを入れておかないとSetChannelに失敗するBonDriverが存在する e.g. BonDriver PT-ST 人柱版3 */
	usleep(500);

	output_message(MSG_NOTIFY, "BonTuner: %s\nSpace: %s\nChannel: %s",
		ci.tuner_name, ci.sp_str, ci.ch_str);
	if (!stat.pBon2->SetChannel(sp_num, ch_num)) {
		stat.pBon2->CloseTuner();
		dlclose(stat.hdll);
		output_message(MSG_ERROR, "SetChannel()に失敗しました");
		return 0;
	}

	stat.n_rem = 1;

	*chinfo = ci;
	pstat = (bondriver_stat_t*)malloc(sizeof(bondriver_stat_t));
	*pstat = stat;
	*param = pstat;
	return 1;
}

static void hook_stream_generator_cnr(void *param, double *cnr, signal_value_scale_t *scale)
{
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;
	*cnr = pstat->pBon2->GetSignalLevel();
	*scale = TSDUMP_SCALE_DECIBEL;
}

static void hook_stream_generator_close(void *param)
{
	bondriver_stat_t *pstat = (bondriver_stat_t*)param;
	pstat->pBon2->CloseTuner();
	pstat->pBon2->Release();
	dlclose(pstat->hdll);
}

static hooks_stream_generator_t hooks_stream_generator = {
	hook_stream_generator_open,
	hook_stream_generator,
	hook_stream_generator_wait,
	NULL,
	hook_stream_generator_cnr,
	hook_stream_generator_close
};

static void register_hooks()
{
	if (bon_dll_name) {
		reg_hook = register_hooks_stream_generator(&hooks_stream_generator);
	}
	register_hook_postconfig(hook_postconfig);
}

static const WCHAR *set_bon(const WCHAR* param)
{
	bon_dll_name = (const char *)strdup((const char *)param);
	return NULL;
}

static const WCHAR* set_sp(const WCHAR *param)
{
	sp_num = atoi((const char *)param);
	if (sp_num < 0) {
		return (const WCHAR*)"スペース番号が不正です";
	}
	return NULL;
}

static const WCHAR* set_ch(const WCHAR *param)
{
	ch_num = atoi((const char *)param);
	if (ch_num < 0) {
		return (const WCHAR*)"チャンネル番号が不正です";
	}
	return NULL;
}

static cmd_def_t cmds[] = {
	{ "--bon", "BonDriverのDLL *", 1, (cmd_handler_t)set_bon },
	{ "--sp", "チューナー空間番号 *", 1, (cmd_handler_t)set_sp },
	{ "--ch", "チャンネル番号 *", 1, (cmd_handler_t)set_ch },
	NULL,
};

TSD_MODULE_DEF(
	mod_bondriver,
	register_hooks,
	cmds,
	NULL
);
