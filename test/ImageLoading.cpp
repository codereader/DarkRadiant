#include "RadiantTest.h"

#include "iimage.h"
#include "RGBAImage.h"

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

    EXPECT_EQ(img->getWidth(), 16);
    EXPECT_EQ(img->getHeight(), 16);
}

}