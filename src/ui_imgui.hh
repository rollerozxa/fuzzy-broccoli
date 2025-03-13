
#include "entity.hh"
#include "game.hh"
#include "main.hh"
#include "material.hh"
#include "misc.hh"
#include "object_factory.hh"
#include "pkgman.hh"
#include "settings.hh"
#include "simplebg.hh"
#include "soundmanager.hh"
#include "ui.hh"
#include "world.hh"

#include "tms/backend/print.h"
#include "tms/core/texture.h"

#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_syswm.h>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "ui_imgui_impl_tms.hh"

//--------------------------------------------

//STUFF
static uint64_t __ref;
#define REF_FZERO ((float*) &(__ref = 0))
#define REF_IZERO ((int*) &(__ref = 0))
#define REF_TRUE ((bool*) &(__ref = 1))
#define REF_FALSE ((bool*) &(__ref = 0))

//constants
#define FRAME_FLAGS ImGuiWindowFlags_NoSavedSettings
#define MODAL_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)
#define POPUP_FLAGS (FRAME_FLAGS | ImGuiWindowFlags_NoMove)
#define LEVEL_NAME_LEN_SOFT_LIMIT 250
#define LEVEL_NAME_LEN_HARD_LIMIT 254
#define LEVEL_NAME_PLACEHOLDER (const char*)"<no name>"

//Unroll ImVec4 components
#define IM_XY(V) (V).x, (V).y
#define IM_ZW(V) (V).z, (V).w
#define IM_XYZ(V) (V).x, (V).y, (V).z
#define IM_XYZW(V) (V).x, (V).y, (V).z, (V).w

//HELPER FUNCTIONS

// static void unpack_rgba(uint32_t color, float *r, float *g, float *b, float *a) {
//   int _r = (color >> 24) & 0xFF;
//   int _g = (color >> 16) & 0xFF;
//   int _b = (color >>  8) & 0xFF;
//   int _a = (color      ) & 0xFF;
//   *r = _r / 255.f;
//   *g = _g / 255.f;
//   *b = _b / 255.f;
//   *a = _a / 255.f;
// }

//I stole this one from some random SO post...
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if( size_s <= 0 ){ throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);
}

//converts 0xRRGGBBAA-encoded u32 to ImVec4
static ImVec4 rgba(uint32_t color) {
    float components[4]; //ABGR
    for (int i = 0; i < 4; i++) {
        components[i] = (float)(color & 0xFF) / 255.;
        color >>= 8;
    }
    return ImVec4(components[3], components[2], components[1], components[0]);
}

//check if string should be filtered by a search query
static bool lax_search(const std::string& where, const std::string& what) {
    return std::search(
        where.begin(), where.end(),
        what.begin(), what.end(),
        [](char lhs, char rhs) { return std::tolower(lhs) == std::tolower(rhs); }
    ) != where.end();
}

//imgui helper: Center next imgui window
static void ImGui_CenterNextWindow() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always, //ImGuiCond_Appearing,
        ImVec2(0.5f, 0.5f)
    );
}

static void ImGui_BeginScaleFont(float scale) {
    ImGui::GetFont()->Scale = scale;
    ImGui::PushFont(ImGui::GetFont());
}

static void ImGui_EndScaleFont() {
    ImGui::GetFont()->Scale = 1.;
    ImGui::PopFont();
    ImGui::GetFont()->Scale = 1.;
}

//if &do_open, *do_open = false, and open popup with name
static void handle_do_open(bool *do_open, const char* name) {
    if (*do_open) {
        *do_open = false;
        ImGui::OpenPopup(name);
    }
}

// FILE LOADING //

//Load asset
std::vector<uint8_t> *load_ass(const char *path) {
    tms_infof("(imgui-backend) loading asset from %s...", path);

    FILE_IN_ASSET(true);
    FILE *file = (FILE*) _fopen(path, "rb");
    tms_assertf(file, "file not found");

    _fseek(file, 0, SEEK_END);
    size_t size = _ftell(file);
    tms_debugf("buf size %d", (int) size);
    void *buffer = malloc(size + 1);

    _fseek(file, 0, SEEK_SET);
    _fread(buffer, 1, size, file);
    _fclose(file);

    uint8_t *typed_buffer = (uint8_t*) buffer;
    std::vector<uint8_t> *vec = new std::vector<uint8_t>(typed_buffer, typed_buffer + size);
    free(buffer);

    return vec;
}

/// PFONT ///

struct PFont {
    std::vector<uint8_t> *fontbuffer;
    ImFont *font;
};

static struct PFont im_load_ttf(const char *path, float size_pixels) {
    std::vector<uint8_t>* buf = load_ass(path);

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    if (size_pixels <= 16.) {
        font_cfg.OversampleH = 3;
    }

    ImFont *font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(buf->data(), buf->size(), size_pixels, &font_cfg);

    struct PFont pfont;
    pfont.fontbuffer = buf;
    pfont.font = font;

    return pfont;
}

static struct PFont ui_font;

static void load_fonts() {
    //TODO free existing fonts

    float size_pixels = 12.f;
    size_pixels *= settings["uiscale"]->v.f;
    size_pixels = roundf(size_pixels);

    tms_infof("font size %fpx", size_pixels);

    ui_font = im_load_ttf("data/fonts/Roboto-Bold.ttf", size_pixels);

}

/* forward */
static void update_imgui_ui_scale();

/* forward */
enum class MessageType {
    Message,
    Error
};

/* forward */
namespace UiSandboxMenu  { static void open(); static void layout(); }
namespace UiPlayMenu { static void open(); static void layout(); }
namespace UiLevelManager { static void init(); static void open(); static void layout(); }
namespace UiLogin { static void open(); static void layout(); static void complete_login(int signal); }
namespace UiMessage { static void open(const char* msg, MessageType typ = MessageType::Message); static void layout(); }
namespace UiSettings { static void open(); static void layout(); }
namespace UiSandboxMode  { static void open(); static void layout(); }
namespace UiQuickadd { /*static void init();*/ static void open(); static void layout(); }
namespace UiLevelProperties { static void open(); static void layout(); }
namespace UiSave { static void open(); static void layout(); }
namespace UiNewLevel { static void open(); static void layout(); }

//On debug builds, open imgui demo window by pressing Shift+F9
#ifdef DEBUG
static bool show_demo = false;
static void ui_demo_layout() {
    if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F9) && ImGui::GetIO().KeyShift) {
        show_demo ^= 1;
    }
    if (show_demo) {
        ImGui::ShowDemoWindow(&show_demo);
    }
}
#endif

namespace UiSandboxMenu {
    static bool do_open = false;
    static b2Vec2 sb_position = b2Vec2_zero;
    static std::vector<uint32_t> bookmarks = {};

    static void open() {
        do_open = true;
        sb_position = G->get_last_cursor_pos(0);
    }

