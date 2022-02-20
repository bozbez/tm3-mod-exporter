#include "frame.hpp"

#include <filesystem>
#include <wx/stdpaths.h>

std::filesystem::path GetDocumentsPath()
{
	return wxStandardPaths::Get().GetDocumentsDir().ToStdWstring();
}

std::filesystem::path GetSkinPath()
{
	return GetDocumentsPath().append("Trackmania\\Skins\\Models\\CarSport");
}

std::filesystem::path GetModPath()
{
	return GetDocumentsPath().append("Trackmania\\Skins\\Stadium\\ModWork");
}

bool IsSkinImage(const std::filesystem::path &image)
{
	std::vector<std::string> skin_image_filenames = {
		"Skin",
		"Details",
		"Wheels",
		"Glass",
	};

	for (auto test_name : skin_image_filenames) {
		if (image.stem().string().starts_with(test_name))
			return true;
	}

	return false;
}

bool IsModImage(const std::filesystem::path &image)
{
	std::vector<std::string> mod_image_filenames = {
		"ChronoCheckpoint", "DecalPlatform", "DecoHill", "OpenTech",
		"Platform",         "Road",          "Track",
	};

	for (auto test_name : mod_image_filenames) {
		if (image.stem().string().starts_with(test_name))
			return true;
	}

	return false;
}

MODE GuessMode(const std::filesystem::path &input)
{
	std::vector<std::filesystem::path> images;
	for (auto entry : std::filesystem::recursive_directory_iterator(input)) {
		if (!entry.is_regular_file())
			continue;

		auto path = entry.path();
		if (path.extension() == ".png" || path.extension() == ".PNG")
			images.push_back(path);
	}

	int num_skin = 0;
	int num_mod = 0;
	int num_unknown = 0;

	for (auto path : images) {
		if (IsSkinImage(path)) {
			num_skin++;
			continue;
		}

		if (IsModImage(path)) {
			num_mod++;
			continue;
		}

		num_unknown++;
	}

	wxLogMessage("Images - skin: %d, mod: %d, unknown: %d", num_skin, num_mod, num_unknown);

	if (num_skin > 0 && num_mod == 0)
		return MODE_SKIN;

	if (num_mod > 0)
		return MODE_MOD;

	return MODE_UNKNOWN;
}

Frame::Frame(const wxString &title)
	: wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
		  wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
{
	top_panel = new wxPanel(this);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	sizer->SetMinSize({480, 0});

	top_panel->SetSizer(sizer);

	input_panel = new InputPanel(top_panel, wxID_ANY);

	output_panel = new OutputPanel(top_panel, wxID_ANY);
	output_panel->Disable();

	export_button = new wxButton(top_panel, ID_EXPORT_BUTTON, "Export");
	export_button->Disable();

	progress_bar = new wxGauge(top_panel, wxID_ANY, 0);
	progress_bar->Disable();

	log = new wxTextCtrl(top_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
			     wxTE_RICH | wxTE_MULTILINE | wxTE_READONLY);
	log->SetMinSize({0, 192});

	wxFont::AddPrivateFont("fonts/DejaVuSansMono.ttf");
	wxTextAttr log_style{};
	log_style.SetFontFaceName("DejaVu Sans Mono");
	log_style.SetFontSize(8);

	log->SetDefaultStyle(log_style);

	log_target.emplace(log);
	wxLog::SetActiveTarget(&log_target.value());

	sizer->Add(input_panel, wxSizerFlags().Expand().Border());
	sizer->Add(output_panel, wxSizerFlags().Expand().Border());
	sizer->Add(export_button, wxSizerFlags().Expand().Border());
	sizer->Add(progress_bar, wxSizerFlags().Expand().Border());
	sizer->Add(log, wxSizerFlags().Expand().Border());

	sizer->Fit(top_panel);
	this->Fit();
}

