#ifndef C_LIBRETRO_INSTANCE_H
#define C_LIBRETRO_INSTANCE_H

#include "c_embeddedinstance.h"
#include "libretro.h"
#include "portaudio.h"
#include "c_inputlistenerlibretro.h"
#include <string>
#include <vector>
#include <map>

struct RunningLibretroCores_t
{
	volatile long count;
	std::string last_error;
	std::string last_msg;
};

struct libretro_raw {
	void(*set_environment)(retro_environment_t);
	void(*set_video_refresh)(retro_video_refresh_t);
	void(*set_audio_sample)(retro_audio_sample_t);
	void(*set_audio_sample_batch)(retro_audio_sample_batch_t);
	void(*set_input_poll)(retro_input_poll_t);
	void(*set_input_state)(retro_input_state_t);
	void(*init)(void);
	void(*deinit)(void);
	unsigned(*api_version)(void);
	void(*get_system_info)(struct retro_system_info * info);
	void(*get_system_av_info)(struct retro_system_av_info * info);
	void(*set_controller_port_device)(unsigned port, unsigned device);
	void(*reset)(void);
	void(*run)(void);
	size_t(*serialize_size)(void);
	bool(*serialize)(void* data, size_t size);
	bool(*unserialize)(const void* data, size_t size);
	void(*cheat_reset)(void);
	void(*cheat_set)(unsigned index, bool enabled, const char * code);
	bool(*load_game)(const struct retro_game_info * game);
	bool(*load_game_special)(unsigned game_type, const struct retro_game_info * info, size_t num_info);
	void(*unload_game)(void);
	unsigned(*get_region)(void);
	void* (*get_memory_data)(unsigned id);
	size_t(*get_memory_size)(unsigned id);

	// hardware acceleration stuff
	retro_hw_context_reset_t context_reset;
	retro_hw_context_reset_t context_destroy;
};

struct libretro_core_option {
	std::string name_internal;
	std::string name_display;
	std::string default_value;
	std::vector<std::string> values;
};

class C_LibretroInstance;

struct memory_map_t {
	//RETRO_MEMORY_RTC
	size_t rtcsize;
	uint8_t* rtcdata;

	//RETRO_MEMORY_SAVE_RAM
	size_t saveramsize;
	uint8_t* saveramdata;

	//RETRO_MEMORY_SYSTEM_RAM
	size_t systemramsize;
	uint8_t* systemramdata;

	//RETRO_MEMORY_VIDEO_RAM
	size_t videoramsize;
	uint8_t* videoramdata;
};

static const unsigned int AUDIO_RING_BUFFER_SAMPLES = 4096; // Power of 2, ~43ms at 48kHz stereo

struct AudioRingBuffer_t
{
	int16_t*      pBuffer;     // Heap-allocated interleaved stereo samples
	unsigned int  nCapacity;   // Always AUDIO_RING_BUFFER_SAMPLES
	unsigned int  nMask;       // nCapacity - 1 for fast modulo
	volatile long nWritePos;   // Producer cursor (monotonically increasing)
	volatile long nReadPos;    // Consumer cursor (monotonically increasing)
};

struct LibretroInstanceInfo_t
{
	RunningLibretroCores_t* runninglibretrocores;
	volatile int state;
	volatile bool paused;
	volatile bool reset;
	volatile bool close;
	void* hThreadDoneEvent;		// Windows Event HANDLE - signaled when worker thread exits
	volatile long bForceShutdown;	// Set by main thread after timeout to skip remaining core calls
	volatile bool bDidInit;			// True after raw->init() succeeds; gates unload_game/deinit during cleanup
	volatile bool bCallbacksRegistered;	// True after set_video_refresh/set_audio_* registered; deferred for mesen-like cores
	std::string id;
	volatile bool ready;
	volatile long readyfornextframe;	// 0 or 1, use InterlockedExchange for writes
	volatile long copyingframe;			// 0 or 1, use InterlockedExchange for writes
	volatile long readytocopyframe;		// 0 or 1, use InterlockedExchange for writes
	volatile bool coreloaded;
	volatile bool gameloaded;
	libretro_raw* raw;
	std::string corepath;
	std::string assetspath;
	std::string systempath;
	std::string savepath;
	CSysModule* module;
	uint threadid;
	C_LibretroInstance* libretroinstance;
	std::string core;
	std::string game;
	std::vector<libretro_core_option*> options;
	std::vector<char*> allocated_variable_strings; // Track allocated strings for RETRO_ENVIRONMENT_GET_VARIABLE to prevent memory leaks
	std::vector<retro_game_info_ext*> allocated_game_info_ext; // Track allocated game info ext structures
	void* lastframedata;
	size_t lastframebuffersize;		// Allocated size of lastframedata buffer for reuse
	unsigned int lastframewidth;
	unsigned int lastframeheight;
	size_t lastframepitch;
	retro_pixel_format videoformat;
	volatile bool optionshavechanged;
	PaStream* audiostream;
	AudioRingBuffer_t* pAudioRingBuffer;
	float samplerate;
	float outputsamplerate;        // Actual PortAudio output rate (0 = no resampling needed)
	double resampleAccumulator;    // Fractional source position carried between cbAudioSampleBatch calls
	float framerate;
	float lastrendered;
	float volume;

