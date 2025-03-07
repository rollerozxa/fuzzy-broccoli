
#ifdef TMS_BACKEND_PC

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

// fuckgtk3
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef USE_GTK_SOURCE_VIEW
#include <gtksourceview/gtksource.h>
#endif

#ifdef TMS_BACKEND_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

static gboolean _close_all_dialogs(gpointer unused);

SDL_bool   ui_ready = SDL_FALSE;
SDL_cond  *ui_cond;
SDL_mutex *ui_lock;
static gboolean _sig_ui_ready(gpointer unused);

typedef std::map<int, std::pair<int, int> > freq_container;

enum {
    LF_MENU,
    LF_ITEM,
    LF_DECORATION,

    NUM_LF
};

/* open window columns */
enum {
    OC_ID,
    OC_NAME,
    OC_VERSION,
    OC_DATE,

    OC_NUM_COLUMNS
};

enum {
    OSC_ID,
    OSC_NAME,
    OSC_DATE,
    OSC_SAVE_ID,
    OSC_ID_TYPE,

    OSC_NUM_COLUMNS
};

enum {
    FC_FREQUENCY,
    FC_RECEIVERS,
    FC_TRANSMITTERS,

    FC_NUM_COLUMNS
};

enum {
    RESPONSE_PUZZLE,
    RESPONSE_EMPTY_ADVENTURE,
    RESPONSE_ADVENTURE,
    RESPONSE_CUSTOM,
};

enum {
    RESPONSE_CONN_EDIT,
    RESPONSE_MULTISEL,
    RESPONSE_DRAW,
};

typedef struct {
    uint32_t id;
    gchar   *name;
    long   time;
} oc_column;

int prompt_is_open = 0;
GtkDialog *cur_prompt = 0;

enum mark_type {
    MARK_ENTITY,
    MARK_POSITION,
};

struct goto_mark {
    mark_type type;
    const char *label;
    uint32_t id;
    tvec2 pos;
    GtkMenuItem *menuitem;
    guint key;

    goto_mark(mark_type _type, const char *_label, uint32_t _id, tvec2 _pos)
        : type(_type)
        , label(_label)
        , id(_id)
        , pos(_pos)
        , menuitem(0)
        , key(0)
    { }
};

/** --Menu **/
GtkMenu         *editor_menu;
static uint8_t   editor_menu_on_entity = 0;
GtkMenuItem     *editor_menu_header;
/* ---------------------- */
GtkMenuItem     *editor_menu_move_here_player;
GtkMenuItem     *editor_menu_move_here_object;
GtkMenuItem     *editor_menu_go_to; /* submenu */
GtkMenu         *editor_menu_go_to_menu;
/* -------------------------- */
GtkMenuItem     *editor_menu_set_as_player;
GtkMenuItem     *editor_menu_toggle_mark_entity;
/* -------------------------- */
GtkMenuItem     *editor_menu_lvl_prop;
GtkMenuItem     *editor_menu_save;
GtkMenuItem     *editor_menu_save_copy;
GtkMenuItem     *editor_menu_publish;
GtkMenuItem     *editor_menu_settings;
GtkMenuItem     *editor_menu_login;
struct goto_mark *editor_menu_last_created = new goto_mark(MARK_ENTITY, "Last created entity", 0, tvec2f(0.f, 0.f));
struct goto_mark *editor_menu_last_cam_pos = new goto_mark(MARK_POSITION, "Last camera position", 0, tvec2f(0.f, 0.f));
static std::deque<struct goto_mark*> editor_menu_marks;

static guint valid_keys[9] = {
    GDK_KEY_1,
    GDK_KEY_2,
    GDK_KEY_3,
    GDK_KEY_4,
    GDK_KEY_5,
    GDK_KEY_6,
    GDK_KEY_7,
    GDK_KEY_8,
    GDK_KEY_9
};

static void
refresh_mark_menuitems()
{
    GtkAccelGroup *accel_group = gtk_menu_get_accel_group(editor_menu);
    int x=0;

    for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
            it != editor_menu_marks.end(); ++it) {
        struct goto_mark* mark = *it;
        GtkMenuItem *item = mark->menuitem;
        if (x < 9) {
            mark->key = valid_keys[x];
            gtk_widget_add_accelerator (GTK_WIDGET(item), "activate", accel_group,
                    valid_keys[x], (GdkModifierType)0, GTK_ACCEL_VISIBLE);

            ++ x;
        } else {
            mark->key = 0;
        }
    }
}

/** --Play menu **/
GtkMenu         *play_menu;

/** --Open state **/
GtkWindow    *open_state_window;
GtkTreeModel *open_state_treemodel;
GtkTreeView  *open_state_treeview;
GtkButton    *open_state_btn_open;
GtkButton    *open_state_btn_cancel;
static bool   open_state_no_testplaying = false;

/** --Multi config **/
GtkWindow    *multi_config_window;
GtkNotebook  *multi_config_nb;
GtkButton    *multi_config_apply;
GtkButton    *multi_config_cancel;
int           multi_config_cur_tab = 0;
enum {
    TAB_JOINT_STRENGTH,
    TAB_PLASTIC_COLOR,
    TAB_PLASTIC_DENSITY,
    TAB_CONNECTION_RENDER_TYPE,
    TAB_MISCELLANEOUS,

    NUM_MULTI_CONFIG_TABS
};
/* Joint strength */
GtkScale    *multi_config_joint_strength;
/* Plastic color */
GtkColorChooserWidget *multi_config_plastic_color;
/* Plastic density */
GtkScale    *multi_config_plastic_density;
/* Connection render type */
GtkRadioButton  *multi_config_render_type_normal;
GtkRadioButton  *multi_config_render_type_small;
GtkRadioButton  *multi_config_render_type_hide;
/* Miscellaneous */
GtkButton       *multi_config_unlock_all;
GtkButton       *multi_config_disconnect_all;

/** --Open level **/
GtkWindow    *open_window;
GtkTreeModel *open_treemodel;
GtkTreeView  *open_treeview;
GtkButton    *open_btn_open;
GtkButton    *open_btn_cancel;
GtkMenu      *open_menu;
GtkMenuItem  *open_menu_information;
GtkMenuItem  *open_menu_delete;

/** --Open object **/
bool         object_window_multiemitter;
GtkWindow    *object_window;
GtkTreeModel *object_treemodel;
GtkTreeView  *object_treeview;
GtkButton    *object_btn_open;
GtkButton    *object_btn_cancel;

/* --Save and Save as copy */
GtkWindow *save_window;
GtkEntry  *save_entry;
GtkLabel  *save_status;
GtkButton *save_ok;
GtkButton *save_cancel;
uint8_t    save_type = SAVE_REGULAR;

/* --Export */
GtkWindow *export_window;
GtkEntry  *export_entry;
GtkLabel  *export_status;
GtkButton *export_ok;
GtkButton *export_cancel;

/** --Package manager **/
GtkWindow       *package_window;
GtkTreeModel    *pk_pkg_treemodel;
GtkTreeView     *pk_pkg_treeview;
GtkCheckButton  *pk_pkg_first_is_menu;
GtkCheckButton  *pk_pkg_return_on_finish;
GtkSpinButton   *pk_pkg_unlock_count;
//GtkWidget       *pk_pkg_delete;
GtkWidget       *pk_pkg_create;
GtkWidget       *pk_pkg_play;
GtkWidget       *pk_pkg_publish;
GtkTreeModel    *pk_lvl_treemodel;
GtkTreeView     *pk_lvl_treeview;
GtkWidget       *pk_lvl_add;
GtkWidget       *pk_lvl_del;
GtkWidget       *pk_lvl_play;
bool pk_ignore_lvl_changes = true;

/* --Package name dialog */
GtkDialog *pkg_name_dialog;
GtkEntry  *pkg_name_entry;
GtkButton *pkg_name_ok;

/** --Level properties **/
GtkDialog       *properties_dialog;
GtkButton       *lvl_ok;
GtkButton       *lvl_cancel;
GtkRadioButton  *lvl_radio_adventure;
GtkRadioButton  *lvl_radio_custom;
GtkEntry        *lvl_title;
GtkTextView     *lvl_descr;
GtkComboBoxText *lvl_bg;
GtkColorButton  *lvl_bg_color;
uint32_t         new_bg_color;
GtkEntry        *lvl_width_left;
GtkEntry        *lvl_width_right;
GtkEntry        *lvl_height_down;
GtkEntry        *lvl_height_up;
GtkButton       *lvl_autofit;
GtkSpinButton   *lvl_gx;
GtkSpinButton   *lvl_gy;
GtkScale       *lvl_pos_iter;
GtkScale       *lvl_vel_iter;
GtkScale       *lvl_prismatic_tol;
GtkScale       *lvl_pivot_tol;
GtkScale       *lvl_linear_damping;
GtkScale       *lvl_angular_damping;
GtkScale       *lvl_joint_friction;

GtkEntry        *lvl_score;
GtkCheckButton  *lvl_pause_on_win;
GtkCheckButton  *lvl_show_score;
GtkButton       *lvl_upgrade;

enum ROW_TYPES {
    ROW_CHECKBOX,
    ROW_HSCALE,
};

struct setting_row_type
{
    int type;

    /* hscale */
    double min;
    double max;
    double step;

    static const struct setting_row_type
    create_checkbox()
    {
        struct setting_row_type srt;
        srt.type = ROW_CHECKBOX;

        return srt;
    }

    static const struct setting_row_type
    create_hscale(double min, double max, double step)
    {
        struct setting_row_type srt;
        srt.type = ROW_HSCALE;

        srt.min = min;
        srt.max = max;
        srt.step = step;

        return srt;
    }
};

struct table_setting_row {
    const char *label;
    const char *help;
    const char *setting_name;
    const struct setting_row_type row;
    GtkWidget *wdg;
};

struct table_setting_row settings_graphic_rows[] = {
    {
        "Vertical sync",
        0,
        "vsync",
        setting_row_type::create_checkbox()
    }, {
        "Gamma correction",
        0,
        "gamma_correct",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_audio_rows[] = {
    {
        "Volume",
        "Master volume",
        "volume",
        setting_row_type::create_hscale(0.0, 1.0, 0.05),
    }, {
        "Mute all sounds",
        0,
        "muted",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_control_rows[] = {
    {
        "Enable cursor jail",
        "Enable this if you want the cursor to be locked to the game while playing a level.",
        "jail_cursor",
        setting_row_type::create_checkbox()
    }, {
        "Smooth camera",
        "Whether the camera movement should be smooth or direct.",
        "smooth_cam",
        setting_row_type::create_checkbox()
    }, {
        "Camera speed",
        "How fast you can move the camera.",
        "cam_speed_modifier",
        setting_row_type::create_hscale(0.1, 15.0, 0.5),
    }, {
        "Smooth zoom",
        "Whether the zooming should be smooth or direct.",
        "smooth_zoom",
        setting_row_type::create_checkbox()
    }, {
        "Zoom speed",
        "How fast you can zoom in your level.",
        "zoom_speed",
        setting_row_type::create_hscale(0.1, 3.0, 0.5),
    }, {
        "Smooth menu",
        "Whether the menu scrolling should be smooth or direct.",
        "smooth_menu",
        setting_row_type::create_checkbox()
    }, {
        "Menu scroll speed",
        "How fast you can scroll through the menu.",
        "menu_speed",
        setting_row_type::create_hscale(1.0, 15.0, 0.5),
    }, {
        "Widget sensitivity",
        "Controls the mouse-movement-sensitivity used to control sliders, radials and fields using the hotkey mode.",
        "widget_control_sensitivity",
        setting_row_type::create_hscale(0.1, 8.0, 0.25),
    }, {
        "Enable RC cursor lock",
        "Lock the cursor if you active an RC widgets mouse control.",
        "rc_lock_cursor",
        setting_row_type::create_checkbox()
    }, {
        "Emulate touch device",
        "Enable this if you use an external device other than a mouse to control Principia, such as a Wacom pad.",
        "emulate_touch",
        setting_row_type::create_checkbox()
    },
};

struct table_setting_row settings_interface_rows[] = {
    {
        "UI scale",
        "A restart is required for this change to take effect",
        "uiscale",
        setting_row_type::create_hscale(0.25, 2.0, 0.05),
    },{
        "Fullscreen mode",
        "Toggle fullscreen mode",
        "window_fullscreen",
        setting_row_type::create_checkbox()
    }, {
        "Display object ID",
        "Display ID of object on selection (bottom-left corner).",
        "display_object_id",
        setting_row_type::create_checkbox()
    }, {
        "Resizable window",
        "Allow the window to be resized. NOTE: Principia does not support resizing while in-game. Things will break.",
        "window_resizable",
        setting_row_type::create_checkbox()
    }, {
        "Autosave screen size",
        "Save the screen size when resizing the window.",
        "autosave_screensize",
        setting_row_type::create_checkbox()
    },
};

static const int settings_num_graphic_rows = sizeof(settings_graphic_rows) / sizeof(settings_graphic_rows[0]);
static const int settings_num_audio_rows = sizeof(settings_audio_rows) / sizeof(settings_audio_rows[0]);
static const int settings_num_control_rows = sizeof(settings_control_rows) / sizeof(settings_control_rows[0]);
static const int settings_num_interface_rows = sizeof(settings_interface_rows) / sizeof(settings_interface_rows[0]);

struct gtk_level_property {
    uint64_t flag;
    const char *label;
    const char *help;
    GtkCheckButton *checkbutton;
};

struct gtk_level_property gtk_level_properties[] = {
    { LVL_DISABLE_LAYER_SWITCH,
      "Disable layer switch",
      "If adventure mode, disable manual layer switching of the robots.\nIf puzzle mode, disable layer switching of objects." },
    { LVL_DISABLE_INTERACTIVE,
      "Disable interactive",
      "Disable the ability to handle interactive objects." },
    { LVL_DISABLE_CONNECTIONS,
      "Disable connections",
      "Puzzle mode only, disable the ability to create connections." },
    { LVL_DISABLE_STATIC_CONNS,
      "Disable static connections",
      "Puzzle mode only, disable connections to static objects such as platforms." },
    { LVL_DISABLE_ZOOM,
      "Disable zoom",
      "Disable the players ability to zoom." },
    { LVL_DISABLE_CAM_MOVEMENT,
      "Disable cam movement",
      "Disable the players ability to manually move the camera." },
    { LVL_DISABLE_INITIAL_WAIT,
      "Disable initial wait",
      "Disable the waiting state when a level is started." },
    { LVL_ENABLE_INTERACTIVE_DESTRUCTION,
      "Interactive destruction",
      "If enabled, interactive objects can be destroyed by shooting them a few times or blowing them up." },
    { LVL_SNAP,
      "Snap by default",
      "For puzzle levels, when the player drags or rotates an object it will snap to a grid by default (good for easy beginner levels)." },
    { LVL_NAIL_CONNS,
      "Hide beam connections",
      "Use less visible nail-shaped connections for planks and beams. Existing connections will not be changed if this flag is changed." },
    { LVL_SINGLE_LAYER_EXPLOSIONS,
      "Single-layer explosions",
      "Enable this flag to prevent explosions from reaching objects in other layers." },
    { LVL_DISABLE_RC_CAMERA_SNAP,
      "Disable RC camera snap",
      "If enabled, the camera won't move to any selected RC." },
    { LVL_DISABLE_PHYSICS,
      "Disable physics",
      "If enabled, physics simulation in the level will be disabled." },
    { LVL_CHUNKED_LEVEL_LOADING,
      "Chunked level loading",
      "Splits up the level into chunks, leading to better performance for large levels." },
    { LVL_DISABLE_ENDSCREENS,
      "Disable end-screens",
      "Disable any end-game sound or messages. Works well when Pause on WIN is disabled. Note that this also disabled the score submission button." },
};

static int num_gtk_level_properties = sizeof(gtk_level_properties) / sizeof(gtk_level_properties[0]);

/** --Publish **/
GtkDialog      *publish_dialog;
GtkEntry       *publish_name;
GtkTextView    *publish_descr;
GtkCheckButton *publish_locked;

/** --New level **/
GtkDialog      *new_level_dialog;

/** --Mode **/
GtkDialog      *mode_dialog;

/** --Item **/
GtkDialog       *item_dialog;
GtkComboBoxText *item_cb;


/** --Quickadd **/
GtkWindow *quickadd_window;
GtkEntry  *quickadd_entry;

/** --Info Dialog **/
GtkWindow       *info_dialog;
GtkLabel        *info_name;
GtkLabel        *info_text;
char            *_pass_info_descr;
char            *_pass_info_name;

/** --Error Dialog **/
GtkDialog       *error_dialog;
GtkLabel        *error_text;
char            *_pass_error_text;

/** --Confirm Dialog **/
GtkDialog       *confirm_dialog;
GtkLabel        *confirm_text;
GtkButton       *confirm_button1;
GtkButton       *confirm_button2;
GtkButton       *confirm_button3;
struct confirm_data confirm_data(CONFIRM_TYPE_DEFAULT);
char            *_pass_confirm_text;
char            *_pass_confirm_button1;
char            *_pass_confirm_button2;
char            *_pass_confirm_button3;
int              confirm_action1;
int              confirm_action2;
int              confirm_action3;
void            *confirm_action1_data = 0;
void            *confirm_action2_data = 0;
void            *confirm_action3_data = 0;

/** --Alert Dialog **/
GtkMessageDialog    *alert_dialog;
char                *_alert_text = 0;
uint8_t              _alert_type;

/** --Tips Dialog **/
GtkDialog       *tips_dialog;
GtkLabel        *tips_text;
GtkCheckButton  *tips_hide;

/** --Autosave Dialog **/
GtkDialog       *autosave_dialog;

/** --Login **/
GtkWindow       *login_window;
GtkEntry        *login_username;
GtkEntry        *login_password;
GtkLabel        *login_status;
GtkButton       *login_btn_log_in;
GtkButton       *login_btn_cancel;
GtkButton       *login_btn_register;

/** --Settings **/
GtkDialog       *settings_dialog;

/* Graphics */
GtkCheckButton  *settings_enable_shadows;
GtkSpinButton   *settings_shadow_quality;
GtkComboBoxText *settings_shadow_res;
//GtkSpinButton   *settings_ao_quality;
GtkCheckButton  *settings_enable_ao;
GtkComboBoxText *settings_ao_res;

/** --Confirm Quit Dialog **/
GtkDialog       *confirm_quit_dialog;
GtkButton       *confirm_btn_quit;

/** --Level upgrade Dialog **/
GtkDialog       *confirm_upgrade_dialog;

/** --Puzzle play **/
GtkDialog       *puzzle_play_dialog;

/** --Published **/
GtkDialog       *published_dialog;

/** --Community **/
GtkDialog       *community_dialog;

static gboolean
on_window_close(GtkWidget *w, void *unused)
{
    P.focused = true;
    gtk_widget_hide(w);
    return true;
}

/* Generate help widget with a tooltip */
static GtkWidget* help_widget(const char *text) {
    //help-about
    //help-browser-symbolic
    //dialog-information-symbolic
    GtkWidget *r = gtk_image_new_from_icon_name("help-about", GTK_ICON_SIZE_MENU); //16px
    gtk_widget_set_tooltip_text(r, text);

    return r;
}

static GtkCellRenderer*
add_text_column(GtkTreeView *tv, const char *title, int id)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    renderer = GTK_CELL_RENDERER(gtk_cell_renderer_text_new());
    column = GTK_TREE_VIEW_COLUMN(gtk_tree_view_column_new_with_attributes(title, renderer, "text", id, NULL));

    gtk_tree_view_column_set_sort_column_id(column, id);
    gtk_tree_view_append_column(tv, column);

    return renderer;
}

static GtkWidget*
new_lbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);

    return r;
}

static void
clear_cb(GtkComboBoxText *cb)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    gtk_list_store_clear(GTK_LIST_STORE(model));
}

