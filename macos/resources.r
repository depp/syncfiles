#include "Controls.r"
#include "Dialogs.r"
#include "Finder.r"
#include "MacTypes.r"
#include "Processes.r"

#include "resources.h"

resource 'ALRT' (rAlrtError, purgeable) {
	{ 40, 40, 141, 360 },
	128, /* DITL ID */
	{
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
	},
	centerMainScreen
};

resource 'ALRT' (rAlrtFatal, purgeable) {
	{ 40, 40, 141, 360 },
	129, /* DITL ID */
	{
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
		OK, visible, sound1,
	},
	centerMainScreen
};

/* See Macintosh Human Interface Guidelines (1992) p. 197 "Basic Dialog Box Layout" */
/* Note that there are an extra 3px of margin for "free" on the dialog border. */
resource 'DITL' (128, purgeable) {{
	/* Quit button: 13px margin */
	{ 71, 240, 91, 310 },
	Button { enabled, "OK" },
	/* Error text: 13px margin, 3 lines @ 16px line height */
	{ 10, 75, 58, 310 },
	StaticText { enabled, "^0" },
	/* Icon: 13px vertical, 23px horizontal */
	{ 10, 20, 42, 52 },
	Icon { enabled, 0 },
}};

resource 'DITL' (129, purgeable) {{
	{ 71, 240, 91, 310 },
	Button { enabled, "Quit" },
	{ 10, 75, 58, 310 },
	StaticText { enabled, "^0" },
	{ 10, 20, 42, 52 },
	Icon { enabled, 0 },
}};

resource 'DLOG' (rDLOG_OpenFolder, purgeable) {
	/* Height += 31 */
	{0, 0, 197, 344},
	dBoxProc,
	invisible,
	noGoAway,
	0,
	130,
	"",
	noAutoCenter,
};

/* Open Folder, based on system -6042 */
resource 'DITL' (130, purgeable) {{
	/* Standard file (do not change, see IM: Files 3-18) */
	{135, 252, 155, 332},
	Button {enabled, "Open"},
	{104, 252, 124, 332},
	Button {enabled, "Cancel"},
	{0, 0, 0, 0},
	HelpItem {disabled, HMScanhdlg {-6042}},
	{8, 235, 24, 337},
	UserItem {enabled},
	{32, 252, 52, 332},
	Button {enabled, "Eject"},
	{60, 252, 80, 332},
	Button {enabled, "Desktop"},
	{29, 12, 159, 230},
	UserItem {enabled},
	{6, 12, 25, 230},
	UserItem {enabled},
	{91, 251, 92, 333},
	Picture {disabled, 11},
	/* Additions */
	{166, 252, 186, 332},
	Button {enabled, "Choose"},
	{166, 12, 186, 230},
	Button {enabled, "Choose Current Directory"},
}};

#ifdef MPW
resource 'SIZE' (-1) {
	dontSaveScreen,
	acceptSuspendResumeEvents,
	enableOptionSwitch,
	canBackground,
	multiFinderAware,
	backgroundAndForeground,
	dontGetFrontClicks,
	ignoreChildDiedEvents,
	is32BitCompatible,
	notHighLevelEventAware,
	onlyLocalHLEvents,
	notStationeryAware,
	dontUseTextEditServices,
	notDisplayManagerAware,
	reserved,
	reserved,
	60 * 1024,
	60 * 1024
};
#endif

type kCreator as 'STR ';
resource kCreator (0, purgeable) {
	APPNAME " 0.1 ©2022 Dietrich Epp"
};

resource 'vers' (1) {
	0x00, 0x01, development, 0x00,
	verUS,
	"0.1",
	"0.1, Copyright ©2021 Dietrich Epp"
};

resource 'FREF' (128, purgeable) {'APPL', 0, ""};
resource 'FREF' (129, purgeable) { kTypeProject, 1, ""};
resource 'BNDL' (128, purgeable) {
	kCreator,
	0,
	{
		'ICN#', {0, 128, 1, 129},
		'FREF', {0, 128, 1, 129},
	},
};

resource 'STR#' (rSTRS_Errors) {{
	"An error of type ^2 occurred.",
	"Error at: ^1:^2",
	"Assertion: ^1",
	"An internal error occurred.",
	"An unknown error occurred.",
	"Out of memory.",
	"Could not save project “^1”.",
	"Could not read project “^1”.",
	"The project file is damaged.",
	"The project data is too large.",
	"The project file is from an unknown version of SyncFiles.",
	"Could not query volume parameters.",
	"Could not create alias.",
	"Could not get directory path.",
	"Local project directory not found.",
	"Remote project directory not found.",
	"The volume is not mounted.",
}};

resource 'WIND' (rWIND_Project, preload, purgeable) {
	{32, 32, 256, 32 + 128*3},
	documentProc,
	invisible,
	goAway,
	0,
	"Untitled Project",
	staggerMainScreen,
};

resource 'CNTL' (rCNTL_ChooseFolder, preload, purgeable) {
	{10, 10, 30, 90},
	0,
	visible,
	1,
	0,
	pushButProc,
	0,
	"Choose…",
};
