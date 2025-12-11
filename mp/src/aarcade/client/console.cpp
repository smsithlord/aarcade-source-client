#include "cbase.h"

//#include "c_simple_image_entity.h"
//#include "c_webViewInput.h"
//#include "aa_globals.h"
#include "c_anarchymanager.h"

#include "../../../game/client/cdll_client_int.h"
#include "cliententitylist.h"

#include "c_openglmanager.h"
#include "filesystem.h"
#include "c_aitests.h"
#include <algorithm>

#include "view.h"
//#include "../../public/view_shared.h"

#include "../client/hlvr/proxydll.h"
#include "client_virtualreality.h"//"iclientvirtualreality.h"
#include "sourcevr/isourcevirtualreality.h"

//#include "KeyValues.h"	// Added for Anarchy Arcade

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Required DLL entry point
/*BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	KeyValues::SetUseGrowableStringTable(true);	// Added for Anarchy Arcade
	return TRUE;
}*/
/*BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		KeyValues::SetUseGrowableStringTable(true);
	}
	return TRUE;
}*/

//ConVar xbmc_enable( "xbmc_enable", "0", FCVAR_ARCHIVE );
//ConVar default_width( "default_width", "256", FCVAR_ARCHIVE);	// obsolete
//ConVar default_height( "default_height", "256", FCVAR_ARCHIVE);	// obsolete

ConVar debug_invert_vr_matrices("debug_invert_vr_matrices", "0", FCVAR_NONE, "Internal testing.");

ConVar modelthumbsize("model_thumb_size", "256", FCVAR_ARCHIVE, "Size, in pixels, to create model thumbnail images.  512x512 is about 1MB each.");
ConVar localvideobehavior("local_video_behavior", "0", FCVAR_ARCHIVE, "Set to 1 for local videos to auto-resume.");

ConVar usesbs("usesbs", "0", FCVAR_HIDDEN, "Internal testing.");
ConVar ipd("ipd", "63", FCVAR_ARCHIVE, "IPD for stereo rendering.");
ConVar lastfov("lastfov", "90", FCVAR_ARCHIVE, "Internal.  The most recent FOV used during a view render.");

ConVar sequence_blacklist("sequence_blacklist", "", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Space-separated list of animation sequence names you want to blacklist.");

ConVar pointWithinNode("point_within_node", "-1", FCVAR_REPLICATED, "Set to -1 if not in a node space.  Otherwise it is set to the node info entity.");

ConVar vrspectator("vrspectator", "0", FCVAR_ARCHIVE);
ConVar vrspectatormirror("vrspectatormirror", "1", FCVAR_ARCHIVE);
ConVar vrhmdrender("vrhmdrender", "1", FCVAR_NONE);

ConVar testerjointval("testerjointval", "0", FCVAR_NONE);

ConVar spectator("spectator", "0", FCVAR_NONE);
ConVar fixed_camera_min_dist("fixed_camera_min_dist", "32.0", FCVAR_NONE, "Minimum distance the player can be standing next to the fixed camera position before it's considered an invalid position.");
ConVar fixed_camera_max_dist("fixed_camera_max_dist", "500.0", FCVAR_NONE, "Maximum distance the player can be from a screenshot for it to be considered as a fixed camera position.");

ConVar screenshot_multiverse("screenshot_multiverse", "0", FCVAR_ARCHIVE);
ConVar quests_enabled("quests_enabled", "1", FCVAR_ARCHIVE);

ConVar nodraw_shortcuts("nodraw_shortcuts", "0", FCVAR_NONE, "When enabled, shortcuts will not be rendered.  Useful for taking screenshots.");
ConVar fixed_camera_spectate_mode("fixed_camera_spectate_mode", "0", FCVAR_NONE, "Set to 1 to use screenshots in the map as 3rd person fixed camera positions.");

ConVar allowmultipleactive("allow_multiple_active", "0", FCVAR_ARCHIVE, "When enabled, multiple objects will be allowed to be considered ACTIVE at the same time - playing their animation & dynanmic effects.");

ConVar video_chroma_light("video_chroma_light", "0 255 0", FCVAR_NONE, "RGB of the lightest green to key out. (Used with object_video_filter.)");
ConVar video_chroma_dark("video_chroma_dark", "0 250 0", FCVAR_NONE, "RGB of the darkest green to key out. (Used with object_video_filter.)");
ConVar video_chroma_a1("video_chroma_a1", "1.0", FCVAR_NONE, "A1 float value. Range 0.5 to 1.5.");
ConVar video_chroma_a2("video_chroma_a2", "0.7", FCVAR_NONE, "A2 float value. Range 0.5 to 1.5.");

ConVar autobuildsoundcache("autobuildsoundcache", "1", FCVAR_ARCHIVE, "Disable this to decrease startup times.  The only side-effect is that maps with custom sounds might not play their audio.");
ConVar alwaysrefreshsnapshots("always_refresh_snapshots", "0", FCVAR_ARCHIVE, "When you deselect an item that has no explicit screen image, a snapshot of the last rendered frame is saved for the screen. This only happens once, unless you set this convar to 1 - then it'll always happen.");

ConVar always_animating_images("always_animating_images", "1", FCVAR_ARCHIVE, "Enable this to allow images marked as Always Anaimated to work.");
ConVar always_animate_mp4s("always_animate_mp4s", "1", FCVAR_ARCHIVE, "Enable this to allow MP4s to be animated along with GIFs as Always Animated Images.");

// Action Bar Slots
ConVar abslot0("abslot0", "", FCVAR_ARCHIVE);
ConVar abslot1("abslot1", "", FCVAR_ARCHIVE);
ConVar abslot2("abslot2", "", FCVAR_ARCHIVE);
ConVar abslot3("abslot3", "", FCVAR_ARCHIVE);
ConVar abslot4("abslot4", "", FCVAR_ARCHIVE);
ConVar abslot5("abslot5", "", FCVAR_ARCHIVE);
ConVar abslot6("abslot6", "", FCVAR_ARCHIVE);
ConVar abslot7("abslot7", "", FCVAR_ARCHIVE);
ConVar abslot8("abslot8", "", FCVAR_ARCHIVE);
ConVar abslot9("abslot9", "", FCVAR_ARCHIVE);

// Camera Cut Slots
ConVar camslot0("camslot0", "", FCVAR_ARCHIVE);
ConVar camslot1("camslot1", "", FCVAR_ARCHIVE);
ConVar camslot2("camslot2", "", FCVAR_ARCHIVE);
ConVar camslot3("camslot3", "", FCVAR_ARCHIVE);
ConVar camslot4("camslot4", "", FCVAR_ARCHIVE);
ConVar camslot5("camslot5", "", FCVAR_ARCHIVE);
ConVar camslot6("camslot6", "", FCVAR_ARCHIVE);
ConVar camslot7("camslot7", "", FCVAR_ARCHIVE);
ConVar camslot8("camslot8", "", FCVAR_ARCHIVE);
ConVar camslot9("camslot9", "", FCVAR_ARCHIVE);
ConVar camcuttype("camcuttype", "0", FCVAR_ARCHIVE);

ConVar jumpPower("jump_power", "1.0", FCVAR_NONE, "Multiplayer to apply to jump power for mega jump.");

ConVar atlas_width("atlas_width", "1280", FCVAR_NONE, "The resolution to use on the always animating texture atlas. Must re-fresh the atlas web tab to take effect.");
ConVar atlas_height("atlas_height", "720", FCVAR_NONE, "The resolution to use on the always animating texture atlas. Must re-fresh the atlas web tab to take effect.");

ConVar view_overlay("view_overlay", "", FCVAR_NONE, "Internal.  Keeps track of what material is being used on the screen overlay.");

ConVar useglobalrotation("use_global_rotation", "0", FCVAR_ARCHIVE, "When set to 1, the orientation of objects you are placing will take the transform menu's values literally.");

ConVar prioritize_legacy_workshop_images("prioritize_legacy_workshop_images", "0", FCVAR_ARCHIVE);

ConVar debug_object_spawn("debug_object_spawn", "0", FCVAR_NONE, "Set to 1 to have the ID of each object logged to the console prior to spawning them. Then, if you crash at a certain object ID, you can set it as your skip_objects ConVar value to skip it next time.");
ConVar skip_objects("skip_objects", "", FCVAR_ARCHIVE, "Comma-separated list of object IDs that you want to skip when loading maps.  Debug purposes only.");

ConVar node_model("node_model", "", FCVAR_ARCHIVE, "Override the default model used for nodes origin points.  Leave blank for default.");

ConVar painted_skyname("painted_skyname", "", FCVAR_HIDDEN, "Internal.  Used to remember the name of a painted sky.");

ConVar inspect_model_id("inspect_model_id", "", FCVAR_HIDDEN, "Internal.  Used for the UI renderer to access the model ID when appropriate.");

ConVar temppinnedcamindex("temppinnedcamindex", "-1", FCVAR_HIDDEN, "Internal. Used to keep track of when the 3rd person camera pin command is active.");

ConVar default_engine_no_focus_sleep("default_engine_no_focus_sleep", "0", FCVAR_ARCHIVE);
ConVar auto_load_map("auto_load_map", "1", FCVAR_ARCHIVE);
ConVar last_map_loaded("last_map_loaded", "0", FCVAR_ARCHIVE);
ConVar shouldshowwindowstaskbar("should_show_windows_task_bar", "1", FCVAR_ARCHIVE);
ConVar freemousemode("freemousemode", "1", FCVAR_NONE);
ConVar tempfreelook("tempfreelook", "0", FCVAR_NONE);
ConVar glowenabled("glow_enabled", "1", FCVAR_NONE);
ConVar firstserver("firstserver", "", FCVAR_HIDDEN);
ConVar broadcast_mode("broadcast_mode", "0", FCVAR_NONE);	// ALWAYS start off.
ConVar broadcast_game("broadcast_game", "Anarchy Arcade", FCVAR_NONE);	// ALWAYS start on Anarchy Arcade.
ConVar broadcast_auto_game("broadcast_auto_game", "1", FCVAR_ARCHIVE);
ConVar broadcast_folder("broadcast_folder", "Z:\\scripts", FCVAR_ARCHIVE);	// but remember where to write to if the user turns it on
ConVar kodi("kodi", "0", FCVAR_ARCHIVE, "Set to 1 to use Kodi playback of video files using the settings in kodi_info variable.");
ConVar kodi_ip("kodi_ip", "192.168.0.100", FCVAR_ARCHIVE, "The ip of the Kodi host.");
ConVar kodi_port("kodi_port", "8080", FCVAR_ARCHIVE, "The port of the Kodi host.");
ConVar kodi_user("kodi_user", "", FCVAR_ARCHIVE, "The user of the Kodi host.");
ConVar kodi_password("kodi_password", "", FCVAR_ARCHIVE, "The password of the Kodi host. (NOT HIDDEN, SENT OVER HTTP GET REQUESTS AS PART OF THE URL TO TALK TO KODI!)");
ConVar libretro_sound("libretro_sound", "1", FCVAR_NONE, "Set to 0 to completely disable AA's sound system for Libretro.");
ConVar libretro_volume("libretro_volume", "1.0", FCVAR_ARCHIVE, "Libretro's volume level, a float between 0 and 1.");
ConVar old_libretro_volume("old_libretro_volume", "1.0", FCVAR_HIDDEN, "Internal.  Used to remember what value to set Libretro to when un-muted.");
ConVar old_volume("old_volume", "1.0", FCVAR_HIDDEN, "Internal.  Used to remember what value to set volume to when un-muted.");
ConVar use_deferred_texture_cleanup("use_deferred_texture_cleanup", "1", FCVAR_ARCHIVE, "Turn deferred texture cleanup on only if you crash often when it's off.");
ConVar throttle_embedded_render("throttle_embedded_render", "0", FCVAR_NONE, "Throttle embedded instance rendering to 1-per-game-frame-render, or disable throttling completely.");
ConVar libretro_gui_gamepad("libretro_gui_gamepad", "0", FCVAR_ARCHIVE, "The starting state of the Libretro GUI on-screen gamepad.");
ConVar auto_libretro("auto_libretro", "0", FCVAR_ARCHIVE, "Automatically run compatible shortcuts on the in-game screens with Libretro when selecting objects.");
ConVar wait_for_libretro("wait_for_libretro", "1", FCVAR_ARCHIVE, "Allow AArcade to hang while it waits for Libretro instances to fully close.");
ConVar cl_hovertitles("cl_hovertitles", "1", FCVAR_ARCHIVE, "Show the titles of items under your crosshair.");
ConVar cl_toastmsgs("cl_toastmsgs", "0", FCVAR_ARCHIVE, "Show event notifications on the top-left of the screen.");
ConVar workshop("workshop", "1", FCVAR_NONE, "Internal. Read-only. Set with launcher.");
ConVar mounts("mounts", "1", FCVAR_NONE, "Internal. Read-only. Set with launcher.");
ConVar inspect_object_showui("inspect_object_showui", "0", FCVAR_ARCHIVE, "Bool. Determines if the UI is shown during inspect mode or not.");
ConVar reshade("reshade", "0", FCVAR_NONE, "Internal. Read-only. Set with launcher.");
ConVar manualSpawnMode("manual_spawn_mode", "0", FCVAR_ARCHIVE, "Bool. Set to 1, objects will not automatically spawn in until you press the Spawn Nearby Objects button.");
ConVar autoUnspawnMode("auto_unspawn_mode", "0", FCVAR_ARCHIVE, "Bool. Set to 1 to automatically unspawn entities (to potentially make maps larger than the Source engine limit of 2048.)");
ConVar autoUnspawnEntityFrameSkip("auto_unspawn_entity_frameskip", "250", FCVAR_ARCHIVE, "Number of frames an entity needs to have missed before it is considered for auto-unspawning.");
ConVar autoUnspawnIntervalFrameSkip("auto_unspawn_interval_frameskip", "500", FCVAR_ARCHIVE, "Number of frames AArcade skips between attempting to detect entities to auto-unspawn.");
ConVar reshadedepth("reshadedepth", "0", FCVAR_NONE, "Internal. Read-only. Set with launcher.");
ConVar play_everywhere("play_everywhere", "0", FCVAR_NONE, "Bool. Causes ALL screens to temporarily behave as video mirrors.");
ConVar attempted_quick_load("attempted_quick_load", "0", FCVAR_HIDDEN, "Internal.");
ConVar show_arcade_hud("show_arcade_hud", "1",  FCVAR_ARCHIVE, "Internal.");
ConVar vrworldmenutest("vrworldmenutest", "0", FCVAR_NONE);
ConVar web_home("web_home", "", FCVAR_ARCHIVE, "The URL used as your home page for AArcade's Steamworks web browser.");
ConVar aampPublic("aamp_public", "1", FCVAR_ARCHIVE);
ConVar aampPersistent("aamp_persistent", "0", FCVAR_ARCHIVE);
ConVar loop_local_videos("loop_local_videos", "1", FCVAR_ARCHIVE);
ConVar twitchEnabled("twitch_enabled", "0", FCVAR_ARCHIVE);
ConVar twitchBotEnabled("twitch_bot_enabled", "0", FCVAR_ARCHIVE);
ConVar cloudAssetsDownload("cloud_assets_download", "1", FCVAR_NONE);
ConVar cloudAssetsUpload("cloud_assets_upload", "1", FCVAR_NONE);
ConVar debugDisableMPModels("debug_disable_mp_models", "0", FCVAR_NONE);
ConVar debugDisableMPPlayers("debug_disable_mp_players", "0", FCVAR_NONE);
ConVar debugDisableMPItems("debug_disable_mp_items", "0", FCVAR_NONE);
ConVar avr("avr", "0", FCVAR_ARCHIVE);
ConVar avramp("avramp", "1.0", FCVAR_ARCHIVE);
ConVar aampLobbyPassword("aamp_lobby_password", "", FCVAR_ARCHIVE);
ConVar output_test_dom("output_test_dom", "0", FCVAR_ARCHIVE);
ConVar interactive_directory_url("interactive_directory_url", "", FCVAR_ARCHIVE);
ConVar aampLobbyId("aamp_lobby_id", "", FCVAR_ARCHIVE);
ConVar aampClientId("aamp_client_id", "", FCVAR_ARCHIVE);
ConVar aampClientKey("aamp_client_key", "", FCVAR_ARCHIVE);
ConVar aampServerKey("aamp_server_key", "", FCVAR_ARCHIVE);
ConVar clampDynamicTextures("clamp_dynamic_textures", "1", FCVAR_ARCHIVE);
ConVar auto_save("auto_save", "1", FCVAR_ARCHIVE);
ConVar old_pause_mode("old_pause_mode", "0", FCVAR_HIDDEN, "Remember the previous pause mode, for when we must FORCE hard pause.");
ConVar pause_mode("pause_mode", "0", FCVAR_ARCHIVE, "When enabled, AArcade will more aggressively suspend itself.  The trade-off is that it takes a moment to un-pause from this mode.");
ConVar old_auto_save("old_auto_save", "1", FCVAR_HIDDEN);

ConVar attract_mode_active("attract_mode_active", "0", FCVAR_HIDDEN);
ConVar camcut_attract_mode_active("camcut_attract_mode_active", "0", FCVAR_HIDDEN);
ConVar attract_mode_wipe("attract_mode_wipe", "0", FCVAR_ARCHIVE, "Do a before/after wipe when cutting between screenshots in attract mode.");

ConVar modelThumbs("model_thumbs_enabled", "1", FCVAR_ARCHIVE);

ConVar local_auto_playlists("local_auto_playlists", "1", FCVAR_ARCHIVE);

ConVar inspect_yaw("inspect_yaw", "0", FCVAR_NONE);
ConVar inspect_pitch("inspect_pitch", "0", FCVAR_NONE);
ConVar inspect_horiz("inspect_horiz", "0", FCVAR_NONE);
ConVar inspect_vert("inspect_vert", "0", FCVAR_NONE);
ConVar inspect_tall("inspect_tall", "0", FCVAR_NONE);

ConVar youtube_end_behavior("youtube_end_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_playlist_behavior("youtube_playlist_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_video_behavior("youtube_video_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_related("youtube_related", "default", FCVAR_ARCHIVE);
ConVar youtube_mixes("youtube_mixes", "1", FCVAR_ARCHIVE);
ConVar youtube_annotations("youtube_annotations", "0", FCVAR_ARCHIVE);

ConVar projector_fix("projector_fix", "1", FCVAR_ARCHIVE);
ConVar autoplay_enabled("autoplay_enabled", "1", FCVAR_ARCHIVE);

ConVar paint_texture("paint_texture", "shantzplacecss/zoeywhite", FCVAR_NONE);

ConVar play_as_next_pet("play_as_next_pet", "0", FCVAR_HIDDEN);
ConVar pet_persistence("pet_persistence", "1", FCVAR_ARCHIVE, "Bool. When true, all pet states will be saved/restored in each instance.");
ConVar pet_resume_at("pet_resume_at", "1", FCVAR_ARCHIVE, "Bool. When true, resuming from wherever the current play-as-pet was in the instance - if it exists - is preferred over the targeted spawn point.");

ConVar disable_multiplayer("disable_multiplayer", "0", FCVAR_NONE);
ConVar avatarUrl("avatar_url", "", FCVAR_HIDDEN, "");
ConVar autoinspect_image_flags("autoinspect_image_flags", "0", FCVAR_ARCHIVE, "");
ConVar sync_overview("sync_overview", "1", FCVAR_ARCHIVE, "");
ConVar right_free_mouse_toggle("right_free_mouse_toggle", "1", FCVAR_ARCHIVE, "When enabled, holding right-mouse-button will toggle free mouse mode.");
ConVar auto_close_tasks("auto_close_tasks", "1", FCVAR_ARCHIVE, "When enabled, AArcade will auto-close most tasks when you open a new task.");
ConVar crosshair_color("crosshair_color", "200 200 200 150", FCVAR_ARCHIVE, "The color of the arcade crosshair.  (REQUIRES RESTART)");
/*ConVar crosshair_size("crosshair_size", "4", FCVAR_ARCHIVE, "The size of the arcade crosshair.  (REQUIRES RESTART)");*/
ConVar should_show_crosshair("should_show_crosshair", "1", FCVAR_ARCHIVE, "Controls the arcade crosshair's visiblity. Requires restart if you modify directly. Use show_arcade_crosshair command instead.");
ConVar host_next_map("host_next_map", "0", FCVAR_NONE);
ConVar ignore_next_tab_up("ignore_next_tab_up", "0", FCVAR_NONE);
ConVar recent_model_id("recent_model_id", "acec221c", FCVAR_NONE, "Stores the most recently used model ID, so it can be quickly used again next time.");
ConVar allow_weapons("allow_weapons", "0", FCVAR_ARCHIVE, "Allow weapons to be switched to & used.");
ConVar process_batch_size("process_batch_size", "100", FCVAR_ARCHIVE, "Control how much of batch operations are processed between render cycles.");
ConVar auto_res("auto_res", "1", FCVAR_ARCHIVE, "Automatically manage window size and position. NOTE: Requires restart to take effect.");
ConVar wait_for_initial_images("wait_for_initial_images", "1", FCVAR_ARCHIVE, "Wait for the textures to load on visible objects when first loading a map.");
ConVar detect_maps_at_startup("detect_maps_at_startup", "1", FCVAR_ARCHIVE, "Automatically detect maps when AArcade starts up.");
ConVar playermodel("playermodel", "", FCVAR_NONE, "The MDL of the model to use on the player.  Don't use models that don't exist.");

ConVar data_aa_npcs_used("data_aa_npcs_used", "", FCVAR_ARCHIVE | FCVAR_HIDDEN, "");
ConVar data_aa_maps_discovered("data_aa_maps_discovered", "", FCVAR_ARCHIVE | FCVAR_HIDDEN, "");
ConVar data_aa_props_used("data_aa_props_used", "", FCVAR_ARCHIVE | FCVAR_HIDDEN, "");
ConVar data_aa_letters_used("data_aa_letters_used", "", FCVAR_ARCHIVE | FCVAR_HIDDEN, "");

ConVar last_opened_project("last_opened_project", "", FCVAR_HIDDEN);	// so we can notify users to load a new map they created in the Level Deisnger menu.
ConVar current_project("current_project", "", FCVAR_HIDDEN);	// So we can show level design people a "Reload Map" button.
ConVar next_player_spawn_override("next_player_spawn_override", "", FCVAR_HIDDEN);	// So map transitions can tell us where to spawn at.
//ConVar player_spawn_override("player_spawn_override", "", FCVAR_REPLICATED, "Entity name to spawn players at (if it exists.)");	// For map transitions, but also to alter respawn position during gameplay via console or map scripting.

ConVar aapropfademin("aapropfademin", "-1", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Distance AArcade objects will start to fade out at. Set to -1 to disable fade. Takes effect upon map reload.");
ConVar aapropfademax("aapropfademax", "0", FCVAR_ARCHIVE | FCVAR_REPLICATED, "Distance AArcade objects will be fully faded out at. Set to 0 to disable fade. Takes effect upon map reload.");
ConVar spawn_dist("spawn_dist", "0", FCVAR_ARCHIVE, "The maximum distance from the player position that objects will spawn in at. A value of 0 means the distance check is disabled. Do NOT set this directly during runtime - use set_spawn_dist instead.");
ConVar spawn_objects_double_tap("spawn_objects_double_tap", "0", FCVAR_ARCHIVE, "If enabled, pressing Spawn Nearby Objects once will show you which items are nearby.  Pressing it a 2nd time will actually spawn them in.");
ConVar spawn_objects_within_view_only("spawn_objects_within_view_only", "1", FCVAR_ARCHIVE, "If enabled, only objects within line-of-sight to the player will get spawned in when using a spawn_dist to limit object spawning.");
ConVar spawn_objects_in_batches("spawn_objects_in_batches", "0", FCVAR_ARCHIVE, "If enabled, all qualifying objects will quickly spawn in all at once instead of one at a time. However, you will notice stutter when large object batches quickly spawn in. This convar only has an impact if spawn_dist is greater than 0.");

ConVar cabinet_attract_mode_active("cabinet_attract_mode_active", "0", FCVAR_REPLICATED | FCVAR_HIDDEN, "Internal use only.");

bool IsFileEqual(const char* inFileA, std::string inFileB)
{
	std::string fileA = inFileA;
	std::string fileB = inFileB;

	std::transform(fileA.begin(), fileA.end(), fileA.begin(), ::tolower);
	std::transform(fileB.begin(), fileB.end(), fileB.begin(), ::tolower);

	std::replace(fileA.begin(), fileA.end(), '\\', '/');
	std::replace(inFileB.begin(), inFileB.end(), '\\', '/');

	return (fileA == fileB);
}

// A LEGACY COMMAND BEING ISSUED MEANS THAT THE PLAYER SHOULD HAVE THEIR KEYBINDS CONFIG RESET FOR REDUX!!
void legacyShowHelpVideo(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_show_help_video("showhelpvideo", legacyShowHelpVideo, "Usage: obsolete");

void legacyRemember(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_remember("+remember", legacyRemember, "Usage: obsolete");

void legacyFocus(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_focus("focus", legacyFocus, "Usage: obsolete");

void legacyHdviewInputToggle(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_hdview_input_toggle("+hdview_input_toggle", legacyHdviewInputToggle, "Usage: obsolete");

void legacyCreateHotlink(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_create_hotlink("createhotlink", legacyCreateHotlink, "Usage: obsolete");

void legacyZoomToggle(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_zoom_toggle("zoomtoggle", legacyZoomToggle, "Usage: obsolete");

void legacyRewards(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_rewards("rewards", legacyRewards, "Usage: obsolete");

void legacyBrowser(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_browser("browser", legacyBrowser, "Usage: obsolete");

void legacyDatabases(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_databases("databases", legacyDatabases, "Usage: obsolete");

void legacyScreenCap(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_screen_cap("screencap", legacyScreenCap, "Usage: obsolete");

void legacyFbShare(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_fb_share("fbshare", legacyFbShare, "Usage: obsolete");

void legacyContinuous(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_continuous("continuous", legacyContinuous, "Usage: obsolete");

void legacySmarcadeMotd(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_smarcade_motd("smarcade_motd", legacySmarcadeMotd, "Usage: obsolete");

void legacyAaEscape(const CCommand &args){ g_pAnarchyManager->ObsoleteLegacyCommandReceived(); }
ConCommand legacy_aa_escape("aa_escape", legacyAaEscape, "Usage: obsolete");
// END OF LEGACY COMMANDS

void DumpItem(const CCommand &args)
{
	KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(std::string(args[1])));
	if (pItemKV)
	{
		DevMsg("Item %s:\n", args[1]);
		for (KeyValues *sub = pItemKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
		{
			if (!sub->GetFirstSubKey())
				DevMsg("\t%s: %s\n", sub->GetName(), sub->GetString());
			else
			{
				DevMsg("\t%s:\n", sub->GetName());
				for (KeyValues *sub2 = sub->GetFirstSubKey(); sub2; sub2 = sub2->GetNextKey())
				{
					if (!sub2->GetFirstSubKey())
						DevMsg("\t\t%s: %s\n", sub2->GetName(), sub2->GetString());
					else
					{
						DevMsg("\t\t%s:\n", sub2->GetName());
						for (KeyValues *sub3 = sub2->GetFirstSubKey(); sub3; sub3 = sub3->GetNextKey())
						{
							if (!sub3->GetFirstSubKey())
								DevMsg("\t\t\t%s: %s\n", sub3->GetName(), sub3->GetString());
						}
					}
				}
			}
		}
	}
}
ConCommand dump_item("dump_item", DumpItem, "Usage: dump the item for the given item ID to the console");


void MyFirstConsoleCommand(const CCommand &args)
{
	// Uncomment the DevMsg line below & change its text to make sure your compile worked & is the one being used.
	// (Don't forget to turn on developer 1 first, if needed.)

	//DevMsg("Hello world!  This is my first console command!\n");
}
ConCommand myFirstConsoleCommand("my_first_console_command", MyFirstConsoleCommand, "Usage: used as an example for making sure the C++ solution compiled correctly.  Don't forget to turn on developer 1 first!");

#include <vgui/IInput.h>
void MouseIdle(const CCommand &args)
{
	if (!g_pAnarchyManager->IsInitialized())
		return;

	if (!g_pAnarchyManager->GetInputManager()->GetInputMode() && tempfreelook.GetBool() && !vgui::input()->IsMouseDown(MOUSE_RIGHT) && !vgui::input()->IsMouseDown(MOUSE_LEFT))
	{
		g_pAnarchyManager->EndTempFreeLook();
		tempfreelook.SetValue(false);
	}
}
ConCommand mouseidle("mouse_idle", MouseIdle, "Usage: the mouse is idling w/o the right-mouse button held down. (this causes the main menu to come up in 1 specific case.)", FCVAR_HIDDEN);

void SetSpawnDist(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	float flDist = Q_atof(args.Arg(1));
	//spawn_dist.SetValue(flDist);
	g_pAnarchyManager->GetInstanceManager()->SetNearestSpawnDistFast(flDist);
}
ConCommand set_spawn_dist("set_spawn_dist", SetSpawnDist, "Usage: Set the maximum distance from the player that objects will spawn in at. Setting it to 0 disables the distance check.", FCVAR_NONE);

void ComparisonRender(const CCommand &args)
{
	g_pAnarchyManager->DoComparisonRender();
}
ConCommand comparison_render("comparison_render", ComparisonRender, "Usage: Do a comparison render of before/after all of the stuff you've spawned into the world.", FCVAR_NONE);

void InstaLaunchShortcut(const CCommand &args)
{
	std::string itemId = (args.ArgC() > 1) ? args[1] : "";
	g_pAnarchyManager->InstaLaunchItem(itemId);
}
ConCommand insta_launch_shortcut("insta_launch_shortcut", InstaLaunchShortcut, "Usage: Instantly launch the object under your crosshair without pulling up a menu. Note that this command **only** works if you launched AArcade with the -allow_insta_launch launch option.", FCVAR_NONE);


//void TestClient(const CCommand &args)
//{
	/*
	C_AwesomiumBrowserInstance* pNetworkInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->GetNetworkAwesomiumBrowserInstance();
	if (!pNetworkInstance)
		return;

	DevMsg("There is, indeed, a network tab.\n");

	std::vector<std::string> params;
	params.push_back("yadda");

	pNetworkInstance->DispatchJavaScriptMethod("aampNetwork", "clientTest", params);
	*/
	//g_pAnarchyManager->GetMetaverseManager()->ReallyJoinNow();
//}
//ConCommand testclient("test_client", TestClient, "Usage: just to test stuff.  temp command.", FCVAR_NONE);


void VRTakeScreenshot(const CCommand &args)
{
#ifdef VR_ALLOWED
	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.bottom = g_pAnarchyManager->GetVRBufferHeight();
	rect.right = g_pAnarchyManager->GetVRBufferWidth();// / 2;

	std::string file = "C:\\vrscreenshot.jpg";//g_pAnarchyManager->GetAArcadeUserFolder() + "\\vrscreenshot.jpg"; //

	char* charFile = new char(file.length()+1);
	charFile[file.length()] = '\0';

	DevMsg("Saving VR Screenshot (%i x %i) to: %s\n", (rect.right / 1), (rect.bottom / 1), file.c_str());
	hmdCaptureScreen(charFile, rect.left, rect.top, rect.right, rect.bottom);// &rect);
	delete[] charFile;
#endif	// end VR_ALLOWED
}
ConCommand vr_takescreenshot("vr_takescreenshot", VRTakeScreenshot, "Usage: VR only.  Takes a screenshot.", FCVAR_HIDDEN);

/*
void UnstickPlayer(const CCommand &args)
{
	engine->ClientCmd("noclip");
}
ConCommand unstickplayer("unstick_player", UnstickPlayer, "Usage: this toggles fly mode on & off again, in an attempt to un-stick the player.", FCVAR_HIDDEN);
*/
void SetStartWithWindows(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	bool bValue = (Q_atoi(args[1]) == 1);
	g_pAnarchyManager->SetStartWithWindows(bValue);
}
ConCommand set_start_with_windows("set_start_with_windows", SetStartWithWindows, "Usage: dump the item for the given item ID to the console", FCVAR_HIDDEN);

void DebugInfo(const CCommand &args)
{
	engine->ClientCmd("developer 1; showconsole;");
	Msg("Debug Info:\n\tInteral State: %i\n", g_pAnarchyManager->GetState());
}
ConCommand debug_info("debug_info", DebugInfo, "Usage: Prints debug info to the console of the internal state of various AArcade systems.", FCVAR_NONE);

void VRRecenter(const CCommand &args)
{
#ifdef VR_ALLOWED
	if (g_pAnarchyManager->IsVRActive())
	{
		hmdRecenter();
		//hmdResetTracking();
	}
#endif
}
ConCommand vr_recenter("vr_recenter", VRRecenter, "Usage: Resets VR orientation.", FCVAR_NONE);

/*
void CreateNavNode(const CCommand &args)
{
	Vector v = g_pAnarchyManager->GetSelectorTraceVector();
	engine->ClientCmd(VarArgs("create_nav_node_server %f %f %f", v.x, v.y, v.z));
}
ConCommand create_nav_node("create_nav_node", CreateNavNode, "Usage: Spawn a nav node.", FCVAR_NONE);
*/

/*
void SetActiveSpawnedShadows(const CCommand &args)
{
	unsigned int numArgs = args.ArgC();
	int val = (numArgs > 1) ? Q_atoi(args[1]) : cvar->FindVar("object_shadows")->GetInt();
	engine->ServerCmd(VarArgs("set_active_spawned_shadows %i", val));
}
ConCommand setactivespawnedshadows("set_active_spawned_shadows", SetActiveSpawnedShadows, "Usage: Bool.  Enable/disable shadows on spawned objects that are already loaded.", FCVAR_NONE);
*/


void DestroyAllPets(const CCommand &args)
{
	g_pAnarchyManager->DestroyAllPets();
}
ConCommand destroy_all_pets("destroy_all_pets", DestroyAllPets, "Usage: Destroy all pets that are live in the current world.");

void ToggleLookspot(const CCommand &args)
{
	int iValue = (args.ArgC() > 1) ? Q_atoi(args[1]) : -1;
	g_pAnarchyManager->ToggleLookspot(iValue);
}
ConCommand toggle_lookspot("toggle_lookspot", ToggleLookspot, "Usage: Toggles lookspot UI. -1 (or no param) for toggle, 0 for off, 1 for on.");

void DestroyLookspot(const CCommand &args)
{
	g_pAnarchyManager->DestroyLookspot();
}
ConCommand destroy_lookspot("destroy_lookspot", DestroyLookspot, "Usage: Interal use only (eventually.) It destroys the lookspots, if they exist.");

void PetTarget(const CCommand &args)
{
	int iArgCount = args.ArgC();

	// optional POSITION string vector to pet_target (so we can pet_target "0 0 0" to clear it)
	bool bWasGivenPosition = (iArgCount > 1);
	if (bWasGivenPosition) {
		std::string pos = (bWasGivenPosition) ? args[1] : "0 0 0";
		Vector posVec;
		if (pos == "-1") {
			posVec.x = 0;
			posVec.y = 0;
			posVec.z = 0;
		}
		else {
			UTIL_StringToVector(posVec.Base(), pos.c_str());
		}
		g_pAnarchyManager->TogglePetTargetPos(posVec);
	}
	else {
		g_pAnarchyManager->TogglePetTargetPos(g_pAnarchyManager->GetSelectorTraceVector());
	}
}
ConCommand pet_target("pet_target", PetTarget, "Usage: Tell pets to go somewhere. Use a 2nd time to return to follow-me mode. The parameter is optional position string. Or use -1 to indicate 0 0 0.");

void PetCreated(const CCommand &args)
{
	int iEntIndex = Q_atoi(args[1]);
	g_pAnarchyManager->PetCreated(iEntIndex);
}
ConCommand pet_created("pet_created", PetCreated, "Usage: Internal use only.", FCVAR_HIDDEN);

void LookSpotCreaated(const CCommand &args)
{
	g_pAnarchyManager->LookspotCreated(Q_atoi(args[1]), Q_atoi(args[2]), Q_atoi(args[3]));
}
ConCommand lookspot_created("lookspot_created", LookSpotCreaated, "Usage: Internal use only.", FCVAR_HIDDEN);

void SpawnPet(const CCommand &args)
{
	std::string model = (args.ArgC() > 1) ? args.Arg(1) : "";

	if (model == "")
	{
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(g_pAnarchyManager->GetSelectedEntity());
		if (!pShortcut)
		{
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));
		}

		if (pShortcut)
		{
			KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pShortcut->GetModelId());
			if (pModelKV) {
				model = pModelKV->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
			}
		}
		if (model == "")
		{
			// grab the next nearest object then
			float flMaxRange = 1000.0f;
			object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
			while (pObject)
			{
				KeyValues* pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetLibraryModel(pObject->modelId);
				if (pModelKV) {
					model = pModelKV->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
					break;
				}
			}
		}
	}
	if (model == "")
	{
		DevMsg("No model found to use as a pet.\n");
		return;
	}

	std::string run = (args.ArgC() > 2) ? args.Arg(2) : "";
	std::string walk = (args.ArgC() > 3) ? args.Arg(3) : "";
	std::string idle = (args.ArgC() > 4) ? args.Arg(4) : "";
	std::string fall = (args.ArgC() > 5) ? args.Arg(5) : "";
	float flScale = (args.ArgC() > 6) ? Q_atof(args.Arg(6)) : 1.0f;
	std::string rot = (args.ArgC() > 7) ? args.Arg(7) : "";
	std::string pos = (args.ArgC() > 8) ? args.Arg(8) : "";
	float flNear = (args.ArgC() > 9) ? Q_atof(args.Arg(9)) : 100.0f;
	float flFar = (args.ArgC() > 10) ? Q_atof(args.Arg(10)) : 200.0f;
	float flRunSpeed = (args.ArgC() > 11) ? Q_atof(args.Arg(11)) : 100.0f;
	float flWalkSpeed = (args.ArgC() > 12) ? Q_atof(args.Arg(12)) : 50.0f;
	std::string outfit = (args.ArgC() > 13) ? args.Arg(13) : "";
	std::string behavior = (args.ArgC() > 14) ? args.Arg(14) : "";
	std::string sequence = (args.ArgC() > 15) ? args.Arg(15) : "";

	g_pAnarchyManager->SpawnPet(model, run, walk, idle, fall, flScale, rot, pos, flNear, flFar, flRunSpeed, flWalkSpeed, outfit, behavior, sequence);
}
ConCommand spawn_pet("spawn_pet", SpawnPet, "For internal use only, for now.");

void AnimalityByTargetnameClient(const CCommand &args)
{
	if (args.ArgC() > 1) {
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(Q_atoi(args[1]));
		//C_BaseEntity* pBaseEntity2 = C_BaseEntity::Instance(Q_atoi(args[2]));

		if (pBaseEntity ){//&& pBaseEntity2) {
			const model_t* pModel = pBaseEntity->GetModel();
			if (pModel)
			{
				std::string modelFilename = modelinfo->GetModelName(pModel);
				DevMsg("Detected pet model: %s\n", modelFilename.c_str());
				modelFilename = g_pAnarchyManager->NormalizeModelFilename(modelFilename);

				/*Vector origin = pBaseEntity2->GetAbsOrigin();
				std::string position = VarArgs("%.10g %.10g %.10g", origin.x, origin.y, origin.z);

				QAngle angles = pBaseEntity2->GetAbsAngles();
				std::string rotation = VarArgs("%.10g %.10g %.10g", angles.x, angles.y, angles.z);*/

				Vector origin;
				UTIL_StringToVector(origin.Base(), args.Arg(3));

				QAngle angles;
				UTIL_StringToVector(angles.Base(), args.Arg(4));

				// determine if pet already exists
				pet_t* pPet = g_pAnarchyManager->FindPetByModel(modelFilename);
				if (!pPet )
				{
					// create our pet
					//g_pAnarchyManager->SpawnPet(model, run, walk, idle, fall, flScale, rot, pos, flNear, flFar, flRunSpeed, flWalkSpeed);

					int iOffset = 1;	// so we know where the rest of the regular pet options begin.
					std::string run = (args.ArgC() > 2 + iOffset) ? args.Arg(2 + iOffset) : "";
					std::string walk = (args.ArgC() > 3 + iOffset) ? args.Arg(3 + iOffset) : "";
					std::string idle = (args.ArgC() > 4 + iOffset) ? args.Arg(4 + iOffset) : "";
					std::string fall = (args.ArgC() > 5 + iOffset) ? args.Arg(5 + iOffset) : "";
					//float flScale = (args.ArgC() > 6) ? Q_atof(args.Arg(6)) : 1.0f;
					std::string flScale = (args.ArgC() > 6 + iOffset) ? args.Arg(6 + iOffset) : "";
					std::string rot = (args.ArgC() > 7 + iOffset) ? args.Arg(7 + iOffset) : "";
					std::string pos = (args.ArgC() > 8 + iOffset) ? args.Arg(8 + iOffset) : "";
					std::string flNear = args.Arg(9 + iOffset);
					std::string flFar = args.Arg(10 + iOffset);
					std::string flRunSpeed = args.Arg(11 + iOffset);
					std::string flWalkSpeed = args.Arg(12 + iOffset);

					//g_pAnarchyManager->SpawnPet(modelFilename, run, walk, idle, fall, flScale, rot, pos, flNear, flFar, flRunSpeed, flWalkSpeed);
					
					// teleport us to the destination
					//engine->ClientCmd(VarArgs("teleport_player_to_entity %i;", pBaseEntity2->entindex()));
					engine->ClientCmd(VarArgs("play_as_next_pet 1; spawn_pet \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\";", modelFilename.c_str(), run.c_str(), walk.c_str(), idle.c_str(), fall.c_str(), flScale.c_str(), rot.c_str(), pos.c_str(), flNear.c_str(), flFar.c_str(), flRunSpeed.c_str(), flWalkSpeed.c_str()));
					//pPet = g_pAnarchyManager->FindPetByModel(modelFilename);
				}

				/*if (pPet)
				{
					// set the pet as our PlayAsPet pet.
					g_pAnarchyManager->SetPlayAsPet(pPet);

					//"ENTER PLAYASPET MODE"
					engine->ClientCmd("set_prop_vis -1 0;");
					engine->ClientCmd(VarArgs("set_prop_vis %i 1;", pPet->iEntityIndex));
				}
				else
				{
					DevMsg("No pet found.\n");
				}*/
			}
		}
	}
}
ConCommand animality_by_targetname_client("animality_by_targetname_client", AnimalityByTargetnameClient, "For internal use only.");

void ArcadeHud(const CCommand &args)
{
	int iVal = show_arcade_hud.GetInt();
	int iNextVal = iVal + 1;
	int iMax = 1;
	if (iNextVal > iMax)
		iNextVal = 0;

	show_arcade_hud.SetValue(iNextVal);
}
ConCommand arcade_hud("arcade_hud", ArcadeHud, "Usage: Cycles the arcade HUD state.", FCVAR_NONE);

void CamMayaToggle(const CCommand &args)
{
	if (cvar->FindVar("cam_is_thirdperson_mode")->GetBool())
	{
		engine->ClientCmd("thirdperson_mayamode");
	}
	else
	{
		if (cvar->FindVar("cam_is_maya_mode")->GetBool())
		{
			engine->ClientCmd("thirdperson; thirdperson_mayamode;");
		}
		else		
			engine->ClientCmd("thirdperson");	// there is a bug when you try to go into 3rd person while maya mode is on, so avoid this.
	}
}
ConCommand cam_maya_toggle("cam_maya_toggle", CamMayaToggle, "Usage: Toggles maya camera mode. (ie. rotation lock)", FCVAR_NONE);

void TogglePerspective(const CCommand &args)
{
	if (cvar->FindVar("cam_is_thirdperson_mode")->GetBool())
	{
		engine->ClientCmd("firstperson");
	}
	else
	{
		if (cvar->FindVar("cam_is_maya_mode")->GetBool())	// there is a bug when you try to go into 3rd person while maya mode is on, so avoid this.
			engine->ClientCmd("thirdperson; thirdperson_mayamode;");
		else
			engine->ClientCmd("thirdperson");
	}
}
ConCommand toggle_perspective("toggle_perspective", TogglePerspective, "Usage: Toggles your camera between 1st & 3rd person mode.", FCVAR_NONE);

void TinyModeToggle(const CCommand &args)
{
	float flPlayerScale = C_BasePlayer::GetLocalPlayer()->GetModelScale();

	if (flPlayerScale == 0.1f)
	{
		engine->ClientCmd("ent_fire !player setmodelscale 1.0; cl_forwardspeed 450; cl_sidespeed 450; cl_backspeed 450; znear 7;");
	}
	else
	{
		engine->ClientCmd("ent_fire !player setmodelscale 0.1; cl_forwardspeed 50; cl_sidespeed 50; cl_backspeed 50; znear 2;");
	}
}
ConCommand tiny_mode_toggle("tiny_mode_toggle", TinyModeToggle, "Usage: Toggles tiny mode.", FCVAR_NONE);

void ToggleAttractMode(const CCommand &args)
{
	g_pAnarchyManager->ToggleAttractMode();
}
ConCommand toggle_attract_mode("toggle_attract_mode", ToggleAttractMode, "Usage: Turn on/off camera attract mode.", FCVAR_NONE);

void AttractCameraReached(const CCommand &args)
{
	g_pAnarchyManager->AttractCameraReached();
}
ConCommand attract_camera_reached("attract_camera_reached", AttractCameraReached, "Usage: Internal Use Only.", FCVAR_HIDDEN);

void ToggleOverlay(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		engine->ClientCmd("r_screenoverlay \"off\"");
	}
	else
	{
		std::string materialName = args.Arg(1);
		if (!Q_stricmp(cvar->FindVar("view_overlay")->GetString(), materialName.c_str()))
			engine->ClientCmd("r_screenoverlay \"off\"");
		else
			engine->ClientCmd(VarArgs("r_screenoverlay \"%s\"", materialName.c_str()));
	}
}
ConCommand toggleoverlay("toggle_overlay", ToggleOverlay, "Usage: Toggles the given overlay material.", FCVAR_NONE);

void SelectNext(const CCommand &args)
{
	g_pAnarchyManager->SelectNext();
}
ConCommand select_next("select_next", SelectNext, "Usage: Selects the next nearest cabinet (you must currently have one selected first.)", FCVAR_NONE);

void SelectPrev(const CCommand &args)
{
	g_pAnarchyManager->SelectPrev();
}
ConCommand select_prev("select_prev", SelectPrev, "Usage: Selects the previous nearest cabinet (you must currently have one selected first.)", FCVAR_NONE);

void SetDrawForeground(const CCommand &args)
{
	//SetDrawForeground
	
	bool bValue = (args.ArgC() > 1) ? Q_atoi(args[1]) : true;

	// get the object we are given, or the object we are aimed at, or the object nearest to where we are aiming (in that priority)
	C_PropShortcutEntity* pShortcut = null;

	if (args.ArgC() > 1)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[2])));

	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if (!pShortcut)
	{
		// grab the next nearest object then
		float flMaxRange = 1000.0f;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		while (pObject)
		{
			if (pObject->spawned)
			{
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
				break;
			}

			pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(pObject, flMaxRange);
		}
	}

	if (pShortcut)
		pShortcut->SetDrawForeground(bValue);
}
ConCommand set_draw_foreground("set_draw_foreground", SetDrawForeground, "Usage: Causes the model to be drawn in front of everything else.", FCVAR_NONE);

void LookAtMe(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = null;// = (pEntity) ? dynamic_cast<C_PropShortcutEntity*>(pEntity) : null;
	//C_BaseEntity* pEntity;

	if (args.ArgC() > 1)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1])));

	if (!pShortcut)
	{
		// check if a propshortcut is under the player's crosshair
		//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

		/*
		// fire a trace line
		trace_t tr;
		g_pAnarchyManager->SelectorTraceLine(tr);
		//Vector forward;
		//pPlayer->EyeVectors(&forward);
		//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if( tr.fraction != 1.0 )
			pEntity = (tr.DidHitNonWorldEntity()) ? tr.m_pEnt : null;
			*/

		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));
	}

	if (!pShortcut)
	{
		// grab the next nearest object then
		float flMaxRange = 1000.0f;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		while (pObject)
		{
			if (pObject->spawned)
			{
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
				break;
			}

			pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(pObject, flMaxRange);
		}

	}

	
	// only allow prop shortcuts
	//C_PropShortcutEntity* pShortcut = (pEntity) ? dynamic_cast<C_PropShortcutEntity*>(pEntity) : null;
	if (pShortcut)
	{
		g_pAnarchyManager->ToggleAlwaysLookObject(pShortcut);
	}
}
ConCommand look_at_me("look_at_me", LookAtMe, "Usage: Causes the item under your crosshair to constantly look at you.", FCVAR_NONE);

void InspectObject(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = null;

	if (args.ArgC() > 1)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1])));

	if (!pShortcut)
	{
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));
	}

	if (!pShortcut)
	{
		// grab the next nearest object then
		float flMaxRange = 1000.0f;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		while (pObject)
		{
			if (pObject->spawned)
			{
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
				break;
			}

			pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(pObject, flMaxRange);
		}

	}

	if (pShortcut && (!pShortcut->GetMoveParent() || pShortcut->GetMoveParent() == pShortcut))
	{
		pShortcut->SetDrawForeground(true);
		//g_pAnarchyManager->SetForegroundShortcut(pShortcut);
		g_pAnarchyManager->ActivateInspectObject(pShortcut);
	}
}
ConCommand inspect_object("inspect_object", InspectObject, "Usage: Inspect the object under your crosshair.", FCVAR_NONE);

