#ifndef __APP_FN_H__
#define __APP_FN_H__

enum {
	APP_FN_OK = 0,
	APP_FN_ERR,
	APP_FN_DONE,
	NUM_APP_FN_RETS,
};

int app_fn_proc(void);

void app_fn_next(void);
void app_fn_exit(void);
void app_fn_enter(void);

void app_fn_btn_plus(void);
void app_fn_btn_minus(void);

#endif /* __APP_FN_H__ */
