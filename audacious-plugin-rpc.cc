#include "audacious-plugin-rpc.hh"

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
    static void init_discord();
    static void update_presence();
    static void init_presence();
    static void cleanup_discord();
    static void title_changed();
    static void update_title_presence(void*, void*);
    static const ComboItem status_display_types[];
    static const PreferencesWidget privacy_settings[];
};

EXPORT RPCPlugin aud_plugin_instance;

DiscordEventHandlers handlers;
DiscordRichPresence presence;
std::string title, titleText;
std::string artist, artistText;
std::string album, albumText;
std::string playingStatus;
std::int64_t length;
std::int64_t timestamp;

void RPCPlugin::init_discord() {
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
    AUDINFO("Cleaning up Discord presence\n");
    Discord_ClearPresence();
    Discord_Shutdown();
}

void RPCPlugin::title_changed() {
    bool hideCurrentSong = aud_get_bool(CFG_SECTION, SETTING_HIDE_STATUS);
    bool showProgressBar = aud_get_bool(CFG_SECTION, SETTING_SHOW_PROGRESS);
    bool hidePaused = aud_get_bool(CFG_SECTION, SETTING_HIDE_PAUSED);
    bool hidePlaybackState = aud_get_bool(CFG_SECTION, SETTING_HIDE_STATE);
    std::string activityType(aud_get_str(CFG_SECTION, SETTING_STATUS_TYPE));

    if (aud_drct_get_ready() && aud_drct_get_playing()) {
        bool paused = aud_drct_get_paused();
        bool shuffle = aud_get_bool("shuffle") || aud_get_bool("album_shuffle");
        bool no_advance = aud_get_bool("no_playlist_advance");
        
        if(paused && hidePaused) {
            Discord_ClearPresence();
            return;
        }

        Tuple tuple = aud_drct_get_tuple();
        title = tuple.get_str(Tuple::Title);
        
        titleText = title.append(" ").substr(0, 128);

        String artistString = tuple.get_str(Tuple::Artist);
        if(artistString) {
            artist = tuple.get_str(Tuple::Artist);
            artistText = artist.append(" ").substr(0, 128);
        } else {
            artistText = "";
        }
        
        String albumString = tuple.get_str(Tuple::Album);
        if(albumString) {
            album = tuple.get_str(Tuple::Album);
            albumText = album.append(" ").substr(0, 128);
        } else {
            albumText = "";
        }

        length = tuple.get_int(Tuple::Length);
        timestamp = (length / 1000) - (aud_drct_get_time() / 1000);

        if(!hidePlaybackState) {
            playingStatus = paused ? "Paused" : length == -1 ? "Listening" : no_advance ? "Looping" : shuffle ? "Shuffling" : "Listening";
            presence.smallImageKey = paused ? "pause" : length == -1 ? "play" : no_advance ? "repeat_song" : shuffle ? "shuffle" : hideCurrentSong ? "play" : "";
        } else {
            playingStatus = "";
            presence.smallImageKey = "";
        }

        if(hideCurrentSong) {
            presence.details = "";
            presence.state = playingStatus.c_str();
            presence.smallImageText = playingStatus.c_str();
            presence.largeImageText = "";
            presence.status_display_type = DiscordStatusDisplayType_Name;
        }
        else {
            presence.details = titleText.c_str();
            presence.state = artistText.c_str();
            presence.largeImageText = albumText.c_str();
            presence.smallImageText = playingStatus.c_str();

            if(artistString && activityType == "state")
                presence.status_display_type = DiscordStatusDisplayType_State;
            else if(activityType == "details")
                presence.status_display_type = DiscordStatusDisplayType_Details;
            else
                presence.status_display_type = DiscordStatusDisplayType_Name;
        }

        presence.startTimestamp = paused ? time(NULL) : (time(NULL) - aud_drct_get_time() / 1000);
        if(hideCurrentSong && !showProgressBar)
            presence.endTimestamp = 0;
        else
            presence.endTimestamp = (paused || length == -1) ? 0 : time(NULL) + timestamp;
        
        presence.largeImageKey = "";
    } else {
        if(hidePaused) {
            Discord_ClearPresence();
            return;
        }
        playingStatus = "Stopped";
        presence.details = "";
        presence.state = "Stopped";
        presence.status_display_type = DiscordStatusDisplayType_Name;
        presence.largeImageText = "";
        presence.largeImageKey = "logo";
        presence.smallImageKey = "stop";
        presence.startTimestamp = time(NULL);
        presence.endTimestamp = 0;
    }
    
    update_presence();
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
    SETTING_STATUS_TYPE, "state",
    SETTING_HIDE_STATUS, "FALSE",
    SETTING_SHOW_PROGRESS, "TRUE",
    SETTING_HIDE_PAUSED, "FALSE",
    SETTING_HIDE_STATE, "FALSE",
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
  WidgetBox ({{privacy_settings}}, WIDGET_CHILD),
  WidgetCheck(
      N_("Hide presence while paused"),
      WidgetBool(CFG_SECTION, SETTING_HIDE_PAUSED, title_changed)
  ),
  WidgetCheck(
      N_("Hide playback icon"),
      WidgetBool(CFG_SECTION, SETTING_HIDE_STATE, title_changed)
  ),
};

const PluginPreferences RPCPlugin::prefs = {{ widgets }};