static GtkCheckButton*
new_check_button(const char *lbl)
{
    GtkCheckButton *ret = GTK_CHECK_BUTTON(gtk_check_button_new_with_label(lbl));

    return ret;
}

static GtkButton*
new_lbtn(const char *text, gboolean (*on_click)(GtkWidget*, GdkEventButton*, gpointer))
{
    GtkButton *btn = GTK_BUTTON(gtk_button_new_with_label(text));
    g_signal_connect(btn, "clicked",
            G_CALLBACK(on_click), 0);

    return btn;
}

static GtkWidget*
new_clbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
}

static GtkWidget*
new_rlbl(const char *text)
{
    GtkWidget *r = gtk_label_new(0);
    gtk_label_set_markup(GTK_LABEL(r), text);
    gtk_label_set_xalign(GTK_LABEL(r), 1.0f);
    gtk_label_set_yalign(GTK_LABEL(r), 0.5f);
    return r;
}

static void
notebook_append(GtkNotebook *nb, const char *title, GtkBox *base)
{
    gtk_notebook_append_page(nb, GTK_WIDGET(base), new_lbl(title));
}

static void apply_dialog_defaults(
    void *w,
    GtkCallback on_show=0,
    gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0
) {
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    //gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);
    g_signal_connect(w, "delete-event", G_CALLBACK(on_window_close), 0);

    if (on_show) {
        g_signal_connect(w, "show", G_CALLBACK(on_show), 0);
    }
    if (on_keypress) {
        g_signal_connect(w, "key-press-event", G_CALLBACK(on_keypress), 0);
    }
}

static GtkGrid* create_settings_table() {
    GtkGrid *tbl = GTK_GRID(gtk_grid_new());

    gtk_grid_set_column_spacing(tbl, 15);
    gtk_grid_set_row_spacing(tbl, 6);

    gtk_grid_set_column_homogeneous(tbl, false);
    gtk_grid_set_row_homogeneous(tbl, false);

    g_object_set (
        G_OBJECT(tbl),
        "margin", 10,
        NULL
    );

    return tbl;
}

static void add_setting_row(GtkGrid *tbl, int y, const char *label, GtkWidget *widget, const char *help_text = NULL) {
    //label
    gtk_grid_attach(
        tbl, new_rlbl(label),
        0, y,
        1, 1
    );


    //widget
    gtk_widget_set_hexpand(widget, true);
    gtk_grid_attach(
        tbl, widget,
        1, y,
        1, 1
    );

    //help
    if (help_text) {
        gtk_grid_attach(
            tbl, help_widget(help_text),
            2, y,
            1, 1
        );
    }
}

static GtkMenuItem*
add_menuitem_m(GtkMenu *menu, const char *label, void (*on_activate)(GtkMenuItem*, gpointer userdata)=0, gpointer userdata=0)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(label));

    if (on_activate) {
        g_signal_connect(i, "activate", G_CALLBACK(on_activate), userdata);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkMenuItem*
add_menuitem(GtkMenu *menu, const char *label, void (*on_activate)(GtkMenuItem*, gpointer userdata)=0, gpointer userdata=0)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_menu_item_new_with_label(label));

    if (on_activate) {
        g_signal_connect(i, "activate", G_CALLBACK(on_activate), userdata);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkMenuItem*
add_separator(GtkMenu *menu)
{
    GtkMenuItem *i = GTK_MENU_ITEM(gtk_separator_menu_item_new());

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(i));

    return i;
}

static GtkDialog* new_dialog_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0);

static GtkDialog*
new_dialog_defaults(const char *title, GtkCallback on_show/*=0*/, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)/*=0*/)
{
    GtkWidget *r = gtk_dialog_new_with_buttons(
            title,
            0, (GtkDialogFlags)(0),
            "_OK", GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            NULL);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_DIALOG(r);
}

static GtkWindow* new_window_defaults(const char *title, GtkCallback on_show=0, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)=0);

static GtkWindow*
new_window_defaults(const char *title, GtkCallback on_show/*=0*/, gboolean (*on_keypress)(GtkWidget*, GdkEventKey*, gpointer)/*=0*/)
{
    GtkWidget *r = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(r), 10);
    gtk_window_set_title(GTK_WINDOW(r), title);
    gtk_window_set_resizable(GTK_WINDOW(r), false);
    // gtk_window_set_policy(GTK_WINDOW(r),
    //         FALSE,
    //         FALSE, FALSE);

    apply_dialog_defaults(r, on_show, on_keypress);

    return GTK_WINDOW(r);
}

static inline void
update_all_spin_buttons(GtkWidget *wdg, gpointer unused)
{
    if (GTK_IS_SPIN_BUTTON(wdg)) {
        gtk_spin_button_update(GTK_SPIN_BUTTON(wdg));
    } else if (GTK_IS_CONTAINER(wdg)) {
        gtk_container_forall(GTK_CONTAINER(wdg), update_all_spin_buttons, NULL);
    }
}

struct cb_find_data {
    int index;
    const char *str;
};

static gchar*
format_joint_strength(GtkScale *scale, gdouble value)
{
    if (value >= 1.0) {
        return g_strdup("Indestructible");
    } else {
        return g_strdup_printf("%0.*f", gtk_scale_get_digits(scale), value);
    }
}

static gchar*
format_auto_absorb(GtkScale *scale, gdouble value)
{
    if (value <= 1.0) {
        return g_strdup("Don't absorb");
    } else {
        return g_strdup_printf("%0.*f seconds", gtk_scale_get_digits(scale), value);
    }
}

gboolean
foreach_model_find_str(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, struct cb_find_data** user_data)
{
    GValue val = {0, };
    gtk_tree_model_get_value(model, iter, 0, &val);

    if (strcmp(g_value_get_string(&val), (*user_data)->str) == 0) {
        gint *index = gtk_tree_path_get_indices(path);

        (*user_data)->index = index[0];
        g_value_unset(&val);
        return true;
    }

    g_value_unset(&val);
    return false;
}

const char*
get_cb_val(GtkComboBoxText *cb)
{
    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    GtkTreeIter iter;
    gboolean r = false;
    r = gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cb), &iter);
    if (r == false) {
        tms_errorf("unable to get cb value");
        return "";
    }

    const char *ret;
    GValue val = {0, };

    gtk_tree_model_get_value(model, &iter, 0, &val);
    ret = g_value_dup_string(&val);

    g_value_unset(&val);
    return ret;
}

gint
find_cb_val(GtkComboBoxText *cb, const char *str)
{
    gint ret = -1;
    struct cb_find_data *d = (struct cb_find_data*)malloc(sizeof(struct cb_find_data));
    d->index = -1;
    d->str = str;

    GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(cb));
    gtk_tree_model_foreach(model, (GtkTreeModelForeachFunc)(foreach_model_find_str), &d);
    ret = d->index;
    free(d);

    return ret;
}

bool
btn_pressed(GtkWidget *ref, GtkButton *btn, gpointer user_data)
{
    return (
        ref == GTK_WIDGET(btn) &&
        (
            ((gtk_widget_get_state_flags(ref) & GTK_STATE_ACTIVE) != 0) ||
            GPOINTER_TO_INT(user_data) == 1
        )
    );
}

void
pk_reload_pkg_list()
{
    GtkTreeIter iter;
    gtk_list_store_clear(GTK_LIST_STORE(pk_pkg_treemodel));

    pkginfo *p = pkgman::get_pkgs(LEVEL_LOCAL);

    while (p) {
        gtk_list_store_append(GTK_LIST_STORE(pk_pkg_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(pk_pkg_treemodel), &iter,
                0, p->id,
                1, p->name,
                -1
                );
        p = p->next;
    }
}

bool
pk_get_current(pkginfo *out)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };
    sel = gtk_tree_view_get_selection(pk_pkg_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_pkg_treemodel,
                                 &iter,
                                 0,
                                 &val);

        uint32_t pkg_id = g_value_get_uint(&val);

        out->open(LEVEL_LOCAL, pkg_id);

        return true;
    }

    return false;
}

void pk_update_level_list()
{
    /* update the list of levels in the package according to the
     * level treemodel */
    GtkTreeIter iter;

    tms_infof("update level list");

    pkginfo p;
    if (pk_get_current(&p)) {
        p.clear_levels();

        if (gtk_tree_model_get_iter_first(
                GTK_TREE_MODEL(pk_lvl_treemodel),
                &iter)) {
            do {
                GValue val = {0, };
                gtk_tree_model_get_value(pk_lvl_treemodel,
                                         &iter,
                                         0,
                                         &val);
                p.add_level((uint32_t)g_value_get_uint(&val));
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pk_lvl_treemodel), &iter));
        } else {
        }

        p.save();
    }
}

void pk_lvl_row_inserted(GtkTreeModel *treemodel,
                         GtkTreePath *arg1,
                         GtkTreeIter *arg2,
                         gpointer user_data)
{
    if (!pk_ignore_lvl_changes)
        pk_update_level_list();
}

void pk_lvl_row_deleted(GtkTreeModel *treemodel,
                           GtkTreePath *arg1,
                           gpointer user_data)
{
    if (!pk_ignore_lvl_changes)
        pk_update_level_list();
}

void
pk_lvl_row_activated(GtkTreeView *view,
                     GtkTreePath *path,
                     GtkTreeViewColumn *col,
                     gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint lvl_id;
    gtk_tree_model_get(model, &iter,
                       0, &lvl_id,
                       -1);

    P.add_action(ACTION_OPEN, (uint32_t)lvl_id);
}

void
pk_reload_level_list()
{
    GtkTreeIter iter;
    char tmp[257];

    pk_ignore_lvl_changes = true;

    pkginfo p;
    gtk_list_store_clear(GTK_LIST_STORE(pk_lvl_treemodel));

    if (pk_get_current(&p)) {

        for (int x=0; x<p.num_levels; x++) {
            pkgman::get_level_name(p.type, p.levels[x], 0, tmp);
            gtk_list_store_append(GTK_LIST_STORE(pk_lvl_treemodel), &iter);
            gtk_list_store_set(GTK_LIST_STORE(pk_lvl_treemodel), &iter,
                    0, p.levels[x],
                    1, tmp,
                    -1
                    );
        }

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_first_is_menu), (bool)p.first_is_menu);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_return_on_finish), (bool)p.return_on_finish);
        gtk_spin_button_set_value(pk_pkg_unlock_count, p.unlock_count);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_unlock_count), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_return_on_finish), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_first_is_menu), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_treeview), true);

        if (G->state.sandbox) {
            gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), true);
        } else {
            gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), false);
        }

        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_del), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_play), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_play), true);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_publish), true);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_unlock_count), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_return_on_finish), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_first_is_menu), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_treeview), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_add), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_play), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_pkg_publish), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_del), false);
        gtk_widget_set_sensitive(GTK_WIDGET(pk_lvl_play), false);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_first_is_menu), false);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pk_pkg_return_on_finish), false);
    }

    pk_ignore_lvl_changes = false;
}

void
value_changed_unlock_count(GtkSpinButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.unlock_count = (uint8_t)gtk_spin_button_get_value(pk_pkg_unlock_count);
        p.save();
    }
}

void
toggle_first_is_menu(GtkToggleButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.first_is_menu = (uint8_t)gtk_toggle_button_get_active(btn);
        p.save();
    }
}

void
toggle_return_on_finish(GtkToggleButton *btn, gpointer unused)
{
    pkginfo p;

    if (pk_get_current(&p)) {
        p.return_on_finish = (uint8_t)gtk_toggle_button_get_active(btn);
        p.save();
    }
}

void
pk_name_edited(GtkCellRendererText *cell, gchar *path, gchar *new_text, gpointer unused)
{
    GtkTreeIter iter;
    GValue val = {0, };

    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pk_pkg_treemodel),
            &iter, path);

    gtk_tree_model_get_value(pk_pkg_treemodel,
                             &iter,
                             0,
                             &val);

    uint32_t pkg_id = g_value_get_uint(&val);

    pkginfo p;
    if (p.open(LEVEL_LOCAL, pkg_id)) {

        if (strcmp(p.name, new_text) == 0)
            return;

        strncpy(p.name, new_text, 255);
        p.save();
        pk_reload_pkg_list();
        pk_reload_level_list();
        //
        //gtk_cell_renderer_text_set_text(cell, new_text);
        tms_infof("name edited of %u", pkg_id);
    }
}

void
press_add_current_level(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };

    if (W->level.local_id == 0) {
        ui::message("Please save the current level before adding it.");
    } else if (!G->state.sandbox) {
        ui::message("You must be in edit mode of the level when adding it.");
    } else {
        sel = gtk_tree_view_get_selection(pk_pkg_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            gtk_tree_model_get_value(pk_pkg_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t pkg_id = g_value_get_uint(&val);

            pkginfo p;

            p.open(LEVEL_LOCAL, pkg_id);

            if (!p.add_level(W->level.local_id))
                ui::message("Level already added to package.");
            else
                p.save();

            pk_reload_level_list();
        } else
            ui::message("No package selected!");
    }
}

void
press_del_selected(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;

    sel = gtk_tree_view_get_selection(pk_lvl_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(pk_lvl_treemodel), &iter);
    }
}

void
press_play_selected(GtkButton *w, gpointer unused)
{
    GtkTreeSelection *sel;
    GtkTreeIter       iter;
    GValue            val = {0, };

    sel = gtk_tree_view_get_selection(pk_lvl_treeview);
    if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
        gtk_tree_model_get_value(pk_lvl_treemodel, &iter, 0, &val);
        uint32_t lvl_id = g_value_get_uint(&val);
        P.add_action(ACTION_OPEN, lvl_id);
    }
}

void
editor_mark_activate(GtkMenuItem *i, gpointer mark_pointer)
{
    struct goto_mark *mark = static_cast<struct goto_mark*>(mark_pointer);
    tvec2 prev_pos = tvec2f(G->cam->_position.x, G->cam->_position.y);

    switch (mark->type) {
        case MARK_ENTITY:
            {
                entity *e = W->get_entity_by_id(mark->id);

                if (!e) {
                    return;
                }

                G->cam->_position.x = e->get_position().x;
                G->cam->_position.y = e->get_position().y;
            }
            break;

        case MARK_POSITION:
            G->cam->_position.x = mark->pos.x;
            G->cam->_position.y = mark->pos.y;
            break;

    }

    editor_menu_last_cam_pos->pos = prev_pos;
}

