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

// Wrapper for a light entity and its respective node interfaces
struct Light
{
    IEntityNodePtr node;
    ILightNodePtr iLightNode;
    Entity* entity = nullptr;

    Light()
    : node(createByClassName("light"))
    {
        if (node)
        {
            entity = Node_getEntity(node);
            iLightNode = Node_getLightNode(node);
        }
    }
};

TEST_F(RendererTest, CreateLightNode)
{
    Light light;
    ASSERT_TRUE(light.node);
    ASSERT_TRUE(light.entity);
    ASSERT_TRUE(light.iLightNode);
}

TEST_F(RendererTest, GetLightTextureTransform)
{
    Light light;

    // Set a radius
    Vector3 SIZE(10, 128, 1002);
    light.entity->setKeyValue("light_radius", string::to_string(SIZE));

    // Get the RendererLight
    const RendererLight& rLight = light.iLightNode->getRendererLight();

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

TEST_F(RendererTest, UpdateLightRadius)
{
    // Set initial radius
    Light light;
    light.entity->setKeyValue("light_radius", "256 64 512");

    // Save initial matrix
    const RendererLight& rLight = light.iLightNode->getRendererLight();
    const Matrix4 initMat = rLight.getLightTextureTransformation();

    // Change the light radius
    Vector3 SIZE(92, 100, 64);
    light.entity->setKeyValue("light_radius", string::to_string(SIZE));

    // Matrix should have changed from its initial value
    Matrix4 newMat = rLight.getLightTextureTransformation();
    EXPECT_NE(newMat, initMat);

    // New value should be correct
    EXPECT_EQ(newMat, Matrix4::byRows(0.5/SIZE.x(), 0, 0, 0.5,
                                      0, 0.5/SIZE.y(), 0, 0.5,
                                      0, 0, 0.5/SIZE.z(), 0.5,
                                      0, 0, 0, 1));
}

TEST_F(RendererTest, LightCenterDoesNotAffectMatrix)
{
    Light light;
    Vector3 SIZE(100, 128, 2046);
    light.entity->setKeyValue("light_radius", string::to_string(SIZE));

    // Store initial matrix
    const RendererLight& rLight = light.iLightNode->getRendererLight();
    const Matrix4 initMat = rLight.getLightTextureTransformation();

    // Set a light center
    light.entity->setKeyValue("light_center", "50 64 512");

    // Matrix should not have changed
    EXPECT_EQ(rLight.getLightTextureTransformation(), initMat);

    // Make another change just to be sure
    light.entity->setKeyValue("light_center", "0 -1000 2");
    EXPECT_EQ(rLight.getLightTextureTransformation(), initMat);
}

TEST_F(RendererTest, LightMatrixInWorldSpace)
{
    Light light;

    // Set both an origin and a radius
    const Vector3 ORIGIN(128, 64, -192);
    const Vector3 RADIUS(32, 32, 32);
    light.entity->setKeyValue("origin", string::to_string(ORIGIN));
    light.entity->setKeyValue("light_radius", string::to_string(RADIUS));

    // Get the texture matrix transform
    const RendererLight& rLight = light.iLightNode->getRendererLight();
    Matrix4 texMat = rLight.getLightTextureTransformation();

    // Light matrix should subtract the origin scaled to the light bounds (twice
    // the radius), then add the 0.5 offset to get to [0, 1].
    const Vector3 BOUNDS = RADIUS * 2;
    EXPECT_EQ(
        texMat,
        Matrix4::byRows(0.5 / RADIUS.x(), 0, 0, 0.5 - ORIGIN.x() / BOUNDS.x(),
                        0, 0.5 / RADIUS.y(), 0, 0.5 - ORIGIN.y() / BOUNDS.y(),
                        0, 0, 0.5 / RADIUS.z(), 0.5 - ORIGIN.z() / BOUNDS.z(),
                        0, 0, 0, 1)
    );
}

}