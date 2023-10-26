#pragma once

#include <wx/wx.h>
#include <set>

enum ID {
	ID_INPUT_PICKER,
	ID_OUTPUT_PICKER,
	ID_NAME_TEXT,
	ID_FORMAT_CHOICE,
	ID_MODE_CHOICE,
	ID_MAX_RES_CHOICE,
	ID_QUALITY_CHOICE,
	ID_BUILD_MIPMAPS_CHOICE,
	ID_EXPORT_BUTTON,
};

enum FORMAT {
	FORMAT_FOLDER = 0,
	FORMAT_ARCHIVE,
};

enum MODE {
	MODE_UNKNOWN = 0,
	MODE_SKIN,
	MODE_MOD,
};

static const std::set<std::string> input_extensions = {
	".png", ".PNG", ".jpg", ".JPG", ".jpeg", ".JPEG",
};

wxDECLARE_EVENT(EVT_EXPORT_FINISHED, wxThreadEvent);
wxDECLARE_EVENT(EVT_EXPORT_PROGRESS, wxThreadEvent);
wxDECLARE_EVENT(EVT_EXPORT_PROGRESS_RANGE, wxThreadEvent);
wxDECLARE_EVENT(EVT_EXPORT_PROGRESS_RESET, wxThreadEvent);