/*void SetTransmitState(const CCommand &args)
{
	
}
ConCommand set_transmit_state("set_transmit_state", SetTransmitState, "Change the transmit state of an entity between FL_EDICT_ALWAYS and default.", FCVAR_NONE);*/

void InspectObjectStop(const CCommand &args)
{
	g_pAnarchyManager->DeactivateInspectObject();
}
ConCommand inspect_object_stop("inspect_object_stop", InspectObjectStop, "Usage: Exits inspect object mode.", FCVAR_NONE);

void UseShortcut(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1])));
	if (pShortcut)
		pShortcut->OnUsed();
}
ConCommand use_shortcut("use_shortcut", UseShortcut, "Usage: ", FCVAR_NONE);

void GetColMinMaxClient(const CCommand &args)
{
	// check if a propshortcut is under the player's crosshair
	//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	/*
	// fire a trace line
	trace_t tr;
	g_pAnarchyManager->SelectorTraceLine(tr);
	//Vector forward;
	//pPlayer->EyeVectors(&forward);
	//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	C_BaseEntity *pEntity = (tr.DidHitNonWorldEntity()) ? tr.m_pEnt : null;
	*/

	C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());

	// only allow prop shortcuts
	C_PropShortcutEntity* pShortcut = (pEntity) ? dynamic_cast<C_PropShortcutEntity*>(pEntity) : null;
	if (pShortcut)// && tr.fraction != 1.0)
		engine->ClientCmd(VarArgs("getcolminmax %i;\n", pShortcut->entindex()));	// servercmdfix , false);
}
ConCommand getcolminmaxclient("getcolminmaxclient", GetColMinMaxClient, "Interal use only.", FCVAR_HIDDEN);

void SelectorValue(const CCommand &args)
{
	int iEntity = Q_atoi(args[1]);
	float flX = Q_atof(args[2]);
	float flY = Q_atof(args[3]);
	float flZ = Q_atof(args[4]);
	float flNormalX = Q_atof(args[5]);
	float flNormalY = Q_atof(args[6]);
	float flNormalZ = Q_atof(args[7]);

	g_pAnarchyManager->OnSelectorTraceResponse(iEntity, flX, flY, flZ, flNormalX, flNormalY, flNormalZ);
}
ConCommand selectorvalue("selector_value", SelectorValue, "Interal use only.", FCVAR_HIDDEN);

void ColMinMaxGotten(const CCommand &args)
{
	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(Q_atoi(args[1]));
	if (pBaseEntity)
	{
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (pShortcut)
		{
			if (Q_atoi(args[2]) == 1)
			{
				KeyValues* pKV = new KeyValues("verts");
				pKV->LoadFromFile(g_pFullFileSystem, "temp_verts.txt", "DEFAULT_WRITE_PATH", true);

				int minX = ScreenWidth();
				int minY = ScreenHeight();
				int maxX = 0;
				int maxY = 0;
				Vector tester;
				bool bIsOnScreen;
				int iX = 0;	// FIXME: 0 is probably a bad default value.  Valve seems to set it to -640 when clamping.
				int iY = 0;
				Vector dummy;
				for (KeyValues *sub = pKV->GetFirstSubKey(); sub; sub = sub->GetNextKey())
				{
					UTIL_StringToVector(tester.Base(), sub->GetString());
					VectorTransform(tester, pBaseEntity->EntityToWorldTransform(), dummy);
					bIsOnScreen = GetVectorInScreenSpace(dummy, iX, iY);

					//if (bIsOnScreen)
					//{
						if (iX < minX)
							minX = iX;
						if (iY < minY)
							minY = iY;
						if (iX > maxX)
							maxX = iX;
						if (iY > maxY)
							maxY = iY;
					//}
				}
				pKV->deleteThis();

				//DevMsg("Min: %i %i\nMax: %i %i\n", minX, minY, maxX, maxY);
				C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
				if (pHudInstance)
				{
					std::vector<std::string> params;
					params.push_back(VarArgs("%i", minX));
					params.push_back(VarArgs("%i", minY));
					params.push_back(VarArgs("%i", maxX));
					params.push_back(VarArgs("%i", maxY));
					pHudInstance->DispatchJavaScriptMethod("bboxListener", "onBBoxObtained", params);
				}
			}
		}
	}
}
ConCommand colminmaxgotten("colminmaxgotten", ColMinMaxGotten, "Interal use only.", FCVAR_HIDDEN);

void RefreshImages(const CCommand &args)
{
	std::string itemId = args[1];
	std::string modelId = args[2];
	g_pAnarchyManager->RefreshImages(itemId, modelId);
}
ConCommand refreshimages("refreshimages", RefreshImages, "Usage: refreshimages ITEMID MODELID", FCVAR_NONE);

void GotValid2DBBoxes(const CCommand &args)
{
	std::vector<std::string> params;

	KeyValues* pKV = new KeyValues("verts");
	pKV->LoadFromFile(g_pFullFileSystem, "temp_entities.txt", "DEFAULT_WRITE_PATH", true);

	//float dist;
	int screenWidth = ScreenWidth();
	int screenHeight = ScreenHeight();
	int minX;
	int minY;
	int maxX;
	int maxY;
	Vector tester;
	bool bIsOnScreen;
	int iX;
	int iY;
	Vector dummy;
	C_BaseEntity* pBaseEntity;
	C_PropShortcutEntity* pShortcutEntity;
	Vector playerPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
	for (KeyValues *pEntityEntryKV = pKV->GetFirstSubKey(); pEntityEntryKV; pEntityEntryKV = pEntityEntryKV->GetNextKey())
	{
		pBaseEntity = C_BaseEntity::Instance(Q_atoi(pEntityEntryKV->GetName()));
		if (!pBaseEntity)
			continue;

		pShortcutEntity = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pShortcutEntity)
			continue;

		//dist = pShortcutEntity->GetAbsOrigin().DistTo(playerPos);
		params.push_back(pShortcutEntity->GetObjectId());
		//params.push_back(VarArgs("%.02f", dist));

		minX = screenWidth;
		minY = screenHeight;
		maxX = 0;
		maxY = 0;

		for (KeyValues *pVertKV = pEntityEntryKV->GetFirstSubKey(); pVertKV; pVertKV = pVertKV->GetNextKey())
		{
			UTIL_StringToVector(tester.Base(), pVertKV->GetString());
			VectorTransform(tester, pBaseEntity->EntityToWorldTransform(), dummy);
			bIsOnScreen = GetVectorInScreenSpace(dummy, iX, iY);

			if (bIsOnScreen)
			{
				if (iX < minX)
					minX = iX;
				if (iY < minY)
					minY = iY;
				if (iX > maxX)
					maxX = iX;
				if (iY > maxY)
					maxY = iY;
			}
		}

		params.push_back(VarArgs("%i", minX));
		params.push_back(VarArgs("%i", minY));
		params.push_back(VarArgs("%i", maxX));
		params.push_back(VarArgs("%i", maxY));
	}
	pKV->deleteThis();

	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (pHudInstance)
	{
		pHudInstance->DispatchJavaScriptMethod("bboxListener", "onBBoxesObtained", params);
	}
}
ConCommand gotvalid2dbboxes("got_valid_2d_bboxes", GotValid2DBBoxes, "Interal use only.", FCVAR_HIDDEN);

/*
void PrepMediaScreenshot(const CCommand &args)
{
	g_pAnarchyManager->TakeMediaScreenshot();
}
ConCommand prep_media_screenshot("prep_media_screenshot", PrepMediaScreenshot, "Usage: prep a special screenshot that generates an interactive web page.", FCVAR_NONE);

void TakeMediaScreenshot(const CCommand &args)
{
	g_pAnarchyManager->TakeMediaScreenshot();
}
ConCommand take_media_screenshot("media_screenshot", TakeMediaScreenshot, "Usage: take a special screenshot that generates an interactive web page.", FCVAR_NONE);
*/
void DisableWeapons(const CCommand &args)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!!pPlayer && Q_strcmp(pPlayer->GetActiveWeapon()->GetName(), "weapon_physcannon"))
	{
		CBaseCombatWeapon *pWeapon = pPlayer->Weapon_OwnsThisType("weapon_physcannon");
		if (pWeapon && pPlayer->GetActiveWeapon()->CanHolster())
			engine->ClientCmd("phys_swap");
	}

	cvar->FindVar("r_drawviewmodel")->SetValue(false);
	cvar->FindVar("cl_drawhud")->SetValue(false);
	allow_weapons.SetValue(false);
}
ConCommand disable_weapons("disable_weapons", DisableWeapons, "Usage: disables weapons (also switches you to grav gun)");

void EnableWeapons(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	cvar->FindVar("r_drawviewmodel")->SetValue(true);
	cvar->FindVar("cl_drawhud")->SetValue(true);
	allow_weapons.SetValue(true);
}
ConCommand enable_weapons("enable_weapons", EnableWeapons, "Usage: enables weapons (as well as POV models & the weapon HUD.)");

void ToggleWeapons(const CCommand &args)
{
	if (allow_weapons.GetBool())
	{
		engine->ClientCmd("disable_weapons;");
	}
	else
	{
		engine->ClientCmd("enable_weapons; impulse 101;");
	}
}
ConCommand toggle_weapons("toggle_weapons", ToggleWeapons, "Usage: Togles weapons (as well as POV models & the weapon HUD.)");

void Panoshot(const CCommand &args)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || pConnection->isHost)
		g_pAnarchyManager->Panoshot();
	else
		g_pAnarchyManager->AddToastMessage("Guests are not allowed to do that in this session.");
	/*
	engine->ExecuteClientCmd("disable_weapons; jpeg_quality 97; fov 106; setang 0 0 0; jpeg;");
	engine->ExecuteClientCmd("setang 0 -90 0; jpeg;");
	engine->ExecuteClientCmd("setang 0 180 0; jpeg;");
	engine->ExecuteClientCmd("setang 0 90 0; jpeg;");
	engine->ExecuteClientCmd("setang 90 180 0; jpeg;");
	engine->ExecuteClientCmd("setang -90 180 0; jpeg;");
	engine->ExecuteClientCmd("setang 0 0 0; fov 90;");
	*/

	//engine->ExecuteClientCmd("disable_weapons; jpeg_quality 97; fov 106; setang 0 0 0; jpeg; setang 0 -90 0; jpeg; setang 0 180 0; jpeg; setang 0 90 0; jpeg; setang 90 180 0; jpeg; setang -90 180 0; jpeg; setang 0 0 0; fov 90;");
}
ConCommand panoshot("panoshot", Panoshot, "Usage: takes a panoramic screenshot.");

void MapTransition(const CCommand &args)
{
	std::string mapfile = args[1];
	std::string spawnEntityName = args[2];
	g_pAnarchyManager->MapTransition(mapfile, spawnEntityName);
}
ConCommand maptransition("map_transition", MapTransition, "Usage: INTERNAL. transitions to the specified map ID.", FCVAR_HIDDEN);

void ChatMessage(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/chat.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand chat_message("chat_message", ChatMessage, "Usage: show the chat msg menu.");

void WheelMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/wheelspin.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand wheel_menu("wheel_menu", WheelMenu, "Usage: show the wheel menu.");

void RadialMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/radialMenu.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand radial_menu("radial_menu", RadialMenu, "Usage: show the radial menu.");

void ChatbotOutputOverlay(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/chatbot_output_overlay.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance, false);
}
ConCommand chatbot_output_overlay("chatbot_output_overlay", ChatbotOutputOverlay, "Usage: show the chatbot output overlay.");

void ChatbotMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/chatbot_menu.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand chatbot_menu("chatbot_menu", ChatbotMenu, "Usage: show the chatbot menu.");

