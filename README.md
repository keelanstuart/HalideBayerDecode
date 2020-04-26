# ******************

# HalideBayerDecode
An experimental Bayer mosaic decoder (raw) written in C++ using Halide
Copyright © 2018-2020, Keelan Stuart

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

# ****************

First, to build this, you will need Halide present... you can obtain it from their github release page:
https://github.com/halide/Halide/releases

You'll also need libpng from https://libpng.sourceforge.io (and other mirrors)

You'll need to change this project's settings to indicate the direct you have placed Halide and libpng
(currently "d:\proj\halide" and "d:\proj\png", respectively, in "Settings" -> "VC++ Directories")

This makes use to Halide to convert a bayer mosaicked image to RGB
Algorithm loosely based on the paper by McGuire
(https://pdfs.semanticscholar.org/088a/2f47b7ab99c78d41623bdfaf4acdb02358fb.pdf)

I make some assumptions here...
a) the input is in RG/GB form (as opposed to GR/GB, GB/RG, or BG/GR)
b) the input image has all three channels, not a single channel

Halide makes some things drastically more complicated because it goes forward from
the input -- as opposed to working backwards from the output as you might in 3D
rendering, where the resultant pixel fragment is what causes the relevant source texels to be sampled
and processed in whatever way is desired.