    static void layout() {
        handle_do_open(&do_open, "sandbox_menu");
        ImGui::SetNextWindowSizeConstraints(
            ImVec2(225., 0.),
            ImVec2(FLT_MAX, FLT_MAX)
        );
        if (ImGui::BeginPopup("sandbox_menu", POPUP_FLAGS)) {
            //TODO keyboard shortcuts

            //True if current level can be saved as a copy
            //Saves can only be created if current level state is sandbox
            bool is_sandbox = G->state.sandbox;

            //True if already saved and the save can be updated
            //Saves can only be updated if:
            // - Current level state is sandbox
            // - Level is local (and not an auto-save)
            // - Level is already saved
            bool can_update_save =
                    G->state.sandbox &&
                    (W->level_id_type == LEVEL_LOCAL) &&
                    (W->level.local_id != 0); //&& W->level.name_len;

            //Info panel

            //Cursor:
            ImGui::Text("Cursor: (%.2f, %.2f)", sb_position.x, sb_position.y);

            //Go to menu
            if (ImGui::BeginMenu("Go to...")) {
                float z = G->cam->_position.z;

                auto goto_entity = [z](entity *ment) {
                    G->lock();
                    b2Vec2 xy = ment->get_position();
                    G->cam->set_position(xy.x, xy.y, z);
                    G->selection.select(ment);
                    G->unlock();
                    sb_position = xy;
                };
                auto goto_position = [z](b2Vec2 xy) {
                    G->lock();
                    G->cam->set_position(xy.x, xy.y, z);
                    //XXX: should we reset the selection here?
                    G->selection.disable();
                    G->unlock();
                    sb_position = xy;
                };

                if (ImGui::MenuItem("0, 0")) {
                    goto_position(b2Vec2_zero);
                }
                //TODO the rest of goto options
                // if (ImGui::MenuItem("Player")) {
                //
                // }
                // if (ImGui::MenuItem("Last created entity")) {
                //
                // }
                // if (ImGui::MenuItem("Last camera position")) {
                //
                // }

                ImGui::Separator();
                if (bookmarks.size() > 0) {
                    for (uint32_t eid : bookmarks) {
                        //TODO: remove bookmark by right clicking
                        //XXX: maybe auto remove if id is no longer valid???
                        ImGui::PushID(eid);
                        entity* ment = W->get_entity_by_id(eid);
                        if (!ment) continue;
                        //ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                        std::string item_name = string_format("%s (id: %d)", ment->get_name(), eid);
                        bool activated = ImGui::MenuItem(item_name.c_str());
                        ImGui::SetItemTooltip(
                            "Position: (%.02f, %.02f)\n(Layer %d)",
                            // ment->get_name(),
                            // ment->g_id,
                            // eid,
                            ment->get_position().x,
                            ment->get_position().y,
                            ment->get_layer() + 1
                        );
                        if (activated) {
                            goto_entity(ment);
                        }
                        //ImGui::PopItemFlag();
                        ImGui::PopID();
                    }
                } else {
                    ImGui::BeginDisabled();
                    ImGui::TextUnformatted("<no bookmarks>");
                    ImGui::EndDisabled();
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();

            //Selected object info:
            if (G->selection.e) {
                //If an object is selected, display it's info...
                //XXX: some of this stuff does the same things as principia ui items...
                //---- consider removal?
                entity* sent = G->selection.e;
                b2Vec2 sent_pos = sent->get_position();

                //ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                bool is_bookmarked = std::find(bookmarks.begin(), bookmarks.end(), sent->id) != bookmarks.end();
                if (ImGui::MenuItem("Bookmark entity", NULL, &is_bookmarked)) {
                    ///XXX: this is UB if called multiple times with the same is_bookmarked value (which should never happen)
                    if (is_bookmarked) {
                        bookmarks.push_back(sent->id);
                    } else {
                        auto x = std::remove(bookmarks.begin(), bookmarks.end(), sent->id);
                        bookmarks.erase(x, bookmarks.end());
                    }
                }
                //ImGui::PopItemFlag();
                ImGui::SetItemTooltip("Save the entity to the 'Go to...' menu");

                bool already_at_cursor = sent_pos.x == sb_position.x && sent_pos.y == sb_position.y;
                ImGui::BeginDisabled(already_at_cursor);
                ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
                if (ImGui::MenuItem("Move to cursor" /*, NULL, already_at_cursor*/)) {
                    G->selection.e->set_position(sb_position);
                };
                ImGui::PopItemFlag();
                ImGui::EndDisabled();

                ImGui::Separator();
            }

            //"Level properties"
            if (ImGui::MenuItem("Level properties")) {
                UiLevelProperties::open();
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("New level")) {
                UiNewLevel::open();
                ImGui::CloseCurrentPopup();
            }

            //"Save": update current save
            if (can_update_save && ImGui::MenuItem("Save")) {
                P.add_action(ACTION_SAVE, 0);
                ImGui::CloseCurrentPopup();
            }

            //"Save as...": create a new save
            if (is_sandbox && ImGui::MenuItem("Save copy")) {
                //TODO
                UiSave::open();
                ImGui::CloseCurrentPopup();
            }

            // Open the Level Manager
            if (ImGui::MenuItem("Open")) {
                UiLevelManager::open();
                ImGui::CloseCurrentPopup();
            }

            //"Publish online"
            if (is_sandbox) {
                ImGui::BeginDisabled(!P.user_id);
                ImGui::MenuItem("Publish online");
                ImGui::EndDisabled();
            }

            if (P.user_id && P.username) {
                // blah
            } else {
                if (ImGui::MenuItem("Log in")) {
                    UiLogin::open();
                };
            }

            if (ImGui::MenuItem("Settings")) {
                UiSettings::open();
            }

            if (ImGui::MenuItem("Back to menu")) {
                P.add_action(ACTION_GOTO_MAINMENU, 0);
            }

            if (ImGui::MenuItem("Help: Principia Wiki")) {
                ui::open_url("https://principia-web.se/wiki/");
            }

            if (ImGui::MenuItem("Help: Getting Started")) {
                ui::open_url("https://principia-web.se/wiki/Getting_Started");
            }

            ImGui::EndMenu();
        }
    }
};

namespace UiPlayMenu {
    static bool do_open = false;

    static void open() {
        do_open = true;
    }

    static void layout() {
        handle_do_open(&do_open, "play_menu");
        if (ImGui::BeginPopup("play_menu", POPUP_FLAGS)) {
            if (ImGui::MenuItem("Controls")) {
                G->render_controls = true;
            }
            if (ImGui::MenuItem("Restart")) {
                P.add_action(ACTION_RESTART_LEVEL, 0);
            }
            if (ImGui::MenuItem("Back")) {
                P.add_action(ACTION_BACK, 0);
            }
            ImGui::EndMenu();
        }
    }
}

// TODO: open object

namespace UiLevelManager {
    struct lvlinfo_ext {
        lvlinfo info;
        uint32_t id;
        int type;
    };

    static bool do_open = false;
    static std::string search_query{""};

    static lvlfile *level_list = nullptr;
    static int level_list_type = LEVEL_LOCAL;

    static lvlinfo_ext *level_metadata = nullptr;

    static int update_level_info(int id_type, uint32_t id) {
        if (level_metadata) {
            //Check if data needs to be reloaded
            if ((level_metadata->id == id) && (level_metadata->type == id_type)) return 0;

            //Dealloc current data
            level_metadata->info.~lvlinfo();
            free(level_metadata);
        }

        level_metadata = new lvlinfo_ext;

        //Update meta
        level_metadata->id = id;
        level_metadata->type = id_type;

        //Read level info
        lvledit lvl;
        if (lvl.open(id_type, id)) {
            level_metadata->info = lvl.lvl;
            if (level_metadata->info.descr_len && level_metadata->info.descr) {
                level_metadata->info.descr = strdup(level_metadata->info.descr);
            }
            return 1;
        } else {
            delete level_metadata;
            level_metadata = nullptr;
            return -1;
        }
    }

    static void reload_level_list() {
        //Recursively deallocate the linked list
        while (level_list) {
            lvlfile* next = level_list->next;
            delete level_list;
            level_list = next;
        }
        //Get a new list of levels
        level_list = pkgman::get_levels(level_list_type);
    }

    static void init() {

    }

    static void open() {
        do_open = true;
        search_query = "";
        level_list_type = LEVEL_LOCAL;
        reload_level_list();
    }

    static void layout() {
        ImGuiIO& io = ImGui::GetIO();
        handle_do_open(&do_open, "Level Manager");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSize(ImVec2(800., 0.));
        if (ImGui::BeginPopupModal("Level Manager", REF_TRUE, MODAL_FLAGS)) {
            bool any_level_found = false;

            //Top action bar
            {
                //Align stuff to the right
                //lvlname width + padding
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 200.);

                //Actual level name field
                ImGui::PushItemWidth(200.);
                if (ImGui::IsWindowAppearing()) {
                    ImGui::SetKeyboardFocusHere();
                }
                ImGui::InputTextWithHint("##LvlmanLevelName", "Search levels", &search_query);
                ImGui::PopItemWidth();
            }

            ImGui::Separator();

            //Actual level list
            ImGui::BeginChild("save_list_child", ImVec2(0., 500.), false, FRAME_FLAGS | ImGuiWindowFlags_NavFlattened);
            if (ImGui::BeginTable("save_list", 5, ImGuiTableFlags_NoSavedSettings | ImGuiWindowFlags_NavFlattened | ImGuiTableFlags_Borders)) {
                //Setup table columns
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Last modified", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Version", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                lvlfile *level = level_list;
                while (level) {
                    //Search (lax_search is used to ignore case)
                    if ((search_query.length() > 0) && !(
                        lax_search(level->name, search_query) ||
                        (std::to_string(level->id).find(search_query) != std::string::npos)
                    )) {
                        //Just skip levels we don't like
                        level = level->next;
                        continue;
                    }

                    //This is required to prevent ID conflicts
                    ImGui::PushID(level->id);

                    //Start laying out the table row...
                    ImGui::TableNextRow();

                    //ID
                    if (ImGui::TableNextColumn()) {
                        ImGui::Text("%d", level->id);
                    }

                    //Name
                    if (ImGui::TableNextColumn()) {
                        ImGui::SetNextItemWidth(999.);
                        ImGui::LabelText("##levelname", "%s", level->name);

                        //Display description if hovered
                        if (ImGui::BeginItemTooltip()) {
                            update_level_info(level->id_type, level->id);

                            if (!level_metadata) {
                                ImGui::TextColored(ImVec4(1.,.3,.3,1.), "Failed to load level metadata");
                            } else if (level_metadata->info.descr_len && level_metadata->info.descr) {
                                ImGui::PushTextWrapPos(400);
                                ImGui::TextWrapped("%s", level_metadata->info.descr);
                                ImGui::PopTextWrapPos();
                            } else {
                                ImGui::TextColored(ImVec4(.6,.6,.6,1.), "<no description>");
                            }
                            ImGui::EndTooltip();
                        }
                    }

                    //Modified date
                    if (ImGui::TableNextColumn()) {
                        ImGui::TextUnformatted(level->modified_date);
                    }

                    //Version
                    if (ImGui::TableNextColumn()) {
                        const char* version_str = level_version_string(level->version);
                        ImGui::Text("%s", version_str);
                    }

                    //Actions
                    if (ImGui::TableNextColumn()) {
                        // Delete level ---
                        // To prevent accidental level deletion,
                        // Shift must be held while clicking the button
                        bool allow_delete = io.KeyShift;
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, allow_delete ? 1. : .6);
                        if (ImGui::Button("Delete##delete-sandbox-level")) {
                            G->lock();
                            if (allow_delete && G->delete_level(level->id_type, level->id, level->save_id)) {
                                //If deleting current local level, remove it's local_id
                                //This disables the "save" option
                                if ((level->id_type == LEVEL_LOCAL) && (level->id == W->level.local_id)) {
                                    W->level.local_id = 0;
                                }
                                //Reload the list of levels
                                reload_level_list();
                            }
                            G->unlock();
                        }
                        ImGui::PopStyleVar();
                        if (!allow_delete) ImGui::SetItemTooltip("Hold Shift to unlock");

                        // Open level ---
                        ImGui::SameLine();
                        if (ImGui::Button("Open level")) {
                            P.add_action(ACTION_OPEN, level->id);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    level = level->next;
                    any_level_found = true;

                    ImGui::PopID();
                }
                ImGui::EndTable();
                if (!any_level_found) {
                    ImGui::TextUnformatted("No levels found");
                }
                ImGui::EndChild();
            }
            ImGui::EndPopup();
        }
    }
};

namespace UiLogin {
    enum class LoginStatus {
        No,
        LoggingIn,
        ResultSuccess,
        ResultFailure
    };

    static bool do_open = false;
    static std::string username{""};
    static std::string password{""};
    static LoginStatus login_status = LoginStatus::No;

    static void complete_login(int signal) {
        switch (signal) {
            case SIGNAL_LOGIN_SUCCESS:
                login_status = LoginStatus::ResultSuccess;
                break;
            case SIGNAL_LOGIN_FAILED:
                login_status = LoginStatus::ResultFailure;
                P.user_id = 0;
                P.username = nullptr;
                username = "";
                password = "";
                break;
        }
    }

    static void open() {
        do_open = true;
        username = "";
        password = "";
        login_status = LoginStatus::No;
    }

    static void layout() {
        handle_do_open(&do_open, "Log in");
        ImGui_CenterNextWindow();
        //Only allow closing the window if a login attempt is not in progress
        bool *allow_closing = (login_status != LoginStatus::LoggingIn) ? REF_TRUE : NULL;
        if (ImGui::BeginPopupModal("Log in", allow_closing, MODAL_FLAGS)) {
            if (login_status == LoginStatus::ResultSuccess) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            bool req_username_len = username.length() > 0;
            bool req_pass_len = password.length() > 0;

            ImGui::BeginDisabled(
                (login_status == LoginStatus::LoggingIn) ||
                (login_status == LoginStatus::ResultSuccess)
            );

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }
            bool activate = false;
            activate |= ImGui::InputTextWithHint("###username", "Username", &username, ImGuiInputTextFlags_EnterReturnsTrue);
            activate |= ImGui::InputTextWithHint("###password", "Password", &password, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password);

            ImGui::EndDisabled();

            bool can_submit =
                (login_status != LoginStatus::LoggingIn) &&
                (login_status != LoginStatus::ResultSuccess) &&
                (req_pass_len && req_username_len);
            ImGui::BeginDisabled(!can_submit);
            if (ImGui::Button("  Log in  ") || (can_submit && activate)) {
                login_status = LoginStatus::LoggingIn;
                login_data *data = new login_data;
                strncpy(data->username, username.c_str(), 256);
                strncpy(data->password, password.c_str(), 256);
                P.add_action(ACTION_LOGIN, data);
            }
            ImGui::EndDisabled();

            ImGui::SameLine();

            switch (login_status) {
                case LoginStatus::LoggingIn:
                    ImGui::TextUnformatted("Logging in...");
                    break;
                case LoginStatus::ResultFailure:
                    ImGui::TextColored(ImVec4(1., 0., 0., 1.), "Login failed"); // Login attempt failed
                    break;
                default:
                    break;
            }

            ImGui::EndPopup();
        }
    }
}

namespace UiMessage {
    static bool do_open = false;
    static std::string message {""};
    static MessageType msg_type = MessageType::Error;

