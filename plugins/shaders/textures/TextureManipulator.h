#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

#include "iimage.h"
typedef unsigned char byte;

namespace shaders {

class TextureManipulator
{
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

private:
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

} // namespace shaders

#endif /*TEXTUREMANIPULATOR_H_*/
