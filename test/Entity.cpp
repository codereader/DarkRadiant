#include "RadiantTest.h"

#include "ieclass.h"
#include "ientity.h"
#include "irendersystemfactory.h"
#include "iselectable.h"
#include "iselection.h"
#include "iundo.h"
#include "ishaders.h"
#include "icolourscheme.h"
#include "ieclasscolours.h"

#include "render/NopVolumeTest.h"
#include "string/convert.h"
#include "transformlib.h"
#include "registry/registry.h"
#include "eclass.h"
#include "string/join.h"
#include "scenelib.h"
#include "algorithm/Entity.h"
#include "algorithm/Scene.h"

namespace test
{

using EntityTest = RadiantTest;

namespace
{

// Create an entity from a simple classname string
IEntityNodePtr createByClassName(const std::string& className)
{
    auto cls = GlobalEntityClassManager().findClass(className);
    return GlobalEntityModule().createEntity(cls);
}

// Obtain entity attachments as a simple std::list
std::list<Entity::Attachment> getAttachments(const IEntityNodePtr& node)
{
    std::list<Entity::Attachment> attachments;
    if (node)
    {
        node->getEntity().forEachAttachment(
            [&](const Entity::Attachment& a) { attachments.push_back(a); }
        );
    }
    return attachments;
}

}

using StringMap = std::map<std::string, std::string>;

TEST_F(EntityTest, LookupEntityClass)
{
    // Nonexistent class should return null (but not throw or crash)
    auto cls = GlobalEntityClassManager().findClass("notAnEntityClass");
    EXPECT_FALSE(cls);

    // Real entity class should return a valid pointer
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls);
}

TEST_F(EntityTest, LightEntitiesRecognisedAsLights)
{
    // The 'light' class should be recognised as an actual light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls->isLight());

    // Things which are not lights should also be correctly identified
    auto notLightCls = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");
    EXPECT_TRUE(notLightCls);
    EXPECT_FALSE(notLightCls->isLight());

    // Anything deriving from the light class should also be a light
    auto derived1 = GlobalEntityClassManager().findClass("atdm:light_base");
    EXPECT_TRUE(derived1->isLight());

    // Second level derivations too
    auto derived2 = GlobalEntityClassManager().findClass("light_extinguishable");
    EXPECT_TRUE(derived2->isLight());

    // torch_brazier is not a light itself, but has a light attached, so it
    // should not have isLight() == true
    auto brazier = GlobalEntityClassManager().findClass("atdm:torch_brazier");
    EXPECT_FALSE(brazier->isLight());
}

TEST_F(EntityTest, EntityClassInheritsAttributes)
{
    auto cls = GlobalEntityClassManager().findClass("light_extinguishable");
    ASSERT_TRUE(cls);

    // Inherited from 'light'
    EXPECT_EQ(cls->getAttribute("editor_color").getValue(), "0 1 0");
    EXPECT_EQ(cls->getAttribute("spawnclass").getValue(), "idLight");

    // Inherited from 'atdm:light_base'
    EXPECT_EQ(cls->getAttribute("AIUse").getValue(), "AIUSE_LIGHTSOURCE");
    EXPECT_EQ(cls->getAttribute("shouldBeOn").getValue(), "0");

    // Inherited but overridden on 'light_extinguishable' itself
    EXPECT_EQ(cls->getAttribute("editor_displayFolder").getValue(),
              "Lights/Base Entities, DoNotUse");

    // Lookup without considering inheritance
    EXPECT_EQ(cls->getAttribute("editor_color", false).getValue(), "");
    EXPECT_EQ(cls->getAttribute("spawnclass", false).getValue(), "");
}

TEST_F(EntityTest, VisitInheritedClassAttributes)
{
    auto cls = GlobalEntityClassManager().findClass("light_extinguishable");
    ASSERT_TRUE(cls);

    // Map of attribute names to inherited flag
    std::map<std::string, bool> attributes;

    // Visit all attributes and store in the map
    cls->forEachAttribute(
        [&attributes](const EntityClassAttribute& a, bool inherited) {
            attributes.insert({a.getName(), inherited});
        },
        true /* editorKeys */
    );

    // Confirm inherited flags set correctly
    EXPECT_EQ(attributes.at("spawnclass"), true);
    EXPECT_EQ(attributes.at("AIUse"), true);
    EXPECT_EQ(attributes.at("maxs"), false);
    EXPECT_EQ(attributes.at("clipmodel_contents"), false);
    EXPECT_EQ(attributes.at("editor_displayFolder"), false);
}

// #5621: When the classname key is selected in the entity inspector, the description of that
// attribute should deliver the text that is stored in the editor_usage attributes
TEST_F(EntityTest, MultiLineEditorUsage)
{
    auto eclass = GlobalEntityClassManager().findClass("eclass_with_usage_attribute");
    ASSERT_TRUE(eclass);

    // Assume we have non-empty editor_usage/1/2 attributes
    EXPECT_NE(eclass->getAttribute("editor_usage").getValue(), "");
    EXPECT_NE(eclass->getAttribute("editor_usage1").getValue(), "");
    EXPECT_NE(eclass->getAttribute("editor_usage2").getValue(), "");

    auto editor_usage = eclass::getUsage(*eclass);

    std::vector<std::string> singleAttributes =
    {
        eclass->getAttribute("editor_usage").getValue(),
        eclass->getAttribute("editor_usage1").getValue(),
        eclass->getAttribute("editor_usage2").getValue()
    };

    EXPECT_EQ(editor_usage, string::join(singleAttributes, "\n"));
}

void checkBucketEntityDef(const IEntityClassPtr& eclass)
{
    // These spawnargs are all defined directly on bucket_metal
    EXPECT_EQ(eclass->getAttribute("editor_usage").getValue(), "So you can kick the bucket.");
    EXPECT_EQ(eclass->getAttribute("editor_displayFolder").getValue(), "Moveables/Containers");
    EXPECT_EQ(eclass->getAttribute("mass").getValue(), "8");
    EXPECT_EQ(eclass->getAttribute("inherit").getValue(), "bucket_base");
    EXPECT_EQ(eclass->getAttribute("model").getValue(), "models/darkmod/containers/bucket.lwo");
    EXPECT_EQ(eclass->getAttribute("friction").getValue(), "0.2");
    EXPECT_EQ(eclass->getAttribute("clipmodel").getValue(), "models/darkmod/misc/clipmodels/bucket_cm.lwo");
    EXPECT_EQ(eclass->getAttribute("bouncyness").getValue(), "0.5");
    EXPECT_EQ(eclass->getAttribute("snd_bounce").getValue(), "tdm_impact_metal_bucket");
    EXPECT_EQ(eclass->getAttribute("snd_bounce_carpet").getValue(), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttribute("snd_bounce_cloth").getValue(), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttribute("snd_bounce_grass").getValue(), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttribute("snd_bounce_dirt").getValue(), "tdm_impact_metal_bucket_on_soft");

    // This is defined in the parent entityDef:
    EXPECT_EQ(eclass->getAttribute("snd_bounce_snow").getValue(), "tdm_impact_dirt");
}

// #5652: Reloading DEFs must not mess up the eclass attributes
TEST_F(EntityTest, ReloadDefsOnUnchangedFiles)
{
    auto eclass = GlobalEntityClassManager().findClass("bucket_metal");
    auto parent = eclass->getParent();

    // Check the parent, it defines an editor_usage which will mess up the child (when the #5652 problem was unfixed)
    EXPECT_TRUE(parent != nullptr);
    EXPECT_EQ(parent->getAttribute("editor_usage").getValue(), "Don't use. Base class for all TDM moveables.");

    checkBucketEntityDef(eclass);

    // Reload the defs and re-use the same eclass reference we got above, it ought to be still valid
    GlobalEntityClassManager().reloadDefs();

    checkBucketEntityDef(eclass);
}