    static void open(const char* msg, MessageType typ /*=MessageType::Message*/) {
        do_open = true;
        msg_type = typ;
        message.assign(msg);
    }

    static void layout() {
        handle_do_open(&do_open, "###info-popup");
        ImGui_CenterNextWindow();
        const char* typ;
        switch (msg_type) {
            case MessageType::Message:
                typ = "Message###info-popup";
                break;

            case MessageType::Error:
                typ = "Error###info-popup";
                break;
        }
        ImGui::SetNextWindowSize(ImVec2(400., 0.));
        if (ImGui::BeginPopupModal(typ, NULL, MODAL_FLAGS)) {
            ImGui::TextWrapped("%s", message.c_str());
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy to clipboard")) {
                ImGui::SetClipboardText(message.c_str());
            }
            ImGui::EndPopup();
        }
    }
}

namespace UiSettings {
    static bool do_open = false;

    enum class IfDone {
        Nothing,
        Exit,
        Reload,
    };

    static IfDone if_done = IfDone::Nothing;
    static bool is_saving = false;

    static std::unordered_map<const char*, setting*> local_settings;

    static const char* copy_settings[] = {
        //GRAPHICS
        "enable_shadows",
        "shadow_quality",
        "shadow_map_resx",
        "shadow_map_resy",
        "vsync",
        "gamma_correct",
        //VOLUME
        "volume",
        "muted",
        //CONTROLS
        "cam_speed_modifier",
        "smooth_cam",
        "zoom_speed",
        "smooth_zoom",
        //INTERFACE
        "display_object_id",
        "display_fps",
        "uiscale",
        "emulate_touch",
        "rc_lock_cursor",
#ifdef DEBUG
        "debug",
#endif
        NULL
    };

