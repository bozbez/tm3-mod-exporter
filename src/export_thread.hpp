#pragma once

#include "common.hpp"

#include <nvtt/nvtt.h>
#include <wx/wx.h>

#include <filesystem>
#include <vector>

struct Paths {
	std::filesystem::path input;
	std::filesystem::path output;

	Paths(std::filesystem::path input, std::filesystem::path output)
		: input{input}, output{output}
	{
	}
};

struct BufferHandler : nvtt::OutputHandler {
	~BufferHandler() = default;

	void beginImage(int size, int width, int height, int depth, int face, int miplevel) {}
	void endImage() {}

	bool writeData(const void *data, int size)
	{
		auto data2 = reinterpret_cast<const uint8_t *>(data);
		buffer.insert(buffer.end(), data2, &data2[size]);

		return true;
	}

	std::vector<uint8_t> buffer;
};

class ExportThread : public wxThread {
public:
	ExportThread(wxEvtHandler *parent, std::filesystem::path input_dir,
		     std::filesystem::path output_dir, std::wstring name, FORMAT format,
		     long long max_res);

private:
	nvtt::Context ctx{};

	wxEvtHandler *parent;

	std::filesystem::path input_dir;
	std::filesystem::path output_dir;

	std::wstring name;
	FORMAT format;

	long long max_res;

	virtual ExitCode Entry();

	void ExportArchive(const std::vector<Paths> &paths);
	void ExportFolder(const std::vector<Paths> &paths);
};