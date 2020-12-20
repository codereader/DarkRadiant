#include "RadiantTest.h"

#include "iimage.h"
#include "RGBAImage.h"

namespace test
{

using ImageLoadingTest = RadiantTest;

TEST_F(ImageLoadingTest, LoadPng8Bit)
{
    auto filePath = _context.getTestProjectPath() + "textures/pngs/twentyone_8bit.png";
    auto img = GlobalImageLoader().imageFromFile(filePath);

    EXPECT_EQ(img->getWidth(), 32);
    EXPECT_EQ(img->getHeight(), 32);
}

TEST_F(ImageLoadingTest, LoadPng16Bit)
{
    auto filePath = _context.getTestProjectPath() + "textures/pngs/twentyone_16bit.png";
    auto img = GlobalImageLoader().imageFromFile(filePath);

    EXPECT_EQ(img->getWidth(), 32);
    EXPECT_EQ(img->getHeight(), 32);
}

TEST_F(ImageLoadingTest, LoadPngGreyscaleWithAlpha)
{
    // This is a 8-Bit Greyscale PNG with Alpha channel, so pixel depth is 16 bits
    auto filePath = _context.getTestProjectPath() + "textures/pngs/transparent_greyscale.png";
    auto img = GlobalImageLoader().imageFromFile(filePath);

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

}