TEST_F(EntityTest, CannotCreateEntityWithoutClass)
{
    // Creating with a null entity class should throw an exception
    EXPECT_THROW(GlobalEntityModule().createEntity({}), std::runtime_error);
}

TEST_F(EntityTest, CreateBasicLightEntity)
{
    // Create a basic light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    auto light = GlobalEntityModule().createEntity(lightCls);

    // Light has a sensible autogenerated name
    EXPECT_EQ(light->name(), "light_1");

    // Entity should have a "classname" key matching the actual entity class we
    // created
    auto clsName = light->getEntity().getKeyValue("classname");
    EXPECT_EQ(clsName, "light");

    // Entity should have an IEntityClass pointer which matches the one we
    // looked up
    EXPECT_EQ(light->getEntity().getEntityClass().get(), lightCls.get());

    // This basic light entity should have no attachments
    auto attachments = getAttachments(light);
    EXPECT_EQ(attachments.size(), 0);
}

TEST_F(EntityTest, EnumerateEntitySpawnargs)
{
    auto light = createByClassName("light");
    auto& spawnArgs = light->getEntity();

    // Visit spawnargs by key and value string
    StringMap keyValuesInit;
    spawnArgs.forEachKeyValue([&](const std::string& k, const std::string& v) {
        keyValuesInit.insert({k, v});
    });

    // Initial entity should have a name and a classname value and no other
    // properties
    EXPECT_EQ(keyValuesInit.size(), 2);
    EXPECT_EQ(keyValuesInit["name"], light->name());
    EXPECT_EQ(keyValuesInit["classname"], "light");

    // Add some new properties of our own
    spawnArgs.setKeyValue("origin", "128 256 -1024");
    spawnArgs.setKeyValue("_color", "0.5 0.5 0.5");

    // Ensure that our new properties are also enumerated
    StringMap keyValuesAll;
    spawnArgs.forEachKeyValue([&](const std::string& k, const std::string& v) {
        keyValuesAll.insert({k, v});
    });
    EXPECT_EQ(keyValuesAll.size(), 4);
    EXPECT_EQ(keyValuesAll["origin"], "128 256 -1024");
    EXPECT_EQ(keyValuesAll["_color"], "0.5 0.5 0.5");

    // Enumerate as full EntityKeyValue objects as well as strings
    StringMap keyValuesByObj;
    spawnArgs.forEachEntityKeyValue(
        [&](const std::string& k, const EntityKeyValue& v) {
            keyValuesByObj.insert({k, v.get()});
        }
    );
    EXPECT_EQ(keyValuesAll, keyValuesByObj);
}

TEST_F(EntityTest, EnumerateInheritedSpawnargs)
{
    auto light = createByClassName("atdm:light_base");
    auto& spawnArgs = light->getEntity();

    // Enumerate all keyvalues including the inherited ones
    StringMap keyValues;
    spawnArgs.forEachKeyValue(
        [&](const std::string& k, const std::string& v) {
            keyValues.insert({k, v});
        },
        true /* includeInherited */
    );

    // Check we have some inherited properties from the entitydef (including
    // spawnclass from the entitydef's own parent def)
    EXPECT_EQ(keyValues["spawnclass"], "idLight");
    EXPECT_EQ(keyValues["shouldBeOn"], "0");
    EXPECT_EQ(keyValues["AIUse"], "AIUSE_LIGHTSOURCE");
    EXPECT_EQ(keyValues["noshadows"], "0");
}

TEST_F(EntityTest, GetKeyValuePairs)
{
    auto torch = createByClassName("atdm:torch_brazier");
    auto& spawnArgs = torch->getEntity();

    using Pair = Entity::KeyValuePairs::value_type;

    // Retrieve single spawnargs as single-element lists of pairs
    auto classNamePairs = spawnArgs.getKeyValuePairs("classname");
    EXPECT_EQ(classNamePairs.size(), 1);
    EXPECT_EQ(classNamePairs[0], Pair("classname", "atdm:torch_brazier"));

    auto namePairs = spawnArgs.getKeyValuePairs("name");
    EXPECT_EQ(namePairs.size(), 1);
    EXPECT_EQ(namePairs[0], Pair("name", "atdm_torch_brazier_1"));

    // Add some spawnargs with a common prefix
    const StringMap SR_KEYS{
        {"sr_type_1", "blah"},
        {"sr_type_2", "bleh"},
        {"sR_tYpE_a", "123"},
        {"SR_type_1a", "0 123 -120"},
    };
    for (const auto& pair: SR_KEYS)
        spawnArgs.setKeyValue(pair.first, pair.second);

    // Confirm all added prefix keys are found regardless of case
    auto srPairs = spawnArgs.getKeyValuePairs("sr_type");
    EXPECT_EQ(srPairs.size(), SR_KEYS.size());
    for (const auto& pair: srPairs)
        EXPECT_EQ(SR_KEYS.at(pair.first), pair.second);
}

TEST_F(EntityTest, CopySpawnargs)
{
    auto light = createByClassName("atdm:light_base");
    auto& spawnArgs = light->getEntity();

    // Add some custom spawnargs to copy
    const StringMap EXTRA_SPAWNARGS{{"first", "1"},
                                    {"second", "two"},
                                    {"THIRD", "3333"},
                                    {"_color", "1 0 1"}};

    for (const auto& pair: EXTRA_SPAWNARGS)
        spawnArgs.setKeyValue(pair.first, pair.second);

    // Clone the entity node
    auto lightCopy = light->clone();
    Entity* clonedEnt = Node_getEntity(lightCopy);
    ASSERT_TRUE(clonedEnt);

    // Clone should have all the same spawnarg strings
    std::size_t count = 0;
    clonedEnt->forEachKeyValue([&](const std::string& k, const std::string& v) {
        EXPECT_EQ(spawnArgs.getKeyValue(k), v);
        ++count;
    });
    EXPECT_EQ(count, EXTRA_SPAWNARGS.size() + 2 /* name and classname */);

    // Clone should NOT have the same actual KeyValue object pointers, although
    // the count should be the same
    std::set<EntityKeyValue*> origPointers;
    std::set<EntityKeyValue*> copiedPointers;
    spawnArgs.forEachEntityKeyValue(
        [&](const std::string& k, EntityKeyValue& v) {
            origPointers.insert(&v);
        });
    clonedEnt->forEachEntityKeyValue(
        [&](const std::string& k, EntityKeyValue& v) {
            copiedPointers.insert(&v);
        });
    EXPECT_EQ(origPointers.size(), count);
    EXPECT_EQ(copiedPointers.size(), count);

    std::vector<EntityKeyValue*> overlap;
    std::set_intersection(origPointers.begin(), origPointers.end(),
                          copiedPointers.begin(), copiedPointers.end(),
                          std::back_inserter(overlap));
    EXPECT_EQ(overlap.size(), 0);
}

TEST_F(EntityTest, SelectEntity)
{
    auto light = createByClassName("light");

    // Confirm that setting entity node's selection status propagates to the
    // selection system
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
    Node_getSelectable(light)->setSelected(true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);
    Node_getSelectable(light)->setSelected(false);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
}

