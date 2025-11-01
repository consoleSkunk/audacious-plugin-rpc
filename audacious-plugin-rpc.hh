#include <iostream>
#include <string.h>
#include <cstdint>

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
static const char *SETTING_HIDE_PAUSED = "hide_when_paused";
static const char *SETTING_HIDE_STATE = "hide_playback_state";

