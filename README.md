# tm3-mod-exporter

A small C++ GUI tool to aid conversion of PNG skin and mod textures to archived DDS textures for Trackmania 2020. Internally NVIDIA's [NVTT-3](https://developer.nvidia.com/gpu-accelerated-texture-compression) is used for the format conversion, with mipmaps generated in high quality with a Kaiser-windowed Sinc downsampling filter.

As input it takes a folder (with subfolders for map mods) of PNGs, and can output either a folder or archive of DDS files (the latter for car skins). DDS block compression variant is detected automatically based on the input filename:

- *_B.png -> BC3
- *_R.png -> BC5
- *_I.png -> BC3
- *_N.png -> BC5
- *_A0.png -> BC1
- *_DirtMask.png -> BC4
- *_D.png -> BC3 if the PNG has an alpha channel, otherwise BC1

<p align="center">
  <img src="https://github.com/bozbez/tm3-mod-exporter/blob/main/media/screenshot.png" />
</p>