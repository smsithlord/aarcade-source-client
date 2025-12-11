//#ifndef VROKAY
//#define VROKAY 1
//#endif

/*
#ifndef VR_ALLOWED
#define VR_ALLOWED 1
#endif
*/

/*
#ifndef AA_USE_GENERATED_CODE
#define AA_USE_GENERATED_CODE 1
#endif
*/

//#ifndef MAPBASE
//#define MAPBASE 1
//#endif

#ifndef USETHREADEDVR
#define USETHREADEDVR false
#endif //USETHREADEDVR

#define VR_BACK_BUFFER_0 "_rt_two_eyes_0"
#define VR_BACK_BUFFER_1 "_rt_two_eyes_1"
#define VR_BACK_BUFFER_2 "_rt_two_eyes_2"
#define VR_SPECTATOR_CAMERA "_rt_vr_spectator"

#ifndef SBSOKAY
#define SBSOKAY 1
#endif

#ifndef AA_MAX_STRING
#define AA_MAX_STRING 1024
#endif

#ifndef AA_LIBRETRO_3D
#define AA_LIBRETRO_3D false
#endif

#ifndef AA_EMBEDDED_INSTANCE_WIDTH
#define AA_EMBEDDED_INSTANCE_WIDTH 1280
#endif

#ifndef AA_EMBEDDED_INSTANCE_HEIGHT
#define AA_EMBEDDED_INSTANCE_HEIGHT 720
#endif

#ifndef AA_HUD_INSTANCE_WIDTH
#define AA_HUD_INSTANCE_WIDTH 1920
#endif

#ifndef AA_HUD_INSTANCE_HEIGHT
#define AA_HUD_INSTANCE_HEIGHT 1080
#endif

#ifndef AA_ATLAS_INSTANCE_WIDTH
#define AA_ATLAS_INSTANCE_WIDTH 1280
#endif

#ifndef AA_ATLAS_INSTANCE_HEIGHT
#define AA_ATLAS_INSTANCE_HEIGHT 720
#endif

#ifndef AA_NETWORK_INSTANCE_WIDTH
#define AA_NETWORK_INSTANCE_WIDTH 32
#endif

#ifndef AA_NETWORK_INSTANCE_HEIGHT
#define AA_NETWORK_INSTANCE_HEIGHT 32
#endif

#ifndef AA_MASTER_INSTANCE_WIDTH
#define AA_MASTER_INSTANCE_WIDTH 32
#endif

#ifndef AA_MASTER_INSTANCE_HEIGHT
#define AA_MASTER_INSTANCE_HEIGHT 32
#endif

#ifndef AA_THUMBNAIL_SIZE
#define AA_THUMBNAIL_SIZE 512
#endif

#ifndef AA_MODEL_THUMB_SIZE
#define AA_MODEL_THUMB_SIZE 256
#endif

#ifndef AA_PLATFORM_ID
#define AA_PLATFORM_ID "-KJvcne3IKMZQTaG7lPo"
#endif

#ifndef AA_DEFAULT_TYPEID
#define AA_DEFAULT_TYPEID ""//"-KKa1MHJTls2KqNphWFM"
#endif

#ifndef AA_CLIENT_VERSION
#define AA_CLIENT_VERSION 1.2	// THIS is what to increment every time new default items are added to the library.
#endif

#ifndef AA_LIBRARY_VERSION
#define AA_LIBRARY_VERSION 2	// Internal format of the library SQL.  This should RARELY EVER change - and when it does, AArcade must be told how to migrate between the previous version and the new version.
#endif

#ifndef AA_IMPORT_INFO
#define AA_IMPORT_INFO
#include <string>
#include <vector>

enum aaImportType
{
	AAIMPORT_NONE = 0,
	AAIMPORT_MODELS = 1
};

enum aaImportStatus
{
	AAIMPORTSTATUS_NONE = 0,
	AAIMPORTSTATUS_WAITING_TO_START = 1,
	AAIMPORTSTATUS_WORKING = 2,
	AAIMPORTSTATUS_COMPLETE = 3,
	AAIMPORTSTATUS_ABORTED = 4,
	AAIMPORTSTATUS_WAITING_FOR_PROCESSING = 5,
	AAIMPORTSTATUS_PROCESSING = 6,
	AAIMPORTSTATUS_WAITING_FOR_ADDING = 7,
	AAIMPORTSTATUS_ADDING = 8
};

struct importInfo_t {
	unsigned int count;
	aaImportType type;
	aaImportStatus status;
	std::vector<std::string> data;
	std::vector<unsigned int> duplicates;
};


/*
namespace ControllerTypes
{
	enum _ControllerTypes
	{
		controllerType_NA = 0,
		controllerType_XBox = 1,
		controllerType_Virtual = 2,
	};
};

namespace ButtonsList
{
	enum _ButtonsList
	{
		left_Menu,
		left_Trigger,
		left_Bumper,
		left_ButtonA,
		left_ButtonB,
		left_Pad,
		left_PadXAxis,
		left_PadYAxis,
		left_DPad_Up,
		left_DPad_Down,
		left_DPad_Left,
		left_DPad_Right,

		right_Menu,
		right_Trigger,
		right_Bumper,
		right_ButtonA,
		right_ButtonB,
		right_Pad,
		right_PadXAxis,
		right_PadYAxis,
		right_DPad_Up,
		right_DPad_Down,
		right_DPad_Left,
		right_DPad_Right
	};
};
*/

/*
//namespace ButtonsList
//{
enum ButtonsList
{
	left_Menu,
	left_Trigger,
	left_Bumper,
	left_ButtonA,
	left_ButtonB,
	left_Pad,
	left_PadXAxis,
	left_PadYAxis,
	left_DPad_Up,
	left_DPad_Down,
	left_DPad_Left,
	left_DPad_Right,

	right_Menu,
	right_Trigger,
	right_Bumper,
	right_ButtonA,
	right_ButtonB,
	right_Pad,
	right_PadXAxis,
	right_PadYAxis,
	right_DPad_Up,
	right_DPad_Down,
	right_DPad_Left,
	right_DPad_Right
};
//};

//namespace ControllerTypes
//{
enum ControllerTypes
{
	controllerType_NA = 0,
	controllerType_XBox = 1,
	controllerType_Virtual = 2,
};
//};
*/

#endif

/*
#ifndef AA_LIBRETRO_PATH
#define AA_LIBRETRO_PATH "D:\\Projects\\AArcade-Source\\game\\frontend\\libretro\\cores"
#endif
*/
//#include "cbase.h"

//#include "c_liveView.h"
//#include "c_anarchymanager.h"
/*
struct DynamicImage {
	int status;
	std::string file;
	ITexture* texture;
//	C_LiveView* ownedBy;
};
*/

//#include <vector>
//#include <string>
//#include "cbase.h"