#pragma once

#include "misc.hh"
#include "pscreen.hh"
#include "entity.hh"
#include <tms/bindings/cpp/cpp.hh>
#include <set>
#include <map>
#include "panel.hh"
#include "types.hh"

#include "text.hh"
#include "world.hh"

#define SS_ACTION_DISCONNECT 0
#define SS_ACTION_SELECT     1

class activator;
class game_message;
class p_text;
class principia_wdg;
class plug;
class edevice;
class isocket;
class screenshot_marker;
class prompt;

enum {
    ERROR_NONE,
    ERROR_SOLVE,
    ERROR_RC_NO_WIDGETS,
    ERROR_RC_ACTIVATOR_INVALID,
    ERROR_SCRIPT_COMPILE,

    ERROR_COUNT,
};

enum {
    GW_PLAYPAUSE,
    GW_ORTHOGRAPHIC,
    GW_LAYERVIS,
    GW_MODE,
    GW_ADVANCED,
    GW_HELP,
    GW_ERROR,
    GW_DEFAULT_LAYER,

    GW_MULTISEL_IMPORT_EXPORT,
    GW_REMOVE,

    GW_INFO,
    GW_LAYER_DOWN_CYCLE,
    GW_LAYER_UP,
    GW_LAYER_DOWN,
    GW_TOGGLE_LOCK,
    GW_DETACH,
    GW_UNPLUG,
    GW_TOGGLE_AXIS_ROT,
    GW_TOGGLE_MOVEABLE,
    GW_CONFIG,
    GW_TRACKER,
    GW_TOGGLE_CLOCKWISE,
    GW_FREQ_UP,
    GW_FREQ_DOWN,

    GW_MENU,
    GW_QUICKADD,

    GW_FOLLOW_CONNECTIONS,
    GW_FOLLOW_CABLES,
    GW_ADDITIVE,
    GW_SELECT_THROUGH_LAYERS,
    GW_BOX_SELECT,

    GW_DISCONNECT_FROM_RC,
    GW_USERNAME,

    GW_CLOSE_PANEL_EDIT,
    GW_PANEL_SLIDER_KNOB,

    GW_TOOL_HELP,

    GW_IGNORE
};

struct game_debug_line
{
    float x1, y1, x2, y2;
    float r, g, b;
    int64_t life;
};

// dt required before normalizing camera movement
// 0.016 ~= game running at 60 fps
// 0.032 ~= game running at 30 fps
// 0.064 ~= game running at 15 fps
#define CAMERA_NORMALIZE_THRESHOLD 0.064
#define MAX_P 24

#define CLICK_MAX_STEPS            25

#define MAX_RECENT 6

enum {
    GAME_MODE_DEFAULT,
    GAME_MODE_MENU_DRAG,
    GAME_MODE_PLACE_NEW,
    GAME_MODE_SELECT_SOCKET,
    GAME_MODE_SELECT_CONN_TYPE,
    GAME_MODE_EDIT_PANEL,
    GAME_MODE_MULTISEL,
    GAME_MODE_SELECT_OBJECT,
    GAME_MODE_SELECT_CONN,
    GAME_MODE_QUICK_PLUG,
    GAME_MODE_ROTATE,
    GAME_MODE_GRAB,
    GAME_MODE_CONN_EDIT,
};

#define GAME_RESUME_NEW          0
#define GAME_RESUME_OPEN         1
#define GAME_RESUME_CONTINUE     2
#define GAME_RESUME_NEW_EMPTY    3

#define GAME_END_PROCEED 0
#define GAME_END_WARP    1

#define GAME_FADE_SPEED 5.f

typedef std::pair<entity*,entity*> c_key;
typedef std::map<c_key, connection*> c_map;
typedef std::set<entity*> entity_set;

#define NUM_CA 10
#define NUM_HL 20
#define NUM_HP 10

