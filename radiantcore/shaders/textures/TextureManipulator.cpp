#include "TextureManipulator.h"

#include "igl.h"
#include <stdlib.h>
#include "itextstream.h"
#include "registry/registry.h"
#include "math/Vector3.h"
#include "ipreferencesystem.h"
#include "../MaterialManager.h"
#include "RGBAImage.h"

namespace 
{
	static byte *row1 = NULL, *row2 = NULL;
	static std::size_t rowsize = 0;

	const std::size_t MAX_TEXTURE_QUALITY = 3;

	const std::string RKEY_TEXTURES_QUALITY = "user/ui/textures/quality";
	const std::string RKEY_TEXTURES_GAMMA = "user/ui/textures/gamma";
}

namespace shaders {

TextureManipulator::TextureManipulator() :
	_gamma(registry::getValue<float>(RKEY_TEXTURES_GAMMA)),
	_maxTextureSize(0),
	_textureQuality(registry::getValue<int>(RKEY_TEXTURES_QUALITY))
{
	GlobalRegistry().signalForKey(RKEY_TEXTURES_GAMMA).connect(
        sigc::mem_fun(this, &TextureManipulator::keyChanged)
    );
	GlobalRegistry().signalForKey(RKEY_TEXTURES_QUALITY).connect(
        sigc::mem_fun(this, &TextureManipulator::keyChanged)
    );

	calculateGammaTable();

	// greebo: Construct the preferences
	constructPreferences();
}

// the accessor function
TextureManipulator& TextureManipulator::instance() {
    static TextureManipulator _instance;
    return _instance;
}

void TextureManipulator::keyChanged() 
{
	_textureQuality = registry::getValue<int>(RKEY_TEXTURES_QUALITY);

	float newGamma = registry::getValue<float>(RKEY_TEXTURES_GAMMA);

	// Has the gamma actually changed?
	if (_gamma != newGamma) {
		_gamma = newGamma;
		calculateGammaTable();
	}
}

Vector3 TextureManipulator::getFlatshadeColour(const ImagePtr& input) {
	// Calculate the number of pixels in this image
	std::size_t numPixels = input->getWidth() * input->getHeight();

	// Calculate the pixel step value, ensuring it is greater than 0
	int incr = static_cast<int>(static_cast<float>(numPixels) / 20.0f);
	if (incr < 1)
		incr = 1;

	// Set the pixel pointer to the very first pixel
	byte* pixels = input->getPixels();

	Vector3 returnValue;
	int pixelCount = 0;

	// Go over all the pixels and change their value accordingly
	for (std::size_t i = 0; i < (numPixels*4); i += incr*4, pixelCount++)
	{
		// Sum up the RGBA values
		returnValue[0] += pixels[i];
		returnValue[1] += (pixels + 1)[i];
		returnValue[2] += (pixels + 2)[i];
	}

	// Take the average value of each RGB channel
	returnValue /= pixelCount;

	// Normalise the colour to 1.0
	returnValue /= 255;

	return returnValue;
}

ImagePtr TextureManipulator::getProcessedImage(const ImagePtr& input) {

	ImagePtr output;

	// Make the image dimensions match powers of two
	output = getResized(input);

	// Apply the gamma
	output = processGamma(output);

	return output;
}

ImagePtr TextureManipulator::getResized(const ImagePtr& input) {

	std::size_t width = input->getWidth();
	std::size_t height = input->getHeight();
	byte* sourcePixels = input->getPixels();

	ImagePtr output;

	// Determine the next larger power of two
	std::size_t gl_width = 1;
	while (gl_width < width)
		gl_width <<= 1;

	std::size_t gl_height = 1;
	while (gl_height < height)
		gl_height <<= 1;

	// Check, if the image dimensions are already powers of two, if not, rescale it
	if (!(gl_width == width && gl_height == height)) {

		// Create a new Image that hold the resampled texture
		output.reset(new image::RGBAImage(gl_width, gl_height));

		// Resample the texture into the allocated image
		resampleTexture(sourcePixels, width, height,
						output->getPixels(), gl_width, gl_height, 4);
	}
	else {
		// Nothing to do, return the source image
		output = input;
	}

	// Now retrieve the maximum texture size opengl can handle
	if (_maxTextureSize == 0)
	{
		int temp;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &temp);
		_maxTextureSize = temp;

		// If the value is still zero, fill it to some default value of 1024
		if (_maxTextureSize == 0) {
			_maxTextureSize = 1024;
		}
	}