void PetMenu(const CCommand &args)
{
	//if (g_pAnarchyManager->GetSelectedEntity())
	//	g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (args.ArgC() > 1)
	{
		pHudBrowserInstance->SetUrl(VarArgs("asset://ui/petMenu.html?petEntity=%i", Q_atoi(args[1])));
	}
	else
	{
		pHudBrowserInstance->SetUrl("asset://ui/petMenu.html");
	}
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand pet_menu("pet_menu", PetMenu, "Usage: show the pet context menu. INTERNAL USE ONLY. Needs an entity index as the parameter.", FCVAR_HIDDEN);

void PetsTab(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowPetsMenu();
}
ConCommand pets_tab("pets_tab", PetsTab, "Usage: Show the pets tab.");

void OnPetUsed(const CCommand &args)
{
	//C_DynamicProp* pProp = NULL;
	//C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(Q_atoi(args[1]));
	//if (pBaseEntity)
		//pProp = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);

	engine->ClientCmd(VarArgs("pet_menu %i;", Q_atoi(args[1])));
}
ConCommand on_pet_used("on_pet_used", OnPetUsed, "Usage: a pet has been USED by the local player");

void AvatarMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->TaskRemember();

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudBrowserInstance->SetUrl("asset://ui/avatar.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
}
ConCommand avatar_menu("avatar_menu", AvatarMenu, "Usage: show the avatar menu.");

/*
void TestLink(const CCommand &args)
{
	aampConnection_t* pConnection = g_pAnarchyManager->GetConnectedUniverse();
	if (!pConnection || !pConnection->connected)
	{
		DevMsg("No online universe connected.");
	}
	else
	{
		DevMsg("http://www.anarchyarcade.com/session.html?universe=%s&instance=%s", pConnection->universe.c_str(), pConnection->instance.c_str());//pConnection->address.c_str(), 
	}
}
ConCommand testlink("testlink", TestLink, "Usage: gives a link to the currently connected session.");
*/

void HandleDragDrop(HDROP hDrop)
{
	// get where mouse is.. if u care
	POINT mousePt;
	DragQueryPoint(hDrop, &mousePt);


	// get file(s)
	int numFiles = DragQueryFile(hDrop, -1, NULL, 0);
	for (int x = 0; x<numFiles; x++)
	{
		char filename[MAX_PATH];
		DragQueryFile(hDrop, x, filename, MAX_PATH);

		MessageBox(NULL, filename, NULL, 0);
	}



	// clean up
	DragFinish(hDrop);
}

void AvatarObjectCreated(const CCommand &args)
{
	int iEntIndex = Q_atoi(args[1]);
	std::string userId = args[2];
	g_pAnarchyManager->GetMetaverseManager()->AvatarObjectCreated(iEntIndex, userId);
}
ConCommand avatar_object_created("avatar_object_created", AvatarObjectCreated, "Usage: interal use only.");

void LocalAvatarObjectCreated(const CCommand &args)
{
	int iEntIndex = Q_atoi(args[1]);
	//std::string userId = args[2];
	g_pAnarchyManager->LocalAvatarObjectCreated(iEntIndex);
}
ConCommand local_avatar_object_created("local_avatar_object_created", LocalAvatarObjectCreated, "Usage: interal use only.");

void LocalPlayerDied(const CCommand &args)
{
	g_pAnarchyManager->LocalPlayerDied();
}
ConCommand local_player_died("local_player_died", LocalPlayerDied, "Usage: interal use only.");

void LocalPlayerSpawned(const CCommand &args)
{
	g_pAnarchyManager->LocalPlayerSpawned();
}
ConCommand local_player_spawned("local_player_spawned", LocalPlayerSpawned, "Usage: interal use only.");

void VRHandCreated(const CCommand &args)
{
	int iEntIndex = Q_atoi(args[1]);
	int iHandSide = Q_atoi(args[2]);
	int iPointerEntIndex = (args.ArgC() > 3) ? Q_atoi(args[3]) : -1;
	int iTeleportEntIndex = (args.ArgC() > 4) ? Q_atoi(args[4]) : -1;
	g_pAnarchyManager->VRHandCreated(iEntIndex, iHandSide, iPointerEntIndex, iTeleportEntIndex);
}
ConCommand vr_hand_created("vr_hand_created", VRHandCreated, "Usage: interal use only.");

ConVar usevr("usevr", "0", FCVAR_HIDDEN, "Internal testing.");
//#include "client_virtualreality.h"//"iclientvirtualreality.h"
//#include "sourcevr/isourcevirtualreality.h"
void TestFunction( const CCommand &args )
{
	//usevr.SetValue(true);
	//g_ClientVirtualReality.Activate();


	/*
		return VarArgs("steam://run/%llu", Q_atoui64(this->SecurityFilter(fileLocation.c_str())));




		std::string buf = engine->GetGameDirectory();
		size_t found = buf.find_last_of("\\");
		buf = buf.substr(0, found);
		found = buf.find_last_of("\\");
		buf = buf.substr(0, found);
		found = buf.find_last_of("\\");
		buf = buf.substr(0, found);
		buf += "\\steam.exe";

		std::string finalBuf = "\"";
		finalBuf += buf;
		finalBuf += "\" -applaunch ";
		finalBuf += fileLocation;
	*/


	//SetScreenOverlayMaterial(IMaterial *pMaterial) = 0;
	//virtual IMaterial	*GetScreenOverlayMaterial() = 0;
	
	//gHLClient;
	//ViewportClientSystem();
	//render->Cline
	//engine->WriteSaveGameScreenshotOfSize

	// WORKING SEND/RECIEVE FILE CALLS
	//#include "inetchannel.h"
//	INetChannel* pINetChannel = static_cast<INetChannel*>(engine->GetNetChannelInfo());
//	pINetChannel->RequestFile("downloads/<hash>.vtf", false);
//	pINetChannel->SendFile("stuff/test.jpg", 0, false);

	// WORKING CURSOR POSITIONS
//	#include "vgui/IInput.h"
//	#include <vgui_controls/Controls.h>
//	int x, y;
//	vgui::input()->GetCursorPos(x, y);

	// save out text versions of all the required stuff so that they can be added to the player's library when ever needed (setup or repair)
	// SCRAPERS (just make backup copies of the scraper .js files)

	/*
	unsigned int i;
	unsigned int max;
	KeyValues* entry;
	std::string fileName;
	size_t found;
	std::map<std::string, KeyValues*>::iterator it;
	bool bIsDefault;

	// MAPS
	std::vector<std::string> defaultMapNames;
	defaultMapNames.push_back("dm_lockdown.bsp");
	defaultMapNames.push_back("hub_floors.bsp");
	defaultMapNames.push_back("hub_highrise.bsp");
	defaultMapNames.push_back("hub_walls.bsp");
	defaultMapNames.push_back("learn_basic.bsp");
	defaultMapNames.push_back("meta_hood.bsp");
	defaultMapNames.push_back("oververse.bsp");
	defaultMapNames.push_back("sm_acreage.bsp");
	defaultMapNames.push_back("sm_apartment.bsp");
	defaultMapNames.push_back("sm_apartmentsuite.bsp");
	defaultMapNames.push_back("sm_expo.bsp");
	defaultMapNames.push_back("sm_gallery.bsp");
	defaultMapNames.push_back("sm_garage.bsp");
	defaultMapNames.push_back("sm_orchard.bsp");
	defaultMapNames.push_back("sm_primo.bsp");

	std::vector<std::string>::iterator defaultMapsIt;
	std::map<std::string, KeyValues*>& maps = g_pAnarchyManager->GetMetaverseManager()->GetAllMaps();
	it = maps.begin();
	while (it != maps.end())
	{
		entry = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		defaultMapsIt = std::find(defaultMapNames.begin(), defaultMapNames.end(), std::string(entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file")));
		if (defaultMapsIt != defaultMapNames.end())
		{
			//DevMsg("Map file: %s\n", entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"));
			fileName = entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
			found = fileName.find_last_of(".");
			fileName = fileName.substr(0, found);

			it->second->SaveToFile(g_pFullFileSystem, VarArgs("defaultLibrary/maps/%s.txt", fileName.c_str()), "DEFAULT_WRITE_PATH");
		}

		it++;
	}

	// CABINETS
	std::vector<std::string> defaultCabinetNames;
	defaultCabinetNames.push_back("models/cabinets/wood_cabinet_steam.mdl");
	defaultCabinetNames.push_back("models/cabinets/wood_cabinet.mdl");
	defaultCabinetNames.push_back("models/icons/wall_pad_w.mdl");
	defaultCabinetNames.push_back("models/icons/wall_pad_t.mdl");
	defaultCabinetNames.push_back("models/cabinets/wall_arcade.mdl");
	defaultCabinetNames.push_back("models/cabinets/two_player_arcade.mdl");
	defaultCabinetNames.push_back("models/cabinets/tripple_standup_racer.mdl");
	defaultCabinetNames.push_back("models/cabinets/trading_card_table.mdl");
	defaultCabinetNames.push_back("models/cabinets/trading_card_big_table.mdl");
	defaultCabinetNames.push_back("models/cabinets/trading_card_big.mdl");
	defaultCabinetNames.push_back("models/cabinets/trading_card.mdl");
	defaultCabinetNames.push_back("models/cabinets/theater_facade.mdl");
	defaultCabinetNames.push_back("models/frames/tall_rotator.mdl");
	defaultCabinetNames.push_back("models/cabinets/tabletop_tv.mdl");
	defaultCabinetNames.push_back("models/cabinets/tabletop_console_steam.mdl");
	defaultCabinetNames.push_back("models/icons/table_pad_w.mdl");
	defaultCabinetNames.push_back("models/icons/table_pad_t.mdl");
	defaultCabinetNames.push_back("models/cabinets/swordfish.mdl");
	defaultCabinetNames.push_back("models/cabinets/standup_car_racer.mdl");
	defaultCabinetNames.push_back("models/banners/spinning_cap.mdl");
	defaultCabinetNames.push_back("models/cabinets/sound_pillar.mdl");
	defaultCabinetNames.push_back("models/cabinets/single_car_racer.mdl");
	defaultCabinetNames.push_back("models/cabinets/racer_multiscreen.mdl");
	defaultCabinetNames.push_back("models/cabinets/posterscreen.mdl");
	defaultCabinetNames.push_back("models/cabinets/poster.mdl");
	defaultCabinetNames.push_back("models/cabinets/pinball_standard.mdl");
	defaultCabinetNames.push_back("models/frames/pic_wide_l.mdl");
	defaultCabinetNames.push_back("models/frames/pic_tall_l.mdl");
	defaultCabinetNames.push_back("models/cabinets/phaser_rifle_coop.mdl");
	defaultCabinetNames.push_back("models/cabinets/phaser_gun_coop.mdl");
	defaultCabinetNames.push_back("models/cabinets/pc_wallmount_small.mdl");
	defaultCabinetNames.push_back("models/cabinets/pc_wallmount.mdl");
	defaultCabinetNames.push_back("models/cabinets/pc_kiosk_standard.mdl");
	defaultCabinetNames.push_back("models/cabinets/normal_laptop.mdl");
	defaultCabinetNames.push_back("models/cabinets/movie_stand_standard.mdl");
	defaultCabinetNames.push_back("models/cabinets/movie_display_wallmount.mdl");
	defaultCabinetNames.push_back("models/cabinets/motoracer.mdl");
	defaultCabinetNames.push_back("models/cabinets/lunar.mdl");
	defaultCabinetNames.push_back("models/cabinets/imax.mdl");
	defaultCabinetNames.push_back("models/cabinets/icade.mdl");
	defaultCabinetNames.push_back("models/cabinets/future_speaker.mdl");
	defaultCabinetNames.push_back("models/cabinets/four_player_arcade.mdl");
	defaultCabinetNames.push_back("models/cabinets/extended_four_player_arcade.mdl");
	defaultCabinetNames.push_back("models/cabinets/enclosed_flight.mdl");
	defaultCabinetNames.push_back("models/cabinets/double_phasergun_coop.mdl");
	defaultCabinetNames.push_back("models/cabinets/double_car_racer.mdl");
	defaultCabinetNames.push_back("models/cabinets/console_steam.mdl");
	defaultCabinetNames.push_back("models/cabinets/console_kiosk_steam.mdl");
	defaultCabinetNames.push_back("models/cabinets/coffee_cabinet.mdl");
	defaultCabinetNames.push_back("models/Frames/ceiling_pic_wide_l.mdl");
	defaultCabinetNames.push_back("models/frames/ceiling_pic_tall_l.mdl");
	defaultCabinetNames.push_back("models/cabinets/cd_wall.mdl");
	defaultCabinetNames.push_back("models/cabinets/cd_table.mdl");
	defaultCabinetNames.push_back("models/cabinets/cd_player_headphones.mdl");
	defaultCabinetNames.push_back("models/cabinets/cd_headphones_wall.mdl");
	defaultCabinetNames.push_back("models/cabinets/camcorder.mdl");
	defaultCabinetNames.push_back("models/cabinets/cabsolo.mdl");
	defaultCabinetNames.push_back("models/cabinets/buttonmasher.mdl");
	defaultCabinetNames.push_back("models/cabinets/brainiac.mdl");
	defaultCabinetNames.push_back("models/cabinets/boxcade.mdl");
	defaultCabinetNames.push_back("models/cabinets/big_movie_wallmount_no_banner.mdl");
	defaultCabinetNames.push_back("models/cabinets/big_movie_wallmount.mdl");
	defaultCabinetNames.push_back("models/banners/big_marquee.mdl");
	max = defaultCabinetNames.size();

	//std::vector<std::string>::iterator defaultCabinetsIt;
	std::map<std::string, KeyValues*>& cabinets = g_pAnarchyManager->GetMetaverseManager()->GetAllModels();
	it = cabinets.begin();
	while (it != cabinets.end())
	{
		entry = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		if (entry->GetInt("dynamic") == 1)
		{
			//defaultCabinetsIt = std::find(defaultCabinetNames.begin(), defaultCabinetNames.end(), std::string(entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file")));
			//if (defaultCabinetsIt != defaultCabinetNames.end())
			bIsDefault = false;
			for (i = 0; i < max; i++)
			{
				bIsDefault = IsFileEqual(entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), defaultCabinetNames[i]);
				if (bIsDefault)
					break;
			}

			if ( bIsDefault )
			{
				//DevMsg("Cabinet file: %s\n", entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"));
				fileName = entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
				found = fileName.find_last_of(".");
				fileName = fileName.substr(0, found);
				found = fileName.find_last_of("/\\");
				fileName = fileName.substr(found + 1);

				it->second->SaveToFile(g_pFullFileSystem, VarArgs("defaultLibrary/cabinets/%s.txt", fileName.c_str()), "DEFAULT_WRITE_PATH");
			}
		}

		it++;
	}

	// MODELS
	std::vector<std::string> defaultModelNames;
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_n.mdl");
	defaultModelNames.push_back("models/de_vegas/service_trolly.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_g.mdl");
	defaultModelNames.push_back("models/de_vegas/sith_sphynx.mdl");
	defaultModelNames.push_back("models/props/sithlord/floorprojector.mdl");
	defaultModelNames.push_back("models/props/sithlord/table.mdl");
	defaultModelNames.push_back("models/sithlord/giftbox.mdl");
	defaultModelNames.push_back("models/props/sithlord/longbar.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_u.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_j.mdl");
	defaultModelNames.push_back("models/de_halloween/tombstone.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_c.mdl");
	defaultModelNames.push_back("models/de_vegas/cash_cart.mdl");
	defaultModelNames.push_back("models/props/sithlord/walltubelight_rainbow.mdl");
	defaultModelNames.push_back("models/de_halloween/jacklight.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_x.mdl");
	defaultModelNames.push_back("models/sithlord/xmasbell.mdl");
	defaultModelNames.push_back("models/props/sithlord/colorrectangle.mdl");
	defaultModelNames.push_back("models/props/sithlord/lightstrobe.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_q.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_f.mdl");
	defaultModelNames.push_back("models/de_vegas/card_table.mdl");
	defaultModelNames.push_back("models/de_halloween/spooky_tree.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_o.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_t.mdl");
	defaultModelNames.push_back("models/last_resort/villa_chair.mdl");
	defaultModelNames.push_back("models/props/sithlord/lightsyrin.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_w.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_e.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_i.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_m.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_y.mdl");
	defaultModelNames.push_back("models/props/stormy/floorsign_games.mdl");
	defaultModelNames.push_back("models/last_resort/villa_couch.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_k.mdl");
	defaultModelNames.push_back("models/sithlord/candycane.mdl");
	defaultModelNames.push_back("models/cabinets/room_divider.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_a.mdl");
	defaultModelNames.push_back("models/props/sithlord/walltubelight.mdl");
	defaultModelNames.push_back("models/props/sithlord/wood_shelf.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_s.mdl");
	defaultModelNames.push_back("models/cabinets/newton_toy.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_b.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_p.mdl");
	defaultModelNames.push_back("models/props/sithlord/ceilingprojector.mdl");
	defaultModelNames.push_back("models/props/sithlord/studiolight_floor_alwayson.mdl");
	defaultModelNames.push_back("models/de_vegas/roulette_light.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_l.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_z.mdl");
	defaultModelNames.push_back("models/sithlord/xmastree.mdl");
	defaultModelNames.push_back("models/props/sithlord/colorcube.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_h.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_v.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_d.mdl");
	defaultModelNames.push_back("models/props/stormy/neon_alphabet/flurolight_r.mdl");
	defaultModelNames.push_back("models/props/sithlord/colorsquare.mdl");
	max = defaultModelNames.size();

	//std::vector<std::string>::iterator defaultModelsIt;
	std::map<std::string, KeyValues*>& models = g_pAnarchyManager->GetMetaverseManager()->GetAllModels();
	it = models.begin();
	while (it != models.end())
	{
		entry = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		//defaultModelsIt = std::find(defaultModelNames.begin(), defaultModelNames.end(), std::string(entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file")));
		//if (defaultModelsIt != defaultModelNames.end())
		bIsDefault = false;
		for (i = 0; i < max; i++)
		{
			bIsDefault = IsFileEqual(entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"), defaultModelNames[i]);
			if (bIsDefault)
				break;
		}

		if (bIsDefault)
		{
			//DevMsg("Model file: %s\n", entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file"));
			fileName = entry->GetString("platforms/-KJvcne3IKMZQTaG7lPo/file");
			found = fileName.find_last_of(".");
			fileName = fileName.substr(0, found);
			found = fileName.find_last_of("/\\");
			fileName = fileName.substr(found + 1);

			it->second->SaveToFile(g_pFullFileSystem, VarArgs("defaultLibrary/models/%s.txt", fileName.c_str()), "DEFAULT_WRITE_PATH");
		}

		it++;
	}

	// TYPES
	std::vector<std::string> defaultTypeNames;

	// basic types
	defaultTypeNames.push_back("websites");
	defaultTypeNames.push_back("youtube");
	defaultTypeNames.push_back("images");
	defaultTypeNames.push_back("twitch");
	defaultTypeNames.push_back("videos");
	defaultTypeNames.push_back("cards");
	defaultTypeNames.push_back("pc");
	defaultTypeNames.push_back("movies");
	defaultTypeNames.push_back("tv");
	defaultTypeNames.push_back("comics");
	defaultTypeNames.push_back("music");
	defaultTypeNames.push_back("books");
	defaultTypeNames.push_back("maps");
	defaultTypeNames.push_back("other");	// probably not a real time.  this is probably empty type.

	// retro types
	defaultTypeNames.push_back("wii");
	defaultTypeNames.push_back("gba");
	defaultTypeNames.push_back("32x");
	defaultTypeNames.push_back("n64");
	defaultTypeNames.push_back("snes");
	defaultTypeNames.push_back("ds");
	defaultTypeNames.push_back("3ds");
	defaultTypeNames.push_back("gameboy");
	defaultTypeNames.push_back("genesis");
	defaultTypeNames.push_back("gamecube");
	defaultTypeNames.push_back("arcade");
	defaultTypeNames.push_back("ps");
	defaultTypeNames.push_back("ps2");
	defaultTypeNames.push_back("ps3");
	defaultTypeNames.push_back("ps4");
	defaultTypeNames.push_back("megadrive");
	defaultTypeNames.push_back("nes");
	defaultTypeNames.push_back("gamegear");
	defaultTypeNames.push_back("wiiu");
	defaultTypeNames.push_back("switch");
	defaultTypeNames.push_back("atari5200");
	defaultTypeNames.push_back("gbc");
	defaultTypeNames.push_back("psp");
	defaultTypeNames.push_back("sms");
	defaultTypeNames.push_back("3do");
	defaultTypeNames.push_back("pinball");
	defaultTypeNames.push_back("neogeo");

	std::vector<std::string>::iterator defaultTypesIt;
	std::map<std::string, KeyValues*>& types = g_pAnarchyManager->GetMetaverseManager()->GetAllTypes();
	it = types.begin();
	while (it != types.end())
	{
		entry = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		defaultTypesIt = std::find(defaultTypeNames.begin(), defaultTypeNames.end(), std::string(entry->GetString("title")));
		if (defaultTypesIt != defaultTypeNames.end())
		{
			//DevMsg("Type title: %s\n", entry->GetString("title"));
			fileName = entry->GetString("title");

			it->second->SaveToFile(g_pFullFileSystem, VarArgs("defaultLibrary/types/%s.txt", fileName.c_str()), "DEFAULT_WRITE_PATH");
		}

		it++;
	}

	// APPS
	std::vector<std::string> defaultAppNames;

	// retro apps
	//defaultAppNames.push_back("BAM");
	defaultAppNames.push_back("DeSmuME");
	defaultAppNames.push_back("Dolphin");
	defaultAppNames.push_back("ePSXe");
	defaultAppNames.push_back("FCEUX");
	defaultAppNames.push_back("Fusion");
	defaultAppNames.push_back("JNES");
	defaultAppNames.push_back("Kawaks");
	defaultAppNames.push_back("MAME");
	//defaultAppNames.push_back("MESS");
	defaultAppNames.push_back("PCSX2");
	defaultAppNames.push_back("PPSSPP");
	defaultAppNames.push_back("Project64");
	defaultAppNames.push_back("Snes9x");
	////defaultAppNames.push_back("SSF");	// not supported yet until helper apps are officially supported
	defaultAppNames.push_back("VisualBoyAdvance");
	defaultAppNames.push_back("VPinball");
	//defaultAppNames.push_back("VPinbqall8");
	defaultAppNames.push_back("ZSNES");	

	std::vector<std::string>::iterator defaultAppsIt;
	std::map<std::string, KeyValues*>& apps = g_pAnarchyManager->GetMetaverseManager()->GetAllApps();
	it = apps.begin();
	while (it != apps.end())
	{
		entry = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(it->second);
		defaultAppsIt = std::find(defaultAppNames.begin(), defaultAppNames.end(), std::string(entry->GetString("title")));
		if (defaultAppsIt != defaultAppNames.end())
		{
			DevMsg("App title: %s\n", entry->GetString("title"));
			fileName = entry->GetString("title");

			it->second->SaveToFile(g_pFullFileSystem, VarArgs("defaultLibrary/apps/%s.txt", fileName.c_str()), "DEFAULT_WRITE_PATH");
		}

		it++;
	}

	*/

	/*
	// broken & not working.
	CMatRenderContextPtr pRenderContext(materials);

	int x, y, w, h;
	pRenderContext->GetViewport(x, y, w, h);
	//pRenderContext = materials->GetRenderContext();

	//ITexture *pRtFullFrame = NULL;
	//pRtFullFrame = materials->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);

	Rect_t rect;
	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;
	//TEXTURE_GROUP_RENDER_TARGET
	ITexture* pTexture = g_pMaterialSystem->CreateProceduralTexture("TesterTexture", TEXTURE_GROUP_VGUI, w, h, IMAGE_FORMAT_BGR888, 1);

	pRenderContext->CopyRenderTargetToTextureEx(pTexture, 0, &rect, &rect);
	unsigned int width = pTexture->GetActualWidth();
	DevMsg("Text dev is: %i\n", width);
	*/

	// Get pointer to FullFrameFB
//	ITexture *pRtFullFrame = NULL;
	//pRtFullFrame = materials->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);

	//if (pRtFullFrame)
	//{
		//pRtFullFrame->Sa
		//DevMsg("Saving FB to file...\n");
		//pRtFullFrame->GetResourceData()
		//pRtFullFrame->SaveToFile("materials/tester/test.vtf");
		//pRtFullFrame->
	//}
		//pRtFullFrame->SaveToFile(VarArgs("materials/tester/test.vtf))

	//ITexture* pTexture = 
	//pRenderContext->CopyRenderTargetToTexture()

	// NEW TEST

	//webviewinput->Create();
	//DevMsg("Planel created.\n");

	//g_pAnarchyManager->TestSQLite();

	/*
	DevMsg("Setting url to overlay test...\n");
	C_AwesomiumBrowserInstance* pHudInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	pHudInstance->SetUrl("asset://ui/cabinetSelect.html");
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, null, true);
	*/

	//system("Arcade_Launcher.bat");

	//C_OpenGLManager* pOpenGLManager = new C_OpenGLManager();
	//pOpenGLManager->Init();





	/*
	// Scan user profile.
	// 1. Activate input mode.
	// 2. Navigate to the user's games list on their Steam profile in the in-game Steamworks browser.
	// 3. Notify & instruct the user if their profile is set to private, otherwise have an "IMPORT" button appear.
	// 4. Import all games from their list into a KeyValues file ownedGames.key
	// 5. Load all entries from ownedGames.key as items, but do not automatically save them out until the user modifies them.

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->DeselectEntity("asset://ui/blank.html");
	else
		pHudBrowserInstance->SetUrl("asset://ui/blank.html");
	
	CSteamID sid = steamapicontext->SteamUser()->GetSteamID();
	std::string profileUrl = "http://www.steamcommunity.com/profiles/" + std::string(VarArgs("%llu", sid.ConvertToUint64())) + "/games/?tab=all";

	C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->CreateSteamBrowserInstance();
	pSteamBrowserInstance->SetActiveScraper("importSteamGames", "", "");
	pSteamBrowserInstance->Init("", profileUrl, null);
	pSteamBrowserInstance->Focus();
	pSteamBrowserInstance->Select();
	g_pAnarchyManager->GetInputManager()->SetEmbeddedInstance(pSteamBrowserInstance);
	g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, true, pSteamBrowserInstance);
	*/

	// GOOD STEAM GAMES IMPOOOOOOORT!!
	//g_pAnarchyManager->BeginImportSteamGames();

	/*
	KeyValues* kv = new KeyValues("tester");
	kv->SetString("val", "Will it work? xxxxxx");

	// NOTE: The Source filesystem will try to auto-lowercase file names!!!
	// So the file must be created case-correct outside of the Source filesystem and already exist before the Source filesystem writes to it.
	kv->SaveToFile(g_pFullFileSystem, "tEsTeR.key", "DEFAULT_WRITE_PATH");
	*/


	/*
	CUtlBuffer buf;
	KeyValues* pObjectKV = new KeyValues("originalTester");//pInstanceObjectsKV->FindKey(VarArgs("%s/local", objectId.c_str()), true);
	pObjectKV->SetString("originalTesterKey", "yup");
	pObjectKV->SetString("originalTesterKey2", "yup2");
	pObjectKV->SetString("originalTesterKey3", "yup3");
	pObjectKV->WriteAsBinary(buf);
	pObjectKV->deleteThis();

	int size = buf.Size();
	DevMsg("Buffer size here is: %i\n", size);
	void* mem = malloc(size);
	Q_memcpy(mem, buf.Base(), size);

	CUtlBuffer buf2(0, size, 0);
	buf2.CopyBuffer(mem, size);
	int size2 = buf2.Size();
	DevMsg("Processed buffer size is: %i\n", size2);
	
	KeyValues* pTesterKV = new KeyValues("reduxTester");
	pTesterKV->ReadAsBinary(buf2);
	DevMsg("Annd here the big result is: %s\n", pTesterKV->GetString("originalTesterKey"));
	pTesterKV->deleteThis();
	*/
}
ConCommand test_function( "testfunc", TestFunction, "Usage: executes an arbitrary hard-coded C++ routine" );

void TestFunctionOff(const CCommand &args)
{
	//g_pAnarchyManager->TestSQLite2();



	////g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
	////g_pAnarchyManager->GetHUDManager
}
ConCommand test_function_off("testfunc2", TestFunctionOff, "Usage: executes an arbitrary hard-coded C++ routine");

void APIObjectCreated(const CCommand &args)
{
	std::string sessionId = args[1];
	std::string objectId = args[2];
	int iEntityIndex = Q_atoi(args[3]);
	int iParentEntityIndex = Q_atoi(args[4]);

	C_SteamBrowserInstance* pBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstance(sessionId);
	if (!pBrowserInstance)
		return;

	pBrowserInstance->OnAPIObjectCreated(sessionId, objectId, iEntityIndex, iParentEntityIndex);
}
ConCommand apiobjectcreated("api_object_created", APIObjectCreated, "Usage:");

//#include "vfw.h"
#include "../clientsideeffects.h"
#define OGT_VOX_IMPLEMENTATION
#include "../server/ogt_vox.h"
void CreateVoxel(Vector origin, float flScale, Color color)
{
	// Now spawn it
	FX_AddCenteredCube(origin, flScale, Vector(color.r(), color.g(), color.b()), 10.0f, "stormy/da_metal_oldtrusty");
}

void ReallySpawnVoxels(C_BaseEntity* pParentEntity, std::string voxfile)
{
	float flVoxelSize = 4.0f;
	float flBaseScale = 1.0f;
	Vector playerOrigin = pParentEntity->GetAbsOrigin();


	// LOAD THE VOX INTO MEMORY BUFFER
	FileHandle_t fh = filesystem->Open(voxfile.c_str(), "rb", "GAME");
	if (!fh)
		return;

	int file_len = filesystem->Size(fh);

	/* Changed this known working unsigned char* code to be uint8_t* to match the OGT Vox code.
	unsigned char* pVoxelData = new unsigned char[file_len + 1];
	filesystem->Read((void*)pVoxelData, file_len, fh);
	*/

	uint8_t* buffer = new uint8_t[file_len + 1];
	filesystem->Read((void*)buffer, file_len, fh);

	buffer[file_len] = 0; // null terminator
	filesystem->Close(fh);

	// PARSE THE MEMORY BUFFER INTO A VOX OBJECT
	const ogt_vox_scene* scene = ogt_vox_read_scene(buffer, file_len);
	// the buffer can be safely deleted once the scene is instantiated.
	delete[] buffer;

	if (!scene)
		return;

	DevMsg("# models: %u\n", scene->num_models);
	DevMsg("# instances: %u\n", scene->num_instances);
	DevMsg("# of layers: %u\n", scene->num_layers);
	DevMsg("# of groups: %u\n", scene->num_groups);

	// TODO: work...

	// PROCESS A MODEL
	// Then, time to display a set of voxels somewhere...
	if (scene->num_models < 1)
		return;

	const ogt_vox_model* pVoxModel = scene->models[0];
	DevMsg("\tModel Size: %ux%ux%u\n", pVoxModel->size_x, pVoxModel->size_y, pVoxModel->size_z);

	// PROCESS A MODEL'S DATA
	// Step through the planes of the voxel grid, grabbing info about each voxel.

	Vector voxelOrigin;
	QAngle voxelAngles;
	voxelAngles.Init();

	int iHalfX, iHalfY, iHalfZ;

	iHalfX = pVoxModel->size_x / 2;
	iHalfY = pVoxModel->size_y / 2;
	iHalfZ = pVoxModel->size_z / 2;

	unsigned int iVoxelIndex;
	uint8_t color_index;
	ogt_vox_rgba color;
	int x, y, z;
	for (z = 0; z < pVoxModel->size_z; ++z)
	{
		for (y = 0; y < pVoxModel->size_y; ++y)
		{
			for (x = 0; x < pVoxModel->size_x; ++x)
			{
				iVoxelIndex = x + (y * pVoxModel->size_x) + (z * pVoxModel->size_x * pVoxModel->size_y);
				color_index = pVoxModel->voxel_data[iVoxelIndex];

				if (color_index != 0)
				{
					color = scene->palette.color[color_index];
					DevMsg("Voxel Color: %u %u %u\n", color.r, color.g, color.b, color.a);

					// spawn a voxel.
					// FIXME: Move this into the mesh builder, so that each VOX cluster is only 1 entity instead of many.
					// TODO: figure out this voxel's origin relative to its hierarchy - including AArcade-parent-object-space and AArcade-world-space.
					//voxelOrigin = ?;

					// The grid gets centered on all 3 axes.
					voxelOrigin.Init();
					voxelOrigin.x = (x - iHalfX) * flVoxelSize;
					voxelOrigin.y = (y - iHalfY) * flVoxelSize;
					voxelOrigin.z = (z - iHalfZ) * flVoxelSize;

					voxelOrigin *= flBaseScale;

					voxelAngles.Init();
					VMatrix childMatrix;
					childMatrix.SetupMatrixOrgAngles(voxelOrigin, voxelAngles);

					VMatrix composedMatrix;
					QAngle fakeAngles;
					fakeAngles.Init();
					composedMatrix.SetupMatrixOrgAngles(pParentEntity->GetAbsOrigin(), fakeAngles);//pParentEntity->GetAbsAngles()
					composedMatrix = composedMatrix * childMatrix;

					// back to vecs & angles
					MatrixAngles(composedMatrix.As3x4(), voxelAngles, voxelOrigin);

					//CreateVoxel(dynamic_cast<CDynamicProp*>(pParentEntity), voxelOrigin, voxelAngles, flBaseScale, Color(color.r, color.g, color.b, color.a));

					CreateVoxel(voxelOrigin, (flVoxelSize * flBaseScale)/2.0, Color(color.r, color.g, color.b, color.a));
				}
			}
		}
	}

	ogt_vox_destroy_scene(scene);

	DevMsg("fin\n");
}

//ConVar did_use_quest_object("did_use_quest_object", "0", FCVAR_REPLICATED | FCVAR_HIDDEN, "Internal use only.");
void OnPlayerUse(const CCommand &args)
{
	g_pAnarchyManager->GetQuestManager()->OnPlayerUse();
}
ConCommand onplayeruse("on_player_use", OnPlayerUse, "Usage: ", FCVAR_HIDDEN);

void SteamSpeak(const CCommand &args)
{
	int iArgCount = args.ArgC();
	if (iArgCount <= 1)
		return;

	std::string text = args[1];
	std::string voice = (iArgCount > 2) ? args[2] : "US English Female";
	float flPitch = (iArgCount > 3) ? Q_atof(args[3]) : 1.0;
	float flRate = (iArgCount > 4) ? Q_atof(args[4]) : 1.0;
	float flVolume = (iArgCount > 5) ? Q_atof(args[5]) : 0.9;

	g_pAnarchyManager->SteamTalker(text, voice, flPitch, flRate, flVolume);
}
ConCommand ttsspeak("ttsspeak", SteamSpeak, "Usage: [text] [voice] [pitch] [rate] [volume]", FCVAR_NONE);

void TestSpawnVox(const CCommand &args)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	std::string voxfile = "chr_knight.vox";
	if (args.ArgC() > 1)
	{
		voxfile = args[1];
		if (voxfile.find(".vox") == std::string::npos && voxfile.find(".VOX") == std::string::npos)
			voxfile += ".vox";
	}

	ReallySpawnVoxels(pPlayer, voxfile);

}
ConCommand testspawnvox("test_spawn_vox", TestSpawnVox, "Spawn a VOX file that is inside your aarcade_user folder. If you do not give a filename, it will look for one called chr_knight.vox. Usage: test_spawn_vox [filename]", FCVAR_NONE);

/*void ToggleAlwaysAnimatingImages(const CCommand &args)
{
	if ( )
}
ConCommand togglealwaysanimatingimages("toggle_always_animating_images", ToggleAlwaysAnimatingImages, "Toggles off / on animated images. You may have to reload the map to fully turn off the UV atlas effect.", FCVAR_NONE);*/

void RestartQuestSystem(const CCommand &args)
{
	g_pAnarchyManager->GetQuestManager()->RestartQuestSystem();
}
ConCommand restartquestsystem("restart_quest_system", RestartQuestSystem, "Resets & restarts the available quests in this world.", FCVAR_NONE);