	// OpenGL hardware rendering (only used when AA_LIBRETRO_3D = true)
	void* gl_context;                    // Opaque pointer to LibretroGLContext

	// HW context fields used: context_type, context_reset, get_current_framebuffer,
	// get_proc_address, depth, stencil, version_major/minor, debug_context
	// TODO: bottom_left_origin, context_destroy

	const retro_controller_info* portdata;
	std::vector<int> currentPortTypes;
	unsigned int numports;
	KeyValues* libretrokeybinds;
	KeyValues* corekeybinds;
	KeyValues* gamekeybinds;
	KeyValues* inputstate;
	KeyValues* coreCoreOptions;
	KeyValues* gameCoreOptions;

	// hardware acceleration stuff
	retro_hw_context_type context_type;
	bool depth;
	bool stencil;
	bool bottom_left_origin;
	unsigned version_major;
	unsigned version_minor;
	bool cache_context;
	bool debug_context;

	// system info stuff
	std::string library_name;      // Descriptive name of library. Should not contain any version numbers, etc.
	std::string library_version;   // Descriptive version of core.
	std::string valid_extensions;  // A string listing probably content extensions the core will be able to load, separated with pipe. I.e. "bin|rom|iso". Typically used for a GUI to filter out extensions.
	bool need_fullpath;	// If true, retro_load_game() is guaranteed to provide a valid pathname in retro_game_info::path. ::data and ::size are both invalid. If false, ::data and ::size are guaranteed to be valid, but ::path might not be valid. This is typically set to true for libretro implementations that must load from file. Implementations should strive for setting this to false, as it allows the frontend to perform patching, etc.
	bool block_extract;	// If true, the frontend is not allowed to extract any archives before loading the real content. Necessary for certain libretro implementations that load games from zipped archives.

	// Content info override support (RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE)
	std::vector<retro_system_content_info_override> content_overrides;
	bool has_content_overrides;

	// Extended game info tracking (RETRO_ENVIRONMENT_GET_GAME_INFO_EXT)
	std::string loaded_full_path;        // Full path to the loaded content file
	std::string loaded_archive_path;     // Path to archive file (if file_in_archive = true)
	std::string loaded_archive_file;     // Name of file within archive
	std::string loaded_dir;              // Directory containing the content/archive
	std::string loaded_name;             // Canonical name (basename without extension)
	std::string loaded_ext;              // File extension in lowercase
	bool loaded_file_in_archive;         // True if content was extracted from an archive
	bool loaded_persistent_data;         // Whether data buffer remains valid after load_game
	const void* loaded_data;             // Pointer to loaded game data buffer (valid during load_game)
	size_t loaded_data_size;             // Size of loaded game data buffer

	size_t statesize;
	void* statedata;// = malloc(pitch*height);

	memory_map_t* memorymap;

	std::string prettycore;
	std::string prettygame;
	KeyValues* settings;

	bool soundAllowed;


};

class C_LibretroInstance : public C_EmbeddedInstance
{
public:
	C_LibretroInstance();
	~C_LibretroInstance();
	void SelfDestruct();

	std::string GetId() { return m_id; }

	void CleanUpTexture();

	void GoSomewhere(int iDirection);
	void GoPrevious();
	void GoNext();

	void OnMouseMove(float x, float y);
	KeyValues* GetOverlayKV() { return m_pOverlayKV; }
	void SetOverlay(std::string overlayId);
	void SaveOverlay(std::string type, std::string overlayId, float x, float y, float width, float height);
	void ClearOverlay(std::string type, std::string overlayId);
	void Init(std::string id, std::string title, int iEntIndex);
	bool CreateWorkerThread(std::string core);
	void Update();
	void TakeScreenshot(std::string nextTaskScreenshotName = "");
	void TakeScreenshotNow(ITexture* pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect, unsigned char* dest, unsigned int width, unsigned int height, unsigned int pitch, unsigned int depth);
	bool LoadCore(std::string coreFile = "");
	static bool LoadGame();
	void SetReset(bool bValue);
	void SetPause(bool bValue);
	void SetVolume(float fVolume);
	bool GetPause();
	void OnGameLoaded();
	void OnCoreLoaded();
	std::string GetLibretroCore();
	std::string GetLibretroFile();
	static bool BuildInterface(libretro_raw* raw, void* pLib);
	static void CreateAudioStream();
	static void DestroyAudioStream(LibretroInstanceInfo_t* info);
	std::string GetOriginalItemId() { return m_originalItemId; }
	int GetOriginalEntIndex() { return m_iOriginalEntIndex; }
	void SetOriginalItemId(std::string itemId) { m_originalItemId = itemId; }
	void SetOriginalEntIndex(int val) { m_iOriginalEntIndex = val; }