	// Determine the target dimensions
	std::size_t qualityReduction = MAX_TEXTURE_QUALITY - _textureQuality;
	std::size_t targetWidth = std::min(gl_width >> qualityReduction, _maxTextureSize);
	std::size_t targetHeight = std::min(gl_height >> qualityReduction, _maxTextureSize);

	// Reduce the image to the next smaller power of two until it fits the openGL max texture size
	while (gl_width > targetWidth || gl_height > targetHeight)
	{
		mipReduce(output->getPixels(), output->getPixels(),
				  gl_width, gl_height, targetWidth, targetHeight);

		if (gl_width > targetWidth)
			gl_width >>= 1;
		if (gl_height > targetHeight)
			gl_height >>= 1;
	}

	return output;
}

// resample texture gamma according to user settings
ImagePtr TextureManipulator::processGamma(const ImagePtr& input) {

	// Don't do unnecessary work here...
	if (_gamma == 1.0f) {
		return input;
	}

	// Calculate the number of pixels in this image
	std::size_t numPixels = input->getWidth() * input->getHeight();

	// Set the pixel pointer to the very first pixel
	byte* pixels = input->getPixels();

	// Go over all the pixels and change their value accordingly
	for (std::size_t i = 0; i < (numPixels*4); i += 4)
	{
		// Change the current RGB pixel value to the one in the gamma table
		pixels[i] = _gammaTable[pixels[i]];
		(pixels + 1)[i] = _gammaTable[(pixels + 1)[i]];
		(pixels + 2)[i] = _gammaTable[(pixels + 2)[i]];
	}

	return input;
}

// Recalculates the gamma table according to the given gamma value
// This is called on first startup or if the user changes the value
void TextureManipulator::calculateGammaTable() {
	// Linear gamma, just fill the array linearly
	if (_gamma == 1.0) {
		for (int i = 0; i < 256; i++)
			_gammaTable[i] = static_cast<byte>(i);
	}
	else {
		// Calculate the gamma values
		for (int i = 0; i < 256; i++) {
			int inf = (int)(
				255 *
				pow(static_cast<double>((i + 0.5) / 255.5),
					static_cast<double>(_gamma)) + 0.5
			);

			// Constrain the values to (0..255)
			if (inf < 0) {
				inf = 0;
			}
			else if (inf > 255) {
				inf = 255;
			}

			_gammaTable[i] = static_cast<byte>(inf);
		}
	}
}

void TextureManipulator::resampleTextureLerpLine(const byte *in, byte *out,
							 std::size_t inwidth, std::size_t outwidth, int bytesperpixel)
{
	std::size_t j, xi, oldx = 0, f, lerp;
#define LERPBYTE(i) out[i] = (byte) ((((row2[i] - row1[i]) * lerp) >> 16) + row1[i])

	std::size_t fstep = static_cast<std::size_t>(inwidth * 65536.0f / outwidth);
	std::size_t endx = (inwidth - 1);
	if (bytesperpixel == 4) {
		for (j = 0,f = 0;j < outwidth;j++, f += fstep) {
			xi = f >> 16;
			if (xi != oldx) {
				in += (xi - oldx) * 4;
				oldx = xi;
			}

			if (xi < endx) {
				lerp = f & 0xFFFF;
				*out++ = (byte) ((((in[4] - in[0]) * lerp) >> 16) + in[0]);
				*out++ = (byte) ((((in[5] - in[1]) * lerp) >> 16) + in[1]);
				*out++ = (byte) ((((in[6] - in[2]) * lerp) >> 16) + in[2]);
				*out++ = (byte) ((((in[7] - in[3]) * lerp) >> 16) + in[3]);
			}
			else // last pixel of the line has no pixel to lerp to
			{
				*out++ = in[0];
				*out++ = in[1];
				*out++ = in[2];
				*out++ = in[3];
			}
		}
	}
	else if (bytesperpixel == 3) {
		for (j = 0, f = 0; j < outwidth; j++, f += fstep) {
			xi = f >> 16;
			if (xi != oldx) {
				in += (xi - oldx) * 3;
				oldx = xi;
			}

			if (xi < endx) {
				lerp = f & 0xFFFF;
				*out++ = (byte) ((((in[3] - in[0]) * lerp) >> 16) + in[0]);
				*out++ = (byte) ((((in[4] - in[1]) * lerp) >> 16) + in[1]);
				*out++ = (byte) ((((in[5] - in[2]) * lerp) >> 16) + in[2]);
			}
			else // last pixel of the line has no pixel to lerp to
			{
				*out++ = in[0];
				*out++ = in[1];
				*out++ = in[2];
			}
		}
	}
	else {
		rMessage() << "resampleTextureLerpLine: unsupported bytesperpixel " << bytesperpixel << "\n";
	}
}

