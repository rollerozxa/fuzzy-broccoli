
#if defined(TMS_BACKEND_ANDROID)

#include "SDL.h"
#include "network.hh"
#include <jni.h>
#include <sstream>

void ui::init(){};

void
ui::set_next_action(int action_id)
{
    ui::next_action = action_id;
}

/* TODO: handle this in some way */
void ui::emit_signal(int signal_id, void *data/*=0*/)
{
    switch (signal_id) {
        case SIGNAL_LOGIN_SUCCESS:
            P.add_action(ui::next_action, 0);
            break;

        case SIGNAL_LOGIN_FAILED:
            /* XXX */
            break;

        case SIGNAL_REGISTER_SUCCESS:
            ui::open_dialog(CLOSE_REGISTER_DIALOG);
            tms_infof("Register success!!!!!!!!!");
            break;

        case SIGNAL_REGISTER_FAILED:
            ui::open_dialog(DISABLE_REGISTER_LOADER);
            tms_infof("Register failed!!!!!!!!!");
            break;

        case SIGNAL_REFRESH_BORDERS:
            /* XXX */
            break;

        default:
            {
                /* By default, passthrough the signal to the Java part */
                JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
                jobject activity = (jobject)SDL_AndroidGetActivity();
                jclass cls = env->GetObjectClass(activity);

                jmethodID mid = env->GetStaticMethodID(cls, "emit_signal", "(I)V");

                if (mid) {
                    env->CallStaticVoidMethod(cls, mid, (jvalue*)(jint)signal_id);
                }
            }
            break;
    }

    ui::next_action = ACTION_IGNORE;
}

void ui::open_url(const char *url)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "open_url", "(Ljava/lang/String;)V");

    if (mid) {
        jstring str = env->NewStringUTF(url);
        env->CallStaticVoidMethod(cls, mid, (jvalue*)str);
    }
}

void
ui::confirm(const char *text,
        const char *button1, principia_action action1,
        const char *button2, principia_action action2,
        const char *button3/*=0*/, principia_action action3/*=ACTION_IGNORE*/,
        struct confirm_data _confirm_data/*=none*/
        )
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "confirm", "(Ljava/lang/String;Ljava/lang/String;IJLjava/lang/String;IJLjava/lang/String;IJZ)V");

    if (mid) {
        jstring _text = env->NewStringUTF(text);
        jstring _button1 = env->NewStringUTF(button1);
        jstring _button2 = env->NewStringUTF(button2);
        jstring _button3 = env->NewStringUTF(button3 ? button3 : "");
        env->CallStaticVoidMethod(cls, mid,
                _text,
                _button1, (jint)action1.action_id, (jlong)action1.action_data,
                _button2, (jint)action2.action_id, (jlong)action2.action_data,
                _button3, (jint)action3.action_id, (jlong)action3.action_data,
                (jboolean)_confirm_data.confirm_type == CONFIRM_TYPE_BACK_SANDBOX);
    } else {
        tms_errorf("Unable to run confirm");
    }
}

void
ui::alert(const char *text, uint8_t alert_type/*=ALERT_INFORMATION*/)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Principia", text, NULL);
}

void
ui::open_error_dialog(const char *error_msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", error_msg, NULL);
}

void
ui::open_dialog(int num, void *data/*=0*/)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "open_dialog", "(IZ)V");

    if (mid) {
        env->CallStaticVoidMethod(cls, mid, (jvalue*)(jint)num, (jboolean)(data ? true : false));
    }
}

void
ui::quit()
{
    _tms.state = TMS_STATE_QUITTING;
}

void ui::open_help_dialog(const char *title, const char *description)
{
    JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
    jobject activity = (jobject)SDL_AndroidGetActivity();
    jclass cls = env->GetObjectClass(activity);

    jmethodID mid = env->GetStaticMethodID(cls, "showHelpDialog", "(Ljava/lang/String;Ljava/lang/String;)V");

    if (mid) {
        jstring t = env->NewStringUTF(title);
        jstring d = env->NewStringUTF(description);
        env->CallStaticVoidMethod(cls, mid, (jvalue*)t, (jvalue*)d);
    } else
        tms_errorf("could not run showHelpDialog");
}