void Frame::OnInputChange(wxFileDirPickerEvent &event)
{
	input_dir = event.GetPath().ToStdWstring();
	if (!std::filesystem::exists(input_dir.value()))  {
		output_panel->Disable();
		export_button->Disable();

		return;
	}

	output_panel->Enable();

	name = input_dir->filename();
	mode = GuessMode(input_dir.value());
	output_dir = std::filesystem::path("");

	if (mode == MODE_SKIN) {
		output_dir = GetSkinPath();
		format = FORMAT_ARCHIVE;
	} else if (mode == MODE_MOD) {
		output_dir = GetModPath();
		format = FORMAT_FOLDER;
	}

	output_panel->SetPath(output_dir.value());
	output_panel->SetName(name);
	output_panel->SetFormat(format);
	output_panel->SetMode(mode);

	export_button->Enable(output_dir != "");

	wxLogMessage("Guessed mode: %s",
		     mode == MODE_SKIN ? "skin" : (mode == MODE_MOD ? "mod" : "unknown"));
}

void Frame::OnOuputChange(wxFileDirPickerEvent &event)
{
	output_dir = event.GetPath().ToStdWstring();
	export_button->Enable(output_dir != "");
}

void Frame::OnNameChange(wxCommandEvent &event)
{
	name = event.GetString();
}

void Frame::OnFormatChoice(wxCommandEvent &event)
{
	format = static_cast<FORMAT>(reinterpret_cast<long long>(event.GetClientData()));
}

void Frame::OnModeChoice(wxCommandEvent &event)
{
	mode = static_cast<MODE>(reinterpret_cast<long long>(event.GetClientData()));

	output_dir = std::filesystem::path("");

	if (mode == MODE_SKIN) {
		output_dir = GetSkinPath();
		format = FORMAT_ARCHIVE;
	} else if (mode == MODE_MOD) {
		output_dir = GetModPath();
		format = FORMAT_FOLDER;
	}

	output_panel->SetFormat(format);
	output_panel->SetPath(output_dir.value());
	export_button->Enable(output_dir != "");
}

void Frame::OnMaxResChoice(wxCommandEvent &event)
{
	max_res = reinterpret_cast<long long>(event.GetClientData());
}

void Frame::OnExportPressed(wxCommandEvent &event)
{
	input_panel->Disable();
	output_panel->Disable();
	export_button->Disable();

	progress_bar->Enable();

	export_thread = new ExportThread(this, input_dir.value(), output_dir.value(), name, format,
					 max_res);
	export_thread->Run();
}

void Frame::OnExportFinished(wxCommandEvent &event)
{
	progress_bar->Disable();
	progress_bar->SetValue(-1);

	input_panel->Enable();
	output_panel->Enable();
	export_button->Enable();

	export_thread->Wait();
	delete export_thread;
}

void Frame::OnExportProgress(wxCommandEvent &event)
{
	progress_bar->SetValue(progress_bar->GetValue() + 1);
}

void Frame::OnExportProgressRange(wxCommandEvent &event)
{
	progress_bar->SetRange(event.GetInt());
}

void Frame::OnExportProgressReset(wxCommandEvent &event)
{
	progress_bar->SetValue(0);
}

/* clang-format off */
wxBEGIN_EVENT_TABLE(Frame, wxFrame)
	EVT_DIRPICKER_CHANGED(ID_INPUT_PICKER, Frame::OnInputChange)
	EVT_DIRPICKER_CHANGED(ID_OUTPUT_PICKER, Frame::OnOuputChange)
	EVT_TEXT(ID_NAME_TEXT, Frame::OnNameChange)
	EVT_CHOICE(ID_FORMAT_CHOICE, Frame::OnFormatChoice)
	EVT_CHOICE(ID_MODE_CHOICE, Frame::OnModeChoice)
	EVT_CHOICE(ID_MAX_RES_CHOICE, Frame::OnMaxResChoice)
	EVT_BUTTON(ID_EXPORT_BUTTON, Frame::OnExportPressed)
	
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_FINISHED, Frame::OnExportFinished)
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_PROGRESS, Frame::OnExportProgress)
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_PROGRESS_RANGE, Frame::OnExportProgressRange)
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_PROGRESS_RESET, Frame::OnExportProgressReset)
wxEND_EVENT_TABLE();
/* clang-format on */