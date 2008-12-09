#include "MapExpression.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <iostream>

#include "os/path.h"
#include "string/string.h" // contains floatToStr() helper
#include "imagelib.h"
#include "math/FloatTools.h" // contains float_to_integer() helper
#include "math/Vector3.h"

#include "textures/DefaultConstructor.h"
#include "textures/FileLoader.h"
#include "textures/HeightmapCreator.h"
#include "textures/TextureManipulator.h"

/* CONSTANTS */
namespace {
	
	// Default image maps for optional material stages
	const std::string IMAGE_BLACK = "_black.bmp";
	const std::string IMAGE_CUBICLIGHT = "_cubiclight.bmp";
	const std::string IMAGE_CURRENTRENDER = "_currentrender.bmp";
	const std::string IMAGE_DEFAULT = "_default.bmp";
	const std::string IMAGE_FLAT = "_flat.bmp";
	const std::string IMAGE_FOG = "_fog.bmp";
	const std::string IMAGE_NOFALLOFF = "_nofalloff.bmp";
	const std::string IMAGE_POINTLIGHT1 = "_pointlight1.bmp";
	const std::string IMAGE_POINTLIGHT2 = "_pointlight2.bmp";
	const std::string IMAGE_POINTLIGHT3 = "_pointlight3.bmp";
	const std::string IMAGE_QUADRATIC = "_quadratic.bmp";
	const std::string IMAGE_SCRATCH = "_scratch.bmp";
	const std::string IMAGE_SPOTLIGHT = "_spotlight.bmp";
	const std::string IMAGE_WHITE = "_white.bmp";
}

namespace shaders {

MapExpressionPtr IMapExpression::createForToken(DefTokeniser& token) {
	// Switch on the first keyword, to determine what kind of expression this
	// is.
	std::string type = boost::algorithm::to_lower_copy(token.nextToken());

	if (type == "heightmap") {
		return MapExpressionPtr(new HeightMapExpression (token));
	}
	else if (type == "addnormals") {
		return MapExpressionPtr(new AddNormalsExpression (token));
	}
	else if (type == "smoothnormals") {
		return MapExpressionPtr(new SmoothNormalsExpression (token));
	}
	else if (type == "add") {
		return MapExpressionPtr(new AddExpression (token));
	}
	else if (type == "scale") {
		return MapExpressionPtr(new ScaleExpression (token));
	}
	else if (type == "invertalpha") {
		return MapExpressionPtr(new InvertAlphaExpression (token));
	}
	else if (type == "invertcolor") {
		return MapExpressionPtr(new InvertColorExpression (token));
	}
	else if (type == "makeintensity") {
		return MapExpressionPtr(new MakeIntensityExpression (token));
	}
	else if (type == "makealpha") {
		return MapExpressionPtr(new MakeAlphaExpression (token));
	}
	else {
		// since we already took away the expression into the variable type, we need to pass type instead of token
		return MapExpressionPtr(new ImageExpression (type));
	}
}

MapExpressionPtr IMapExpression::createForString(std::string str) {
	parser::BasicDefTokeniser<std::string> token(str);
	return createForToken(token);
}

ImagePtr IMapExpression::getResampled(ImagePtr input, unsigned int width, unsigned int height) {
	// Check if the dimensions differ from the desired ones
	if (width != input->getWidth(0) || height != input->getHeight(0)) {
		// Allocate a new image buffer
		ImagePtr resampled (new RGBAImage(width, height));
	
		// Resample the texture to match the dimensions of the first image
		TextureManipulator::instance().resampleTexture(
			input->getMipMapPixels(0), 
			input->getWidth(0), input->getHeight(0), 
			resampled->getMipMapPixels(0), 
			width, height, 4
		);
		return resampled;
	}
	else {
		// Nothing to do here, dimensions match
		return input;
	}
}

HeightMapExpression::HeightMapExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	heightMapExp = createForToken(token);
	token.assertNextToken(",");
	scale = strToFloat(token.nextToken());
	token.assertNextToken(")");
}

ImagePtr HeightMapExpression::getImage() {
	// Get the heightmap from the contained expression
	ImagePtr heightMap = heightMapExp->getImage();
	
	if (heightMap == NULL) return ImagePtr();
	
	// Convert the heightmap into a normalmap
	ImagePtr normalMap = createNormalmapFromHeightmap(heightMap, scale);
	return normalMap;
}

std::string HeightMapExpression::getIdentifier() {
	std::string identifier = "_heightmap_";
	identifier.append(heightMapExp->getIdentifier() + floatToStr(scale));
	return identifier;
}

