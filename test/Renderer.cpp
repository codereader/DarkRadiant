#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "irender.h"
#include "ilightnode.h"
#include "math/Matrix4.h"
#include "scenelib.h"

namespace test
{

using RendererTest = RadiantTest;
using RenderSystemTest = RadiantTest;

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

    // Construct a projected light with vectors and origin
    static Light projected(const V3& target, const V3& right,
                           const V3& up, const V3& origin)
    {
        Light light = projected(target, right, up);
        light.entity->setKeyValue("origin", string::to_string(origin));
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
    Matrix4 mat = light.getMatrix();

    // At the origin (which is also the light's origin) we have a singularity:
    // the light texture image is infinitely small, which means any X or Y
    // coordinate must go to -INF or +INF in texture space. This is achieved in
    // projective space by setting the W coordinate to 0, while the X/Y/Z
    // coordinates are unchanged (and irrelevant).
    const V4 origin = mat * V4(0, 0, 0, 1);
    EXPECT_EQ(origin, V4(0, 0, 0, 0));

    // Any point on the Z=0 plane should also have the same W coordinate of 0
    EXPECT_EQ((mat * V4(128, 456, 0, 1)).w(), 0);
    EXPECT_EQ((mat * V4(9999, -500, 0, 1)).w(), 0);
    EXPECT_EQ((mat * V4(0.004, 23.3445, 0, 1)).w(), 0);

    // The W coordinate should increase linearly from 0 at the origin to 1 at
    // the target plane.
    V4 projT = mat * V4(TARGET);
    EXPECT_EQ(projT.w(), 1);
    EXPECT_EQ((mat * V4(0.25 * TARGET)).w(), 0.25);
    EXPECT_EQ((mat * V4(0.5 * TARGET)).w(), 0.5);
    EXPECT_EQ((mat * V4(0.75 * TARGET)).w(), 0.75);

    // Target vector points to the center of the projected image, so should have
    // S and T coordinates [0.5, 0.5]
    EXPECT_EQ(projT.x(), 0.5);
    EXPECT_EQ(projT.y(), 0.5);

    // Follow the target vector, then +/- RIGHT should get to the S (X) texture
    // boundaries
    EXPECT_EQ(mat * V4(TARGET + RIGHT), V4(1, 0.5, 1, 1));
    EXPECT_EQ(mat * V4(TARGET - RIGHT), V4(0, 0.5, 1, 1));

    // Likewise, TARGET +/- UP should get to the boundaries of T (Y). Currently
    // the T coordinate is inverted so 0 is at the top and 1 at the bottom.
    EXPECT_EQ(mat * V4(TARGET + UP), V4(0.5, 0, 1, 1));
    EXPECT_EQ(mat * V4(TARGET - UP), V4(0.5, 1, 1, 1));

    // Z coordinate controls the falloff; this should increase from 0 at the
    // origin to 1 at the target plane (so it basically does the same as W for
    // this example)
    EXPECT_EQ(projT.z(), 1);
    EXPECT_EQ((mat * V4(0.25 * TARGET)).z(), 0.25);
    EXPECT_EQ((mat * V4(0.5 * TARGET)).z(), 0.5);
    EXPECT_EQ((mat * V4(0.75 * TARGET)).z(), 0.75);
}

TEST_F(RendererTest, TranslatedProjectedLight)
{
    // This light points directly downwards as with SimpleProjectedLight, but it
    // is not at the origin
    const V3 TARGET(0, 0, -8), UP(0, 4, 0), RIGHT(4, 0, 0);
    const V3 ORIGIN(128, 64, -80);
    Light light = Light::projected(TARGET, RIGHT, UP, ORIGIN);

    Matrix4 mat = light.getMatrix();

    // The light's own origin should transform to [0, 0, 0, 0]
    auto origT = mat * V4(ORIGIN);
    EXPECT_EQ(origT, V4(0, 0, 0, 0));

    // Target vector is relative (it does not update when the light is moved),
    // so we add it to the light origin to get the absolute target point.
    auto targetT = mat * V4(ORIGIN + TARGET);
    EXPECT_EQ(targetT, V4(0.5, 0.5, 1, 1));
}

TEST_F(RendererTest, RotatedProjectedLight)
{
    // Light is at [-16, 0, 0] and is rotated (not targeted) to point towards
    // [16, 0, 0], so directly along the positive X axis.
    const V3 ORIGIN(-16, 0, 0), TARGET(0, 0, -32), UP(0, 8, 0), RIGHT(8, 0, 0);
    Light light = Light::projected(TARGET, RIGHT, UP, ORIGIN);

    // Apply the rotation matrix (rotation key is 3x3)
    light.entity->setKeyValue("rotation", "0 0 1 0 1 0 -1 0 0");

    Matrix4 mat = light.getMatrix();

    // Check the origin
    EXPECT_EQ(mat * V4(ORIGIN), V4(0, 0, 0, 0));

    // World space target should be [16, 0, 0], not [0, 0, -32], due to the
    // rotation
    EXPECT_EQ(mat * V4(16, 0, 0, 1), V4(0.5, 0.5, 1, 1));
}

namespace
{

std::size_t getEntityCount(RenderSystemPtr& renderSystem)
{
    std::size_t count = 0;

    renderSystem->foreachEntity([&](const IRenderEntityPtr&)
    {
        ++count;
    });

    return count;
}

std::size_t getLightCount(RenderSystemPtr& renderSystem)
{
    std::size_t count = 0;

    renderSystem->foreachLight([&](const RendererLightPtr&)
    {
        ++count;
    });

    return count;
}

}

// Ensure that any entity in the scene is connected to the rendersystem
TEST_F(RenderSystemTest, EntityRegistration)
{
    auto rootNode = GlobalMapModule().getRoot();
    auto renderSystem = rootNode->getRenderSystem();

    EXPECT_TRUE(renderSystem);
    EXPECT_EQ(getEntityCount(renderSystem), 0) << "Rendersystem should be pristine";

    auto entity = createByClassName("func_static");
    auto entity2 = createByClassName("func_static");

    scene::addNodeToContainer(entity, rootNode);
    EXPECT_EQ(getEntityCount(renderSystem), 1) << "Rendersystem should contain one entity now";

    scene::addNodeToContainer(entity2, rootNode);
    EXPECT_EQ(getEntityCount(renderSystem), 2) << "Rendersystem should contain two entities now";
    
    scene::removeNodeFromParent(entity);
    EXPECT_EQ(getEntityCount(renderSystem), 1) << "Rendersystem should contain one entity now";

    scene::addNodeToContainer(entity, rootNode);
    EXPECT_EQ(getEntityCount(renderSystem), 2) << "Rendersystem should contain two entities now";

    scene::removeNodeFromParent(entity);
    scene::removeNodeFromParent(entity2);
    EXPECT_EQ(getEntityCount(renderSystem), 0) << "Rendersystem should be empty again";
}

TEST_F(RenderSystemTest, DuplicateEntityRegistration)
{
    auto rootNode = GlobalMapModule().getRoot();
    auto renderSystem = rootNode->getRenderSystem();

    auto entity = createByClassName("func_static");
    scene::addNodeToContainer(entity, rootNode);
    EXPECT_EQ(getEntityCount(renderSystem), 1) << "Rendersystem should contain one entity now";

    // Manually try to register the same entity twice
    EXPECT_THROW(renderSystem->addEntity(entity), std::logic_error);
}

TEST_F(RenderSystemTest, DuplicateEntityDeregistration)
{
    auto rootNode = GlobalMapModule().getRoot();
    auto renderSystem = rootNode->getRenderSystem();

    auto entity = createByClassName("func_static");

    renderSystem->addEntity(entity);
    EXPECT_EQ(getEntityCount(renderSystem), 1) << "Rendersystem should contain one entity now";

    // Manually try to remove the entity
    renderSystem->removeEntity(entity);
    EXPECT_EQ(getEntityCount(renderSystem), 0) << "Rendersystem should be empty now";

    // Same call again should trigger an exception
    EXPECT_THROW(renderSystem->removeEntity(entity), std::logic_error);
}

TEST_F(RenderSystemTest, EntityEnumeration)
{
    auto rootNode = GlobalMapModule().getRoot();
    auto renderSystem = rootNode->getRenderSystem();
    
    auto entity = createByClassName("func_static");
    scene::addNodeToContainer(entity, rootNode);

    std::vector<IRenderEntityPtr> visitedEntities;
    renderSystem->foreachEntity([&](const IRenderEntityPtr& entity)
    {
        visitedEntities.push_back(entity);
    });

    EXPECT_EQ(visitedEntities.size(), 1) << "We should've hit one entity";
    EXPECT_EQ(visitedEntities.front(), entity) << "We should've hit our known entity";
}

TEST_F(RenderSystemTest, LightRegistration)
{
    auto rootNode = GlobalMapModule().getRoot();
    auto renderSystem = rootNode->getRenderSystem();

    EXPECT_TRUE(renderSystem);
    EXPECT_EQ(getEntityCount(renderSystem), 0) << "Rendersystem should be pristine";
    EXPECT_EQ(getLightCount(renderSystem), 0) << "Rendersystem should be pristine";

    auto light = createByClassName("atdm:light_base");
    auto entity = createByClassName("func_static");

    scene::addNodeToContainer(entity, rootNode);
    EXPECT_EQ(getLightCount(renderSystem), 0) << "Rendersystem should still be empty";

    scene::addNodeToContainer(light, rootNode);
    EXPECT_EQ(getLightCount(renderSystem), 1) << "Rendersystem should contain one light now";

    scene::removeNodeFromParent(entity);
    EXPECT_EQ(getLightCount(renderSystem), 1) << "Rendersystem should still contain one light";

    scene::removeNodeFromParent(light);
    EXPECT_EQ(getLightCount(renderSystem), 0) << "Rendersystem should be empty now";
}

TEST_F(RenderSystemTest, AttachmentIsRegisteredAsLight)
{
    auto renderSystem = GlobalMapModule().getRoot()->getRenderSystem();

    EXPECT_EQ(getLightCount(renderSystem), 0) << "Rendersystem should be empty at first";

    auto light = createByClassName("light");
    scene::addNodeToContainer(light, GlobalMapModule().getRoot());

    EXPECT_EQ(getLightCount(renderSystem), 1) << "Rendersystem should know of 1 light now";

    // Insert a static entity with an attached light to the scene
    auto torch = createByClassName("atdm:torch_brazier");
    scene::addNodeToContainer(torch, GlobalMapModule().getRoot());

    EXPECT_EQ(getLightCount(renderSystem), 2) << "Rendersystem should know of 2 lights now";

    // Remove the static entity again
    scene::removeNodeFromParent(torch);

    EXPECT_EQ(getLightCount(renderSystem), 1) << "Rendersystem should know of 1 light after removing the torch";
}

}