#define HL_TYPE_ERROR           (1U << 0)
#define HL_TYPE_PERSISTENT      (1U << 1)
#define HL_TYPE_MULTI           (1U << 2)
#define HL_TYPE_DO_FREE         (1U << 3)
#define HL_TYPE_TINT            (1U << 4)

#define HL_PRESET_NO_FREE       0U
#define HL_PRESET_NO_FREE_MULTI HL_TYPE_MULTI
#define HL_PRESET_DEFAULT       HL_TYPE_DO_FREE
#define HL_PRESET_DEFAULT_MULTI HL_TYPE_DO_FREE + HL_TYPE_MULTI

#define PUZZLE_TEST_PLAY 0
#define PUZZLE_SIMULATE  1

#define GAME_AUTOSAVE_INTERVAL 30000000llu /* 30 sec */
//#define GAME_AUTOSAVE_INTERVAL 1000000llu /* 1 sec */

class game;

extern float menu_xdim;
extern float menu_ydim;
extern int b_w;
extern int b_h;
extern int b_w_pad;
extern int b_h_pad;
extern int b_y;
extern int b_margin_y;
extern int b_margin_x;

#define OVERLAP_THRESHOLD .3f

struct menu_obj {
    entity *e;
    int pos;
    int category;
    bool highlighted;
    struct tms_sprite image;
    struct tms_sprite *name;
};

class _uncull_handler : public b2QueryCallback
{
  public:
    bool ReportFixture(b2Fixture *f);
};

class _box_select_handler : public b2QueryCallback
{
  public:
    bool ReportFixture(b2Fixture *f);
};

extern std::vector<struct menu_obj> menu_objects;

/* used to test the placement of a simple object, so the user does not
 * place objects inside other objects, or between two objects */
class overlap_query : public b2QueryCallback
{
  public:
    int desired_layerdist;
    b2Shape *test_sh;
    b2Body *test_bd;
    entity *test_e;

    bool overlap;
    bool ReportFixture(b2Fixture *fx);

    overlap_query(){overlap=false;};
};

class selection_handler
{
  public:
    entity            *e;
    connection        *c;
    std::set<entity*> *m;
    b2Body *b;
    tvec2 offs;
    uint8_t frame;

    entity            *e_saved;
    connection        *c_saved;
    std::set<entity*> *m_saved;
    tvec2 offs_saved;
    uint8_t frame_saved;

    int menu_width;

    selection_handler()
    {
        m = 0;
        e = 0;
        c = 0;
        b = 0;
        e_saved = 0;
        c_saved = 0;
        m_saved = 0;
        offs_saved = (tvec2){0.f, 0.f};
    }

    inline void save(){
        e_saved = e;
        c_saved = c;
        m_saved = m;
        frame_saved = frame;
        offs_saved = offs;
    }

    inline void load(){
        if (m_saved) {
            this->select(m);
        } else if (c_saved) {
            this->select(c_saved);
        } else
            this->select(e_saved, e_saved ? e_saved->get_body(frame_saved) : 0, offs_saved, frame_saved, true);

        this->reset();
    }

    inline void reset(){
        this->e_saved = 0;
        this->c_saved = 0;
    }

    inline bool enabled() {return e != 0 || c != 0 || m != 0;}
    void disable(bool refresh_widgets=true);

    void select(entity *e)
    {
        this->select(e, 0, (tvec2){0,0}, 0, false);
    }

    void select(entity *e, b2Body *b, tvec2 offs, uint8_t frame, bool ui);
    void select(connection *c);
    void select(entity_set *new_m);
};