AddNormalsExpression::AddNormalsExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExpOne = createForToken(token);
	token.assertNextToken(",");
	mapExpTwo = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr AddNormalsExpression::getImage() {
    ImagePtr imgOne = mapExpOne->getImage();
    
    if (imgOne == NULL) return ImagePtr();

    unsigned int width = imgOne->getWidth(0);
    unsigned int height = imgOne->getHeight(0);

    ImagePtr imgTwo = mapExpTwo->getImage();

    if (imgTwo == NULL) return ImagePtr();
    
	// The image must match the dimensions of the first 
	imgTwo = getResampled(imgTwo, width, height);

    ImagePtr result (new RGBAImage(width, height));

    byte* pixOne = imgOne->getMipMapPixels(0);
    byte* pixTwo = imgTwo->getMipMapPixels(0);
    byte* pixOut = result->getMipMapPixels(0);

    // iterate through the pixels
    for( int y = 0; y < static_cast<int>(height); y++ ) {
	for( int x = 0; x < static_cast<int>(width); x++ ) {
	    // create the two vectors
	    Vector3 vectorOne(
	    	static_cast<double>(pixOne[0]), 
	    	static_cast<double>(pixOne[1]), 
	    	static_cast<double>(pixOne[2])
	    );
	    Vector3 vectorTwo(
	    	static_cast<double>(pixTwo[0]), 
	    	static_cast<double>(pixTwo[1]), 
	    	static_cast<double>(pixTwo[2])
	    );
	    // Take the mean value of the two vectors
	    Vector3 vectorOut = (vectorOne + vectorTwo) * 0.5;

	    pixOut[0] = float_to_integer(vectorOut.x());
	    pixOut[1] = float_to_integer(vectorOut.y());
	    pixOut[2] = float_to_integer(vectorOut.z());
	    pixOut[3] = 255;

	    // advance the pixel pointer
	    pixOne += 4;
	    pixTwo += 4;
	    pixOut += 4;
	}
    }
    return result;
}

std::string AddNormalsExpression::getIdentifier() {
	std::string identifier = "_addnormals_";
	identifier.append(mapExpOne->getIdentifier() + mapExpTwo->getIdentifier());
	return identifier;
}

SmoothNormalsExpression::SmoothNormalsExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr SmoothNormalsExpression::getImage() {

	ImagePtr normalMap = mapExp->getImage();
	
	if (normalMap == NULL) return ImagePtr();
	 
	unsigned int width = normalMap->getWidth(0);
	unsigned int height = normalMap->getHeight(0);
	 
	ImagePtr result (new RGBAImage(width, height));
 
	byte* in = normalMap->getMipMapPixels(0);
	byte* out = result->getMipMapPixels(0);

	struct KernelElement {
		// offset to the current pixel
		int dx, dy;
	};

	// a 3x3 kernel with the surrounding pixels including the pixel itself
	const int kernelSize = 9;
	KernelElement kernel[kernelSize] = {
		{-1, -1 },
		{ 0, -1 },
		{ 1, -1 },
		{ 1,  0 },
		{ 1,  1 },
		{ 0,  1 },
		{-1,  1 },
		{-1,  0 },
		{ 0,  0 }
	};
	const float perKernelSize = 1.0f/kernelSize;

	// iterate through the pixels
	for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			//the new normal vector for this pixel
			Vector3 smoothVector(0,0,0);

			// calculate the average direction of the surrounding vectors
			for (KernelElement* i = kernel; i != kernel + kernelSize; ++i) {
				// temporary vector to represent one of the surrounding pixels
				byte* pixel = getPixel(in, width, height, x + i->dx, y + i->dy);
				Vector3 temp(pixel[0], pixel[1], pixel[2]);

				smoothVector += temp;
			}

			// Take the average normal vector as result 
			smoothVector *= perKernelSize;
			
			out[0] = float_to_integer(smoothVector.x());
			out[1] = float_to_integer(smoothVector.y());
			out[2] = float_to_integer(smoothVector.z());
			out[3] = 255;
			
			// advance the pixel pointer
			out += 4;
	    }
	}
    return result;
}

std::string SmoothNormalsExpression::getIdentifier() {
	std::string identifier = "_smoothnormals_";
	identifier.append(mapExp->getIdentifier());
	return identifier;
}