/*
================
R_ResampleTexture
================
*/
void TextureManipulator::resampleTexture(const void *indata, std::size_t inwidth, std::size_t inheight,
										 void *outdata,  std::size_t outwidth, std::size_t outheight, int bytesperpixel)
{
	if (rowsize < outwidth * bytesperpixel) {
		if (row1)
			free(row1);
		if (row2)
			free(row2);

		rowsize = outwidth * bytesperpixel;
		row1 = (byte *)malloc(rowsize);
		row2 = (byte *)malloc(rowsize);
	}

	if (bytesperpixel == 4) {
		std::size_t i, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth4 = inwidth*4, outwidth4 = outwidth*4;
		long j;
		byte *inrow, *out;
		out = (byte *)outdata;
		fstep = (int) (inheight * 65536.0f / outheight);
#define LERPBYTE(i) out[i] = (byte) ((((row2[i] - row1[i]) * lerp) >> 16) + row1[i])

		inrow = (byte *)indata;
		oldy = 0;
		resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);
		resampleTextureLerpLine(inrow + inwidth4, row2, inwidth, outwidth, bytesperpixel);

		for (i = 0, f = 0;i < outheight;i++,f += fstep) {
			yi = f >> 16;
			if (yi < endy) {
				lerp = f & 0xFFFF;
				if (yi != oldy) {
					inrow = (byte *)indata + inwidth4 * yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth4);
					else
						resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);

					resampleTextureLerpLine(inrow + inwidth4, row2, inwidth, outwidth, bytesperpixel);
					oldy = yi;
				}
				j = static_cast<long>(outwidth - 4);
				while (j >= 0) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					LERPBYTE( 3);
					LERPBYTE( 4);
					LERPBYTE( 5);
					LERPBYTE( 6);
					LERPBYTE( 7);
					LERPBYTE( 8);
					LERPBYTE( 9);
					LERPBYTE(10);
					LERPBYTE(11);
					LERPBYTE(12);
					LERPBYTE(13);
					LERPBYTE(14);
					LERPBYTE(15);
					out += 16;
					row1 += 16;
					row2 += 16;
					j -= 4;
				}
				if (j & 2) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					LERPBYTE( 3);
					LERPBYTE( 4);
					LERPBYTE( 5);
					LERPBYTE( 6);
					LERPBYTE( 7);
					out += 8;
					row1 += 8;
					row2 += 8;
				}
				if (j & 1) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					LERPBYTE( 3);
					out += 4;
					row1 += 4;
					row2 += 4;
				}
				row1 -= outwidth4;
				row2 -= outwidth4;
			}
			else {
				if (yi != oldy) {
					inrow = (byte *)indata + inwidth4*yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth4);
					else
						resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);

					oldy = yi;
				}
				memcpy(out, row1, outwidth4);
			}
		}
	}
	else if (bytesperpixel == 3) {
		std::size_t i, yi, oldy, f, fstep, lerp, endy = (inheight-1), inwidth3 = inwidth * 3, outwidth3 = outwidth * 3;
		long j;
		byte *inrow, *out;
		out = (byte *)outdata;
		fstep = (int) (inheight*65536.0f/outheight);
#define LERPBYTE(i) out[i] = (byte) ((((row2[i] - row1[i]) * lerp) >> 16) + row1[i])

		inrow = (byte *)indata;
		oldy = 0;
		resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);
		resampleTextureLerpLine(inrow + inwidth3, row2, inwidth, outwidth, bytesperpixel);
		for (i = 0, f = 0;i < outheight;i++,f += fstep) {
			yi = f >> 16;
			if (yi < endy) {
				lerp = f & 0xFFFF;
				if (yi != oldy) {
					inrow = (byte *)indata + inwidth3*yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth3);
					else
						resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);

					resampleTextureLerpLine(inrow + inwidth3, row2, inwidth, outwidth, bytesperpixel);
					oldy = yi;
				}
				j = static_cast<long>(outwidth - 4);
				while (j >= 0) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					LERPBYTE( 3);
					LERPBYTE( 4);
					LERPBYTE( 5);
					LERPBYTE( 6);
					LERPBYTE( 7);
					LERPBYTE( 8);
					LERPBYTE( 9);
					LERPBYTE(10);
					LERPBYTE(11);
					out += 12;
					row1 += 12;
					row2 += 12;
					j -= 4;
				}
				if (j & 2) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					LERPBYTE( 3);
					LERPBYTE( 4);
					LERPBYTE( 5);
					out += 6;
					row1 += 6;
					row2 += 6;
				}
				if (j & 1) {
					LERPBYTE( 0);
					LERPBYTE( 1);
					LERPBYTE( 2);
					out += 3;
					row1 += 3;
					row2 += 3;
				}
				row1 -= outwidth3;
				row2 -= outwidth3;
			}
			else {
				if (yi != oldy) {
					inrow = (byte *)indata + inwidth3*yi;
					if (yi == oldy+1)
						memcpy(row1, row2, outwidth3);
					else
						resampleTextureLerpLine(inrow, row1, inwidth, outwidth, bytesperpixel);

					oldy = yi;
				}
				memcpy(out, row1, outwidth3);
			}
		}
	}
	else {
		rMessage() << "R_ResampleTexture: unsupported bytesperpixel " << bytesperpixel << "\n";
	}
}

