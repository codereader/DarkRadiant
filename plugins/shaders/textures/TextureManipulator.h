#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

#include "iimage.h"
#include "ishaders.h"
#include "iregistry.h"
typedef unsigned char byte;

namespace shaders {

class TextureManipulator :
 	public RegistryKeyObserver
{
	// The gamma correction table
	byte _gammaTable[256];
	
	// The currently active gamma value (0.0...1.0)
	float _gamma;
	
	// Gets filled in by an OpenGL query
	int _maxTextureSize;
	
	// The image reduction indicator (3 = no reduction, 0 = 12.5%)
	int _textureQuality;

protected:
	// this is a singleton
	TextureManipulator();

	// prevent copy-construction
	TextureManipulator(const TextureManipulator&);

	// prevent construction by assignment
	TextureManipulator& operator= (const TextureManipulator&);
public:
	// the accessor function creates a static instance of the class
	static TextureManipulator& instance();

	// RegistryKeyObserver implementation
	void keyChanged(const std::string& key, const std::string& val);

	// Constructs the prefpage
	void constructPreferences();

	void resampleTexture(const void *indata, int inwidth, int inheight, 
						 void *outdata, int outwidth, int outheight, int bytesperpixel);

	void mipReduce(byte *in, byte *out, 
				   int width, int height, 
				   int destwidth, int destheight);

	/* greebo: Returns the readily fabricated pixel data, that passed
	 * a bunch of stages (gamma calculation, mip reduction, stretching) 
	 */
	ImagePtr getProcessedImage(ImagePtr input);

	/* greebo: Performs a fast scan over the pixel data, taking every
	 * 20th pixel to determine the representative flat shade colour 
	 */
	Colour3 getFlatshadeColour(ImagePtr input);

private:

	// Returns the gamma corrected image taken from <input>
	// (Does not allocate new memory)
	ImagePtr processGamma(ImagePtr input);
	
	/* greebo: This ensures that the image has dimensions that 
	 * match a power of two. If it does not, the according length is
	 * stretched to match the next largest power of two.
	 * 
	 * Additionally, this ensures that the image is not larger
	 * than the maximum texture size openGL can handle.
	 */
	ImagePtr getResized(ImagePtr input);
	
	// Recalculates the gamma table according to the given gamma value
	// This is called on first startup or if the user changes the value
	void calculateGammaTable();
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

} // namespace shaders

#endif /*TEXTUREMANIPULATOR_H_*/