TEST_F(EntityTest, DestroySelectedEntity)
{
    auto light = createByClassName("light");

    // Confirm that setting entity node's selection status propagates to the
    // selection system
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 0);
    Node_getSelectable(light)->setSelected(true);
    EXPECT_EQ(GlobalSelectionSystem().countSelected(), 1);

    // Destructor called here and should not crash
}

namespace
{
    // A simple RenderableCollector which just logs/stores whatever is submitted
    struct TestRenderableCollector: public RenderableCollector
    {
        // Count of submitted renderables and lights
        int renderables = 0;
        int lights = 0;

        // List of actual RendererLight objects
        std::list<const RendererLight*> lightPtrs;

        // List of renderables and their shaders
        std::vector< std::pair<const Shader*, const OpenGLRenderable*> > renderablePtrs;

        void addRenderable(Shader& shader, const OpenGLRenderable& renderable,
                           const Matrix4& localToWorld,
                           const LitObject* litObject = nullptr,
                           const IRenderEntity* entity = nullptr) override
        {
            ++renderables;
            renderablePtrs.push_back(std::make_pair(&shader, &renderable));
        }

        void addLight(const RendererLight& light)
        {
            ++lights;
            lightPtrs.push_back(&light);
        }

        bool supportsFullMaterials() const override { return true; }
        void setHighlightFlag(Highlight::Flags flags, bool enabled) override
        {}
    };

    // Collection of objects needed for rendering. Since not all tests require
    // rendering, these objects are in an auxiliary fixture created when needed
    // rather than part of the EntityTest fixture used by every test. This class
    // also implements scene::NodeVisitor enabling it to visit trees of nodes
    // for rendering.
    struct RenderFixture: public scene::NodeVisitor
    {
        RenderSystemPtr backend = GlobalRenderSystemFactory().createRenderSystem();
        render::NopVolumeTest volumeTest;
        TestRenderableCollector collector;

        // Whether to render solid or wireframe
        const bool renderSolid;

        // Keep track of nodes visited
        int nodesVisited = 0;

        // Construct
        RenderFixture(bool solid = false): renderSolid(solid)
        {}

        // Convenience method to set render backend and traverse a node and its
        // children for rendering
        void renderSubGraph(const scene::INodePtr& node)
        {
            node->setRenderSystem(backend);
            node->traverse(*this);
        }

        // NodeVisitor implementation
        bool pre(const scene::INodePtr& node) override
        {
            // Count the node itself
            ++nodesVisited;

            // Render the node in appropriate mode
            if (renderSolid)
                node->renderSolid(collector, volumeTest);
            else
                node->renderWireframe(collector, volumeTest);

            // Continue traversing
            return true;
        }
    };
}

TEST_F(EntityTest, ModifyEntityClass)
{
    auto cls = GlobalEntityClassManager().findClass("light");
    auto light = GlobalEntityModule().createEntity(cls);
    auto& spawnArgs = light->getEntity();

    // Light doesn't initially have a colour set
    RenderFixture rf;
    light->setRenderSystem(rf.backend);
    const ShaderPtr origWireShader = light->getWireShader();
    ASSERT_TRUE(origWireShader);

    // The shader shouldn't just change by itself (this would invalidate the
    // test)
    EXPECT_EQ(light->getWireShader(), origWireShader);

    // Set a new colour value on the entity *class* (not the entity)
    cls->setColour(Vector3(0.5, 0.24, 0.87));

    // Shader should have changed due to the entity class update (although there
    // aren't currently any public Shader properties that we can examine to
    // confirm its contents)
    EXPECT_NE(light->getWireShader(), origWireShader);
}

TEST_F(EntityTest, LightLocalToWorldFromOrigin)
{
    auto light = createByClassName("light");

    // Initial localToWorld should be identity
    EXPECT_EQ(light->localToWorld(), Matrix4::getIdentity());

    // Set an origin
    const Vector3 ORIGIN(123, 456, -10);
    light->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // localToParent should reflect the new origin
    auto transformNode = std::dynamic_pointer_cast<ITransformNode>(light);
    ASSERT_TRUE(transformNode);
    EXPECT_EQ(transformNode->localToParent(), Matrix4::getTranslation(ORIGIN));

    // Since there is no parent, the final localToWorld should be the same as
    // localToParent
    EXPECT_EQ(light->localToWorld(), Matrix4::getTranslation(ORIGIN));
}

TEST_F(EntityTest, LightWireframeShader)
{
    auto light = createByClassName("light");

    // Initially there is no shader because there is no rendersystem
    auto wireSh = light->getWireShader();
    EXPECT_FALSE(wireSh);

    // Set a render system
    RenderSystemPtr backend = GlobalRenderSystemFactory().createRenderSystem();
    light->setRenderSystem(backend);

    // There should be a shader now
    auto newWireSh = light->getWireShader();
    ASSERT_TRUE(newWireSh);

    // Get the name for the shader. Since this is a simple built-in wireframe
    // shader, this should be an internally-constructed name based on the entity
    // colour. Note that this colour is derived from the entity *class*, which
    // for "light" is a default green. Actual lights will be rendered with a
    // colour based on their _color key.
    EXPECT_EQ(newWireSh->getName(), "<0.000000 1.000000 0.000000>");
}

TEST_F(EntityTest, LightVolumeColorFromColorKey)
{
    // Create a default light
    auto light = createByClassName("light");

    {
        // Render the default light
        RenderFixture rf;
        rf.renderSubGraph(light);

        // Shader should have been submitted. Since a light's default _color is
        // white, this is the shader we should get for rendering.
        EXPECT_EQ(rf.collector.renderables, 1);
        const Shader* shader = rf.collector.renderablePtrs.at(0).first;
        ASSERT_TRUE(shader);
        EXPECT_EQ(shader->getName(), "<1.000000 1.000000 1.000000>");
    }

    // Set a different colour on the light
    light->getEntity().setKeyValue("_color", "0.75 0.25 0.1");

    {
        // Re-render the light
        RenderFixture rf;
        rf.renderSubGraph(light);

        // The shader should have changed to match the new _color
        EXPECT_EQ(rf.collector.renderables, 1);
        const Shader* shader = rf.collector.renderablePtrs.at(0).first;
        ASSERT_TRUE(shader);
        EXPECT_EQ(shader->getName(), "<0.750000 0.250000 0.100000>");
    }
}

TEST_F(EntityTest, OverrideLightVolumeColour)
{
    // Create a light with an arbitrary colour
    auto light = createByClassName("light");
    light->getEntity().setKeyValue("_color", "0.25 0.55 0.9");

    // Set the "override light volume colour" key
    registry::setValue(colours::RKEY_OVERRIDE_LIGHTCOL, true);

    {
        RenderFixture rf;
        rf.renderSubGraph(light);

        // The shader should ignore the _color key and render based on the entity
        // class colour
        EXPECT_EQ(rf.collector.renderables, 1);
        const Shader* shader = rf.collector.renderablePtrs.at(0).first;
        ASSERT_TRUE(shader);
        EXPECT_EQ(shader->getName(), "<0.000000 1.000000 0.000000>");
    }

    // Unset the override key
    registry::setValue(colours::RKEY_OVERRIDE_LIGHTCOL, false);

    {
        RenderFixture rf;
        rf.renderSubGraph(light);

        // Light should be rendered with its original _color key again
        EXPECT_EQ(rf.collector.renderables, 1);
        const Shader* shader = rf.collector.renderablePtrs.at(0).first;
        ASSERT_TRUE(shader);
        EXPECT_EQ(shader->getName(), "<0.250000 0.550000 0.900000>");
    }

    // Changing the override key after deleting the light must not crash
    // (because the LightNode's CachedKey is sigc::trackable)
    light.reset();
    registry::setValue(colours::RKEY_OVERRIDE_LIGHTCOL, true);
    registry::setValue(colours::RKEY_OVERRIDE_LIGHTCOL, false);
}

