#include "export_thread.hpp"
#include "format.hpp"
#include "nvtt/nvtt.h"
#include "wx/log.h"

#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/datstrm.h>

#include <set>

static std::vector<nvtt::Surface> BuildMipmapChain(nvtt::Surface image, bool premultiplyAlpha)
{
	std::vector<nvtt::Surface> chain;
	chain.push_back(image);

	while (chain.back().canMakeNextMipmap()) {
		chain.emplace_back(chain.back());

		auto &image = chain.back();

		image.toLinearFromSrgb();
		if(premultiplyAlpha) image.premultiplyAlpha();

		image.buildNextMipmap(nvtt::MipmapFilter_Kaiser);

		if(premultiplyAlpha) image.demultiplyAlpha();
		image.toSrgb();
	}

	return chain;
}

static bool CompressImage(nvtt::Context &ctx, const std::filesystem::path input,
			  const nvtt::OutputOptions &output_options, long long max_res,
			  nvtt::Quality quality, bool build_mipmaps)
{
	nvtt::Surface image;
	if (!image.load(input.string().c_str())) {
		wxLogWarning("Unable to load %ls, skipping", input.filename().wstring());
		return false;
	}

	auto formatOptional = Format::GuessFormat(input, image);
	if (!formatOptional.has_value()) {
		wxLogWarning("Unable to guess format for %ls, skipping",
			     input.filename().wstring());
		return false;
	}
	auto format = formatOptional.value();

	auto needs_resize = max_res > 0 && (image.width() > max_res || image.height() > max_res);
	wxLogMessage("+ %ls (%s, %s%s)", input.filename().wstring(),
		     format.GetNvttFormatName(), needs_resize ? "resizing" : "no resize",
		     format.ShouldPremultiplyAlpha() ? "" : ", no premultiplied alpha");

	if (needs_resize)
		image.resize(max_res, nvtt::RoundMode_None, nvtt::ResizeFilter_Kaiser);

	auto chain = build_mipmaps ? BuildMipmapChain(image, format.ShouldPremultiplyAlpha()) : std::vector<nvtt::Surface>{image};

	nvtt::BatchList batch_list;
	for (int i = 0; i < chain.size(); ++i)
		batch_list.Append(&chain[i], 0, i, &output_options);

	nvtt::CompressionOptions compression_options;
	compression_options.setQuality(quality);
	compression_options.setFormat(format.GetNvttFormat());

	if (!ctx.outputHeader(chain.front(), chain.size(), compression_options, output_options))
		return false;

	if (!ctx.compress(batch_list, compression_options))
		return false;

	return true;
}

ExportThread::ExportThread(wxEvtHandler *parent, std::filesystem::path input_dir,
			   std::filesystem::path output_dir, std::wstring name, FORMAT format,
			   long long max_res, nvtt::Quality quality, bool build_mipmaps)
	: wxThread(wxTHREAD_JOINABLE),
	  parent{parent},
	  input_dir{input_dir},
	  output_dir{output_dir},
	  name{name},
	  format{format},
	  max_res{max_res},
	  quality{quality},
	  build_mipmaps{build_mipmaps}
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

		if (!input_extensions.contains(input_file.extension().string()))
			continue;

		auto output_file = input_file.lexically_relative(input_dir);
		output_file.replace_extension("dds");

		for (auto &[_, output_file2] : paths) {
			if (output_file == output_file2)
				wxLogWarning("Duplicate input stem \"%s\", skipping",
					     input_file.stem().string());
			continue;
		}

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

		auto success = CompressImage(ctx, paths[i].input, output_options, max_res, quality,
					     build_mipmaps);
		if (!success) {
			wxLogError("Error compressing %ls -> %ls", paths[i].input.c_str(),
				   paths[i].output.c_str());
		}

		wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}

	std::filesystem::create_directories(output_dir);

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

		auto success = CompressImage(ctx, paths[i].input, output_options, max_res, quality,
					     build_mipmaps);
		if (!success) {
			wxLogError("Error compressing %ls -> %ls", paths[i].input.c_str(),
				   output_path.c_str());
		}

		wxQueueEvent(parent, new wxThreadEvent(EVT_EXPORT_PROGRESS));
	}
}