#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "ilightnode.h"
#include "math/Matrix4.h"

namespace test
{

using RendererTest = RadiantTest;

IEntityNodePtr createByClassName(const std::string& className)
{
    auto cls = GlobalEntityClassManager().findClass(className);
    return GlobalEntityModule().createEntity(cls);
}

TEST_F(RendererTest, CreateLightNode)
{
    auto light = createByClassName("light");
    ILightNodePtr node = Node_getLightNode(light);
    ASSERT_TRUE(node);
}

TEST_F(RendererTest, GetLightTextureTransform)
{
    auto light = createByClassName("light");
    Entity* lightEnt = Node_getEntity(light);
    ASSERT_TRUE(lightEnt);

    // Set a radius
    Vector3 SIZE(10, 128, 1002);
    lightEnt->setKeyValue("light_radius", string::to_string(SIZE));

    // Get the RendererLight
    ILightNodePtr node = Node_getLightNode(light);
    ASSERT_TRUE(node);
    const RendererLight& rLight = node->getRendererLight();

    // Get the texture matrix transform
    Matrix4 texMat = rLight.getLightTextureTransformation();

    // Radius is symmetric around the origin, so the scale factor should be
    // 0.5/SIZE, with an offset to map the resulting light-space coordinates from
    // [-0.5, 0.5] onto ST coordinates spanning [0, 1]
    EXPECT_EQ(texMat, Matrix4::byRows(0.5/SIZE.x(), 0, 0, 0.5,
                                      0, 0.5/SIZE.y(), 0, 0.5,
                                      0, 0, 0.5/SIZE.z(), 0.5,
                                      0, 0, 0, 1));
}

}