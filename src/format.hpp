#pragma once

#include <nvtt/nvtt_lowlevel.h>
#include <optional>
#include <filesystem>
#include <nvtt/nvtt.h>

class Format {

public:
	// Attempt to determine
	static std::optional<Format> GuessFormat(const std::filesystem::path &input,
						 const nvtt::Surface &image);

private:
	nvtt::Format nvttFormat; // NVTT's internal compression format

	// Should premultiplied alpha be used when building mipmaps? The answer is NO if the alpha
	// channel of represents something other than opacity data (e.g. _I textures use alpha to
	// represent the type of event triggers illumination).
	bool premultiplyAlpha;

public:
	Format(nvtt::Format nvttFormat, bool premultiplyAlpha) : nvttFormat(nvttFormat),
		  premultiplyAlpha(premultiplyAlpha) {}

	nvtt::Format GetNvttFormat() const { return nvttFormat; }
	std::string GetNvttFormatName() const;
	bool ShouldPremultiplyAlpha() const { return premultiplyAlpha; }

};