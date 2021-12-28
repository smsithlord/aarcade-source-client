#include "cbase.h"

//#include "c_simple_image_entity.h"
//#include "c_webViewInput.h"
//#include "aa_globals.h"
#include "c_anarchymanager.h"

#include "../../../game/client/cdll_client_int.h"

#include "c_openglmanager.h"
#include "filesystem.h"
#include <algorithm>

#include "../client/hlvr/proxydll.h"
#include "client_virtualreality.h"//"iclientvirtualreality.h"
#include "sourcevr/isourcevirtualreality.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

ConVar screenshot_multiverse("screenshot_multiverse", "0", FCVAR_ARCHIVE);

ConVar nodraw_shortcuts("nodraw_shortcuts", "0", FCVAR_NONE, "When enabled, shortcuts will not be rendered.  Useful for taking screenshots.");

ConVar allowmultipleactive("allow_multiple_active", "0", FCVAR_ARCHIVE, "When enabled, multiple objects will be allowed to be considered ACTIVE at the same time - playing their animation & dynanmic effects.");

ConVar video_chroma_light("video_chroma_light", "0 255 0", FCVAR_NONE, "RGB of the lightest green to key out. (Used with object_video_filter.)");
ConVar video_chroma_dark("video_chroma_dark", "0 250 0", FCVAR_NONE, "RGB of the darkest green to key out. (Used with object_video_filter.)");
ConVar video_chroma_a1("video_chroma_a1", "1.0", FCVAR_NONE, "A1 float value. Range 0.5 to 1.5.");
ConVar video_chroma_a2("video_chroma_a2", "0.7", FCVAR_NONE, "A2 float value. Range 0.5 to 1.5.");

ConVar autobuildsoundcache("autobuildsoundcache", "1", FCVAR_ARCHIVE, "Disable this to decrease startup times.  The only side-effect is that maps with custom sounds might not play their audio.");
ConVar alwaysrefreshsnapshots("always_refresh_snapshots", "0", FCVAR_ARCHIVE, "When you deselect an item that has no explicit screen image, a snapshot of the last rendered frame is saved for the screen. This only happens once, unless you set this convar to 1 - then it'll always happen.");

ConVar always_animating_images("always_animating_images", "1", FCVAR_ARCHIVE, "Enable this to allow images marked as Animated Always to work.");

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

ConVar view_overlay("view_overlay", "", FCVAR_NONE, "Internal.  Keeps track of what material is being used on the screen overlay.");

ConVar useglobalrotation("use_global_rotation", "0", FCVAR_ARCHIVE, "When set to 1, the orientation of objects you are placing will take the transform menu's values literally.");

ConVar prioritize_legacy_workshop_images("prioritize_legacy_workshop_images", "0", FCVAR_ARCHIVE);

ConVar debug_object_spawn("debug_object_spawn", "0", FCVAR_NONE, "Set to 1 to have the ID of each object logged to the console prior to spawning them. Then, if you crash at a certain object ID, you can set it as your skip_objects ConVar value to skip it next time.");
ConVar skip_objects("skip_objects", "", FCVAR_ARCHIVE, "Comma-separated list of object IDs that you want to skip when loading maps.  Debug purposes only.");

ConVar node_model("node_model", "", FCVAR_ARCHIVE, "Override the default model used for nodes origin points.  Leave blank for default.");

ConVar painted_skyname("painted_skyname", "", FCVAR_HIDDEN, "Internal.  Used to remember the name of a painted sky.");

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
ConVar reshade("reshade", "0", FCVAR_NONE, "Internal. Read-only. Set with launcher.");
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
ConVar attract_mode_wipe("attract_mode_wipe", "0", FCVAR_ARCHIVE, "Do a before/after wipe when cutting between screenshots in attract mode.");

ConVar modelThumbs("model_thumbs_enabled", "1", FCVAR_ARCHIVE);

ConVar local_auto_playlists("local_auto_playlists", "1", FCVAR_ARCHIVE);

ConVar youtube_end_behavior("youtube_end_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_playlist_behavior("youtube_playlist_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_video_behavior("youtube_video_behavior", "default", FCVAR_ARCHIVE);
ConVar youtube_related("youtube_related", "default", FCVAR_ARCHIVE);
ConVar youtube_mixes("youtube_mixes", "1", FCVAR_ARCHIVE);
ConVar youtube_annotations("youtube_annotations", "0", FCVAR_ARCHIVE);

ConVar projector_fix("projector_fix", "1", FCVAR_ARCHIVE);
ConVar autoplay_enabled("autoplay_enabled", "1", FCVAR_ARCHIVE);

ConVar paint_texture("paint_texture", "shantzplacecss/zoeywhite", FCVAR_NONE);

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

	if (pShortcut)
	{
		g_pAnarchyManager->ActivateInspectObject(pShortcut);
	}
}
ConCommand inspect_object("inspect_object", InspectObject, "Usage: Inspect the object under your crosshair.", FCVAR_NONE);

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
}
ConCommand testerjoint("testerjoint", TesterJoint, "Usage: ");

void VROff(const CCommand &args)
{
	g_pAnarchyManager->VROff();
}
ConCommand vroff("vroff", VROff, "Usage: Forces VR off.  Warning: You can't re-enable VR after this until next time you launch AArcade.");

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

	engine->ClientCmd(VarArgs("donpcmove %02f %02f %02f\n", pos.x, pos.y, pos.z));
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
	if (g_pAnarchyManager->IsInitialized())
		g_pAnarchyManager->RunAArcade();
}
ConCommand main_menu("main_menu", MainMenu, "Usage: runs AArcade");	// used from Main Menu

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
				if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER))
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

								if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER))
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

						if (vgui::input()->IsKeyDown(KEY_XBUTTON_LTRIGGER))
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