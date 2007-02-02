#ifndef TEXTUREMANIPULATOR_H_
#define TEXTUREMANIPULATOR_H_

typedef unsigned char byte;

class TextureManipulator
{
	
public:
	TextureManipulator() 
	{}
	
	void resampleTexture(const void *indata, int inwidth, int inheight, 
						 void *outdata, int outwidth, int outheight, int bytesperpixel);
	
	void mipReduce(byte *in, byte *out, 
				   int width, int height, 
				   int destwidth, int destheight);

	void resampleGamma(float gamma);

private:
	
	void resampleTextureLerpLine(const byte *in, byte *out, 
								 int inwidth, int outwidth, int bytesperpixel);

}; // class TextureManipulator

#endif /*TEXTUREMANIPULATOR_H_*/