AddExpression::AddExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExpOne = createForToken(token);
	token.assertNextToken(",");
	mapExpTwo = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr AddExpression::getImage() {
    ImagePtr imgOne = mapExpOne->getImage();
    
    if (imgOne == NULL) return ImagePtr();

    unsigned int width = imgOne->getWidth(0);
    unsigned int height = imgOne->getHeight(0);

	ImagePtr imgTwo = mapExpTwo->getImage();
	
	if (imgTwo == NULL) return ImagePtr();
	
	// Resize the image to match the dimensions of the first
    imgTwo = getResampled(imgTwo, width, height);

    ImagePtr result (new RGBAImage(width, height));

    byte* pixOne = imgOne->getMipMapPixels(0);
    byte* pixTwo = imgTwo->getMipMapPixels(0);
    byte* pixOut = result->getMipMapPixels(0);

    // iterate through the pixels
    for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			// add the colors
			pixOut[0] = float_to_integer((static_cast<float>(pixOne[0]) + pixTwo[0]) * 0.5f);
			pixOut[1] = float_to_integer((static_cast<float>(pixOne[1]) + pixTwo[1]) * 0.5f);
			pixOut[2] = float_to_integer((static_cast<float>(pixOne[2]) + pixTwo[2]) * 0.5f);
			pixOut[3] = float_to_integer((static_cast<float>(pixOne[3]) + pixTwo[3]) * 0.5f);

			//advance the pixel pointer
			pixOne += 4;
			pixTwo += 4;
			pixOut += 4;
		}
    }
	return result;
}

std::string AddExpression::getIdentifier() {
	std::string identifier = "_add_";
	identifier.append(mapExpOne->getIdentifier() + mapExpTwo->getIdentifier());
	return identifier;
}

ScaleExpression::ScaleExpression (DefTokeniser& token) : scaleGreen(0),scaleBlue(0),scaleAlpha(0) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(",");
	scaleRed = strToFloat(token.nextToken());
	if (token.nextToken() == ")") {
		return;
	}
	scaleGreen = strToFloat(token.nextToken());
	if (token.nextToken() == ")") {
		return;
	}
	scaleBlue = strToFloat(token.nextToken());
	if (token.nextToken() == ")") {
		return;
	}
	scaleAlpha = strToFloat(token.nextToken());
	token.assertNextToken(")");
}

ImagePtr ScaleExpression::getImage() {
    ImagePtr img = mapExp->getImage();
    
    if (img == NULL) return ImagePtr();

    unsigned int width = img->getWidth(0);
    unsigned int height = img->getHeight(0);
    
    if (scaleRed < 0 || scaleGreen < 0 || scaleBlue < 0 || scaleAlpha < 0) {
		std::cout << "[shaders] ScaleExpression: Invalid scale values found.\n";
		return img; 
	}
	 
    ImagePtr result (new RGBAImage(width, height));
 
    byte* in = img->getMipMapPixels(0);
    byte* out = result->getMipMapPixels(0);

    // iterate through the pixels
    for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			// prevent negative values and check for values >255
			int red = float_to_integer(static_cast<float>(in[0]) * scaleRed);
			out[0] = (red>255) ? 255 : red;

			int green = float_to_integer(static_cast<float>(in[1]) * scaleGreen);
			out[1] = (green>255) ? 255 : green;

			int blue = float_to_integer(static_cast<float>(in[2]) * scaleBlue);
			out[2] = (blue>255) ? 255 : blue;

			int alpha = float_to_integer(static_cast<float>(in[3]) * scaleAlpha);
			out[3] = (alpha>255) ? 255 : alpha;

			// advance the pixel pointer
			in += 4;
			out += 4;
		}
    }
	return result;
}

std::string ScaleExpression::getIdentifier() {
	std::string identifier = "_scale_";
	identifier.append(mapExp->getIdentifier() + floatToStr(scaleRed) + floatToStr(scaleGreen) + floatToStr(scaleBlue) + floatToStr(scaleAlpha));
	return identifier;
}

InvertAlphaExpression::InvertAlphaExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr InvertAlphaExpression::getImage() {
	ImagePtr img = mapExp->getImage();
	
	if (img == NULL) return ImagePtr();

	unsigned int width = img->getWidth(0);
	unsigned int height = img->getHeight(0);

	ImagePtr result (new RGBAImage(width, height));

	byte* in = img->getMipMapPixels(0);
	byte* out = result->getMipMapPixels(0);

	// iterate through the pixels
	for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			out[0] = in[0];
			out[1] = in[1];
			out[2] = in[2];
			out[3] = 255 - in[3];

			// advance the pixel pointer
			in += 4;
			out += 4;
		}
	}
	return result;
}

std::string InvertAlphaExpression::getIdentifier() {
	std::string identifier = "_invertalpha_";
	identifier.append(mapExp->getIdentifier());
	return identifier;
}