void ApplyCarryData(const CCommand &args)
{
	std::string originOffset = args[1];
	g_pAnarchyManager->ApplyCarryData(originOffset);
}
ConCommand applyCarryData("apply_carry_cata", ApplyCarryData, "Internal. Applies an override to the carry origin offset for inspect mode.", FCVAR_HIDDEN); // IFXME: There is a type, it says cata. this con function is most likely OBSOLETE!


#include "../openvr/openvr.h"
void TesterJoint(const CCommand &args)
{
	//g_pAnarchyManager->GetQuestManager()->RestartQuestSystem();

	//systemTimeState_t systemTimeState = g_pAnarchyManager->GetSystemTimeState();
	//DevMsg("Day %i / Date %i / Month %i / Year %i / Hour %i / Minutes %i / Seconds %i / Postfix %i\n", systemTimeState.iDay, systemTimeState.iDate, systemTimeState.iMonths, systemTimeState.iYears, systemTimeState.iHours, systemTimeState.iMinutes, systemTimeState.iSeconds, systemTimeState.iPostfix);

	// time running maybe?
	//long lTime = vgui::system()->GetTimeMillis();
	//DevMsg("Time: %ld\n", lTime);

	/*
	// system time
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);
	
	int iDay = now->tm_wday;
	int iDate = now->tm_mday;
	int iMonth = now->tm_mon;
	int iYear = 1900 + now->tm_year;
	int iPostfix = 0;
	int iHour = now->tm_hour;
	if (iHour > 11)
	{
		iPostfix = 1;
		iHour = iHour - 12;
	}

	int iMinutes = now->tm_min;
	int iSeconds = now->tm_sec;

	DevMsg("Day %i / Date %i / Month %i / Year %i / Hour %i / Minutes %i / Seconds %i / Postfix %i\n", iDay, iDate, iMonth, iYear, iHour, iMinutes, iSeconds, iPostfix);
	*/

	/*
	DevMsg("VR System Version: %s\n", vr::IVRSystem_Version);

	//vr::EVRInputError err = vr::VRInput()->SetActionManifestPath("D:\Projects\AArcade-Source\game\frontend\inputsystem\actions.json");
	//DevMsg("VR System Response: %i\n", (int)err);

	vr::HmdError vrError;
	vr::EVRApplicationType vrAppType = vr::VRApplication_Scene;
	vr::IVRSystem* vrSystem = vr::VR_Init(&vrError, vrAppType);
	
	DevMsg("VR System Error: %i\n", (int)vrError);

	//DevMsg("Is HMD Connected via SteamVR API: %i\n", vr::present)

	// IMPORTANT: vr::VR_Shutdown() at some point down the road.
	*/

	/*
	//bool bIsHMDTracked = vrSystem->IsTrackedDeviceConnected(0);
	bool bIsDevice1Tracked = vrSystem->IsTrackedDeviceConnected(1);
	//bool bIsDevice2Tracked = vrSystem->IsTrackedDeviceConnected(2);
	//bool bIsDevice3Tracked = vrSystem->IsTrackedDeviceConnected(3);
	//DevMsg("Device tracking: %i/%i/%i/%i\n", bIsHMDTracked, bIsDevice1Tracked, bIsDevice2Tracked, bIsDevice3Tracked);

	//vr::VRControllerAxis_t 
	vr::VRControllerState_t pConstrollerState;
	if (!vrSystem->GetControllerState(1, &pConstrollerState, sizeof(vr::VRControllerState_t)))
	{
		DevMsg("Failed to get constroller state!\n");
		return;
	}

	DevMsg("Button Code: %u\n", pConstrollerState.ulButtonPressed);
	*/

	// WORKING VOX SPAWNER!
	/*
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	std::string voxfile = "chr_knight.vox";
	if (args.ArgC() > 1)
	{
		voxfile = args[1];
		if (voxfile.find(".vox") == std::string::npos && voxfile.find(".VOX") == std::string::npos)
			voxfile += ".vox";
	}

	ReallySpawnVoxels(pPlayer, voxfile);
	*/



	/* Gets a device (when is linked and vfw.h is included) but can't do shit with it and isn't my webam.
	char szDeviceName[80];
	char szDeviceVersion[80];

	for (unsigned int wIndex = 0; wIndex < 10; wIndex++)
	{
		if (capGetDriverDescription(
			wIndex,
			szDeviceName,
			sizeof(szDeviceName),
			szDeviceVersion,
			sizeof(szDeviceVersion)
			))
		{
			// Append name to list of installed capture drivers
			// and then let the user select a driver to use.
			DevMsg("Capture Device: %s\n", szDeviceName);
		}
	}
	*/
	//if (!BlockInput(true))
	//	BlockInput(false);
	//return;

	//g_pAnarchyManager->GetMetaverseManager()->OnAssetUploadBatchReady();
	//std::string file = "models/striker/nicebongstriker.mdl";
	//g_pAnarchyManager->GetMetaverseManager()->RequestAsset(file);


	/*if (g_pVGuiSystem->CommandLineParamExists("-vr"))
	{
		char value[1024];
		g_pVGuiSystem->GetCommandLineParamValue("-vr", &value[0], 1024);
		value[1024] = '\n';
		DevMsg("Exists: %s\n", value);
	}
	else
		DevMsg("Does not exist.\n");*/

	// WORKS: Capture a non-3d accelerated window
	//g_pAnarchyManager->CaptureWindowSnapshotsAll();


	/* WORKS: Save mounted paths to a txt file
	char rawPaths[20480];	// 1024*20
	g_pFullFileSystem->GetSearchPath("game", false, rawPaths, 20480);

	std::string paths = rawPaths;

	KeyValues* pPathsKV = new KeyValues("SearchPaths");
	KeyValues* pPathKV;
	std::vector<std::string> pathTokens;
	g_pAnarchyManager->Tokenize(paths, pathTokens, ";");
	for (unsigned int i = 0; i < pathTokens.size(); i++)
	{
		pPathKV = pPathsKV->CreateNewKey();
		pPathKV->SetName("game");
		pPathKV->SetString("", pathTokens[i].c_str());
	}
	pPathsKV->SaveToFile(g_pFullFileSystem, "gameinfo_paths.txt", "DEFAULT_WRITE_PATH");
	pPathsKV->deleteThis();
	DevMsg("Saved to gameinfo_paths.txt!\n");
	*/

/*
	DevMsg("Hello world.\n");
	DevMsg("Downloading image...\n");
	//https://images.igdb.com/igdb/image/upload/t_cover_big/co2kyg.png
	g_pAnarchyManager->DownloadSingleFile("https://images.igdb.com/igdb/image/upload/t_cover_big/co2kyg.png");
	//g_pAnarchyManager->DownloadSingleFile("https://m3org.com/");
	*/

	//g_pAnarchyManager->TestVRStuff();


/*

	std::string toolsFolder = g_pAnarchyManager->GetAArcadeToolsFolder();
	std::string userFolder = g_pAnarchyManager->GetAArcadeUserFolder();

	FileHandle_t launch_file = filesystem->Open("Arcade_Launcher.bat", "w", "EXECUTABLE_PATH");
	if (launch_file)
	{
		std::string executable = VarArgs("%s\\vtf2tga.exe", toolsFolder.c_str());
		std::string goodExecutable = "\"" + executable + "\"";
		filesystem->FPrintf(launch_file, "%s:\n", goodExecutable.substr(1, 1).c_str());
		filesystem->FPrintf(launch_file, "cd \"%s\"\n", goodExecutable.substr(1, goodExecutable.find_last_of("/\\", goodExecutable.find("\"", 1)) - 1).c_str());
		filesystem->FPrintf(launch_file, "START \"Launching VTEX...\" %s -i \"%s\\cache\\textures\\temp.vtf\" -o \"%s\\cache\\textures\\%s.tga\"", goodExecutable.c_str(), userFolder.c_str(), userFolder.c_str(), textureId.c_str());
		filesystem->Close(launch_file);
		system("Arcade_Launcher.bat");
		return textureId;
	}
*/

	// WORKING (i think) implementation of modifying an MDL **file** to use a different material. Literally creating a diffrent/new asset & saving it to disk.
/*
	std::string matName = args[1];
	std::string varName = args[2];
	std::string mdlName = args[3];
	std::string vtfName = args[4];
	C_AITests* pAITests = new C_AITests();
	//pAITests->CloneModelMaterialAndApplyTexture(matName.c_str(), varName.c_str(), mdlName.c_str(), vtfName.c_str());
	//std::string cloneddynvtfscreen = "models/automats/cloneddynvtfscreen";
	std::string cloneddynvtfscreen = "models/automats/cloneddynvtfscreen.mdl";
	//pAITests->CloneModelAndApplyMaterial(mdlName.c_str(), cloneddynvtfscreen.c_str());
	pAITests->ChangeModelInternalNameAndSave(mdlName.c_str(), cloneddynvtfscreen.c_str(), "automats/cloneddynvtfscreen");
	delete pAITests;
*/
	//g_pAnarchyManager->GetMetaverseManager()->TesterJoint();
//DevMsg("Client values for:\n\tMAX_EDICT_BITS\t%i\n\tMAX_EDICTS\t%i\n\tNUM_ENT_ENTRY_BITS\t%i\n\tNUM_ENT_ENTRIES\t%i\n", MAX_EDICT_BITS, MAX_EDICTS, NUM_ENT_ENTRY_BITS, NUM_ENT_ENTRIES);

	//g_pAnarchyManager->GetInstanceManager()->ProcessAutoUnspawnUpdate();	// This object unspawning system ACTUALLY WORKS!!!!!! But there is such a performance stutter loading in new assets still, that it needs to find a proper implementation.  (Also note stuff like item texture cleanup is probably not implemented, it's still an unfinished feature.)





/*
	//C_BaseEntity* pPet = null;
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	//if (pPlayAsPet)
		//pPet = C_BaseEntity::Instance(pPlayAsPet->iEntityIndex);

	if (pPet)
	{
		C_PropShortcutEntity* pShortcut = NULL;
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
		if (pBaseEntity)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);

		if (!pShortcut)
		{
			float flMaxRange = -1;
			object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
			if (pObject->spawned)
			{
				pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
				if (pBaseEntity)
					pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			}
		}

		if (pShortcut)
		{
			//std::string attachment_name = "head";
			std::string attachment_name = "rhand";
			//int iAttachment = pPet->LookupAttachment(attachment_name.c_str());
			engine->ClientCmd(VarArgs("setpetattach %i %i \"%s\";\n", pShortcut->entindex(), pPet->iEntityIndex, attachment_name.c_str()));
			//engine->ServerCmd(VarArgs("snap_object_pos %i %f %f %f %f %f %f;\n", pShortcut->entindex(), origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
		}
	}
	*/

	g_pAnarchyManager->DownloadSingleFile("https://akns-images.eonline.com/eol_images/Entire_Site/202414/rs_1200x1200-240204170033-1200-taylor-swift-2024-grammys.jpg");
}
ConCommand testerjoint("testerjoint", TesterJoint, "Usage: ");

void TesterJoint22(const CCommand &args)
{
	std::vector<std::string> results;
	g_pAnarchyManager->GetMountManager()->DetectGamePaths(results);
	for (unsigned int i = 0; i < results.size(); i++)
	{
		DevMsg("\tDetected path: %s\n", results[i].c_str());
	}
}
ConCommand testerjoint22("testerjoint22", TesterJoint22, "Usage: ");

// Console command to display the number of mount paths
void CC_CountMountPaths()
{
	// Declare a buffer to hold the search paths
	const int bufferSize = 4096 * 20;  // Adjust the size as needed 
	//					  17995
	char searchPaths[bufferSize];

	// Retrieve the search paths (search paths are separated by a semicolon)
	g_pFullFileSystem->GetSearchPath(nullptr, false, searchPaths, bufferSize);

	// Split the search paths by semicolons
	CUtlVector<char*> pathList;
	char* token = strtok(searchPaths, ";");
	while (token != nullptr)
	{
		pathList.AddToTail(token);
		token = strtok(nullptr, ";");
	}

	// Output the number of mount paths
	Msg("Number of mount paths: %d\n", pathList.Count());

	// Optionally, print each mount path
	for (int i = 0; i < pathList.Count(); i++)
	{
		Msg("Mount path %d: %s\n", i + 1, pathList[i]);
	}
}

// Register the console command
static ConCommand count_mount_paths("count_mount_paths", CC_CountMountPaths, "Outputs the number of currently loaded mount paths", FCVAR_CHEAT);

void _PetAddAttachment(pet_t* pPet, const std::string& attachmentName, const std::string& attachmentModelFilename) {
	pPet->attachments[attachmentName].push_back(attachmentModelFilename);
}

C_BaseEntity* HasChildModel(C_DynamicProp* pParent, std::string modelName)
{
	if (!pParent || modelName == "")
		return null;

	// Get the first child of the parent entity
	C_BaseEntity* pChild = pParent->FirstMoveChild();
	while (pChild)
	{
		const model_t* pModel = pChild->GetModel();
		if (pModel)
		{
			std::string attachmentModelFilename = modelinfo->GetModelName(pModel);	// this has nothing to do with attachment. bad var names.
			attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(attachmentModelFilename);

			if (attachmentModelFilename == modelName)
			{
				return pChild; // Found a child with the matching model
			}
		}

		// Move to the next sibling
		pChild = pChild->NextMovePeer();
	}

	return null; // No child with the specified model found
}

void _PetRemoveAttachedModel(pet_t* pPet, const std::string& attachmentName, const std::string& attachmentModelFilename) {
	if (attachmentName.empty()) {
		// If attachmentName is blank, remove the model filename from all vectors
		for (auto& pair : pPet->attachments) {
			// Get the vector of filenames
			std::vector<std::string>& filenames = pair.second;

			// Remove the specified model filename from the vector
			auto newEnd = std::remove(filenames.begin(), filenames.end(), attachmentModelFilename);

			// Resize the vector to remove the undefined elements
			filenames.erase(newEnd, filenames.end());

			// Optional: Remove the key if no attachments are left
			//if (filenames.empty()) {
			//	pPet->attachments.erase(pair.first);
			//}
		}
	}
	else {
		// Search for the attachment name in the map
		auto it = pPet->attachments.find(attachmentName);

		if (it != pPet->attachments.end()) {
			// Get the vector of filenames
			std::vector<std::string>& filenames = it->second;

			// Remove the specified model filename from the vector
			auto newEnd = std::remove(filenames.begin(), filenames.end(), attachmentModelFilename);

			// Resize the vector to remove the undefined elements
			filenames.erase(newEnd, filenames.end());

			// Optional: Remove the key if no attachments are left
			//if (filenames.empty()) {
			//	pPet->attachments.erase(it);
			//}
		}
		else {
			//std::cout << "Attachment name not found." << std::endl;
		}
	}
}

/*void _PetRemoveAllAttachments(bool bAlsoRemoveEntities) {
	C_BaseEntity* pPetEntity = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	if (pPetEntity)
	{
		int iPetEntityIndex = pPetEntity->entindex();
		pet_t* pPet = g_pAnarchyManager->GetPetByEntIndex(iPetEntityIndex);

		// Iterate over each attachment name
		for (auto& pair : pPet->attachments) {
			// Reverse iterate over the vector of filenames
			for (auto rit = pair.second.rbegin(); rit != pair.second.rend(); ++rit) {
				// Call removeAttachment for each filename, using reverse iterators to avoid affecting iteration

				if (bAlsoRemoveEntities)
				{
					bool bDidRemove = false;
					C_DynamicProp* pDPet = dynamic_cast<C_DynamicProp*>(pPetEntity);
					C_BaseEntity* pExistingProp = HasChildModel(pDPet, *rit);
					while (pExistingProp)
					{
						// remove it...
						bDidRemove = true;
						engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
						DevMsg("Removed existing attached prop.\n");
						pExistingProp = NULL;// HasChildModel(pDPet, attachmentModelFilename);// removing objects is not syncronous. :(
					}
				}

				_PetRemoveAttachedModel(pPet, pair.first, *rit);
			}
		}
	}
}*/



void TestDoPetAttach(const CCommand &args)
{
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		std::string attachment_name = (args.ArgC() > 1) ? args[1] : "head";

		//C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();

		C_PropShortcutEntity* pShortcut = NULL;
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
		if (pBaseEntity)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);

		if (!pShortcut)
		{
			float flMaxRange = -1;
			object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
			if (pObject->spawned)
			{
				pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
				if (pBaseEntity)
					pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			}
		}

		if (pShortcut)
		{
			//std::string attachmentModelFilename = modelinfo->GetModelName(pShortcut->GetModel());
			//attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(attachmentModelFilename);

			/*
			// PLAN B (for now)
			// If model exists ANYWHERE on the model, remove ALL instances of it.
			bool bDidRemove = false;
			C_DynamicProp* pDPet = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(pPet->iEntityIndex));
			C_BaseEntity* pExistingProp = HasChildModel(pDPet, attachmentModelFilename);
			while (pExistingProp)
			{
				// remove it...
				bDidRemove = true;
				engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
				DevMsg("Removed existing attached prop.\n");
				pExistingProp = NULL;// HasChildModel(pDPet, attachmentModelFilename);// removing objects is not syncronous. :(
			}

			// Now remove ALL instances of it from all bookeeping
			if (bDidRemove)
			{
				_PetRemoveAttachedModel(pPet, "", attachmentModelFilename);
			}

			_PetAddAttachment(pPet, attachment_name, attachmentModelFilename);	// bookeeping
			*/

			engine->ClientCmd(VarArgs("testsetpetattach %i %i \"%s\";\n", pShortcut->entindex(), pPet->iEntityIndex, attachment_name.c_str()));
		}
	}
}
ConCommand testdopetattach("testdopetattach", TestDoPetAttach, "TEST Usage: internal use only. Attaches the current model you are moving to your PET avatar. Should only be used from the buildmode menu.", FCVAR_HIDDEN);


void TestPetOutfitStrip(const CCommand &args)
{
	bool bAlsoRemoveEntities = false;
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		pPet->outfitId = "";
		C_DynamicProp* pParent = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(pPet->iEntityIndex));

		std::vector<C_BaseEntity*> victims;

		// Get the first child of the parent entity
		C_BaseEntity* pChild = pParent->FirstMoveChild();
		while (pChild)
		{
			const model_t* pModel = pChild->GetModel();
			if (pModel)
			{
				victims.push_back(pChild);
			}

			// Move to the next sibling
			pChild = pChild->NextMovePeer();
		}

		for (unsigned int i = 0; i < victims.size(); i++ )
		{
			// remove it...
			//engine->ClientCmd(VarArgs("removeobject %i;\n", victims[i]->entindex()));
			engine->ClientCmd(VarArgs("setparent %i;\n", victims[i]->entindex()));
		}
	}
}
ConCommand testpetoutfitstrip("testpetoutfitstrip", TestPetOutfitStrip, "TESTJust for testing for now. Removes all of the attachments on the play-as-pet.", FCVAR_HIDDEN);



void DoPetAttach(const CCommand &args)
{
	//C_BaseEntity* pPetEntity = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();

	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		std::string attachment_name = (args.ArgC() > 1) ? args[1] : "rhand";//"head";
		bool bIsToggle = (args.ArgC() > 2) ? (Q_atoi(args[2]) != 0) : false;

		C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();

		// check if this model is already attached. if it is, then detatch it instead.
		//const model_t* pModel = pShortcut->GetModel();
		//if (pModel)
		if (pShortcut)
		{
			//std::string attachmentModelFilename = modelinfo->GetModelName(pModel);
			//std::string attachmentModelFilename = STRING(pShortcut->GetModelName());
			std::string attachmentModelFilename = modelinfo->GetModelName(pShortcut->GetModel());
			//std::string attachmentModelFilename = STRING(((C_BaseEntity*)pShortcut)->GetModelName());
			attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(attachmentModelFilename);

			// PLAN B (for now)
			// If model exists ANYWHERE on the model, remove ALL instances of it.
			bool bDidRemove = false;
			C_DynamicProp* pDPet = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(pPet->iEntityIndex));
			C_BaseEntity* pExistingProp = HasChildModel(pDPet, attachmentModelFilename);
			while (pExistingProp)
			{
				// remove it...
				bDidRemove = true;
				engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
				DevMsg("Removed existing attached prop.\n");
				pExistingProp = NULL;// HasChildModel(pDPet, attachmentModelFilename);// removing objects is not syncronous. :(
			}

			// Now remove ALL instances of it from all bookeeping
			if (bDidRemove)
			{
				_PetRemoveAttachedModel(pPet, "", attachmentModelFilename);
			}

			if (!bDidRemove || !bIsToggle)
			{
				_PetAddAttachment(pPet, attachment_name, attachmentModelFilename);	// bookeeping

				engine->ClientCmd(VarArgs("setpetattach %i %i \"%s\";\n", pShortcut->entindex(), pPet->iEntityIndex, attachment_name.c_str()));
			}
		}
	}
}
ConCommand dopetattach("dopetattach", DoPetAttach, "Usage: internal use only. Attaches the current model you are moving to your PET avatar. Should only be used from the buildmode menu.", FCVAR_HIDDEN);

KeyValues* getPetAttachment(KeyValues* models, const std::string& petModelFilename, const std::string& attachmentName, const std::string& attachmentModelFilename)
{
	for (KeyValues *model = models->GetFirstSubKey(); model; model = model->GetNextKey())
	{
		if (model->GetString("file") == petModelFilename)
		{
			KeyValues* attachments = model->FindKey("attachments");
			for (KeyValues *attachment = attachments->GetFirstSubKey(); attachment; attachment = attachment->GetNextKey())
			{
				if (attachment->GetString("name") == attachmentName)
				{
					KeyValues* modelsSubKey = attachment->FindKey("models");
					for (KeyValues *modelSub = modelsSubKey->GetFirstSubKey(); modelSub; modelSub = modelSub->GetNextKey())
					{
						if (modelSub->GetString("file") == attachmentModelFilename)
						{
							return modelSub;
						}
					}
				}
			}
		}
	}
	return NULL;
}

std::map<std::string, KeyValues*> findAllPetAttachments(KeyValues* models, const std::string& petModelFilename, const std::string& attachmentModelFilename)
{
	std::map<std::string, KeyValues*> matches;
	for (KeyValues *model = models->GetFirstSubKey(); model; model = model->GetNextKey())
	{
		if (model->GetString("file") == petModelFilename)
		{
			KeyValues* attachments = model->FindKey("attachments");
			for (KeyValues *attachment = attachments->GetFirstSubKey(); attachment; attachment = attachment->GetNextKey())
			{
				KeyValues* modelsSubKey = attachment->FindKey("models");
				for (KeyValues *modelSub = modelsSubKey->GetFirstSubKey(); modelSub; modelSub = modelSub->GetNextKey())
				{
					if (modelSub->GetString("file") == attachmentModelFilename)
					{
						matches[std::string(attachment->GetString("name"))] = modelSub;
					}
				}
			}
		}
	}

	return matches;
}

void PetFlag(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = NULL;
	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
	if (pBaseEntity)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);

	if (!pShortcut)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
		{
			pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
			if (pBaseEntity)
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		}
	}

	if (pShortcut)
	{
		DevMsg("TODO: Write to arcade_user/pet_library.txt\n");
	}
}
ConCommand petflag("petflag", PetFlag, "Flag the object under your crosshair as a pet.");

void DoPetOutfitStrip(pet_t* pPet)
{

	//_PetRemoveAllAttachments(true);
	bool bAlsoRemoveEntities = true;

	//C_BaseEntity* pPetEntity = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	//pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		pPet->outfitId = "";
		C_DynamicProp* pDPet = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(pPet->iEntityIndex));

		// Iterate over each attachment name
		for (auto& pair : pPet->attachments) {
			// Reverse iterate over the vector of filenames
			for (auto rit = pair.second.rbegin(); rit != pair.second.rend(); ++rit) {
				// Call removeAttachment for each filename, using reverse iterators to avoid affecting iteration

				std::string attachmentModelFilename = *rit;
				if (bAlsoRemoveEntities)
				{
					bool bDidRemove = false;
					DevMsg("attachment: %s\n", attachmentModelFilename.c_str());
					C_BaseEntity* pExistingProp = HasChildModel(pDPet, *rit);
					while (pExistingProp)
					{
						// remove it...
						bDidRemove = true;
						engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
						DevMsg("Removed existing attached prop.\n");
						pExistingProp = NULL;// HasChildModel(pDPet, attachmentModelFilename);// removing objects is not syncronous. :(
					}
				}

				_PetRemoveAttachedModel(pPet, pair.first, attachmentModelFilename);
			}
		}
	}
}

void PetOutfitStrip(const CCommand &args)
{
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		DoPetOutfitStrip(pPet);
	}
}
ConCommand petoutfitstrip("petoutfitstrip", PetOutfitStrip, "Just for testing for now. Removes all of the attachments on the play-as-pet.");

void PetOutfitSave(const CCommand &args)
{
	//C_BaseEntity* pPetEntity = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		//const model_t* pPetModel = pPetEntity->GetModel();
		//if (pPetModel)
		//{
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(modelinfo->GetModelName(pPetModel));
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(STRING(pPetEntity->GetModelName()));
			std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(modelinfo->GetModelName(pPetEntity->GetModel()));
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(STRING(((C_BaseEntity*)pPetEntity)->GetModelName()));
			//pet_t* pPet = g_pAnarchyManager->GetPetByEntIndex(pPetEntity->entindex());
			std::string outfitId = (args.ArgC() > 1) ? args[1] : pPet->outfitId;
			// FIXME: Validate outfitId here.
			if (outfitId == "")
			{
				return;
			}

			pPet->outfitId = outfitId;

			//void MapToKeyValues(const std::map<std::string, std::vector<std::string>>& attachments, KeyValues* pRoot) {
			KeyValues* pFile;
			KeyValues* pAttachment;
			KeyValues* pModels;
			KeyValues* pOutfit = new KeyValues("outfit");
			KeyValues* pAttachments = pOutfit->FindKey("attachments", true);
			// Iterate over the map
			for (const auto& pair : pPet->attachments) {
				// Create a new KeyValues node for this attachment name
				pAttachment = pAttachments->CreateNewKey();//new KeyValues(pair.first.c_str());
				pAttachment->SetString("name", pair.first.c_str());
				pModels = pAttachment->FindKey("models", true);
				// Iterate over the vector associated with this attachment name
				for (const std::string& filename : pair.second) {
					// Add each filename as a child KeyValues node under the current attachment node
					pFile = pModels->CreateNewKey();
					pFile->SetString("file", filename.c_str());
				}
			}

			//std::string outfitId = pPet->outfitId;//(args.ArgC() > 1) ? args[1] : pPet->outfitId;
			//if (outfitId == "")
				//outfitId = "default";
			//pPet->outfitId = outfitId;

			std::string petModelHash = g_pAnarchyManager->GenerateLegacyHash(petModelFilename.c_str());
			std::string filename = VarArgs("pets/%s/outfits/%s.txt", petModelHash.c_str(), outfitId.c_str());

			g_pFullFileSystem->CreateDirHierarchy(VarArgs("pets/%s/outfits", petModelHash.c_str()), "DEFAULT_WRITE_PATH");

			if (pOutfit->SaveToFile(g_pFullFileSystem, filename.c_str(), "DEFAULT_WRITE_PATH"))
			{
				DevMsg("Saved %s\n", filename.c_str());
			}
		//}
	}
}
ConCommand petoutfitsave("petoutfitsave", PetOutfitSave, "Saves the outfit on the current play-as-pet. (Generates a new outfitid if one is not given.)");

void PetOutfitDelete(const CCommand&args)
{
	if (args.ArgC() < 2)	// we MUST be given an outfit ID
		return;

	// Get the filename (under DEFAULT_WRITE_PATH) we are removing...
	//C_BaseEntity* pPetEntity = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		//const model_t* pPetModel = pPetEntity->GetModel();
		//if (pPetModel)
		//{
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(modelinfo->GetModelName(pPetModel));
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(STRING(pPetEntity->GetModelName()));
			std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(modelinfo->GetModelName(pPetEntity->GetModel()));
			//std::string petModelFilename = g_pAnarchyManager->NormalizeModelFilename(STRING(((C_BaseEntity*)pPetEntity)->GetModelName()));

			std::string petModelHash = g_pAnarchyManager->GenerateLegacyHash(petModelFilename.c_str());
			std::string outfitId = args[1];

			if (pPet->outfitId == outfitId)
			{
				pPet->outfitId = "";
			}

			std::string petFile = VarArgs("%s/outfits/%s.txt", petModelHash.c_str(), outfitId.c_str());

			// Make sure the filename isn't jumping folders...
			if (petFile == "" || petFile.find("..") != std::string::npos)	// only if a file is found and do **not** allow jumping up from here, as we are about to delete a file.
				return;
			
			// Confirm that the BSP is in aarcade_user/download/maps/[mapFile].bsp
			if (!g_pFullFileSystem->FileExists(VarArgs("pets/%s", petFile.c_str()), "DEFAULT_WRITE_PATH"))
				return;

			// FIXME: TODO: Explicity confirm that the file exists in the **absolute** path that aligns with the DEFAULT_WRITE_PATH. (The above check *might* be doing that already.)

			// Remove the TXT file
			g_pFullFileSystem->RemoveFile(VarArgs("pets/%s", petFile.c_str()), "DEFAULT_WRITE_PATH");
			g_pAnarchyManager->AddToastMessage(VarArgs("Removed pet outfit pets/%s", petFile.c_str()));
		//}
	}
}
ConCommand petoutfitdelete("petoutfitdelete", PetOutfitDelete, "Interal use only. Removes the specified outfit.", FCVAR_HIDDEN);

void PetOutfitLoad(const CCommand &args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	//DoPetOutfitStrip();

	//pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	bool bWasGivenIndex = (args.ArgC() > 2);
	pet_t* pPet = (bWasGivenIndex) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[2])) : g_pAnarchyManager->GetPlayAsPet();

	if (pPet)
	{
		DoPetOutfitStrip(pPet);

		C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		std::string petModelFilename = modelinfo->GetModelName(pPetEntity->GetModel());
		petModelFilename = g_pAnarchyManager->NormalizeModelFilename(petModelFilename);
		std::string attachmentName;
		std::string petModelHash = g_pAnarchyManager->GenerateLegacyHash(petModelFilename.c_str());

		std::string outfitId = args[1];
		pPet->outfitId = outfitId;

		KeyValues* kv = new KeyValues("outfit");
		if (kv->LoadFromFile(g_pFullFileSystem, VarArgs("pets/%s/outfits/%s.txt", petModelHash.c_str(), outfitId.c_str()), "DEFAULT_WRITE_PATH"))
		{
			for (KeyValues *subKey = kv->GetFirstSubKey(); subKey != nullptr; subKey = subKey->GetNextKey())
			{
				if (!Q_stricmp(subKey->GetName(), "attachments"))
				{
					for (KeyValues *attachment = subKey->GetFirstSubKey(); attachment != nullptr; attachment = attachment->GetNextKey())
					{
						attachmentName = attachment->GetString("name");
						KeyValues *models = attachment->FindKey("models");
						if (models)
						{
							for (KeyValues *model = models->GetFirstSubKey(); model != nullptr; model = model->GetNextKey())
							{
								engine->ClientCmd(VarArgs("petattach \"%s\" %i;", model->GetString("file"), pPet->iEntityIndex));
							}
						}
					}
				}
			}
		}
		kv->deleteThis();
		kv = null;
	}
}
ConCommand petoutfitload("petoutfitload", PetOutfitLoad, "Saves the outfit on the current play-as-pet. (Generates a new outfitid if one is not given.)");

void PetPlayAnimRaw(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	std::string sequenceName = args[1];

	/*
	pet_t* pPet = (args.ArgC()>2) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[2])) : g_pAnarchyManager->GetPlayAsPet();
	if (!pPet) {
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (!pPet) {
		return;
	}*/

	bool bWasGivenIndex = (args.ArgC() > 2);
	pet_t* pPet = (bWasGivenIndex) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[2])) : g_pAnarchyManager->GetPlayAsPet();
	if (!pPet) {
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (!pPet) {
		return;
	}

	//pPet->iState = AAPETSTATE_RUN;
	//std::string realSequenceTitle = sequenceName;//pPet->pConfigKV->GetString("run");
	C_DynamicProp* pProp = dynamic_cast<C_DynamicProp*>(C_BaseEntity::Instance(pPet->iEntityIndex));
	if (pProp)
	{
		g_pAnarchyManager->PlaySequenceRegularOnProp(pProp, sequenceName.c_str());
		pPet->iCurSequence = pProp->LookupSequence(sequenceName.c_str());
	}
}
ConCommand petplayanimraw("petplayanimraw", PetPlayAnimRaw, "Plays the exact sequence on the pet using the entity index. (Or nearest pet if -1 index specified. Or the play-as pet if index NOT specified.)"); //(Or the play-as pet if no pet index specified. Or nearest pet if neither.)");

