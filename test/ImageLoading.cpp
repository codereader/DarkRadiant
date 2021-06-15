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

    EXPECT_TRUE(std::dynamic_pointer_cast<RGBAImage>(img));
    auto pixels = reinterpret_cast<RGBAPixel*>(img->getPixels());

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

TEST_F(ImageLoadingTest, LoadDDSUncompressed)
{
    auto img = loadImage("textures/dds/test_16x16_uncomp.dds");
    ASSERT_TRUE(img);

    // Check properties are correct
    EXPECT_EQ(img->getWidth(), 16);
    EXPECT_EQ(img->getHeight(), 16);
    EXPECT_EQ(img->getLevels(), 1);

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

    // 5 mipmap levels (16, 8, 4, 2, 1)
    EXPECT_EQ(img->getLevels(), 5);
}

}