class gamestate
{
  public:
    gamestate()
    {
        this->modified = false; /* if the level has been modified in sandbox since open/create */
        this->last_autosave_try = 0;
        this->test_playing = false;
        this->fade = 0.f;
        this->sandbox = true;
        this->finished = false;
        this->success = false;
        this->abo_architect_mode = false;
#ifdef DEBUG
        this->advanced_mode = true;
#else
        this->advanced_mode = false;
#endif
        this->finish_step = 0;
        this->waiting = false;
        this->gridsize = .25f;
        this->edit_layer = 0;
        this->edev_labels = false;
        this->m_score = 0;
        this->submitted_score = false;
        this->pkg = 0;
        this->bg_color = (tvec4){0,0,0,0};
        this->time_mul = 1.f;
        this->adventure_id = 0xffffffff;
        this->is_main_puzzle = false;
        this->new_adventure = false;
        this->step_num = 0;
    }

    uint32_t adventure_id;
    float   fade;
    uint64_t last_autosave_try;
    bool    abo_architect_mode; /* do not set manually, use the game set_architect_mode function */
    bool    advanced_mode;
    bool    modified;
    bool    sandbox;
    bool    test_playing;
    bool    edev_labels;
    bool    finished; /* whether the level has finished or not */
    bool    success; /* win or lose finish state */
    bool    ending;
    int     end_action;
    uint32_t end_warp;
    uint32_t finish_step; /* what step we finished on */
    bool    waiting; /* waiting for input, after a non-puzzle level has been loaded and to be played */
    float   time_mul;
    float   gridsize;
    int     edit_layer;
    int     m_score;
    bool    submitted_score;
    pkginfo *pkg;
    tvec4   bg_color;
    bool    is_main_puzzle;
    bool    new_adventure;
    uint32_t step_num;
};

struct fadeout_entity {
    entity  *e;
    bool     do_free;
    b2Vec2   velocity;
};
struct fadeout_event {
    std::vector<fadeout_entity> entities;
    float time;

    entity  *absorber;
    b2Vec2   absorber_point;
    uint8_t  absorber_frame;
};

#define GAME_TIME_MUL(x) (1.f - ((x)*.99))

struct er {
    entity *e;
    float alpha;
    uint8_t type;
    char *message;

    er() {
        e = 0;
        message = 0;
        alpha = 1.f;
        type = ERROR_NONE;
    }
};

class game : public pscreen
{
  private:
    void say_goodbye(b2Joint *j);

  public:
    panel::widget *active_hori_wdg;
    panel::widget *active_vert_wdg;
    int wdg_base_x;
    int wdg_base_y;

  protected:
    uint8_t opened_special_level;
    _uncull_handler uncull_handler;
    _box_select_handler box_select_handler;
    void init_background();
  public:
    std::set<er*> errors;
    struct tms_atlas *texts;
    p_text *text_small;
    entity *bgent;
    entity *grident;

    void refresh_widgets();
    bool widget_clicked(principia_wdg *w, uint8_t button_id, int pid);
    void refresh_info_label();
    /** Widgets **/
    principia_wdg *wdg_username;
    principia_wdg *wdg_playpause;
    principia_wdg *wdg_orthographic;
    principia_wdg *wdg_layervis;
    principia_wdg *wdg_mode;
    principia_wdg *wdg_advanced;
    principia_wdg *wdg_help;
    principia_wdg *wdg_default_layer;
    principia_wdg *wdg_multisel;
    principia_wdg *wdg_remove;
    principia_wdg *wdg_multimat;
    principia_wdg *wdg_connstrength;
    principia_wdg *wdg_conndamping;
    principia_wdg *wdg_menu;
    principia_wdg *wdg_quickadd;

    // entity selection
    principia_wdg *wdg_info;
    principia_wdg *wdg_layer;
    principia_wdg *wdg_lock;
    principia_wdg *wdg_detach;
    principia_wdg *wdg_unplug;
    principia_wdg *wdg_moveable;
    principia_wdg *wdg_config;
    principia_wdg *wdg_cwccw;
    principia_wdg *wdg_selection_slider[ENTITY_MAX_SLIDERS];
    p_text        *info_label;
    principia_wdg *wdg_follow_connections;
    principia_wdg *wdg_follow_cables;
    principia_wdg *wdg_additive;
    principia_wdg *wdg_select_through_layers;
    principia_wdg *wdg_box_select;
    principia_wdg *wdg_multi_help;