void PetSaveBatch(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	std::string batchName = args[1];
	g_pAnarchyManager->SavePetBatch(batchName);
}
ConCommand petsavebatch("petsavebatch", PetSaveBatch, "Saves the current pets as the given name. (So be sure to give it a name.) Use it with petloadbatch later.");

void PetLoadBatch(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	std::string batchName = args[1];
	g_pAnarchyManager->LoadPetBatch(batchName);
}
ConCommand petloadbatch("petloadbatch", PetLoadBatch, "Loads the pets of a given name. (So be sure to give it a name.) Use it with petsavebatch.");

void PetBehavior(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	int iBehavior = Q_atoi(args[1]);

	bool bWasGivenIndex = (args.ArgC() > 2);
	pet_t* pPet = (bWasGivenIndex) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[2])) : g_pAnarchyManager->GetPlayAsPet();
	if (!pPet) {
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (!pPet) {
		return;
	}

	pPet->iBehavior = iBehavior;
}
ConCommand petbehavior("petbehavior", PetBehavior, "Sets the behavior on the pet using the entity index. (Or nearest pet if -1 index specified. Or the play-as pet if index NOT specified.)");

void PetBehaviorToggle(const CCommand &args)
{
	if (args.ArgC() < 3)
		return;

	int iBehaviorA = Q_atoi(args[1]);
	int iBehaviorB = Q_atoi(args[2]);
	if (iBehaviorB < 0) {
		iBehaviorB = 0;
	}

	bool bWasGivenIndex = (args.ArgC() > 3);
	pet_t* pPet = (bWasGivenIndex) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[3])) : g_pAnarchyManager->GetPlayAsPet();
	if (!pPet) {
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (!pPet) {
		return;
	}

	int iBehavior = (pPet->iBehavior == iBehaviorA) ? iBehaviorB : iBehaviorA;
	pPet->iBehavior = iBehavior;
}
ConCommand petbehaviortoggle("petbehaviortoggle", PetBehaviorToggle, "[behaviorA, behaviorB, petEntityIndex] Toggles the behavior on the pet using the entity index. (Or nearest pet if -1 index specified. Or the play-as pet if index NOT specified.) First param is primary behavior, 2nd param is 2ndary behavior, use -1 to just use 'idle'.");

void PetUpdate(const CCommand &args)
{
	//if (args.ArgC() < 4)
	//	return;

	//bool bWasGivenIndex = (args.ArgC() > 2);
	int iEntityIndex = Q_atoi(args[1]);
	pet_t* pPet = (iEntityIndex >= 0) ? g_pAnarchyManager->GetPetByEntIndex(iEntityIndex) : g_pAnarchyManager->GetPlayAsPet();

	if (!pPet) {
		return;
	}

	KeyValues* pConfigKV = pPet->pConfigKV;

	std::string fieldName = args[2];
	if (fieldName == "pos")
	{
		// pos
		UTIL_StringToVector(pPet->pos.Base(), args[3]);
		pConfigKV->SetString("pos", args[3]);
	}
	else if (fieldName == "rot")
	{
		// rot
		UTIL_StringToVector(pPet->rot.Base(), args[3]);
		pConfigKV->SetString("rot", args[3]);
	}
	else if (fieldName == "scale")
	{
		// scale
		float flScale = Q_atof(args[3]);
		pConfigKV->SetString("scale", VarArgs("%f %f %f", flScale, flScale, flScale));
		engine->ClientCmd(VarArgs("setscale %i %f;\n", pPet->iEntityIndex, flScale));
	}
	else if (fieldName == "idle")
	{
		// idle
		pConfigKV->SetString("idle", args[3]);
	}
	else if (fieldName == "run")
	{
		// run
		pConfigKV->SetString("run", args[3]);
	}
	else if (fieldName == "walk")
	{
		// walk
		pConfigKV->SetString("walk", args[3]);
	}
	else if (fieldName == "fall")
	{
		// fall
		pConfigKV->SetString("fall", args[3]);
	}
	else if (fieldName == "runspeed")
	{
		// runspeed
		pConfigKV->SetString("runspeed", args[3]);
	}
	else if (fieldName == "walkspeed")
	{
		// walkspeed
		pConfigKV->SetString("walkspeed", args[3]);
	}
	else if (fieldName == "near")
	{
		// near
		pConfigKV->SetString("near", args[3]);
	}
	else if (fieldName == "far")
	{
		// far
		pConfigKV->SetString("far", args[3]);
	}
}
ConCommand petupdate("petupdate", PetUpdate, "Updates the specified attribute on the pet using the entity index. petupdate INDEX FIELDNAME VALUE");

void PetPlay(const CCommand &args)
{
	pet_t* pPlayAsPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPlayAsPet)
	{
		g_pAnarchyManager->SetPlayAsPet(null);

		//"EXIT PLAYASPETMODE"	// also when you destroy the pet w/o calling petplaystop, and now also in PetPlay
		engine->ClientCmd("set_prop_vis -1 1;");
		engine->ClientCmd(VarArgs("set_prop_vis %i 1;", pPlayAsPet->iEntityIndex));
	}

	pet_t* pPet = null;
	int iEntityIndex = (args.ArgC() > 1) ? Q_atoi(args[1]) : -1;
	if (iEntityIndex >= 0)
	{
		pPet = g_pAnarchyManager->GetPetByEntIndex(iEntityIndex);
	}

	if (!pPet)
	{
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (pPet)// && !g_pAnarchyManager->GetPlayAsPet())
	{
		// Teleport us to the pet
		engine->ClientCmd(VarArgs("teleport_player_to_entity %i %f %f %f;", pPet->iEntityIndex, pPet->pos.x, pPet->pos.y, pPet->pos.z));

		/*C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		// Get the target entity's origin and angles
		Vector targetOrigin = pPetEntity->GetAbsOrigin();
		QAngle targetAngles = pPetEntity->GetAbsAngles();

		// Prepare a teleportation structure
		Vector vecVelocity(0, 0, 0);  // Stop all motion
		QAngle vecAngles = targetAngles;  // Optionally keep target entity's angles

		// Use the Teleport method from the CBaseEntity class
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		pPlayer->Teleport(&targetOrigin, &vecAngles, &vecVelocity);*/
		
		// Set the pet as our PlayAsPet pet.


		/*ConVar* pConVar = cvar->FindVar("cam_idealdist");
		float flVal = pConVar->GetFloat();
		flVal += 10.0f;
		if (flVal >= 1000.0f)
			flVal = 1000.0f;

		engine->ClientCmd(VarArgs("cam_idealdist %f; thirdperson;", flVal));*/
		engine->ClientCmd("thirdperson;");




		g_pAnarchyManager->SetPlayAsPet(pPet);

		//"ENTER PLAYASPET MODE"
		engine->ClientCmd("set_prop_vis -1 0;");
		engine->ClientCmd(VarArgs("set_prop_vis %i 1;", pPet->iEntityIndex));

		// TODO: Other stuff, like make sure we're 3rd person? Turn on toggle_lookspot?? Not sure yet.
		DevMsg("System ready...\n");
	}
}
ConCommand petplay("petplay", PetPlay, "Teleports you to the specified pet (or nearest pet to your aim) and starts playing as it.", FCVAR_NONE);

void PetPlayAsNext(const CCommand &args)
{
	pet_t* pPlayAsPet = g_pAnarchyManager->GetPlayAsPet();
	std::vector<pet_t*> pets = g_pAnarchyManager->GetAllLivingPets();
	pet_t* pPetTest = null;
	bool bDidFind = false;

	if (!pPlayAsPet) {
		if (pets.size() > 0)
		{
			bDidFind = true;
			pPetTest = pets[0];
		}
	}
	else
	{
		unsigned int uFoundAt = 0;
		for (unsigned int u = 0; u < pets.size(); u++) {
			pPetTest = pets[u];
			if (bDidFind) {
				break;
			}
			else if (pPlayAsPet && pPetTest == pPlayAsPet) {
				// pointers match.
				bDidFind = true;
				uFoundAt = u;
				pPetTest = null;
			}
		}

		if (bDidFind && !pPetTest && pets.size() > 1)
		{
			pPetTest = pets[0];
		}
	}

	if (!bDidFind || !pPetTest)
	{
		DevMsg("Could not find a pet to change to.\n");
		return;
	}

	// The next pet has been found.
	engine->ClientCmd(VarArgs("petplay %i;", pPetTest->iEntityIndex));
}
ConCommand petplayasnext("petplayasnext", PetPlayAsNext, "Switches to play as the NEXT pet.", FCVAR_NONE);

void PetPlayAsPrev(const CCommand &args)
{
	pet_t* pPlayAsPet = g_pAnarchyManager->GetPlayAsPet();
	std::vector<pet_t*> pets = g_pAnarchyManager->GetAllLivingPets();
	pet_t* pPetTest = null;
	bool bDidFind = false;

	if (!pPlayAsPet) {
		if (pets.size() > 0)
		{
			bDidFind = true;
			pPetTest = pets[pets.size()-1];
		}
	}
	else
	{
		for (unsigned int u = 0; u < pets.size(); u++) {
			pPetTest = pets[u];
			if (pPlayAsPet && pPetTest == pPlayAsPet) {
				// pointers match.
				// get the previous index.
				if (u > 0)
				{
					pPetTest = pets[u - 1];
				}
				else if (pets.size() > 1)
				{
					pPetTest = pets[pets.size() - 1];
				}
				bDidFind = true;
				break;
			}
		}
	}

	if (!bDidFind || !pPetTest)
	{
		DevMsg("Could not find a pet to change to.\n");
		return;
	}

	// The next pet has been found.
	engine->ClientCmd(VarArgs("petplay %i;", pPetTest->iEntityIndex));
}
ConCommand petplayasprev("petplayasprev", PetPlayAsPrev, "Switches to play as the PREVIOUS pet.", FCVAR_NONE);

void PetRemove(const CCommand &args)
{
	pet_t* pPet = null;
	int iEntityIndex = (args.ArgC() > 1) ? Q_atoi(args[1]) : -1;
	if (iEntityIndex >= 0)
	{
		pPet = g_pAnarchyManager->GetPetByEntIndex(iEntityIndex);
	}

	if (!pPet)
	{
		pPet = g_pAnarchyManager->GetNearestPetToPlayerLook();
	}

	if (pPet)
	{
		g_pAnarchyManager->DestroyPet(pPet->iEntityIndex);
	}
}
ConCommand petremove("petremove", PetRemove, "Removes the pet w/ the specified entity index (or the one nearest your crosshair.)", FCVAR_NONE);

void PetPlayStop(const CCommand &args)
{
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		g_pAnarchyManager->SetPlayAsPet(null);

		//"EXIT PLAYASPETMODE"	// also when you destroy the pet w/o calling petplaystop
		engine->ClientCmd("set_prop_vis -1 1;");
		engine->ClientCmd(VarArgs("set_prop_vis %i 1;", pPet->iEntityIndex));
	}
}
ConCommand petplaystop("petplaystop", PetPlayStop, "Exits play-as-pet mode.", FCVAR_NONE);

/*
bool PetRemoveAttachedModel(std::string attachmentName, std::string attachmentModelFilename)
{
	C_BaseEntity* pPet = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	if (pPet)
	{
		int iPetEntityIndex = pPet->entindex();
		pet_t* pet = g_pAnarchyManager->GetPetByEntIndex(iPetEntityIndex);

		auto it = pet->attachments.find(attachmentName);
		if (it != pet->attachments.end()) {
			std::vector<std::string>& filenames = it->second;
			auto newEnd = std::remove(filenames.begin(), filenames.end(), attachmentModelFilename);

			// Resize the vector to remove the undefined elements
			filenames.erase(newEnd, filenames.end());

			// Optional: Remove the key if no attachments are left
			if (filenames.empty()) {
				pet->attachments.erase(it);
			}
			return true;
		}
		else {
			// Attachment name not found.
		}
	}
	return false;
}
bool PetRemoveAllAttachedModels()
{
}*/

/*
bool _PetHasAttachment(pet_t* pPet, const std::string& attachmentName, const std::string& attachmentModelFilename) {
	// Search for the attachment name in the map
	auto it = pPet->attachments.find(attachmentName);
	if (it != pPet->attachments.end()) {
		// Get the vector of filenames
		const std::vector<std::string>& filenames = it->second;

		// Check if the model filename exists in the vector
		if (std::find(filenames.begin(), filenames.end(), attachmentModelFilename) != filenames.end()) {
			return true;
		}
	}

	return false;
}*/

// This is the one that should be bound to a key instead of pet attach. It always is about the play-as pet too.
void PetWear(const CCommand &args)
{
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (!pPet)
	{
		DevMsg("Must be playing as a pet to use this command.\n");
		g_pAnarchyManager->AddToastMessage("Must be playing as a pet to use this command.");
		return;
	}

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (pPlayer)
	{
		bool bIsThirdPerson = g_pAnarchyManager->CAM_IsThirdPerson();
		if (!bIsThirdPerson)
		{
			DevMsg("Must be 3rd person to attach to play-as pet.\n");
			g_pAnarchyManager->AddToastMessage("Must be 3rd person to attach to play-as pet.");
			engine->ClientCmd(VarArgs("thirdperson; set_prop_vis -1 0; set_prop_vis %i 1;", pPet->iEntityIndex));
			return;
		}

		engine->ClientCmd("petattach");
	}
}
ConCommand petwear("pet_wear", PetWear, "Attaches/detaches the object you're looking at to your current play-as pet.");

void PetAttach(const CCommand &args)
{
	//pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();

	bool bWasGivenIndex = (args.ArgC() > 2 && Q_atoi(args[2]) >= 0);
	pet_t* pPet = (bWasGivenIndex) ? g_pAnarchyManager->GetPetByEntIndex(Q_atoi(args[2])) : g_pAnarchyManager->GetPlayAsPet();

	if (!pPet)
		return;

	std::string attachmentModelFilename = (args.ArgC() > 1) ? args[1] : "";
	if (attachmentModelFilename == "")
	{
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
		C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		if (!pShortcut)
		{
			float flMaxRange = -1;
			object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
			if (pObject->spawned)
			{
				pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
				if (pBaseEntity)
					pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			}
		}

		if (pShortcut)
		{
			//const model_t* TheModel = pShortcut->GetModel();
			//attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(modelinfo->GetModelName(TheModel));
			attachmentModelFilename = modelinfo->GetModelName(pShortcut->GetModel());
			//attachmentModelFilename = STRING(((C_BaseEntity*)pShortcut)->GetModel());
		}
	}
	attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(attachmentModelFilename);

	//DevMsg("Pet Attachment Model Filename: %s\n", attachmentModelFilename.c_str());

	// get a reference to the current pet & to the object under the crosshair
	//C_BaseEntity* pPet = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	if (attachmentModelFilename != "")
	{
		//int iPetEntityIndex = pPet->iEntityIndex;
		//pet_t* pet = g_pAnarchyManager->GetPetByEntIndex(iPetEntityIndex);

		C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		//const model_t* TheModel = pPetEntity->GetModel();
		//std::string petModelFilename = modelinfo->GetModelName(TheModel);
		//std::string petModelFilename = STRING(pPetEntity->GetModelName());
		std::string petModelFilename = modelinfo->GetModelName(pPetEntity->GetModel());
		//std::string petModelFilename = STRING(pPetEntity->GetModelName());
		petModelFilename = g_pAnarchyManager->NormalizeModelFilename(petModelFilename);






		/* In order to do dupe removal per-bone, we need to store actual ENTITY references to the models attached to each bone.
		We CANNOT determine which attachment a model belongs to by traversing the game object, because the same model could be attached multiple times but to different bones. :(

		// DUPE (mostly) with "dopetattach"
		// check if this model is already attached. if it is, then detatch it instead.
		C_BaseEntity* pExistingProp = HasChildModel(dynamic_cast<C_DynamicProp*>(pPet), attachmentModelFilename);
		if (pExistingProp)
		{
			// remove it...
			engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
			DevMsg("Removed existing attached prop.\n");

			_PetRemoveAttachedModel(pet, )

			auto it = pet->attachments.find(attachmentModelFilename);
			if (it != pet->attachments.end()) {
				pet->attachments.erase(it);
			}

			//auto it = std::find(pet->attachments.begin(), pet->attachments.end(), attachmentModelFilename);
			//if (it != pet->attachments.end()) {
				pet->attachments.erase(it);
			//}
			return;
		}
	*/

		// PLAN B (for now)
		// If model exists ANYWHERE on the model, remove ALL instances of it.
		bool bDidRemove = false;
		C_DynamicProp* pDPet = dynamic_cast<C_DynamicProp*>(pPetEntity);
		C_BaseEntity* pExistingProp = HasChildModel(pDPet, attachmentModelFilename);
		while (pExistingProp)
		{
			// remove it...
			bDidRemove = true;
			engine->ClientCmd(VarArgs("removeobject %i;\n", pExistingProp->entindex()));	// servercmdfix , false);
			DevMsg("Removed existing attached prop.\n");
			pExistingProp = NULL;// HasChildModel(pDPet, attachmentModelFilename);// removing objects is not syncronous. :(
		}

		// Now remove ALL instances of it from all bookeeping
		if (bDidRemove)
		{
			_PetRemoveAttachedModel(pPet, "", attachmentModelFilename);
			return;
		}





		// we will likely perform checks for saved attachments...
		KeyValues* models = new KeyValues("attachments");
		if (!models->LoadFromFile(g_pFullFileSystem, "pet_attachments.txt", "DEFAULT_WRITE_PATH"))
		{
			models = NULL;
		}

		// Are we given an attachment name? If so, search for a saved attachment that is bound to it.
		//bool bChangedAttachmentName = false;
		KeyValues* pAttachmentKV = NULL;
		std::string attachmentName = (args.ArgC() > 3) ? args[3] : "";
		if (attachmentName != "")
		{
			if (models)
			{
				//KeyValues* models, const std::string& petModelFilename, const std::string& attachmentName, const std::string& attachmentModelFilename)
				pAttachmentKV = getPetAttachment(models, petModelFilename, attachmentName, attachmentModelFilename);
			}
		}

		if (!pAttachmentKV)
		{
			// otherwise, find ANY attachments for us that match our pet & model.
			if (models)
			{
				//std::map<std::string, KeyValues*> attachments = findAllPetAttachments(models, petModelFilename, attachmentModelFilename);
				auto attachments = findAllPetAttachments(models, petModelFilename, attachmentModelFilename);
				for (auto& pair : attachments) {
					//bChangedAttachmentName = true;
					attachmentName = pair.first;
					pAttachmentKV = pair.second;
					break;
				}
			}
		}

		if (attachmentName != "" && pAttachmentKV)
		{
			_PetAddAttachment(pPet, attachmentName, attachmentModelFilename);	// bookeeping

			Vector pos;
			UTIL_StringToVector(pos.Base(), pAttachmentKV->GetString("pos", "0 0 0"));
			QAngle rot;
			UTIL_StringToVector(rot.Base(), pAttachmentKV->GetString("rot", "0 0 0"));
			float flScale = pAttachmentKV->GetFloat("scale", 1.0f);
			engine->ClientCmd(VarArgs("loadpetattach %i \"%s\" \"%s\" %f %f %f %f %f %f %f;", pPet->iEntityIndex, attachmentModelFilename.c_str(), attachmentName.c_str(), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, flScale));
		}
		else
		{
			Msg("Attaching models w/o first setting an attachment position in the MOVE OBJECT menu is not yet supported!\n");
			g_pAnarchyManager->AddToastMessage("Need to manually attach this prop at least once first.");
		}

		if (models)
		{
			models->deleteThis();
		}
	}
}
ConCommand petattach("petattach", PetAttach, "Attaches the object you're looking at to your Play As Pet pet. Only works if you're playing as a pet.");

void TesterJoint2(const CCommand &args)
{
	//C_BaseEntity* pPet = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		// load the pet attachment save KV.
		// FIXME: Cache the loading of the pet attachment save KV.
		KeyValues* pModelAttachments = new KeyValues("attachments");
		if (pModelAttachments->LoadFromFile(g_pFullFileSystem, "pet_attachments.txt", "DEFAULT_WRITE_PATH"))
		{
			std::string petFilename = "models/pets/hackerhaley/pet_hackerhaley.mdl";
			std::string attachmentFilename = "models\\props\\sithlord\\beer.mdl";
			std::string attachmentName = "rhand";
			KeyValues* pAttachmentKV = getPetAttachment(pModelAttachments, petFilename.c_str(), attachmentName, attachmentFilename);
			if (pAttachmentKV)
			{
				Vector pos;
				UTIL_StringToVector(pos.Base(), pAttachmentKV->GetString("pos", "0 0 0"));
				QAngle rot;
				UTIL_StringToVector(rot.Base(), pAttachmentKV->GetString("rot", "0 0 0"));
				float flScale = pAttachmentKV->GetFloat("scale", 1.0f);
				pModelAttachments->deleteThis();
				engine->ClientCmd(VarArgs("loadpetattach %i \"%s\" \"%s\" %f %f %f %f %f %f %f;", pPet->iEntityIndex, attachmentFilename.c_str(), attachmentName.c_str(), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, flScale));
			}
		}
	}
}
ConCommand testerjoint2("testerjoint2", TesterJoint2, "Usage: ");

bool savePetAttachment(const std::string& petModelFilename, const std::string& attachmentName, const std::string& attachmentModelFilename, const std::string& posVector, const std::string& rotQAngle, float flScale)
{
	// load the pet attachment save KV.
	// FIXME: Cache the loading of the pet attachment save KV.
	KeyValues* models = new KeyValues("attachments");
	if (!models->LoadFromFile(g_pFullFileSystem, "pet_attachments.txt", "DEFAULT_WRITE_PATH"))
	{
		models = new KeyValues("attachments");
	}

	KeyValues* targetModel = nullptr;
	for (KeyValues *model = models->GetFirstSubKey(); model; model = model->GetNextKey()) {
		if (model->GetString("file") == petModelFilename) {
			targetModel = model;
			break;
		}
	}

	if (!targetModel) {
		targetModel = models->CreateNewKey();  // Create new model key
		targetModel->SetString("file", petModelFilename.c_str());
	}

	KeyValues* attachments = targetModel->FindKey("attachments", true);

	KeyValues* targetAttachment = nullptr;
	for (KeyValues *attachment = attachments->GetFirstSubKey(); attachment; attachment = attachment->GetNextKey()) {
		if (attachment->GetString("name") == attachmentName) {
			targetAttachment = attachment;
			break;
		}
	}

	if (!targetAttachment) {
		targetAttachment = attachments->CreateNewKey();  // Create new attachment key
		targetAttachment->SetString("name", attachmentName.c_str());
	}

	KeyValues* modelsSubKey = targetAttachment->FindKey("models", true);
	KeyValues* targetModelSub = nullptr;

	for (KeyValues *modelSub = modelsSubKey->GetFirstSubKey(); modelSub; modelSub = modelSub->GetNextKey()) {
		if (modelSub->GetString("file") == attachmentModelFilename) {
			targetModelSub = modelSub;
			break;
		}
	}

	if (!targetModelSub) {
		targetModelSub = modelsSubKey->CreateNewKey();  // Create new model subkey
		targetModelSub->SetString("file", attachmentModelFilename.c_str());
	}

	// Set position, rotation, and scale
	targetModelSub->SetString("pos", posVector.c_str());
	targetModelSub->SetString("rot", rotQAngle.c_str());
	targetModelSub->SetFloat("scale", flScale);

	bool bSaved = models->SaveToFile(g_pFullFileSystem, "pet_attachments.txt", "DEFAULT_WRITE_PATH");
	models->deleteThis();
	return bSaved;
}

void SavePetAttach(const CCommand &args)
{
	//C_BaseEntity* pPet = (C_BaseEntity*)g_pAnarchyManager->GetPlayAsPetEntity();
	pet_t* pPet = g_pAnarchyManager->GetPlayAsPet();
	if (pPet)
	{
		C_BaseEntity* pPetEntity = C_BaseEntity::Instance(pPet->iEntityIndex);
		//const model_t* TheModel = pPetEntity->GetModel();
		//std::string petModelFilename = modelinfo->GetModelName(TheModel);
		//std::string petModelFilename = STRING(pPetEntity->GetModelName());
		std::string petModelFilename = modelinfo->GetModelName(pPetEntity->GetModel());
		//std::string petModelFilename = STRING(((C_BaseEntity*)pPetEntity)->GetModelName());

		// normalize the model filename
		/*int iMaxString = petModelFilename.length() + 1;
		char* petModelFilenameFixed = new char[iMaxString];
		Q_strncpy(petModelFilenameFixed, petModelFilename.c_str(), iMaxString);
		V_FixSlashes(petModelFilenameFixed);
		V_FixDoubleSlashes(petModelFilenameFixed);
		petModelFilename = petModelFilenameFixed;*/
		petModelFilename = g_pAnarchyManager->NormalizeModelFilename(petModelFilename);


		std::string attachmentName = args[1];
		std::string attachmentModelFilename = args[2];

		// normalize the model filename
		/*iMaxString = attachmentModelFilename.length() + 1;
		char* attachmentModelFilenameFixed = new char[iMaxString];
		Q_strncpy(attachmentModelFilenameFixed, attachmentModelFilename.c_str(), iMaxString);
		V_FixSlashes(attachmentModelFilenameFixed);
		V_FixDoubleSlashes(attachmentModelFilenameFixed);
		attachmentModelFilename = attachmentModelFilenameFixed;*/
		attachmentModelFilename = g_pAnarchyManager->NormalizeModelFilename(attachmentModelFilename);

		std::string posVector = VarArgs("%s %s %s", args[3], args[4], args[5]);
		std::string rotQAngle = VarArgs("%s %s %s", args[6], args[7], args[8]);
		float flScale = Q_atof(args[9]);
		if (savePetAttachment(petModelFilename, attachmentName, attachmentModelFilename, posVector, rotQAngle, flScale))
		{
			DevMsg("Successfully saved pet attachment to pet_attachments.txt\n");
		}
	}
}
ConCommand savepetattach("savepetattach", SavePetAttach, "Usage: internal use only.", FCVAR_HIDDEN);


void ManualSpawn(const CCommand &args)
{
	cvar->FindVar("auto_unspawn_mode")->SetValue(2);	// c_instanceManager will catch this next update cycle & perform the logic - to avoid cmd overflow from doing it on-demand from here.
}
ConCommand manualSpawn("manual_spawn", ManualSpawn, "Usage: Manually unspawns all eligible objects, and spawns in any eligible objects as defined by spawn distance & spawn in view.");

void UnspawnAllObjects(const CCommand &args)
{
	int iMinSkippedFrames = (args.ArgC() < 2) ? -1 : Q_atoi(args[1]);
	int iNumUnspawned = g_pAnarchyManager->GetInstanceManager()->UnspawnAllObjects(iMinSkippedFrames);
	DevMsg("Unspawned %i objects.\n", iNumUnspawned);
}
ConCommand unspawnAllObjects("unspawn_all_objects", UnspawnAllObjects, "Usage: Unspawns all of the objects that are already spawned.");

/*void GetIdTime(const CCommand &args)
{
	std::string id = (args.ArgC() > 1) ? args[1] : "";
	long long llVal = g_pAnarchyManager->DecodeTimestampFromId(id);
	DevMsg("Timestamp: %lld\n", llVal);
}
ConCommand getIdTime("get_id_time", GetIdTime, "Usage: Attempts to extract a timtestamp from an item ID. Only works on IDs generated w/ the Firebase-style algo - such that items uses in Redux.");*/

void VROff(const CCommand &args)
{
	g_pAnarchyManager->VROff();
}
ConCommand vroff("vroff", VROff, "Usage: Forces VR off.  Warning: You can't re-enable VR after this until next time you launch AArcade.");

void TaskScreenshot(const CCommand &args)
{
	C_EmbeddedInstance* pInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
	if (!pInstance) {
		pInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();
	}

	if (!pInstance) {
		DevMsg("ERROR: You must have a task selected to take a screenshot of it.\n");
		return;
	}

	// all systems go
	pInstance->TakeScreenshot();
}
ConCommand taskscreenshot("task_screenshot", TaskScreenshot, "Usage: Saves the last rendered frame of the active embedded task to the aarcade_user/taskshots folder.");	// TODO: Let users choose the folder where task screenshots save to?

void GotTempPinnedCam(const CCommand &args)
{
	temppinnedcamindex.SetValue(Q_atoi(args[1]));
}
ConCommand gottemppinnedcam("gottemppinnedcam", GotTempPinnedCam, "Usage: Forces VR off.  Warning: You can't re-enable VR after this until next time you launch AArcade.", FCVAR_HIDDEN);

void CameraPinToggle(const CCommand &args)
{
	//engine->ClientCmd("gettemppinnedcam");
	bool bAlreadyExists = (temppinnedcamindex.GetInt() >= 0);

	/*
	char* val;
	for (C_BaseEntity *pEnt = ClientEntityList().FirstBaseEntity(); pEnt; pEnt = ClientEntityList().NextBaseEntity(pEnt))
	{
		if (pEnt->GetKeyValue("targetname", val, 16)) {
			if (!Q_strcmp(val, "aatmpcamdrop")) {
				bAlreadyExists = true;
				break;
			}
		}
	}*/

	/*
	C_BaseEntityIterator iterator;
	C_BaseEntity *pEnt;
	char* val;
	while ((pEnt = iterator.Next()) != NULL)
	{
		if (pEnt->GetKeyValue("targetname", val, 16)) {
			if (!Q_strcmp(val, "aatmpcamdrop")) {
				bAlreadyExists = true;
				break;
			}
		}
	}
	*/

	if (bAlreadyExists) {
		engine->ClientCmd("ent_fire aatmpcamdrop Disable; thirdperson_mayamode; ent_fire aatmpcamdrop KillHierarchy;");
	}
	else {

		if (cvar->FindVar("cam_is_thirdperson_mode")->GetBool())
		{
			if (cvar->FindVar("cam_is_maya_mode")->GetBool())
				engine->ClientCmd("ent_create point_viewcontrol targetname aatmpcamdrop spawnflags 9; ent_fire aatmpcamdrop Enable;");//thirdperson; thirdperson_mayamode;
			else
				engine->ClientCmd("ent_create point_viewcontrol targetname aatmpcamdrop spawnflags 9; thirdperson_mayamode; ent_fire aatmpcamdrop Enable;");//thirdperson;
		}
		else
		{
			engine->ClientCmd("thirdperson");
		}
	}
}
ConCommand camerapintoggle("camera_pin_toggle", CameraPinToggle, "Usage: Toggle the 3rd person camera being pinned in space.");

void VRToggle(const CCommand &args)
{
	g_pAnarchyManager->ToggleVR();

	/*
	//engine->ClientCmd("sbs_rendering 1\n");
	//g_pAnarchyManager->InitializeVR();
	HMODULE module = GetModuleHandle(TEXT("d3d9.dll"));

	if (module == null)
	DevMsg("Module d3d9.dll not found.\n");
	else
	{
	DevMsg("Found module d3d9.dll.\n");

	if ((bool(*)(void))GetProcAddress(module, "hmdIsConnected") == NULL)
	DevMsg("Could not find proc address for hmdIsConnected.\n");
	else
	DevMsg("Found proc address for hmdIsConnected!\n");
	}
	*/
}
ConCommand vrtoggle("vrtoggle", VRToggle, "Usage: Toggles VR.");

void SBSRendering(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	int val = Q_atoi(args[1]);
	if ( val == 1 )
		g_pAnarchyManager->EnableSBSRendering();
	else
		g_pAnarchyManager->DisableSBSRendering();
	//g_pHLVR->Activate();
	//g_pAnarchyManager->GetMetaverseManager()->OnAssetUploadBatchReady();
	//std::string file = "models/striker/nicebongstriker.mdl";
	//g_pAnarchyManager->GetMetaverseManager()->RequestAsset(file);
}
ConCommand sbsrendering("sbs_rendering", SBSRendering, "Usage: bool");

void AlwaysAnimatedImageToggle(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = null;

	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
	if (pBaseEntity)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);

	if (!pShortcut)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
		{
			pBaseEntity = C_BaseEntity::Instance(pObject->entityIndex);
			if (pBaseEntity)
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
		}
	}

	if (!pShortcut)
		return;

	std::string itemId = pShortcut->GetItemId();
	std::string modelId = pShortcut->GetModelId();
	if (itemId != "" && itemId != modelId)
		g_pAnarchyManager->GetAAIManager()->ToggleMarkAnimatedItem(itemId);
}
ConCommand always_animated_image_toggle("always_animated_image_toggle", AlwaysAnimatedImageToggle, "Usage: Toggles the item under your crosshair to be an always animated image.");