    static void on_before_apply() {
        tms_infof("Preparing to reload stuff later...");
    }

    static void on_after_apply() {
        tms_infof("Now, reloading some stuff (as promised!)...");

        //Reload sound manager settings to apply new volume
        sm::load_settings();
    }

    static void save_thread() {
        tms_debugf("inside save_thread()");
        tms_infof("Waiting for can_set_settings...");
        while (!P.can_set_settings) {
            tms_debugf("Waiting for can_set_settings...");
            SDL_Delay(1);
        }
        tms_debugf("Ok, ready, saving...");
        on_before_apply();
        for (size_t i = 0; copy_settings[i] != NULL; i++) {
            tms_infof("writing setting %s", copy_settings[i]);
            memcpy(settings[copy_settings[i]], local_settings[copy_settings[i]], sizeof(setting));
        }
        tms_assertf(settings.save(), "Unable to save settings.");
        on_after_apply();
        tms_infof("Successfully saved settings, returning...");
        P.can_reload_graphics = true;
        is_saving = false;
        tms_debugf("save_thread() completed");
    }

    static void save_settings() {
        tms_infof("Saving settings...");
        is_saving = true;
        P.can_reload_graphics = false;
        P.can_set_settings = false;
        P.add_action(ACTION_RELOAD_GRAPHICS, 0);
        std::thread thread(save_thread);
        thread.detach();
    }

    static void read_settings() {
        tms_infof("Reading settings...");
        for (auto& it: local_settings) {
            tms_debugf("free %s", it.first);
            free((void*) local_settings[it.first]);
        }
        local_settings.clear();
        for (size_t i = 0; copy_settings[i] != NULL; i++) {
            tms_debugf("reading setting %s", copy_settings[i]);
            setting *heap_setting = new setting;
            memcpy(heap_setting, settings[copy_settings[i]], sizeof(setting));
            local_settings[copy_settings[i]] = heap_setting;
        }
    }

    static void open() {
        do_open = true;
        is_saving = false;
        if_done = IfDone::Nothing;
        read_settings();
    }

    static void im_resolution_picker(
        std::string friendly_name,
        const char *setting_x,
        const char *setting_y,
        const char* items[],
        int32_t items_x[],
        int32_t items_y[]
    ) {
        int item_count = 0;
        while (items[item_count] != NULL) { item_count++; }
        item_count++; //to overwrite the terminator

        std::string cust = string_format("%dx%d", local_settings[setting_x]->v.i, local_settings[setting_y]->v.i);
        items_x[item_count - 1] = local_settings[setting_x]->v.i;
        items_y[item_count - 1] = local_settings[setting_y]->v.i;
        items[item_count - 1] = cust.c_str();

        int item_current = item_count - 1;
        for (int i = 0; i < item_count; i++) {
            if (
                (items_x[i] == local_settings[setting_x]->v.i) &&
                (items_y[i] == local_settings[setting_y]->v.i)
            ) {
                item_current = i;
                break;
            }
        }

        ImGui::PushID(friendly_name.c_str());
        ImGui::TextUnformatted(friendly_name.c_str());
        ImGui::Combo("###combo", &item_current, items, (std::max)(item_count - 1, item_current + 1));
        ImGui::PopID();

        local_settings[setting_x]->v.i = items_x[item_current];
        local_settings[setting_y]->v.i = items_y[item_current];
    }

