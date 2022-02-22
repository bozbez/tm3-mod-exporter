#include "export_thread.hpp"

#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/datstrm.h>

#include <set>

using namespace std::string_literals;

static std::optional<nvtt::Format> GuessFormat(const std::filesystem::path &input,
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

static std::string FormatToString(nvtt::Format format)
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

static bool CompressImage(nvtt::Context &ctx, const std::filesystem::path input,
			  const nvtt::OutputOptions &output_options, long long max_res)
{
	nvtt::Surface image;
	if (!image.load(input.string().c_str())) {
		wxLogWarning("Unable to load %ls, skipping", input.filename().wstring());
		return false;
	}

	auto format = GuessFormat(input, image);
	if (!format.has_value()) {
		wxLogWarning("Unable to guess format for %ls, skipping",
			     input.filename().wstring());
		return false;
	}

	auto needs_resize = max_res > 0 && (image.width() > max_res || image.height() > max_res);
	wxLogMessage("+ %ls (%s, %s)", input.filename().wstring(), FormatToString(format.value()),
		     needs_resize ? "resizing" : "no resize");

	if (needs_resize)
		image.resize(max_res, nvtt::RoundMode_None, nvtt::ResizeFilter_Kaiser);

	nvtt::CompressionOptions compression_options;
	compression_options.setFormat(format.value());

	if (!ctx.outputHeader(image, 1, compression_options, output_options))
		return false;

	if (!ctx.compress(image, 0, 0, compression_options, output_options))
		return false;

	return true;
}

ExportThread::ExportThread(wxEvtHandler *parent, std::filesystem::path input_dir,
			   std::filesystem::path output_dir, std::wstring name, FORMAT format,
			   long long max_res)
	: wxThread(wxTHREAD_JOINABLE),
	  parent{parent},
	  input_dir{input_dir},
	  output_dir{output_dir},
	  name{name},
	  format{format},
	  max_res{max_res}
{
}

ExportThread::ExitCode ExportThread::Entry()
{
	ctx.enableCudaAcceleration(true);

	std::vector<Paths> paths;
	std::set<std::filesystem::path> output_dirs;

	for (auto entry : std::filesystem::recursive_directory_iterator(input_dir)) {
		if (!entry.is_regular_file())
			continue;

		auto input_file = entry.path();

		if (input_file.extension() != ".png" && input_file.extension() != ".PNG")
			continue;

		auto output_file = input_file.lexically_relative(input_dir);
		output_file.replace_extension("dds");

		output_dirs.insert(output_file.parent_path());
		paths.push_back({input_file, output_file});
	}

	wxLogMessage("Starting export");
	if (format == FORMAT_ARCHIVE)
		ExportArchive(paths);
	else if (format == FORMAT_FOLDER)
		ExportFolder(paths);

	wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_FINISHED));
	wxLogMessage("Export finished");

	return 0;
}

void ExportThread::ExportArchive(const std::vector<Paths> &paths)
{
	auto progress_range_event = new wxThreadEvent(EVT_EXPORT_PROGRESS_RANGE);
	progress_range_event->SetInt(paths.size());
	wxQueueEvent(parent, progress_range_event);

	std::vector<BufferHandler> buffers{paths.size()};

#pragma omp parallel for
	for (int i = 0; i < paths.size(); ++i) {
		nvtt::OutputOptions output_options;
		output_options.setOutputHandler(&buffers[i]);

		auto success = CompressImage(ctx, paths[i].input, output_options, max_res);
		if (!success) {
			wxLogError("Error compressing %ls -> %ls", paths[i].input.c_str(),
				   paths[i].output.c_str());
		}

		wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}

	auto output_zip = output_dir;
	output_zip /= name;
	output_zip.replace_extension(".zip");

	wxLogMessage("Archiving...");

	wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS_RESET));

	wxFFileOutputStream output_stream(output_zip.wstring());
	wxZipOutputStream zip_stream(output_stream);
	wxDataOutputStream data_stream(zip_stream);

	for (int i = 0; i < paths.size(); ++i) {
		auto &buffer = buffers[i].buffer;

		zip_stream.PutNextEntry(paths[i].output.wstring());
		data_stream.Write8(buffer.data(), buffer.size());

		wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}

	zip_stream.Close();
}

void ExportThread::ExportFolder(const std::vector<Paths> &paths)
{
	for (auto &[_, output_file] : paths) {
		auto output_path = output_dir;
		output_path /= name;
		output_path /= output_file;

		std::filesystem::create_directories(output_path.parent_path());
	}

	auto progress_range_event = new wxThreadEvent(EVT_EXPORT_PROGRESS_RANGE);
	progress_range_event->SetInt(paths.size());
	wxQueueEvent(parent, progress_range_event);

#pragma omp parallel for
	for (int i = 0; i < paths.size(); ++i) {
		auto output_path = output_dir;
		output_path /= name;
		output_path /= paths[i].output;

		nvtt::OutputOptions output_options;
		output_options.setFileName(output_path.string().c_str());

		auto success = CompressImage(ctx, paths[i].input, output_options, max_res);
		if (!success) {
			wxLogError("Error compressing %ls -> %ls", paths[i].input.c_str(),
				   output_path.c_str());
		}

		wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}
}