void
editor_menu_activate(GtkMenuItem *i, gpointer unused)
{
    if (i == editor_menu_lvl_prop) {
        if (gtk_dialog_run(properties_dialog) == GTK_RESPONSE_ACCEPT) {
            const char *name = gtk_entry_get_text(lvl_title);
            int name_len = strlen(name);
            W->level.name_len = name_len;
            memcpy(W->level.name, name, name_len);

            GtkTextIter start, end;
            GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(lvl_descr);

            gtk_text_buffer_get_bounds(text_buffer, &start, &end);

            const char *descr = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
            int descr_len = strlen(descr);

            if (descr_len > 0) {
                W->level.descr_len = descr_len;
                W->level.descr = (char*)realloc(W->level.descr, descr_len);

                memcpy(W->level.descr, descr, descr_len);
            } else {
                W->level.descr_len = 0;
            }

            uint16_t left  = (uint16_t)atoi(gtk_entry_get_text(lvl_width_left));
            uint16_t right = (uint16_t)atoi(gtk_entry_get_text(lvl_width_right));
            uint16_t down  = (uint16_t)atoi(gtk_entry_get_text(lvl_height_down));
            uint16_t up    = (uint16_t)atoi(gtk_entry_get_text(lvl_height_up));

            float w = (float)left + (float)right;
            float h = (float)down + (float)up;

            bool resized = false;

            if (w < 5.f) {
                resized = true;
                left += 6-(uint16_t)w;
            }
            if (h < 5.f) {
                resized = true;
                down += 6-(uint16_t)w;
            }

            if (resized)
                ui::message("Your level size was increased to the minimum allowed.");

            W->level.size_x[0] = left;
            W->level.size_x[1] = right;
            W->level.size_y[0] = down;
            W->level.size_y[1] = up;
            W->level.gravity_x = (float)gtk_spin_button_get_value(lvl_gx);
            W->level.gravity_y = (float)gtk_spin_button_get_value(lvl_gy);

            uint8_t vel_iter = (uint8_t)gtk_range_get_value(GTK_RANGE(lvl_vel_iter));
            uint8_t pos_iter = (uint8_t)gtk_range_get_value(GTK_RANGE(lvl_pos_iter));

            float prismatic_tolerance = gtk_range_get_value(GTK_RANGE(lvl_prismatic_tol));
            float pivot_tolerance = gtk_range_get_value(GTK_RANGE(lvl_pivot_tol));

            float angular_damping = gtk_range_get_value(GTK_RANGE(lvl_angular_damping));
            float joint_friction = gtk_range_get_value(GTK_RANGE(lvl_joint_friction));
            float linear_damping = gtk_range_get_value(GTK_RANGE(lvl_linear_damping));

            W->level.angular_damping = angular_damping;
            W->level.joint_friction = joint_friction;
            W->level.linear_damping = linear_damping;

            W->level.prismatic_tolerance = prismatic_tolerance;
            W->level.pivot_tolerance = pivot_tolerance;

            tms_infof("vel_iter: %d,  pos_iter: %d", vel_iter, pos_iter);
            W->level.velocity_iterations = vel_iter;
            W->level.position_iterations = pos_iter;
            W->level.final_score = (uint32_t)atoi(gtk_entry_get_text(lvl_score));

            if (W->level.version >= 7) {
                W->level.show_score = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_show_score));
                W->level.pause_on_finish = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_pause_on_win));
            }

            if (W->level.version >= 9) {
                W->level.show_score = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_show_score));
                W->level.flags = 0;
                for (int x=0; x<num_gtk_level_properties; ++x) {
                    W->level.flags |= ((int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_level_properties[x].checkbutton)) * gtk_level_properties[x].flag);
                }
            }

            W->level.bg = gtk_combo_box_get_active(GTK_COMBO_BOX(lvl_bg));
            W->level.bg_color = new_bg_color;

            if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_radio_adventure))) {
                P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_ADVENTURE);
            } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lvl_radio_custom))) {
                P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_CUSTOM);
            }

            P.add_action(ACTION_RELOAD_LEVEL, 0);
        }

        gtk_widget_hide(GTK_WIDGET(properties_dialog));

    } else if (i == editor_menu_move_here_player) {


    } else if (i == editor_menu_move_here_object) {
        if (G->selection.e) {
            const b2Vec2 _pos = G->get_last_cursor_pos(G->selection.e->get_layer());

            b2Vec2 *pos = new b2Vec2();
            pos->Set(_pos.x, _pos.y);

            W->add_action(G->selection.e->id, ACTION_MOVE_ENTITY, (void*)pos);
        }
    } else if (i == editor_menu_set_as_player) {

    } else if (i == editor_menu_toggle_mark_entity) {
        if (G->selection.e) {
            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;

                if (mark != editor_menu_last_created && mark->type == MARK_ENTITY && mark->id == G->selection.e->id) {
                    gtk_container_remove(GTK_CONTAINER(editor_menu_go_to_menu), GTK_WIDGET(mark->menuitem));

                    editor_menu_marks.erase(it);

                    delete mark;

                    return;
                }
            }

            char tmp[128];
            snprintf(tmp, 127, "%s - %d", G->selection.e->get_name(), G->selection.e->id);
            struct goto_mark *mark = new goto_mark(MARK_ENTITY, tmp, G->selection.e->id, tvec2f(0.f, 0.f));
            mark->menuitem = add_menuitem(editor_menu_go_to_menu, mark->label, editor_mark_activate, (gpointer)mark);

            editor_menu_marks.push_back(mark);

            refresh_mark_menuitems();
        }
    }
}

void
activate_mode_dialog(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(mode_dialog);

    switch (result) {
        case RESPONSE_MULTISEL:
            G->set_mode(GAME_MODE_MULTISEL);
            break;
        case RESPONSE_CONN_EDIT:
            G->set_mode(GAME_MODE_CONN_EDIT);
            break;

        default: break;
    }

    gtk_widget_hide(GTK_WIDGET(mode_dialog));
}

void
activate_new_level(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(new_level_dialog);

    switch (result) {
        case RESPONSE_PUZZLE:
            P.add_action(ACTION_NEW_LEVEL, LCAT_PUZZLE);
            break;
        case RESPONSE_CUSTOM:
            P.add_action(ACTION_NEW_LEVEL, LCAT_CUSTOM);
            break;

        default: break;
    }

    gtk_widget_hide(GTK_WIDGET(new_level_dialog));
}

/** --Confirm Quit Dialog **/
void
on_confirm_quit_show(GtkWidget *wdg, gpointer unused)
{
    gtk_widget_grab_focus(GTK_WIDGET(confirm_btn_quit));
}

void
on_pkg_name_show(GtkWidget *wdg, void *unused)
{
    gtk_entry_set_text(pkg_name_entry, "");
}

void
on_tips_show(GtkWidget *wdg, void *unused)
{
    if (ctip == -1) ctip = rand()%num_tips;

    gtk_label_set_markup(tips_text, tips[ctip]);

    ctip = (ctip+1)%num_tips;
}

void
on_publish_show(GtkWidget *wdg, void *unused)
{
    char *current_descr = (char*)malloc(W->level.descr_len+1);
    memcpy(current_descr, W->level.descr, W->level.descr_len);
    current_descr[W->level.descr_len] = '\0';
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(publish_descr);
    gtk_text_buffer_set_text(text_buffer, current_descr, -1);

    char current_name[257];
    memcpy(current_name, W->level.name, W->level.name_len);
    current_name[W->level.name_len] = '\0';
    gtk_entry_set_text(publish_name, current_name);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(publish_locked), W->level.visibility == LEVEL_LOCKED);

    free(current_descr);
}

void
on_package_manager_show(GtkWidget *wdg, void *unused)
{
    pk_reload_pkg_list();
    pk_reload_level_list();
}

void
on_object_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(object_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_PARTIAL);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(object_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(object_treemodel), &iter,
                OC_ID, level->id,
                OC_NAME, level->name,
                OC_DATE, level->modified_date,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(object_treeview);

    gtk_tree_model_get_iter(object_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(object_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    gtk_tree_path_free(path);
}

/** --Open state **/
void
on_open_state_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(open_state_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL_STATE);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(open_state_treemodel), &iter);
        gtk_list_store_set(GTK_LIST_STORE(open_state_treemodel), &iter,
                OSC_ID, level->id,
                OSC_NAME, level->name,
                OSC_DATE, level->modified_date,
                OSC_SAVE_ID, level->save_id,
                OSC_ID_TYPE, level->id_type,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(open_state_treeview);

    gtk_tree_model_get_iter(open_state_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(open_state_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    tms_infof("got id: %d", g_value_get_uint(&val));
    gtk_tree_path_free(path);
}

static void
open_state_row(GtkTreeIter *iter)
{
    if (!iter) {
        return;
    }

    guint _level_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID, &_level_id,
            -1);
    guint _save_id;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_SAVE_ID, &_save_id,
            -1);

    guint _level_id_type;
    gtk_tree_model_get(open_state_treemodel, iter,
            OSC_ID_TYPE, &_level_id_type,
            -1);

    uint32_t level_id = (uint32_t)_level_id;
    uint32_t save_id = (uint32_t)_save_id;
    uint32_t id_type = (uint32_t)_level_id_type;

    tms_infof("clicked level id %u save %u ", level_id, save_id);

    uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
    info[0] = id_type;
    info[1] = level_id;
    info[2] = save_id;

    if (open_state_no_testplaying) {
        G->state.test_playing = false;
        G->screen_back = P.s_menu_main;
    }

    P.add_action(ACTION_OPEN_STATE, info);

    gtk_widget_hide(GTK_WIDGET(open_state_window));
}

static void
activate_open_state_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    open_state_row(&iter);
}

/** --Open level **/
void
on_open_show(GtkWidget *wdg, void *unused)
{
    GtkTreeIter iter;

    gtk_list_store_clear(GTK_LIST_STORE(open_treemodel));

    lvlfile *level = pkgman::get_levels(LEVEL_LOCAL);

    while (level) {
        gtk_list_store_append(GTK_LIST_STORE(open_treemodel), &iter);
        const char *version_string = level_version_string(level->version);
        gtk_list_store_set(GTK_LIST_STORE(open_treemodel), &iter,
                OC_ID, level->id,
                OC_NAME, level->name,
                OC_VERSION, version_string,
                OC_DATE, level->modified_date,
                -1
                );
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    GtkTreePath      *path;
    GtkTreeSelection *sel;

    path = gtk_tree_path_new_from_indices(0, -1);
    sel  = gtk_tree_view_get_selection(open_treeview);

    gtk_tree_model_get_iter(open_treemodel,
                            &iter,
                            path);

    GValue val = {0, };

    gtk_tree_model_get_value(open_treemodel,
                             &iter,
                             0,
                             &val);

    gtk_tree_selection_select_path(sel, path);

    tms_infof("got id: %d", g_value_get_uint(&val));
    gtk_tree_path_free(path);
}

static void
activate_open_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _level_id;
    gtk_tree_model_get(model, &iter,
                       OC_ID, &_level_id,
                       -1);

    uint32_t level_id = (uint32_t)_level_id;

    tms_infof("clicked level id %u", level_id);

    P.add_action(ACTION_OPEN, level_id);

    gtk_widget_hide(GTK_WIDGET(open_window));
}

static void
open_menu_item_activated(GtkMenuItem *i, gpointer userdata)
{
    if (i == open_menu_information) {
        static GtkMessageDialog *msg_dialog = 0;

        if (msg_dialog == 0) {
            msg_dialog = GTK_MESSAGE_DIALOG(gtk_message_dialog_new(
                    0, (GtkDialogFlags)(0),
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_CLOSE,
                    "Level information"));
        }

        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_treeview);

        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            GValue val_name = {0, };
            GValue val_date = {0, };
            GValue val_version = {0, };
            char msg[2048];
            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_NAME,
                    &val_name);
            const char *name = g_value_get_string(&val_name);

            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_DATE,
                    &val_date);
            const char *lastmodified = g_value_get_string(&val_date);

            gtk_tree_model_get_value(open_treemodel,
                    &iter,
                    OC_VERSION,
                    &val_version);
            const char *version = g_value_get_string(&val_version);

            snprintf(
                msg, 2048,
                "Name: %s\nVersion: %s\nLast modified: %s",
                name, version, lastmodified
            );

            g_object_set(msg_dialog, "text", msg, NULL);

            int r = gtk_dialog_run(GTK_DIALOG(msg_dialog));
            switch (r) {
                default:
                    gtk_widget_hide(GTK_WIDGET(msg_dialog));
                    break;
            }
        }
    } else if (i == open_menu_delete) {
        GtkDialog* confirm = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Delete level",
            GTK_WINDOW(open_window),
            (GtkDialogFlags)(0),
            "_Confirm",
            GTK_RESPONSE_ACCEPT,
            "_Cancel",
            GTK_RESPONSE_REJECT,
            NULL
        ));
        gtk_container_add(
            GTK_CONTAINER(gtk_dialog_get_content_area(confirm)),
            gtk_label_new("Are you sure you want to delete this level")
        );
        int r = gtk_dialog_run(confirm);
        switch (r) {
            case GTK_RESPONSE_ACCEPT: {
                tms_infof("deleting uwu");

                //get level id
                GtkTreeIter iter;
                GtkTreeSelection *sel = gtk_tree_view_get_selection(open_treeview);
                if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
                    GValue val_id = {0, };
                    gtk_tree_model_get_value(
                        open_treemodel,
                        &iter,
                        OC_ID,
                        &val_id
                    );
                    uint32_t level_id = g_value_get_uint(&val_id);
                    tms_infof("will DELETE local level with id of %d RIGHT NOW!", level_id);

                    //XXX: Levels in the "open" should always be local
                    //XXX: Save id is only used for state saves (LEVEL_*_STATE), not levels
                    if (G->delete_level(LEVEL_LOCAL, level_id, -1)) {
                        //success
                        tms_infof("deleted successfully :3");

                        //remove from the list
                        gtk_list_store_remove(GTK_LIST_STORE(open_treemodel), &iter);
                    } else {
                        //error
                        tms_errorf("unlink failed");

                        //show error dialog
                        GtkDialog* error = GTK_DIALOG(gtk_message_dialog_new(
                            GTK_WINDOW(open_window),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            GTK_MESSAGE_ERROR,
                            GTK_BUTTONS_CLOSE,
                            "Failed to delete the level"
                        ));
                        g_signal_connect_swapped(
                            G_OBJECT(error), "response",
                            G_CALLBACK(gtk_widget_destroy),
                            error
                        );
                        gtk_dialog_run(error);
                    }
                }
                break;
            }
            default: {
                gtk_widget_hide(GTK_WIDGET(confirm));
            }
        }
        gtk_widget_destroy(GTK_WIDGET(confirm));
    }
}

static gboolean
open_row_button_press(GtkWidget *wdg, GdkEvent *event, gpointer userdata)
{
    if (event->type == GDK_BUTTON_PRESS) {
        GdkEventButton *bevent = (GdkEventButton*)event;
        if (bevent->button == 3) {
            gtk_widget_show_all(GTK_WIDGET(open_menu));
            gtk_menu_popup_at_pointer(open_menu, event);
        }
    }
    return FALSE;
}

static void
confirm_import(uint32_t level_id)
{
    if (object_window_multiemitter) {
        P.add_action(ACTION_MULTIEMITTER_SET, level_id);
    } else {
        P.add_action(ACTION_SELECT_IMPORT_OBJECT, level_id);
    }
}

void
activate_object_row(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model(view);
    gtk_tree_model_get_iter_from_string(model, &iter, gtk_tree_path_to_string(path));

    guint _level_id;
    gtk_tree_model_get(model, &iter,
                       OC_ID, &_level_id,
                       -1);

    confirm_import((uint32_t)_level_id);

    gtk_widget_hide(GTK_WIDGET(object_window));
}

gboolean
on_autofit_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, (GtkButton*)w, user_data)) {
        P.add_action(ACTION_AUTOFIT_LEVEL_BORDERS, 0);
    }

    return false;
}

gboolean
on_lvl_bg_changed(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {

    return false;
}

gboolean
on_lvl_bg_color_set(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    tms_debugf("bg color button COLOR SET");

    GtkColorChooser *sel = GTK_COLOR_CHOOSER(lvl_bg_color);

    GdkRGBA new_color;
    gtk_color_chooser_get_rgba(sel, &new_color);

    tms_debugf("new_r: %.2f", new_color.red);
    tms_debugf("new_g: %.2f", new_color.green);
    tms_debugf("new_b: %.2f", new_color.blue);

    new_bg_color = pack_rgba(new_color.red, new_color.green, new_color.blue, 1.f);

    return false;
}

gboolean
on_upgrade_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, (GtkButton*)w, user_data)) {
        gint result = gtk_dialog_run(confirm_upgrade_dialog);

        if (result == GTK_RESPONSE_ACCEPT) {
            P.add_action(ACTION_UPGRADE_LEVEL, 0);
            _close_all_dialogs(0);
        }

        gtk_widget_hide(GTK_WIDGET(confirm_upgrade_dialog));
    }

    return false;
}

gboolean
on_open_state_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, open_state_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(open_state_window));
    } else if (btn_pressed(w, open_state_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            open_state_row(&iter);

        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_open_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, open_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(open_window));
    } else if (btn_pressed(w, open_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(open_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(open_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            tms_infof("Opening level %d from Open window", level_id);

            P.add_action(ACTION_OPEN, level_id);

            gtk_widget_hide(GTK_WIDGET(open_window));
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_object_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, object_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(object_window));
    } else if (btn_pressed(w, object_btn_open, user_data)) {
        /* open ! */
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(GTK_WIDGET(object_window));
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_object_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(object_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(object_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            confirm_import(level_id);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_open_state_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;

        sel = gtk_tree_view_get_selection(open_state_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */
            guint _level_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID, &_level_id,
                               -1);
            guint _save_id;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_SAVE_ID, &_save_id,
                               -1);

            guint _level_id_type;
            gtk_tree_model_get(open_state_treemodel, &iter,
                               OSC_ID_TYPE, &_level_id_type,
                               -1);

            uint32_t level_id = (uint32_t)_level_id;
            uint32_t save_id = (uint32_t)_save_id;
            uint32_t id_type = (uint32_t)_level_id_type;

            tms_infof("clicked level id %u save %u ", level_id, save_id);

            uint32_t *info = (uint32_t*)malloc(sizeof(uint32_t)*3);
            info[0] = id_type;
            info[1] = level_id;
            info[2] = save_id;

            P.add_action(ACTION_OPEN_STATE, info);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_open_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        GtkTreeSelection *sel;
        GtkTreeIter       iter;
        GValue            val = {0, };

        sel = gtk_tree_view_get_selection(open_treeview);
        if (gtk_tree_selection_get_selected(sel, NULL, &iter)) {
            /* A row is selected */

            /* Fetch the value of the first column into `val' */
            gtk_tree_model_get_value(open_treemodel,
                                     &iter,
                                     0,
                                     &val);

            uint32_t level_id = g_value_get_uint(&val);

            tms_infof("Opening level %d from Open window", level_id);

            P.add_action(ACTION_OPEN, level_id);

            gtk_widget_hide(w);
            return true;
        } else {
            tms_infof("No row selected.");
        }
    }

    return false;
}

gboolean
on_lvl_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(GTK_WIDGET(save_window));
        return true;
    }

    if (key->keyval == GDK_KEY_Return) {
        gtk_button_clicked(save_ok);
        return true;
    }

    return false;
}

static void
save_setting_row(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            settings[r->setting_name]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r->wdg));
            break;

        case ROW_HSCALE:
            settings[r->setting_name]->v.f = (float)gtk_range_get_value(GTK_RANGE(r->wdg));
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

static void
load_setting_row(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r->wdg), settings[r->setting_name]->v.b);
            break;

        case ROW_HSCALE:
            gtk_range_set_value(GTK_RANGE(r->wdg), (double)settings[r->setting_name]->v.f);
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