    static void layout() {
        handle_do_open(&do_open, "Settings");
        ImGui_CenterNextWindow();
        //TODO unsaved changes indicator
        if (ImGui::BeginPopupModal("Settings", is_saving ? NULL : REF_TRUE, MODAL_FLAGS)) {
            if ((if_done == IfDone::Exit) && !is_saving) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            } else if ((if_done == IfDone::Reload) && !is_saving) {
                if_done = IfDone::Nothing;
                read_settings();
            }

            if (ImGui::BeginTabBar("###settings-tabbbar")) {
                bool graphics_tab = ImGui::BeginTabItem("Graphics");
                ImGui::SetItemTooltip("Configure graphics and display settings");
                if (graphics_tab) {
                    // ImGui::BeginTable("###graphics-settings", 2);
                    // ImGui::TableNextColumn();

                    ImGui::SeparatorText("Shadows");
                    ImGui::Checkbox("Enable shadows", (bool*) &local_settings["enable_shadows"]->v.b);
                    ImGui::BeginDisabled(!local_settings["enable_shadows"]->v.b);
                    ImGui::Checkbox("Smooth shadows", (bool*) &local_settings["shadow_quality"]->v.u8);
                    {
                        const char* resolutions[] = { "4096x4096", "4096x2048", "2048x2048", "2048x1024", "1024x1024", "1024x512", "512x512", "512x256", NULL };
                        int32_t values_x[] = { 4096, 4096, 2048, 2048, 1024, 1024, 512, 512, -1 };
                        int32_t values_y[] = { 4096, 2048, 2048, 1024, 1024, 512,  512, 256, -1 };
                        im_resolution_picker(
                            "Shadow resolution",
                            "shadow_map_resx",
                            "shadow_map_resy",
                            resolutions,
                            values_x,
                            values_y
                        );
                    }
                    ImGui::EndDisabled();

                    ImGui::SeparatorText("Post-processing");

                    ImGui::Checkbox("Gamma correction", (bool*) &local_settings["gamma_correct"]->v.b);
                    ImGui::SetItemTooltip("Adjusts the brightness and contrast to ensure accurate color representation");

                    ImGui::SeparatorText("Display");

                    //VSync option has no effect on Android
                    #ifdef TMS_BACKEND_PC
                    ImGui::Checkbox("Enable V-Sync", (bool*) &local_settings["vsync"]->v.b);
                    ImGui::SetItemTooltip("Helps eliminate screen tearing by limiting the refresh rate.\nMay introduce a slight input delay.");
                    #endif

                    ImGui::EndTabItem();
                }

                bool sound_tab = ImGui::BeginTabItem("Sound");
                ImGui::SetItemTooltip("Change volume and other sound settings");
                if (sound_tab) {
                    ImGui::SeparatorText("Volume");

                    ImGui::BeginDisabled(local_settings["muted"]->v.b);
                    ImGui::SliderFloat(
                        "###volume-slider",
                        local_settings["muted"]->v.b ? REF_FZERO : ((float*) &local_settings["volume"]->v.f),
                        0.f, 1.f
                    );
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        float volume = sm::volume;
                        sm::volume = local_settings["volume"]->v.f;
                        sm::play(&sm::click, sm::position.x, sm::position.y, rand(), 1., false, 0, true);
                        sm::volume = volume;
                    }
                    ImGui::EndDisabled();

                    ImGui::Checkbox("Mute", (bool*) &local_settings["muted"]->v.b);

                    ImGui::EndTabItem();
                }

                bool controls_tab = ImGui::BeginTabItem("Controls");
                ImGui::SetItemTooltip("Mouse, keyboard and touchscreen settings");
                if (controls_tab) {
                    ImGui::EndTabItem();

                    ImGui::SeparatorText("Camera");

                    ImGui::TextUnformatted("Camera speed");
                    ImGui::SliderFloat("###Camera-speed", (float*) &local_settings["cam_speed_modifier"]->v.f, 0.1, 15.);

                    ImGui::Checkbox("Smooth camera", (bool*) &local_settings["smooth_cam"]->v.b);

                    ImGui::TextUnformatted("Zoom speed");
                    ImGui::SliderFloat("###Camera-zoom-speed", (float*) &local_settings["zoom_speed"]->v.f, 0.1, 3.);

                    ImGui::Checkbox("Smooth zoom", (bool*) &local_settings["smooth_zoom"]->v.b);

                    ImGui::SeparatorText("Mouse");

                    ImGui::Checkbox("Enable RC cursor lock", (bool*) &local_settings["rc_lock_cursor"]->v.b);
                    ImGui::SetItemTooltip("Lock the cursor while controlling RC widgets");

                    ImGui::SeparatorText("Touchscreen");

                    ImGui::Checkbox("Emulate touch", (bool*) &local_settings["emulate_touch"]->v.b);
                    ImGui::SetItemTooltip("Enable this if you use an external device other than a mouse to control Principia, such as a Wacom pad.");
                }

                bool interface_tab = ImGui::BeginTabItem("Interface");
                ImGui::SetItemTooltip("Change UI scaling, visibility options and other interface settings");
                if (interface_tab) {
                    ImGui::SeparatorText("Interface");

                    ImGui::TextUnformatted("UI Scale (requires restart)");
                    std::string display_value = string_format("%.01f", local_settings["uiscale"]->v.f);
                    ImGui::SliderFloat("###uiScale", &local_settings["uiscale"]->v.f, 0.2, 2., display_value.c_str());
                    local_settings["uiscale"]->v.f = (int)(local_settings["uiscale"]->v.f * 10) * 0.1f;

                    ImGui::TextUnformatted("Display FPS");
                    ImGui::Combo("###displayFPS", (int*) &local_settings["display_fps"]->v.u8, "Off\0On\0Graph\0Graph (Raw)\0", 4);

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();

                //This assumes separator height == 1. which results in actual height of 0
                float button_area_height =
                    ImGui::GetStyle().ItemSpacing.y + //Separator spacing
                    (ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.); // Buttons
                if (ImGui::GetContentRegionAvail().y > button_area_height) {
                    ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - button_area_height);
                }
                ImGui::Separator();
                ImGui::BeginDisabled(is_saving);
                bool do_save = false;
                if (ImGui::Button("Apply")) {
                    if_done = IfDone::Reload;
                    save_settings();
                }
                ImGui::SameLine();
                if (ImGui::Button("Save")) {
                    if_done = IfDone::Exit;
                    save_settings();
                }
                ImGui::EndDisabled();
            }
            ImGui::EndPopup();
        }
    }
}

namespace UiSandboxMode {
    static bool do_open = false;

    static void open() {
        do_open = true;
    }

    static void layout() {
        handle_do_open(&do_open, "sandbox_mode");
        if (ImGui::BeginPopup("sandbox_mode", POPUP_FLAGS)) {
            if (ImGui::MenuItem("Multiselect")) {
                G->lock();
                G->set_mode(GAME_MODE_MULTISEL);
                G->unlock();
            }
            if (ImGui::MenuItem("Connection edit")) {
                G->lock();
                G->set_mode(GAME_MODE_CONN_EDIT);
                G->unlock();
            }
            ImGui::EndPopup();
        }
    }
}