    principia_wdg *wdg_disconnect_from_rc;

    // GAME_MODE_EDIT_PANEL
    principia_wdg *wdg_close_panel_edit;
    principia_wdg *wdg_panel_slider_knob;
    p_text *help_dragpanel;

  protected:
    SDL_mutex *_lock;

    tvec3 last_static_update;
    bool do_static_update;

  public:
    struct tms_ddraw *dd;
    int force_static_update;

  protected:
    tms::camera *gi_cam;

  public:
    tvec3 cam_vel;
    b2Vec2 cam_rel_pos;
    b2Vec2 adv_rel_pos;
    tms::graph *graph;
    tms::graph *gi_graph;

    /* Activator points waiting to be rendered */
    std::deque<activator*> pending_activators;

    inline float get_time_mul(){return GAME_TIME_MUL(this->state.time_mul);};
    inline uint64_t timemul(uint64_t x){return (uint64_t)((double)x * (double)get_time_mul());};
    bool occupy_prompt_slot();
  protected:

    std::set<entity*> u_ghost;
    std::set<entity*> u_fastbody;
    std::set<entity*> u_grouped;
    std::set<entity*> u_custom;
    std::set<entity*> u_static;
    std::set<entity*> u_static_custom;
    std::set<entity*> u_joint_pivot;
    std::set<entity*> u_joint;

  public:
    std::set<entity*> u_effects;

  protected:

    inline bool
    shift_down()
    {
        return this->current_keymod & TMS_MOD_SHIFT;
    }

    inline bool
    ctrl_down()
    {
        return this->current_keymod & TMS_MOD_CTRL;
    }

    uint16_t current_keymod;
    uint16_t previous_keymod;

  public:
    inline void lock(){SDL_LockMutex(_lock);};
    inline void unlock(){SDL_UnlockMutex(_lock);};

    uint32_t previous_level;
    tvec2 tmp_ambientdiffuse;
    tms::camera *cam;

    gamestate state;
    void set_architect_mode(bool val);

    std::map<uint32_t, screenshot_marker*>::iterator cam_iterator;
    bool delete_level(int id_type, uint32_t id, uint32_t save_id);
    bool delete_partial(uint32_t id);
    bool autosave();
    bool save(bool force=false);
    bool save_copy(void);
    bool upload();
    void reselect();
    bool autosave_exists();

    entity *current_prompt;
    selection_handler selection;

    void update_ghost_entity(entity *ths);

    /** in-game control panel shit **/
    entity *current_panel;
    tms_wdg *w_panel_exit;
    void set_control_panel(entity *e);
    void setup_panel(panel *p);

    void set_copy_entity(uint8_t slot, entity *e);
    void copy_properties(entity *destination, entity *source, bool hl=false);

  protected:

    tms_fb *icon_fb;
    tms_fb *main_fb;

    tms::graph *outline_graph;

#ifdef TMS_BACKEND_PC
    /* entity fetched by hover query */
    entity *hov_ent;
    p_text *hov_text;
#endif

    /* pending for selection*/
  public:
    entity *sel_p_ent;
  protected:
    b2Body *sel_p_body;
    tvec2 sel_p_offs;
    uint8_t sel_p_frame;

    /* currently selected */

    float rot_offs;
    tvec2 rot_mouse_pos;
    b2Vec2 grab_mouse_pos;
    float rot_mouse_base;

    int _mode; /**< never set this manually **/
  public:
    void set_mode(int new_mode);
    inline int get_mode()
    {
        return this->_mode;
    }

    inline float get_zoom()
    {
        return this->cam->_position.z;
    }