static void
create_setting_row_widget(struct table_setting_row *r)
{
    const struct setting_row_type &row = r->row;

    switch (row.type) {
        case ROW_CHECKBOX:
            r->wdg = gtk_check_button_new();
            break;

        case ROW_HSCALE:
            r->wdg = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, row.min, row.max, row.step);
            break;

        default:
            tms_errorf("Unknown row type: %d", row.type);
            break;
    }
}

/** --Settings **/
void
save_settings()
{
    P.can_reload_graphics = false;
    P.can_set_settings = false;
    P.add_action(ACTION_RELOAD_GRAPHICS, 0);
    tms_infof("Saving...");

    while (!P.can_set_settings) {
        tms_debugf("Waiting for can_set_settings...");
        SDL_Delay(1);
    }

    char tmp[64];
    settings["enable_shadows"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_shadows));
    settings["enable_ao"]->v.b = (bool)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(settings_enable_ao));
    settings["shadow_quality"]->v.u8 = (uint8_t)gtk_spin_button_get_value(settings_shadow_quality);

    /* Graphics */
    for (int x=0; x<settings_num_graphic_rows; ++x) {
        struct table_setting_row *r = &settings_graphic_rows[x];
        save_setting_row(r);
    }

    /* Audio */
    for (int x=0; x<settings_num_audio_rows; ++x) {
        struct table_setting_row *r = &settings_audio_rows[x];
        save_setting_row(r);
    }

    /* Controls */
    for (int x=0; x<settings_num_control_rows; ++x) {
        struct table_setting_row *r = &settings_control_rows[x];
        save_setting_row(r);
    }

    /* Interface */
    for (int x=0; x<settings_num_interface_rows; ++x) {
        struct table_setting_row *r = &settings_interface_rows[x];
        save_setting_row(r);
    }

    sm::load_settings();

    strcpy(tmp, get_cb_val(settings_shadow_res));
    char *x = strchr(tmp, 'x');
    if (x == NULL) {
        settings["shadow_map_resx"]->v.i = _tms.window_width;
        settings["shadow_map_resy"]->v.i = _tms.window_height;
    } else {
        char *res_x = (char*)malloc(64);
        char *res_y = (char*)malloc(64);
        int pos = x-tmp;

        strncpy(res_x, tmp, pos);
        strcpy(res_y, tmp+pos+1);
        res_x[pos] = '\0';

        //tms_infof("Setting shadow map to '%s'x'%s'", res_x, res_y);
        settings["shadow_map_resx"]->v.i = atoi(res_x);
        settings["shadow_map_resy"]->v.i = atoi(res_y);

        free(res_x);
        free(res_y);
    }

    strcpy(tmp, get_cb_val(settings_ao_res));
    x = strchr(tmp, 'x');
    if (x != NULL) {
        char *res = (char*)malloc(64);
        int pos = x-tmp;

        strncpy(res, tmp, pos);
        res[pos] = '\0';

        //tms_infof("Setting ao map to '%s'x'%s'", res, res);
        settings["ao_map_res"]->v.i = atoi(res);

        free(res);
    }

    if (!settings.save()) {
        tms_errorf("Unable to save settings.");
    } else {
        tms_infof("Successfully saved settings to file.");
    }

    tms_infof("done!");

#ifdef TMS_BACKEND_WINDOWS
    SDL_EventState(SDL_SYSWMEVENT, settings["emulate_touch"]->is_true() ? SDL_ENABLE : SDL_DISABLE);
#endif

    P.can_reload_graphics = true;
}

/* SETTINGS LOAD */
void
on_settings_show(GtkWidget *wdg, void *unused)
{
    char tmp[64];

    gtk_spin_button_set_value(settings_shadow_quality, settings["shadow_quality"]->v.u8);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings_enable_shadows), settings["enable_shadows"]->v.b);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(settings_enable_ao), settings["enable_ao"]->v.b);

    /* Graphics */
    for (int x=0; x<settings_num_graphic_rows; ++x) {
        struct table_setting_row *r = &settings_graphic_rows[x];
        load_setting_row(r);
    }

    /* Audio */
    for (int x=0; x<settings_num_audio_rows; ++x) {
        struct table_setting_row *r = &settings_audio_rows[x];
        load_setting_row(r);
    }

    /* Controls */
    for (int x=0; x<settings_num_control_rows; ++x) {
        struct table_setting_row *r = &settings_control_rows[x];
        load_setting_row(r);
    }

    /* Interface */
    for (int x=0; x<settings_num_interface_rows; ++x) {
        struct table_setting_row *r = &settings_interface_rows[x];
        load_setting_row(r);
    }

    snprintf(tmp, 64, "%dx%d", settings["shadow_map_resx"]->v.i, settings["shadow_map_resy"]->v.i);
    if (settings["shadow_map_resx"]->v.i == _tms.window_width && settings["shadow_map_resy"]->v.i == _tms.window_height) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), 0);
    } else {
        gint index = find_cb_val(settings_shadow_res, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), index);
        } else {
            gtk_combo_box_text_append_text(settings_shadow_res, tmp);

            index = find_cb_val(settings_shadow_res, tmp);
            if (index != -1) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(settings_shadow_res), index);
            } else {
                tms_errorf("Unable to get index for a value we just appended");
            }
        }
    }

    snprintf(tmp, 64, "%dx%d", settings["ao_map_res"]->v.i, settings["ao_map_res"]->v.i);
    if (settings["ao_map_res"]->v.i == _tms.window_width && settings["ao_map_res"]->v.i == _tms.window_height) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), 0);
    } else {
        gint index = find_cb_val(settings_ao_res, tmp);
        if (index != -1) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), index);
        } else {
            gtk_combo_box_text_append_text(settings_ao_res, tmp);

            index = find_cb_val(settings_ao_res, tmp);
            if (index != -1) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(settings_ao_res), index);
            } else {
                tms_errorf("Unable to get index for a value we just appended");
            }
        }
    }
}

/** --Save and Save as copy **/
void
on_save_show(GtkWidget *wdg, void *unused)
{
    char tmp[257];
    memcpy(tmp, W->level.name, W->level.name_len);
    tmp[W->level.name_len] = '\0';
    gtk_entry_set_text(save_entry, tmp);

    gtk_widget_grab_focus(GTK_WIDGET(save_entry));
}

gboolean
on_save_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, save_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(save_window));
    } else if (btn_pressed(w, save_ok, user_data)) {
        if (gtk_entry_get_text_length(save_entry) > 0) {
            const char *name = gtk_entry_get_text(save_entry);
            int name_len = strlen(name);
            if (name_len == 0) {
                ui::message("Your level must have a name.");
                return false;
            }
            W->level.name_len = name_len;
            memcpy(W->level.name, name, name_len);

            tms_infof("set level name to %s", name);

            if (save_type == SAVE_COPY)
                P.add_action(ACTION_SAVE_COPY, 0);
            else
                P.add_action(ACTION_SAVE, 0);
            gtk_widget_hide(GTK_WIDGET(save_window));
        } else {
            gtk_label_set_text(save_status, "You must enter a name!");
        }
    }

    return false;
}

gboolean
on_save_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(GTK_WIDGET(save_window));
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(save_cancel))) {
            on_save_btn_click(GTK_WIDGET(save_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_save_btn_click(GTK_WIDGET(save_ok), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

/** --Export **/
void
on_export_show(GtkWidget *wdg, void *unused)
{
    gtk_entry_set_text(export_entry, "");

    gtk_widget_grab_focus(GTK_WIDGET(export_entry));
}

gboolean
on_export_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, export_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(export_window));
    } else if (btn_pressed(w, export_ok, user_data)) {
        if (gtk_entry_get_text_length(export_entry) > 0) {
            char *name = strdup(gtk_entry_get_text(export_entry));
            tms_infof("set export name to %s", name);

            P.add_action(ACTION_EXPORT_OBJECT, name);
            gtk_widget_hide(GTK_WIDGET(export_window));
        } else {
            gtk_label_set_text(export_status, "You must enter a name!");
        }
    }

    return false;
}

gboolean
on_export_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(GTK_WIDGET(export_window));
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(export_cancel))) {
            on_export_btn_click(GTK_WIDGET(export_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_export_btn_click(GTK_WIDGET(export_ok), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

/** --Tips Dialog **/
gboolean
on_tips_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Info Dialog **/
void
on_info_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_text(info_text, _pass_info_descr);
    gtk_label_set_text(info_name, _pass_info_name);
}

gboolean
on_info_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Confirm Dialog **/
void
on_confirm_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_markup(confirm_text, _pass_confirm_text);
    gtk_button_set_label(confirm_button1, _pass_confirm_button1);
    gtk_button_set_label(confirm_button2, _pass_confirm_button2);
    if (_pass_confirm_button3) {
        tms_infof("BUTTON3 EXISTS!!!!!!!!!!!!");
        gtk_button_set_label(confirm_button3, _pass_confirm_button3);
    } else {
        gtk_widget_hide(GTK_WIDGET(confirm_button3));
    }

    gtk_widget_set_size_request(GTK_WIDGET(confirm_dialog), -1, -1);
}

gboolean
on_confirm_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            if (!gtk_widget_has_focus(GTK_WIDGET(confirm_button2))) {
                gtk_dialog_response(confirm_dialog, 1);
            }
            break;
    }

    return false;
}

/** --Alert Dialog **/
void
on_alert_show(GtkWidget *wdg, void *unused)
{
    // set text without markup
    // g_object_set(alert_dialog, "text", _alert_text, NULL);

    gtk_message_dialog_set_markup(alert_dialog, _alert_text);
}

gboolean
on_alert_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    switch (key->keyval) {
        case GDK_KEY_Escape:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_CANCEL);
            break;

        case GDK_KEY_Return:
            gtk_dialog_response(confirm_dialog, GTK_RESPONSE_ACCEPT);
            break;
    }

    return false;
}

/** --Error Dialog **/
void
on_error_show(GtkWidget *wdg, void *unused)
{
    gtk_label_set_text(error_text, _pass_error_text);
}

gboolean
on_error_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape || key->keyval == GDK_KEY_Return) {
        gtk_widget_hide(w);
        return true;
    }

    return false;
}

/** --Level properties **/
static void
on_level_flag_toggled(GtkToggleButton *btn, gpointer _flag)
{
    bool toggled = gtk_toggle_button_get_active(btn);
    uint64_t flag = VOID_TO_UINT64(_flag);
    tms_debugf("flag: %" PRIu64, flag);

}

gboolean
on_properties_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(lvl_cancel))) {
            gtk_button_clicked(lvl_cancel);
        } else if (gtk_widget_has_focus(GTK_WIDGET(lvl_descr))) {
            /* do nothing */
        } else {
            gtk_button_clicked(lvl_ok);
        }
    }

    return false;

}

void
refresh_borders()
{
    char tmp[128];
    sprintf(tmp, "%d", W->level.size_x[0]);
    gtk_entry_set_text(lvl_width_left, tmp);

    sprintf(tmp, "%d", W->level.size_x[1]);
    gtk_entry_set_text(lvl_width_right, tmp);

    sprintf(tmp, "%d", W->level.size_y[0]);
    gtk_entry_set_text(lvl_height_down, tmp);

    sprintf(tmp, "%d", W->level.size_y[1]);
    gtk_entry_set_text(lvl_height_up, tmp);

}

/**
 * Get stuff from the currently loaded level and fill in the fields
 **/
void
on_properties_show(GtkWidget *wdg, void *unused)
{
    char *current_descr;
    char current_name[257];
    char tmp[128];

    current_descr = (char*)malloc(W->level.descr_len+1);
    memcpy(current_descr, W->level.descr, W->level.descr_len);
    current_descr[W->level.descr_len] = '\0';
    GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(lvl_descr);
    gtk_text_buffer_set_text(text_buffer, current_descr, -1);

    memcpy(current_name, W->level.name, W->level.name_len);
    current_name[W->level.name_len] = '\0';
    gtk_entry_set_text(lvl_title, current_name);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_radio_adventure), (W->level.type == LCAT_ADVENTURE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_radio_custom), (W->level.type == LCAT_CUSTOM));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(publish_locked), W->level.visibility == LEVEL_LOCKED);

    refresh_borders();

    gtk_spin_button_set_value(lvl_gx, W->level.gravity_x);
    gtk_spin_button_set_value(lvl_gy, W->level.gravity_y);

    /* Gameplay */
    sprintf(tmp, "%u", W->level.final_score);
    gtk_entry_set_text(lvl_score, tmp);

    if (W->level.version >= 7) {
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_show_score), true);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_show_score), W->level.show_score);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_pause_on_win), true);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lvl_pause_on_win), W->level.pause_on_finish);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_show_score), false);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_pause_on_win), false);
    }

    if (W->level.version >= 9) {
        /* TODO: Check current game mode and see if these should be enabled or not.
         * also add an on_click to the type radio buttons which updates this */
        tms_infof("flags: %" PRIu64, W->level.flags);
        for (int x=0; x<num_gtk_level_properties; ++x) {
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_level_properties[x].checkbutton), true);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_level_properties[x].checkbutton), ((uint64_t)(W->level.flags & gtk_level_properties[x].flag) != 0));
        }
    } else {
        for (int x=0; x<num_gtk_level_properties; ++x) {
            gtk_widget_set_sensitive(GTK_WIDGET(gtk_level_properties[x].checkbutton), false);
        }
    }

    char vv[32];

    if (W->level.version == LEVEL_VERSION) {
        snprintf(vv, 31, "%d (latest)", LEVEL_VERSION);
        gtk_button_set_label(GTK_BUTTON(lvl_upgrade), vv);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_upgrade), false);
    } else {
        snprintf(vv, 31, "%d (Upgrade to %d)", W->level.version, LEVEL_VERSION);
        gtk_button_set_label(GTK_BUTTON(lvl_upgrade), vv);
        gtk_widget_set_sensitive(GTK_WIDGET(lvl_upgrade), true);

    }

    uint8_t vel_iter = W->level.velocity_iterations;
    uint8_t pos_iter = W->level.position_iterations;
    gtk_range_set_value(GTK_RANGE(lvl_vel_iter), (double)vel_iter);

    gtk_range_set_value(GTK_RANGE(lvl_pos_iter), (double)pos_iter);

    gtk_range_set_value(GTK_RANGE(lvl_prismatic_tol), W->level.prismatic_tolerance);
    gtk_range_set_value(GTK_RANGE(lvl_pivot_tol), W->level.pivot_tolerance);

    gtk_range_set_value(GTK_RANGE(lvl_linear_damping), W->level.linear_damping);
    gtk_range_set_value(GTK_RANGE(lvl_angular_damping), W->level.angular_damping);
    gtk_range_set_value(GTK_RANGE(lvl_joint_friction), W->level.joint_friction);

    gtk_combo_box_set_active(GTK_COMBO_BOX(lvl_bg), W->level.bg);
    new_bg_color = W->level.bg_color;

    {
        GdkRGBA bg_color;
        float r, g, b, a;

        unpack_rgba(W->level.bg_color, &r, &g, &b, &a);

        bg_color.red   = r;
        bg_color.green = g;
        bg_color.blue  = b;
        bg_color.alpha = 1.0;

        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(lvl_bg_color), &bg_color);
    }

    free(current_descr);
}

void
activate_open_state(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(open_state_window));
}

void
activate_open(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(open_window));
}

void
activate_object(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(object_window));
}

void
activate_export(GtkMenuItem *i, gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(export_window));
}

void
activate_controls(GtkMenuItem *i, gpointer unused)
{
    G->render_controls = true;
}

void
activate_restart_level(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_RESTART_LEVEL, 0);
}

void
activate_back(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_BACK, 0);
}

void
activate_save(GtkMenuItem *i, gpointer unused)
{
    bool ask_for_new_name = false;

    if (W->level.name_len == 0 || strcmp(W->level.name, "<no name>") == 0) {
        ask_for_new_name = true;
    }

    if (ask_for_new_name) {
        save_type = SAVE_REGULAR;
        gtk_widget_show_all(GTK_WIDGET(save_window));
    } else {
        P.add_action(ACTION_SAVE, 0);
    }
}

void
activate_save_copy(GtkMenuItem *i, gpointer unused)
{
    save_type = SAVE_COPY;
    gtk_widget_show_all(GTK_WIDGET(save_window));
}

/* When activate_settings is called normally, userdata is an uint8_t with the value 0.
 * That means the graphics should reload and return to the G screen
 * When activate_settings is called via open_dialog(DIALOG_SETTINGS), userdata is 1.
 * That means RELOAD_GRAPHICS should return to the main menu instead. */
void
activate_settings(GtkMenuItem *i, gpointer userdata)
{
    gint result = gtk_dialog_run(settings_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        save_settings();
    }

    gtk_widget_hide(GTK_WIDGET(settings_dialog));
}

void
activate_publish(GtkMenuItem *i, gpointer unused)
{
    gint result = gtk_dialog_run(publish_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(publish_name);
        int name_len = strlen(name);
        if (name_len == 0) {
            ui::message("You cannot publish a level without a name.");
            activate_publish(0,0);
            return;
        }
        W->level.name_len = name_len;
        memcpy(W->level.name, name, name_len);

        GtkTextIter start, end;
        GtkTextBuffer *text_buffer = gtk_text_view_get_buffer(publish_descr);
        char *descr;

        gtk_text_buffer_get_bounds(text_buffer, &start, &end);

        descr = gtk_text_buffer_get_text(text_buffer, &start, &end, FALSE);
        int descr_len = strlen(descr);

        if (descr_len > 0) {
            W->level.descr_len = descr_len;
            W->level.descr = (char*)realloc(W->level.descr, descr_len+1);

            memcpy(W->level.descr, descr, descr_len);
            descr[descr_len] = '\0';
        } else
            W->level.descr_len = 0;

        W->level.visibility = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(publish_locked)) ? LEVEL_LOCKED : LEVEL_VISIBLE;

        tms_infof("Setting level name to:  %s", name);
        tms_infof("Setting level descr to: %s", descr);

        P.add_action(ACTION_PUBLISH, 0);
    }

    gtk_widget_hide(GTK_WIDGET(publish_dialog));
}

