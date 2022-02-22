# tm3-mod-exporter

A small C++ GUI tool to aid conversion of PNG and JPEG skin and mod textures to archived DDS textures for Trackmania 2020. Internally NVIDIA's [NVTT-3](https://developer.nvidia.com/gpu-accelerated-texture-compression) is used for the format conversion, with optional high quality downsampling with a Kaiser-windowed Sinc filter.

As input it takes a folder (with subfolders for map mods) of PNGs and JPEGs, and can output either a folder or archive of DDS files (the latter for car skins). DDS block compression variant is detected automatically based on the input filename:

- *_B -> BC3
- *_R -> BC5
- *_I -> BC3
- *_N -> BC5
- *_A0 -> BC1
- *_DirtMask -> BC4
- *_D -> BC3 if the source image has an alpha channel, otherwise BC1

<br />
<p align="center">
  <img src="https://raw.githubusercontent.com/bozbez/tm3-mod-exporter/master/media/screenshot.png" />
</p>