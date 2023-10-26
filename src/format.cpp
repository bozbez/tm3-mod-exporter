#include "format.hpp"

using namespace std::string_literals;

std::optional<Format> Format::GuessFormat(const std::filesystem::path &input,
					  const nvtt::Surface &image)
{
	auto stem = input.stem().wstring();

	if (stem.ends_with(L"_B")) {
		return Format(nvtt::Format_BC1, true);
	} else if (stem.ends_with(L"_R")) {
		return Format(nvtt::Format_BC5, true);
	} else if (stem.ends_with(L"_I")) {
		return Format(nvtt::Format_BC3, false);
	} else if (stem.ends_with(L"_N")) {
		return Format(nvtt::Format_BC5, true);
	} else if (stem.ends_with(L"_AO")) {
		return Format(nvtt::Format_BC1, true);
	} else if (stem.ends_with(L"_DirtMask")) {
		return Format(nvtt::Format_BC1, true);
	} else if (stem.ends_with(L"_D")) {
		if (image.alphaMode() == nvtt::AlphaMode_None)
			return Format(nvtt::Format_BC1, true);
		else
			return Format(nvtt::Format_BC3, true);
	} else if (stem.ends_with(L"_H")) {
		return Format(nvtt::Format_BC1, true);
	} else if (stem.ends_with(L"_M")) {
		return Format(nvtt::Format_BC3, true);
	} else if (stem.ends_with(L"_L")) {
		return Format(nvtt::Format_BC3, true);
	} else if (stem.ends_with(L"_CoatR")) {
		return Format(nvtt::Format_BC1, true);
	}

	return std::nullopt;
}

std::string Format::GetNvttFormatName() const
{
	switch (nvttFormat) {
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