/** --Multi config **/
static void
on_multi_config_show(GtkWidget *wdg, void *unused)
{
    gtk_range_set_value(GTK_RANGE(multi_config_joint_strength), 1.0);

    bool any_entity_locked = false;

    bool enabled_tabs[NUM_MULTI_CONFIG_TABS];
    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        enabled_tabs[x] = false;
    }

    enabled_tabs[TAB_JOINT_STRENGTH]            = true;
    enabled_tabs[TAB_CONNECTION_RENDER_TYPE]    = true;
    enabled_tabs[TAB_MISCELLANEOUS]             = true;

    if (G->state.sandbox && W->is_paused() && !G->state.test_playing) {
        if (G->get_mode() == GAME_MODE_MULTISEL && G->selection.m) {
            for (std::set<entity*>::iterator i = G->selection.m->begin();
                    i != G->selection.m->end(); i++) {
                entity *e = *i;

                if (e->flag_active(ENTITY_IS_PLASTIC)) {
                    enabled_tabs[TAB_PLASTIC_COLOR] = true;
                    enabled_tabs[TAB_PLASTIC_DENSITY] = true;
                }

                if (e->flag_active(ENTITY_IS_LOCKED)) {
                    any_entity_locked = true;
                }
            }
        }
    }

    for (int x=0; x<NUM_MULTI_CONFIG_TABS; ++x) {
        GtkWidget *page = gtk_notebook_get_nth_page(multi_config_nb, x);

        if (!enabled_tabs[x]) {
            gtk_widget_hide(page);
        } else {
            gtk_widget_show(page);
        }
    }

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_unlock_all), any_entity_locked);
}

static gboolean
on_multi_config_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, multi_config_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_apply, user_data)) {
        tms_debugf("cur tab: %d", multi_config_cur_tab);

        switch (multi_config_cur_tab) {
            case TAB_JOINT_STRENGTH:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_joint_strength)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_JOINT_STRENGTH, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_PLASTIC_COLOR:
                {
                    GdkRGBA color;
                    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(multi_config_plastic_color), &color);

                    tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
                    vec->r = color.red;
                    vec->g = color.green;
                    vec->b = color.blue;
                    vec->a = 1.0f;

                    P.add_action(ACTION_MULTI_PLASTIC_COLOR, (void*)vec);
                }
                break;

            case TAB_PLASTIC_DENSITY:
                {
                    float val = tclampf(gtk_range_get_value(GTK_RANGE(multi_config_plastic_density)), 0.f, 1.f);
                    P.add_action(ACTION_MULTI_PLASTIC_DENSITY, INT_TO_VOID(val * 100.f));
                }
                break;

            case TAB_CONNECTION_RENDER_TYPE:
                {
                    uint8_t render_type = CONN_RENDER_DEFAULT;

                    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_normal))) {
                        render_type = CONN_RENDER_DEFAULT;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_small))) {
                        render_type = CONN_RENDER_SMALL;
                    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(multi_config_render_type_hide))) {
                        render_type = CONN_RENDER_HIDE;
                    }

                    P.add_action(ACTION_MULTI_CHANGE_CONNECTION_RENDER_TYPE, UINT_TO_VOID(render_type));
                }
                break;

            default:
                tms_errorf("Unknown multi config tab: %d", multi_config_cur_tab);
                return false;
                break;
        }

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_unlock_all, user_data)) {
        P.add_action(ACTION_MULTI_UNLOCK_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    } else if (btn_pressed(w, multi_config_disconnect_all, user_data)) {
        P.add_action(ACTION_MULTI_DISCONNECT_ALL, 0);

        gtk_widget_hide(GTK_WIDGET(multi_config_window));
    }

    return false;
}

static void
on_multi_config_tab_changed(GtkNotebook *nb, GtkWidget *page, gint tab_num, gpointer unused)
{
    multi_config_cur_tab = tab_num;

    gtk_widget_set_sensitive(GTK_WIDGET(multi_config_apply), (tab_num != TAB_MISCELLANEOUS));
}

/** --Login **/
static void
on_login_show(GtkWidget *wdg, void *unused)
{
    gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), true);

    gtk_entry_set_text(login_username, "");
    gtk_entry_set_text(login_password, "");

    gtk_label_set_text(login_status, "");

    gtk_widget_grab_focus(GTK_WIDGET(login_username));
}

void
on_login_hide(GtkWidget *wdg, void *unused)
{
    tms_infof("login hiding");
    prompt_is_open = 0;
}

gboolean
on_login_btn_click(GtkWidget *w, GdkEventButton *ev, gpointer user_data)
{
    if (btn_pressed(w, login_btn_cancel, user_data)) {
        gtk_widget_hide(GTK_WIDGET(login_window));
    } else if (btn_pressed(w, login_btn_log_in, user_data)) {
        if (gtk_entry_get_text_length(login_username) > 0 &&
            gtk_entry_get_text_length(login_password) > 0) {
            struct login_data *data = (struct login_data*)malloc(sizeof(struct login_data));
            strcpy(data->username, gtk_entry_get_text(login_username));
            strcpy(data->password, gtk_entry_get_text(login_password));

            gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), false);
            gtk_label_set_text(login_status, "Logging in...");

            P.add_action(ACTION_LOGIN, (void*)data);
        } else {
            gtk_label_set_text(login_status, "Enter data into both fields.");
        }
    } else if (btn_pressed(w, login_btn_register, user_data)) {
        COMMUNITY_URL("register");
        ui::open_url(url);
    }

    return false;
}

gboolean
on_login_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape)
        gtk_widget_hide(w);
    else if (key->keyval == GDK_KEY_Return) {
        if (gtk_widget_has_focus(GTK_WIDGET(login_btn_cancel))) {
            on_login_btn_click(GTK_WIDGET(login_btn_cancel), NULL, GINT_TO_POINTER(1));
        } else {
            on_login_btn_click(GTK_WIDGET(login_btn_log_in), NULL, GINT_TO_POINTER(1));
        }
    }

    return false;
}

void
activate_principiawiki(GtkMenuItem *i, gpointer unused)
{
    ui::open_url("https://principia-web.se/wiki/");
}

void
activate_gettingstarted(GtkMenuItem *i, gpointer unused)
{
    ui::open_url("https://principia-web.se/wiki/Getting_Started");
}

void
activate_login(GtkMenuItem *i, gpointer unused)
{
    prompt_is_open = 1;
    P.focused = 0;
    gtk_widget_show_all(GTK_WIDGET(login_window));
}

void
editor_menu_back_to_menu(GtkMenuItem *i, gpointer unused)
{
    P.add_action(ACTION_GOTO_MAINMENU, 0);
}

static void show_grab_focus(GtkWidget *w, gpointer user_data)
{
    GdkWindow *w_window = gtk_widget_get_window(w);
    GdkDisplay* display = gdk_display_get_default();
    GdkSeat* seat = gdk_display_get_default_seat(display);
    if ((gdk_seat_get_capabilities(seat) & GDK_SEAT_CAPABILITY_KEYBOARD) == 0) {
        tms_warnf("seat has no keyboard capability");
        return;
    }
    while (gdk_seat_grab(
        seat, w_window,
        GDK_SEAT_CAPABILITY_KEYBOARD,
        FALSE,
        NULL, NULL, NULL, NULL
    ) != GDK_GRAB_SUCCESS) {
        SDL_Delay(10);
    }
}

void activate_quickadd(GtkWidget *i, gpointer unused);

gboolean
keypress_quickadd(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    GValue s = {0};
    GValue e = {0};

    g_value_init(&s, G_TYPE_UINT);
    g_value_init(&e, G_TYPE_UINT);

    g_object_get_property(G_OBJECT(w), "cursor-position", &s);
    g_object_get_property(G_OBJECT(w), "selection-bound", &e);

    guint sel = g_value_get_uint(&s)+g_value_get_uint(&e);

    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(GTK_WIDGET(quickadd_window));
    } else if (key->keyval == GDK_KEY_space
            && sel == strlen(gtk_entry_get_text(GTK_ENTRY(w)))) {
        /* if space is pressed and the whole string is selected,
         * activate it */
        activate_quickadd(w, 0);
        return true;
    }

    gtk_entry_completion_complete(gtk_entry_get_completion(GTK_ENTRY(w)));

    return false;
}

/** --Quickadd **/
static gboolean
match_selected_quickadd(GtkEntryCompletion *widget,
  GtkTreeModel       *model,
  GtkTreeIter        *iter,
  gpointer            user_data)
{
    gtk_widget_hide(GTK_WIDGET(quickadd_window));

    guint _gid;
    guint _type;
    gtk_tree_model_get(model, iter,
                       0, &_gid,
                       2, &_type,
                       -1);

    uint32_t gid = (uint32_t)_gid;
    tms_infof("selected gid %d", gid);

    switch (_type) {
        case LF_MENU:
            P.add_action(ACTION_CONSTRUCT_ENTITY, gid);
            break;

        case LF_DECORATION:
            P.add_action(ACTION_CONSTRUCT_DECORATION, gid);
            break;
    }

    return false;
}

void
refresh_quickadd()
{
    GtkListStore *list = GTK_LIST_STORE(gtk_entry_completion_get_model(gtk_entry_get_completion(quickadd_entry)));
    GtkTreeIter iter;
    int n = 0;
    for (int x=0; x<menu_objects.size(); x++) {
        const struct menu_obj &mo = menu_objects[x];

        gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter,
                0, mo.e->g_id,
                1, mo.e->get_name(),
                2, LF_MENU,
                -1
                );
    }
}

void
activate_quickadd(GtkWidget *i, gpointer unused)
{
    /* there seems to be absolutely no way of retrieving the top completion entry...
     * we have to find it manually */

    const char *search = gtk_entry_get_text(quickadd_entry);
    int len = strlen(search);
    int found_arg = -1;
    int found_score = -10000000;
    int found_lf = -1;

    tms_debugf("Looking for %s", search);

    for (int i=0; i<NUM_LF; ++i) {
        switch (i) {
            case LF_MENU:
                {
                    for (int x=0; x<menu_objects.size(); ++x) {
                        int diff = strncasecmp(search, menu_objects[x].e->get_name(), len);
                        /* Only look for 'exact' matches, meaning they must contain that exact string in the beginning
                         * i.e. 'sub' fits 'sub' and 'sublayer plank' */

                        if (diff == 0) {
                            /* Now we find out what the real difference between the match is */
                            int score = strcasecmp(search, menu_objects[x].e->get_name());

                            if (score == 0) {
                                /* A return value of 0 means it's an exacth match, i.e. 'sub' == 'sub' */
                                found_arg = menu_objects[x].e->g_id;
                                found_score = 0;
                                found_lf = i;
                                break;
                            } else if (score < 0 && score > found_score) {
                                /* Otherwise, we could settle for this half-match, i.e. 'sub' == 'sublayer plank' */
                                found_arg = menu_objects[x].e->g_id;
                                found_score = score;
                                found_lf = i;
                            }
                        }
                    }
                }
                break;

        }

        if (found_score == 0) break;
    }

    if (found_arg >= 0) {
        switch (found_lf) {
            case LF_MENU:
                P.add_action(ACTION_CONSTRUCT_ENTITY, found_arg);
                break;
        }
    } else {
        tms_infof("'%s' matched no entity name", search);
    }

    gtk_widget_hide(GTK_WIDGET(quickadd_window));
}

gboolean
on_goto_menu_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    GtkAccelGroup *accel_group = gtk_menu_get_accel_group(editor_menu);

    if (key->keyval >= GDK_KEY_1 && key->keyval <= GDK_KEY_9) {
        for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                it != editor_menu_marks.end(); ++it) {
            const struct goto_mark *mark = *it;
            GtkMenuItem *item = mark->menuitem;

            if (mark->key == key->keyval) {
                gtk_menu_item_activate(item);
                gtk_widget_hide(GTK_WIDGET(editor_menu));
                return true;
            }
        }
    }

    return false;
}

gboolean
on_menu_keypress(GtkWidget *w, GdkEventKey *key, gpointer unused)
{
    if (key->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(w);
    } else {
        /* redirect the event to tms? */
        /*
        struct tms_event e;
        e.type = TMS_EV_KEY_PRESS;

        return true;
        */
    }

    return false;
}

const gchar* css_global = R"(
    .display-cell {
        border: none;
        box-shadow: none;
        border-radius: 0;
        background: #101010;
    }

    .display-cell:checked {
        background: #5fbd5a;
    }

    .code-editor {
        font-family: "Cascadia Mono Normal", "Cascadia Mono", "Ubuntu Mono Normal", "Ubuntu Mono", monospace, mono;
        font-size: 1.25em;
    }
)";

void load_gtk_css() {
    //Load global CSS
    {
        GtkCssProvider* css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            css_provider,
            css_global,
            -1, NULL
        );
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }

    //Try to load debug.css in debug builds
    #ifdef DEBUG
    {
        GtkCssProvider* css_provider = gtk_css_provider_new();
        gtk_css_provider_load_from_path (
            css_provider,
            "debug.css",
            NULL
        );
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    #endif
}

