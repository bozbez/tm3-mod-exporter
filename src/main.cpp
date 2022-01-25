#include "common.hpp"
#include "frame.hpp"

class ModExporter : public wxApp {
public:
	virtual bool OnInit();
};

bool ModExporter::OnInit()
{
	Frame *frame = new Frame("TM2020 Mod Exporter");
	frame->Show();

	return true;
}

wxIMPLEMENT_APP(ModExporter);

wxDEFINE_EVENT(EVT_EXPORT_FINISHED, wxThreadEvent);
wxDEFINE_EVENT(EVT_EXPORT_PROGRESS, wxThreadEvent);
wxDEFINE_EVENT(EVT_EXPORT_PROGRESS_RANGE, wxThreadEvent);
wxDEFINE_EVENT(EVT_EXPORT_PROGRESS_RESET, wxThreadEvent);