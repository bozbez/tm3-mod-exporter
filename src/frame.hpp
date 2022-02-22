#pragma once

#include "common.hpp"
#include "input_panel.hpp"
#include "output_panel.hpp"
#include "export_thread.hpp"
#include "nvtt/nvtt.h"

#include <wx/wx.h>

#include <optional>
#include <filesystem>

class Frame : public wxFrame {
public:
	Frame(const wxString &title);

private:
	wxPanel *top_panel;

	InputPanel *input_panel;
	OutputPanel *output_panel;

	wxButton *export_button;
	ExportThread *export_thread;

	wxGauge *progress_bar;

	wxTextCtrl *log;
	std::optional<wxLogTextCtrl> log_target;

	std::optional<std::filesystem::path> input_dir;
	std::optional<std::filesystem::path> output_dir;

	std::wstring name = L"";

	FORMAT format = FORMAT_FOLDER;
	MODE mode = MODE_UNKNOWN;

	long long max_res = 4096;

	nvtt::Quality quality = nvtt::Quality_Normal;
	bool build_mipmaps = false;

	void OnInputChange(wxFileDirPickerEvent &event);
	void OnOuputChange(wxFileDirPickerEvent &event);
	void OnNameChange(wxCommandEvent &event);
	void OnFormatChoice(wxCommandEvent &event);
	void OnModeChoice(wxCommandEvent &event);
	void OnMaxResChoice(wxCommandEvent &event);
	void OnQualityChoice(wxCommandEvent &event);
	void OnBuildMipmapsChoice(wxCommandEvent &event);
	void OnExportPressed(wxCommandEvent &event);

	void OnExportFinished(wxCommandEvent &event);
	void OnExportProgress(wxCommandEvent &event);
	void OnExportProgressRange(wxCommandEvent &event);
	void OnExportProgressReset(wxCommandEvent &event);

	wxDECLARE_EVENT_TABLE();
};