int _gtk_loop(void *p)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return T_OK;
#endif

    gtk_init(NULL, NULL);

    //Load CSS themes
    load_gtk_css();

    g_object_set(
        gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", true,
        "gtk-tooltip-timeout", 100,
        NULL
    );

    GtkDialog *dialog;

    /** --Play menu **/
    {
        play_menu = GTK_MENU(gtk_menu_new());

        add_menuitem(play_menu, "Restart level", activate_restart_level);
        add_menuitem(play_menu, "Back", activate_back);
    }

    /** --Menu **/

    /**
     * menu header: -x, y-
     * Move player here (if adventure, creates a default robot if no player exists)
     * Move selected object here
     * -separator-
     * Go to: 0, 0
     * Go to: Player
     * Go to: Last created entity
     * Go to: Last camera position (before previous go to)
     * Go to: Plank 1543 (see marked entity note below)
     * Go to: Robot 1337
     * -separator-
     * -default menu items, open save, etc-
     *
     *  If on an entity:
     * menu header: -entity id, gid, position, angle-
     * Set as player (if adventure and clicked a creature)
     * Mark entity (marks the entity with a flag and adds it to the Go to list)
     * Unmark entity
     **/
    {
        editor_menu = GTK_MENU(gtk_menu_new());
        editor_menu_go_to_menu = GTK_MENU(gtk_menu_new());

        editor_menu_header = add_menuitem(editor_menu, "HEADER");

        /* --------------------------- */

        editor_menu_move_here_player = add_menuitem(editor_menu, "Move player here", editor_menu_activate);

        editor_menu_move_here_object = add_menuitem(editor_menu, "Move selected object here", editor_menu_activate);

        editor_menu_go_to = add_menuitem_m(editor_menu, "_Go to:");

        GtkAccelGroup *accel_group = gtk_accel_group_new();
        gtk_menu_set_accel_group(editor_menu, accel_group);

        gtk_menu_item_set_submenu(editor_menu_go_to, GTK_WIDGET(editor_menu_go_to_menu));
        {
            editor_menu_marks.push_back(new goto_mark(
                MARK_POSITION,
                "0, 0",
                0,
                tvec2f(0.f, 0.f)
            ));

            editor_menu_marks.push_back(editor_menu_last_created);
            editor_menu_marks.push_back(editor_menu_last_cam_pos);

            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;
                mark->menuitem = add_menuitem(editor_menu_go_to_menu, mark->label, editor_mark_activate, (gpointer)mark);
            }

            refresh_mark_menuitems();
        }

        /* --------------------------- */

        editor_menu_set_as_player = add_menuitem(editor_menu, "Set as player", editor_menu_activate);
        editor_menu_toggle_mark_entity = add_menuitem(editor_menu, "Mark entity", editor_menu_activate);

        /* --------------------------- */

        add_separator(editor_menu);

        editor_menu_lvl_prop = add_menuitem_m(editor_menu, "Level _properties", editor_menu_activate);

        add_menuitem_m(editor_menu, "_New level", activate_new_level);
        editor_menu_save = add_menuitem_m(editor_menu, "_Save", activate_save);
        editor_menu_save_copy = add_menuitem_m(editor_menu, "Save _copy", activate_save_copy);
        add_menuitem_m(editor_menu, "_Open", activate_open);

        editor_menu_publish = add_menuitem_m(editor_menu, "P_ublish online", activate_publish);

        editor_menu_settings = add_menuitem_m(editor_menu, "S_ettings", activate_settings);

        editor_menu_login = add_menuitem_m(editor_menu, "_Login", activate_login);

        add_menuitem_m(editor_menu, "_Back to menu", editor_menu_back_to_menu);
        add_menuitem(editor_menu, "Help: Principia Wiki", activate_principiawiki);
        add_menuitem(editor_menu, "Help: Getting Started", activate_gettingstarted);

        //g_signal_connect(editor_menu, "selection-done", G_CALLBACK(on_menu_select), 0);
        g_signal_connect(editor_menu, "key-press-event", G_CALLBACK(on_menu_keypress), 0);
        g_signal_connect(editor_menu_go_to_menu, "key-press-event", G_CALLBACK(on_goto_menu_keypress), 0);
    }

    /** --Open object **/
    {
        object_window = new_window_defaults("Import object", &on_object_show, &on_object_keypress);
        gtk_window_set_default_size(GTK_WINDOW(object_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(object_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(object_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

        object_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

        object_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(object_treemodel));
        gtk_tree_view_set_search_column(object_treeview, OC_NAME);
        g_signal_connect(GTK_WIDGET(object_treeview), "row-activated", G_CALLBACK(activate_object_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_box_set_spacing(GTK_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        object_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(object_btn_open, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        object_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(object_btn_cancel, "clicked",
                G_CALLBACK(on_object_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(object_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(object_treeview));
        gtk_container_add(GTK_CONTAINER(object_window), GTK_WIDGET(content));

        add_text_column(object_treeview, "ID", OC_ID);
        add_text_column(object_treeview, "Name", OC_NAME);
        add_text_column(object_treeview, "Version", OC_VERSION);
        add_text_column(object_treeview, "Modified", OC_DATE);
    }

    /** --Open state **/
    {
        open_state_window = new_window_defaults("Load saved game", &on_open_state_show, &on_open_state_keypress);
        gtk_window_set_default_size(GTK_WINDOW(open_state_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(open_state_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(open_state_window), true);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OSC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);

        open_state_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OSC_DATE, GTK_SORT_DESCENDING);

        open_state_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(open_state_treemodel));
        gtk_tree_view_set_search_column(open_state_treeview, OSC_NAME);
        g_signal_connect(GTK_WIDGET(open_state_treeview), "row-activated", G_CALLBACK(activate_open_state_row), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        open_state_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(open_state_btn_open, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        open_state_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(open_state_btn_cancel, "clicked",
                G_CALLBACK(on_open_state_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_state_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(open_state_treeview));
        gtk_container_add(GTK_CONTAINER(open_state_window), GTK_WIDGET(content));

        add_text_column(open_state_treeview, "Name", OSC_ID);
        add_text_column(open_state_treeview, "Modified", OSC_NAME);
    }

    /** --Open level **/
    {
        open_window = new_window_defaults("Open level", &on_open_show, &on_open_keypress);
        gtk_window_set_default_size(GTK_WINDOW(open_window), 600, 600);
        gtk_widget_set_size_request(GTK_WIDGET(open_window), 600, 600);
        gtk_window_set_resizable(GTK_WINDOW(open_window), true);

        open_menu = GTK_MENU(gtk_menu_new());

        open_menu_information = add_menuitem_m(open_menu, "_Information", open_menu_item_activated);
        open_menu_delete = add_menuitem_m(open_menu, "_Delete", open_menu_item_activated);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkListStore *store;

        store = gtk_list_store_new(OC_NUM_COLUMNS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

        open_treemodel = GTK_TREE_MODEL(store);

        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), OC_DATE, GTK_SORT_DESCENDING);

        open_treeview = GTK_TREE_VIEW(gtk_tree_view_new_with_model(open_treemodel));
        gtk_tree_view_set_search_column(open_treeview, OC_NAME);
        g_signal_connect(GTK_WIDGET(open_treeview), "row-activated", G_CALLBACK(activate_open_row), 0);
        g_signal_connect(GTK_WIDGET(open_treeview), "button-press-event", G_CALLBACK(open_row_button_press), 0);

        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);

        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Open button */
        open_btn_open   = GTK_BUTTON(gtk_button_new_with_label("Open"));
        g_signal_connect(open_btn_open, "clicked",
                G_CALLBACK(on_open_btn_click), 0);

        open_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(open_btn_cancel, "clicked",
                G_CALLBACK(on_open_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_btn_open));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(open_btn_cancel));

        gtk_box_pack_start(content, GTK_WIDGET(ew), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(open_treeview));
        gtk_container_add(GTK_CONTAINER(open_window), GTK_WIDGET(content));

        add_text_column(open_treeview, "ID", OC_ID);
        add_text_column(open_treeview, "Name", OC_NAME);
        add_text_column(open_treeview, "Version", OC_VERSION);
        add_text_column(open_treeview, "Modified", OC_DATE);
    }

    /** --Package name dialog **/
    {
        pkg_name_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Create new package",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                NULL, NULL));

        apply_dialog_defaults(pkg_name_dialog);

        pkg_name_ok = GTK_BUTTON(gtk_dialog_add_button(pkg_name_dialog, "_Save", GTK_RESPONSE_ACCEPT));
        gtk_dialog_add_button(pkg_name_dialog, "_Cancel", GTK_RESPONSE_REJECT);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(pkg_name_dialog));
        pkg_name_entry = GTK_ENTRY(gtk_entry_new());

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Enter a name for this package</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(pkg_name_entry), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(pkg_name_dialog, "show", G_CALLBACK(on_pkg_name_show), 0);
    }

    /** --Save and Save as copy **/
    {
        save_window = new_window_defaults("Save level", &on_save_show, &on_save_keypress);
        gtk_window_set_default_size(GTK_WINDOW(save_window), 400, 100);
        gtk_widget_set_size_request(GTK_WIDGET(save_window), 400, 100);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Name entry */
        save_entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(save_entry, 255);
        gtk_entry_set_activates_default(save_entry, true);

        /* Name label */
        gtk_box_pack_start(GTK_BOX(entries), new_lbl("<b>Enter a name for this level</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(entries), GTK_WIDGET(save_entry), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* OK button */
        save_ok = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(save_ok, "clicked",
                G_CALLBACK(on_save_btn_click), 0);

        /* Cancel button */
        save_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(save_cancel, "clicked",
                G_CALLBACK(on_save_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(save_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(save_cancel));

        /* Status label */
        save_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(save_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(save_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(save_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(save_window), GTK_WIDGET(content));
    }

    /** --Export **/
    {
        export_window = new_window_defaults("Export object", &on_export_show, &on_export_keypress);
        gtk_window_set_default_size(GTK_WINDOW(export_window), 400, 100);
        gtk_widget_set_size_request(GTK_WIDGET(export_window), 400, 100);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Name entry */
        export_entry = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(export_entry, 255);
        gtk_entry_set_activates_default(export_entry, true);

        /* Name label */
        gtk_box_pack_start(GTK_BOX(entries), new_lbl("<b>Enter a name for this object</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(entries), GTK_WIDGET(export_entry), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* OK button */
        export_ok = GTK_BUTTON(gtk_button_new_with_label("Save"));
        g_signal_connect(export_ok, "clicked",
                G_CALLBACK(on_export_btn_click), 0);

        /* Cancel button */
        export_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(export_cancel, "clicked",
                G_CALLBACK(on_export_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(export_ok));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(export_cancel));

        /* Status label */
        export_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(export_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(export_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(export_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(export_window), GTK_WIDGET(content));
    }


    /** --Level properties **/
    {
        properties_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Level properties",
            0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
            NULL, NULL
        ));

        apply_dialog_defaults(properties_dialog);

        g_signal_connect(properties_dialog, "show", G_CALLBACK(on_properties_show), 0);
        g_signal_connect(properties_dialog, "key-press-event", G_CALLBACK(on_properties_keypress), 0);

        GtkBox *layout = GTK_BOX(gtk_dialog_get_content_area(properties_dialog));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_widget_set_size_request(GTK_WIDGET(nb), 550, 550);
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);

        GtkGrid *tbl_info = create_settings_table();
        {
            int y = -1;

            lvl_title = GTK_ENTRY(gtk_entry_new());
            lvl_descr = GTK_TEXT_VIEW(gtk_text_view_new());

            GtkBox* lvl_type_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

            lvl_radio_adventure = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(NULL, "Adventure"));
            lvl_radio_custom = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(gtk_radio_button_get_group(lvl_radio_adventure), "Custom"));

            gtk_container_add(GTK_CONTAINER(lvl_type_box), GTK_WIDGET(lvl_radio_adventure));
            gtk_container_add(GTK_CONTAINER(lvl_type_box), GTK_WIDGET(lvl_radio_custom));

            GtkScrolledWindow *ew = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(0, 0));
            gtk_scrolled_window_set_min_content_height(ew, 64);
            gtk_scrolled_window_set_policy(ew, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
            gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(lvl_descr));

            GtkWidget *fr = gtk_frame_new(NULL);
            gtk_container_add(GTK_CONTAINER(fr), GTK_WIDGET(ew));

            add_setting_row(
                tbl_info, ++y,
                "Name", GTK_WIDGET(lvl_title)
            );

            add_setting_row(
                tbl_info, ++y,
                "Description", GTK_WIDGET(fr)
            );

            add_setting_row(
                tbl_info, ++y,
                "Type", GTK_WIDGET(lvl_type_box)
            );
        }

        GtkGrid *tbl_world = create_settings_table();
        {
            int y = -1;

            lvl_bg = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_widget_set_hexpand(GTK_WIDGET(lvl_bg), true);
            g_signal_connect(lvl_bg, "changed", G_CALLBACK(on_lvl_bg_changed), 0);

            for (int x=0; x<num_bgs; x++) {
                gtk_combo_box_text_append_text(lvl_bg, available_bgs[x]);
            }

            lvl_bg_color = GTK_COLOR_BUTTON(gtk_color_button_new());
            gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(lvl_bg_color), false);
            g_signal_connect(lvl_bg_color, "color-set", G_CALLBACK(on_lvl_bg_color_set), 0);

            GtkBox* lvl_bg_box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
            gtk_container_add(GTK_CONTAINER(lvl_bg_box), GTK_WIDGET(lvl_bg));
            gtk_container_add(GTK_CONTAINER(lvl_bg_box), GTK_WIDGET(lvl_bg_color));

            lvl_width_left = GTK_ENTRY(gtk_entry_new());
            lvl_width_right = GTK_ENTRY(gtk_entry_new());
            lvl_height_down = GTK_ENTRY(gtk_entry_new());
            lvl_height_up = GTK_ENTRY(gtk_entry_new());

            lvl_autofit = (GtkButton*)gtk_button_new_with_label("Auto-fit borders");
            g_signal_connect(lvl_autofit, "clicked",
                    G_CALLBACK(on_autofit_btn_click), 0);

            lvl_gx = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, -999, 999, 1, 1, 0)),
                        1, 0));
            lvl_gy = GTK_SPIN_BUTTON(gtk_spin_button_new(
                        GTK_ADJUSTMENT(gtk_adjustment_new(1, -999, 999, 1, 1, 0)),
                        1, 0));

            add_setting_row(
                tbl_world, ++y,
                "Background",
                GTK_WIDGET(lvl_bg_box)
            );

            add_setting_row(
                tbl_world, ++y,
                "Left border",
                GTK_WIDGET(lvl_width_left)
            );

            add_setting_row(
                tbl_world, ++y,
                "Right border",
                GTK_WIDGET(lvl_width_right)
            );

            add_setting_row(
                tbl_world, ++y,
                "Bottom border",
                GTK_WIDGET(lvl_height_down)
            );

            add_setting_row(
                tbl_world, ++y,
                "Top border",
                GTK_WIDGET(lvl_height_up)
            );

            gtk_grid_attach(
                tbl_world,
                GTK_WIDGET(lvl_autofit),
                0, ++y, 3, 1
            );

            add_setting_row(
                tbl_world, ++y,
                "Gravity X",
                GTK_WIDGET(lvl_gx)
            );

            add_setting_row(
                tbl_world, ++y,
                "Gravity Y",
                GTK_WIDGET(lvl_gy)
            );
        }

        GtkGrid *tbl_physics = create_settings_table();
        {
            int y = -1;

            lvl_pos_iter = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 255, 5));
            lvl_vel_iter = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 255, 5));

            lvl_prismatic_tol = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, .075f, 0.0125f/2.f));
            lvl_pivot_tol = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, .075f, 0.0125f/2.f));

            lvl_linear_damping = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));
            lvl_angular_damping = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));
            lvl_joint_friction = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.f, 10.0f, 0.05f));

            add_setting_row(
                tbl_physics, ++y,
                "Position iterations",
                GTK_WIDGET(lvl_pos_iter),
                "The amount of position iterations primarily affects dynamic objects. Lower = better performance."
            );

            add_setting_row(
                tbl_physics, ++y,
                "Velocity iterations",
                GTK_WIDGET(lvl_vel_iter),
                "Primarily affects motors and connection. Lower = better performance."
            );

            add_setting_row(
                tbl_physics, ++y,
                "Prismatic tolerance",
                GTK_WIDGET(lvl_prismatic_tol)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Pivot tolerance",
                GTK_WIDGET(lvl_pivot_tol)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Linear damping",
                GTK_WIDGET(lvl_linear_damping)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Angular damping",
                GTK_WIDGET(lvl_angular_damping)
            );

            add_setting_row(
                tbl_physics, ++y,
                "Joint friction",
                GTK_WIDGET(lvl_joint_friction)
            );
        }

        GtkGrid *tbl_gameplay = create_settings_table();
        {
            int y = -1;

            lvl_score = GTK_ENTRY(gtk_entry_new());

            add_setting_row(
                tbl_gameplay, ++y,
                "Final score",
                GTK_WIDGET(lvl_score),
                "What score the player has to reach to win the level."
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Level version",
                GTK_WIDGET(lvl_upgrade = (GtkButton*) gtk_button_new())
            );
            g_signal_connect(lvl_upgrade, "clicked", G_CALLBACK(on_upgrade_btn_click), 0);

            add_setting_row(
                tbl_gameplay, ++y,
                "Pause on win",
                GTK_WIDGET(lvl_pause_on_win = (GtkCheckButton*) gtk_check_button_new()),
                "Pause the simulation once the win condition has been reached."
            );

            add_setting_row(
                tbl_gameplay, ++y,
                "Display score",
                GTK_WIDGET(lvl_show_score = (GtkCheckButton*) gtk_check_button_new()),
                "Display the score in the top-right corner."
            );

            for (int x=0; x<num_gtk_level_properties; ++x) {
                struct gtk_level_property *prop = &gtk_level_properties[x];
                add_setting_row(
                    tbl_gameplay, ++y,
                    prop->label,
                    GTK_WIDGET(prop->checkbutton = GTK_CHECK_BUTTON(gtk_check_button_new())),
                    prop->help
                );
                g_signal_connect(prop->checkbutton, "toggled", G_CALLBACK(on_level_flag_toggled), UINT_TO_VOID(prop->flag));
            }
        }

        lvl_ok      = GTK_BUTTON(gtk_dialog_add_button(properties_dialog, "_OK", GTK_RESPONSE_ACCEPT));
        lvl_cancel  = GTK_BUTTON(gtk_dialog_add_button(properties_dialog, "_Cancel", GTK_RESPONSE_REJECT));

        GtkWidget *view_info = gtk_viewport_new(0,0);
        GtkWidget *win_info = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_info),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_info), GTK_WIDGET(tbl_info));
        gtk_container_set_border_width(GTK_CONTAINER(tbl_info), 5);
        gtk_container_set_border_width(GTK_CONTAINER(view_info), 0);
        gtk_container_add(GTK_CONTAINER(win_info), GTK_WIDGET(view_info));
        gtk_notebook_append_page(nb, win_info, gtk_label_new("Info"));

        GtkWidget *view_world = gtk_viewport_new(0,0);
        GtkWidget *win_world = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_world),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_world), GTK_WIDGET(tbl_world));
        gtk_container_add(GTK_CONTAINER(win_world), view_world);
        gtk_notebook_append_page(nb, win_world, gtk_label_new("World"));

        GtkWidget *view_physics = gtk_viewport_new(0,0);
        GtkWidget *win_physics = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_physics),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_physics), GTK_WIDGET(tbl_physics));
        gtk_container_add(GTK_CONTAINER(win_physics), view_physics);
        gtk_notebook_append_page(nb, win_physics, gtk_label_new("Physics"));

        GtkWidget *view_gameplay = gtk_viewport_new(0,0);
        GtkWidget *win_gameplay = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (win_gameplay),
                      GTK_POLICY_NEVER,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(view_gameplay), GTK_WIDGET(tbl_gameplay));
        gtk_container_add(GTK_CONTAINER(win_gameplay), view_gameplay);
        gtk_notebook_append_page(nb, win_gameplay, gtk_label_new("Gameplay"));

        gtk_box_pack_start(GTK_BOX(layout), GTK_WIDGET(nb), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(nb));

        gtk_widget_show_all(GTK_WIDGET(layout));
    }

    /* confirm upgrade version dialog */
    {
        confirm_upgrade_dialog = new_dialog_defaults("Confirm Upgrade");

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(confirm_upgrade_dialog));

        GtkWidget *t = gtk_label_new(0);
        gtk_label_set_markup(GTK_LABEL(t),
        "<b>Are you sure you want to upgrade the version of this level?</b>"
        "\n\n"
        "To get access to new features the version associated with this level "
        "must be upgraded. This action can not be undone. Please save a copy before "
        "upgrading your level."
        "\n\n"
        "By upgrading this level, some object properties such as density, "
        "restitution, friction and applied forces might differ from earlier versions and affect "
        "how your level is simulated."
        );
        gtk_widget_set_size_request(GTK_WIDGET(t), 400, -1);
        gtk_label_set_line_wrap(GTK_LABEL(t), true);
        gtk_box_pack_start(GTK_BOX(content), t, false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Publish **/
    {
        publish_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Publish",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Publish", GTK_RESPONSE_ACCEPT,
                "_Cancel", GTK_RESPONSE_REJECT,
                NULL));

        apply_dialog_defaults(publish_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(publish_dialog));

        publish_name = GTK_ENTRY(gtk_entry_new());
        publish_descr = GTK_TEXT_VIEW(gtk_text_view_new());
        gtk_text_view_set_wrap_mode(publish_descr, GTK_WRAP_WORD);

        GtkBox *box_locked = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
        publish_locked = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Locked"));

        gtk_box_pack_start(box_locked, GTK_WIDGET(publish_locked), 1, 1, 0);
        gtk_box_pack_start(box_locked, help_widget("Disallow other players from seeing this level outside of packages."), 0, 0, 0);

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Level name:</b>"), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(publish_name), false, false, 0);

        GtkWidget *ew = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(ew),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC
        );
        gtk_widget_set_size_request(GTK_WIDGET(ew), 400, 150);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(publish_descr));

        GtkWidget *fr = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(fr), GTK_WIDGET(ew));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Level description:</b>"), false, false, 0);

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(fr), false, false, 0);

        /* Locked box */
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(box_locked), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(publish_dialog, "show", G_CALLBACK(on_publish_show), 0);

        /* TODO: add key-press-events to everything but the cancel-button */
    }

    /** --New level **/
    {
        new_level_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "New level",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Empty Adventure", RESPONSE_EMPTY_ADVENTURE,
                "Adventure", RESPONSE_ADVENTURE,
                "Custom", RESPONSE_CUSTOM,
                NULL));

        apply_dialog_defaults(new_level_dialog);

        /* XXX: Should we add some information about the various level types? */
    }

    /** --Sandbox mode**/
    {
        mode_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Sandbox mode",
                0, (GtkDialogFlags)(0)/*GTK_DIALOG_MODAL*/,
                "Connection Edit", RESPONSE_CONN_EDIT,
                "Multi-Select", RESPONSE_MULTISEL,
                NULL));

        apply_dialog_defaults(mode_dialog);

        /* XXX: Should we add some informationa bout the varius modes? */
    }

    /** --Quickadd **/
    {
        quickadd_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        //VX: GTK_WIDGET_SET_FLAGS(quickadd_window, GTK_CAN_FOCUS);
        //VX: GTK_WINDOW(quickadd_window)->type = GTK_WINDOW_TOPLEVEL;
        gtk_window_set_decorated(GTK_WINDOW(quickadd_window), FALSE);
        //VX: gtk_window_set_has_frame(GTK_WINDOW(quickadd_window), FALSE);
        gtk_window_set_type_hint(GTK_WINDOW(quickadd_window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);

        gtk_container_set_border_width(GTK_CONTAINER(quickadd_window), 4);
        gtk_window_set_default_size(GTK_WINDOW(quickadd_window), 200, 20);
        gtk_widget_set_size_request(GTK_WIDGET(quickadd_window), 200, 20);
        gtk_window_set_resizable(GTK_WINDOW(quickadd_window), false);
        // gtk_window_set_policy(GTK_WINDOW(quickadd_window),
        //               FALSE,
        //               FALSE, FALSE);

        quickadd_entry = GTK_ENTRY(gtk_entry_new());

        GtkEntryCompletion *comp = gtk_entry_completion_new();
        GtkListStore *list = gtk_list_store_new(3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT);

        gtk_entry_completion_set_model(comp, GTK_TREE_MODEL(list));
        gtk_entry_completion_set_text_column(comp, 1);
        gtk_entry_completion_set_inline_completion(comp, false);
        gtk_entry_completion_set_inline_selection(comp, true);

#if 0
        /* add all objects from the menu */
        GtkTreeIter iter;
        for (int x=0; x<menu_objects.size(); x++) {
            gtk_list_store_append(list, &iter);
            gtk_list_store_set(list, &iter,
                    0, menu_objects[x].e->g_id,
                    1, menu_objects[x].e->get_name(),
                    1, menu_objects[x].e->get_name(),
                    -1
                    );
        }
#endif

        gtk_entry_set_completion(quickadd_entry, comp);
        gtk_container_add(GTK_CONTAINER(quickadd_window), GTK_WIDGET(quickadd_entry));

        g_signal_connect(comp, "match-selected", G_CALLBACK(match_selected_quickadd), 0);
        g_signal_connect(quickadd_window, "show", G_CALLBACK(show_grab_focus), 0);
        g_signal_connect(quickadd_window, "delete-event", G_CALLBACK(on_window_close), 0);
        g_signal_connect(quickadd_entry, "activate", G_CALLBACK(activate_quickadd), 0);
        g_signal_connect(quickadd_entry, "key-press-event", G_CALLBACK(keypress_quickadd), 0);
    }

    /** --Autosave Dialog **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Autosave prompt",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                "Open", GTK_RESPONSE_YES,
                "Remove", GTK_RESPONSE_NO,
                NULL));

        apply_dialog_defaults(dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        GtkWidget *l = gtk_label_new("Autosave file detected. Open or remove?");
        gtk_box_pack_start(GTK_BOX(content), l, false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        autosave_dialog = dialog;
    }

    /** --Info Dialog **/
    {
        info_dialog = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
        gtk_container_set_border_width(GTK_CONTAINER(info_dialog), 10);
        gtk_window_set_title(GTK_WINDOW(info_dialog), "Info");
        gtk_window_set_resizable(GTK_WINDOW(info_dialog), true);
        gtk_window_set_position(GTK_WINDOW(info_dialog), GTK_WIN_POS_CENTER);
        //gtk_window_set_keep_above(GTK_WINDOW(info_dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(info_dialog), 425, 400);

        info_name = GTK_LABEL(gtk_label_new(0));
        info_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(info_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(info_text));
        //gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(info_name), 0, 0, 0);
        //gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);
        gtk_container_add(GTK_CONTAINER(info_dialog), GTK_WIDGET(ew));

        gtk_label_set_line_wrap(GTK_LABEL(info_text), true);

        //gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(info_dialog, "show", G_CALLBACK(on_info_show), 0);
        g_signal_connect(info_dialog, "delete-event", G_CALLBACK(on_window_close), 0);

        g_signal_connect(info_dialog, "key-press-event", G_CALLBACK(on_info_keypress), 0);
    }

    /** --Error Dialog **/
    {
        error_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Errors",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                "OK", GTK_RESPONSE_ACCEPT,
                NULL));

        apply_dialog_defaults(error_dialog);

        gtk_window_set_default_size(GTK_WINDOW(error_dialog), 425, 400);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(error_dialog));

        error_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(error_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(error_text));
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);

        gtk_label_set_line_wrap(GTK_LABEL(error_text), true);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(error_dialog, "show", G_CALLBACK(on_error_show), 0);
        g_signal_connect(error_dialog, "delete-event", G_CALLBACK(on_window_close), 0);
        g_signal_connect(error_dialog, "key-press-event", G_CALLBACK(on_error_keypress), 0);
    }

    /** --Confirm Dialog **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
                "Confirm",
                0, (GtkDialogFlags)(0),/*GTK_MODAL*/
                NULL, NULL));

        apply_dialog_defaults(dialog);

        confirm_button1 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button1", 1)
                );
        confirm_button2 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button2", 2)
                );

        confirm_button3 = GTK_BUTTON(
                gtk_dialog_add_button(
                    dialog,
                    "Button3", 3)
                );

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        confirm_text = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_selectable(confirm_text, 1);
        GtkWidget *ew = gtk_scrolled_window_new(0,0);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (ew),
                      GTK_POLICY_AUTOMATIC,
                      GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(ew), GTK_WIDGET(confirm_text));
        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(ew), 1, 1, 3);

        gtk_label_set_line_wrap(GTK_LABEL(confirm_text), true);

        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(dialog, "show", G_CALLBACK(on_confirm_show), 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_confirm_keypress), 0);

        confirm_dialog = dialog;
    }

    /** --Alert Dialog **/
    {
        dialog = GTK_DIALOG(gtk_message_dialog_new(
                0, (GtkDialogFlags)(0),
                GTK_MESSAGE_INFO,
                GTK_BUTTONS_CLOSE,
                "Alert"));

        apply_dialog_defaults(dialog);

        g_signal_connect(dialog, "show", G_CALLBACK(on_alert_show), 0);
        g_signal_connect(dialog, "key-press-event", G_CALLBACK(on_alert_keypress), 0);

        alert_dialog = GTK_MESSAGE_DIALOG(dialog);
    }

    /** --Multi config **/
    {
        multi_config_window = new_window_defaults("Multi config", &on_multi_config_show);
        gtk_window_set_default_size(GTK_WINDOW(multi_config_window), 600, 350);
        gtk_widget_set_size_request(GTK_WIDGET(multi_config_window), 600, 350);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);
        g_signal_connect(nb, "switch-page", G_CALLBACK(on_multi_config_tab_changed), 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Log in button */
        multi_config_apply = GTK_BUTTON(gtk_button_new_with_label("Apply"));
        g_signal_connect(multi_config_apply, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        /* Cancel button */
        multi_config_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(multi_config_cancel, "clicked",
                G_CALLBACK(on_multi_config_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_apply));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(multi_config_cancel));

        {
            /* Joint strength */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_joint_strength = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));
            g_signal_connect(multi_config_joint_strength, "format-value", G_CALLBACK(format_joint_strength), 0);

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_joint_strength), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Settings a new joint might make your selection change it's position/state slightly.\nMake sure you save your level before you press Apply."), 0, 0, 0);

            notebook_append(nb, "Joint strength", box);
        }

        {
            /* Plastic color */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_color = GTK_COLOR_CHOOSER_WIDGET(gtk_color_chooser_widget_new());

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_color), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the color of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic color", box);
        }

        {
            /* Plastic density */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_plastic_density = GTK_SCALE(gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.05));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_plastic_density), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the density of all plastic objects in your current selection."), 1, 1, 0);

            notebook_append(nb, "Plastic density", box);
        }

        {
            /* Connection render type */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_render_type_normal = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        0, "Default"));
            multi_config_render_type_small = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Small"));
            multi_config_render_type_hide = GTK_RADIO_BUTTON(gtk_radio_button_new_with_label(
                        gtk_radio_button_get_group(multi_config_render_type_normal), "Hide"));

            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_normal), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_small), 0, 0, 0);
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_render_type_hide), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("This will change the render type of all connections in your current selection."), 1, 1, 0);

            notebook_append(nb, "Connection render type", box);
        }

        {
            /* Miscellaneous */
            GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));

            multi_config_unlock_all = new_lbtn("Unlock all", &on_multi_config_btn_click);
            multi_config_disconnect_all = new_lbtn("Disconnect all", &on_multi_config_btn_click);

            {
                GtkBox *hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));
                gtk_box_pack_start(hbox, GTK_WIDGET(multi_config_unlock_all), 1, 1, 0);
                gtk_box_pack_start(hbox, help_widget("Unlock any previously locked entities.\nOnly active if at least one of the selected entities is locked."), 0, 0, 0);

                gtk_box_pack_start(box, GTK_WIDGET(hbox), 0, 0, 0);
            }
            gtk_box_pack_start(box, GTK_WIDGET(multi_config_disconnect_all), 0, 0, 0);
            gtk_box_pack_start(box, new_lbl("Click on any of the buttons above to perform the given action on your current selection."), 1, 1, 0);

            notebook_append(nb, "Miscellaneous", box);
        }

        gtk_box_pack_start(entries, GTK_WIDGET(nb), 1, 1, 0);

        multi_config_nb = nb;

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(multi_config_window), GTK_WIDGET(content));
    }

    /** --Login **/
    {
        login_window = new_window_defaults("Log in", &on_login_show, &on_login_keypress);
        gtk_window_set_default_size(GTK_WINDOW(login_window), 400, 150);
        gtk_widget_set_size_request(GTK_WIDGET(login_window), 400, 150);

        GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *entries = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 5));
        GtkBox *bottom_content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5));

        /* Username entry */
        login_username = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(login_username, 255);
        gtk_entry_set_activates_default(login_username, true);

        /* Password entry */
        login_password = GTK_ENTRY(gtk_entry_new());
        gtk_entry_set_max_length(login_password, 255);
        gtk_entry_set_visibility(login_password, false);

        /* Username label */
        gtk_box_pack_start(entries, new_lbl("<b>Username:</b>"), false, false, 0);
        gtk_box_pack_start(entries, GTK_WIDGET(login_username), false, false, 0);

        /* Password label */
        gtk_box_pack_start(entries, new_lbl("<b>Password:</b>"), false, false, 0);
        gtk_box_pack_start(entries, GTK_WIDGET(login_password), false, false, 0);

        /* Buttons and button box */
        GtkButtonBox *button_box = GTK_BUTTON_BOX(gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL));
        gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
        gtk_box_set_spacing(GTK_BOX(button_box), 5);

        /* Log in button */
        login_btn_log_in = GTK_BUTTON(gtk_button_new_with_mnemonic("_Login"));
        g_signal_connect(login_btn_log_in, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        /* Cancel button */
        login_btn_cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
        g_signal_connect(login_btn_cancel, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        /* Register button */
        login_btn_register = GTK_BUTTON(gtk_button_new_with_label("Register"));
        g_signal_connect(login_btn_register, "clicked",
                G_CALLBACK(on_login_btn_click), 0);

        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_log_in));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_cancel));
        gtk_container_add(GTK_CONTAINER(button_box), GTK_WIDGET(login_btn_register));

        /* Status label */
        login_status = GTK_LABEL(gtk_label_new(0));
        gtk_label_set_xalign(GTK_LABEL(login_status), 0.0f);
        gtk_label_set_yalign(GTK_LABEL(login_status), 0.5f);

        gtk_box_pack_start(bottom_content, GTK_WIDGET(login_status), 1, 1, 0);
        gtk_box_pack_start(bottom_content, GTK_WIDGET(button_box), 0, 0, 0);

        gtk_box_pack_start(content, GTK_WIDGET(entries), 1, 1, 0);
        gtk_box_pack_start(content, GTK_WIDGET(bottom_content), 0, 0, 0);

        gtk_container_add(GTK_CONTAINER(login_window), GTK_WIDGET(content));

        g_signal_connect(login_window, "hide", G_CALLBACK(on_login_hide), 0);
    }

    /** --Settings **/
    {
        settings_dialog = new_dialog_defaults("Settings", &on_settings_show);
        gtk_widget_set_size_request(GTK_WIDGET(settings_dialog), 550, -1);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(settings_dialog));

        GtkNotebook *nb = GTK_NOTEBOOK(gtk_notebook_new());
        gtk_notebook_set_tab_pos(nb, GTK_POS_TOP);

        GtkGrid *tbl_graphics;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            settings_enable_shadows = GTK_CHECK_BUTTON(gtk_check_button_new());
            settings_shadow_quality = GTK_SPIN_BUTTON(gtk_spin_button_new(
                    GTK_ADJUSTMENT(gtk_adjustment_new(1, 0, 1, 1, 1, 0)),
                    1,0));
            settings_shadow_res = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(settings_shadow_res, "(native)");
            gtk_combo_box_text_append_text(settings_shadow_res, "2048x2048");
            gtk_combo_box_text_append_text(settings_shadow_res, "2048x1024");
            gtk_combo_box_text_append_text(settings_shadow_res, "1024x1024");
            gtk_combo_box_text_append_text(settings_shadow_res, "1024x512");
            gtk_combo_box_text_append_text(settings_shadow_res, "512x512");
            gtk_combo_box_text_append_text(settings_shadow_res, "512x256");

            settings_enable_ao = GTK_CHECK_BUTTON(gtk_check_button_new());
            settings_ao_res = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
            gtk_combo_box_text_append_text(settings_ao_res, "512x512");
            gtk_combo_box_text_append_text(settings_ao_res, "256x256");
            gtk_combo_box_text_append_text(settings_ao_res, "128x128");

            add_setting_row(
                tbl, ++y,
                "Enable shadows",
                GTK_WIDGET(settings_enable_shadows)
            );

            add_setting_row(
                tbl, ++y,
                "Shadow quality",
                GTK_WIDGET(settings_shadow_quality),
                "Shadow quality 0: Sharp\nShadow quality 1: Smooth"
            );

            add_setting_row(
                tbl, ++y,
                "Shadow resolution",
                GTK_WIDGET(settings_shadow_res)
            );

            add_setting_row(
                tbl, ++y,
                "Enable AO",
                GTK_WIDGET(settings_enable_ao)
            );

            add_setting_row(
                tbl, ++y,
                "AO map resolution",
                GTK_WIDGET(settings_ao_res)
            );

            for (int x=0; x<settings_num_graphic_rows; ++x) {
                struct table_setting_row *r = &settings_graphic_rows[x];
                create_setting_row_widget(r);
                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_graphics = tbl;
        }

        GtkGrid *tbl_audio;
        {
            GtkGrid *tbl = create_settings_table();
            int y = -1;

            for (int x=0; x<settings_num_audio_rows; ++x) {
                struct table_setting_row *r = &settings_audio_rows[x];

                create_setting_row_widget(r);

                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_audio = tbl;
        }

        GtkGrid *tbl_controls;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            for (int x=0; x<settings_num_control_rows; ++x) {
                struct table_setting_row *r = &settings_control_rows[x];
                create_setting_row_widget(r);
                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_controls = tbl;
        }

        GtkGrid *tbl_interface;
        {
            GtkGrid *tbl = create_settings_table();

            int y = -1;

            for (int x=0; x<settings_num_interface_rows; ++x) {
                struct table_setting_row *r = &settings_interface_rows[x];

                create_setting_row_widget(r);

                add_setting_row(
                    tbl, ++y,
                    r->label,
                    r->wdg,
                    r->help
                );
            }

            tbl_interface = tbl;
        }

        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_graphics),  new_lbl("<b>Graphics</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_audio),     new_lbl("<b>Audio</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_controls),  new_lbl("<b>Controls</b>"));
        gtk_notebook_append_page(nb, GTK_WIDGET(tbl_interface), new_lbl("<b>Interface</b>"));

        gtk_widget_show_all(GTK_WIDGET(nb));

        gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(nb), true, true, 0);

        gtk_notebook_set_current_page(nb,0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Confirm Quit Dialog **/
    {
        confirm_quit_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Confirm Quit",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            NULL, NULL
        ));

        apply_dialog_defaults(confirm_quit_dialog);

        confirm_btn_quit = GTK_BUTTON(gtk_dialog_add_button(confirm_quit_dialog, "_Quit", GTK_RESPONSE_ACCEPT));
        gtk_dialog_add_button(confirm_quit_dialog, "_Cancel", GTK_RESPONSE_REJECT);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(confirm_quit_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("<b>Are you sure you want to quit?\nAny unsaved changes will be lost!</b>"), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));

        g_signal_connect(confirm_quit_dialog, "show", G_CALLBACK(on_confirm_quit_show), 0);
    }

    /** --Puzzle play **/
    {
        puzzle_play_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Play method",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Test play", PUZZLE_TEST_PLAY,
            "Simulate", PUZZLE_SIMULATE,
            "_Cancel", GTK_RESPONSE_CANCEL,
            NULL
        ));

        apply_dialog_defaults(puzzle_play_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(puzzle_play_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Do you want to test-play the level, or just simulate it?"), false, false, 0);
        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Published **/
    {
        published_dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Level published!",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Go to level page", GTK_RESPONSE_ACCEPT,
            "_Cancel", GTK_RESPONSE_REJECT,
            NULL
        ));

        apply_dialog_defaults(published_dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(published_dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Your level has been successfully published on the community website."), false, false, 0);
        gtk_box_pack_start(GTK_BOX(content), new_lbl("To view your level, or submit it to a running contest, please click the button below."), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));
    }

    /** --Community **/
    {
        dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
            "Back to main menu?",
            0, (GtkDialogFlags)(0),/*GTK_MODAL*/
            "Yes", GTK_RESPONSE_ACCEPT,
            "No", GTK_RESPONSE_REJECT,
            NULL
        ));

        apply_dialog_defaults(dialog);

        GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(dialog));

        gtk_box_pack_start(GTK_BOX(content), new_lbl("Do you want to return to the main menu?"), false, false, 0);

        gtk_widget_show_all(GTK_WIDGET(content));

        community_dialog = dialog;
    }


    gdk_threads_add_idle(_sig_ui_ready, 0);


    gtk_main();


    return T_OK;
}