namespace UiQuickadd {
    static bool do_open = false;
    static std::string query{""};

    enum class ItemCategory {
        MenuObject,
        //TODO other categories (like items, animals etc) like in gtk3
    };
    struct SearchItem {
        ItemCategory cat;
        uint32_t id;
    };

    static bool is_haystack_inited = false;
    static std::vector<SearchItem> haystack;
    static std::vector<size_t> search_results; //referencing idx in haystack
    //last known best item, will be used in case there are no search results
    static size_t last_viable_solution;

    static std::string resolve_item_name(SearchItem item) {
        std::string name;
        switch (item.cat) {
            case ItemCategory::MenuObject: {
                const struct menu_obj &obj = menu_objects[item.id];
                name = obj.e->get_name(); // XXX: get_real_name??
                break;
            }
        }
        return name;
    }

    //TODO fuzzy search and scoring
    static std::vector<uint32_t> low_confidence;
    static void search() {
        search_results.clear();
        low_confidence.clear();
        for (int i = 0; i < haystack.size(); i++) {
            SearchItem item = haystack[i];
            std::string name = resolve_item_name(item);
            if (lax_search(name, query)) {
                search_results.push_back(i);
            } else if (lax_search(query, name)) {
                low_confidence.push_back(i);
            }
        }
        //Low confidence results are pushed after regular ones
        //Low confidence = no query in name, but name is in query.
        //So while searching for "Thick plank"...
        // Thick plank is a regular match
        // Plank is a low confidence match
        for (int i = 0; i < low_confidence.size(); i++) {
            search_results.push_back(low_confidence[i]);
        }
        tms_infof(
            "search \"%s\" %d/%d matched, (%d low confidence)",
            query.c_str(),
            (int) search_results.size(),
            (int) haystack.size(),
            (int) low_confidence.size()
        );
        if (search_results.size() > 0) {
            last_viable_solution = search_results[0];
        }
    }

    // HAYSTACK IS LAZY-INITED!
    // WE CAN'T CALL THIS RIGHT AWAY
    // AS MENU OBJECTS ARE INITED *AFTER* UI!!!
    static void init_haystack() {
        if (is_haystack_inited) return;
        is_haystack_inited = true;

        //Setup haystack
        haystack.clear();
        haystack.reserve(menu_objects.size());
        for (int i = 0; i < menu_objects.size(); i++) {
            SearchItem itm;
            itm.cat = ItemCategory::MenuObject;
            itm.id = i;
            haystack.push_back(itm);
        }
        tms_infof("init qs haystack with size %d", (int) haystack.size());
        tms_debugf("DEBUG: menu obj cnt %d", (int) menu_objects.size());

        //for opt. reasons
        search_results.reserve(haystack.size());
        low_confidence.reserve(haystack.size());
    }

    static void open() {
        do_open = true;
        query = "";
        search_results.clear();
        init_haystack();
        search();
    }

    static void activate_item(SearchItem item) {
        switch (item.cat) {
            case ItemCategory::MenuObject: {
                p_gid g_id = menu_objects[item.id].e->g_id;
                P.add_action(ACTION_CONSTRUCT_ENTITY, g_id);
                break;
            }
        }
    }