TEST_F(EntityTest, OverrideEClassColour)
{
    auto light = createByClassName("light");
    auto torch = createByClassName("light_torchflame_small");
    auto lightCls = light->getEntity().getEntityClass();
    auto torchCls = torch->getEntity().getEntityClass();

    static const Vector3 GREEN(0, 1, 0);
    static const Vector3 YELLOW(1, 0, 1);

    // Light has an explicit green editor_color
    EXPECT_EQ(lightCls->getColour(), GREEN);

    // This class does not have an explicit editor_color value, but should
    // inherit the one from 'light'.
    EXPECT_EQ(torchCls->getColour(), GREEN);

    // Set an override for the 'light' class
    GlobalEclassColourManager().addOverrideColour("light", YELLOW);

    // Both 'light' and its subclasses should have the new colour
    EXPECT_EQ(lightCls->getColour(), YELLOW);
    EXPECT_EQ(torchCls->getColour(), YELLOW);
}

TEST_F(EntityTest, FuncStaticLocalToWorld)
{
    auto funcStatic = createByClassName("func_static");
    auto& spawnArgs = funcStatic->getEntity();
    spawnArgs.setKeyValue("origin", "0 0 0");

    // Initial localToWorld should be an identity matrix
    EXPECT_EQ(funcStatic->localToWorld(), Matrix4::getIdentity());

    // Set a new origin and make sure the localToWorld reflects the
    // corresponding translation
    const Vector3 MOVED(46, -128, 4096);
    spawnArgs.setKeyValue("origin", string::to_string(MOVED));
    EXPECT_EQ(funcStatic->localToWorld(),
              Matrix4::getTranslation(MOVED));

    // Clear transformation and get back to identity
    spawnArgs.setKeyValue("origin", "0 0 0");
    EXPECT_EQ(funcStatic->localToWorld(), Matrix4::getIdentity());
}

TEST_F(EntityTest, LightTransformedByParent)
{
    // Parent a light to another entity (this isn't currently how the attachment
    // system is implemented, but it should validate that a light node can
    // inherit the transformation of its parent).
    auto light = createByClassName("light");
    auto parentModel = createByClassName("func_static");
    parentModel->addChildNode(light);

    // Parenting should automatically set the parent pointer of the child
    EXPECT_EQ(light->getParent(), parentModel);

    // Set an offset for the parent model
    const Vector3 ORIGIN(1024, 512, -320);
    parentModel->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // Parent entity should have a transform matrix corresponding to its
    // translation
    EXPECT_EQ(parentModel->localToWorld(), Matrix4::getTranslation(ORIGIN));

    // The light itself should have the same transformation as the parent (since
    // the method is localToWorld not localToParent).
    EXPECT_EQ(light->localToWorld(), Matrix4::getTranslation(ORIGIN));

    // Render the light to obtain the RendererLight pointer
    RenderFixture renderF(true /* solid */);
    renderF.renderSubGraph(parentModel);
    EXPECT_EQ(renderF.nodesVisited, 2);
    EXPECT_EQ(renderF.collector.lights, 1);
    ASSERT_FALSE(renderF.collector.lightPtrs.empty());

    // Check the rendered light's geometry
    const RendererLight* rLight = renderF.collector.lightPtrs.front();
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN);
    EXPECT_EQ(rLight->lightAABB().extents, Vector3(320, 320, 320));
}

TEST_F(EntityTest, RenderUnselectedLightEntity)
{
    auto light = createByClassName("light");
    RenderFixture renderF;

    // Render the light in wireframe mode.
    light->setRenderSystem(renderF.backend);
    light->renderWireframe(renderF.collector, renderF.volumeTest);

    // Only the light origin diamond should be rendered
    EXPECT_EQ(renderF.collector.renderables, 1);
    EXPECT_EQ(renderF.collector.lights, 0);
}

TEST_F(EntityTest, RenderSelectedLightEntity)
{
    auto light = createByClassName("light");
    RenderFixture renderF;

    // Select the light then render it in wireframe mode
    Node_getSelectable(light)->setSelected(true);
    light->setRenderSystem(renderF.backend);
    light->renderWireframe(renderF.collector, renderF.volumeTest);

    // With the light selected, we should get the origin diamond, the radius and
    // the center vertex.
    EXPECT_EQ(renderF.collector.renderables, 3);
    EXPECT_EQ(renderF.collector.lights, 0);
}

TEST_F(EntityTest, RenderLightAsLightSource)
{
    auto light = createByClassName("light_torchflame_small");
    auto& spawnArgs = light->getEntity();

    // Set a non-default origin for the light
    static const Vector3 ORIGIN(-64, 128, 963);
    spawnArgs.setKeyValue("origin", string::to_string(ORIGIN));

    // Render the light in full materials mode
    RenderFixture renderF;
    light->setRenderSystem(renderF.backend);
    light->renderSolid(renderF.collector, renderF.volumeTest);

    // We should get one renderable for the origin diamond, and one light source
    EXPECT_EQ(renderF.collector.renderables, 1);
    EXPECT_EQ(renderF.collector.lights, 1);

    // Confirm properties of the submitted RendererLight
    ASSERT_EQ(renderF.collector.lightPtrs.size(), 1);
    const RendererLight* rLight = renderF.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN);

    // Default light properties from the entitydef
    EXPECT_EQ(rLight->lightAABB().extents, Vector3(240, 240, 240));
    ASSERT_TRUE(rLight->getShader() && rLight->getShader()->getMaterial());
    EXPECT_EQ(rLight->getShader()->getMaterial()->getName(),
              "lights/biground_torchflicker");
}

TEST_F(EntityTest, RenderEmptyFuncStatic)
{
    auto funcStatic = createByClassName("func_static");

    // Func static without a model key is empty
    RenderFixture rf;
    rf.renderSubGraph(funcStatic);
    EXPECT_EQ(rf.nodesVisited, 1);
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 0);
}

TEST_F(EntityTest, RenderFuncStaticWithModel)
{
    // Create a func_static with a model key
    auto funcStatic = createByClassName("func_static");
    funcStatic->getEntity().setKeyValue("model", "models/moss_patch.ase");

    RenderFixture rf;
    rf.renderSubGraph(funcStatic);

    // The entity node itself does not render the model; it is a parent node
    // with the model as a child (e.g. as a StaticModelNode). Therefore we
    // should have visited two nodes in total: the entity and its model child.
    EXPECT_EQ(rf.nodesVisited, 2);

    // Only one of the nodes should have submitted renderables
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 1);
}

TEST_F(EntityTest, RenderFuncStaticWithMultiSurfaceModel)
{
    // Create a func_static with a model key
    auto funcStatic = createByClassName("func_static");
    funcStatic->getEntity().setKeyValue("model", "models/torch.lwo");

    // This torch model has 3 renderable surfaces
    RenderFixture rf;
    rf.renderSubGraph(funcStatic);
    EXPECT_EQ(rf.collector.lights, 0);
    EXPECT_EQ(rf.collector.renderables, 3);
}

