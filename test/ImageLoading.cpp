#include "RadiantTest.h"

#include "iimage.h"
#include "RGBAImage.h"

// Helpers for examining pixel data
using RGB8 = BasicVector3<uint8_t>;

// Override operator<< to print RGB8 components as numbers, rather than random
// ASCII characters
std::ostream& operator<< (std::ostream& os, const RGB8& rgb)
{
    return os << "[" << int(rgb.x()) << ", " << int(rgb.y()) << ", "
              << int(rgb.z()) << "]";
}

// Helper class for retrieving pixels by X and Y coordinates and casting them to
// the appropriate pixel type.
template<typename Pixel_T> class Pixelator
{
    const Image& _image;

public:

    // Construct with image to access
    Pixelator(const Image& im): _image(im)
    {}

    // Get pixel at given coordinates
    Pixel_T& operator() (int x, int y)
    {
        Pixel_T* p0 = reinterpret_cast<Pixel_T*>(_image.getPixels());
        return *(p0 + x + y * _image.getWidth());
    }
};

namespace test
{

// Test fixture for image loading. Provides a convenient method to load an image
// relative to the test project path.
class ImageLoadingTest: public RadiantTest
{
protected:

    // Load an image from the given path
    ImagePtr loadImage(const std::string& path)
    {
        auto filePath = _context.getTestProjectPath() + path;
        return GlobalImageLoader().imageFromFile(filePath);
    }
};

TEST_F(ImageLoadingTest, LoadPng8Bit)
{
    auto img = loadImage("textures/pngs/twentyone_8bit.png");

    EXPECT_EQ(img->getWidth(), 32);
    EXPECT_EQ(img->getHeight(), 32);
}

TEST_F(ImageLoadingTest, LoadPng16Bit)
{
    auto img = loadImage("textures/pngs/twentyone_16bit.png");

    EXPECT_EQ(img->getWidth(), 32);
    EXPECT_EQ(img->getHeight(), 32);
}

TEST_F(ImageLoadingTest, LoadPngGreyscaleWithAlpha)
{
    // This is a 8-Bit Greyscale PNG with Alpha channel, so pixel depth is 16 bits
    auto img = loadImage("textures/pngs/transparent_greyscale.png");

    EXPECT_EQ(img->getWidth(), 32);
    EXPECT_EQ(img->getHeight(), 32);
    EXPECT_FALSE(img->isPrecompressed());

    // If the image loader interprets the file correctly, we should have an RGBA
    // image with the colour values being the same for R, G and B.
    // If the image loader didn't convert grey to RGB, the grey value is
    // smeared across the whole RGB channels and they are not uniform

    EXPECT_TRUE(std::dynamic_pointer_cast<image::RGBAImage>(img));
    auto pixels = reinterpret_cast<image::RGBAPixel*>(img->getPixels());

    auto numPixels = img->getWidth() * img->getHeight();
    for (auto i = 0; i < numPixels; ++i)
    {
        EXPECT_EQ(pixels[i].blue, pixels[i].green) << "Expected Green == Blue";
        EXPECT_EQ(pixels[i].red, pixels[i].green) << "Expected Red == Blue";

        if (pixels[i].blue != pixels[i].green || pixels[i].red != pixels[i].green)
        {
            break;
        }
    }
}

TEST_F(ImageLoadingTest, LoadInvalidDDS)
{
    auto img = loadImage("textures/dds/not_a_dds.dds");
    ASSERT_FALSE(img);
}

TEST_F(ImageLoadingTest, LoadDDSUncompressed)
{
    auto img = loadImage("textures/dds/test_16x16_uncomp.dds");
    ASSERT_TRUE(img);

    // Check properties are correct
    EXPECT_EQ(img->getWidth(), 16);
    EXPECT_EQ(img->getHeight(), 16);
    EXPECT_EQ(img->getLevels(), 1);
    EXPECT_FALSE(img->isPrecompressed());

    // Examine pixel data
    Pixelator<RGB8> pixels(*img);
    EXPECT_EQ(pixels(0, 0), RGB8(0, 0, 0));         // border
    EXPECT_EQ(pixels(2, 1), RGB8(255, 255, 255));   // background
    EXPECT_EQ(pixels(6, 7), RGB8(0, 255, 0));       // green band
    EXPECT_EQ(pixels(7, 14), RGB8(255, 255, 0));    // cyan pillar (BGR)
    EXPECT_EQ(pixels(8, 1), RGB8(255, 0, 255));     // magenta pillar
    EXPECT_EQ(pixels(8, 8), RGB8(0, 0, 255));       // red centre (BGR)
    EXPECT_EQ(pixels(14, 13), RGB8(255, 255, 255)); // background
    EXPECT_EQ(pixels(15, 15), RGB8(0, 0, 0));       // border
}

TEST_F(ImageLoadingTest, LoadDDSUncompressedMipMaps)
{
    auto img = loadImage("textures/dds/test_16x16_uncomp_mips.dds");
    ASSERT_TRUE(img);

    // Overall size is unchanged
    EXPECT_EQ(img->getWidth(), 16);
    EXPECT_EQ(img->getHeight(), 16);
    EXPECT_FALSE(img->isPrecompressed());

    // 5 mipmap levels (16, 8, 4, 2, 1)
    EXPECT_EQ(img->getLevels(), 5);
    EXPECT_EQ(img->getWidth(0), 16);
    EXPECT_EQ(img->getWidth(1), 8);
    EXPECT_EQ(img->getHeight(1), 8);
    EXPECT_EQ(img->getWidth(2), 4);
    EXPECT_EQ(img->getHeight(3), 2);
    EXPECT_EQ(img->getHeight(4), 1);
}

TEST_F(ImageLoadingTest, LoadDDSUncompressedNPOT)
{
    auto img = loadImage("textures/dds/test_10x16_uncomp.dds");
    ASSERT_TRUE(img);

    EXPECT_EQ(img->getWidth(), 10);
    EXPECT_EQ(img->getHeight(), 16);
    EXPECT_FALSE(img->isPrecompressed());

    // Examine pixel data
    Pixelator<RGB8> pixels(*img);
    EXPECT_EQ(pixels(0, 0), RGB8(0, 0, 0));      // border
    EXPECT_EQ(pixels(1, 1), RGB8(0, 0, 255));    // red diag
    EXPECT_EQ(pixels(8, 1), RGB8(255, 0, 255));  // magenta pillar
    EXPECT_EQ(pixels(8, 14), RGB8(255, 255, 0)); // cyan pillar
    EXPECT_EQ(pixels(9, 15), RGB8(0, 0, 0));     // border
}

TEST_F(ImageLoadingTest, LoadDDSCompressedDXT1)
{
    auto img = loadImage("textures/dds/test_128x128_dxt1.dds");
    ASSERT_TRUE(img);

    // 128x128 image with no mipmaps
    EXPECT_EQ(img->getWidth(), 128);
    EXPECT_EQ(img->getHeight(), 128);
    EXPECT_EQ(img->getLevels(), 1);

    // Must be compressed
    EXPECT_TRUE(img->isPrecompressed());
    EXPECT_EQ(img->getGLFormat(), GL_COMPRESSED_RGBA_S3TC_DXT1_EXT);
}

TEST_F(ImageLoadingTest, LoadDDSCompressedDXT5NPOT)
{
    auto img = loadImage("textures/dds/test_60x128_dxt5.dds");
    ASSERT_TRUE(img);

    // 60x128 image with no mipmaps
    EXPECT_EQ(img->getWidth(), 60);
    EXPECT_EQ(img->getHeight(), 128);
    EXPECT_EQ(img->getLevels(), 1);

    // Must be compressed
    EXPECT_TRUE(img->isPrecompressed());
    EXPECT_EQ(img->getGLFormat(), GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);
}

TEST_F(ImageLoadingTest, LoadDDSCompressedDXT5MipMapsNPOT)
{
    auto img = loadImage("textures/dds/test_60x128_dxt5_mips.dds");
    ASSERT_TRUE(img);

    // 60x128 image with 8 mipmaps
    EXPECT_EQ(img->getWidth(), 60);
    EXPECT_EQ(img->getHeight(), 128);
    EXPECT_EQ(img->getLevels(), 8);

    // Must be compressed
    EXPECT_TRUE(img->isPrecompressed());
    EXPECT_EQ(img->getGLFormat(), GL_COMPRESSED_RGBA_S3TC_DXT5_EXT);

    // Check mipmap size sequence
    EXPECT_EQ(img->getWidth(1), 30);
    EXPECT_EQ(img->getHeight(1), 64);
    EXPECT_EQ(img->getWidth(2), 15);
    EXPECT_EQ(img->getHeight(2), 32);
    EXPECT_EQ(img->getWidth(3), 7);
    EXPECT_EQ(img->getHeight(3), 16);
    EXPECT_EQ(img->getWidth(4), 3);
    EXPECT_EQ(img->getHeight(4), 8);
    EXPECT_EQ(img->getWidth(5), 1);
    EXPECT_EQ(img->getHeight(5), 4);
    EXPECT_EQ(img->getWidth(6), 1);
    EXPECT_EQ(img->getHeight(6), 2);
    EXPECT_EQ(img->getWidth(7), 1);
    EXPECT_EQ(img->getHeight(7), 1);
}

TEST_F(ImageLoadingTest, LoadDDSCompressedBC5MipMaps)
{
    auto img = loadImage("textures/dds/test_16x16_bc5.dds");
    ASSERT_TRUE(img);

    // 16x16 image
    EXPECT_EQ(img->getWidth(), 16);
    EXPECT_EQ(img->getHeight(), 16);
    EXPECT_EQ(img->getLevels(), 5);

    // Check compressed GL format
    EXPECT_EQ(img->getGLFormat(), GL_COMPRESSED_RG_RGTC2);
}

}