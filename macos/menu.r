#include "Menus.r"

#include "resources.h"

resource 'MBAR' (rMBAR_Main) {{
	rMENU_Apple,
	rMENU_File,
	rMENU_Edit,
	rMENU_Project,
}};

resource 'MENU' (rMENU_Apple, preload) {
	rMENU_Apple,
	textMenuProc,
	0b01111111111111111111111111111101,
	enabled,
	apple,
	{
		"About " APPNAME "…", noIcon, noKey, noMark, plain,
		"-",                noIcon, noKey, noMark, plain,
	}
};

resource 'MENU' (rMENU_File, preload) {
	rMENU_File,
	textMenuProc,
	0b01111111111111111111111101111011,
	enabled,
	"File",
	{
		"New",              noIcon, "N",   noMark, plain,
		"Open…",            noIcon, "O",   noMark, plain,
		"-",                noIcon, noKey, noMark, plain,
		"Close",            noIcon, "W",   noMark, plain,
		"Save",             noIcon, "S",   noMark, plain,
		"Save As…",         noIcon, noKey, noMark, plain,
		"Revert",           noIcon, noKey, noMark, plain,
		"-",                noIcon, noKey, noMark, plain,
		"Quit",             noIcon, "Q",   noMark, plain,
	}
};

resource 'MENU' (rMENU_Edit, preload) {
	rMENU_Edit,
	textMenuProc,
	0b01111111111111111111111111111101,
	enabled,
	"Edit",
	{
		"Undo",             noIcon, "Z",   noMark, plain,
		"-",                noIcon, noKey, noMark, plain,
		"Cut",              noIcon, "X",   noMark, plain,
		"Copy",             noIcon, "C",   noMark, plain,
		"Paste",            noIcon, "V",   noMark, plain,
		"Clear",            noIcon, noKey, noMark, plain,
	}
};

resource 'MENU' (rMENU_Project, preload) {
	rMENU_Project,
	textMenuProc,
	0b01111111111111111111111111111111,
	enabled,
	"Project",
	{
		"Upload Files",		noIcon, "U",   noMark, plain,
		"Download Files",	noIcon, "D",   noMark, plain,
	}
};