static gboolean
_sig_ui_ready(gpointer unused)
{
    SDL_LockMutex(ui_lock);
    ui_ready = SDL_TRUE;
    SDL_CondSignal(ui_cond);
    SDL_UnlockMutex(ui_lock);

    return false;
}

void ui::init()
{
    ui_lock = SDL_CreateMutex();
    ui_cond = SDL_CreateCond();
    ui_ready = SDL_FALSE;

    SDL_Thread *gtk_thread;

    gtk_thread = SDL_CreateThread(_gtk_loop, "_gtk_loop", 0);

    if (gtk_thread == NULL) {
        tms_errorf("SDL_CreateThread failed: %s", SDL_GetError());
    }
}

static gboolean
_open_play_menu(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(play_menu));
    gtk_menu_popup(play_menu, 0, 0, 0, 0, 0, gtk_get_current_event_time());

    return false;
}

static gboolean
_open_sandbox_menu(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(editor_menu));
    gtk_menu_popup(editor_menu, 0, 0, 0, 0, 0, gtk_get_current_event_time());

    if (G->state.sandbox) {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save),      true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save_copy), true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish),   true);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_lvl_prop),  true);
    } else {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save),      false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_save_copy), false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish),   false);
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_lvl_prop),  false);
    }

    if (W->is_paused()) {
        gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_move_here_object), G->selection.e != 0);

        char tmp[256];

        if (G->selection.e) {
            snprintf(tmp, 255, "- id:%u, g_id:%u, pos:%.2f/%.2f, angle:%.2f -",
                    G->selection.e->id, G->selection.e->g_id,
                    G->selection.e->get_position().x,
                    G->selection.e->get_position().y,
                    G->selection.e->get_angle()
                    );
            gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
            gtk_widget_hide(GTK_WIDGET(editor_menu_go_to));

            bool is_marked = false;

            for (std::deque<struct goto_mark*>::iterator it = editor_menu_marks.begin();
                    it != editor_menu_marks.end(); ++it) {
                struct goto_mark *mark = *it;

                if (mark != editor_menu_last_created && mark->type == MARK_ENTITY && mark->id == G->selection.e->id) {
                    is_marked = true;
                    break;
                }
            }

            char mark_entity[256];

            if (is_marked) {
                snprintf(mark_entity, 255, "Un_mark entity");
            } else {
                snprintf(mark_entity, 255, "_Mark entity");
            }
            gtk_menu_item_set_label(editor_menu_toggle_mark_entity, mark_entity);
            gtk_menu_item_set_use_underline(editor_menu_toggle_mark_entity, true);
        } else {
            b2Vec2 pos = G->get_last_cursor_pos(0);
            snprintf(tmp, 255, "- %.2f/%.2f -", pos.x, pos.y);

            gtk_widget_hide(GTK_WIDGET(editor_menu_set_as_player));
            gtk_widget_hide(GTK_WIDGET(editor_menu_toggle_mark_entity));

            gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
        }

        gtk_menu_item_set_label(editor_menu_header, tmp);

    } else {
        gtk_widget_hide(GTK_WIDGET(editor_menu_header));
        gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_player));
        gtk_widget_hide(GTK_WIDGET(editor_menu_move_here_object));
        gtk_widget_hide(GTK_WIDGET(editor_menu_go_to));
        gtk_widget_hide(GTK_WIDGET(editor_menu_set_as_player));
        gtk_widget_hide(GTK_WIDGET(editor_menu_toggle_mark_entity));
    }

    // Disable the Login button if the user is already logged in.
    gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_login), (P.user_id == 0));

    // Disable the Publish button if the user is not logged in.
    gtk_widget_set_sensitive(GTK_WIDGET(editor_menu_publish), (P.user_id != 0));

    return false;
}