	bool HasInfo() { return (m_info != null); }
	std::vector<libretro_core_option*>& GetAllOptions() { return m_info->options; }	// others should always check if m_info exists first themselves!!

	bool IsSelected();
	bool HasFocus();
	bool Focus();
	bool Blur();
	bool Select();
	bool Deselect();

	void Close();
	void GetFullscreenInfo(float& fPositionX, float& fPositionY, float& fSizeX, float& fSizeY, std::string& overlayId);

	std::string GetURL() { return ""; }

	// callbacks
	static void cbMessage(enum retro_log_level level, const char * fmt, ...);
	static bool cbEnvironment(unsigned cmd, void* data);
	static void cbVideoRefresh(const void * data, unsigned width, unsigned height, size_t pitch);
	static void cbAudioSample(int16_t left, int16_t right);
	static size_t cbAudioSampleBatch(const int16_t * data, size_t frames);
	static void cbInputPoll(void);
	static int16_t cbInputState(unsigned port, unsigned device, unsigned index, unsigned id);

	void ResizeFrameFromRGB565(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void ResizeFrameFromRGB1555(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void ResizeFrameFromXRGB8888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth, bool bFlip = false);
	void ResizeFrameFromRGB888(const void* pSrc, void* pDst, unsigned int sourceWidth, unsigned int sourceHeight, size_t sourcePitch, unsigned int sourceDepth, unsigned int destWidth, unsigned int destHeight, size_t destPitch, unsigned int destDepth);
	void CopyLastFrame(unsigned char* dest, unsigned int width, unsigned int height, size_t pitch, unsigned int depth);

	bool IsDirty();
	void OnProxyBind(C_BaseEntity* pBaseEntity);
	void Render();
	void RegenerateTextureBits(ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pSubRect);
	C_EmbeddedInstance* GetParentSelectedEmbeddedInstance();

	void SaveLibretroKeybind(std::string type, unsigned int retroport, unsigned int retrodevice, unsigned int retroindex, unsigned int retrokey, std::string steamkey);
	void SaveLibretroOption(std::string type, std::string name_internal, std::string value);

	void ResetFastForwardSeconds();
	void OnSecondsUpdated();
	void FastForward(int iAmount, bool bAutoSkip = false);
	void Rewind(int iAmount, bool bAutoSkip = false);
	int CurrentSeconds();
	void SetAdjustedStartTime();
	int GetLibretroSeconds();
	int GetLibretroStartSeconds();

	// accessors
	int GetLastDelta() { return m_iLastDelta; }
	int GetAdjustedStartTime() { return m_iAdjustedStartTime; }
	int FastForwardSeconds() { return m_iFastForwardSeconds; }
	void GetLastMouse(float &fMouseX, float &fMouseY);
	std::string GetOverlayId() { return m_overlayId; }
	libretro_raw* GetRaw() { return m_raw; }
	LibretroInstanceInfo_t* GetInfo() { return m_info; }
	ITexture* GetTexture() { return m_pTexture; }
	int GetLastVisibleFrame() { return m_iLastVisibleFrame; }
	int GetLastRenderedFrame() { return m_iLastRenderedFrame; }
	C_InputListener* GetInputListener();
	std::string GetOriginalGame() { return m_originalGame; }
	std::string GetTitle() { return m_title; }
	bool GetShouldReopen() { return m_bShouldReopen; }
	bool GetFinishedResuming() { return m_bFinishedResuming; }

	// mutators
	void SetFastForwardSeconds(int iValue) { m_iFastForwardSeconds = iValue; }
	void SetLastDelta(int iVal) { m_iLastDelta = iVal; }
	bool SetGame(std::string file);
	void SetOriginalGame(std::string file);
	void SetTitle(std::string title) { m_title = title; }
	void SetShouldReopen(bool bValue) { m_bShouldReopen = bValue; }
	void MarkAsDirty() { m_bIsDirty = true; }
	void SetFinishedResuming(bool bValue) { m_bFinishedResuming = bValue; }

private:
	bool m_bTakeScreenshot;
	ConVar* m_pLocalVideoBehaviorConVar;
	bool m_bGotTime;
	int m_iLastDelta;
	bool m_bFinishedResuming;
	int m_iFastForwardSeconds;
	int m_iAdjustedStartTime;
	bool m_bIsDirty;
	ConVar* m_pProjectorFixConVar;
	float m_fLastMouseX;
	float m_fLastMouseY;
	bool m_bShouldReopen;
	std::string m_overlayId;
	KeyValues* m_pOverlayKV;
	std::string m_originalGame;
	std::string m_originalGameHash;
	int m_iLastVisibleFrame;
	ITexture* m_pTexture;
	int m_iLastRenderedFrame;
	std::string m_title;
	std::string m_id;
	std::string m_originalItemId;
	libretro_raw* m_raw;
	LibretroInstanceInfo_t* m_info;
	int m_iOriginalEntIndex;
};

#endif