#pragma once

#include "edevice.hh"
#include "activator.hh"

struct widget_decl {
    bool dragging;
    int type;
    float dx; float dy;
    int sx; int sy;
    struct tms_sprite *s;
};

struct widget_info {
    int wdg_type;
    struct tms_sprite **s1;
    struct tms_sprite **s2;
    int sx; int sy;
    uint8_t num_outputs;
    float default_value[2];
    uint8_t version_required;
    bool can_save_default_value;
};

enum {
    PANEL_OK,
    PANEL_NO_ROOM,
};

#define PANEL_WIDGET_USED   (1ULL << 0)
#define PANEL_WIDGET_OUT0   (1ULL << 1)
#define PANEL_WIDGET_OUT1   (1ULL << 2)
#define PANEL_WIDGET_OUT2   (1ULL << 3)
#define PANEL_WIDGET_OUT3   (1ULL << 4)
#define PANEL_WIDGET_OUT4   (1ULL << 5)
#define PANEL_WIDGET_OUT5   (1ULL << 6)
#define PANEL_WIDGET_OUT6   (1ULL << 7)
#define PANEL_WIDGET_OUT7   (1ULL << 8)
#define PANEL_WIDGET_OWNED  (1ULL << 9)

#define PANEL_SMALL 0

enum {
    PANEL_SLIDER,
    PANEL_LEFT,
    PANEL_RIGHT,
    PANEL_UP,
    PANEL_DOWN,
    PANEL_BTN,
    PANEL_BIGSLIDER,
    PANEL_VSLIDER,
    PANEL_VBIGSLIDER,

    NUM_PANEL_WIDGET_TYPES
};

extern struct widget_info widget_data[NUM_PANEL_WIDGET_TYPES];

#define PANEL_MAX_WIDGETS 8

#define PANEL_WDG_OUTER_X (b_w_pad*1.2f)
#define PANEL_WDG_OUTER_Y (b_h_pad*1.2f)

class panel : public brcomp_multiconnect, public activator
{
  public:
    float light[6];
    int ptype;

    float get_sensor_radius()
    {
        return 1.25f;

    }

    b2Vec2 get_sensor_offset()
    {
        return b2Vec2(0.f, 0.f);
    }

    entity *get_activator_entity() { return this; }
    float get_activator_radius() { return this->get_sensor_radius(); }
    b2Vec2 get_activator_pos() { return this->get_position() + this->get_sensor_offset(); }

    void on_touch(b2Fixture *my, b2Fixture *other)
    {
        if (my == this->fx_sensor) {
            this->activator_touched(other);
        }
    }

    void on_untouch(b2Fixture *my, b2Fixture *other)
    {
        if (my == this->fx_sensor) {
            this->activator_untouched(other);
        }
    }

    int num_feed;
    int num_set;
    int has_focus;

    class widget : public tms_wdg
    {
      public:
        panel *p;
        int index;
        float default_value[2];
        uint32_t outputs;
        uint8_t sock[9];
        uint8_t num_socks;
        uint8_t num_outputs;
        uint8_t wtype;
        bool used;
        bool owned;
        bool first_click;
        struct glyph *glyph;

        inline bool is_slider() { return (this->type == TMS_WDG_SLIDER); }
        inline bool is_vslider() { return (this->type == TMS_WDG_VSLIDER); }
        inline bool is_radial() { return (this->type == TMS_WDG_RADIAL); }
        inline bool is_field() { return (this->type == TMS_WDG_FIELD); }
    };
    widget widgets[PANEL_MAX_WIDGETS];
    int widgets_in_use;
    int num_widgets;

    panel();
    void init_smallpanel();

    void pre_write(void);
    void on_load(bool created);
    void remove_widget(int index);
    void init_widget(panel::widget *w);
    int add_widget(struct widget_decl decl, int x, int y, int z);
    bool slot_used(int x, int y, int z);
    bool slot_owned_by_radial(int x, int y, int z);
    edevice* solve_electronics(void);
    void setup();
    void update_panel_key_labels();
    void panel_disconnected();

    const char* get_name(){
        return "RC Basic";
    }
    activator *get_activator() { return this; }

};