static gboolean
_open_quickadd(gpointer unused)
{
    gtk_window_set_position(quickadd_window, GTK_WIN_POS_MOUSE);
    //gtk_entry_set_text(quickadd_entry, "");
    //gtk_entry_
    gtk_widget_show_all(GTK_WIDGET(quickadd_window));
    gtk_widget_grab_focus(GTK_WIDGET(quickadd_entry));
    //gtk_window_set_keep_above(GTK_WINDOW(quickadd_window), TRUE);
    tms_infof("open quickadd");

    return false;
}

static gboolean
_open_save_window(gpointer unused)
{
    activate_save(NULL, 0);

    return false;
}

static gboolean
_open_publish_dialog(gpointer unused)
{
    activate_publish(NULL, 0);

    return false;
}

static gboolean
_open_login_dialog(gpointer unused)
{
    activate_login(NULL, 0);

    return false;
}

static gboolean
_open_export(gpointer unused)
{
    activate_export(NULL, 0);

    return false;
}

static gboolean
_open_open_state_dialog(gpointer unused)
{
    activate_open_state(NULL, 0);

    return false;
}

static gboolean
_open_open_dialog(gpointer unused)
{
    activate_open(NULL, 0);

    return false;
}

static gboolean
_open_multiemitter_dialog(gpointer unused)
{
    object_window_multiemitter = true;
    activate_object(NULL, 0);

    return false;
}

static gboolean
_open_object_dialog(gpointer unused)
{
    object_window_multiemitter = false;
    activate_object(NULL, 0);

    return false;
}

static gboolean
_open_new_level_dialog(gpointer unused)
{
    activate_new_level(NULL, 0);

    return false;
}

static gboolean
_open_mode_dialog(gpointer unused)
{
    activate_mode_dialog(NULL, 0);

    return false;
}

static gboolean
_open_autosave(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));
    gint result = gtk_dialog_run(autosave_dialog);
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));

    if (result == GTK_RESPONSE_YES) {
        P.add_action(ACTION_OPEN_AUTOSAVE, 0);
    } else if (result == GTK_RESPONSE_NO) {
        P.add_action(ACTION_REMOVE_AUTOSAVE, 0);
    }

    return false;
}

static gboolean
_open_tips_dialog(gpointer unused)
{
    do {
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tips_hide), settings["hide_tips"]->v.b);

        gtk_widget_hide(GTK_WIDGET(tips_dialog));
        gint result = gtk_dialog_run(tips_dialog);
        gtk_widget_hide(GTK_WIDGET(tips_dialog));

        settings["hide_tips"]->v.b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tips_hide));

        if (result == GTK_RESPONSE_APPLY) {
            tms_infof("reshowing tips");
            continue;
        }

        if (result == GTK_RESPONSE_YES)
            ui::open_url("https://principia-web.se/wiki/");

        break;
    } while (true);

    return false;
}

static gboolean
_open_info_dialog(gpointer unused)
{
    gtk_widget_show_all(GTK_WIDGET(info_dialog));

    return false;
}

static gboolean
_open_error_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(error_dialog));
    gtk_dialog_run(error_dialog);
    gtk_widget_hide(GTK_WIDGET(error_dialog));

    return false;
}

/** --Confirm Dialog **/
static gboolean
_open_confirm_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(confirm_dialog));
    P.focused = false;
    int r = gtk_dialog_run(confirm_dialog);
    P.focused = true;

    switch (r) {
        case 1:
            // button 1 pressed
            if (confirm_action1 != ACTION_IGNORE) {
                P.add_action(confirm_action1, confirm_action1_data);
            }
            break;

        case 2:
            // button 2 pressed
            if (confirm_action2 != ACTION_IGNORE) {
                P.add_action(confirm_action2, confirm_action2_data);
            }
            break;

        case 3:
            // button 2 pressed
            if (confirm_action3 != ACTION_IGNORE) {
                P.add_action(confirm_action3, confirm_action3_data);
            }
            break;
    }

    gtk_widget_hide(GTK_WIDGET(confirm_dialog));

    return false;
}

/** --Alert Dialog **/
static gboolean
_open_alert_dialog(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(alert_dialog));
    P.focused = false;
    gtk_dialog_run(GTK_DIALOG(alert_dialog));
    P.focused = true;

    gtk_widget_hide(GTK_WIDGET(alert_dialog));

    return false;
}

static gboolean
_open_puzzle_play(gpointer unused)
{
    gint result = gtk_dialog_run(puzzle_play_dialog);

    switch (result) {
        case PUZZLE_SIMULATE:
        case PUZZLE_TEST_PLAY:
            P.add_action(ACTION_PUZZLEPLAY, (void*)(intptr_t)result);
            break;
    }

    gtk_widget_hide(GTK_WIDGET(puzzle_play_dialog));

    return false;
}

/** --Published **/
static gboolean
_open_published(gpointer unused)
{
    gint result = gtk_dialog_run(published_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        COMMUNITY_URL("level/%d", W->level.community_id);
        ui::open_url(url);
    }

    gtk_widget_hide(GTK_WIDGET(published_dialog));

    return false;
}

/** --Community **/
static gboolean
_open_community(gpointer unused)
{
    gint result = gtk_dialog_run(community_dialog);

    if (result == GTK_RESPONSE_ACCEPT) {
        P.add_action(ACTION_GOTO_MAINMENU, 0);
    }

    gtk_widget_hide(GTK_WIDGET(community_dialog));

    return false;
}

static gboolean
_open_settings(gpointer unused)
{
    activate_settings(0, 0);

    return false;
}

static gboolean
_open_multi_config(gpointer unused)
{
    g_object_set(
        G_OBJECT(multi_config_plastic_color),
        "show-editor", FALSE,
        NULL
    );

    gtk_widget_show_all(GTK_WIDGET(multi_config_window));

    return false;
}


/** --Confirm Quit Dialog **/
static gboolean
_open_confirm_quit(gpointer unused)
{
    if (gtk_dialog_run(confirm_quit_dialog) == GTK_RESPONSE_ACCEPT) {
        tms_infof("Quitting!");
        _tms.state = TMS_STATE_QUITTING;
    } else {
        tms_infof("not quitting.");
    }

    gtk_widget_hide(GTK_WIDGET(confirm_quit_dialog));

    return false;
}

static gboolean
_close_all_dialogs(gpointer unused)
{
    gtk_widget_hide(GTK_WIDGET(play_menu));
    gtk_widget_hide(GTK_WIDGET(editor_menu));
    gtk_widget_hide(GTK_WIDGET(open_window));
    gtk_widget_hide(GTK_WIDGET(open_state_window));
    gtk_widget_hide(GTK_WIDGET(object_window));
    gtk_widget_hide(GTK_WIDGET(save_window));
    gtk_widget_hide(GTK_WIDGET(properties_dialog));
    gtk_widget_hide(GTK_WIDGET(publish_dialog));
    gtk_widget_hide(GTK_WIDGET(new_level_dialog));
    gtk_widget_hide(GTK_WIDGET(mode_dialog));
    gtk_widget_hide(GTK_WIDGET(quickadd_window));
    gtk_widget_hide(GTK_WIDGET(settings_dialog));
    gtk_widget_hide(GTK_WIDGET(confirm_quit_dialog));
    gtk_widget_hide(GTK_WIDGET(puzzle_play_dialog));
    gtk_widget_hide(GTK_WIDGET(login_window));
    gtk_widget_hide(GTK_WIDGET(autosave_dialog));
    gtk_widget_hide(GTK_WIDGET(community_dialog));
    gtk_widget_hide(GTK_WIDGET(published_dialog));
    //if (cur_prompt) gtk_widget_hide(GTK_WIDGET(cur_prompt));
    return false;
}

static gboolean
_close_absolutely_all_dialogs(gpointer unused)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return false;
#endif

    _close_all_dialogs(0);
    gtk_widget_hide(GTK_WIDGET(info_dialog));
    gtk_widget_hide(GTK_WIDGET(package_window));

    return false;
}

static void wait_ui_ready()
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    SDL_LockMutex(ui_lock);
    if (!ui_ready) {
        SDL_CondWaitTimeout(ui_cond, ui_lock, 4000);
        if (!ui_ready) tms_fatalf("Could not initialise game (GTK not ready)");
    }
    SDL_UnlockMutex(ui_lock);
}

void ui::open_url(const char *url)
{
#if SDL_VERSION_ATLEAST(2,0,14)
    tms_infof("open url (SDL): %s", url);
    SDL_OpenURL(url);
#else
    #error "SDL2 2.0.14+ is required for this platform"
#endif
}

void
ui::open_dialog(int num, void *data/*=0*/)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        return;
    }
#endif

    wait_ui_ready();

    switch (num) {
        case DIALOG_SANDBOX_MENU:
			editor_menu_on_entity = 0;
			if (data) {
				editor_menu_on_entity = VOID_TO_UINT8(data);
			}

			gdk_threads_add_idle(_open_sandbox_menu, 0);
            break;

        case DIALOG_OPEN_AUTOSAVE:  gdk_threads_add_idle(_open_autosave, 0); break;
        case DIALOG_EXPORT:         gdk_threads_add_idle(_open_export, 0); break;
        case DIALOG_PLAY_MENU:      gdk_threads_add_idle(_open_play_menu, 0); break;
        case DIALOG_QUICKADD:       gdk_threads_add_idle(_open_quickadd, 0); break;
        case DIALOG_SAVE:           gdk_threads_add_idle(_open_save_window, 0); break;
        case DIALOG_OPEN:           gdk_threads_add_idle(_open_open_dialog, 0); break;

        case DIALOG_OPEN_STATE:
            if (data && VOID_TO_UINT8(data) == 1) {
                open_state_no_testplaying = true;
            } else {
                open_state_no_testplaying = false;
            }

            gdk_threads_add_idle(_open_open_state_dialog, 0);
            break;

        case DIALOG_OPEN_OBJECT:    gdk_threads_add_idle(_open_object_dialog, 0); break;
        case DIALOG_NEW_LEVEL:      gdk_threads_add_idle(_open_new_level_dialog, 0); break; /* XXX: */
        case DIALOG_SANDBOX_MODE:   gdk_threads_add_idle(_open_mode_dialog, 0); break; /* XXX: */
        case DIALOG_CONFIRM_QUIT:   gdk_threads_add_idle(_open_confirm_quit, 0); break;
        case DIALOG_PUZZLE_PLAY:    gdk_threads_add_idle(_open_puzzle_play, 0); break;
        case DIALOG_SETTINGS:       gdk_threads_add_idle(_open_settings, 0); break;
        case DIALOG_PUBLISHED:      gdk_threads_add_idle(_open_published, 0); break;
        case DIALOG_COMMUNITY:      gdk_threads_add_idle(_open_community, 0); break;
        case DIALOG_MULTI_CONFIG:   gdk_threads_add_idle(_open_multi_config, 0); break;

        case CLOSE_ALL_DIALOGS:     gdk_threads_add_idle(_close_all_dialogs, 0); break;
        case CLOSE_ABSOLUTELY_ALL_DIALOGS: gdk_threads_add_idle(_close_absolutely_all_dialogs, 0); break;

        case DIALOG_PUBLISH:        gdk_threads_add_idle(_open_publish_dialog, 0); break;
        case DIALOG_LOGIN:          gdk_threads_add_idle(_open_login_dialog, 0); break;

        default:
            tms_warnf("Unhandled dialog ID: %d", num);
            break;
    }

    gdk_display_flush(gdk_display_get_default());
}

void ui::open_sandbox_tips()
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    wait_ui_ready();

    gdk_threads_add_idle(_open_tips_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::open_help_dialog(const char *title, const char *description)
{
    wait_ui_ready();

    /* title and description are constant static strings in
     * object facotyr, should be safe to use directly
     * from any thread */
    _pass_info_name = const_cast<char*>(title);
    _pass_info_descr = const_cast<char*>(description);
    gdk_threads_add_idle(_open_info_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::set_next_action(int action_id)
{
    tms_infof("set_next_Actino: %d", action_id);
    ui::next_action = action_id;
}

void
ui::emit_signal(int num, void *data/*=0*/)
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) return;
#endif

    wait_ui_ready();

    /* XXX this stuff probably needs to be added to gdk_threads_idle_add()! */

    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            P.add_action(ui::next_action, 0);
            ui::next_action = 0;
            gtk_widget_hide(GTK_WIDGET(login_window));
            break;

        case SIGNAL_LOGIN_FAILED:
            gtk_label_set_text(login_status, "An error occured.");
            gtk_widget_set_sensitive(GTK_WIDGET(login_btn_log_in), true);
            return;

        case SIGNAL_QUICKADD_REFRESH:
            refresh_quickadd();
            break;

        case SIGNAL_REFRESH_BORDERS:
            refresh_borders();
            break;

        case SIGNAL_ENTITY_CONSTRUCTED:
            editor_menu_last_created->id = VOID_TO_UINT32(data);
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void
ui::quit()
{
    /* TODO: add proper quit stuff here */
    _tms.state = TMS_STATE_QUITTING;
}

void
ui::open_error_dialog(const char *error_msg)
{
    wait_ui_ready();

    _pass_error_text = strdup(error_msg);
    gdk_threads_add_idle(_open_error_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
#ifdef BUILD_VALGRIND
    if (RUNNING_ON_VALGRIND) {
        P.add_action(action1.action_id, 0);
        return;
    }
#endif

    wait_ui_ready();

    _pass_confirm_text    = strdup(text);
    _pass_confirm_button1 = strdup(button1);
    _pass_confirm_button2 = strdup(button2);
    if (button3) {
        _pass_confirm_button3 = strdup(button3);
    } else {
        _pass_confirm_button3 = 0;
    }

    confirm_action1 = action1.action_id;
    confirm_action2 = action2.action_id;
    confirm_action3 = action3.action_id;

    confirm_action1_data = action1.action_data;
    confirm_action2_data = action2.action_data;
    confirm_action3_data = action3.action_data;

    confirm_data = _confirm_data;

    gdk_threads_add_idle(_open_confirm_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

void
ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/)
{
    wait_ui_ready();

    if (_alert_text) {
        free(_alert_text);
    }

    _alert_type = alert_type;
    _alert_text = strdup(text);

    gdk_threads_add_idle(_open_alert_dialog, 0);

    gdk_display_flush(gdk_display_get_default());
}

#pragma GCC diagnostic pop

#endif
