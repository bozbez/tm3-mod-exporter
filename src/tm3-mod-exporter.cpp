#include <iostream>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include <fmt/format.h>
#include <nvtt/nvtt.h>
#include <CLI/CLI.hpp>

// B        BC1
// R        BC5
// I        BC3
// N        BC5
// AO       BC1
// DirtMask BC4
// D        BC1 BC3

using namespace std::string_literals;

nvtt::Format guess_format(const std::filesystem::path &input, const nvtt::Surface &image)
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

	throw std::runtime_error(
		fmt::format("Unable to guess format for input {}", input.string()));
}

std::string format_to_string(nvtt::Format format)
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

std::vector<nvtt::Surface> make_mipmap_chain(nvtt::Surface image)
{
	std::vector<nvtt::Surface> chain;
	chain.push_back(image);

	while (chain.back().canMakeNextMipmap()) {
		chain.emplace_back(chain.back());
		chain.back().buildNextMipmap(nvtt::MipmapFilter_Kaiser);
	}

	return chain;
}

void compress_image(nvtt::Context &ctx, const std::filesystem::path input,
		    const std::filesystem::path output)
{
	nvtt::Surface image;
	if (!image.load(input.string().c_str())) {
		fmt::print("Unable to load input image {}, skipping", input.string());
		return;
	}

	nvtt::Format format;
	try {
		format = guess_format(input, image);
	} catch (std::runtime_error e) {
		fmt::print("Unable to guess format for {}, skipping\n", input.string());
		return;
	}

	fmt::print("Compressing {} -> {} (format {})\n", input.string(), output.string(),
		   format_to_string(format));

	auto chain = make_mipmap_chain(image);

	nvtt::OutputOptions output_options;
	output_options.setFileName(output.string().c_str());

	nvtt::BatchList batch_list;
	for (int i = 0; i < chain.size(); ++i)
		batch_list.Append(&chain[i], 0, i, &output_options);

	nvtt::CompressionOptions compression_options;
	compression_options.setFormat(format);

	if (!ctx.outputHeader(chain.front(), chain.size(), compression_options, output_options)) {
		fmt::print("Error outputting header for input {}, output {}", input.string(),
			   output.string());
		return;
	}

	if (!ctx.compress(batch_list, compression_options)) {
		fmt::print("Error writing compressed batch for input {}, output {}", input.string(),
			   output.string());
		return;
	}
}

int main(int argc, char **argv)
{
	CLI::App app{"A convenience utility for converting TM2020 texture mods to DDS files"};

	std::filesystem::path input;
	app.add_option("input,-i,--input", input, "Folder containing the input PNGs")
		->required()
		->check(CLI::ExistingDirectory);

	std::filesystem::path output = "";
	app.add_option("-o,--output", output, "Folder under which to place the output DDSs")
		->check(CLI::ExistingDirectory);

	CLI11_PARSE(app, argc, argv);

	input = input.lexically_normal();

	if (output == "")
		output = input;
	else
		output = output.lexically_normal();

	fmt::print("NVTT version: {}\n", nvtt::version());

	nvtt::Context ctx;
	ctx.enableCudaAcceleration(true);

	std::vector<std::array<std::filesystem::path, 2>> paths;
	for (auto const &entry : std::filesystem::directory_iterator(input)) {
		if (!entry.is_regular_file())
			continue;

		auto input_path = entry.path();

		if (input_path.extension() != ".png" && input_path.extension() != ".PNG") {
			fmt::print("Skipping non-png {}\n", input_path.string());
			continue;
		}

		auto output_path = output;
		output_path /= input_path.stem();
		output_path.replace_extension("dds");

		paths.push_back({input_path, output_path});
	}

#pragma omp parallel for
	for (int i = 0; i < paths.size(); ++i) {
		compress_image(ctx, paths[i][0], paths[i][1]);
	}
}