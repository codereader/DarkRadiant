#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

#include "iimage.h"
#include "ishaders.h"
typedef unsigned char byte;

namespace shaders {

class TextureManipulator
{
	// The gamma correction table
	byte _gammaTable[256];
	
	float _gamma;
	
public:
	TextureManipulator();
	
	void resampleTexture(const void *indata, int inwidth, int inheight, 
						 void *outdata, int outwidth, int outheight, int bytesperpixel);
	
	void mipReduce(byte *in, byte *out, 
				   int width, int height, 
				   int destwidth, int destheight);

	/* greebo: Returns the readily fabricated pixel data, that passed
	 * a bunch of stages (gamma calculation, mip reduction, stretching) 
	 */
	Image* getProcessedImage(Image* input);

	/* greebo: Performs a fast scan over the pixel data, taking every
	 * 20th pixel to determine the representative flat shade colour 
	 */
	Colour3 getFlatshadeColour(Image* input);

private:

	// Returns the gamma corrected image taken from <input>
	// (Does not allocate new memory)
	Image* processGamma(Image* input);
	
	// Recalculates the gamma table according to the given gamma value
	// This is called on first startup or if the user changes the value
	void calculateGammaTable();
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

} // namespace shaders

#endif /*TEXTUREMANIPULATOR_H_*/
