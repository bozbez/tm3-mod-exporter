#include "common.hpp"
#include "input_panel.hpp"

#include <wx/filepicker.h>

InputPanel::InputPanel(wxWindow *parent, wxWindowID id) : wxPanel(parent, id)
{
	auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Input");

	auto input_picker = new wxDirPickerCtrl(sizer->GetStaticBox(), ID_INPUT_PICKER);
	sizer->Add(input_picker, wxSizerFlags().Expand().Border());

	SetSizer(sizer);
}