void ImportSteamGames(const CCommand &args)
{
	std::string tabName = (args.ArgC() > 1) ? args[1] : "all";
	g_pAnarchyManager->BeginImportSteamGames(tabName);
}
ConCommand import_steam_games("import_steam_games", ImportSteamGames, "Usage: imports your steam games from your public profile");
/*
void WheelUp(const CCommand &args)
{
	g_pAnarchyManager->GetInputManager()->OnMouseWheeled(1);
}
ConCommand wheel_up("wheelup", WheelUp, "Usage: mouse wheel up");

void WheelDown(const CCommand &args)
{
	g_pAnarchyManager->GetInputManager()->OnMouseWheeled(-1);
}
ConCommand wheel_down("wheeldown", WheelDown, "Usage: mouse wheel down");
*/
void RunEmbeddedLibretro(const CCommand &args)
{
	C_LibretroManager* pLibretroManager = g_pAnarchyManager->GetLibretroManager();
	if (pLibretroManager)
		pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Flash Gordon (1980).avi");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Jay and silent Bob Strike Back (2001).avi");
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:\\Emulators\\N64\\Roms\\Super Mario 64 (U) [!].zip");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Jay and silent Bob Strike Back (2001).avi");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Flash Gordon (1980).avi");
		//pLibretroManager->RunEmbeddedLibretro("mednafen_psx_libretro.dll", "X:\\Emulators\\PSP\\roms\\ffn-spac.iso");
		//pLibretroManager->RunEmbeddedLibretro("mednafen_psx_libretro.dll", "X:\\Emulators\\PS\\roms\\Need For Speed 4 - High Stakes [U] [SLUS-00826].cue");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Jay and silent Bob Strike Back (2001).avi");
		//pLibretroManager->RunEmbeddedLibretro("mednafen_psx_libretro.dll", "X:\\Emulators\\PS\\roms\\Need For Speed 4 - High Stakes [U] [SLUS-00826].cue");
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:\\Emulators\\N64\\Roms\\Super Mario 64 (U) [!].zip");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Jay and silent Bob Strike Back (2001).avi");
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:/Emulators/N64/Roms/GoldenEye 007 (U) [!].zip");
		//pLibretroManager->RunEmbeddedLibretro("mame_libretro.dll", "X:\\Emulators\\Arcade\\roms\\sfrush.zip");
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:\\Emulators\\N64\\Roms\\Super Mario 64 (U) [!].zip");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Jay and silent Bob Strike Back (2001).avi");
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:\\Emulators\\N64\\Roms\\Super Mario 64 (U) [!].zip");
		
		//pLibretroManager->RunEmbeddedLibretro("mupen64plus_libretro.dll", "X:\\Emulators\\N64\\Roms\\Super Mario 64 (U) [!].zip");
		
		//pLibretroManager->RunEmbeddedLibretro("mame2014_libretro.dll", "X:\\Emulators\\Arcade\\roms\\lethalen.zip");
		//pLibretroManager->RunEmbeddedLibretro("snes9x_libretro.dll", "X:\\Emulators\\SNES\\Roms\\Donkey Kong Country - Competition Cartridge (U).smc");
		//pLibretroManager->RunEmbeddedLibretro("mame_libretro.dll", "X:\\Emulators\\Arcade\\roms\\lethalen.zip");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Flash Gordon (1980).avi");
		//pLibretroManager->RunEmbeddedLibretro("snes9x_libretro.dll", "X:\\Emulators\\SNES\\Roms\\Donkey Kong Country - Competition Cartridge (U).smc");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/TV/Beavis & Butthead/Beavis and Butthead - Season 7/731 Drinking Butt-ies.mpg");
	
		//pLibretroManager->RunEmbeddedLibretro("mame_libretro.dll", "X:\\Emulators\\Arcade\\roms\\lethalen.zip");
		//pLibretroManager->RunEmbeddedLibretro("snes9x_libretro.dll", "X:\\Emulators\\SNES\\Roms\\Donkey Kong Country - Competition Cartridge (U).smc");
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Flash Gordon (1980).avi");
		//pLibretroManager->RunEmbeddedLibretro("mame2014_libretro.dll", "X:\\Emulators\\Arcade\\roms\\lethalen.zip");
		
		//pLibretroManager->RunEmbeddedLibretro("ffmpeg_libretro.dll", "V:/Movies/Flash Gordon (1980).avi");
	
		//pLibretroManager->RunEmbeddedLibretro("V:/Movies/Jay and silent Bob Strike Back (2001).avi");
}
ConCommand run_embedded_libretro("run_embedded_libretro", RunEmbeddedLibretro, "Usage: runs embedded apps");

void RunEmbeddedSteamBrowser(const CCommand &args)
{
	C_SteamBrowserManager* pSteamBrowserManager = g_pAnarchyManager->GetSteamBrowserManager();
	if (pSteamBrowserManager)
		pSteamBrowserManager->RunEmbeddedSteamBrowser();
}
ConCommand run_embedded_steam_browser("run_embedded_steam_browser", RunEmbeddedSteamBrowser, "Usage: runs embedded apps");

void RunEmbeddedAwesomiumBrowser(const CCommand &args)
{
	C_AwesomiumBrowserManager* pAwesomiumBrowserManager = g_pAnarchyManager->GetAwesomiumBrowserManager();
	if (pAwesomiumBrowserManager)
		pAwesomiumBrowserManager->RunEmbeddedAwesomiumBrowser();
}
ConCommand run_embedded_awesomium_browser("run_embedded_awesomium_browser", RunEmbeddedAwesomiumBrowser, "Usage: runs embedded apps");

void QuickRemember(const CCommand &args)
{
	std::string mode = (args.ArgC() > 2) ? args[2] : "";
	g_pAnarchyManager->QuickRemember(Q_atoi(args[1]), mode);
}
ConCommand quick_remember("quick_remember", QuickRemember, "Usage:");

void PlayNearestGIF(const CCommand &args)
{
	g_pAnarchyManager->GetInstanceManager()->PlayNearestGIF();
}
ConCommand play_nearest_gif("play_nearest_gif", PlayNearestGIF, "Usage: Plays the nearest GIF based on where you are looking at.  Useful if you use a lot of GIFs as animated holograms on your walls.");

void TaskRemember(const CCommand &args)
{
	bool bHandled = false;
	/*
	if (args.ArgC() > 1)
	{
		C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(Q_atoi(args[1]));
		if (pBaseEntity)
		{
			C_PropShortcutEntity* pPropShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
			if (pPropShortcut)
			{
				bHandled = true;
				g_pAnarchyManager->TaskRemember(pPropShortcut);
			}
		}
	}
	*/
	if ( !bHandled )
		g_pAnarchyManager->TaskRemember();
}
ConCommand task_remember("task_remember", TaskRemember, "Usage: sets the selected entity as continuous play.");

void MorphModel(const CCommand &args)
{
	//C_ArcadeResources* pClientArcadeResources = C_ArcadeResources::GetSelf();
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	std::string actualModel = "";
	if (args.ArgC() > 1)
	{
		actualModel = args[1];
		if (!g_pFullFileSystem->FileExists(actualModel.c_str(), "GAME"))	// SLOW!  Try to do it from model cache instead.
			return;
		//const model_t* pModel = modelinfo->FindOrLoadModel(actualModel.c_str());
		//if (modelinfo->GetModelIndex(actualModel.c_str()) < 0)
		//	return;
	}
	else
	{
		/*
		// Get the model the player is looking at
		trace_t tr;
		g_pAnarchyManager->SelectorTraceLine(tr);
		//Vector forward;
		//pPlayer->EyeVectors(&forward);
		//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		C_BaseEntity *bEntity = null;
		if (tr.fraction != 1.0 && tr.DidHitNonWorldEntity())
			bEntity = tr.m_pEnt;
			*/

		C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());

		if (!pEntity)
		{
			float flMaxRange = -1;
			object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
			if (pObject->spawned)
				pEntity = C_BaseEntity::Instance(pObject->entityIndex);
			else
				DevMsg("ERROR: The nearest object has not spwned in yet.\n");
		}

		if (!pEntity)
		{
			g_pAnarchyManager->AddToastMessage("You must be looking at an object to be able to morph into it.");
			return;
		}
		
		actualModel = MAKE_STRING(modelinfo->GetModelName(modelinfo->GetModel(pEntity->GetModelIndex())));
	}

	// check if we already are
	std::string standardizedModelFile = actualModel;
	std::replace(standardizedModelFile.begin(), standardizedModelFile.end(), '/', '\\');
	std::transform(standardizedModelFile.begin(), standardizedModelFile.end(), standardizedModelFile.begin(), ::tolower);
	ConVar* pPlayerModelConVar = cvar->FindVar("playermodel");
	//if (actualModel == pPlayerModelConVar->GetString())
		//return;
	const char *pCurrentModel = modelinfo->GetModelName(pPlayer->GetModel());
	if (!Q_stricmp(pCurrentModel, actualModel.c_str()))
		return;

	std::string model = actualModel;
	if (model.find("models") == 0 || model.find("MODELS") == 0)
	{
		model = model.substr(6);

		if (model.at(0) == '/' || model.at(0) == '\\')
		{
			model = model.substr(1);

			if (model.find(".mdl") == model.length() - 4 || model.find(".MDL") == model.length() - 4)
			{
				engine->ClientCmd(VarArgs("setplayermodel \"%s\";\n", actualModel.c_str()));	// servercmdfix

				if (args.ArgC() > 2 && Q_atoi(args[2]) != 0)
					engine->ClientCmd("thirdperson;\n");

				pPlayerModelConVar->SetValue(actualModel.c_str());
				cvar->FindVar("cl_playermodel")->SetValue(actualModel.c_str());
				engine->ClientCmd("host_writeconfig");

				g_pAnarchyManager->AddToastMessage(VarArgs("Set Player Model: %s", model.c_str()));
			}
		}
	}
}
ConCommand morphmodel("morphmodel", MorphModel, "Usegae: Look at a model and use this command to change your player model to it.");

void MorphEntityModel(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
	if (!pEntity)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
			pEntity = C_BaseEntity::Instance(pObject->entityIndex);
		else
			DevMsg("ERROR: The nearest object has not spwned in yet.\n");
	}

	if (!pEntity)
	{
		g_pAnarchyManager->AddToastMessage("You must be looking at an object to be able to morph it.");
		return;
	}

	//actualModel = MAKE_STRING(modelinfo->GetModelName(modelinfo->GetModel(pEntity->GetModelIndex())));

	//char goodModel[AA_MAX_STRING];
	int iAAMaxString = strlen(args[1]) + 1;
	char* goodModel = new char[iAAMaxString];
	Q_strncpy(goodModel, args[1], iAAMaxString);

	// Convert it to lowercase & change all slashes to back-slashes
	V_FixSlashes(goodModel, '/');

	engine->ClientCmd(VarArgs("setentitymodel %i \"%s\";\n", pEntity->entindex(), goodModel));
	g_pAnarchyManager->AddToastMessage(VarArgs("Morphed Entity Model: %s", goodModel));

	delete[] goodModel;
}
ConCommand morphentitymodel("morphentitymodel", MorphEntityModel, "Usegae: Morph the object under your crosshair into what ever your parameter is.");

void NPCMove(const CCommand &args)
{
	Vector pos = g_pAnarchyManager->GetSelectorTraceVector();

	engine->ClientCmd(VarArgs("donpcmove %.6g %.6g %.6g\n", pos.x, pos.y, pos.z));
}
ConCommand npcmove("npcmove", NPCMove, "Usegae: Morph the object under your crosshair into what ever your parameter is.");

void MorphNearestPlayerModel(const CCommand &args)
{
	std::string modelName = "";
	C_BaseEntity* pEntity;
	float flMaxRange = -1;
	bool bFoundPlayerModelSequence = false;
	object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
	object_t* pOriginalobject = pObject;
	CStudioHdr *hdr;
	KeyValues* pModelKV;
	C_PropShortcutEntity* pShortcut;
	int numseq;
	while ( pObject )
	{
		if (pObject->spawned)
		{
			pEntity = C_BaseEntity::Instance(pObject->entityIndex);

			if (pEntity)
			{
				pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
				if (pShortcut)
				{
					// test some model stuff
					/*
					{
						const model_t* TheModel = pShortcut->GetModel();
						studiohdr_t* studiomodel = modelinfo->GetStudiomodel(TheModel);
						int iNumIncludedModels = studiomodel->numincludemodels;
						DevMsg("Num included MDLs: %i\n", iNumIncludedModels);
						mstudiomodelgroup_t* pModelGroup;
						for (int i = 0; i < iNumIncludedModels; i++)
						{
							pModelGroup = studiomodel->pModelGroup(i);
							if (pModelGroup)
							{
								DevMsg("Included: %s\n", pModelGroup->pszName());
							}
						}
					}
					*/

					/*const model_t* TheModel = pShortcut->GetModel();// modelinfo->FindOrLoadModel(modelFileFixed);
					studiohdr_t* studiomodel = modelinfo->GetStudiomodel(TheModel);
					int iNumIncludedModels = studiomodel->numincludemodels;
					DevMsg("Num included MDLs: %i\n", iNumIncludedModels);*/
					
					/*
					const model_t* TheModel = pShortcut->GetModel();
					studiohdr_t* studiomodel = modelinfo->GetStudiomodel(TheModel);
					int iNumIncludedModels = studiomodel->numincludemodels;
					mstudiomodelgroup_t* pModelGroup;
					bool bAllGood = true;
					for (int i = 0; i < iNumIncludedModels; i++)
					{
						pModelGroup = studiomodel->pModelGroup(i);
						if (pModelGroup)
						{
							if (!Q_stricmp(pModelGroup->pszName(), "models/m_gst.mdl") || !Q_stricmp(pModelGroup->pszName(), "models\\m_gst.mdl"))
							{
								bAllGood = false;
								break;
							}
						}
					}
					

					if (bAllGood)
					{*/
					hdr = pShortcut->GetModelPtr();
					if (hdr)
					{
						// NOTE: This block only ever gets dropped into if the object is spawned AFTER initial map load.  hdr is always NULL on objects that already exist in the arcade upon map load.
						//numseq = hdr->GetNumSeq();
						//if (numseq < 1000)
						//{
						int iSequenceIndex = pShortcut->LookupSequence("idle_slam");	// Note that this might return 0 as an invalid response OR because this is the top sequence.  :S
						if (iSequenceIndex >= 0 && !Q_strcmp(pShortcut->GetSequenceName(iSequenceIndex), "idle_slam"))	// This eliminates the edge case from above.
						{
							if (pShortcut->LookupSequence("proportions") <= 0 && pShortcut->LookupSequence("pose_standing_01") <= 0)	// The edge case might hit us here, but it's unlikely and the impact is merely us morphing into a model that doesn't quite animate right.
							{
								pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("models", pObject->modelId));
								if (pModelKV)
								{
									modelName = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
									//DevMsg("Model Name: %s\n", modelName.c_str());
									bFoundPlayerModelSequence = true;
									break;
								}
							}
						}
						//}
					}


						/*
						hdr = pShortcut->GetModelPtr();
						if (hdr)
						{
							// NOTE: This block only ever gets dropped into if the object is spawned AFTER initial map load.  hdr is always NULL on objects that already exist in the arcade upon map load.
							numseq = hdr->GetNumSeq();

							for (unsigned int i = 0; i < numseq; i++)
							{
								if (!Q_strcmp(pShortcut->GetSequenceName(i), "HL2DM_Aim_Gravgun"))
								{
									pModelKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryEntry("models", pObject->modelId));
									if (pModelKV)
									{
										modelName = pModelKV->GetString(VarArgs("platforms/%s/file", AA_PLATFORM_ID));
										DevMsg("Model Name: %s\n", modelName.c_str());
										bFoundPlayerModelSequence = true;
									}

									break;
								}
							}

							if (bFoundPlayerModelSequence)
								break;
						}
						*/
					//}
				}
			}
		}

		pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(pObject, flMaxRange);
		//pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToObject("next", pOriginalobject, pObject, false, 999999.0f);
	}

	if (bFoundPlayerModelSequence)
	{
		//char input[AA_MAX_STRING];
		int iAAMaxString = modelName.length() + 1;
		char* input = new char[iAAMaxString];
		Q_strncpy(input, modelName.c_str(), iAAMaxString);

		// Convert it to lowercase & change all slashes to back-slashes
		V_FixSlashes(input, '/');

		engine->ClientCmd(VarArgs("morphmodel \"%s\"", input));
		delete[] input;
	}
}
ConCommand morphNearestPlayerModel("morphplayermodel", MorphNearestPlayerModel, "Useage: Morph into the nearest player model to where you are looking.");

void ResetModel(const CCommand &args)
{
	engine->ClientCmd("setplayermodel \"models/police.mdl\";\n");	// servercmdfix
	engine->ClientCmd("firstperson;\n");

	cvar->FindVar("playermodel")->SetValue("");
	//cvar->FindVar("cl_playermodel")->SetValue("");
	engine->ClientCmd("host_writeconfig");
}
ConCommand resetmodel("resetmodel", ResetModel, "Useage: Make sure your player model isn't an ERROR symbol.");

void SetVehicleModel(const CCommand &args)
{
	// Get the entity under the crosshair
	C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
	if (pEntity)
	{
		const model_t* TheModel = pEntity->GetModel();
		std::string modelName = modelinfo->GetModelName(TheModel);

		if (modelName != "")
		{
			g_pAnarchyManager->AddToastMessage(VarArgs("Set Vehicle Model: %s", modelName.c_str()));
			engine->ClientCmd(VarArgs("air_boat_model \"%s\";", modelName.c_str()));
		}
	}
}
ConCommand setvehiclemodel("setvehiclemodel", SetVehicleModel, "Usegae: Sets the player's current vehicle model to the one specified.");

/*
void GetPeak(const CCommand &args)
{
	g_pAnarchyManager->GetPeakAudio();
}
ConCommand getepak("getpeak", GetPeak, "Usegae: Make sure your player model isn't an ERROR symbol.");
*/
void SetLibretroVolume(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	float fVolume = Q_atof(args[1]);
	if (fVolume > 3.0)
		fVolume = 3.0;
	else if (fVolume < 0.0)
		fVolume = 0.0;
	
	libretro_volume.SetValue(fVolume);
	g_pAnarchyManager->GetLibretroManager()->SetVolume(fVolume);
}
ConCommand set_libretro_volume("set_libretro_volume", SetLibretroVolume, "Usage: sets the libretro volume & updates any currently running instances too.");

void TaskClear(const CCommand &args)
{
	g_pAnarchyManager->TaskClear();
}
ConCommand task_clear("task_clear", TaskClear, "Usage: closes all open instaces (execpt for important game system ones)");

void ShowTaskMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowTaskMenu();
}
ConCommand showtaskmenu("+task_menu", ShowTaskMenu, "Usage: check which in-game tasks are open.");

void HideTaskMenu(const CCommand &args)
{
	if (!ignore_next_tab_up.GetBool())
		g_pAnarchyManager->HideTaskMenu();
	else
		ignore_next_tab_up.SetValue(false);
}
ConCommand hidetaskmenu("-task_menu", HideTaskMenu, "Usage: hides the task menu.");

void ShowTaskMenuToggle(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowTaskMenu(true);
}
ConCommand show_task_menu("task_menu", ShowTaskMenuToggle, "Usage: ");

void ShowVehicleMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowVehicleMenu();
}
ConCommand show_vehicle_menu("vehicle_menu", ShowVehicleMenu, "Usage: ");

void ShowScreenshotMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowScreenshotMenu();
}
ConCommand show_screenshot_menu("screenshot_menu", ShowScreenshotMenu, "Usage: ");

void ShowFavoritesMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowFavoritesMenu();
}
ConCommand show_favorites_menu("favorites_menu", ShowFavoritesMenu, "Usage: ");

void ShowCommandsMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowCommandsMenu();
}
ConCommand show_commands_menu("commands_menu", ShowCommandsMenu, "Usage: ");

void ShowPaintMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowPaintMenu();
}
ConCommand show_paint_menu("paint_menu", ShowPaintMenu, "Usage: ");

void ShowPlayersMenu(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ShowPlayersMenu();
}
ConCommand show_players_menu("players_menu", ShowPlayersMenu, "Usage: ");

void PrepPano(const CCommand &args)
{
	g_pAnarchyManager->PrepPano();
}
ConCommand prep_pano("prep_pano", PrepPano, "Usage: Internal.");

void FinishPano(const CCommand &args)
{
	g_pAnarchyManager->FinishPano();
}
ConCommand finish_pano("finish_pano", FinishPano, "Usage: Internal.");

void ManualPause(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
		g_pAnarchyManager->HandleUiToggle();

	g_pAnarchyManager->ManualPause();
}
ConCommand manual_pause("manual_pause", ManualPause, "Usage: Bind this to a button if you want a hotkey for pausing.");

void TakeScreenshot(const CCommand &args)
{
	g_pAnarchyManager->TakeScreenshot(true);
	g_pAnarchyManager->ShowScreenshotMenu();
}
ConCommand take_screenshot("take_screenshot", TakeScreenshot, "Usage: ");

///*
void VRSpazzFixed(const CCommand &args)
{
	int iIndex = Q_atoi(args[1]);
	int iIndex2 = Q_atoi(args[2]);

	g_pAnarchyManager->OnVRSpazzFixCreated(iIndex, iIndex2);
}
ConCommand vr_spazz_fixed("vr_spazz_fixed", VRSpazzFixed, "Fixed the spazz that happens on some maps w/ VR mode.");
//*/

/*
void HideScreenshotMenu(const CCommand &args)
{
	g_pAnarchyManager->HideScreenshotMenu();
}
ConCommand hide_screenshot_menu("-screenshot_menu", HideScreenshotMenu, "Usage: ");
*/

/*
void RememberWrapper(const CCommand &args)
{
	engine->ClientCmd("setcontinuous\n");
}
ConCommand rememberwrapper("-remember", RememberWrapper, "Usage: wrapper for the remember button to mean setcontinous now.");
*/

void MainMenu(const CCommand &args)
{
	if (g_pAnarchyManager->IsInitialized()) {
		g_pAnarchyManager->RunAArcade();
	}
}
ConCommand main_menu("main_menu", MainMenu, "Usage: runs AArcade");	// used from Main Menu

void CycleStaggerPattern(const CCommand &args)
{
	g_pAnarchyManager->CycleStaggerPattern();
	aaPetStaggerPattern staggerPattern = g_pAnarchyManager->GetCurrentStaggerPattern();
	int currentPattern = static_cast<int>(staggerPattern);
	DevMsg("Cycled to pattern: %i\n", currentPattern);
	g_pAnarchyManager->AddToastMessage(VarArgs("Cycled to pattern: %i\n", currentPattern));
}
ConCommand cycle_stagger_pattern("cycle_stagger_pattern", CycleStaggerPattern, "Usage: Cycles the pet stagger pattern.");

void ShowConnect(const CCommand &args)
{
	std::string lobbyId = args[1];
	g_pAnarchyManager->ShowConnect(lobbyId);
}
ConCommand show_connect("show_connect", ShowConnect, "Usage: runs AArcade", FCVAR_HIDDEN);	// used from Main Menu

/*
void TestFunction2( const CCommand &args )
{
	// WORKING SEND/RECIEVE FILE CALLS
//	#include "inetchannel.h"
//	INetChannel* pINetChannel = static_cast<INetChannel*>(engine->GetNetChannelInfo());
//	pINetChannel->RequestFile("downloads/<hash>.vtf", false);
//	pINetChannel->SendFile("stuff/test.jpg", 0, false);

	// WORKING CURSOR POSITIONS
//	#include "vgui/IInput.h"
//	#include <vgui_controls/Controls.h>
//	int x, y;
//	vgui::input()->GetCursorPos(x, y);

	// NEW TEST


	webviewinput->Create();
	DevMsg("Planel created.\n");
}

ConCommand test_function2( "testfunc2", TestFunction2, "Usage: executes an arbitrary hard-coded C++ routine" );
*/

void ShowHubsMenuClient(const CCommand &args)
{
	g_pAnarchyManager->ShowNodeManagerMenu();
}
ConCommand showhubsmenuclient("showhubsmenuclient", ShowHubsMenuClient, "Opens the Node Manager.", FCVAR_HIDDEN);

void ShowArcadeCrosshair(const CCommand &args)
{
	g_pAnarchyManager->CreateArcadeCrosshair();
}
ConCommand show_arcade_crosshair("show_arcade_crosshair", ShowArcadeCrosshair, "Shows the arcade crosshair.", FCVAR_NONE);

void HideArcadeCrosshair(const CCommand &args)
{
	g_pAnarchyManager->DestroyArcadeCrosshair();
}
ConCommand hide_arcade_crosshair("hide_arcade_crosshair", HideArcadeCrosshair, "Hides the arcade crosshair.", FCVAR_NONE);

void ShowHubSaveMenuClient(const CCommand &args)
{
	C_BaseEntity* pBaseEntity = C_BaseEntity::Instance(Q_atoi(args[1]));
	if (!pBaseEntity)
		return;

	C_PropShortcutEntity* pPropShortcut = dynamic_cast<C_PropShortcutEntity*>(pBaseEntity);
	if (!pPropShortcut)
		return;

	g_pAnarchyManager->ShowHubSaveMenuClient(pPropShortcut);
}
ConCommand showhubsavemenuclient("showhubsavemenuclient", ShowHubSaveMenuClient, "Does some logic to save the node.", FCVAR_HIDDEN);

void AnarchyManager(const CCommand &args)
{
	g_pAnarchyManager->AnarchyStartup();
}
ConCommand anarchymanager("anarchymanager", AnarchyManager, "Starts the Anarchy Manager.", FCVAR_HIDDEN);

void StoppedHoldingPrimaryFire(const CCommand &args)
{
	g_pAnarchyManager->StopHoldingPrimaryFire();
}
ConCommand stoppedholdingprimaryfire("stoppedHoldingPrimaryFire", StoppedHoldingPrimaryFire, "", FCVAR_HIDDEN);

void StartedHoldingPrimaryFire(const CCommand &args)
{
	g_pAnarchyManager->StartHoldingPrimaryFire();
}
ConCommand startedholdingprimaryfire("startedHoldingPrimaryFire", StartedHoldingPrimaryFire, "", FCVAR_HIDDEN);

void BuildContextUp(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
		return;

	// check if a propshortcut is under the player's crosshair
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (Q_strcmp(pPlayer->GetActiveWeapon()->GetName(), "weapon_physcannon"))
	{
		CBaseCombatWeapon *pWeapon = pPlayer->Weapon_OwnsThisType("weapon_physcannon");
		if (pWeapon && pPlayer->GetActiveWeapon()->CanHolster())
			engine->ClientCmd("phys_swap");
		return;
	}

	//bool SwitchToNextBestWeapon(C_BaseCombatWeapon *pCurrent);

	//virtual C_BaseCombatWeapon	*GetActiveWeapon(void) const;
	//int					WeaponCount() const;
	//C_BaseCombatWeapon	*GetWeapon(int i) const;

	//if (!pPlayer)
	//return;

	//if (pPlayer->GetHealth() <= 0)
	//return;

	bool bAutoChooseLibrary = true;

	/*
	// fire a trace line
	trace_t tr;
	g_pAnarchyManager->SelectorTraceLine(tr);
	//Vector forward;
	//pPlayer->EyeVectors(&forward);
	//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	C_BaseEntity *pEntity = (tr.DidHitNonWorldEntity()) ? tr.m_pEnt : null;
	*/

	C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());

	// only allow prop shortcuts
	C_PropShortcutEntity* pShortcut = (pEntity) ? dynamic_cast<C_PropShortcutEntity*>(pEntity) : null;
	if (pShortcut)//&& tr.fraction != 1.0)
		bAutoChooseLibrary = false;	// TODO: If you want to highlight the object that the context menu applies to, now's the time.

	C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
	if (bAutoChooseLibrary)
	{
		//DevMsg("DISPLAY MAIN MENU\n");
		//* pConnection = g_pAnarchyManager->GetConnectedUniverse();
		//if (!pConnection || pConnection->isHost)
		//{
			if (g_pAnarchyManager->GetSelectedEntity())
				g_pAnarchyManager->DeselectEntity("asset://ui/libraryBrowser.html");
			else
				pHudBrowserInstance->SetUrl("asset://ui/libraryBrowser.html");

			if (vgui::input()->WasKeyReleased(KEY_XBUTTON_Y))
				g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
			g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false);// true);
		//}
		//else
		//	g_pAnarchyManager->AddToastMessage("Guests are not allowed to do that in this session.");
	}
	else
	{
		//DevMsg("DISPLAY BUILD MODE CONTEXT MENU\n");
		if (g_pAnarchyManager->GetInputManager()->GetInputMode())
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

		std::string url = VarArgs("asset://ui/buildModeContext.html?entity=%i", pShortcut->entindex());

		if (g_pAnarchyManager->GetSelectedEntity())
			g_pAnarchyManager->DeselectEntity(url);
		else
			pHudBrowserInstance->SetUrl(url);

		if (vgui::input()->WasKeyReleased(KEY_XBUTTON_Y))
			g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);// true, pHudBrowserInstance);
	}

	return;
}
ConCommand buildcontextup("-remote_control", BuildContextUp, "Open up the library, or shows the edit object menu.", FCVAR_NONE);

void ReapplyPaint(const CCommand &args)
{
	g_pAnarchyManager->GetInstanceManager()->ResetAllMaterialMods();
	g_pAnarchyManager->GetInstanceManager()->ApplyAllMaterialMods();
}
ConCommand repaint("repaint", ReapplyPaint, "Reapply all material modifications.  This usually fixes props that failed to apply their material modifications.", FCVAR_NONE);

void ToggleCameraFlyMode(const CCommand &args)
{
	//bind joy5 +movedown; bind joy6 +moveup; sv_noclipspeed 0.2; cl_upspeed 100; ent_fire !player setmodelscale 0.1; noclip;

	ConVar* pNoClipSpeedConVar = cvar->FindVar("sv_noclipspeed");
	if (pNoClipSpeedConVar->GetFloat() != 5.0f)
	{
		// toggle OFF
		engine->ClientCmd("unbind joy5; unbind joy6; sv_noclipspeed 5; cl_upspeed 321; ent_fire !player setmodelscale 1.0; noclip;");
	}
	else
	{
		// toggle ON
		engine->ClientCmd("bind joy5 +movedown; bind joy6 +moveup; sv_noclipspeed 0.2; cl_upspeed 100; ent_fire !player setmodelscale 0.1; noclip;");
	}
}
ConCommand toggle_camera_fly_mode("toggle_camera_fly_mode", ToggleCameraFlyMode, "Shrinks you down (Tiny Mode), enables flymode, adjusts flymode movement speed, and rebinds the gamepad top shoulder buttons to be Move Up / Down instead.", FCVAR_NONE);

void Paint(const CCommand &args)
{
	g_pAnarchyManager->PaintMaterial();
}
ConCommand paint("paint", Paint, "Paint the material under your crosshair with the value of paint_texture.", FCVAR_NONE);

void Unpaint(const CCommand &args)
{
	g_pAnarchyManager->UnpaintMaterial();
}
ConCommand unpaint("unpaint", Unpaint, "Unpaint the material under your crosshair and return it to its normal texture.", FCVAR_NONE);

void UnpaintAll(const CCommand &args)
{
	g_pAnarchyManager->GetInstanceManager()->ClearAllMaterialMods();
}
ConCommand unpaintall("unpaintall", UnpaintAll, "Unpaint ALL the materials in the instance.", FCVAR_NONE);

void PickTexture(const CCommand &args)
{
	g_pAnarchyManager->PickPaintTexture();
}
ConCommand picktexture("pick_texture", PickTexture, "Sets the texture under your crosshair as the paint texture to use next time you issue the paint command.", FCVAR_NONE);

