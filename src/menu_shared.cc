#include "menu_shared.hh"
#include "menu_main.hh"
#include "text.hh"
#include "gui.hh"
#include "widget_manager.hh"
#include "version.hh"

tms::texture *menu_shared::tex_bg;
tms::texture *menu_shared::tex_vignette;
tms::texture *menu_shared::tex_menu_bottom;
tms::texture *menu_shared::tex_principia;
tms::texture *menu_shared::tex_vert_line;
tms::texture *menu_shared::tex_hori_line;

float menu_shared::fl_alpha = 0.f;
float menu_shared::contest_alpha = 0.f;
float menu_shared::gs_alpha = 0.f;

p_text *menu_shared::text_version;
p_text *menu_shared::text_message;

int menu_shared::bar_height;

void
menu_shared::init()
{
    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->load("data/textures/menu/menu_bg.jpg");
        tex->colors = GL_RGB;

        tex->upload();

        menu_shared::tex_bg = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/menu_bottom.png");

        tex->upload();

        menu_shared::tex_menu_bottom = tex;
    }

    {
        tms::texture *tex = new tms::texture();

        tex->gamma_correction = 0;
        tex->colors = GL_RGBA;

        tex->load("data/textures/menu/menu_principia.png");

        tex->upload();

        menu_shared::tex_principia = tex;
    }

    menu_shared::text_version = new p_text(font::medium, ALIGN_CENTER, ALIGN_CENTER);
    menu_shared::text_version->set_text(principia_version_string());

    float h = (gui_spritesheet::get_sprite(S_CONFIG)->height / 64.f) * 0.5f * _tms.yppcm * .7f;

    menu_shared::bar_height = std::max(menu_shared::text_version->get_height() + (_tms.yppcm * .125f), h);
}


void
menu_shared::step()
{

}