/** ++Generic **/

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelPage(JNIEnv *env, jclass jcls)
{
    COMMUNITY_URL("level/%d", W->level.community_id);

    return env->NewStringUTF(url);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCommunityHost(JNIEnv *env, jclass jcls)
{
    return env->NewStringUTF(P.community_host);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCookies(JNIEnv *env, jclass jcls)
{
    char *token;
    P_get_cookie_data(&token);

    return env->NewStringUTF(token);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addAction(JNIEnv *env, jclass jcls,
        jint action_id, jstring action_string)
{
    SDL_LockMutex(P.action_mutex);
    if (P.num_actions < MAX_ACTIONS) {
        P.actions[P.num_actions].id = (int)action_id;

        const char *str = env->GetStringUTFChars(action_string, 0);
        P.actions[P.num_actions].id = (int)action_id;
        P.actions[P.num_actions].data = INT_TO_VOID(atoi(str));
        P.num_actions ++;

        env->ReleaseStringUTFChars(action_string, str);
    }
    SDL_UnlockMutex(P.action_mutex);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsInt(JNIEnv *env, jclass jcls,
        jint action_id, jlong action_data)
{
    uint32_t d = (uint32_t)((int64_t)action_data);
    P.add_action(action_id, d);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsVec4(JNIEnv *env, jclass jcls,
        jint action_id, jfloat r, jfloat g, jfloat b, jfloat a)
{
    tvec4 *vec = (tvec4*)malloc(sizeof(tvec4));
    vec->r = r;
    vec->g = g;
    vec->b = b;
    vec->a = a;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsPair(JNIEnv *env, jclass jcls,
        jint action_id, jlong data0, jlong data1)
{
    uint32_t *vec = (uint32_t*)malloc(sizeof(uint32_t)*2);
    vec[0] = data0;
    vec[1] = data1;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_addActionAsTriple(JNIEnv *env, jclass jcls,
        jint action_id, jlong data0, jlong data1, jlong data2)
{
    uint32_t *vec = (uint32_t*)malloc(sizeof(uint32_t)*3);
    vec[0] = data0;
    vec[1] = data1;
    vec[2] = data2;
    P.add_action(action_id, (void*)vec);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setMultiemitterObject(JNIEnv *env, jclass jcls,
        jlong level_id)
{
    P.add_action(ACTION_MULTIEMITTER_SET, (uint32_t)level_id);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setImportObject(JNIEnv *env, jclass jcls,
        jlong level_id)
{
    P.add_action(ACTION_SELECT_IMPORT_OBJECT, (uint32_t)level_id);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getPropertyString(JNIEnv *env, jclass _jcls, jint property_index)
{
    char *nm = 0;
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_STR) {
        nm = e->properties[property_index].v.s.buf;
    }

    if (nm == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(nm);
}

extern "C" jlong
Java_org_libsdl_app_PrincipiaBackend_getPropertyInt(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT) {
        return (jlong)e->properties[property_index].v.i;
    }

    return 0;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getPropertyInt8(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT8) {
        return (jint)e->properties[property_index].v.i8;
    }

    return 0;
}

extern "C" jfloat
Java_org_libsdl_app_PrincipiaBackend_getPropertyFloat(JNIEnv *env, jclass _jcls, jint property_index)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_FLT) {
        return (jfloat)G->selection.e->properties[property_index].v.f;
    }

    return 0.f;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyString(JNIEnv *env, jclass _jcls,
        jint property_index, jstring value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_STR) {
        const char *tmp = env->GetStringUTFChars(value, 0);
        e->set_property(property_index, tmp);
        env->ReleaseStringUTFChars(value, tmp);
    } else {
        tms_errorf("Invalid set_property string");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyInt(JNIEnv *env, jclass _jcls,
        jint property_index, jlong value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT) {
        e->properties[property_index].v.i = (uint32_t)value;
    } else {
        tms_errorf("Invalid set_property int");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyInt8(JNIEnv *env, jclass _jcls,
        jint property_index, jint value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_INT8) {
        e->properties[property_index].v.i8 = (uint8_t)value;
    } else {
        tms_errorf("Invalid set_property int8");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPropertyFloat(JNIEnv *env, jclass _jcls,
        jint property_index, jfloat value)
{
    entity *e = G->selection.e;

    if (e && property_index < e->num_properties && e->properties[property_index].type == P_FLT) {
        e->properties[property_index].v.f = (float)value;
    } else {
        tms_errorf("Invalid set_property float");
    }
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_createObject(JNIEnv *env, jclass _jcls,
        jstring _name)
{
    const char *name = env->GetStringUTFChars(_name, 0);
    /* there seems to be absolutely no way of retrieving the top completion entry...
     * we have to find it manually */

    int len = strlen(name);
    uint32_t gid = 0;
    entity *found = 0;

    for (int x=0; x<menu_objects.size(); x++) {
        if (strncasecmp(name, menu_objects[x].e->get_name(), len) == 0) {
            found = menu_objects[x].e;
            break;
        }
    }

    if (found) {
        uint32_t g_id = found->g_id;
        P.add_action(ACTION_CONSTRUCT_ENTITY, g_id);
    } else
        tms_infof("'%s' matched no entity name", name);

    env->ReleaseStringUTFChars(_name, name);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getObjects(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    tms_infof("menu_objects size: %d", (int)menu_objects.size());
    for (int x=0; x<menu_objects.size(); x++) {
        const char *n = menu_objects[x].e->get_name();
        if (x != 0) b << ',';
        b << n;
    }

    tms_infof("got objects: '%s'", b.str().c_str());

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jobject
Java_org_libsdl_app_PrincipiaBackend_getSettings(JNIEnv *env, jclass _jcls)
{
    jobject ret = 0;
    jclass cls = 0;
    jmethodID constructor;

    cls = env->FindClass("com/bithack/principia/shared/Settings");

    if (cls) {
        constructor = env->GetMethodID(cls, "<init>", "()V");
        if (constructor) {
            ret = env->NewObject(cls, constructor);

            if (ret) {
                jfieldID f;

                f = env->GetFieldID(cls, "enable_shadows", "Z");
                env->SetBooleanField(ret, f, settings["enable_shadows"]->v.b);

                f = env->GetFieldID(cls, "shadow_quality", "I");
                env->SetIntField(ret, f, settings["shadow_quality"]->v.i);

                f = env->GetFieldID(cls, "shadow_map_resx", "I");
                env->SetIntField(ret, f, settings["shadow_map_resx"]->v.i);

                f = env->GetFieldID(cls, "shadow_map_resy", "I");
                env->SetIntField(ret, f, settings["shadow_map_resy"]->v.i);

                f = env->GetFieldID(cls, "ao_map_res", "I");
                env->SetIntField(ret, f, settings["ao_map_res"]->v.i);

                f = env->GetFieldID(cls, "enable_ao", "Z");
                env->SetBooleanField(ret, f, settings["enable_ao"]->v.b);

                f = env->GetFieldID(cls, "uiscale", "F");
                env->SetFloatField(ret, f, settings["uiscale"]->v.f);

                f = env->GetFieldID(cls, "cam_speed", "F");
                env->SetFloatField(ret, f, settings["cam_speed_modifier"]->v.f);

                f = env->GetFieldID(cls, "zoom_speed", "F");
                env->SetFloatField(ret, f, settings["zoom_speed"]->v.f);

                f = env->GetFieldID(cls, "smooth_cam", "Z");
                env->SetBooleanField(ret, f, settings["smooth_cam"]->v.b);

                f = env->GetFieldID(cls, "smooth_zoom", "Z");
                env->SetBooleanField(ret, f, settings["smooth_zoom"]->v.b);

                f = env->GetFieldID(cls, "border_scroll_enabled", "Z");
                env->SetBooleanField(ret, f, settings["border_scroll_enabled"]->v.b);

                f = env->GetFieldID(cls, "border_scroll_speed", "F");
                env->SetFloatField(ret, f, settings["border_scroll_speed"]->v.f);

                f = env->GetFieldID(cls, "display_object_ids", "Z");
                env->SetBooleanField(ret, f, settings["display_object_id"]->v.b);

                f = env->GetFieldID(cls, "display_grapher_value", "Z");
                env->SetBooleanField(ret, f, settings["display_grapher_value"]->v.b);

                f = env->GetFieldID(cls, "display_wireless_frequency", "Z");
                env->SetBooleanField(ret, f, settings["display_wireless_frequency"]->v.b);

                f = env->GetFieldID(cls, "hide_tips", "Z");
                env->SetBooleanField(ret, f, settings["hide_tips"]->v.b);

                f = env->GetFieldID(cls, "sandbox_back_dna", "Z");
                env->SetBooleanField(ret, f, settings["dna_sandbox_back"]->v.b);

                f = env->GetFieldID(cls, "display_fps", "I");
                env->SetIntField(ret, f, settings["display_fps"]->v.u8);

                f = env->GetFieldID(cls, "volume", "F");
                env->SetFloatField(ret, f, settings["volume"]->v.f);

                f = env->GetFieldID(cls, "muted", "Z");
                env->SetBooleanField(ret, f, settings["muted"]->v.b);
            }
        }
    }

    return ret;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setSetting(JNIEnv *env, jclass _jcls,
        jstring setting_name, jboolean value)
{
    const char *str = env->GetStringUTFChars(setting_name, 0);
    tms_infof("Setting setting %s to %s", str, value ? "TRUE" : "FALSE");
    settings[str]->v.b = (bool)value;

    env->ReleaseStringUTFChars(setting_name, str);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getSettingBool(JNIEnv *env, jclass _jcls,
        jstring setting_name)
{
    const char *str = env->GetStringUTFChars(setting_name, 0);
    jboolean ret = (jboolean)settings[str]->v.b;
    env->ReleaseStringUTFChars(setting_name, str);

    return ret;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_login(JNIEnv *env, jclass _jcls,
        jstring username, jstring password)
{
    const char *tmp_username = env->GetStringUTFChars(username, 0);
    const char *tmp_password = env->GetStringUTFChars(password, 0);
    struct login_data *data = (struct login_data*)malloc(sizeof(struct login_data));

    strcpy(data->username, tmp_username);
    strcpy(data->password, tmp_password);

    env->ReleaseStringUTFChars(username, tmp_username);
    env->ReleaseStringUTFChars(password, tmp_password);

    P.add_action(ACTION_LOGIN, (void*)data);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_register(JNIEnv *env, jclass _jcls,
        jstring username, jstring email, jstring password)
{
    const char *tmp_username = env->GetStringUTFChars(username, 0);
    const char *tmp_email = env->GetStringUTFChars(email, 0);
    const char *tmp_password = env->GetStringUTFChars(password, 0);
    struct register_data *data = (struct register_data*)malloc(sizeof(struct register_data));

    strcpy(data->username, tmp_username);
    strcpy(data->email,    tmp_email);
    strcpy(data->password, tmp_password);

    env->ReleaseStringUTFChars(username, tmp_username);
    env->ReleaseStringUTFChars(email, tmp_email);
    env->ReleaseStringUTFChars(password, tmp_password);

    P.add_action(ACTION_REGISTER, (void*)data);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_focusGL(JNIEnv *env, jclass _jcls,
        jboolean focus)
{
    P.focused = (int)(bool)focus;
    if (P.focused) {
        sm::resume_all();
    } else {
        sm::pause_all();
    }
    tms_infof("received focus event: %d", (int)(bool)focus);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_isPaused(JNIEnv *env, jclass _cls)
{
    return (jboolean)(_tms.is_paused == 1 ? true : false);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setPaused(JNIEnv *env, jclass _cls,
        jboolean b)
{
    _tms.is_paused = (b ? 1 : 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setSettings(JNIEnv *env, jclass _jcls,
        jboolean enable_shadows,
        jboolean enable_ao, jint shadow_quality,
        jint shadow_map_resx, jint shadow_map_resy, jint ao_map_res,
        jfloat uiscale,
        jfloat cam_speed, jfloat zoom_speed,
        jboolean smooth_cam, jboolean smooth_zoom,
        jboolean border_scroll_enabled, jfloat border_scroll_speed,
        jboolean display_object_ids,
        jboolean display_grapher_value,
        jboolean display_wireless_frequency,
        jfloat volume,
        jboolean muted,
        jboolean hide_tips,
        jboolean sandbox_back_dna,
        jint display_fps
        )
{
    bool do_reload_graphics = false;
    if (settings["enable_shadows"]->v.b != (bool)enable_shadows) {
        do_reload_graphics = true;
    } else if (settings["enable_ao"]->v.b != (bool)enable_ao) {
        do_reload_graphics = true;
    } else if (settings["shadow_quality"]->v.u8 != (int)shadow_quality) {
        do_reload_graphics = true;
    } else if (settings["shadow_map_resx"]->v.i != (int)shadow_map_resx) {
        do_reload_graphics = true;
    } else if (settings["shadow_map_resy"]->v.i != (int)shadow_map_resy)  {
        do_reload_graphics = true;
    } else if (settings["ao_map_res"]->v.i != (int)ao_map_res) {
        do_reload_graphics = true;
    }

    if (do_reload_graphics) {
        P.can_reload_graphics = false;
        P.can_set_settings = false;
        P.add_action(ACTION_RELOAD_GRAPHICS, 0);

        /* XXX: causes infinite loops on certain devices e.g. nexus 7 (WTF?)
        while (!P.can_set_settings) {
            tms_debugf("waiting for can set settings");
            SDL_Delay(5);
        }*/
    }

    settings["enable_shadows"]->v.b = (bool)enable_shadows;
    settings["enable_ao"]->v.b = (bool)enable_ao;
    settings["shadow_quality"]->v.u8 = (int)shadow_quality;
    settings["shadow_map_resx"]->v.i = (int)shadow_map_resx;
    settings["shadow_map_resy"]->v.i = (int)shadow_map_resy;
    settings["ao_map_res"]->v.i = (int)ao_map_res;

    if (settings["uiscale"]->set((float)uiscale)) {
        ui::message("You need to restart Principia before the UI scale change takes effect.");
    }
    settings["cam_speed_modifier"]->v.f = (float)cam_speed;
    settings["zoom_speed"]->v.f = (float)zoom_speed;
    settings["smooth_cam"]->v.b = (bool)smooth_cam;
    settings["smooth_zoom"]->v.b = (bool)smooth_zoom;
    settings["border_scroll_enabled"]->v.b = (bool)border_scroll_enabled;
    settings["border_scroll_speed"]->v.f = (float)border_scroll_speed;
    settings["display_object_id"]->v.b = (bool)display_object_ids;
    settings["display_grapher_value"]->v.b = (bool)display_grapher_value;
    settings["hide_tips"]->v.b = (bool)hide_tips;
    settings["display_fps"]->v.u8 = (uint8_t)display_fps;

    settings["muted"]->v.b = (bool)muted;
    settings["volume"]->v.f = (float)volume;

    if (do_reload_graphics) {
        P.can_reload_graphics = true;
    }

    if ((bool)enable_shadows) {
        tms_debugf("Shadows ENABLED. Resolution: %dx%d. Quality: %d",
                (int)shadow_map_resx, (int)shadow_map_resy, (int)shadow_quality);
    }
    if ((bool)enable_ao) {
        tms_debugf("AO ENABLED. Resolution: %dx%d",
                (int)ao_map_res, (int)ao_map_res);
    }

    tms_debugf("UI Scale: %.2f. Cam speed: %.2f. Zoom speed: %.2f",
            (float)uiscale, (float)cam_speed, (float)zoom_speed);

    settings.save();

    sm::load_settings();
}


extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setConsumableType(JNIEnv *env, jclass _jcls, jint t)
{

}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getCurrentCommunityUrl(JNIEnv *env, jclass _jcls)
{
    COMMUNITY_URL("level/%d", W->level.community_id);

    return env->NewStringUTF(url);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setGameMode(JNIEnv *env, jclass _jcls, jint mode)
{
    G->set_mode(mode);
}


extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelIdType(
        JNIEnv *env, jclass _jcls)
{
    return W->level_id_type;
}





/** ++Color Chooser **/
extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getEntityColor(JNIEnv *env, jclass _jcls)
{
    int color = 0;

    if (G->selection.e) {
        entity *e = G->selection.e;

        tvec4 c = e->get_color();
        color = ((int)(c.a * 255.f) << 24)
            + ((int)(c.r * 255.f) << 16)
            + ((int)(c.g * 255.f) << 8)
            +  (int)(c.b * 255.f);
    }

    return (jint)color;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setEntityColor(
        JNIEnv *env, jclass _jcls,
        jint color)
{
    int alpha;
    float r,g,b;
    alpha = ((color & 0xFF000000) >> 24);
    if (alpha < 0) alpha = 0;

    r = (float)((color & 0x00FF0000) >> 16) / 255.f;
    g = (float)((color & 0x0000FF00) >> 8 ) / 255.f;
    b = (float)((color & 0x000000FF)      ) / 255.f;

    if (G->selection.e) {
        entity *e = G->selection.e;

        e->set_color4(r, g, b);

        if (e->g_id == O_PIXEL) {
            uint8_t frequency = (uint8_t)alpha;
            e->set_property(4, frequency);
        }

        P.add_action(ACTION_HIGHLIGHT_SELECTED, 0);
        P.add_action(ACTION_RESELECT, 0);
    }
}

extern "C" jfloat
Java_org_libsdl_app_PrincipiaBackend_getEntityAlpha(
        JNIEnv *env, jclass _jcls,
        jfloat alpha)
{
    jfloat a = 1.f;

    if (G->selection.e && G->selection.e->g_id == O_PIXEL) {
        a = (jfloat)(G->selection.e->properties[4].v.i8 / 255);
    }

    return a;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setEntityAlpha(
        JNIEnv *env, jclass _jcls,
        jfloat alpha)
{
    if (G->selection.e && G->selection.e->g_id == O_PIXEL) {
        G->selection.e->properties[4].v.i8 = (uint8_t)(alpha * 255);
    }
}

/** ++Export **/
extern "C" void
Java_org_libsdl_app_PrincipiaBackend_saveObject(
        JNIEnv *env, jclass _jcls,
        jstring name)
{
    const char *tmp = env->GetStringUTFChars(name, 0);
    char *_name = strdup(tmp);

    P.add_action(ACTION_EXPORT_OBJECT, _name);
    ui::message("Saved object!");

    env->ReleaseStringUTFChars(name, tmp);
}

/** ++++++++++++++++++++++++++ **/

static char *_tmp_args[2];
static char _tmp_arg1[256];

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setarg(JNIEnv *env, jclass _jcls,
        jstring arg)
{
    _tmp_args[0] = 0;
    _tmp_args[1] = _tmp_arg1;

    const char *tmp = env->GetStringUTFChars(arg, 0);
    int len = env->GetStringUTFLength(arg);

    if (len > 255)
        len = 255;

    memcpy(&_tmp_arg1, tmp, len);
    _tmp_arg1[len] = '\0';

    tproject_set_args(2, _tmp_args);

    env->ReleaseStringUTFChars(arg, tmp);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelName(JNIEnv *env, jclass _jcls,
        jstring name)
{
    const char *tmp = env->GetStringUTFChars(name, 0);
    int len = env->GetStringUTFLength(name);

    if (len > LEVEL_NAME_MAX_LEN) {
        len = LEVEL_NAME_MAX_LEN;
    }

    memcpy(W->level.name, tmp, len);
    W->level.name_len = len;

    env->ReleaseStringUTFChars(name, tmp);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelName(JNIEnv *env, jclass _jcls)
{
    jstring str;
    char tmp[257];

    char *nm = W->level.name;
    memcpy(tmp, nm, W->level.name_len);

    tmp[W->level.name_len] = '\0';

    str = env->NewStringUTF(tmp);
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevels(JNIEnv *env, jclass _jcls, jint level_type)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    lvlfile *level = pkgman::get_levels((int)level_type);

    while (level) {
        b << level->id << ',';
        b << level->save_id << ',';
        b << level->id_type << ',';
        b << level->name;
        b << '\n';
        lvlfile *next = level->next;
        delete level;
        level = next;
    }

    tms_infof("getLevels: %s", b.str().c_str());

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getSelectionGid(JNIEnv *env, jclass _jcls)
{
    if (G->selection.e) {
        return (jint)G->selection.e->g_id;
    }

    return 0;
}


extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_isAdventure(JNIEnv *env, jclass _jcls)
{
    return (jboolean)false;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelType(JNIEnv *env, jclass _jcls)
{
    tms_infof("Level type: %d", W->level.type);
    return (jint)W->level.type;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelType(JNIEnv *env, jclass _jcls,
        jint type)
{
    if (type >= LCAT_PUZZLE && type <= LCAT_CUSTOM) {
        P.add_action(ACTION_SET_LEVEL_TYPE, (void*)type);
    }
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getAvailableBgs(JNIEnv *env, jclass _jcls)
{
    std::stringstream b("", std::ios_base::app | std::ios_base::out);

    for (int x=0; x<num_bgs; ++x) {
        b << available_bgs[x];
        b << '\n';
    }

    jstring str;
    str = env->NewStringUTF(b.str().c_str());
    return str;
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelDescription(JNIEnv *env, jclass _jcls)
{
    char *descr = W->level.descr;
    if (descr == 0 || W->level.descr_len == 0) {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(descr);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelDescription(JNIEnv *env, jclass _jcls,
        jstring descr)
{
    lvlinfo *l = &W->level;
    const char *tmp = env->GetStringUTFChars(descr, 0);
    int len = env->GetStringUTFLength(descr);

    if (len > LEVEL_DESCR_MAX_LEN)
        len = LEVEL_DESCR_MAX_LEN;

    if (len == 0) {
        l->descr = 0;
    } else {
        l->descr = (char*)realloc(l->descr, len+1);

        memcpy(l->descr, tmp, len);
        l->descr[len] = '\0';
    }

    l->descr_len = len;

    env->ReleaseStringUTFChars(descr, tmp);

    tms_debugf("New description: '%s'", l->descr);
}

extern "C" jstring
Java_org_libsdl_app_PrincipiaBackend_getLevelInfo(JNIEnv *env, jclass _jcls)
{
    char info[2048];

    lvlinfo *l = &W->level;

    /**
     * int bg,
     * int left_border, int right_border, int bottom_border, int top_border,
     * float gravity_x, float gravity_y,
     * int position_iterations, int velocity_iterations,
     * int final_score,
     * boolean pause_on_win, boolean display_score,
     * float prismatic_tolerance, float pivot_tolerance,
     * int color,
     * float linear_damping,
     * float angular_damping,
     * float joint_friction,
     * float creature_absorb_time
     **/

    sprintf(info,
            "%u,"
            "%u,"
            "%u,"
            "%u,"
            "%u,"
            "%.1f,"
            "%.1f,"
            "%u,"
            "%u,"
            "%u,"
            "%s,"
            "%s,"
            "%f,"
            "%f,"
            "%d,"
            "%f,"
            "%f,"
            "%f,"
            "%f,"
            "%f,"
            ,
                l->bg,
                l->size_x[0], l->size_x[1], l->size_y[0], l->size_y[1],
                l->gravity_x, l->gravity_y,
                l->position_iterations, l->velocity_iterations,
                l->final_score,
                l->pause_on_finish ? "true" : "false",
                l->show_score ? "true" : "false",
                l->prismatic_tolerance, l->pivot_tolerance,
                l->bg_color,
                l->linear_damping,
                l->angular_damping,
                l->joint_friction,
                l->dead_enemy_absorb_time,
                l->time_before_player_can_respawn
            );

    jstring str;
    str = env->NewStringUTF(info);
    return str;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getLevelVersion(JNIEnv *env, jclass _jcls)
{
    lvlinfo *l = &W->level;

    return (jint)l->version;
}

extern "C" jint
Java_org_libsdl_app_PrincipiaBackend_getMaxLevelVersion(JNIEnv *env, jclass _jcls)
{
    return (jint)LEVEL_VERSION;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelLocked(
        JNIEnv *env, jclass _jcls,
        jboolean locked)
{
    lvlinfo *l = &W->level;

    l->visibility = ((bool)locked ? LEVEL_LOCKED : LEVEL_VISIBLE);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelInfo(
        JNIEnv *env, jclass _jcls,
        jint bg,
        jint border_left, jint border_right, jint border_bottom, jint border_top,
        jfloat gravity_x, jfloat gravity_y,
        jint position_iterations, jint velocity_iterations,
        jint final_score,
        jboolean pause_on_win, jboolean display_score,
        jfloat prismatic_tolerance, jfloat pivot_tolerance,
        jint bg_color,
        jfloat linear_damping,
        jfloat angular_damping,
        jfloat joint_friction,
        jfloat dead_enemy_absorb_time,
        jfloat time_before_player_can_respawn
        )
{
    lvlinfo *l = &W->level;

    /**
      * int bg,
      * int left_border, int right_border, int bottom_border, int top_border,
      * float gravity_x, float gravity_y,
      * int position_iterations, int velocity_iterations,
      * int final_score,
      * boolean pause_on_win,
      * boolean display_score
      **/
    l->bg = (uint8_t)bg;

    uint16_t left  = (uint16_t)border_left;
    uint16_t right = (uint16_t)border_right;
    uint16_t down  = (uint16_t)border_bottom;
    uint16_t up    = (uint16_t)border_top;

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

    if (resized) {
        ui::message("Your level size was increased to the minimum allowed.");
    }

    l->size_x[0] = left;
    l->size_x[1] = right;
    l->size_y[0] = down;
    l->size_y[1] = up;
    l->gravity_x = (float)gravity_x;
    l->gravity_y = (float)gravity_y;
    l->final_score = (uint32_t)final_score;
    l->position_iterations = (uint8_t)position_iterations;
    l->velocity_iterations = (uint8_t)velocity_iterations;
    l->pause_on_finish = (bool)pause_on_win;
    l->show_score = (bool)display_score;
    l->prismatic_tolerance = (float)prismatic_tolerance;
    l->pivot_tolerance = (float)pivot_tolerance;
    l->bg_color = (int)bg_color;
    l->linear_damping = (float)linear_damping;
    l->angular_damping = (float)angular_damping;
    l->joint_friction = (float)joint_friction;
    l->dead_enemy_absorb_time = (float)dead_enemy_absorb_time;
    l->time_before_player_can_respawn = (float)time_before_player_can_respawn;

    P.add_action(ACTION_RELOAD_LEVEL, 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_resetLevelFlags(
        JNIEnv *env, jclass _jcls,
        jlong flag)
{
    lvlinfo *l = &W->level;

    l->flags = 0;
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_setLevelFlag(
        JNIEnv *env, jclass _jcls,
        jlong _flag)
{
    lvlinfo *l = &W->level;

    uint32_t flag = (uint32_t)_flag;

    //tms_infof("x: %d", flag);
    uint64_t f = (uint64_t)(1ULL << flag);
    //tms_infof("f: %llu", f);

    //tms_infof("Flags before: %llu", l->flags);
    l->flags |= f;
    //tms_infof("Flags after: %llu", l->flags);
}

extern "C" jboolean
Java_org_libsdl_app_PrincipiaBackend_getLevelFlag(
        JNIEnv *env, jclass _jcls,
        jlong _flag)
{
    lvlinfo *l = &W->level;

    uint32_t flag = (uint32_t)_flag;
    uint64_t f = (1ULL << flag);

    return (jboolean)(l->flag_active(f));
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_triggerSave(
        JNIEnv *env, jclass _jcls, jboolean save_copy)
{
    if (save_copy)
        P.add_action(ACTION_SAVE_COPY, 0);
    else
        P.add_action(ACTION_SAVE, 0);
}

extern "C" void
Java_org_libsdl_app_PrincipiaBackend_triggerCreateLevel(
        JNIEnv *env, jclass _jcls, jint level_type)
{
    P.add_action(ACTION_NEW_LEVEL, level_type);
}

#endif