/*
void DebugUploadMaterial(const CCommand &args)
{

	if (!g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance())
		return;

	std::string materialName = (args.ArgC() > 1) ? args.Arg(1) : "";
	//std::string customSubFolderName = (args.ArgC() > 2) ? args.Arg(2) : "adopted";

	if (materialName == "")
	{
		trace_t tr;
		Vector forward;
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit())
		{
			if (!Q_strcmp(tr.surface.name, "**studio**"))
			{
				DevMsg("Prop Detected.  Use adopt_model instead.\n");
				g_pAnarchyManager->AddToastMessage("Prop Detected. Use adopt_model instead.");
				return;
			}
			else
			{
				IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
				if (!pMaterial || pMaterial->IsErrorMaterial())
					pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

				if (pMaterial && !pMaterial->IsErrorMaterial())
					materialName = pMaterial->GetName();
				//{
				//g_pAnarchyManager->GetMetaverseManager()->AdoptMaterial(null, pMaterial, null);
				//g_pAnarchyManager->AddToastMessage(VarArgs("Adopted Material %s", pMaterial->GetName()));
				//}
			}
		}
	}

	if (materialName != "")
	{
		KeyValues* pParentBatchKV = new KeyValues("batch");
		g_pAnarchyManager->GetMetaverseManager()->AddMaterialToUploadBatch(null, "", pParentBatchKV, materialName);
		pParentBatchKV->SaveToFile(g_pFullFileSystem, "tester_what.txt", "DEFAULT_WRITE_PATH");
		pParentBatchKV->deleteThis();

		//KeyValues* pAdoptedFilesKV = new KeyValues("adopted");
		//g_pAnarchyManager->GetMetaverseManager()->AdoptMaterial(pAdoptedFilesKV, null, materialName.c_str(), customSubFolderName);
		//g_pAnarchyManager->AddToastMessage(VarArgs("Adopted Material %s into custom/%s", materialName.c_str(), customSubFolderName.c_str()));

		//pAdoptedFilesKV->SaveToFile(g_pFullFileSystem, "adopted_log.txt", "DEFAULT_WRITE_PATH");
		//pAdoptedFilesKV->deleteThis();
	}


	//g_pAnarchyManager->PickPaintTexture();
	//g_pAnarchyManager->AdoptMaterial(materialName, materialVar, value);
}
ConCommand debug_uploadmaterial("debug_uploadmaterial", DebugUploadMaterial, "Internal use only.", FCVAR_NONE);
*/

/*
void DebugUploadMap(const CCommand &args)
{
	if (!g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance())
		return;

	std::string mapName = (args.ArgC() > 1) ? args.Arg(1) : g_pAnarchyManager->MapName();

	if (mapName != "")
	{
		KeyValues* pMapKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->FindMap(VarArgs("%s.bsp", mapName.c_str())));
		if (pMapKV)
		{
			g_pAnarchyManager->GetMetaverseManager()->AddMapToUploadBatch(pMapKV->GetString("info/id"), "");
		}
	}
}
ConCommand debug_uploadmap("debug_uploadmap", DebugUploadMap, "Internal use only.", FCVAR_NONE);
*/

void AdoptSky(const CCommand &args)
{
	std::string skyname = (args.ArgC() > 1) ? args.Arg(1) : "";
	if (skyname == "")
	{
		skyname = cvar->FindVar("painted_skyname")->GetString();
		if (skyname == "")
			skyname = cvar->FindVar("sv_skyname")->GetString();
	}

	if (skyname == "")
		return;

	g_pAnarchyManager->AdoptSky(skyname);
}
ConCommand adoptsky("adopt_sky", AdoptSky, "Adopts the sky material that is specified and adds its assets to your aarcade_user/custom/adopted folder. Do not include materials/skybox nor the .vmt file extension, nor the _ft etc. postfix.", FCVAR_NONE);

void AdoptAssetMenu(const CCommand &args)
{
	g_pAnarchyManager->ShowAdoptAssetMenu();
}
ConCommand adoptassetmenu("adopt_asset_menu", AdoptAssetMenu, "Bring up the Adopt Menu to make an extra COPY of the asset files for a model or material.", FCVAR_NONE);

void AdoptUploadBatch(const CCommand &args) {
	// Add this phy file
	//buf = VarArgs("%s.phy", modelName.c_str());
	//if (filesystem->FileExists(buf.c_str(), packingPath))
	//	this->AdoptFile(pAdoptedFilesKV, buf.c_str(), goodCustomSubFolder, bDoNotReallyAdopt);

	std::string customSubFolder = "upload_batch";
	std::string fileName = "upload_batch_log.txt";
	KeyValues* pKV = new KeyValues("files");
	pKV->LoadFromFile(g_pFullFileSystem, fileName.c_str(), "GAME");
	KeyValues* pUniqueFilesKV = pKV->FindKey("unique");
	KeyValues* pAdoptedFilesKV = new KeyValues("files");
	if (pUniqueFilesKV) {
		for (KeyValues *sub = pUniqueFilesKV->GetFirstSubKey(); sub; sub = sub->GetNextKey()) {
			g_pAnarchyManager->GetMetaverseManager()->AdoptFile(pAdoptedFilesKV, sub->GetString(), customSubFolder, false);
		}
	}
	pKV->deleteThis();

	unsigned int count = 0;
	for (KeyValues *sub = pAdoptedFilesKV->GetFirstSubKey(); sub; sub = sub->GetNextKey()) {
		count++;
	}
	pAdoptedFilesKV->deleteThis();

	DevMsg("Finished adopting %u files into %s\n", count, customSubFolder.c_str());
}
ConCommand adoptuploadbatch("adopt_upload_batch", AdoptUploadBatch, "Adopts the upload batch.", FCVAR_NONE);

void AdoptModel(const CCommand &args)
{
	std::string modelName = (args.ArgC() > 1) ? args[1] : "";
	std::string customSubFolderName = (args.ArgC() > 2) ? args.Arg(2) : "adopted";

	int iIndex = g_pAnarchyManager->GetSelectorTraceEntityIndex();
	C_BaseEntity* pEntity = C_BaseEntity::Instance(iIndex);
	if (pEntity)
	{
		const model_t* TheModel = pEntity->GetModel();
		modelName = modelinfo->GetModelName(TheModel);
	}

	if (modelName != "")
	{
		KeyValues* pAdoptedFilesKV = new KeyValues("adopted");
		g_pAnarchyManager->GetMetaverseManager()->AdoptModel("", pAdoptedFilesKV, modelName, customSubFolderName);
		g_pAnarchyManager->AddToastMessage(VarArgs("Adopted Model %s into custom/%s", modelName.c_str(), customSubFolderName.c_str()));

		pAdoptedFilesKV->SaveToFile(g_pFullFileSystem, "adopted_log.txt", "DEFAULT_WRITE_PATH");
		pAdoptedFilesKV->deleteThis();
	}
}
ConCommand adoptmodel("adopt_model", AdoptModel, "Adopts the model under your crosshair and adds its assets to your aarcade_user/custom/adopted folder.", FCVAR_NONE);

void AdoptMaterial(const CCommand &args)
{

	if (!g_pAnarchyManager->GetInstanceManager()->GetCurrentInstance())
		return;

	std::string materialName = (args.ArgC() > 1) ? args.Arg(1) : "";
	std::string customSubFolderName = (args.ArgC() > 2) ? args.Arg(2) : "adopted";

	if (materialName == "")
	{
		trace_t tr;
		Vector forward;
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.DidHit())
		{
			if (!Q_strcmp(tr.surface.name, "**studio**"))
			{
				DevMsg("Prop Detected.  Use adopt_model instead.\n");
				g_pAnarchyManager->AddToastMessage("Prop Detected. Use adopt_model instead.");
				return;
			}
			else
			{
				IMaterial* pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_WORLD);
				if (!pMaterial || pMaterial->IsErrorMaterial())
					pMaterial = g_pMaterialSystem->FindMaterial(tr.surface.name, TEXTURE_GROUP_MODEL);

				if (pMaterial && !pMaterial->IsErrorMaterial())
					materialName = pMaterial->GetName();
				//{
				//g_pAnarchyManager->GetMetaverseManager()->AdoptMaterial(null, pMaterial, null);
				//g_pAnarchyManager->AddToastMessage(VarArgs("Adopted Material %s", pMaterial->GetName()));
				//}
			}
		}
	}

	if (materialName != "")
	{
		KeyValues* pAdoptedFilesKV = new KeyValues("adopted");
		g_pAnarchyManager->GetMetaverseManager()->AdoptMaterial(pAdoptedFilesKV, null, materialName.c_str(), customSubFolderName);
		g_pAnarchyManager->AddToastMessage(VarArgs("Adopted Material %s into custom/%s", materialName.c_str(), customSubFolderName.c_str()));

		pAdoptedFilesKV->SaveToFile(g_pFullFileSystem, "adopted_log.txt", "DEFAULT_WRITE_PATH");
		pAdoptedFilesKV->deleteThis();
	}


	//g_pAnarchyManager->PickPaintTexture();
	//g_pAnarchyManager->AdoptMaterial(materialName, materialVar, value);
}
ConCommand adoptmaterial("adopt_material", AdoptMaterial, "Adopts the material under your crosshair and puts a copy of it into your aarcade_user/custom/adopted folder.", FCVAR_NONE);

void PaintSkyTexture(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	std::string skyName = args[1];

	// Do 6 material modifications.  Save on the last one.
	std::string originalSky = cvar->FindVar("sv_skyname")->GetString();
	std::string material, materialVarValue;

	// back
	material = VarArgs("skybox/%sbk", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%sbk", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, false);

	// down
	material = VarArgs("skybox/%sdn", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%sdn", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, false);

	// front
	material = VarArgs("skybox/%sft", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%sft", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, false);

	// left
	material = VarArgs("skybox/%slf", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%slf", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, false);

	// right
	material = VarArgs("skybox/%srt", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%srt", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, false);

	// up
	material = VarArgs("skybox/%sup", originalSky.c_str());
	materialVarValue = VarArgs("skybox/%sup", skyName.c_str());
	g_pAnarchyManager->GetInstanceManager()->SetMaterialMod(material, "$basetexture", materialVarValue, true);

	g_pAnarchyManager->AddToastMessage("Manually Painted Skybox");
}
ConCommand paintskytexture("paint_sky_texture", PaintSkyTexture, "Manually paint the sky with the provided material name. Do NOT include skybox/ in the material name, and do NOT include the file extension. It is recommended you use the F6 paint menu instead of this command for painting skyboxes.", FCVAR_NONE);

void ModifyMaterial(const CCommand &args)
{
	/*
	// Load the model and find all of its materials (actually only the first 1024 used on it)
	const model_t* TheModel = modelinfo->FindOrLoadModel(modelFileFixed);

	IMaterial* pMaterials[1024];
	for (int x = 0; x < 1024; x++)
		pMaterials[x] = NULL;

	modelinfo->GetModelMaterials(TheModel, 1024, &pMaterials[0]);

	for (int x = 0; x < 1024; x++)
	{
		if (pMaterials[x])
			this->AdoptMaterial(pAdoptedFilesKV, pMaterials[x]);
	}
	*/

	if (args.ArgC() < 4)
		return;

	std::string materialName = args[1];
	std::string materialVar = args[2];
	std::string value = args[3];

	g_pAnarchyManager->ModifyMaterial(materialName, materialVar, value);
}
ConCommand modifymaterial("modmat", ModifyMaterial, "Modifiy a material on-the-fly.  The 1st param, if not blank, is a material name to modify.  Otherwise, the material under the crosshair gets used.  The 2nd param is the materialvar name to modify.  The 3rd param is the materialvar value to assign to it.", FCVAR_NONE);

void BuildContextDown(const CCommand &args)
{
	// do nothing
}
ConCommand buildcontextdown("+remote_control", BuildContextDown, "Open up the library, or shows the edit object menu.", FCVAR_NONE);

/*
void ReparentShortcutsReady(const CCommand &args)
{
	// get a vector of the trigger entities
	std::string triggerEntityIndexes = args[1];
	std::vector<C_BaseEntity*> triggerEntities;
	std::vector<std::string> tokens;
	g_pAnarchyManager->Tokenize(triggerEntityIndexes, tokens, " ");
	for (unsigned int i = 0; i < tokens.size(); i++)
	{
		triggerEntities.push_back(C_BaseEntity::Instance(Q_atoi(tokens[i].c_str())));
	}

	// get the parent entity
	C_BaseEntity* pParentEntity = C_BaseEntity::Instance(Q_atoi(args[2]));

	if (pParentEntity && triggerEntities.size() > 0) {
		std::vector<C_PropShortcutEntity*> victims;
		std::map<std::string, object_t*> objects = g_pAnarchyManager->GetInstanceManager()->GetObjectsMap();
		object_t* pObject;
		auto it = objects.begin();
		while (it != objects.end())
		{
			pObject = it->second;
			if (pObject->entityIndex) {
				for (unsigned int i = 0; i < triggerEntities.size(); i++) {
					// if it's in ANY of the trigger zones, add it to victims...
					
				}
			}

			it++;
		}


			// loop through every object that is spawned & test if it's current position is inside of the volume. Add to victims list if so (if it's not already on there.)


			//g_pAnarchyManager->GetInstanceManager()->GetObjectsMap();
			//std::find(v.begin(), v.end(), "abc") != v.end()
		}
	}
	// yadda
}
ConCommand reparentshortcutsready("reparent_shortcuts_ready", ReparentShortcutsReady, "Interal use only.", FCVAR_HIDDEN);
*/

void Join(const CCommand &args)
{
	std::string lobby;
	if (args.ArgC() > 1)
		lobby = args[1];

	g_pAnarchyManager->Join(lobby);
}
ConCommand join("join", Join, "Join the given lobby.", FCVAR_NONE);

void MakeRagdoll(const CCommand &args)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());
	if (!pEntity)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
			pEntity = C_BaseEntity::Instance(pObject->entityIndex);
		else
			DevMsg("ERROR: The nearest object has not spwned in yet.\n");
	}

	if (!pEntity)
	{
		g_pAnarchyManager->AddToastMessage("You must be looking at an object to turn it into a ragdoll.");
		return;
	}

	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
	if (!pShortcut)
		return;

	g_pAnarchyManager->MakeRagdoll(pShortcut);
	//g_pAnarchyManager->RagdollInfo(pShortcut);
}
ConCommand makeragdoll("makeragdoll", MakeRagdoll, "Make the object under your crosshir a ragdoll, if it supports it.", FCVAR_NONE);

void RagdollInfo(const CCommand &args)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	int iEntityIndex = (args.ArgC() > 1) ? Q_atoi(args[1]) : g_pAnarchyManager->GetSelectorTraceEntityIndex();

	C_BaseEntity* pEntity = C_BaseEntity::Instance(iEntityIndex);
	if (!pEntity)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
			pEntity = C_BaseEntity::Instance(pObject->entityIndex);
		else
			DevMsg("ERROR: The nearest object has not spwned in yet.\n");
	}

	if (!pEntity)
	{
		g_pAnarchyManager->AddToastMessage("You must be looking at an object to turn it into a ragdoll.");
		return;
	}

	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(pEntity);
	if (!pShortcut)
		return;

	g_pAnarchyManager->RagdollInfo(pShortcut);
}
ConCommand ragdollinfo("ragdollinfo", RagdollInfo, "Internal use only.", FCVAR_NONE);

void InputModeOn()
{

	if (g_pAnarchyManager->IsPaused())
		return;

	// FIXME: Need to reject commands that are sent before the AArcade system is ready.
	//bool fullscreen = (args.ArgC() > 1);

	// if not spawning an object, do regular stuff
	if (!g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
	{
		bool bHandled = false;
		C_EmbeddedInstance* pSelectedEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		if (pSelectedEmbeddedInstance)
		{
			C_BaseEntity* pEntity = C_BaseEntity::Instance(pSelectedEmbeddedInstance->GetOriginalEntIndex());
			if (pEntity && pEntity == g_pAnarchyManager->GetSelectedEntity())
			{
				if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER) || vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT))
					g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);

				g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pSelectedEmbeddedInstance);// fullscreen);
				g_pAnarchyManager->RemoveLastHoverGlowEffect();
				bHandled = true;
			}
		}

		// SEND INPUT TO THE REMEMBER INSTANCE... (correct 90% of the time in this case)
		/*
		if ( !bHandled )
		{
		C_EmbeddedInstance* pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
		if (!pRememberedInstance)
		pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

		if (pRememberedInstance)
		{
		// CHECK IF THE ENTITY UNDER THE USER'S CROSSHAIR IS THIS INSTANCE...
		bool bIsEntityInstance = false;
		// TODO: work

		if (bIsEntityInstance)
		{
		C_BaseEntity* pEntity = C_BaseEntity::Instance(pRememberedInstance->GetOriginalEntIndex());
		if (pEntity)
		{
		g_pAnarchyManager->SelectEntity(pEntity);
		g_pAnarchyManager->GetInputManager()->SetTempSelect(true);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pRememberedInstance);
		bHandled = true;
		}
		}
		}
		}
		*/

		if (!bHandled)
		{
			// 1. Get the entity under the player's crosshair.

			//C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

			/*
			trace_t tr;
			g_pAnarchyManager->SelectorTraceLine(tr);
			//Vector forward;
			//pPlayer->EyeVectors(&forward);
			//UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

			C_BaseEntity *pEntity = (tr.DidHitNonWorldEntity()) ? tr.m_pEnt : null;
			*/

			C_BaseEntity* pEntity = C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex());

			// only prop shortcuts
			C_PropShortcutEntity* pShortcut = (pEntity) ? dynamic_cast<C_PropShortcutEntity*>(pEntity) : null;// && tr.fraction != 1.0
			if (pShortcut)
			{
				// 2. Is this entity used as any embedded instance's "original ent index"?
				int iEntityIndex = pShortcut->entindex();
				C_EmbeddedInstance* pLookingInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstanceByEntityIndex(iEntityIndex);
				if (pLookingInstance)
				{
					//bHandled = (g_pAnarchyManager->GetAutoCloseTasks()) ? g_pAnarchyManager->SelectEntity(pEntity) : g_pAnarchyManager->TempSelectEntity(iEntityIndex);
					if (g_pAnarchyManager->TempSelectEntity(iEntityIndex))
						bHandled = true;
				}

				if (!bHandled)
				{
					// 4. Otherwise, is this a slave screen?
					object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pShortcut->GetObjectId());
					if (pObject->slave)
					{
						// 5. If it is, then temp-select the remember instance for input.
						C_EmbeddedInstance* pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
						if (!pRememberedInstance)
							pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

						if (pRememberedInstance)
						{
							C_BaseEntity* pEntity = C_BaseEntity::Instance(pRememberedInstance->GetOriginalEntIndex());
							if (pEntity)
							{
								g_pAnarchyManager->SelectEntity(pEntity);
								g_pAnarchyManager->GetInputManager()->SetTempSelect(true);

								if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER) || vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT))
									g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
								g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pRememberedInstance);
								//g_pAnarchyManager->RemoveLastHoverGlowEffect();
								bHandled = true;
							}
						}
					}
				}
				/*
				if (!bHandled)
				{
				// 6. Quick-remember the entity if all else failed (but there IS an entity to select under the crosshair)
				if (g_pAnarchyManager->TempSelectEntity(iEntityIndex))
				bHandled = true;
				}*/
			}
		}

		if (!bHandled)
		{
			if (g_pAnarchyManager->GetRightFreeMouseToggle())
			{
				g_pAnarchyManager->ShowMouseMenu();
				bHandled = true;
			}
			else
			{
				C_EmbeddedInstance* pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
				if (!pRememberedInstance)
					pRememberedInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

				if (pRememberedInstance)
				{
					C_BaseEntity* pEntity = C_BaseEntity::Instance(pRememberedInstance->GetOriginalEntIndex());
					if (pEntity)
					{
						g_pAnarchyManager->SelectEntity(pEntity);
						g_pAnarchyManager->GetInputManager()->SetTempSelect(true);

						if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER) || vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT))
							g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
						g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pRememberedInstance);
						bHandled = true;
					}
				}
			}
		}
	}
	else
	{
		C_EmbeddedInstance* pSelectedEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		if (pSelectedEmbeddedInstance && pSelectedEmbeddedInstance->GetId() == "hud")
		{
			g_pAnarchyManager->GetInputManager()->SetInputCapture(true);

			C_AwesomiumBrowserInstance* pHudInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pSelectedEmbeddedInstance);
			std::vector<std::string> params;
			params.push_back("transform");
			pHudInstance->DispatchJavaScriptMethod("cmdListener", "switchMode", params);
		}
		//pSelectedEmbeddedInstance->
		//g_pAnarchyManager->GetInputManager()->ActivateInputMode(false, false, pSelectedEmbeddedInstance);// fullscreen);
		/*
		//g_pAnarchyManager->DeactivateObjectPlacementMode(false);

		// undo changes AND cancel
		C_PropShortcutEntity* pShortcut = g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity();
		g_pAnarchyManager->DeactivateObjectPlacementMode(false);

		//std::string id = pShortcut->GetObjectId();
		//g_pAnarchyManager->GetInstanceManager()->ResetObjectChanges(pShortcut);

		// "save" cha
		//m_pInstanceManager->ApplyChanges(id, pShortcut);
		DevMsg("CHANGES REVERTED\n");
		*/
	}
}

void InputModeOff()
{

	if (g_pAnarchyManager->IsPaused())
		return;

	if (!g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity())
	{
		if (!g_pAnarchyManager->GetInputManager()->IsTempSelect())
		{
			if (g_pAnarchyManager->GetInputManager()->GetInputMode())
			{
				g_pAnarchyManager->GetInputManager()->DeactivateInputMode();
				g_pAnarchyManager->BringToTop();
			}
			else if (g_pAnarchyManager->GetRightFreeMouseToggle())
				g_pAnarchyManager->ShowMouseMenu();
		}
		else
		{
			//g_pAnarchyManager->GetInputManager()->SetInputCapture(false);
			g_pAnarchyManager->TaskRemember();
			g_pAnarchyManager->GetInputManager()->SetTempSelect(false);

			freemousemode.SetValue(false);

			//if (!right_free_mouse_toggle.GetBool())// || !cvar->FindVar("freemousemode")->GetBool())
			//if (!cvar->FindVar("freemousemode")->GetBool())
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode();
			g_pAnarchyManager->BringToTop();
			//g_pAnarchyManager->DeselectEntity("", false);
		}
	}
	else
	{
		C_EmbeddedInstance* pSelectedEmbeddedInstance = g_pAnarchyManager->GetInputManager()->GetEmbeddedInstance();
		if (pSelectedEmbeddedInstance && pSelectedEmbeddedInstance->GetId() == "hud")
		{
			g_pAnarchyManager->GetInputManager()->SetInputCapture(false);

			C_AwesomiumBrowserInstance* pHudInstance = dynamic_cast<C_AwesomiumBrowserInstance*>(pSelectedEmbeddedInstance);
			std::vector<std::string> params;
			params.push_back("browse");
			pHudInstance->DispatchJavaScriptMethod("cmdListener", "switchMode", params);

			if (g_pAnarchyManager->GetInputManager()->GetMainMenuMode() && engine->IsInGame())
				engine->ClientCmd("gamemenucommand ResumeGame");
		}
	}
}

void ActivateInputMode(const CCommand &args)
{
	InputModeOn();
}
ConCommand activateinputmode("+input_mode", ActivateInputMode, "Turns ON input mode.", FCVAR_NONE);

void DeactivateInputMode(const CCommand &args)
{
	InputModeOff();
}
ConCommand deactivateinputmode("-input_mode", DeactivateInputMode, "Turns OFF input mode.", FCVAR_NONE);

void MoveObject(const CCommand &args)
{
	if (!engine->IsInGame())
		return;

	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1]))) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if (!pShortcut)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
		else
			DevMsg("ERROR: The nearest object has not spwned in yet.\n");
	}

	if (pShortcut)
	{
		// TODO: this is redundant code with what happens in c_awesomiumjshandler::moveobject.  consolidate.

		//g_pAnarchyManager->GetInstanceManager()->AdjustObjectRot(Q_atof(pitch.c_str()), Q_atof(yaw.c_str()), Q_atof(roll.c_str()));
		//g_pAnarchyManager->GetInstanceManager()->AdjustObjectOffset(Q_atof(offX.c_str()), Q_atof(offY.c_str()), Q_atof(offZ.c_str()));
		//g_pAnarchyManager->GetInstanceManager()->AdjustObjectScale(Q_atof(scale.c_str()));

		bool bOldGamepadInputMode = false;
		if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		{
			bOldGamepadInputMode = g_pAnarchyManager->GetInputManager()->IsGamepadInputMode();
			g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);
		}

		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetInstanceObject(pShortcut->GetObjectId());
		if (pObject && pObject->child)
			engine->ClientCmd(VarArgs("setparent %i;\n", pShortcut->entindex()));	// temporarily remove us from the group

		g_pAnarchyManager->GetInstanceManager()->AdjustTransformPseudo(true);
		g_pAnarchyManager->ActivateObjectPlacementMode(pShortcut, "move", bOldGamepadInputMode);
	}
}
ConCommand moveobject("moveobject", MoveObject, "Moves the object that is under your crosshair.", FCVAR_NONE);

void ResetPhysics(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
		return;

	if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1]))) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if ( pShortcut )
		g_pAnarchyManager->GetInstanceManager()->ResetPhysicsOnObject(pShortcut);
}
ConCommand resetphysics("resetphysics", ResetPhysics, "Reset's the physics object that is under your crosshair to it's original position & turns toggles OFF physics on it.", FCVAR_NONE);

void DeleteObject(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
		return;

	if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1]))) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if (pShortcut)
	{
		if (g_pAnarchyManager->GetSelectedEntity() == pShortcut)
			g_pAnarchyManager->DeselectEntity();

		g_pAnarchyManager->GetInstanceManager()->RemoveEntity(pShortcut);
	}
}
ConCommand deleteobject("deleteobject", DeleteObject, "Removes the object that is under your crosshair.", FCVAR_NONE);

void TaskCloseEntity(const CCommand &args)
{
	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1]))) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	C_EmbeddedInstance* pEmbeddedInstance;
	if (pShortcut)
	{
		std::string testTaskId = "auto" + pShortcut->GetItemId();
		pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstance(testTaskId);
		if (pEmbeddedInstance)
		{
			if (g_pAnarchyManager->GetSelectedEntity() && pEmbeddedInstance->GetOriginalEntIndex() == g_pAnarchyManager->GetSelectedEntity()->entindex())
					g_pAnarchyManager->DeselectEntity();
			else
				g_pAnarchyManager->GetCanvasManager()->CloseInstance(testTaskId);

			return;
		}
	}

	// otherwise, try to close the active mirrored instance
	pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->GetDisplayInstance();
	if (!pEmbeddedInstance)
		pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->GetFirstInstanceToDisplay();

	if (pEmbeddedInstance)
	{
		if (g_pAnarchyManager->GetSelectedEntity() && pEmbeddedInstance->GetOriginalEntIndex() == g_pAnarchyManager->GetSelectedEntity()->entindex())
			g_pAnarchyManager->DeselectEntity();
		else
			g_pAnarchyManager->GetCanvasManager()->CloseInstance(pEmbeddedInstance->GetId());

		return;
	}
}
ConCommand taskcloseentity("task_close_entity", TaskCloseEntity, "Close the task under your crosshair (or the currently auto-playing task if no active task is found under your crosshair.)", FCVAR_NONE);

void TaskCloseAll(const CCommand &args)
{
	g_pAnarchyManager->GetCanvasManager()->CloseAllInstances();
}
ConCommand taskcloseall("task_close_all", TaskCloseAll, "Close ALL running in-game tasks.", FCVAR_NONE);

void CloneObject(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
		return;

	if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex())) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if (!pShortcut)
	{
		float flMaxRange = -1;
		object_t* pObject = g_pAnarchyManager->GetInstanceManager()->GetNearestObjectToPlayerLook(NULL, flMaxRange);
		if (pObject->spawned)
			pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(pObject->entityIndex));
		else
			DevMsg("ERROR: The nearest object has not spwned in yet.\n");
	}

	if (pShortcut)
	{
		g_pAnarchyManager->GetInstanceManager()->AdjustTransformPseudo(true);
		g_pAnarchyManager->GetInstanceManager()->CloneObject(pShortcut);
	}
}
ConCommand cloneobject("cloneobject", CloneObject, "Clones the object that is under your crosshair.", FCVAR_NONE);

void MirrorObject(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObject())
		return;

	if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		g_pAnarchyManager->GetInputManager()->DeactivateInputMode(true);

	C_PropShortcutEntity* pShortcut = (args.ArgC() > 1) ? dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex())) : null;
	if (!pShortcut)
		pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(g_pAnarchyManager->GetSelectorTraceEntityIndex()));

	if (pShortcut)
		g_pAnarchyManager->SetSlaveScreen(pShortcut->GetObjectId(), !(pShortcut->GetSlave()));		
}
ConCommand mirrorobject("mirrorobject", MirrorObject, "Toggle mirror mode on the object that is under your crosshair.", FCVAR_NONE);

void CreateModelPreview(const CCommand &args)
{
	std::string modelName = "models/props/sithlord/beer.mdl";
	if (args.ArgC() > 1)
		modelName = args[1];

	g_pAnarchyManager->CreateModelPreview(modelName);
}
ConCommand createmodelpreview("create_model_preview", CreateModelPreview, "Creates a model preview.", FCVAR_NONE);

void InputModeToggle(const CCommand &args)
{
	if (g_pAnarchyManager->GetInputManager()->GetInputMode())
		InputModeOff();
	else
		InputModeOn();
}
ConCommand inputmodetoggle("input_mode_toggle", InputModeToggle, "Toggles input mode.", FCVAR_NONE);

void GenerateLegacyHash(const CCommand &args)
{
	std::string hash = g_pAnarchyManager->GenerateLegacyHash(args[1]);
	DevMsg("Legacy hash of the given string is: %s\n", hash.c_str());
}
ConCommand generatelegacyhash("generate_legacy_hash", GenerateLegacyHash, "Generate legacy hash based on the given string.", FCVAR_NONE);

void DetectBackpacks(const CCommand &args)
{
	//g_pAnarchyManager->GetBackpackManager()->DetectAllBackpacks();
}
ConCommand detectbackpacks("detect_backpacks", DetectBackpacks, "Scan the aarcade_user/custom folder for backpacks.", FCVAR_NONE);

void ShowWindowsTaskBar(const CCommand &args)
{
	g_pAnarchyManager->ShowWindowsTaskBar();
}
ConCommand showwindowstaskbar("show_windows_task_bar", ShowWindowsTaskBar, "Show's the window's task bar.", FCVAR_NONE);

void ResetImageLoader(const CCommand &args)
{
	g_pAnarchyManager->ResetImageLoader();
}
ConCommand resetimageloader("reset_image_loader", ResetImageLoader, "Reset the image loader, if it has bugged out.  Or you could just re-load the map to reset it..", FCVAR_NONE);

void GenerateKey(const CCommand &args)
{
	std::string key = g_pAnarchyManager->GenerateUniqueId();
	DevMsg("Generated new key is: %s\n", key.c_str());
}
ConCommand generatekey("generate_key", GenerateKey, "Generate a new key.", FCVAR_NONE);

void RemoteHolstered(const CCommand &args)
{
	if (g_pAnarchyManager->GetMetaverseManager()->GetSpawningObjectEntity() || g_pAnarchyManager->GetInputManager()->GetInputMode())
		g_pAnarchyManager->HandleUiToggle();
}
ConCommand remoteholstered("remote_holstered", RemoteHolstered, "Notifies the client that the remote was holstered.", FCVAR_NONE);

void HardPause(const CCommand &args)
{
	g_pAnarchyManager->HardPause();
}
ConCommand hardpause("hard_pause", HardPause, "Pauses AArcade and releases resources.", FCVAR_NONE);

void WakeUp(const CCommand &args)
{
	g_pAnarchyManager->WakeUp();
}
ConCommand wakeup("wake_up", WakeUp, "Wakes AArcade up and reacquires the resources.", FCVAR_NONE);

void SetBroadcastGame(const CCommand &args)
{
	std::string title = std::string(args[1]);
	g_pAnarchyManager->WriteBroadcastGame(title);
	//g_pAnarchyManager->xCastSetGameName();
	//g_pAnarchyManager->xCastSetLiveURL();
}
ConCommand setbroadcastgame("set_broadcast_game", SetBroadcastGame, "Sets the current broadcast game name.", FCVAR_NONE);

void AttemptSelectObject(const CCommand &args)
{
	//if (g_pAnarchyManager->GetIgnoreNextFire())
	//{
	//	g_pAnarchyManager->SetIgnoreNextFire(false);
	//	return;
	//}

	//if (!g_pAnarchyManager->GetLastHoverGlowEntity())
	//{
		if (args.ArgC() > 1)
		{
			bool bIgnoreSlave = (args.ArgC() > 2) ? Q_atoi(args[2]) : false;
			if (g_pAnarchyManager->AttemptSelectEntity(C_BaseEntity::Instance(Q_atoi(args[1])), bIgnoreSlave))
			{

				if (g_pAnarchyManager->GetSelectedEntity())
					g_pAnarchyManager->GetAccountant()->Action("aa_objects_selected", 1);
			}
		}
		else
		{
			if (g_pAnarchyManager->AttemptSelectEntity())
			{
				if (g_pAnarchyManager->GetSelectedEntity())
					g_pAnarchyManager->GetAccountant()->Action("aa_objects_selected", 1);

				if (broadcast_mode.GetBool())
					g_pAnarchyManager->xCastSetLiveURL();
			}
		}
	//}
}
ConCommand attemptselectobject("select", AttemptSelectObject, "Attempts to select the object under your crosshair.", FCVAR_NONE);