TEST_F(EntityTest, CreateAttachedLightEntity)
{
    // Create the torch entity which has an attached light
    auto torch = createByClassName("atdm:torch_brazier");
    ASSERT_TRUE(torch);

    // Check that the attachment spawnargs are present
    const Entity& spawnArgs = torch->getEntity();
    EXPECT_EQ(spawnArgs.getKeyValue("def_attach"), "light_cageflame_small");
    EXPECT_EQ(spawnArgs.getKeyValue("pos_attach"), "flame");
    EXPECT_EQ(spawnArgs.getKeyValue("name_attach"), "flame");

    // Spawnargs should be parsed into a single attachment
    auto attachments = getAttachments(torch);
    EXPECT_EQ(attachments.size(), 1);

    // Examine the properties of the single attachment
    Entity::Attachment attachment = attachments.front();
    EXPECT_EQ(attachment.eclass, "light_cageflame_small");
    EXPECT_EQ(attachment.offset, Vector3(0, 0, 10));
}

TEST_F(EntityTest, RenderAttachedLightEntity)
{
    auto torch = createByClassName("atdm:torch_brazier");
    ASSERT_TRUE(torch);

    // Confirm that def has the right model
    auto& spawnArgs = torch->getEntity();
    EXPECT_EQ(spawnArgs.getKeyValue("model"), "models/torch.lwo");

    // We must render in solid mode to get the light source
    RenderFixture rf(true /* solid mode */);
    rf.renderSubGraph(torch);

    // There should be 3 renderables from the torch (because the entity has a
    // shadowmesh and a collision mesh as well as the main model) and one from
    // the light (the origin diamond).
    EXPECT_EQ(rf.collector.renderables, 4);

    // The attached light should have been submitted as a light source
    EXPECT_EQ(rf.collector.lights, 1);

    // The submitted light should be fully realised with a light shader
    const RendererLight* rLight = rf.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);
    EXPECT_TRUE(rLight->getShader());
}

TEST_F(EntityTest, AttachedLightAtCorrectPosition)
{
    const Vector3 ORIGIN(256, -128, 635);
    const Vector3 EXPECTED_OFFSET(0, 0, 10); // attach offset in def

    // Create a torch node and set a non-zero origin
    auto torch = createByClassName("atdm:torch_brazier");
    torch->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // Render the torch
    RenderFixture rf(true /* solid mode */);
    rf.renderSubGraph(torch);

    // Access the submitted light source
    ASSERT_FALSE(rf.collector.lightPtrs.empty());
    const RendererLight* rLight = rf.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);

    // Check the light source's position
    EXPECT_EQ(rLight->getLightOrigin(), ORIGIN + EXPECTED_OFFSET);
    EXPECT_EQ(rLight->lightAABB().origin, ORIGIN + EXPECTED_OFFSET);
}

TEST_F(EntityTest, AttachedLightMovesWithEntity)
{
    const Vector3 ORIGIN(12, -0.5, 512);
    const Vector3 EXPECTED_OFFSET(0, 0, 10); // attach offset in def

    // Create a torch node and set a non-zero origin
    auto torch = createByClassName("atdm:torch_brazier");
    torch->getEntity().setKeyValue("origin", string::to_string(ORIGIN));

    // First render
    {
        RenderFixture rf(true /* solid mode */);
        rf.renderSubGraph(torch);
    }

    // Move the torch
    const Vector3 NEW_ORIGIN = ORIGIN + Vector3(128, 512, -54);
    torch->getEntity().setKeyValue("origin", string::to_string(NEW_ORIGIN));

    // Render again to get positions
    RenderFixture rf(true /* solid mode */);
    rf.renderSubGraph(torch);

    // Access the submitted light source
    ASSERT_FALSE(rf.collector.lightPtrs.empty());
    const RendererLight* rLight = rf.collector.lightPtrs.front();
    ASSERT_TRUE(rLight);

    // Check the light source's position
    EXPECT_EQ(rLight->getLightOrigin(), NEW_ORIGIN + EXPECTED_OFFSET);
    EXPECT_EQ(rLight->lightAABB().origin, NEW_ORIGIN + EXPECTED_OFFSET);
}

TEST_F(EntityTest, CreateAIEntity)
{
    auto guard = createByClassName("atdm:ai_builder_guard");
    ASSERT_TRUE(guard);

    // Guard should have a hammer attachment
    auto attachments = getAttachments(guard);
    EXPECT_EQ(attachments.size(), 1);
    EXPECT_EQ(attachments.front().eclass, "atdm:moveable_warhammer");
    EXPECT_EQ(attachments.front().offset, Vector3(14, -6, -6));
    EXPECT_EQ(attachments.front().joint, "Spine2");
}

namespace
{

class TestEntityObserver final :
    public Entity::Observer
{
public:
    bool insertFired;
    bool changeFired;
    bool eraseFired;

    std::vector<std::pair<std::string, std::string>> insertStack;
    std::vector<std::pair<std::string, std::string>> changeStack;
    std::vector<std::pair<std::string, std::string>> eraseStack;

    TestEntityObserver()
    {
        reset();
    }

    void reset()
    {
        insertFired = false;
        insertStack.clear();
        changeFired = false;
        changeStack.clear();
        eraseFired = false;
        eraseStack.clear();
    }

    void onKeyInsert(const std::string& key, EntityKeyValue& value) override
    {
        insertFired = true;
        insertStack.emplace_back(key, value.get());
    }

    void onKeyChange(const std::string& key, const std::string& value) override
    {
        changeFired = true;
        changeStack.emplace_back(key, value);
    }

    void onKeyErase(const std::string& key, EntityKeyValue& value) override
    {
        eraseFired = true;
        eraseStack.emplace_back(key, value.get());
    }
};

inline bool stackHasKeyValuePair(const std::vector<std::pair<std::string, std::string>>& stack, 
    const std::string& key, const std::string& value)
{
    auto it = std::find(stack.begin(), stack.end(), std::make_pair(key, value));
    return it != stack.end();
}

inline bool stackHasKey(const std::vector<std::pair<std::string, std::string>>& stack,
    const std::string& key)
{
    for (const auto& pair : stack)
    {
        if (pair.first == key) return true;
    }

    return false;
}

class TestKeyObserver :
    public KeyObserver
{
public:
    bool hasBeenInvoked;
    std::string receivedValue;

    TestKeyObserver()
    {
        reset();
    }

    void reset()
    {
        hasBeenInvoked = false;
        receivedValue.clear();
    }

    void onKeyValueChanged(const std::string& newValue) override
    {
        hasBeenInvoked = true;
        receivedValue = newValue;
    }
};

inline EntityKeyValue* findKeyValue(Entity* entity, const std::string& keyToFind)
{
    EntityKeyValue* keyValue = nullptr;

    entity->forEachEntityKeyValue([&](const std::string& key, EntityKeyValue& value)
    {
        if (!keyValue && key == keyToFind)
        {
            keyValue = &value;
        }
    });

    return keyValue;
}

inline void expectKeyValuesAreEquivalent(const std::vector<std::pair<std::string, std::string>>& stack1, 
    const std::vector<std::pair<std::string, std::string>>& stack2)
{
    EXPECT_EQ(stack1.size(), stack2.size()) << "Stack1 differs from Stack 2 in size";

    for (const auto& pair : stack1)
    {
        EXPECT_TRUE(stackHasKeyValuePair(stack2, pair.first, pair.second)) <<
            "Stack 2 was missing the key value pair " << pair.first << " = " << pair.second;
    }
}

}

