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

using V4 = Vector4;
using V3 = Vector3;

// Wrapper for a light entity and its respective node interfaces
struct Light
{
    IEntityNodePtr node;
    ILightNodePtr iLightNode;
    Entity* entity = nullptr;

    // Construct with no properties
    Light()
    : node(createByClassName("light"))
    {
        if (node)
        {
            entity = Node_getEntity(node);
            iLightNode = Node_getLightNode(node);
        }
    }

    // Return the light texture matrix
    Matrix4 getMatrix() const
    {
        return iLightNode->getRendererLight().getLightTextureTransformation();
    }

    // Construct a light with a specified radius and default origin
    static Light withRadius(const V3& radius)
    {
        Light light;
        light.entity->setKeyValue("light_radius", string::to_string(radius));
        return light;
    }

    // Construct a projected light with specified vectors
    static Light projected(const V3& target, const V3& right,
                           const V3& up)
    {
        Light light;
        light.entity->setKeyValue("light_target", string::to_string(target));
        light.entity->setKeyValue("light_right", string::to_string(right));
        light.entity->setKeyValue("light_up", string::to_string(up));
        return light;
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
    V3 SIZE(10, 128, 1002);
    Light light = Light::withRadius(SIZE);

    // Get the texture matrix transform
    Matrix4 texMat = light.getMatrix();

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
    Light light = Light::withRadius(V3(256, 64, 512));

    // Save initial matrix
    const Matrix4 initMat = light.getMatrix();

    // Change the light radius
    V3 SIZE(92, 100, 64);
    light.entity->setKeyValue("light_radius", string::to_string(SIZE));

    // Matrix should have changed from its initial value
    Matrix4 newMat = light.getMatrix();
    EXPECT_NE(newMat, initMat);

    // New value should be correct
    EXPECT_EQ(newMat, Matrix4::byRows(0.5/SIZE.x(), 0, 0, 0.5,
                                      0, 0.5/SIZE.y(), 0, 0.5,
                                      0, 0, 0.5/SIZE.z(), 0.5,
                                      0, 0, 0, 1));
}

TEST_F(RendererTest, LightCenterDoesNotAffectMatrix)
{
    V3 SIZE(100, 128, 2046);
    Light light = Light::withRadius(SIZE);

    // Store initial matrix
    const Matrix4 initMat = light.getMatrix();

    // Set a light center
    light.entity->setKeyValue("light_center", "50 64 512");

    // Matrix should not have changed
    EXPECT_EQ(light.getMatrix(), initMat);

    // Make another change just to be sure
    light.entity->setKeyValue("light_center", "0 -1000 2");
    EXPECT_EQ(light.getMatrix(), initMat);
}

TEST_F(RendererTest, LightMatrixInWorldSpace)
{
    const V3 RADIUS(32, 32, 32);
    Light light = Light::withRadius(RADIUS);

    // Set an origin
    const V3 ORIGIN(128, 64, -192);
    light.entity->setKeyValue("origin", string::to_string(ORIGIN));

    // Light matrix should subtract the origin scaled to the light bounds (twice
    // the radius), then add the 0.5 offset to get to [0, 1].
    Matrix4 texMat = light.getMatrix();
    const V3 BOUNDS = RADIUS * 2;
    EXPECT_EQ(
        texMat,
        Matrix4::byRows(0.5 / RADIUS.x(), 0, 0, 0.5 - ORIGIN.x() / BOUNDS.x(),
                        0, 0.5 / RADIUS.y(), 0, 0.5 - ORIGIN.y() / BOUNDS.y(),
                        0, 0, 0.5 / RADIUS.z(), 0.5 - ORIGIN.z() / BOUNDS.z(),
                        0, 0, 0, 1)
    );
}

TEST_F(RendererTest, SimpleProjectedLight)
{
    // Create a light at the origin, pointing directly downwards
    const V3 TARGET(0, 0, -8), UP(0, 4, 0), RIGHT(4, 0, 0);
    Light light = Light::projected(TARGET, RIGHT, UP);

    // Inspect the matrix by transforming some key points into texture space.
    // Note that we are dealing with projective geometry so we need 4 element
    // vectors to handle the W coordinate.
    Matrix4 texMat = light.getMatrix();

    // At the origin (which is also the light's origin) we have a singularity:
    // the light texture image is infinitely small, which means any X or Y
    // coordinate must go to -INF or +INF in texture space. This is achieved in
    // projective space by setting the W coordinate to 0, while the X/Y/Z
    // coordinates are unchanged (and irrelevant).
    const V4 origin = texMat.transform(V4(0, 0, 0, 1));
    EXPECT_EQ(origin, V4(0, 0, 0, 0));

    // Any point on the Z=0 plane should also have the same W coordinate of 0
    EXPECT_EQ(texMat.transform(V4(128, 456, 0, 1)).w(), 0);
    EXPECT_EQ(texMat.transform(V4(9999, -500, 0, 1)).w(), 0);
    EXPECT_EQ(texMat.transform(V4(0.004, 23.3445, 0, 1)).w(), 0);

    // The W coordinate should increase linearly from 0 at the origin to 1 at
    // the target plane.
    EXPECT_EQ(texMat.transform(V4(0.25 * TARGET)).w(), 0.25);
    EXPECT_EQ(texMat.transform(V4(0.5 * TARGET)).w(), 0.5);
    EXPECT_EQ(texMat.transform(V4(0.75 * TARGET)).w(), 0.75);
    EXPECT_EQ(texMat.transform(V4(TARGET)).w(), 1);
}

}