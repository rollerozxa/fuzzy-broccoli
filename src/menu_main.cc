#include "menu_main.hh"
#include "game.hh"
#include "gui.hh"
#include "menu_shared.hh"
#include "misc.hh"
#include "text.hh"
#include "ui.hh"
#include "widget_manager.hh"

bool
menu_main::widget_clicked(principia_wdg *w, uint8_t button_id, int pid)
{
    if (menu_base::widget_clicked(w, button_id, pid)) {
        return true;
    }

    switch (button_id) {
        case BTN_PLAY:
            P.add_action(ACTION_MAIN_MENU_PKG, 0);
            break;

        case BTN_CREATE:
            G->resume_action = GAME_RESUME_NEW_EMPTY;
            G->resume_level_type = LCAT_CUSTOM;
            G->screen_back = 0;
            tms::set_screen(G);
            break;

        case BTN_BROWSE_COMMUNITY: {
            COMMUNITY_URL("");
            ui::open_url(url);
        } break;

        case BTN_UPDATE: {
            COMMUNITY_URL("download");
            ui::open_url(url);
        } break;

        default: return false;
    }

    return true;
}

menu_main::menu_main()
    : menu_base(true)
{
    this->wdg_settings->set_tooltip("Settings");
    this->wdg_settings->add();

    this->wdg_update_available = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_UPDATE, AREA_TOP_CENTER);
    this->wdg_update_available->set_label("Update available!");
    this->wdg_update_available->priority = 900;

    this->wdg_play = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_PLAY, AREA_MENU_TOP_CENTER,
            gui_spritesheet::get_sprite(S_MENU_PLAY));
    this->wdg_play->priority = 1000;
    this->wdg_play->add();

    this->wdg_create = this->wm->create_widget(
            this->get_surface(), TMS_WDG_BUTTON,
            BTN_CREATE, AREA_MENU_TOP_CENTER,
            gui_spritesheet::get_sprite(S_MENU_CREATE));
    this->wdg_create->priority = 900;
    this->wdg_create->add();

    this->wdg_browse_community = this->wm->create_widget(
            this->get_surface(), TMS_WDG_LABEL,
            BTN_BROWSE_COMMUNITY, AREA_MENU_SUB_LEVELS);
    this->wdg_browse_community->priority = 1000;
    this->wdg_browse_community->set_label("Browse more community levels", font::xmedium);
    this->wdg_browse_community->render_background = true;
    this->wdg_browse_community->label->color.a = 1.f;
    this->wdg_browse_community->label->outline_color.a = 0.f;
    this->wdg_browse_community->add();

    this->refresh_widgets();
}

int
menu_main::resume()
{
    menu_base::resume();
    this->refresh_widgets();

    return T_OK;
}

int
menu_main::pause(void)
{
    return T_OK;
}

int
menu_main::render(void)
{
#ifdef SCREENSHOT_BUILD
    return T_OK;
#endif

    menu_base::render();

    glViewport(
            100, 100,
            500, 500);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(
            0, 0,
            _tms.opengl_width, _tms.opengl_height);

    return T_OK;
}

int
menu_main::handle_input(tms::event *ev, int action)
{
#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
    if (ev->type == TMS_EV_POINTER_DOWN) {
        if (!P.focused) {
            if (prompt_is_open) return T_OK;
            else P.focused = 1;
        }
    }
#endif

    if (pscreen::handle_input(ev, action) == EVENT_DONE) {
        return EVENT_DONE;
    }

    if (ev->type == TMS_EV_KEY_PRESS) {
        switch (ev->data.key.keycode) {
            case TMS_KEY_1:
            case TMS_KEY_P:
                this->wdg_play->click();
                return T_OK;

            case TMS_KEY_2:
            case TMS_KEY_C:
                this->wdg_create->click();
                return T_OK;

            case TMS_KEY_3:
                this->wdg_browse_community->click();
                return T_OK;

            case TMS_KEY_R:
                this->refresh_widgets();
                return T_OK;

            case TMS_KEY_V:
                ui::messagef("Community host: %s", P.community_host);
                return T_OK;

#ifdef DEBUG
            case TMS_KEY_S:
                G->create_sandbox_menu();
                return T_OK;

#endif

            case SDL_SCANCODE_AC_BACK:
                ui::quit();
                return T_OK;
        }
    }
    return T_OK;
}

int
menu_main::step(double dt)
{
    menu_base::step(dt);

#ifdef SCREENSHOT_BUILD
    SDL_Delay(150);
    return T_OK;
#endif

    this->wm->step();

    return T_OK;
}

void
menu_main::refresh_widgets()
{
#ifdef SCREENSHOT_BUILD
    this->wm->remove_all();
#else
    menu_base::refresh_widgets();

    this->wdg_update_available->remove();

    if (P.new_version_available) {
        this->wdg_update_available->add();
    }

#endif

    this->wm->rearrange();
}