TEST_F(EntityTest, EntityObserverAttachDetach)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    TestEntityObserver observer;

    // Collect all existing key values of this entity
    auto existingKeyValues = algorithm::getAllKeyValuePairs(guard);
    EXPECT_FALSE(existingKeyValues.empty()) << "Entity doesn't have any keys";

    // On attachment, the observer gets notified about all existing keys (insert)
    guard->attachObserver(&observer);

    EXPECT_EQ(observer.insertStack.size(), existingKeyValues.size()) << "Observer didn't get notified about all keys";

    for (const auto& pair : existingKeyValues)
    {
        EXPECT_TRUE(stackHasKeyValuePair(observer.insertStack, pair.first, pair.second)) <<
            "Insert stack doesn't have the expected kv " << pair.first << " = " << pair.second;
    }

    // Everything else should be silent
    EXPECT_TRUE(observer.changeStack.empty()) << "Change stack should be clean";
    EXPECT_TRUE(observer.eraseStack.empty()) << "Erase stack should be clean";

    observer.reset();

    // On detaching the observer receives an erase call for each key value pair
    guard->detachObserver(&observer);

    EXPECT_EQ(observer.eraseStack.size(), existingKeyValues.size()) << "Observer didn't get notified about all keys";

    for (const auto& pair : existingKeyValues)
    {
        EXPECT_TRUE(stackHasKeyValuePair(observer.eraseStack, pair.first, pair.second)) <<
            "Erase stack doesn't have the expected kv " << pair.first << " = " << pair.second;
    }

    // Everything else should be silent
    EXPECT_TRUE(observer.insertStack.empty()) << "Insert stack should be clean";
    EXPECT_TRUE(observer.changeStack.empty()) << "Change stack should be clean";
}

TEST_F(EntityTest, EntityObserverKeyAddition)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    TestEntityObserver observer;

    // Attach and reset the observer
    guard->attachObserver(&observer);
    observer.reset();

    constexpr const char* key = "New_Unique_Key";
    constexpr const char* value = "New_Unique_Value";
    guard->setKeyValue(key, value);

    // Assert on the new key
    EXPECT_EQ(observer.insertStack.size(), 1) << "Observer didn't get notified about the new key";
    EXPECT_TRUE(stackHasKeyValuePair(observer.insertStack, key, value)) <<
            "Insert stack doesn't have the expected kv " << key << " = " << value;

    // Everything else should be silent
    EXPECT_TRUE(observer.changeStack.empty()) << "Change stack should be clean";
    EXPECT_TRUE(observer.eraseStack.empty()) << "Erase stack should be clean";

    observer.reset();

    // On detaching the observer should receive a corresponding erase for the new key
    guard->detachObserver(&observer);

    EXPECT_TRUE(stackHasKeyValuePair(observer.eraseStack, key, value)) <<
        "Insert stack doesn't have the expected kv " << key << " = " << value;
}

TEST_F(EntityTest, EntityObserverKeyRemoval)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    TestEntityObserver observer;

    constexpr const char* key = "New_Unique_Key";
    constexpr const char* value = "New_Unique_Value";
    guard->setKeyValue(key, value);

    // Attach and reset the observer
    guard->attachObserver(&observer);
    observer.reset();

    // Remove the key
    guard->setKeyValue(key, "");

    // Assert on the event that should have been received
    EXPECT_EQ(observer.eraseStack.size(), 1) << "Observer didn't get notified about the removed key";
    EXPECT_TRUE(stackHasKeyValuePair(observer.eraseStack, key, value)) <<
        "Erase stack doesn't have the expected kv " << key << " = " << value;

    // Everything else should be silent
    EXPECT_TRUE(observer.changeStack.empty()) << "Change stack should be clean";
    EXPECT_TRUE(observer.insertStack.empty()) << "Insert stack should be clean";

    observer.reset();

    // On detaching the observer should not receive a corresponding erase for the already removed key
    guard->detachObserver(&observer);

    EXPECT_FALSE(stackHasKeyValuePair(observer.eraseStack, key, value)) <<
        "Erase stack unexpectedly contained the kv " << key << " = " << value;
}

TEST_F(EntityTest, EntityObserverKeyChange)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    TestEntityObserver observer;

    // Attach and reset the observer
    guard->attachObserver(&observer);
    observer.reset();

    constexpr const char* NameKey = "name";
    constexpr const char* NewName = "Ignazius";

    EXPECT_FALSE(guard->getKeyValue(NameKey).empty()) << "Key " << NameKey << " must exist for this test";

    guard->setKeyValue(NameKey, NewName);

    // Assert on the event that should have been received
    EXPECT_EQ(observer.changeStack.size(), 1) << "Observer didn't get notified about the changed key";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NameKey, NewName)) <<
        "Erase stack doesn't have the expected kv " << NameKey << " = " << NewName;

    // Everything else should be silent
    EXPECT_TRUE(observer.insertStack.empty()) << "Insert stack should be clean";
    EXPECT_TRUE(observer.eraseStack.empty()) << "Erase stack should be clean";

    observer.reset();

    constexpr const char* EvenNewerName = "Bonifazius";
    guard->setKeyValue(NameKey, EvenNewerName);

    // Assert on the event that should have been received
    EXPECT_EQ(observer.changeStack.size(), 1) << "Observer didn't get notified about the changed key";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NameKey, EvenNewerName)) <<
        "Erase stack doesn't have the expected kv " << NameKey << " = " << EvenNewerName;

    // On detaching the observer should not receive a corresponding erase for the newer value
    guard->detachObserver(&observer);

    EXPECT_TRUE(stackHasKeyValuePair(observer.eraseStack, NameKey, EvenNewerName)) <<
        "Erase stack unexpectedly contained the kv " << NameKey << " = " << EvenNewerName;
}

