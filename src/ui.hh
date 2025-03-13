#pragma once

#include "main.hh"

#include <stdint.h>

#define DIALOG_IGNORE          -1
#define DIALOG_SANDBOX_MENU     1
#define DIALOG_QUICKADD         100
#define DIALOG_SAVE             102
#define DIALOG_OPEN             103
#define DIALOG_NEW_LEVEL        104
#define DIALOG_CONFIRM_QUIT     107
#define DIALOG_OPEN_OBJECT      113
#define DIALOG_EXPORT           114
#define DIALOG_PUZZLE_PLAY      118
#define DIALOG_SETTINGS         121
#define DIALOG_SAVE_COPY        122
#define DIALOG_LEVEL_PROPERTIES 123
#define DIALOG_HELP             124
#define DIALOG_PLAY_MENU        126
#define DIALOG_OPEN_AUTOSAVE    127
#define DIALOG_COMMUNITY        128
#define DIALOG_REGISTER         137
#define DIALOG_PUBLISHED        138

#define DIALOG_SANDBOX_MODE     143

#define DIALOG_OPEN_STATE       156
#define DIALOG_MULTI_CONFIG     158

#define CLOSE_ALL_DIALOGS                  200
#define CLOSE_ABSOLUTELY_ALL_DIALOGS       201
#define CLOSE_REGISTER_DIALOG              202
#define DISABLE_REGISTER_LOADER            203

#define DIALOG_PUBLISH          300
#define DIALOG_LOGIN            301

#define SIGNAL_LOGIN_SUCCESS        100
#define SIGNAL_LOGIN_FAILED         101
#define SIGNAL_QUICKADD_REFRESH     200
#define SIGNAL_REFRESH_BORDERS      300

#define SIGNAL_REGISTER_SUCCESS     110
#define SIGNAL_REGISTER_FAILED      111

#define SIGNAL_ENTITY_CONSTRUCTED   406

enum {
    ALERT_INFORMATION,
};

enum {
    CONFIRM_TYPE_DEFAULT,
    CONFIRM_TYPE_BACK_SANDBOX,
};

struct confirm_data
{
    int confirm_type;

    confirm_data(int _confirm_type)
        : confirm_type(_confirm_type)
    { }
};

#ifdef __cplusplus
extern "C" {
#endif
const char* ui_get_property_string(int index);
void ui_set_property_string(int index, const char* val);
uint8_t ui_get_property_uint8(int index);
void ui_set_property_uint8(int index, uint8_t val);
uint32_t ui_get_property_uint32(int index);
void ui_set_property_uint32(int index, uint32_t val);
float ui_get_property_float(int index);
void ui_set_property_float(int index, float val);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class ui
{
  public:
    static int next_action;

    static void init();
    static void message(const char *str, bool long_duration=false);
    static void messagef(const char *str, ...);
    static void open_dialog(int num, void *data=0);
    static void open_help_dialog(const char *title, const char *description);
    static void open_error_dialog(const char *error_string);
    static void open_url(const char *url);
    static void emit_signal(int num, void *data=0);
    static void set_next_action(int action_id);
    static void set_fail_action(int action_id);
    static void set_cancel_action(int action_id);
    static void quit();
    static void confirm(const char *text,
            const char *button1, struct principia_action action1,
            const char *button2, struct principia_action action2,
            const char *button3=0, struct principia_action action3=ACTION_IGNORE,
            confirm_data cd=confirm_data(CONFIRM_TYPE_DEFAULT)
            );
    static void alert(const char *text, uint8_t alert_type=ALERT_INFORMATION);
    static void render();
};
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
extern int prompt_is_open;
#endif
