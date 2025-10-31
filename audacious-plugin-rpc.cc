#include <ctime>
#include <string.h>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/hook.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include <discord_rpc.h>

#define EXPORT __attribute__((visibility("default")))
#define CFG_SECTION "audacious-plugin-rpc"
#define APPLICATION_ID "1277415022707998730"
#define SOURCE_REPOSITORY "https://github.com/consoleSkunk/audacious-plugin-rpc"

static const char *SETTING_STATUS_TYPE = "status_display_type";
static const char *SETTING_HIDE_STATUS = "hide_current_song";
static const char *SETTING_SHOW_PROGRESS = "show_progress_bar";

class RPCPlugin : public GeneralPlugin {

public:
    static const char about[];
    static const char * const defaults[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = {
        N_("Discord RPC"),
        "audacious-plugin-rpc",
        about,
        &prefs
    };

    constexpr RPCPlugin() : GeneralPlugin (info, false) {}

    bool init();
    void cleanup();

private:
    struct Metadata;
    static void init_discord();
    static void update_presence();
    static void init_presence();
    static void cleanup_discord();
    static void title_changed();
    static void update_title_presence(void*, void*);
    static const ComboItem status_display_types[];
    static const PreferencesWidget privacy_settings[];
};

struct RPCPlugin::Metadata {
    String title, artist, album, playingStatus;
    int64_t length, timestamp;
};

EXPORT RPCPlugin aud_plugin_instance;

DiscordRichPresence presence;

void RPCPlugin::init_discord() {
    DiscordEventHandlers handlers;

    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(APPLICATION_ID, &handlers, 0, NULL);
}

void RPCPlugin::update_presence() {
    AUDINFO("Updating Discord presence\n");
    Discord_UpdatePresence(&presence);
}

void RPCPlugin::init_presence() {
    AUDINFO("Initializing Discord presence\n");
    memset(&presence, 0, sizeof(presence));
        presence.type = DiscordActivityType_Listening;
    presence.startTimestamp = time(NULL);
    update_presence();
}

void RPCPlugin::cleanup_discord() {
    AUDINFO("Shutting down Discord presence\n");
    Discord_ClearPresence();
    Discord_Shutdown();
}

void RPCPlugin::title_changed() {
    Metadata meta;
    if (aud_drct_get_ready() && aud_drct_get_playing()) {
        Tuple tuple = aud_drct_get_tuple();

        meta.title = tuple.get_str(Tuple::Title);
        meta.artist = tuple.get_str(Tuple::Artist);
        meta.album = tuple.get_str(Tuple::Album);
        meta.length = tuple.get_int(Tuple::Length);
        meta.timestamp = (meta.length / 1000) - (aud_drct_get_time() / 1000);

        bool hideCurrentSong = aud_get_bool(CFG_SECTION, SETTING_HIDE_STATUS);
        String activityType = aud_get_str(CFG_SECTION, SETTING_STATUS_TYPE);
        bool showProgressBar = aud_get_bool(CFG_SECTION, SETTING_SHOW_PROGRESS);

        bool paused = aud_drct_get_paused();
        bool shuffle = aud_get_bool("shuffle") || aud_get_bool("album_shuffle");
        bool no_advance = aud_get_bool("no_playlist_advance");
        meta.playingStatus = String(paused ? "Paused" : meta.length == -1 ? "Listening" : no_advance ? "Looping" : shuffle ? "Shuffling" : "Listening");

        if(hideCurrentSong) {
            presence.details = "";
            presence.state = strdup(meta.playingStatus);
            presence.largeImageText = "";
            presence.status_display_type = DiscordStatusDisplayType_Name;
        } else {
            presence.details = strdup(meta.title);
            presence.state = strdup(meta.artist);
            presence.largeImageText = strdup(meta.album);
            presence.smallImageText = strdup(meta.playingStatus);

            if(meta.artist && activityType == String("state"))
                presence.status_display_type = DiscordStatusDisplayType_State;
            else if(activityType == String("details"))
                presence.status_display_type = DiscordStatusDisplayType_Details;
            else
                presence.status_display_type = DiscordStatusDisplayType_Name;
        }

        
        presence.smallImageKey = paused ? "pause" : meta.length == -1 ? "play" : no_advance ? "repeat_song" : shuffle ? "shuffle" : "play";

        presence.startTimestamp = paused ? time(NULL) : (time(NULL) - aud_drct_get_time() / 1000);
        if(hideCurrentSong && !showProgressBar)
            presence.endTimestamp = 0;
        else
            presence.endTimestamp = (paused || meta.length == -1) ? 0 : time(NULL) + meta.timestamp;
        
        presence.largeImageKey = "logo";
        update_presence();
    } else {
        // default presence details
        presence.details = "";
        presence.state = "Stopped";
        presence.status_display_type = DiscordStatusDisplayType_Name;
        presence.largeImageText = "";
        presence.largeImageKey = "logo";
        presence.smallImageKey = "stop";
        presence.smallImageText = "Stopped";
        presence.startTimestamp = time(NULL);
        presence.endTimestamp = 0;
        update_presence();
    }
}

void RPCPlugin::update_title_presence(void*, void*) {
    title_changed();
}

bool RPCPlugin::init() {
    aud_config_set_defaults(CFG_SECTION, defaults);
    init_discord();
    init_presence();
    hook_associate("playback ready", update_title_presence, nullptr);
    hook_associate("playback stop", update_title_presence, nullptr);
    hook_associate("playback pause", update_title_presence, nullptr);
    hook_associate("playback unpause", update_title_presence, nullptr);
    hook_associate("playback seek", update_title_presence, nullptr);
    hook_associate("playlist end reached", update_title_presence, nullptr);
    hook_associate ("set shuffle", update_title_presence, nullptr);
    hook_associate ("set no_playlist_advance", update_title_presence, nullptr);
    hook_associate("title change", update_title_presence, nullptr);
    return true;
}

void RPCPlugin::cleanup() {
    hook_dissociate("playback ready", update_title_presence);
    hook_dissociate("playback stop", update_title_presence);
    hook_dissociate("playback pause", update_title_presence);
    hook_dissociate("playback unpause", update_title_presence);
    hook_dissociate("playback seek", update_title_presence);
    hook_dissociate("playlist end reached", update_title_presence);
    hook_dissociate("set shuffle", update_title_presence);
    hook_dissociate("set no_playlist_advance", update_title_presence);
    hook_dissociate("title change", update_title_presence);
    cleanup_discord();
}

const char RPCPlugin::about[] = N_(
    "Discord RPC music status plugin\n\n"
    "Copyright (c) 2025 Derzsi Dániel <daniel@tohka.us>\n"
    "Copyright (c) 2025 consoleSkunk\n\n"
    SOURCE_REPOSITORY
);

const char * const RPCPlugin::defaults[] = {
    "extra_text", "",
    "status_display_type", "state",
    "hide_current_song", "FALSE",
    "show_progress_bar", "TRUE",
    nullptr
};

const ComboItem RPCPlugin::status_display_types[] = {
    ComboItem (N_("Audacious"), "name"),
    ComboItem ("Track title", "details"),
    ComboItem ("Artist name", "state")
};

const PreferencesWidget RPCPlugin::privacy_settings[] = {
  WidgetCheck(
      N_("Display progress bar"),
      WidgetBool(CFG_SECTION, SETTING_SHOW_PROGRESS, title_changed)
  ),
};

const PreferencesWidget RPCPlugin::widgets[] =
{
  WidgetLabel (N_("<b>General</b>")),
  WidgetCombo(
      N_("Status display text:"),
      WidgetString (CFG_SECTION, SETTING_STATUS_TYPE, title_changed),
      {{status_display_types}}
  ),
  WidgetLabel (N_("<b>Privacy</b>")),
  WidgetCheck(
      N_("Hide current song"),
      WidgetBool(CFG_SECTION, SETTING_HIDE_STATUS, title_changed)
  ),
  WidgetBox ({{privacy_settings}}, WIDGET_CHILD)
};

const PluginPreferences RPCPlugin::prefs = {{ widgets }};