    static void layout() {
        handle_do_open(&do_open, "quickadd");
        if (ImGui::BeginPopup("quickadd", POPUP_FLAGS)) {
            if (ImGui::IsKeyReleased(ImGuiKey_Escape)) {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }
            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }

            if (ImGui::InputTextWithHint(
                "###qs-search",
                "Search for components",
                &query,
                ImGuiInputTextFlags_EnterReturnsTrue
            )) {
                if (search_results.size() > 0) {
                    activate_item(haystack[search_results[0]]);
                } else {
                    tms_infof("falling back to last best solution, can't just refuse to do anything!");
                    activate_item(haystack[last_viable_solution]);
                }
                ImGui::CloseCurrentPopup();
            };
            if (ImGui::IsItemEdited()) {
                search();
            }

            const float area_height = ImGui::GetTextLineHeightWithSpacing() * 7.25f + ImGui::GetStyle().FramePadding.y * 2.0f;
            if (ImGui::BeginChildFrame(ImGui::GetID("qsbox"), ImVec2(-FLT_MIN, area_height), ImGuiWindowFlags_NavFlattened)) {
                for (int i = 0; i < search_results.size(); i++) {
                    ImGui::PushID(i);
                    SearchItem item = haystack[search_results[i]];
                    if (ImGui::Selectable(resolve_item_name(item).c_str())) {
                        activate_item(item);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                if (search_results.size() == 0) {
                    SearchItem item = haystack[last_viable_solution];
                    if (ImGui::Selectable(resolve_item_name(item).c_str())) {
                        activate_item(item);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndChildFrame();
            }
            ImGui::EndPopup();
        }
    }
}

namespace UiLevelProperties {
    static bool do_open = false;

    static void open() {
        do_open = true;
    }

    static void layout() {
        handle_do_open(&do_open, "Level properties");
        ImGui_CenterNextWindow();
        ImGui::SetNextWindowSizeConstraints(ImVec2(450., 550.), ImVec2(FLT_MAX, FLT_MAX));
        if (ImGui::BeginPopupModal("Level properties", REF_TRUE, MODAL_FLAGS)) {
            if (ImGui::BeginTabBar("###lvlproptabbar")) {
                if (ImGui::BeginTabItem("Information")) {
                    ImGui::SeparatorText("Metadata");

                    std::string lvl_name(W->level.name, W->level.name_len);
                    bool over_soft_limit = lvl_name.length() >= LEVEL_NAME_LEN_SOFT_LIMIT + 1;
                    ImGui::BeginDisabled(over_soft_limit);
                    ImGui::TextUnformatted("Name");
                    if (ImGui::InputTextWithHint("##LevelName", LEVEL_NAME_PLACEHOLDER, &lvl_name)) {
                        size_t to_copy = (size_t)(std::min)((int) lvl_name.length(), LEVEL_NAME_LEN_HARD_LIMIT);
                        memcpy(&W->level.name, lvl_name.data(), to_copy);
                        W->level.name_len = lvl_name.length();
                    }
                    ImGui::EndDisabled();

                    std::string lvl_descr(W->level.descr, W->level.descr_len);
                    ImGui::TextUnformatted("Description");
                    if (ImGui::InputTextMultiline("###LevelDescr", &lvl_descr)) {
                        W->level.descr = strdup(lvl_descr.c_str());
                        W->level.descr_len = lvl_descr.length();
                    }

                    ImGui::SeparatorText("Type");
                    if (ImGui::RadioButton("Adventure", W->level.type == LCAT_ADVENTURE)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_ADVENTURE);
                    }
                    if (ImGui::RadioButton("Custom", W->level.type == LCAT_CUSTOM)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_CUSTOM);
                    }
#if 0
                    if (ImGui::RadioButton("Puzzle", W->level.type == LCAT_PUZZLE)) {
                        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)LCAT_PUZZLE);
                    }
#endif

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World")) {
                    //Background
                    {
                        static int current_item = 0;
                            const char* items[] = { "Item 1", "Item 2", "Item 3", "Item 4" };

                        if (ImGui::Combo("Background", &current_item, available_bgs, num_bgs)) {
                            W->level.bg = current_item;
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                        ImGuiStyle style = ImGui::GetStyle();

                    }

                    //Gravity
                    {
                        ImGui::SeparatorText("Gravity");
                        ImGui::SliderFloat("X###gravityx", &W->level.gravity_x, -40., 40., "%.01f");
                        ImGui::SliderFloat("Y###gravityy", &W->level.gravity_y, -40., 40., "%.01f");
                    }
                    //ImGui::SameLine();


                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Physics")) {
                    ImGui::TextUnformatted("These settings can affect simulation performance.\nyada yada yada this is a placeholder text\nfor the physics tab :3");

                    auto reload_if_changed = [](){
                        if (ImGui::IsItemDeactivatedAfterEdit()) {
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                    };
                    auto slider_uint8t = [](uint8_t* x) {
                        int tmp = (int)*x;
                        //HACK: use pointer as unique id
                        ImGui::PushID((size_t)x);
                        if (ImGui::SliderInt("###slider", &tmp, 0, 255)) {
                            *x = tmp & 0xff;
                        }
                        ImGui::PopID();
                    };

                    ImGui::SeparatorText("Iteration count");

                    ImGui::TextUnformatted("Position interations");
                    slider_uint8t(&W->level.position_iterations);
                    reload_if_changed();

                    ImGui::TextUnformatted("Velocity interations");
                    slider_uint8t(&W->level.velocity_iterations);
                    reload_if_changed();

                    //TODO add the rest of the physics settings

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Gameplay")) {
                    auto lvl_flag_toggle = [](uint64_t flag, const char *label, const char *help, bool disabled = false) {
                        bool x = (W->level.flags & flag) != 0;
                        if (disabled) {
                            ImGui::BeginDisabled();
                        }
                        if (ImGui::Checkbox(label, &x)) {
                            if (x) {
                                W->level.flags |= flag;
                            } else {
                                W->level.flags &= ~flag;
                            }
                            P.add_action(ACTION_RELOAD_LEVEL, 0);
                        }
                        if (disabled) {
                            ImGui::EndDisabled();
                        }
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip | ImGuiHoveredFlags_AllowWhenDisabled)) {
                            if (ImGui::BeginTooltip()) {
                                if ((help != 0) && (*help != 0)) {
                                    ImGui::TextUnformatted(help);
                                } else {
                                    ImGui::BeginDisabled();
                                    ImGui::TextUnformatted("<no help available>");
                                    ImGui::EndDisabled();
                                }
                                ImGui::EndTooltip();
                            }
                        }
                    };

                    if (ImGui::BeginChild("###gameplay-scroll", ImVec2(0, ImGui::GetContentRegionAvail().y))) {
                        ImGui::SeparatorText("Flags");
                        lvl_flag_toggle(
                            LVL_DISABLE_LAYER_SWITCH,
                            "Disable layer switch",
                                "In adventure mode, disable manual robot layer switching.\nIn puzzle mode, restrict layer switching for objects.",
                            !((W->level.type == LCAT_PUZZLE) || (W->level.type == LCAT_ADVENTURE))
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_INTERACTIVE,
                            "Disable interactive",
                            "Disable the ability to handle interactive objects."
                        );
                         lvl_flag_toggle(
                            LVL_DISABLE_CONNECTIONS,
                            "Disable connections",
                            "Disable the ability to create connections\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );
                        lvl_flag_toggle(
                            LVL_DISABLE_STATIC_CONNS,
                            "Disable static connections",
                            "Disable connections to static objects such as platforms\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );
                        lvl_flag_toggle(
                            LVL_SNAP,
                            "Snap by default",
                            "When the player drags or rotates an object it will snap to a grid by default (good for easy beginner levels).\n(Puzzle mode only)",
                            W->level.type != LCAT_PUZZLE
                        );
                    }
                    ImGui::EndChild();

                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::EndPopup();
        }
    }
}

namespace UiSave {
    static bool do_open = false;
    static std::string level_name{""};

    static void open() {
        do_open = true;
        size_t sz = (std::min)((int) W->level.name_len, LEVEL_NAME_LEN_HARD_LIMIT);
        level_name = std::string((const char*) &W->level.name, sz);
        if (level_name == std::string{LEVEL_NAME_PLACEHOLDER}) {
            level_name = "";
        }
    }

    static void layout() {
        handle_do_open(&do_open, "###sas");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("Save as...###sas", REF_TRUE, MODAL_FLAGS)) {
            ImGuiStyle& style = ImGui::GetStyle();

            ImGui::TextUnformatted("Level name:");

            //Level name input field
            if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
            bool activate = ImGui::InputTextWithHint(
                "###levelname",
                LEVEL_NAME_PLACEHOLDER,
                &level_name,
                ImGuiInputTextFlags_EnterReturnsTrue
            );

            //Validation
            bool invalid = level_name.length() > LEVEL_NAME_LEN_SOFT_LIMIT;

            //Char counter, X/250
            float cpy = ImGui::GetCursorPosY();
            ImGui::SetCursorPosY(cpy + style.FramePadding.y);
            ImGui::TextColored(
                invalid ? ImColor(255, 0, 0) : ImColor(1.f, 1.f, 1.f, style.DisabledAlpha),
                "%zu/%d", level_name.length(), LEVEL_NAME_LEN_SOFT_LIMIT
            );
            ImGui::SetCursorPosY(cpy);

            //Save button, right-aligned
            const char *save_str = "Save";
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - (ImGui::CalcTextSize(save_str).x + style.FramePadding.x * 2.));
            ImGui::BeginDisabled(invalid);
            if (ImGui::Button(save_str)  || (activate && !invalid)) {
                size_t sz = (std::min)((int) level_name.length(), LEVEL_NAME_LEN_SOFT_LIMIT);
                memcpy((char*) &W->level.name, level_name.c_str(), sz);
                W->level.name_len = sz;
                P.add_action(ACTION_SAVE_COPY, 0);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndDisabled();

            ImGui::EndPopup();
        }
    }
}

namespace UiNewLevel {
    static bool do_open = false;

    static void open() {
        do_open = true;
    }

    static void layout() {
        handle_do_open(&do_open, "###new-level");
        ImGui_CenterNextWindow();
        if (ImGui::BeginPopupModal("New level###new-level", REF_TRUE, MODAL_FLAGS)) {

            if (ImGui::Button("Custom")) {
                P.add_action(ACTION_NEW_LEVEL, LCAT_CUSTOM);
                ImGui::CloseCurrentPopup();
            }

#if 0
            if (ImGui::Button("Puzzle")) {
                P.add_action(ACTION_NEW_LEVEL, LCAT_PUZZLE);
                ImGui::CloseCurrentPopup();
            }
#endif

            ImGui::EndPopup();
        }
    }
}

static void ui_init() {
    UiLevelManager::init();
    //UiQuickadd::init();
}

static void ui_layout() {
#ifdef DEBUG
    ui_demo_layout();
#endif
    UiSandboxMenu::layout();
    UiPlayMenu::layout();
    UiLevelManager::layout();
    UiLogin::layout();
    UiMessage::layout();
    UiSettings::layout();
    UiSandboxMode::layout();
    UiQuickadd::layout();
    UiLevelProperties::layout();
    UiSave::layout();
    UiNewLevel::layout();
}

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*

#if defined(TMS_BACKEND_PC) && !defined(NO_UI)
int prompt_is_open = 0;
#endif

static void update_imgui_ui_scale() {
    float scale_factor = settings["uiscale"]->v.f;
    ImGui::GetStyle().ScaleAllSizes(scale_factor);

    //ImGui::GetIO().FontGlobalScale = roundf(9. * scale_factor) / 9.;
}

static void principia_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle *style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    //Rounding
    style->FrameRounding  = style->GrabRounding  = 2.3f;
    style->WindowRounding = style->PopupRounding = style->ChildRounding = 3.0f;

    //style->FrameBorderSize = .5;

    //TODO style
    //colors[ImGuiCol_WindowBg]    = rgba(0xfdfdfdff);
    //colors[ImGuiCol_ScrollbarBg] = rgba(0x767676ff);
    //colors[ImGuiCol_ScrollbarGrab] = rgba(0x767676ff);
    //colors[ImGuiCol_ScrollbarGrabActive] = rgba(0xb1b1b1);
}

static bool init_ready = false;

void ui::init() {
    tms_assertf(!init_ready, "ui::init called twice");

    //create context
#ifdef DEBUG
    IMGUI_CHECKVERSION();
#endif
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    //set flags
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableKeyboard;
    //io.ConfigInputTrickleEventQueue = false;
    io.ConfigWindowsResizeFromEdges = true; //XXX: not active until custom cursors are implemented...
    io.ConfigDragClickToInputText = true;
    //Disable saving state/logging
    io.IniFilename = NULL;
    io.LogFilename = NULL;

    //style
    principia_style();

    //update scale
    update_imgui_ui_scale();

    //load fonts
    load_fonts();

    //ensure gl ctx exists
    tms_assertf(_tms._window != NULL, "window does not exist yet");
    tms_assertf(SDL_GL_GetCurrentContext() != NULL, "no gl ctx");

    //init
    if (!ImGui_ImplOpenGL3_Init()) {
        tms_fatalf("(imgui-backend) gl impl init failed");
    }

    if (ImGui_ImplTMS_Init() != T_OK) {
        tms_fatalf("(imgui-backend) tms impl init failed");
    }

    //call ui_init
    ui_init();

    init_ready = true;
}

void ui::render() {
    if (settings["render_gui"]->is_false()) return;

    tms_assertf(init_ready, "ui::render called before ui::init");
    tms_assertf(GImGui != NULL, "gimgui is null. is imgui ready?");

    ImGuiIO& io = ImGui::GetIO();

    //start frame
    if (ImGui_ImplTms_NewFrame() <= 0) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    ImGui::PushFont(ui_font.font);

    //layout
    ui_layout();

    ImGui::PopFont();

    //render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ui::open_dialog(int num, void *data) {
    tms_assertf(init_ready, "ui::open_dialog called before ui::init");
    switch (num) {
        //XXX: this gets called after opening the sandbox menu, closing it immediately
        case CLOSE_ABSOLUTELY_ALL_DIALOGS:
        case CLOSE_ALL_DIALOGS:
            tms_infof("XXX: CLOSE_ALL_DIALOGS/CLOSE_ABSOLUTELY_ALL_DIALOGS (200/201) are intentionally ignored");
            break;
        case DIALOG_SANDBOX_MENU:
            UiSandboxMenu::open();
            break;
        case DIALOG_PLAY_MENU:
            UiPlayMenu::open();
            break;
        case DIALOG_OPEN:
            UiLevelManager::open();
            break;
        case DIALOG_LOGIN:
            UiLogin::open();
            break;
        case DIALOG_SETTINGS:
            UiSettings::open();
            break;
        case DIALOG_SANDBOX_MODE:
            UiSandboxMode::open();
            break;
        case DIALOG_QUICKADD:
            UiQuickadd::open();
            break;
        case DIALOG_LEVEL_PROPERTIES:
            UiLevelProperties::open();
            break;
        case DIALOG_SAVE:
        case DIALOG_SAVE_COPY:
            UiSave::open();
            break;
        case DIALOG_NEW_LEVEL:
            UiNewLevel::open();
            break;
        default:
            tms_errorf("dialog %d not implemented yet", num);
    }
}

void ui::open_url(const char *url) {
    tms_infof("open url: %s", url);
    #if SDL_VERSION_ATLEAST(2,0,14)
        SDL_OpenURL(url);
    #else
        #error "SDL2 2.0.14+ is required"
    #endif
}

void ui::open_help_dialog(const char* title, const char* description) {
    tms_errorf("ui::open_help_dialog not implemented yet");
}

void ui::emit_signal(int num, void *data){
    switch (num) {
        case SIGNAL_LOGIN_SUCCESS:
            UiLogin::complete_login(num);
            if (ui::next_action != ACTION_IGNORE) {
                P.add_action(ui::next_action, 0);
                ui::next_action = ACTION_IGNORE;
            }
            break;
        case SIGNAL_LOGIN_FAILED:
            ui::next_action = ACTION_IGNORE;
            UiLogin::complete_login(num);
            break;
    }
}

void ui::quit() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplTMS_Shutdown();
    ImGui::DestroyContext();
}

void ui::set_next_action(int action_id) {
    tms_infof("set_next_action %d", action_id);
    ui::next_action = action_id;
}

void ui::open_error_dialog(const char *error_msg) {
    UiMessage::open(error_msg, MessageType::Error);
}

void ui::confirm(
    const char *text,
    const char *button1, principia_action action1,
    const char *button2, principia_action action2,
    const char *button3, principia_action action3,
    struct confirm_data _confirm_data
) {
    //TODO
    UiMessage::open(text, MessageType::Message);
    P.add_action(action1.action_id, 0);
    tms_errorf("ui::confirm not implemented yet");
}

void ui::alert(const char* text, uint8_t type) {
    UiMessage::open(text, MessageType::Message);
}

//NOLINTEND(misc-definitions-in-headers)

//ї
