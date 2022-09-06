#ifndef HEIGHTMAPCREATOR_H_
#define HEIGHTMAPCREATOR_H_

namespace shaders {

// Helper function, wraps around at the borders to prevent buffer overflows
// Note: the <pixels> argument MUST point to the beginning of the buffer
inline byte* getPixel(byte* pixels, std::size_t width, std::size_t height, std::size_t x, std::size_t y) {
  return pixels + (((((y + height) % height) * width) + ((x + width) % width)) * 4);
}

/** greebo: This creates a normalmap for the given heightmap
 *
 * Note: The source image is NOT released from memory, this is the
 * 		 responsibility of the calling method.
 */
ImagePtr createNormalmapFromHeightmap(ImagePtr heightMap, float scale) {
	assert(heightMap);

	std::size_t width = heightMap->getWidth();
	std::size_t height = heightMap->getHeight();

	ImagePtr normalMap (new image::RGBAImage(width, height));

	byte* in = heightMap->getPixels();
	byte* out = normalMap->getPixels();

	struct KernelElement
	{
		int x, y;
		float w;
	};

	// if you want to understand the code below, read http://en.wikipedia.org/wiki/Edge_detection

	/* // no filtering
	const int kernelSize = 2;
	KernelElement kernel_du[kernelSize] = {
    		{-1, 0,-0.5f },
		{ 1, 0, 0.5f }
	};
	KernelElement kernel_dv[kernelSize] = {
		{ 0, 1, 0.5f },
		{ 0,-1,-0.5f }
	}; */

	// 3x3 Prewitt filtering
	const int kernelSize = 6;
	KernelElement kernel_du[kernelSize] = {
		{-1, 1,-1.0f },
		{-1, 0,-1.0f },
		{-1,-1,-1.0f },
		{ 1, 1, 1.0f },
		{ 1, 0, 1.0f },
		{ 1,-1, 1.0f }
	};
	KernelElement kernel_dv[kernelSize] = {
		{-1, 1, 1.0f },
		{ 0, 1, 1.0f },
		{ 1, 1, 1.0f },
		{-1,-1,-1.0f },
		{ 0,-1,-1.0f },
		{ 1,-1,-1.0f }
	};

	// pixel counter
	std::size_t x, y = 0;
	while( y < height ) {
		x = 0;
		while( x < width ) {
			float du = 0;
			for(KernelElement* i = kernel_du; i != kernel_du + kernelSize; ++i) {
				du += (getPixel(in, width, height, x + (*i).x, y + (*i).y)[0] / 255.0f) * (*i).w;
			}
			float dv = 0;
			for(KernelElement* i = kernel_dv; i != kernel_dv + kernelSize; ++i) {
				dv += (getPixel(in, width, height, x + (*i).x, y + (*i).y)[0] / 255.0f) * (*i).w;
			}

			float nx = -du * scale;
			float ny = -dv * scale;
			float nz = 1.0;

			// Normalize
			float norm = 1.0f/sqrt(nx*nx + ny*ny + nz*nz);
			out[0] = static_cast<byte>(float_to_integer(((nx * norm) + 1) * 127.5));
			out[1] = static_cast<byte>(float_to_integer(((ny * norm) + 1) * 127.5));
			out[2] = static_cast<byte>(float_to_integer(((nz * norm) + 1) * 127.5));
			out[3] = 255;

			x++;
			out += 4;
		}
		y++;
	}

	return normalMap;
}

} // namespace shaders

#endif /*HEIGHTMAPCREATOR_H_*/