    /** socket selection stuff **/
    edevice *ss_edev;
  protected:
    plug_base *ss_plug;
    bool ss_quickplug_step2;
    edevice *ss_asker;
    int ss_action;
    isocket *ss_socks[64];
    float ss_anim;
    int ss_num_socks;

    void perform_socket_action(int x);

    /** connection selection shit */
    connection *cs_conn;
    float cs_timer;
    bool cs_found;

    struct ca {
        float dir;
        float life;
        b2Vec2 p;
    } ca[NUM_CA];

    /** building stuff **/
    c_map pairs;

    float   score_highlight;

    void render_selected_entity(void);
    void render_selected_connection(void);
    void render_highlighted(void);
    void render_trails(void);
    void render_socksel(void);
    void select_socksel(int x);
    void render_conn_types(void);
    void render_gui(void);
    void render_selection_gui(void);
    void render_noselection_gui(void);
    void render_sandbox_menu(void);
    void render_connections(void);
    void render_existing_connections(void);
    void render_activators(void);
    void render_starred(void);

    entity *get_pending_ent();

  public:
    void reset_touch(bool hard=true);
    void reset_touch_gui();
    void do_play();
    void do_pause();
    void on_play();
    void on_pause();

    void refresh_gui(void);
    void window_size_changed();
    void create_sandbox_menu();
    void apply_level_properties();

    int get_menu_width();
    void set_menu_width(int new_menu_width);
    float get_bmenu_x();
    float get_bmenu_y();
    float get_bmenu_pad();

  protected:
    void clear_entities(void);
    void reset(void);

    void init_panel_edit(void);
    void panel_refresh_widgets();
    void init_sandbox_menu();
    struct tms_texture* get_sandbox_texture(int n);
    void add_menu_item(int cat, entity *e);
    int menu_handle_event(tms::event *ev);
    int panel_edit_handle_event(tms::event *ev);
    void render_panel_edit(void);
    void update_pairs();
    void panel_edit_refresh(void);
    bool panel_edit_need_scroll;

    p_text *numfeed_text;
    float numfeed_timer;
    char numfeed_str[32];

    /* add a connection animation, dir -1 or 1 */
    inline void add_ca(float dir, b2Vec2 p)
    {
        for (int x=0; x<NUM_CA; x++) {
            if (this->ca[x].life < 0.f || this->ca[x].life > 1.f) {
                this->ca[x].dir = dir;
                if (dir < 0.f) {
                    this->ca[x].life = 1.f;
                } else
                    this->ca[x].life = 0.f;
                this->ca[x].p = p;
                return;
            }
        }
    }

    void select_random_entity();

  public:
    void config_btn_pressed(entity *e);
    void info_btn_pressed(entity *e);
    void toggle_entity_lock(entity *e);
    void begin_tracker(entity *e);

    int delete_entity(entity *e);
    int delete_selected_entity(bool multi=false);
    void post_play_cleanup();
    void post_interact_select(entity *e);
    tvec3 light;
    float SMVP[16];
    float AOMVP[16];
    int resume_action;
    int resume_level_type;
    tms::screen *screen_back;
    int layer_vis;
    int layer_vis_saved;

    int dropping;
    uint32_t drop_step;
    uint8_t drop_amount;
    float drop_speed;

    bool mining[MAX_P];

    bool render_controls;
    tms::texture *tex_controls;

    struct hp {
        entity *e;
        float percent;
        float time;
        tvec3 color;
        bool regen;
        hp(){time=0;e=0;regen=false;};
    };

    void free_fadeout(fadeout_event *ev);

    struct hl {
        entity *e;
        entity_set *entities;
        float time;
        uint8_t type;
        hl(){
            time = 0.f;
            type = HL_PRESET_DEFAULT;
            e = 0;
            entities = 0;
        }
    };

    void back();

    int recent[MAX_RECENT];
    std::set<fadeout_event*> fadeouts;
    struct hl hls[NUM_HL];
    struct hp hps[NUM_HP];

