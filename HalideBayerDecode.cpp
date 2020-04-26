/*

	HalideBayerDecode - a learning experiment to convert .raw / bayer mosaic images into RGB images
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

*/


/*

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

*/


#include "stdafx.h"
#include <string>

#define HALIDE_NO_JPEG

#include "halide_image_io.h"

using namespace Halide::Tools;


int main(int argc, char **argv)
{
    std::string in_filename = "input/";
    in_filename += (argc > 1) ? argv[1] : "bayer_rggb.png";
    std::string out_filename = "output/";
    out_filename += (argc > 1) ? argv[1] : "bayer_rggb.png";

    Halide::Var x("x"), y("y"), c("c");

    Halide::Buffer<uint8_t> input = load_image(in_filename.c_str());

    // the bounds were difficult to figure out and get working!
    // Halide (seemingly) provides several ways to do this in their API, but none worked
    // but this one
    Halide::Func clamped_input("clamped_input");
    clamped_input(x, y, c) = Halide::BoundaryConditions::constant_exterior(input, 0)(x, y, c);

    Halide::Func debayer("debayer");

    Halide::Expr v_identity = clamped_input(x, y, 0);

    // get the pixels surrounding the identity pixel to interpolate
    Halide::Expr v_n  = clamped_input(x    , y - 1, 0); v_n = Halide::min(v_n, 255.0f);
    Halide::Expr v_s  = clamped_input(x    , y + 1, 0); v_s = Halide::min(v_s, 255.0f);
    Halide::Expr v_e  = clamped_input(x + 1, y    , 0); v_e = Halide::min(v_e, 255.0f);
    Halide::Expr v_w  = clamped_input(x - 1, y    , 0); v_w = Halide::min(v_w, 255.0f);
    Halide::Expr v_ne = clamped_input(x + 1, y - 1, 0); v_ne = Halide::min(v_ne, 255.0f);
    Halide::Expr v_nw = clamped_input(x - 1, y - 1, 0); v_nw = Halide::min(v_nw, 255.0f);
    Halide::Expr v_se = clamped_input(x + 1, y + 1, 0); v_se = Halide::min(v_se, 255.0f);
    Halide::Expr v_sw = clamped_input(x - 1, y + 1, 0); v_sw = Halide::min(v_sw, 255.0f);

    // average the multi-sample values

    Halide::Expr v_checker;
    v_checker = (v_ne + v_nw + v_se + v_sw) / 4.0f;
    v_checker = Halide::cast<uint8_t>(v_checker);

    Halide::Expr v_cross;
    v_cross = (v_n + v_s + v_e + v_w) / 4.0f;
    v_cross = Halide::cast<uint8_t>(v_cross);

    Halide::Expr v_horizontal;
    v_horizontal = (v_e + v_w) / 2.0f;
    v_horizontal = Halide::cast<uint8_t>(v_horizontal);

    Halide::Expr v_vertical;
    v_vertical = (v_n + v_s) / 2.0f;
    v_vertical = Halide::cast<uint8_t>(v_vertical);

    // all color components must be derived from the current x/y pixel, regardless of what that pixel's identity is
    // thus, e.g., red comes from red identity pixels in a red/green line, green pixels in a red/green line,
    // green pixels in a green/blue line, and blue pixels in a green/blue line. Selects are used to recursively
    // determine how the data should be interpreted and interpolated.

    Halide::Func red_from_red_rgline;       red_from_red_rgline(x, y, c) = v_identity;
    Halide::Func red_from_green_rgline;     red_from_green_rgline(x, y, c) = v_horizontal;
    Halide::Func red_from_green_gbline;     red_from_green_gbline(x, y, c) = v_vertical;
    Halide::Func red_from_blue_gbline;      red_from_blue_gbline(x, y, c) = v_checker;

    Halide::Func green_from_red_rgline;     green_from_red_rgline(x, y, c) = v_cross;
    Halide::Func green_from_green_rgline;   green_from_green_rgline(x, y, c) = v_identity;
    Halide::Func green_from_green_gbline;   green_from_green_gbline(x, y, c) = v_identity;
    Halide::Func green_from_blue_gbline;    green_from_blue_gbline(x, y, c) = v_cross;

    Halide::Func blue_from_red_rgline;      blue_from_red_rgline(x, y, c) = v_checker;
    Halide::Func blue_from_green_rgline;    blue_from_green_rgline(x, y, c) = v_vertical;
    Halide::Func blue_from_green_gbline;    blue_from_green_gbline(x, y, c) = v_horizontal;
    Halide::Func blue_from_blue_gbline;     blue_from_blue_gbline(x, y, c) = v_identity;

    // *** assume RGGB mosaic

    // RG lines are even
    Halide::Expr is_rgline = (y % 2) == 0;

    // Green pixels are odd on even lines, even on odd lines (by column)
    Halide::Expr is_green = (((x % 2) + (y % 2)) == 1);

    // get components based on whether the identity is green
    Halide::Func red_from_rgline, red_from_gbline;
    red_from_rgline(x, y, c) = Halide::select(is_green, red_from_green_rgline(x, y, c), red_from_red_rgline(x, y, c));
    red_from_gbline(x, y, c) = Halide::select(is_green, red_from_green_gbline(x, y, c), red_from_blue_gbline(x, y, c));

    Halide::Func green_from_rgline, green_from_gbline;
    green_from_rgline(x, y, c) = Halide::select(is_green, green_from_green_rgline(x, y, c), green_from_red_rgline(x, y, c));
    green_from_gbline(x, y, c) = Halide::select(is_green, green_from_green_gbline(x, y, c), green_from_blue_gbline(x, y, c));

    Halide::Func blue_from_rgline, blue_from_gbline;
    blue_from_rgline(x, y, c) = Halide::select(is_green, blue_from_green_rgline(x, y, c), blue_from_red_rgline(x, y, c));
    blue_from_gbline(x, y, c) = Halide::select(is_green, blue_from_green_gbline(x, y, c), blue_from_blue_gbline(x, y, c));

    // drill down for each component, based on the line we're on
    Halide::Func red;
    red(x, y, c) = Halide::select(is_rgline, red_from_rgline(x, y, c), red_from_gbline(x, y, c));

    Halide::Func green;
    green(x, y, c) = Halide::select(is_rgline, green_from_rgline(x, y, c), green_from_gbline(x, y, c));

    Halide::Func blue;
    blue(x, y, c) = Halide::select(is_rgline, blue_from_rgline(x, y, c), blue_from_gbline(x, y, c));

    // finally, depending on the output pixel channel, we choose which value
    debayer(x, y, c) = Halide::select(c == 0, red(x, y, c), Halide::select(c == 1, green(x, y, c), blue(x, y, c)));

    // trying to schedule based on scanline... just playing here
    debayer.parallel(y);

    Halide::Buffer<uint8_t> output;
    output = debayer.realize(input.width(), input.height(), 3);

    save_image(output, out_filename.c_str());

    return 0;
}

