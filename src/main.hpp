#pragma once

#include <optional>
#include <filesystem>

#include <nvtt/nvtt.h>

#include <wx/wx.h>
#include <wx/filepicker.h>
#include <wx/log.h>

enum ID {
	ID_INPUT_PICKER,
	ID_OUTPUT_PICKER,
	ID_EXPORT_BUTTON,
};

wxDEFINE_EVENT(EVT_EXPORT_FINISHED, wxThreadEvent);
wxDEFINE_EVENT(EVT_EXPORT_PROGRESS_RANGE, wxThreadEvent);
wxDEFINE_EVENT(EVT_EXPORT_PROGRESS, wxThreadEvent);

class ModExporter : public wxApp {
public:
	virtual bool OnInit();
};

class ExportThread;

class Frame : public wxFrame {
public:
	Frame(const wxString &title);

private:
	wxPanel *top_panel;

	wxPanel *input_panel;
	wxPanel *output_panel;

	wxButton *export_button;
	ExportThread *export_thread;

	wxGauge *progress_bar;

	wxTextCtrl *log;
	std::optional<wxLogTextCtrl> log_target;

	std::optional<std::filesystem::path> input_dir;
	std::optional<std::filesystem::path> output_dir;

	void OnInputChange(wxFileDirPickerEvent &event);
	void OnOuputChange(wxFileDirPickerEvent &event);
	void OnExportPressed(wxCommandEvent &event);

	void OnExportFinished(wxCommandEvent &event);
	void OnExportProgress(wxCommandEvent &event);
	void OnExportProgressRange(wxCommandEvent &event);

	wxDECLARE_EVENT_TABLE();
};

class ExportThread : public wxThread {
public:
	ExportThread(Frame *frame, std::filesystem::path input_dir,
		     std::filesystem::path output_dir);

private:
	nvtt::Context ctx{};
	
	Frame *frame;

	std::filesystem::path input_dir;
	std::filesystem::path output_dir;

	virtual ExitCode Entry();
};

class InputPanel : public wxPanel {
public:
	InputPanel(wxWindow *parent, wxWindowID id = wxID_ANY);
};

class OutputPanel : public wxPanel {
public:
	OutputPanel(wxWindow *parent, wxWindowID id = wxID_ANY);

private:
	wxDirPickerCtrl *output_picker;

	wxCheckBox *output_copy_checkbox;
	wxDirPickerCtrl *output_copy_picker;
};