    std::set<entity*> starred;
    std::set<entity*> locked;

    void add_error(entity *e, uint8_t error_type=ERROR_NONE, const char *message=0);
    void clear_errors();

    int last_cursor_pos_x;
    int last_cursor_pos_y;

    game();
    ~game();
    void init_framebuffers();
    void init_shaders();
    void init_graphs();
    void init_camera();
    void init_gui();


    int render();
    int post_render();
    int step(double dt);
    void refresh_last_cursor_pos();
    b2Vec2 get_last_cursor_pos(int layer);
    void update_last_cursor_pos(int x, int y);
    int interacting_with(entity *e);
    int is_mover_joint(b2Joint *j);
    bool do_drop_interacting; // delayed drop_interacting, to make sure it happens when the simulation is locked
    void drop_interacting(void);
    void drop_if_interacting(entity *e);
    void numkey_pressed(uint8_t key);
    int handle_input(tms::event *ev, int action);
    connection* apply_connection(connection *c, int option);
    int resume(void);
    int pause(void);
    void create_level(int type);

    bool player_can_build();

    struct multi_options {
        bool follow_connections;
        bool follow_cables;
        bool additive_selection;
        bool select_through_layers;
        b2Vec2 cursor;
        b2Vec2 cursor_size;
        lvledit *import;
        uint8_t box_select;

        multi_options()
        {
            this->import = 0;
            this->cursor_size.Set(.75f, .75f);
            this->cursor.SetZero();

            this->reset();
        }

        void reset()
        {
            this->follow_connections = true;
            this->follow_cables = false;
            this->additive_selection = false;
            this->select_through_layers = true;
            this->box_select = 0;
        }
    } multi;

    struct follow_options {
        tvec2 offset;
        bool linear;
        uint8_t offset_mode;
    } follow_options;

    entity *follow_object;
    void set_follow_object(entity *e, bool snap, bool preserve_pos=false);
    connection* set_connection_strength(connection *c, float strength); /* strength = value between 0.0 and 1.0, 1.0 being indestructible */
    void multiselect_perform(void (*cb)(entity*, void*), void *userdata=0);
    bool apply_multiselection(entity* e);

    void select_import_object(uint32_t id);
    void import_object(uint32_t id);
    void export_object(const char *name);
    void add_entities(std::map<uint32_t, entity*> *entities, std::map<uint32_t, group*> *groups, std::set<connection*> *connections, std::set<cable*> *cables);
    void add_entity(entity *e, bool soft=false);
    void destroy_mover(uint8_t x, bool do_not_deselect=false);
    void remove_entity(entity *e);
    entity* editor_construct_entity(uint32_t g_id, int pid=0, bool force_on_pid=false, b2Vec2 offs=b2Vec2(0.f,0.f));

    void check_all_entities();
    void puzzle_play(int type);

    inline void add_highlight_multi(entity_set *eset, uint8_t type=HL_PRESET_DEFAULT_MULTI, float t=1.f)
    {
        /* Look for free slot */
        struct hl *slot = 0;
        for (int x=0; x<NUM_HL; x++) {
            if (hls[x].time <= 0.f) {
                slot = &hls[x];
                break;
            } else if (hls[x].type & HL_TYPE_MULTI && hls[x].entities == eset) {
                slot = &hls[x];
                break;
            }
        }

        if (slot) {
            this->clear_hl(slot);

            slot->entities = eset;
            slot->time = t;
            slot->type = type;
        }
    }

    inline void clear_hl(struct hl *slot)
    {
        if (slot->type & HL_TYPE_MULTI && slot->entities) {
            if (slot->type & HL_TYPE_DO_FREE) {
                delete slot->entities;
            }
            slot->entities = 0;
        }
    }

