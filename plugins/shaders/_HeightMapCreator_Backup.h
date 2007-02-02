#ifndef _HEIGHTMAPCREATOR_BACKUP_H_
#define _HEIGHTMAPCREATOR_BACKUP_H_

/* greebo: This is currently unreferenced code to create normal maps from
 * height maps, so this may come in handy when it comes to evaluate
 * the according mapexpression.
 */

inline byte* getPixel(byte* pixels, int width, int height, int x, int y)
{
  return pixels + (((((y + height) % height) * width) + ((x + width) % width)) * 4);
}

class KernelElement
{
public:
  int x, y;
  float w;
};

Image& convertHeightmapToNormalmap(Image& heightmap, float scale)
{
  int w = heightmap.getWidth();
  int h = heightmap.getHeight();
  
  Image& normalmap = *(new RGBAImage(heightmap.getWidth(), heightmap.getHeight()));
  
  byte* in = heightmap.getRGBAPixels();
  byte* out = normalmap.getRGBAPixels();

#if 1
  // no filtering
  const int kernelSize = 2;
  KernelElement kernel_du[kernelSize] = {
    {-1, 0,-0.5f },
    { 1, 0, 0.5f }
  };
  KernelElement kernel_dv[kernelSize] = {
    { 0, 1, 0.5f },
    { 0,-1,-0.5f }
  };
#else
  // 3x3 Prewitt
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
#endif

  int x, y = 0;
  while( y < h )
  {
    x = 0;
    while( x < w )
    {
      float du = 0;
      for(KernelElement* i = kernel_du; i != kernel_du + kernelSize; ++i)
      {
        du += (getPixel(in, w, h, x + (*i).x, y + (*i).y)[0] / 255.0) * (*i).w;
      }
      float dv = 0;
      for(KernelElement* i = kernel_dv; i != kernel_dv + kernelSize; ++i)
      {
        dv += (getPixel(in, w, h, x + (*i).x, y + (*i).y)[0] / 255.0) * (*i).w;
      }

      float nx = -du * scale;
      float ny = -dv * scale;
      float nz = 1.0;

      // Normalize      
      float norm = 1.0/sqrt(nx*nx + ny*ny + nz*nz);
      out[0] = float_to_integer(((nx * norm) + 1) * 127.5);
      out[1] = float_to_integer(((ny * norm) + 1) * 127.5);
      out[2] = float_to_integer(((nz * norm) + 1) * 127.5);
      out[3] = 255;
     
      x++;
      out += 4;
    }
    
    y++;
  }
  
  return normalmap;
}

Image* loadHeightmap(void* environment, const char* name)
{
	ImageConstructorPtr heightLoader(new DefaultConstructor(name));
	Image* heightmap = heightLoader->construct();
  if(heightmap != 0)
  {
    Image& normalmap = convertHeightmapToNormalmap(*heightmap, *reinterpret_cast<float*>(environment));
    heightmap->release();
    return &normalmap;
  }
  return 0;
}

#endif /*_HEIGHTMAPCREATOR_BACKUP_H_*/
