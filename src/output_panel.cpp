#include "output_panel.hpp"

OutputPanel::OutputPanel(wxWindow *parent, wxWindowID id) : wxPanel(parent, id)
{
	auto box = new wxStaticBoxSizer(wxVERTICAL, this, "Output");
	auto sizer = new wxFlexGridSizer(2, 2, 0, 0);

	output_picker = new wxDirPickerCtrl(box->GetStaticBox(), ID_OUTPUT_PICKER, wxEmptyString,
					    wxDirSelectorPromptStr, wxDefaultPosition,
					    wxDefaultSize, wxDIRP_USE_TEXTCTRL);

	auto name_text_label = new wxStaticText(box->GetStaticBox(), wxID_ANY, "Name");
	name_text = new wxTextCtrl(box->GetStaticBox(), ID_NAME_TEXT);

	auto format_choice_label = new wxStaticText(box->GetStaticBox(), wxID_ANY, "Format");
	format_choice = new wxChoice(box->GetStaticBox(), ID_FORMAT_CHOICE);

	format_choice->Append("Folder", reinterpret_cast<void *>(FORMAT_FOLDER));
	format_choice->Append("Archive", reinterpret_cast<void *>(FORMAT_ARCHIVE));

	format_choice->SetSelection(0);

	auto mode_choice_label = new wxStaticText(box->GetStaticBox(), wxID_ANY, "Mode");
	mode_choice = new wxChoice(box->GetStaticBox(), ID_MODE_CHOICE);

	mode_choice->Append("Unknown", reinterpret_cast<void *>(MODE_UNKNOWN));
	mode_choice->Append("Car skin", reinterpret_cast<void *>(MODE_SKIN));
	mode_choice->Append("Map mod", reinterpret_cast<void *>(MODE_MOD));

	mode_choice->SetSelection(0);

	auto max_res_choice_label =
		new wxStaticText(box->GetStaticBox(), wxID_ANY, "Maximum resolution");
	max_res_choice = new wxChoice(box->GetStaticBox(), ID_MAX_RES_CHOICE);

	max_res_choice->Append("None", reinterpret_cast<void *>(0));
	for (long long i = 8192; i >= 256; i /= 2)
		max_res_choice->Append(std::format("{}px", i), reinterpret_cast<void *>(i));

	max_res_choice->SetSelection(max_res_choice->FindString("4096px"));

	sizer->Add(name_text_label, wxSizerFlags().Border(wxRIGHT | wxTOP | wxBOTTOM));
	sizer->Add(format_choice_label, wxSizerFlags().Border(wxLEFT | wxTOP | wxBOTTOM));

	sizer->Add(name_text, wxSizerFlags().Expand().Border(wxRIGHT | wxBOTTOM));
	sizer->Add(format_choice, wxSizerFlags().Expand().Border(wxLEFT | wxBOTTOM));

	sizer->Add(mode_choice_label, wxSizerFlags().Border(wxRIGHT | wxTOP | wxBOTTOM));
	sizer->Add(max_res_choice_label, wxSizerFlags().Border(wxLEFT | wxTOP | wxBOTTOM));

	sizer->Add(mode_choice, wxSizerFlags().Expand().Border(wxRIGHT | wxBOTTOM));
	sizer->Add(max_res_choice, wxSizerFlags().Expand().Border(wxLEFT | wxBOTTOM));

	sizer->AddGrowableCol(0);
	sizer->AddGrowableCol(1);

	box->Add(output_picker, wxSizerFlags().Expand().Border());
	box->Add(sizer, wxSizerFlags().Expand().Border());

	box->GetStaticBox()->SetSizerAndFit(sizer);
	SetSizerAndFit(box);
}

void OutputPanel::SetPath(const std::filesystem::path &path)
{
	output_picker->SetPath(path.c_str());
}

void OutputPanel::SetFormat(FORMAT format)
{
	format_choice->SetSelection(format);
}

void OutputPanel::SetName(const std::wstring &name)
{
	name_text->SetValue(name);
}

void OutputPanel::SetMode(MODE mode)
{
	mode_choice->SetSelection(mode);
}