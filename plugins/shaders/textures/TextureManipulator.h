#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

#include "iimage.h"
#include "ishaders.h"
#include "iregistry.h"
#include "preferencesystem.h"
typedef unsigned char byte;

namespace {
	// The OpenGL Texture render modes 
	enum ETexturesMode {
	    eTextures_NEAREST = 0,
	    eTextures_NEAREST_MIPMAP_NEAREST = 1,
	    eTextures_NEAREST_MIPMAP_LINEAR = 2,
	    eTextures_LINEAR = 3,
	    eTextures_LINEAR_MIPMAP_NEAREST = 4,
	    eTextures_LINEAR_MIPMAP_LINEAR = 5,
	};	
}

namespace shaders {

class TextureManipulator :
 	public RegistryKeyObserver
{
	// The gamma correction table
	byte _gammaTable[256];
	
	// The currently active gamma value (0.0...1.0)
	float _gamma;
	
	// The filtering mode (mipmap_linear and such)
	ETexturesMode _textureMode;
	
	// Gets filled in by an OpenGL query
	int _maxTextureSize;
	
	// The image reduction indicator (3 = no reduction, 0 = 12.5%)
	int _textureQuality;
	
public:
	TextureManipulator();
	
	// RegistryKeyObserver implementation
	void keyChanged();

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
	Image* getProcessedImage(Image* input);

	/* greebo: Performs a fast scan over the pixel data, taking every
	 * 20th pixel to determine the representative flat shade colour 
	 */
	Colour3 getFlatshadeColour(Image* input);

	/* greebo: Sends the openGL texturemode commands according to the internal
	 * texture mode member variable. (e.g. MIPMAP_LINEAR)
	 */
	void setTextureParameters();

private:

	// Returns the gamma corrected image taken from <input>
	// (Does not allocate new memory)
	Image* processGamma(Image* input);
	
	/* greebo: This ensures that the image has dimensions that 
	 * match a power of two. If it does not, the according length is
	 * stretched to match the next largest power of two.
	 * 
	 * Additionally, this ensures that the image is not larger
	 * than the maximum texture size openGL can handle.
	 */
	Image* getResized(Image* input);
	
	// Recalculates the gamma table according to the given gamma value
	// This is called on first startup or if the user changes the value
	void calculateGammaTable();
	
	// Converts the <int> to the ETexturesMode enumeration
	ETexturesMode readTextureMode(const unsigned int& mode);
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

} // namespace shaders

#endif /*TEXTUREMANIPULATOR_H_*/
