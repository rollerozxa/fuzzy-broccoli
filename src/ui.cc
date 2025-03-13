#include "ui.hh"
#include <SDL.h>

#include "main.hh"
#include "game.hh"
#include "menu_main.hh"
#include "loading_screen.hh"
#include "game-message.hh"
#include "beam.hh"
#include "wheel.hh"
#include "pkgman.hh"
#include "object_factory.hh"
#include "settings.hh"
#include "simplebg.hh"
#include "soundmanager.hh"

#include <tms/core/tms.h>
#ifdef BUILD_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <sstream>

#define SAVE_REGULAR 0
#define SAVE_COPY 1

int ui::next_action = ACTION_IGNORE;

void
ui::message(const char *msg, bool long_duration)
{
#ifndef NO_UI
    pscreen::message->show(msg, long_duration ? 5.0 : 2.5);
#endif
}

/* always assume short duration */
void
ui::messagef(const char *format, ...)
{
    va_list vl;
    va_start(vl, format);

    char short_msg[256];
    const size_t sz = vsnprintf(short_msg, sizeof short_msg, format, vl) + 1;
    if (sz <= sizeof short_msg) {
        ui::message(short_msg, false);
    } else {
        char *long_msg = (char*)malloc(sz);
        vsnprintf(long_msg, sz, format, vl);
        ui::message(long_msg, false);
    }
}

#if !defined(PRINCIPIA_BACKEND_IMGUI)
void ui::render(){};
#endif

#if defined(NO_UI) || defined(TMS_BACKEND_EMSCRIPTEN)

int prompt_is_open = 0;
void ui::init(){};
void ui::open_dialog(int num, void *data/*=0*/){}
void ui::open_url(const char *url){};
void ui::open_help_dialog(const char*, const char*){};
void ui::emit_signal(int num, void *data/*=0*/){};
void ui::quit(){};
void ui::set_next_action(int action_id){};
void ui::open_error_dialog(const char *error_msg){};
void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
    P.add_action(action1.action_id, 0);
}
void ui::alert(const char*, uint8_t/*=ALERT_INFORMATION*/) {};

#elif defined(PRINCIPIA_BACKEND_IMGUI)

#include "ui_imgui.hh"

#elif defined(TMS_BACKEND_ANDROID)

#include "ui_android.hh"

#elif defined(TMS_BACKEND_PC)

#include "ui_gtk3.hh"

#else

#error "No dialog functions, to compile without please define NO_UI"

#endif