TEST_F(EntityTest, EntityObserverUndoRedo)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKey = "New_Unique_Key";
    constexpr const char* NewValue = "New_Unique_Value";
    guard->setKeyValue(NewKey, NewValue);

    constexpr const char* NewKey2 = "New_Unique_Key2";
    constexpr const char* NewValue2 = "New_Unique_Value2";
    constexpr const char* NameKey = "name";
    constexpr const char* NewNameValue = "Ignazius";
    auto originalName = guard->getKeyValue(NameKey);

    TestEntityObserver observer;

    // Collect all existing key values of this entity
    auto keyValuesBeforeChange = algorithm::getAllKeyValuePairs(guard);
    EXPECT_FALSE(keyValuesBeforeChange.empty()) << "Entity doesn't have any keys";

    // On attachment, the observer gets notified about all existing keys (insert)
    guard->attachObserver(&observer);

    // Perform an undoable operation. The order of additions/changes/removals
    // does actually matter, since adding/removing will push the whole spawnarg set to the undo stack
    // whereas changing a single key will only push that single value to the stack
    {
        UndoableCommand cmd("testcommand");

        // Add another key
        guard->setKeyValue(NewKey2, NewValue2);

        // Change an existing key value
        guard->setKeyValue(NameKey, NewNameValue);

        // Remove a previously existing key
        guard->setKeyValue(NewKey, "");
    }

    auto keyValuesAfterChange = algorithm::getAllKeyValuePairs(guard);

    observer.reset();

    // UNDO
    GlobalUndoSystem().undo();

    // Check that the entity has now the same state as before the change
    expectKeyValuesAreEquivalent(algorithm::getAllKeyValuePairs(guard), keyValuesBeforeChange);

    // The Undo operation spams the observer with an erase() for each existing pair, 
    // and a subsequent insert() for each one imported from the undo stack
    // Note that the value attached to the erase() event might depend on the order the SpawnArgs have been
    // manipulated during the Undoable operation - if a key value got changed before a new one
    // was added to the SpawnArg set, the value passed to erase() might differ from the case where 
    // these two operations were happening the other way around.
    EXPECT_EQ(observer.eraseStack.size(), keyValuesAfterChange.size()) << "All keys before undo should have been reported";

    for (const auto& pair : keyValuesAfterChange)
    {
        // Only check the key of the erase calls, not the value
        EXPECT_TRUE(stackHasKey(observer.eraseStack, pair.first)) <<
            "Erase stack doesn't have the expected key " << pair.first;
    }
    
    EXPECT_EQ(observer.insertStack.size(), keyValuesBeforeChange.size()) << "Not all keys got reported as re-inserted";

    for (const auto& pair : keyValuesBeforeChange)
    {
        EXPECT_TRUE(stackHasKeyValuePair(observer.insertStack, pair.first, pair.second)) <<
            "Erase stack doesn't have the expected kv " << pair.first << " = " << pair.second;
    }

    // The single key value change triggered one key value change notification
    EXPECT_EQ(observer.changeStack.size(), 1) << "Change stack should just contain the single keyvalue change";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NameKey, originalName))
        << "Change stack should just contain the single keyvalue change";

    // REDO
    observer.reset();
    GlobalUndoSystem().redo();

    // Check that the entity has now the same state as before the undo
    expectKeyValuesAreEquivalent(algorithm::getAllKeyValuePairs(guard), keyValuesAfterChange);

    // The Redo operation should behave analogous to the undo, report all key values before the change as erased
    EXPECT_EQ(observer.eraseStack.size(), keyValuesBeforeChange.size()) << "All keys before redo should have been reported";

    for (const auto& pair : keyValuesBeforeChange)
    {
        // Only check the key of the erase calls, not the value
        EXPECT_TRUE(stackHasKey(observer.eraseStack, pair.first)) <<
            "Erase stack doesn't have the expected key " << pair.first;
    }

    EXPECT_EQ(observer.insertStack.size(), keyValuesAfterChange.size()) << "Not all keys got reported as re-inserted";

    // This can be considered a bug: on redo, not even the insert() call receives the correct
    // name key value "Ignazius", instead it receives the name before the change "atdm:ai_builder_guard_1"
    // So we can only assert on the key at this point
    for (const auto& pair : keyValuesAfterChange)
    {
        EXPECT_TRUE(stackHasKey(observer.insertStack, pair.first)) <<
            "Erase stack doesn't have the expected kv " << pair.first << " = " << pair.second;
    }

    // The single key value change triggered one key value change notification
    EXPECT_EQ(observer.changeStack.size(), 1) << "Change stack should just contain the single keyvalue change";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NameKey, NewNameValue))
        << "Change stack should just contain the single keyvalue change";

    guard->detachObserver(&observer);
}

TEST_F(EntityTest, EntityObserverUndoSingleKeyValue)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKey = "New_Unique_Key";
    constexpr const char* NewValue = "New_Unique_Value";
    guard->setKeyValue(NewKey, NewValue);

    TestEntityObserver observer;
    // On attachment, the observer gets notified about all existing keys (insert)
    guard->attachObserver(&observer);

    // Perform an undoable operation. In this scenario, we're only editing
    // a single key, this means the entity is not saving the entire set to the stack
    constexpr const char* SomeOtherValue = "SomeOtherValue";
    {
        UndoableCommand cmd("testcommand");
        guard->setKeyValue(NewKey, SomeOtherValue);
    }

    // UNDO
    observer.reset();
    GlobalUndoSystem().undo();

    EXPECT_EQ(guard->getKeyValue(NewKey), NewValue) << "Key value not reverted properly";

    EXPECT_EQ(observer.changeStack.size(), 1) << "Reverted key didn't get reported";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NewKey, NewValue)) <<
        "Change stack doesn't have the expected kv " << NewKey << " = " << NewValue;
    
    // Everything else should be silent
    EXPECT_TRUE(observer.eraseStack.empty()) << "Erase stack should be clean";
    EXPECT_TRUE(observer.insertStack.empty()) << "Insert stack should be clean";

    // REDO
    observer.reset();
    GlobalUndoSystem().redo();

    EXPECT_EQ(guard->getKeyValue(NewKey), SomeOtherValue) << "Key value not re-done properly";

    EXPECT_EQ(observer.changeStack.size(), 1) << "Reverted key didn't get reported";
    EXPECT_TRUE(stackHasKeyValuePair(observer.changeStack, NewKey, SomeOtherValue)) <<
        "Change stack doesn't have the expected kv " << NewKey << " = " << SomeOtherValue;

    // Everything else should be silent
    EXPECT_TRUE(observer.eraseStack.empty()) << "Erase stack should be clean";
    EXPECT_TRUE(observer.insertStack.empty()) << "Insert stack should be clean";

    guard->detachObserver(&observer);
}

TEST_F(EntityTest, KeyObserverAttachDetach)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKeyName = "New_Unique_Key";
    constexpr const char* NewKeyValue = "New_Unique_Value";
    guard->setKeyValue(NewKeyName, NewKeyValue);

    TestKeyObserver observer;

    EntityKeyValue* keyValue = findKeyValue(guard, NewKeyName);
    EXPECT_TRUE(keyValue != nullptr) << "Could not locate the key value";

    // On attachment, the observer gets notified about the existing value
    keyValue->attach(observer);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on attach";
    EXPECT_EQ(observer.receivedValue, NewKeyValue) << "Observer didn't get the correct value";

    observer.reset();
    observer.receivedValue = "dummyvalue_that_should_be_overwritten";

    // On detaching the observer receives another call with an empty value
    keyValue->detach(observer);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on attach";
    EXPECT_EQ(observer.receivedValue, "") << "Observer didn't get the expected empty value";
}

TEST_F(EntityTest, KeyObserverValueChange)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKeyName = "New_Unique_Key";
    constexpr const char* NewKeyValue = "New_Unique_Value";
    guard->setKeyValue(NewKeyName, NewKeyValue);

    TestKeyObserver observer;
    
    EntityKeyValue* keyValue = findKeyValue(guard, NewKeyName);
    EXPECT_TRUE(keyValue != nullptr) << "Could not locate the key value";

    keyValue->attach(observer);
    observer.reset();
    
    constexpr const char* SomeOtherValue = "SomeOtherValue";
    guard->setKeyValue(NewKeyName, SomeOtherValue);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on change";
    EXPECT_EQ(observer.receivedValue, SomeOtherValue) << "Observer didn't get the correct value";

    // One more round, this time we use the assign() method
    observer.reset();
    constexpr const char* DistinguishableValue = "DistinguishableValue";
    keyValue->assign(DistinguishableValue);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on assign";
    EXPECT_EQ(observer.receivedValue, DistinguishableValue) << "Observer didn't get the correct value";
    observer.reset();

    keyValue->detach(observer);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on attach";
    EXPECT_EQ(observer.receivedValue, "") << "Observer didn't get the expected empty value";
}