void SpecialGamepadInputMode(const CCommand &args)
{
	if (vgui::input()->IsKeyDown(KEY_XBUTTON_RTRIGGER) && !g_pAnarchyManager->GetInputManager()->GetInputMode() && g_pAnarchyManager->GetSelectedEntity())
		g_pAnarchyManager->GetInputManager()->SetGamepadInputMode(true);
}
ConCommand specialgamepadinputmode("specialgamepadinputmode", SpecialGamepadInputMode, "Interal use.", FCVAR_NONE);
/*
void TestEko(const CCommand &args)
{
	g_pAnarchyManager->GetCanvasManager()->CleanupTextures();
}
ConCommand testEko("testeko", TestEko, "tester func", FCVAR_NONE);
*/
void Launch( const CCommand &args )
{
	g_pAnarchyManager->GetLibretroManager()->CreateLibretroInstance();

	//g_pFullFileSystem->AddSearchPath(installFolder, "GAME", PATH_ADD_TO_TAIL);

	//std::string fullPath = VarArgs("%s\\", installFolder);

	//unsigned int uNumModels = 0;
	//unsigned int uNumItems = 0;
	//std::string id = VarArgs("%llu", details->m_nPublishedFileId);




	//g_pAnarchyManager->GetInstanceManager()->LoadLegacyInstance();




	/*
	g_pAnarchyManager->GetWebManager()->GetHudWebTab()->AddHudLoadingMessage("progress", "", "Importing Old AArcade Data", "importfolder", "0", "1", "0");
	std::string path = "A:\\SteamLibrary\\steamapps\\common\\Anarchy Arcade\\aarcade\\";
	g_pAnarchyManager->GetMetaverseManager()->LoadFirstLocalItemLegacy(true, path, "", "");
	g_pFullFileSystem->AddSearchPath(path.c_str(), "MOD", PATH_ADD_TO_TAIL);
	g_pFullFileSystem->AddSearchPath(path.c_str(), "GAME", PATH_ADD_TO_TAIL);
	//DevMsg("Loaded %u items from %s\n", uNumItems, path.c_str());
	*/












//	uNumItems = g_pAnarchyManager->GetMetaverseManager()->LoadAllLocalItemsLegacy(uNumModels, path, "", "");
	//			g_pFullFileSystem->AddSearchPath(installFolder, "MOD", PATH_ADD_TO_TAIL);
	//DevMsg("Loaded %u items from %s\n", uNumItems, path.c_str());

	//if( args.ArgC() < 2 )
//		return;
	/*
	C_PropSimpleImageEntity* pProp = NULL;
	pProp = dynamic_cast<C_PropSimpleImageEntity*>( C_BaseEntity::Instance( Q_atoi(args[1]) ) );

	if( !pProp )
	{
		DevMsg("Invalid entindex specified for activate command!\n");
		return;
	}

	pProp->OnUse();
	*/
}

ConCommand launch( "aa_activated", Launch, "Usage: aa_activated entindex" );

void DetectAllMaps(const CCommand &args)
{
	g_pAnarchyManager->GetMetaverseManager()->DetectAllMaps();
	//DevMsg("Detect all maps!\n");
}
ConCommand detectallmaps("detectallmaps", DetectAllMaps, "Usage: aa_activated entindex");

void SpawnObjects(const CCommand &args)
{
	g_pAnarchyManager->GetInstanceManager()->SpawnActionPressed();

	/*
	std::string instanceId = g_pAnarchyManager->GetInstanceId();
	if (instanceId != "")
	{
		std::string uri = "asset://ui/spawnItems.html?max=" + std::string(args[1]);

		C_AwesomiumBrowserInstance* pHudBrowserInstance = g_pAnarchyManager->GetAwesomiumBrowserManager()->FindAwesomiumBrowserInstance("hud");
		g_pAnarchyManager->GetAwesomiumBrowserManager()->SelectAwesomiumBrowserInstance(pHudBrowserInstance);
		pHudBrowserInstance->SetUrl(uri);
		g_pAnarchyManager->GetInputManager()->ActivateInputMode(true, false, pHudBrowserInstance);
	}
	*/
}
ConCommand spawnobjects("spawnobjects", SpawnObjects, "Usage: ...");

void SpawnObjectsDown(const CCommand &args)
{
	g_pAnarchyManager->OnSpawnObjectsButtonDown();
}
ConCommand spawnobjectsdown("+spawnobjects", SpawnObjectsDown, "Usage: Press this button to spawn nearby objects (if they aren't already spawned in.) Hold it down for 1 second to spawn ALL objects in instead.");

void SpawnObjectUp(const CCommand &args)
{
	g_pAnarchyManager->OnSpawnObjectsButtonUp();
}
ConCommand spawnobjectsup("-spawnobjects", SpawnObjectUp, "Usage: Interal.  Use +spawnobjects instead.");

void GamepadNotify(const CCommand &args)
{
	int mode = Q_atoi(args[1]);
	int state = Q_atoi(args[2]);

	if (mode == 0)
	{
		//if (state == 0)
			//g_pAnarchyManager->SetIgnoreNextFire(true);
		if (state == 1)
		{
			engine->ClientCmd("-attack; -attack2;");
			//g_pAnarchyManager->SetIgnoreNextFire(false);	// just in case +attack didn't fire some how
		}
	}
}
ConCommand gamepadnotify("gamepad_notify", GamepadNotify, "Usage: Allows AArcade to prevent engine errors when plugging or unplugging an xbox 360 gamepad.");

void SetToastText(const CCommand &args)
{
	g_pAnarchyManager->SetToastText(std::string(args[1]));
}
ConCommand settoasttext("set_toast_text", SetToastText, "Usage: ", FCVAR_HIDDEN);

void AddToastMessage(const CCommand &args)
{
	g_pAnarchyManager->AddToastMessage(std::string(args[1]));
}
ConCommand addtoastmessage("add_toast_message", AddToastMessage, "Usage: ", FCVAR_HIDDEN);

void ObjectVideoFilter(const CCommand &args)
{
	// get the object under the player's selector ray
	int iEntityIndex = (args.ArgC() > 1) ? Q_atoi(args[1]) : g_pAnarchyManager->GetSelectorTraceEntityIndex();
	if (iEntityIndex < 0)
		return;

	/*
	std::string itemId = pShortcut->GetItemId();
	C_PropShortcutEntity* pShortcut = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(iEntityIndex));
	if (!pShortcut)
		return;
	KeyValues* pItemKV = g_pAnarchyManager->GetMetaverseManager()->GetActiveKeyValues(g_pAnarchyManager->GetMetaverseManager()->GetLibraryItem(itemId));
	if (!pItemKV)*/

	//C_EmbeddedInstance* pEmbeddedInstance = g_pAnarchyManager->GetCanvasManager()->FindEmbeddedInstanceByEntityIndex(iEntityIndex);
	C_SteamBrowserInstance* pSteamBrowserInstance = g_pAnarchyManager->GetSteamBrowserManager()->FindSteamBrowserInstanceByEntityIndex(iEntityIndex);
	if (!pSteamBrowserInstance)
		return;

	if (pSteamBrowserInstance)
	{
		if (pSteamBrowserInstance->GetUseVideoFilters())
			pSteamBrowserInstance->SetUseVideoFilters(false);
		else
			pSteamBrowserInstance->SetUseVideoFilters(true);
	}
}
ConCommand objectvideofilter("object_video_filter", ObjectVideoFilter, "Usage: Look at an object with a web tab on it and then use this command to make it color-key out green as transparent. Transparency only shows on the SPRITE cabinets. Set video_chroma_light and video_chroma_dark before using this command to adjust the color keying.", FCVAR_NONE);


void NextSequenceReady(const CCommand &args)
{
	if (args.ArgC() < 2)
		return;

	//ConVar* pMPModeVar = cvar->FindVar("mp_mode");

	//if (pMPModeVar->GetBool())
	//	return;

	C_PropShortcutEntity* pHotlink = dynamic_cast<C_PropShortcutEntity*>(C_BaseEntity::Instance(Q_atoi(args[1])));
	if (pHotlink)
	{
		bool bHandleModelChange = false;
		if (args.ArgC() > 2)
		{
			if (Q_atoi(args[3]) == 1)
			{
				bHandleModelChange = true;

				//				DevMsg("In nextrdy, sequence is: %i\n", pHotlink->GetSequence());

				//if (pHotlink->GetSequence() > 0)
				//					pHotlink->SetSequence(1);

				//				pHotlink->SetCycle(0.0f);
				//				pHotlink->SetCycle(0.0f);
				//				pHotlink->ResetSequenceInfo();
			}
		}

		pHotlink->PlaySequenceRegular(args[2], bHandleModelChange);
	}
}
ConCommand nextsequenceready("nextsequenceready", NextSequenceReady, "Usage: Internal.");

void cam_cut(const CCommand &args)
{
	if (args.ArgC() > 1)
	{
		int camSlot = Q_atoi(args.Arg(1));
		std::string position = "0 0 0";
		std::string rotation = "0 0 0";
		if (vgui::input()->IsKeyDown(KEY_LALT) || vgui::input()->IsKeyDown(KEY_LCONTROL)) {
			DevMsg("SET camera %i \n", camSlot);
			if (args.ArgC() >= 8)
			{
				position = std::string(args.Arg(2)) + " " + std::string(args.Arg(3)) + " " + std::string(args.Arg(4));
				rotation = std::string(args.Arg(5)) + " " + std::string(args.Arg(6)) + " " + std::string(args.Arg(7));
			}
			else
			{
				Vector origin = C_BasePlayer::GetLocalPlayer()->EyePosition();
				position = VarArgs("%.10g %.10g %.10g", origin.x, origin.y, origin.z);

				QAngle angles = C_BasePlayer::GetLocalPlayer()->EyeAngles();
				rotation = VarArgs("%.10g %.10g %.10g", angles.x, angles.y, angles.z);
			}

			if (position != "" && rotation != "")
			{
				// save the info into the slot
				std::string cmd = VarArgs("camslot%i %s %s", camSlot, position.c_str(), rotation.c_str());
				engine->ClientCmd(cmd.c_str());
			}
		}
		else {
			if (camSlot == -1)
			{
				g_pAnarchyManager->ClearAttractMode();
			}
			else
			{
				DevMsg("USE camera %i \n", camSlot);

				std::string cvarName = VarArgs("camslot%i", camSlot);
				std::string inputStr = cvar->FindVar(cvarName.c_str())->GetString();

				// Find the first space
				size_t firstSpace = inputStr.find(' ');

				if (firstSpace != std::string::npos) {
					// Find the second space, starting the search from the position of the first space
					size_t secondSpace = inputStr.find(' ', firstSpace + 1);

					if (secondSpace != std::string::npos) {
						position = inputStr.substr(0, secondSpace);
						rotation = inputStr.substr(secondSpace + 1, std::string::npos);
						//cvar->FindVar("cabinet_attract_mode_active");

						int iTransitionType = cvar->FindVar("camcuttype")->GetInt();
						if (iTransitionType == 0)
						{
							//Vector origin = C_BasePlayer::GetLocalPlayer()->EyePosition();
							Vector goodPosition;
							UTIL_StringToVector(goodPosition.Base(), position.c_str());

							trace_t tr;
							UTIL_TraceLine(MainViewOrigin(), goodPosition, CONTENTS_SOLID, NULL, COLLISION_GROUP_NONE, &tr);//MASK_SOLID
							if (tr.fraction >= 0.9)
								iTransitionType = 1;
							else
								iTransitionType = 2;
						}

						std::string cmd = VarArgs("set_attract_mode_transform %s %s %i;", position.c_str(), rotation.c_str(), iTransitionType);
						engine->ClientCmd(cmd.c_str());
						cvar->FindVar("camcut_attract_mode_active")->SetValue(1);
						cvar->FindVar("attract_mode_active")->SetValue(1);
					}
				}
			}
		}
	}
}
ConCommand camCut("camcut", cam_cut, "Usage: Parameters are position then rotation.");

static const size_t KV_STRING_TABLE_LIMIT = 4 * 1024 * 1024; // 4 MB
void kvDumpKeys(const CCommand &args)
{
	/*DevMsg("==== KeyValues String Table Dump ====\n");

	// Get whichever lookup function KeyValues is currently using (classic or growable)
	const char *(*pGetStringForSymbol)(int) = KeyValues::CallGetStringForSymbol;
	if (!pGetStringForSymbol)
	{
		DevMsg("KeyValues string table not initialized yet.\n");
		return;
	}

	int count = 0;

	for (int i = 0;; ++i)
	{
		const char *name = pGetStringForSymbol(i);
		if (!name)
			break;

		DevMsg("[%05d] %s\n", i, name);
		++count;
	}

	DevMsg("---- Total key name symbols: %d ----\n", count);*/

	static unsigned int s_kvDebugCounter = 0;

	char debugName[64];
	Q_snprintf(debugName, sizeof(debugName), "__kv_debug_%u", s_kvDebugCounter++);

	// Force creation of a new symbol at the end of the current pool
	int symbol = KeyValues::CallGetSymbolForString(debugName, true);
	const char *stored = KeyValues::CallGetStringForSymbol(symbol);

	if (!stored)
	{
		DevMsg("kv_spaceleft: Failed to retrieve debug string from KeyValues table.\n");
		return;
	}

	// In classic mode, 'symbol' is effectively the byte offset into the 4MB pool.
	unsigned int offset = (unsigned int)symbol;
	unsigned int len = (unsigned int)(Q_strlen(stored) + 1); // include null terminator

	unsigned int used = offset + len;
	if (used > KV_STRING_TABLE_LIMIT)
		used = KV_STRING_TABLE_LIMIT;

	unsigned int remaining = (used < KV_STRING_TABLE_LIMIT)
		? (KV_STRING_TABLE_LIMIT - used)
		: 0;

	// Percentages
	float percentUsed = 0.0f;
	float percentRemaining = 0.0f;

	if (KV_STRING_TABLE_LIMIT > 0u)
	{
		percentUsed = ((float)used      * 100.0f) / (float)KV_STRING_TABLE_LIMIT;
		percentRemaining = ((float)remaining * 100.0f) / (float)KV_STRING_TABLE_LIMIT;
	}

	DevMsg("==== KeyValues String Table (classic) ====\n");
	DevMsg(" Approx used:        %u characters\n", used);
	DevMsg(" Approx remaining:   %u characters\n", remaining);
	DevMsg(" Total capacity:     %u characters (4 MB)\n", KV_STRING_TABLE_LIMIT);
	DevMsg(" Percent used:       %.2f%%\n", percentUsed);
	DevMsg(" Percent remaining:  %.2f%%\n", percentRemaining);
	DevMsg("==========================================\n");

	if (remaining < 256u * 1024u)
	{
		DevMsg(" WARNING: Less than 256 KB remaining in KeyValues string table!\n");
	}
}
ConCommand kv_dump_keys("kvdumpkeysusage", kvDumpKeys, "Lists all unique KeyValues key names currently registered in the global KeyValues string table.");

void cmd_pet_next(const CCommand &args)
{
	engine->ClientCmd("petplayasnext");
}
ConCommand cmdPetNext("cmd_pet_next", cmd_pet_next, "Usage: Jump to the next pet.");

void cmd_pet_prev(const CCommand &args)
{
	engine->ClientCmd("petplayasprev");
}
ConCommand cmdPetPrev("cmd_pet_prev", cmd_pet_prev, "Usage: Jump to the previous pet.");

void cmd_pet_target(const CCommand &args)
{
	engine->ClientCmd("pet_target");
}
ConCommand cmdPetTarget("cmd_pet_target", cmd_pet_target, "Usage: Sets the point you are looking at as the place 'follow' pets want to go to. (Set at your own feet to make them follow you instead.)");

void cmd_pet_remove_all(const CCommand &args)
{
	engine->ClientCmd("destroy_all_pets");
}
ConCommand cmdPetRemoveAll("cmd_pet_remove_all", cmd_pet_remove_all, "Usage: Unspawns all of the pets that are currently in this world.");

void cmd_pet_wear(const CCommand &args)
{
	engine->ClientCmd("pet_wear");
}
ConCommand cmdPetWear("cmd_pet_wear", cmd_pet_wear, "Usage: Attaches/detatches the prop you are looking at to your current play-as pet.");

void cmd_lookspot_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle_lookspot");
}
ConCommand cmdLookspotToggle("cmd_lookspot_toggle", cmd_lookspot_toggle, "Usage: Shows an arrow indicator at your avatar's feet and another indicator of where you are aiming. Useful in 3rd person view.");

void cmd_cam_collision_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle cam_collision");
}
ConCommand cmdCamCollisionToggle("cmd_cam_collision_toggle", cmd_cam_collision_toggle, "Usage: Toggles 3rd person camera collisions.");

void cmd_vgui_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle r_drawvgui 0 1");
}
ConCommand cmdVGUIToggle("cmd_vgui_toggle", cmd_vgui_toggle, "Usage: Turns on/off the UI layer of the engine. This is most useful for people that have ReShade Depth Effects active - as this is a way to temporarily re-enable the UI for things like hover titles.");

void cmd_task_shot(const CCommand &args)
{
	engine->ClientCmd("task_screenshot");
}
ConCommand cmdTaskShot("cmd_taskshot", cmd_task_shot, "Usage: Saves the last rendered frame of the active embedded task to the aarcade_user/taskshots folder.");

void cmd_camera_pin_toggle(const CCommand &args)
{
	engine->ClientCmd("camera_pin_toggle");
}
ConCommand cmdCameraPin("cmd_camerapin", cmd_camera_pin_toggle, "Usage: Toggle the 3rd person camera being pinned in space.");

void cmd_adopt_asset_menu(const CCommand &args)
{
	engine->ClientCmd("adopt_asset_menu");
}
ConCommand cmdAdoptAssetMenu("cmd_adopt_asset_menu", cmd_adopt_asset_menu, "Usage: Make an extra COPY of the asset files for a model or material into an adopted content folder.");

void cmd_always_animated_image_toggle(const CCommand &args)
{
	engine->ClientCmd("always_animated_image_toggle");
}
ConCommand cmdAlwaysAnimatedImageToggle("cmd_always_animated_image_toggle", cmd_always_animated_image_toggle, "Usage: Flys your camera to the previous nearest attact mode object.");

void cmd_manual_pause(const CCommand &args)
{
	engine->ClientCmd("manual_pause");
}
ConCommand cmdManualPause("cmd_manual_pause", cmd_manual_pause, "Usage: When AArcade is paused & in the background, it becomes optimized so that it doesn't lag your PC down.");

void cmd_reset_image_loader(const CCommand &args)
{
	engine->ClientCmd("reset_image_loader");
}
ConCommand cmdResetImageLoader("cmd_reset_image_loader", cmd_reset_image_loader, "Usage: If images start failing to load, you can reset the image loader here to fix it. (Or reload the map - either way works.)");

void cmd_input_mode_toggle(const CCommand &args)
{
	engine->ClientCmd("input_mode_toggle");
}
ConCommand cmdInputModeToggle("cmd_input_mode_toggle", cmd_input_mode_toggle, "Usage: Toggles you into input mode on the selected cabinet so that you can send input to it as if it were fullscreened.");

void cmd_fog_test(const CCommand &args)
{
	engine->ClientCmd("fog_test");
}
ConCommand cmdFogTest("cmd_fog_test", cmd_fog_test, "Usage: Test some fog.");

void cmd_restart_quest_system(const CCommand &args)
{
	engine->ClientCmd("restart_quest_system");
}
ConCommand cmdRestartQuestSystem("cmd_restart_quest_system", cmd_restart_quest_system, "Usage: Reset & restart the available quests in this world.");

void cmd_shadows_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle object_shadows 0 1; set_active_spawned_shadows;");
}
ConCommand cmdShadowsToggle("cmd_shadows_toggle", cmd_shadows_toggle, "Usage: Shadows of spawned objected can cause visual glitches or decrease performance, depending on the arcade.");

void cmd_legs(const CCommand &args)
{
	engine->ClientCmd("toggle cl_first_person_uses_world_model 0 1;");
}
ConCommand cmdLegs("cmd_legs", cmd_legs, "Usage: Give yourself legs even while in first person mode! However, having 1st person legs will cause your flashlight to act a little weird.");

void cmd_double_rainbow_mode(const CCommand &args)
{
	engine->ClientCmd("toggle avr 0 2");
}
ConCommand cmdDoubleRainbowMode("cmd_double_rainbow_mode", cmd_double_rainbow_mode, "Usage: Toggle sound-sensitive colored reflections on all cabinet screens.");

void cmd_morphmodel(const CCommand &args)
{
	engine->ClientCmd("morphmodel");
}
ConCommand cmdMorphmodel("cmd_morphmodel", cmd_morphmodel, "Usage: Morph yourself into what ever 3D model you are looking at.");

void cmd_resetmodel(const CCommand &args)
{
	engine->ClientCmd("resetmodel");
}
ConCommand cmdResetmodel("cmd_resetmodel", cmd_resetmodel, "Usage: Reset yourself back to the default player model.");

void cmd_toggle_perspective(const CCommand &args)
{
	engine->ClientCmd("toggle_perspective");
}
ConCommand cmdTogglePerspective("cmd_toggle_perspective", cmd_toggle_perspective, "Usage: Cycle between 1st and 3rd person camera modes.");

void cmd_cam_maya_toggle(const CCommand &args)
{
	engine->ClientCmd("cam_maya_toggle");
}
ConCommand cmdCamMayaToggle("cmd_cam_maya_toggle", cmd_cam_maya_toggle, "Usage: Toggles the camera's rotation in 3rd person mode. This allows you to look back at your camera with your avatar while in 3rd person mode.");

void cmd_sound_mute_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle volume 0 1");
}
ConCommand cmdSoundMuteToggle("cmd_sound_mute_toggle", cmd_sound_mute_toggle, "Usage: Toggle the Source engine volume on/off.");

void cmd_camera_auto_director_toggle(const CCommand &args)
{
	engine->ClientCmd("thirdperson; toggle fixed_camera_spectate_mode 0 1;");
}
ConCommand cmdCameraAutoDirectorToggle("cmd_camera_auto_director_toggle", cmd_camera_auto_director_toggle, "Usage: Use F5 screenshots as 3rd person spectator cameras that track your movement.");

void cmd_fly_toggle(const CCommand &args)
{
	engine->ClientCmd("noclip");
}
ConCommand cmdFlyToggle("cmd_fly_toggle", cmd_fly_toggle, "Usage: Fly through solid objects instead of walking.");

void cmd_overlay_bandw(const CCommand &args)
{
	engine->ClientCmd("toggle mat_yuv");
}
ConCommand cmdOverlayBandw("cmd_overlay_bandw", cmd_overlay_bandw, "Usage: Toggle black & white mode.  Very noire of you.");

void cmd_overlay_combine(const CCommand &args)
{
	engine->ClientCmd("toggle_overlay \"effects/combine_binocoverlay.vmt\"");
}
ConCommand cmdOverlayCombine("cmd_overlay_combine", cmd_overlay_combine, "Usage: Toggle combine camera overlay mode.");

void cmd_tiny_mode(const CCommand &args)
{
	engine->ClientCmd("tiny_mode_toggle;");
}
ConCommand cmdTinyMode("cmd_tiny_mode", cmd_tiny_mode, "Usage: Make yourself tiny.");

void cmd_avatar_menu(const CCommand &args)
{
	engine->ClientCmd("avatar_menu;");
}
ConCommand cmdAvatarMenu("cmd_avatar_menu", cmd_avatar_menu, "Usage: Change your avatar to one from a custom favorites list you've prepared.");

void cmd_wheel_menu(const CCommand &args)
{
	engine->ClientCmd("wheel_menu;");
}
ConCommand cmdWheelMenu("cmd_wheel_menu", cmd_wheel_menu, "Usage: Randomly choose one of the objects in front of you.");

void cmd_reflections_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle mat_fastspecular");
}
ConCommand cmdReflectionsToggle("cmd_reflections_toggle", cmd_reflections_toggle, "Usage: Toggles reflections.  This can fix over-bright props or hard-to-see screens.");

void cmd_hdr_lighting_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle mat_hdr_level 0 2; mat_autoexposure_max 1; toggle  mat_autoexposure_min 1 0.5;");
}
ConCommand cmdHdrLightingToggle("cmd_hdr_lighting_toggle", cmd_hdr_lighting_toggle, "Usage: Toggles HDR. HDR simulates your pupils adjusting to lighting conditions.  Turning it off can fix how some maps look.");

void cmd_movement_speed_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle cl_forwardspeed 450 100; cl_sidespeed 450 50; cl_backspeed 450 50;");
}
ConCommand cmdMovementSpeedToggle("cmd_movement_speed_toggle", cmd_movement_speed_toggle, "Usage: Slow your default movement speed.");

void cmd_mirror_everything_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle play_everywhere 0 1;");
}
ConCommand cmdMirrorEverythingToggle("cmd_mirror_everything_toggle", cmd_mirror_everything_toggle, "Usage: Temporarily make all screens behave as if they were video mirrors.");

void cmd_physics_toggle(const CCommand &args)
{
	engine->ClientCmd("togglephysics");
}
ConCommand cmdPhysicsToggle("cmd_physics_toggle", cmd_physics_toggle, "Usage: Temporarily toggles physics on the AArcade object under your crosshair.");

void cmd_look_at_me(const CCommand &args)
{
	engine->ClientCmd("look_at_me");
}
ConCommand cmdLookAtMe("cmd_look_at_me", cmd_look_at_me, "Usage: Temporarily toggles making the nearest object under your crosshair look at you.");

void cmd_spawn_objects(const CCommand &args)
{
	engine->ClientCmd("spawnobjects");
}
ConCommand cmdSpawnObjects("cmd_spawn_objects", cmd_spawn_objects, "Usage: Spawan nearby objects. Only needed if you use limited spawn settings or beta nodes, otherwise, not needed.");

void cmd_play_nearest_gif(const CCommand &args)
{
	engine->ClientCmd("play_nearest_gif");
}
ConCommand cmdPlayNearestGif("cmd_play_nearest_gif", cmd_play_nearest_gif, "Usage: Plays the nearest GIF item to where you are looking (if one exists.)");

void cmd_all_guns_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle_weapons;");
}
ConCommand cmdAllGunsToggle("cmd_all_guns_toggle", cmd_all_guns_toggle, "Usage: Toggles weapons mode. Make sure to set it to 6 or higher, otherwise weapon categories will overpower the toggle off command.");

void cmd_attract_camera_next(const CCommand &args)
{
	engine->ClientCmd("select_next;");
}
ConCommand cmdAttractCameraNext("cmd_attract_camera_next", cmd_attract_camera_next, "Usage: Flys your camera to the next nearest attact mode object.");

void cmd_attract_camera_prev(const CCommand &args)
{
	engine->ClientCmd("select_prev;");
}
ConCommand cmdAttractCameraPrev("cmd_attract_camera_prev", cmd_attract_camera_prev, "Usage: Flys your camera to the previous nearest attact mode object.");

void cmd_camera_slow_fly_mode_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle_camera_fly_mode;");
}
ConCommand cmdCameraSlowFlyModeToggle("cmd_camera_slow_fly_mode_toggle", cmd_camera_slow_fly_mode_toggle, "Usage: Shrinks you down (Tiny Mode), enables flymode, adjusts flymode movement speed, and rebinds the gamepad top shoulder buttons to be Move Up / Down instead.");

void cmd_vehicle_spawn(const CCommand &args)
{
	engine->ClientCmd("ch_createairboat");
}
ConCommand cmdVehicleSpawn("cmd_vehicle_spawn", cmd_vehicle_spawn, "Usage: Try not to spawn it stuck into the ground.");

void cmd_headcrab(const CCommand &args)
{
	engine->ClientCmd("npc_create npc_headcrab");
}
ConCommand cmdHeadcrab("cmd_headcrab", cmd_headcrab, "Usage: They are your buddies, but they don't move around so well.");

void cmd_sound_stop(const CCommand &args)
{
	engine->ClientCmd("stopsound");
}
ConCommand cmdSoundStop("cmd_sound_stop", cmd_sound_stop, "Usage: Stops all the sounds that are currently playing from the map.");

void cmd_developer_mode(const CCommand &args)
{
	engine->ClientCmd("toggle developer");
}
ConCommand cmdDeveloperMode("cmd_developer_mode", cmd_developer_mode, "Usage: Toggle develoepr mode.  Shows excessive debug information in the console & on-screen when enabled.");

void cmd_inspect(const CCommand &args)
{
	engine->ClientCmd("inspect_object");
}
ConCommand cmdInspect("cmd_inspect", cmd_inspect, "Usage: Grab / inspect the object nearest your crosshair.");

void cmd_radial_menu(const CCommand &args)
{
	engine->ClientCmd("radial_menu");
}
ConCommand cmdRadialMenu("cmd_radial_menu", cmd_radial_menu, "Usage: Open a radial menu of favorites & nearby objects. (Experimental)");

void cmd_unpaint_all(const CCommand &args)
{
	engine->ClientCmd("unpaintall");
}
ConCommand cmdUnpaintAll("cmd_unpaint_all", cmd_unpaint_all, "Usage: Resets ALL of your painted materials in your current instance.");

void cmd_unpaint(const CCommand &args)
{
	engine->ClientCmd("unpaint");
}
ConCommand cmdUnpaint("cmd_unpaint", cmd_unpaint, "Usage: Resets the painted material under your crosshair.");

void cmd_paint(const CCommand &args)
{
	engine->ClientCmd("paint");
}
ConCommand cmdPaint("cmd_paint", cmd_paint, "Usage: Paints the painted material under your crosshair.");

void cmd_material_pick(const CCommand &args)
{
	engine->ClientCmd("pick_texture");
}
ConCommand cmdMaterialPick("cmd_material_pick", cmd_material_pick, "Usage: Sets the texture under your crosshair as your active paint texture.");

void cmd_object_move(const CCommand &args)
{
	engine->ClientCmd("moveobject");
}
ConCommand cmdObjectMove("cmd_object_move", cmd_object_move, "Usage: Move the object that is under your crosshair.");

void cmd_object_clone(const CCommand &args)
{
	engine->ClientCmd("cloneobject");
}
ConCommand cmdObjectClone("cmd_object_clone", cmd_object_clone, "Usage: Clone the object that is under your crosshair.");

void cmd_object_delete(const CCommand &args)
{
	engine->ClientCmd("deleteobject");
}
ConCommand cmdObjectDelete("cmd_object_delete", cmd_object_delete, "Usage: Remove the object that is under your crosshair.");

void cmd_vehicle_remove_all(const CCommand &args)
{
	engine->ClientCmd("ent_remove_all prop_vehicle_airboat");
}
ConCommand cmdVehicleRemoveAll("cmd_vehicle_remove_all", cmd_vehicle_remove_all, "Usage: Removes all the vehicles that you already spawned into the world.");

void cmd_vehicle_menu(const CCommand &args)
{
	engine->ClientCmd("vehicle_menu");
}
ConCommand cmdVehicleMenu("cmd_vehicle_menu", cmd_vehicle_menu, "Usage: Brings up the vehicle menu.");

void cmd_task_close(const CCommand &args)
{
	engine->ClientCmd("task_close_entity");
}
ConCommand cmdTaskClose("cmd_task_close", cmd_task_close, "Usage: Close the task under your crosshair (or the currently auto-playing task if no active task is found under your crosshair.)");

void cmd_task_close_all(const CCommand &args)
{
	engine->ClientCmd("task_close_all");
}
ConCommand cmdTaskCloseAll("cmd_task_close_all", cmd_task_close_all, "Usage: Close ALL running in-game tasks.");

void cmd_wireframe_toggle(const CCommand &args)
{
	engine->ClientCmd("toggle mat_wireframe 0 3;");
}
ConCommand cmdWireframeToggle("cmd_wireframe_toggle", cmd_wireframe_toggle, "Usage: Toggle wireframe mode - useful for seeing through walls.");

/*
void(const CCommand &args)
{
	engine->ClientCmd("");
}
ConCommand("", , "Usage: ");
*/