    inline void add_highlight(entity *e, uint8_t type=HL_PRESET_DEFAULT, float t=1.f)
    {
        hl *found = 0;
        for (int x=0; x<NUM_HL; x++) {
            if (hls[x].time <= 0.f) {
                found = &hls[x];
            } else if (hls[x].e == e) {
                hls[x].time = fmaxf(t, hls[x].time);
                return;
            }
        }

        if (found) {
            this->clear_hl(found);

            found->time = t;
            found->e = e;
            found->type = type;
        }
    }

    inline void remove_highlight(entity *e)
    {
        for (int x=0; x<NUM_HL; x++) {
            if (hls[x].e == e) {
                hls[x].type = 0;
                hls[x].time = 0;
                return;
            }
        }
    }

    tvec3 pt[3], half_pt[3];
    int interact_select(entity *e);
    void check_select_object(int x, int y, int pid);
    bool check_click_socksel();
    bool check_quick_plug(uint64_t diff, int x, int y);
    bool check_click_conntype(int x, int y);
    bool check_click_conn(int x, int y);
    bool check_click_rotate(int x, int y);
    bool check_click_shape_resize(int x, int y);
    int handle_input_playing(tms::event *ev, int action);
    int handle_input_paused(tms::event *ev, int action);

    void handle_shape_resize(float x, float y);
    void render_shape_resize();
    int get_selected_shape_corner();

    void passthru_input(struct tms_event *ev);

    void apply_pending_connection(int n);

    void emit_partial_from_buffer(const char *buf, uint16_t buf_len, b2Vec2 position);
    void emit(entity *e, entity *emitter=0, b2Vec2 velocity=b2Vec2(0.f,0.f),bool immediate=false);
    void post_emit(entity *e, entity *emitter=0, b2Vec2 velocity=b2Vec2(0.f,0.f));
    bool absorb(entity *e, bool include_connection=false, entity *absorber=0, b2Vec2 absorber_point=b2Vec2(0.f, 0.f), uint8_t absorber_frame=0);
    bool timed_absorb(uint32_t id, double time);
    bool timed_absorb(entity *e, double time);
    void absorb(std::set<entity *> *loop);
    void destroy_joint(b2Joint *j);
    void add_destructable_joint(b2Joint *j, float max_force);
    void animate_disconnect(entity *e);

    bool ingame_layerswitch_test(entity *e, int dir);
    bool check_placement_allowed(entity *e);
    void recheck_all_placements(void);

    bool add_pair(entity *e1, entity *e2, connection *c);
    void return_tmp_conn(connection *c);

    void open_sandbox(int id_type, uint32_t id);
    void open_play(int id_type, uint32_t id, pkginfo *pkg, bool test_playing=false, int is_main_puzzle=0);

    void begin_play();

    void finish(bool success);
    void proceed();

    void update_entities();
    void update_static_entities();
    connection* get_tmp_conn(void);

    void play_sound(uint32_t sound_id, float x, float y, uint8_t random, float volume, bool loop=false, void *ident=0, bool global=false);

    // action 0 = disconnect
    // action 1 = select
    void open_socket_selector(entity *e, edevice *edev, int action=0);
    b2Joint* create_joint(b2JointDef *jd);

    inline void show_numfeed(float num, int precision=2)
    {
        this->numfeed_timer = 1.5f;
        sprintf(this->numfeed_str, "%.*f", precision, num);
    }

    void cam_move(float x, float y, float z);

#ifdef DEBUG
    std::set<game_debug_line*> debug_lines;
    void clamp_entities();
#endif

    void _multidelete();

    bool _restart_level;
    void restart_level();
    void destroy_possible_mover(entity *e);

    friend class selection_handler;
    friend class adventure;
    friend class world;
    friend class _uncull_handler;
};

extern game *G;

extern int gid_to_menu_pos[256];
extern tms_sprite *catsprites[of::num_categories+1];
extern tms_sprite *catsprite_hints[of::num_categories];
extern tms_sprite *filterlabel;
extern tvec2 move_pos;