// Check that an KeyObserver stays attached to the key value after Undo
TEST_F(EntityTest, KeyObserverAttachedAfterUndo)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKeyName = "New_Unique_Key";
    constexpr const char* NewKeyValue = "New_Unique_Value";
    guard->setKeyValue(NewKeyName, NewKeyValue);

    TestKeyObserver observer;
    EntityKeyValue* keyValue = findKeyValue(guard, NewKeyName);
    EXPECT_TRUE(keyValue != nullptr) << "Could not locate the key value";

    // Monitor this new key
    keyValue->attach(observer);
    observer.reset();

    // Open an undoable transaction and change that keyvalue
    constexpr const char* SomeOtherValue = "SomeOtherValue";
    {
        UndoableCommand cmd("changeKeyValue");
        guard->setKeyValue(NewKeyName, SomeOtherValue);
    }

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on change";
    EXPECT_EQ(observer.receivedValue, SomeOtherValue) << "Observer didn't get the correct value";

    // Hit Undo to revert the changed value
    GlobalUndoSystem().undo();
    EXPECT_EQ(guard->getKeyValue(NewKeyName), NewKeyValue) << "Key is still changed after undo";

    // Reset the observer and check whether it still receives messages
    observer.reset();
    guard->setKeyValue(NewKeyName, SomeOtherValue);

    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on assign";
    EXPECT_EQ(observer.receivedValue, SomeOtherValue) << "Observer didn't get the correct value";

    keyValue->detach(observer);
}

// Checks that the value changes by undo/redo commands are sent out to the KeyObservers
TEST_F(EntityTest, KeyObserverUndoRedoValueChange)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKeyName = "New_Unique_Key";
    constexpr const char* NewKeyValue = "New_Unique_Value";
    guard->setKeyValue(NewKeyName, NewKeyValue);

    TestKeyObserver observer;
    EntityKeyValue* keyValue = findKeyValue(guard, NewKeyName);
    EXPECT_TRUE(keyValue != nullptr) << "Could not locate the key value";

    // Monitor this new key
    keyValue->attach(observer);
    observer.reset();

    // Open an undoable transaction and change that keyvalue
    constexpr const char* SomeOtherValue = "SomeOtherValue";
    {
        UndoableCommand cmd("changeKeyValue");
        guard->setKeyValue(NewKeyName, SomeOtherValue);
    }

    // Undo
    observer.reset();
    GlobalUndoSystem().undo();
    EXPECT_EQ(guard->getKeyValue(NewKeyName), NewKeyValue) << "Key value wasn't properly reverted";
    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on undo";
    EXPECT_EQ(observer.receivedValue, NewKeyValue) << "Observer didn't get the value before change";

    // Redo
    observer.reset();
    GlobalUndoSystem().redo();

    EXPECT_EQ(guard->getKeyValue(NewKeyName), SomeOtherValue) << "Key value wasn't properly redone";
    EXPECT_TRUE(observer.hasBeenInvoked) << "Observer didn't get notified on redo";
    EXPECT_EQ(observer.receivedValue, SomeOtherValue) << "Observer didn't get the value after change";

    keyValue->detach(observer);
}

// KeyObserver doesn't get called when a key is removed entirely from the SpawnArgs
TEST_F(EntityTest, KeyObserverKeyRemoval)
{
    auto guardNode = createByClassName("atdm:ai_builder_guard");
    scene::addNodeToContainer(guardNode, GlobalMapModule().getRoot());
    auto guard = Node_getEntity(guardNode);

    constexpr const char* NewKeyName = "New_Unique_Key";
    constexpr const char* NewKeyValue = "New_Unique_Value";
    guard->setKeyValue(NewKeyName, NewKeyValue);

    UndoableCommand cmd("removeKey"); // prevent the key value from going out of scope
    TestKeyObserver observer;

    EntityKeyValue* keyValue = findKeyValue(guard, NewKeyName);
    EXPECT_TRUE(keyValue != nullptr) << "Could not locate the key value";

    keyValue->attach(observer);
    observer.reset();

    // Remove the key
    guard->setKeyValue(NewKeyName, "");

    // The observer shouldn't have been notified
    EXPECT_FALSE(observer.hasBeenInvoked) << "Observer has been notified on key remove";

    keyValue->detach(observer);
}

inline Entity* findPlayerStartEntity()
{
    Entity* found = nullptr;

    algorithm::findFirstEntity(GlobalMapModule().getRoot(), [&](IEntityNode& entityNode)
    {
        if (entityNode.getEntity().getEntityClass()->getName() == "info_player_start")
        {
            found = &entityNode.getEntity();
        }

        return found == nullptr;
    });

    return found;
}

TEST_F(EntityTest, AddPlayerStart)
{
    // Empty map, check prerequisites
    EXPECT_EQ(findPlayerStartEntity(), nullptr) << "Empty map shouldn't have a player start";

    Vector3 position(50, 30, 40);
    GlobalCommandSystem().executeCommand("PlacePlayerStart", cmd::Argument(position));

    auto playerStart = findPlayerStartEntity();
    EXPECT_TRUE(playerStart) << "Couldn't find the player start entity after placing it";

    EXPECT_EQ(playerStart->getKeyValue("origin"), string::to_string(position)) << "Origin has the wrong value";

    // Ensure this action is undoable
    GlobalUndoSystem().undo();
    EXPECT_EQ(findPlayerStartEntity(), nullptr) << "Couldn't undo the place player start action";
}

TEST_F(EntityTest, MovePlayerStart)
{
    // Empty map, check prerequisites
    auto originalPosition = "50 30 47";
    auto playerStart = GlobalEntityModule().createEntity(GlobalEntityClassManager().findOrInsert("info_player_start", false));
    scene::addNodeToContainer(playerStart, GlobalMapModule().getRoot());
    Node_getEntity(playerStart)->setKeyValue("origin", originalPosition);

    Vector3 position(7, 2, -4);
    GlobalCommandSystem().executeCommand("PlacePlayerStart", cmd::Argument(position));
    EXPECT_EQ(Node_getEntity(playerStart)->getKeyValue("origin"), string::to_string(position)) << "Origin didn't get updated";

    // Ensure this action is undoable
    GlobalUndoSystem().undo();
    EXPECT_EQ(Node_getEntity(playerStart)->getKeyValue("origin"), originalPosition) << "Origin change didn't get undone";
}

TEST_F(EntityTest, GetDefaultAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");
    EXPECT_EQ(eclass->getAttributeType("ordinary_key"), "text");
}

TEST_F(EntityTest, GetNonInheritedAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");
    
    // The "defined_bool" is defined on the eclass, next to its editor_bool descriptor
    EXPECT_EQ(eclass->getAttributeType("defined_bool"), "bool");

    // The "undefined_bool" is not directly set on the eclass
    EXPECT_EQ(eclass->getAttributeType("undefined_bool"), "bool");
}

TEST_F(EntityTest, GetInheritedAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The "base_defined_bool" is described in the base, as is the key
    EXPECT_EQ(eclass->getAttributeType("base_defined_bool"), "bool");

    // The "bool_not_defined_in_base" is set on the subclass, the description is in base
    EXPECT_EQ(eclass->getAttributeType("bool_not_defined_in_base"), "bool");

    // The "bool_not_defined_anywhere" is not set anywhere, only the description is there
    EXPECT_EQ(eclass->getAttributeType("bool_not_defined_anywhere"), "bool");
}

TEST_F(EntityTest, GetVariousAttributeTypes)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // editor_var and editor_string will be converted to "text"
    EXPECT_EQ(eclass->getAttributeType("a_var"), "text");
    EXPECT_EQ(eclass->getAttributeType("a_string"), "text");

    // Some definitions: the suffix after "editor_" should be accepted as type
    EXPECT_EQ(eclass->getAttributeType("a_text"), "text");
    EXPECT_EQ(eclass->getAttributeType("a_vector"), "vector");
    EXPECT_EQ(eclass->getAttributeType("a_hurk"), "hurk");
}

}