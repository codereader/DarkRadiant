#include "RadiantTest.h"

#include "iimage.h"

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

}