// in can be the same as out
void TextureManipulator::mipReduce(byte *in, byte *out,
								   std::size_t width, std::size_t height,
								   std::size_t destwidth, std::size_t destheight)
{
	std::size_t x, y, width2, height2, nextrow;
	if (width > destwidth) {
		if (height > destheight) {
			// reduce both
			width2 = width >> 1;
			height2 = height >> 1;
			nextrow = width << 2;
			for (y = 0;y < height2;y++) {
				for (x = 0;x < width2;x++) {
					out[0] = (byte) ((in[0] + in[4] + in[nextrow  ] + in[nextrow+4]) >> 2);
					out[1] = (byte) ((in[1] + in[5] + in[nextrow+1] + in[nextrow+5]) >> 2);
					out[2] = (byte) ((in[2] + in[6] + in[nextrow+2] + in[nextrow+6]) >> 2);
					out[3] = (byte) ((in[3] + in[7] + in[nextrow+3] + in[nextrow+7]) >> 2);
					out += 4;
					in += 8;
				}
				in += nextrow; // skip a line
			}
		}
		else {
			// reduce width
			width2 = width >> 1;
			for (y = 0;y < height;y++) {
				for (x = 0;x < width2;x++) {
					out[0] = (byte) ((in[0] + in[4]) >> 1);
					out[1] = (byte) ((in[1] + in[5]) >> 1);
					out[2] = (byte) ((in[2] + in[6]) >> 1);
					out[3] = (byte) ((in[3] + in[7]) >> 1);
					out += 4;
					in += 8;
				}
			}
		}
	}
	else {
		if (height > destheight) {
			// reduce height
			height2 = height >> 1;
			nextrow = width << 2;
			for (y = 0;y < height2;y++) {
				for (x = 0;x < width;x++) {
					out[0] = (byte) ((in[0] + in[nextrow  ]) >> 1);
					out[1] = (byte) ((in[1] + in[nextrow+1]) >> 1);
					out[2] = (byte) ((in[2] + in[nextrow+2]) >> 1);
					out[3] = (byte) ((in[3] + in[nextrow+3]) >> 1);
					out += 4;
					in += 4;
				}
				in += nextrow; // skip a line
			}
		}
		else {
			rMessage() << "GL_MipReduce: desired size already achieved\n";
		}
	}
}

/* greebo: This gets called by the preference system and is responsible for adding the
 * according pages and elements to the preference dialog.*/
void TextureManipulator::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage("Settings/Textures");

	// Create the string list containing the quality captions
	std::list<std::string> percentages;

	percentages.push_back("12.5%");
	percentages.push_back("25%");
	percentages.push_back("50%");
	percentages.push_back("100%");

	page.appendCombo("Texture Quality", RKEY_TEXTURES_QUALITY, percentages);

	// Texture Gamma Settings
	page.appendSpinner("Texture Gamma", RKEY_TEXTURES_GAMMA, 0.0f, 1.0f, 10);
}

} // namespace shaders
