#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include "main.hpp"
#include "wx/event.h"

// B        BC1
// R        BC5
// I        BC3
// N        BC5
// AO       BC1
// DirtMask BC4
// D        BC1 BC3

using namespace std::string_literals;

std::optional<nvtt::Format> GuessFormat(const std::filesystem::path &input,
					const nvtt::Surface &image)
{
	auto stem = input.stem().wstring();

	if (stem.ends_with(L"_B")) {
		return nvtt::Format_BC3;
	} else if (stem.ends_with(L"_R")) {
		return nvtt::Format_BC5;
	} else if (stem.ends_with(L"_I")) {
		return nvtt::Format_BC3;
	} else if (stem.ends_with(L"_N")) {
		return nvtt::Format_BC5;
	} else if (stem.ends_with(L"_AO")) {
		return nvtt::Format_BC1;
	} else if (stem.ends_with(L"_DirtMask")) {
		return nvtt::Format_BC4;
	} else if (stem.ends_with(L"_D")) {
		if (image.alphaMode() == nvtt::AlphaMode_None)
			return nvtt::Format_BC1;
		else
			return nvtt::Format_BC3;
	}

	return std::nullopt;
}

std::string FormatToString(nvtt::Format format)
{
	switch (format) {
	case nvtt::Format_BC1:
		return "BC1"s;
	case nvtt::Format_BC1a:
		return "BC1a"s;
	case nvtt::Format_BC2:
		return "BC2"s;
	case nvtt::Format_BC3:
		return "BC3"s;
	case nvtt::Format_BC3_RGBM:
		return "BC3_RGBM"s;
	case nvtt::Format_BC3n:
		return "BC3n"s;
	case nvtt::Format_BC4:
		return "BC4"s;
	case nvtt::Format_BC4S:
		return "BC4S"s;
	case nvtt::Format_BC5:
		return "BC5"s;
	case nvtt::Format_BC5S:
		return "BC5S"s;
	case nvtt::Format_BC7:
		return "BC7"s;
	default:
		return "Unknown"s;
	}
}

std::vector<nvtt::Surface> BuildMipmapChain(nvtt::Surface image)
{
	std::vector<nvtt::Surface> chain;
	chain.push_back(image);

	while (chain.back().canMakeNextMipmap()) {
		chain.emplace_back(chain.back());
		chain.back().buildNextMipmap(nvtt::MipmapFilter_Kaiser);
	}

	return chain;
}

void CompressImage(nvtt::Context &ctx, const std::filesystem::path input,
		   const std::filesystem::path output)
{
	nvtt::Surface image;
	if (!image.load(input.string().c_str())) {
		wxLogWarning("Unable to load %ls, skipping", input.filename().wstring());
		return;
	}

	auto format = GuessFormat(input, image);
	if (!format.has_value()) {
		wxLogWarning("Unable to guess format for %ls, skipping",
			     input.filename().wstring());
		return;
	}

	wxLogMessage("Compressing %ls (format %s)", input.filename().wstring(),
		     FormatToString(format.value()));

	auto chain = BuildMipmapChain(image);

	nvtt::OutputOptions output_options;
	output_options.setFileName(output.string().c_str());

	nvtt::BatchList batch_list;
	for (int i = 0; i < chain.size(); ++i)
		batch_list.Append(&chain[i], 0, i, &output_options);

	nvtt::CompressionOptions compression_options;
	compression_options.setFormat(format.value());

	if (!ctx.outputHeader(chain.front(), chain.size(), compression_options, output_options)) {
		wxLogError("Error outputting header for input %ls, output %ls",
			   input.filename().wstring(), output.filename().wstring());
		return;
	}

	if (!ctx.compress(batch_list, compression_options)) {
		wxLogError("Error writing compressed data for input %ls, output %ls",
			   input.filename().wstring(), output.filename().wstring());
		return;
	}
}

bool ModExporter::OnInit()
{
	Frame *frame = new Frame("TM2020 Mod Exporter");
	frame->Show();

	return true;
}

wxIMPLEMENT_APP(ModExporter);

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

	wxTextAttr log_style{};
	log_style.SetFontFamily(wxFONTFAMILY_TELETYPE);
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
	output_panel->Enable();
	input_dir = event.GetPath().ToStdWstring();
}
void Frame::OnOuputChange(wxFileDirPickerEvent &event)
{
	export_button->Enable();
	output_dir = event.GetPath().ToStdWstring();
}
void Frame::OnExportPressed(wxCommandEvent &event)
{
	input_panel->Disable();
	output_panel->Disable();
	export_button->Disable();

	progress_bar->Enable();

	export_thread = new ExportThread(this, input_dir.value(), output_dir.value());
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

/* clang-format off */
wxBEGIN_EVENT_TABLE(Frame, wxFrame)
	EVT_DIRPICKER_CHANGED(ID_INPUT_PICKER, Frame::OnInputChange)
	EVT_DIRPICKER_CHANGED(ID_OUTPUT_PICKER, Frame::OnOuputChange)
	EVT_BUTTON(ID_EXPORT_BUTTON, Frame::OnExportPressed)
	
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_FINISHED, Frame::OnExportFinished)
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_PROGRESS, Frame::OnExportProgress)
	EVT_COMMAND(wxID_ANY, EVT_EXPORT_PROGRESS_RANGE, Frame::OnExportProgressRange)
wxEND_EVENT_TABLE();
/* clang-format on */

ExportThread::ExportThread(Frame *frame, std::filesystem::path input_dir,
			   std::filesystem::path output_dir)
	: wxThread(wxTHREAD_JOINABLE), frame{frame}, input_dir{input_dir}, output_dir{output_dir}
{
}

ExportThread::ExitCode ExportThread::Entry()
{
	ctx.enableCudaAcceleration(true);

	std::vector<std::array<std::filesystem::path, 2>> paths;
	for (auto const &entry : std::filesystem::directory_iterator(input_dir)) {
		if (!entry.is_regular_file())
			continue;

		auto input_file = entry.path();

		if (input_file.extension() != ".png" && input_file.extension() != ".PNG") {
			wxLogMessage("Skipping non-png %ls", input_file.filename().wstring());
			continue;
		}

		auto output_file = output_dir;
		output_file /= input_file.stem();
		output_file.replace_extension("dds");

		paths.push_back({input_file, output_file});
	}

	auto progress_range_event = new wxThreadEvent(EVT_EXPORT_PROGRESS_RANGE);
	progress_range_event->SetInt(paths.size());
	wxQueueEvent(frame, progress_range_event);

#pragma omp parallel for
	for (int i = 0; i < paths.size(); ++i) {
		CompressImage(ctx, paths[i][0], paths[i][1]);
		wxQueueEvent(frame, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}

	wxQueueEvent(frame, new wxThreadEvent(EVT_EXPORT_FINISHED));
	wxLogMessage("Export finished");

	return 0;
}

InputPanel::InputPanel(wxWindow *parent, wxWindowID id) : wxPanel(parent, id)
{
	auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Input");

	auto input_picker = new wxDirPickerCtrl(sizer->GetStaticBox(), ID_INPUT_PICKER);
	sizer->Add(input_picker, wxSizerFlags().Expand().Border());

	SetSizer(sizer);
}

OutputPanel::OutputPanel(wxWindow *parent, wxWindowID id) : wxPanel(parent, id)
{
	auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Output");
	SetSizer(sizer);

	output_picker = new wxDirPickerCtrl(sizer->GetStaticBox(), ID_OUTPUT_PICKER);

	sizer->Add(output_picker, wxSizerFlags().Expand().Border());
}