InvertColorExpression::InvertColorExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr InvertColorExpression::getImage() {
	ImagePtr img = mapExp->getImage();
	
	if (img == NULL) return ImagePtr();

	unsigned int width = img->getWidth(0);
	unsigned int height = img->getHeight(0);

	ImagePtr result (new RGBAImage(width, height));
 
	byte* in = img->getMipMapPixels(0);
	byte* out = result->getMipMapPixels(0);

	// iterate through the pixels
	for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			out[0] = 255 - in[0];
			out[1] = 255 - in[1];
			out[2] = 255 - in[2];
			out[3] = in[3];

			// advance the pixel pointer
			in += 4;
			out += 4;
		}
	}
	return result;
}

std::string InvertColorExpression::getIdentifier() {
	std::string identifier = "_invertcolor_";
	identifier.append(mapExp->getIdentifier());
	return identifier;
}

MakeIntensityExpression::MakeIntensityExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr MakeIntensityExpression::getImage() {
	ImagePtr img = mapExp->getImage();
	
	if (img == NULL) return ImagePtr();

	unsigned int width = img->getWidth(0);
	unsigned int height = img->getHeight(0);

	ImagePtr result (new RGBAImage(width, height));
 
	byte* in = img->getMipMapPixels(0);
	byte* out = result->getMipMapPixels(0);
	
	// iterate through the pixels
	for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			out[0] = in[0];
			out[1] = in[0];
			out[2] = in[0];
			out[3] = in[0];

			// advance the pixel pointer
			in += 4;
			out += 4;
		}
	}
	return result;
}

std::string MakeIntensityExpression::getIdentifier() {
	std::string identifier = "_makeintensity_";
	identifier.append(mapExp->getIdentifier());
	return identifier;
}

MakeAlphaExpression::MakeAlphaExpression (DefTokeniser& token) {
	token.assertNextToken("(");
	mapExp = createForToken(token);
	token.assertNextToken(")");
}

ImagePtr MakeAlphaExpression::getImage() {
	ImagePtr img = mapExp->getImage();
	
	if (img == NULL) return ImagePtr();

	unsigned int width = img->getWidth(0);
	unsigned int height = img->getHeight(0);

	ImagePtr result (new RGBAImage(width, height));

	byte* in = img->getMipMapPixels(0);
	byte* out = result->getMipMapPixels(0);

	// iterate through the pixels
	for( int y = 0; y < static_cast<int>(height); y++) {
		for( int x = 0; x < static_cast<int>(width); x++) {
			out[0] = 255;
			out[1] = 255;
			out[2] = 255;
			out[3] = (in[0] + in[1] + in[2])/3;

			// advance the pixel pointer
			in += 4;
			out += 4;
		}
	}
	return result;
}

std::string MakeAlphaExpression::getIdentifier() {
	std::string identifier = "_makealpha_";
	identifier.append(mapExp->getIdentifier());
	return identifier;
}

ImageExpression::ImageExpression (std::string imgName) {
	// Replace backslashes with forward slashes and strip of 
	// the file extension of the provided token, and store 
	// the result in the provided string.
	_imgName = os::standardPath(imgName).substr(0, imgName.rfind("."));
}

ImagePtr ImageExpression::getImage() {
	// Check for some image keywords and load the correct file
	if (_imgName == "_black") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_BLACK);
		return d.construct();
	}
	else if (_imgName == "_cubicLight") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_CUBICLIGHT);
		return d.construct();
	}
	else if (_imgName == "_currentRender") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_CURRENTRENDER);
		return d.construct();
	}
	else if (_imgName == "_default") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_DEFAULT);
		return d.construct();
	}
	else if (_imgName == "_flat") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_FLAT);
		return d.construct();
	}
	else if (_imgName == "_fog") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_FOG);
		return d.construct();
	}
	else if (_imgName == "_noFalloff") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_NOFALLOFF);
		return d.construct();
	}
	else if (_imgName == "_pointLight1") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_POINTLIGHT1);
		return d.construct();
	}
	else if (_imgName == "_pointLight2") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_POINTLIGHT2);
		return d.construct();
	}
	else if (_imgName == "_pointLight3") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_POINTLIGHT3);
		return d.construct();
	}
	else if (_imgName == "_quadratic") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_QUADRATIC);
		return d.construct();
	}
	else if (_imgName == "_scratch") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_SCRATCH);
		return d.construct();
	}
	else if (_imgName == "_spotlight") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_SPOTLIGHT);
		return d.construct();
	}
	else if (_imgName == "_white") {
		FileLoader d(GlobalRegistry().get("user/paths/bitmapsPath") + IMAGE_WHITE);
		return d.construct();
	}
	// this is a normal material image, so we use the DefaultConstructor
	else {
		DefaultConstructor d(_imgName);
		return d.construct();
	}
}

std::string ImageExpression::getIdentifier() {
	return _imgName;
}

} // namespace shaders
