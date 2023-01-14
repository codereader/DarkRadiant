#include "RadiantTest.h"

#include "ieclass.h"
#include "ieclasscolours.h"
#include "irendersystemfactory.h"

#include "eclass.h"
#include "string/join.h"

#include "algorithm/Entity.h"
#include "algorithm/FileUtils.h"
#include "algorithm/Scene.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using EntityClassTest = RadiantTest;

TEST_F(EntityClassTest, LookupEntityClass)
{
    // Nonexistent class should return null (but not throw or crash)
    auto cls = GlobalEntityClassManager().findClass("notAnEntityClass");
    EXPECT_FALSE(cls);

    // Real entity class should return a valid pointer
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls);

    EXPECT_EQ(GlobalEntityClassManager().findClass("light"),
        GlobalEntityClassManager().findClass("LiGHT")) << "Lookup should be case-insensitive";
}

TEST_F(EntityClassTest, ForeachEntityClass)
{
    std::set<std::string> visitedNames;

    // Nonexistent class should return null (but not throw or crash)
    GlobalEntityClassManager().forEachEntityClass([&] (const IEntityClassPtr& eclass)
    {
        visitedNames.insert(eclass->getDeclName());
    });

    EXPECT_TRUE(visitedNames.count("light") > 0);
    EXPECT_TRUE(visitedNames.count("light_extinguishable") > 0);
    EXPECT_TRUE(visitedNames.count("dr:entity_using_modeldef") > 0);
    EXPECT_TRUE(visitedNames.count("atdm:light_base") > 0);
}

TEST_F(EntityClassTest, EntityClassDefFilename)
{
    auto cls = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");
    EXPECT_TRUE(cls);

    EXPECT_EQ(cls->getModName(), RadiantTest::DEFAULT_GAME_TYPE);
    EXPECT_EQ(cls->getDeclFilePath(), "def/entity_with_model.def");
}

TEST_F(EntityClassTest, LightEntityRecognition)
{
    // The 'light' class should be recognised as an actual light
    auto lightCls = GlobalEntityClassManager().findClass("light");
    EXPECT_TRUE(lightCls->isLight());
    EXPECT_EQ(lightCls->getClassType(), IEntityClass::Type::Light);

    // Things which are not lights should also be correctly identified
    auto notLightCls = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");
    EXPECT_TRUE(notLightCls);
    EXPECT_FALSE(notLightCls->isLight());
    EXPECT_NE(notLightCls->getClassType(), IEntityClass::Type::Light);

    // Anything deriving from the light class should also be a light
    auto derived1 = GlobalEntityClassManager().findClass("atdm:light_base");
    EXPECT_TRUE(derived1->isLight());
    EXPECT_EQ(derived1->getClassType(), IEntityClass::Type::Light);

    // Second level derivations too
    auto derived2 = GlobalEntityClassManager().findClass("light_extinguishable");
    EXPECT_TRUE(derived2->isLight());
    EXPECT_EQ(derived2->getClassType(), IEntityClass::Type::Light);

    // torch_brazier is not a light itself, but has a light attached, so it
    // should not have isLight() == true
    auto brazier = GlobalEntityClassManager().findClass("atdm:torch_brazier");
    EXPECT_FALSE(brazier->isLight());
    EXPECT_NE(brazier->getClassType(), IEntityClass::Type::Light);
}

TEST_F(EntityClassTest, SpeakerEntityRecognition)
{
    // The 'speaker' class should be recognised as speaker
    auto speakerClass = GlobalEntityClassManager().findClass("speaker");
    EXPECT_TRUE(speakerClass) << "Couldn't find the speaker eclass";
    EXPECT_EQ(speakerClass->getClassType(), IEntityClass::Type::Speaker);
}

TEST_F(EntityClassTest, EclassModelEntityRecognition)
{
    // Any eclass with a "model" spawnarg qualifies, as long as it's "fixed size"

    auto modelClass = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");

    EXPECT_TRUE(modelClass) << "Couldn't find the dr:entity_using_modeldef eclass";
    EXPECT_EQ(modelClass->getClassType(), IEntityClass::Type::EntityClassModel);
    EXPECT_TRUE(modelClass->isFixedSize());

    // "model" key set explicitly
    auto bucket = GlobalEntityClassManager().findClass("bucket_metal");

    // Deriving the "model" key also makes this an eclass model
    auto bucket2 = GlobalEntityClassManager().findClass("bucket_metal2");

    EXPECT_NE(bucket->getAttributeValue("model"), "") << "bucket_metal should have a model";
    EXPECT_NE(bucket2->getAttributeValue("model"), "") << "bucket_metal2 should have an inherited model key";
    EXPECT_EQ(bucket2->getAttributeValue("model", false), "") << "bucket_metal2 should not have a model key set explicitly";

    EXPECT_EQ(bucket->getClassType(), IEntityClass::Type::EntityClassModel);
    EXPECT_TRUE(bucket->isFixedSize());

    EXPECT_EQ(bucket2->getClassType(), IEntityClass::Type::EntityClassModel);
    EXPECT_TRUE(bucket2->isFixedSize());
}

TEST_F(EntityClassTest, GenericEntityRecognition)
{
    // Any fixed-size eclass without "model" spawnarg, and neither a speaker or light

    auto playerStart = GlobalEntityClassManager().findClass("info_player_start");
    EXPECT_TRUE(playerStart) << "Couldn't find the info_player_start eclass";
    EXPECT_EQ(playerStart->getClassType(), IEntityClass::Type::Generic);
    EXPECT_TRUE(playerStart->isFixedSize());
    EXPECT_EQ(playerStart->getAttributeValue("editor_mins", false), "-16 -16 0") << "editor_mins should be set to a vector";
    EXPECT_EQ(playerStart->getAttributeValue("editor_maxs", false), "16 16 64") << "editor_maxs should be set to a vector";

    // Deriving the editor_mins/maxs should work too
    auto derived = GlobalEntityClassManager().findClass("derived_from_info_player_start");
    EXPECT_TRUE(derived) << "Couldn't find the derived_from_info_player_start eclass";
    EXPECT_EQ(derived->getClassType(), IEntityClass::Type::Generic);
    EXPECT_TRUE(derived->isFixedSize());
    EXPECT_EQ(derived->getAttributeValue("editor_mins", false), "") << "editor_mins should not be set explicitly";
    EXPECT_EQ(derived->getAttributeValue("editor_maxs", false), "") << "editor_maxs should not be set explicitly";
}

TEST_F(EntityClassTest, StaticGeometryEntityRecognition)
{
    // Any variable-sized eclass (no editor_mins/maxs or them set to "?")
    auto variableSized = GlobalEntityClassManager().findClass("variable_size_entity");
    EXPECT_TRUE(variableSized) << "Couldn't find the variable_size_entity eclass";
    EXPECT_EQ(variableSized->getAttributeValue("editor_mins", false), "?") << "Expected editor_mins to be set explicitly";
    EXPECT_EQ(variableSized->getAttributeValue("editor_maxs", false), "?") << "Expected editor_maxs to be set explicitly";
    EXPECT_EQ(variableSized->getClassType(), IEntityClass::Type::StaticGeometry);
    EXPECT_FALSE(variableSized->isFixedSize());

    // Deriving the "?" editor_mins/maxs should work too
    auto derived = GlobalEntityClassManager().findClass("derived_from_variable_size_entity");
    EXPECT_TRUE(derived) << "Couldn't find the derived_from_variable_size_entity eclass";
    EXPECT_EQ(derived->getAttributeValue("editor_mins", false), "") << "Didn't expect editor_mins to be set explicitly";
    EXPECT_EQ(derived->getAttributeValue("editor_maxs", false), "") << "Didn't expect editor_maxs to be set explicitly";
    EXPECT_EQ(derived->getClassType(), IEntityClass::Type::StaticGeometry);
    EXPECT_FALSE(derived->isFixedSize());

    auto func_static = GlobalEntityClassManager().findClass("func_static");
    EXPECT_TRUE(func_static) << "Couldn't find the func_static eclass";
    EXPECT_EQ(func_static->getClassType(), IEntityClass::Type::StaticGeometry);
    EXPECT_FALSE(func_static->isFixedSize());
}

TEST_F(EntityClassTest, UnknownEntityClassIsStaticGeometry)
{
    // When inserting an unknown entity, we expect to receive a variable-sized entity class
    auto nonExistent = GlobalEntityClassManager().findOrInsert("some_non_existent_entity_class", true);
    EXPECT_TRUE(nonExistent) << "Couldn't create the some_non_existent_entity_class eclass";
    EXPECT_EQ(nonExistent->getClassType(), IEntityClass::Type::StaticGeometry);
    EXPECT_FALSE(nonExistent->isFixedSize());
}

TEST_F(EntityClassTest, EntityClassInheritsAttributes)
{
    auto cls = GlobalEntityClassManager().findClass("light_extinguishable");
    ASSERT_TRUE(cls);

    // Inherited from 'light'
    EXPECT_EQ(cls->getAttributeValue("editor_color"), "0 1 0");
    EXPECT_EQ(cls->getAttributeValue("spawnclass"), "idLight");

    // Inherited from 'atdm:light_base'
    EXPECT_EQ(cls->getAttributeValue("AIUse"), "AIUSE_LIGHTSOURCE");
    EXPECT_EQ(cls->getAttributeValue("shouldBeOn"), "0");

    // Inherited but overridden on 'light_extinguishable' itself
    EXPECT_EQ(cls->getAttributeValue("editor_displayFolder"),
        "Lights/Base Entities, DoNotUse");

    // Lookup without considering inheritance
    EXPECT_EQ(cls->getAttributeValue("editor_color", false), "");
    EXPECT_EQ(cls->getAttributeValue("spawnclass", false), "");
}

TEST_F(EntityClassTest, VisitInheritedClassAttributes)
{
    auto cls = GlobalEntityClassManager().findClass("light_extinguishable");
    ASSERT_TRUE(cls);

    // Map of attribute names to inherited flag
    std::map<std::string, bool> attributes;

    // Visit all attributes and store in the map
    cls->forEachAttribute(
        [&attributes](const EntityClassAttribute& a, bool inherited) {
            attributes.insert({ a.getName(), inherited });
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
TEST_F(EntityClassTest, MultiLineEditorUsage)
{
    auto eclass = GlobalEntityClassManager().findClass("eclass_with_usage_attribute");
    ASSERT_TRUE(eclass);

    // Assume we have non-empty editor_usage/1/2 attributes
    EXPECT_NE(eclass->getAttributeValue("editor_usage"), "");
    EXPECT_NE(eclass->getAttributeValue("editor_usage1"), "");
    EXPECT_NE(eclass->getAttributeValue("editor_usage2"), "");

    auto editor_usage = eclass::getUsage(eclass);

    std::vector<std::string> singleAttributes =
    {
        eclass->getAttributeValue("editor_usage"),
        eclass->getAttributeValue("editor_usage1"),
        eclass->getAttributeValue("editor_usage2")
    };

    EXPECT_EQ(editor_usage, string::join(singleAttributes, "\n"));
}

void checkBucketEntityDef(const IEntityClassPtr& eclass)
{
    // These spawnargs are all defined directly on bucket_metal
    EXPECT_EQ(eclass->getAttributeValue("editor_usage"), "So you can kick the bucket.");
    EXPECT_EQ(eclass->getAttributeValue("editor_displayFolder"), "Moveables/Containers");
    EXPECT_EQ(eclass->getAttributeValue("mass"), "8");
    EXPECT_EQ(eclass->getAttributeValue("inherit"), "bucket_base");
    EXPECT_EQ(eclass->getAttributeValue("model"), "models/darkmod/containers/bucket.lwo");
    EXPECT_EQ(eclass->getAttributeValue("friction"), "0.2");
    EXPECT_EQ(eclass->getAttributeValue("clipmodel"), "models/darkmod/misc/clipmodels/bucket_cm.lwo");
    EXPECT_EQ(eclass->getAttributeValue("bouncyness"), "0.5");
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce"), "tdm_impact_metal_bucket");
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce_carpet"), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce_cloth"), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce_grass"), "tdm_impact_metal_bucket_on_soft");
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce_dirt"), "tdm_impact_metal_bucket_on_soft");

    // This is defined in the parent entityDef:
    EXPECT_EQ(eclass->getAttributeValue("snd_bounce_snow"), "tdm_impact_dirt");
}

// #5652: Reloading DEFs must not mess up the eclass attributes
TEST_F(EntityClassTest, ReloadDefsOnUnchangedFiles)
{
    auto eclass = GlobalEntityClassManager().findClass("bucket_metal");
    auto parent = eclass->getParent();

    // Check the parent, it defines an editor_usage which will mess up the child (when the #5652 problem was unfixed)
    EXPECT_TRUE(parent != nullptr);
    EXPECT_EQ(parent->getAttributeValue("editor_usage"), "Don't use. Base class for all TDM moveables.");

    checkBucketEntityDef(eclass);

    // Reload the defs and re-use the same eclass reference we got above, it ought to be still valid
    GlobalEntityClassManager().reloadDefs();

    checkBucketEntityDef(eclass);
}

TEST_F(EntityClassTest, ModifyEntityClass)
{
    auto cls = GlobalEntityClassManager().findClass("light");
    auto light = GlobalEntityModule().createEntity(cls);
    auto& spawnArgs = light->getEntity();

    // Light doesn't initially have a colour set
    light->setRenderSystem(GlobalRenderSystemFactory().createRenderSystem());
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

static const Vector4 GREEN(0, 1, 0, 1);
static const Vector4 YELLOW(1, 0, 1, 1);

void expectEntityClassColour(const IEntityClassPtr& eclass, const Vector4& expectedColour)
{
    EXPECT_EQ(eclass->getColour(), expectedColour) << "Expected colour mismatch on class " << eclass->getDeclName();
}

TEST_F(EntityClassTest, OverrideEClassColour)
{
    auto lightCls = algorithm::createEntityByClassName("light")->getEntity().getEntityClass();
    auto torchCls = algorithm::createEntityByClassName("light_torchflame_small")->getEntity().getEntityClass();

    // Light has an explicit green editor_color
    expectEntityClassColour(lightCls, GREEN);

    // This class does not have an explicit editor_color value, but should
    // inherit the one from 'light'.
    expectEntityClassColour(torchCls, GREEN);

    // Set an override for the 'light' class
    GlobalEclassColourManager().addOverrideColour("light", YELLOW);

    // Both 'light' and its subclasses should have the new colour
    EXPECT_EQ(lightCls->getColour(), YELLOW) << "Eclass colour override not working";
    EXPECT_EQ(torchCls->getColour(), YELLOW) << "Eclass colour override not working";
}

// Provided an override colour is set before the eclass is requested, it should still be effective
TEST_F(EntityClassTest, OverrideEClassColourSetBeforeRequest)
{
    // Set an override for the 'light' class
    GlobalEclassColourManager().addOverrideColour("light", YELLOW);

    auto lightCls = algorithm::createEntityByClassName("light")->getEntity().getEntityClass();
    auto torchCls = algorithm::createEntityByClassName("light_torchflame_small")->getEntity().getEntityClass();

    // Both 'light' and its subclasses should have the overridden colour
    EXPECT_EQ(lightCls->getColour(), YELLOW) << "Eclass colour override not working";
    EXPECT_EQ(torchCls->getColour(), YELLOW) << "Eclass colour override not working";
}

// Reload decls should keep the override intact
TEST_F(EntityClassTest, OverrideEClassColourAfterReload)
{
    auto lightCls = algorithm::createEntityByClassName("light")->getEntity().getEntityClass();
    auto torchCls = algorithm::createEntityByClassName("light_torchflame_small")->getEntity().getEntityClass();

    EXPECT_EQ(lightCls->getColour(), GREEN); // explicitly set
    EXPECT_EQ(torchCls->getColour(), GREEN); // inherited

    // Set an override for the 'light' class
    GlobalEclassColourManager().addOverrideColour("light", YELLOW);

    // Both 'light' and its subclasses should have the new colour
    EXPECT_EQ(lightCls->getColour(), YELLOW) << "Eclass colour override not working";
    EXPECT_EQ(torchCls->getColour(), YELLOW) << "Eclass colour override not working";

    GlobalDeclarationManager().reloadDeclarations();

    EXPECT_EQ(lightCls->getColour(), YELLOW) << "Eclass colour wrong after reloadDecls";
    EXPECT_EQ(torchCls->getColour(), YELLOW) << "Eclass colour wrong after reloadDecls";
}

// Removing an override should revert to defaults
TEST_F(EntityClassTest, OverrideEClassColourRemoval)
{
    auto lightCls = algorithm::createEntityByClassName("light")->getEntity().getEntityClass();
    auto torchCls = algorithm::createEntityByClassName("light_torchflame_small")->getEntity().getEntityClass();

    GlobalEclassColourManager().addOverrideColour("light", YELLOW);

    EXPECT_EQ(lightCls->getColour(), YELLOW) << "Eclass colour override not working";
    EXPECT_EQ(torchCls->getColour(), YELLOW) << "Eclass colour override not working";

    // Remove the override
    GlobalEclassColourManager().removeOverrideColour("light");

    // We should now be back at the defaults
    EXPECT_EQ(lightCls->getColour(), GREEN); // explicitly set
    EXPECT_EQ(torchCls->getColour(), GREEN); // inherited
}

TEST_F(EntityClassTest, DefaultEclassColourIsValid)
{
    auto eclass = GlobalEntityClassManager().findClass("dr:entity_using_modeldef");

    EXPECT_FALSE(eclass->getParent()) << "Entity Class is not supposed to have a parent, please adjust the test data";
    EXPECT_EQ(eclass->getAttributeValue("editor_color", true), "") << "Entity Class shouldn't have an editor_color in this test";

    EXPECT_EQ(eclass->getColour(), Vector4(0.3, 0.3, 1, 1)) << "The entity class should have the same value as in EntityClass.cpp:DefaultEntityColour";
}

TEST_F(EntityClassTest, MissingEclassColourIsValid)
{
    auto eclass = GlobalEntityClassManager().findOrInsert("___nonexistingeclass___", true);

    EXPECT_EQ(eclass->getAttributeValue("editor_color", true), "") << "Entity Class shouldn't have an editor_color in this test";
    EXPECT_EQ(eclass->getColour(), Vector4(0.3, 0.3, 1, 1)) << "The entity class should have the same value as in EntityClass.cpp:DefaultEntityColour";
}

TEST_F(EntityClassTest, GetEClassVisibility)
{
    // Normal entity with NORMAL visibility
    auto playerStart = GlobalEntityClassManager().findClass("info_player_start");
    ASSERT_TRUE(playerStart);
    EXPECT_EQ(playerStart->getVisibility(), vfs::Visibility::NORMAL);

    // Hidden entity
    auto entityBase = GlobalEntityClassManager().findClass("atdm:entity_base");
    ASSERT_TRUE(entityBase);
    EXPECT_EQ(entityBase->getAttributeValue("editor_visibility"), "hidden");
    EXPECT_EQ(entityBase->getVisibility(), vfs::Visibility::HIDDEN);
}

TEST_F(EntityClassTest, EClassVisibilityIsNotInherited)
{
    // func_static derives from the hidden atdm:entity_base, but should not in itself be hidden
    auto funcStatic = GlobalEntityClassManager().findClass("func_static");
    ASSERT_TRUE(funcStatic);
    EXPECT_EQ(funcStatic->getVisibility(), vfs::Visibility::NORMAL);
}

TEST_F(EntityClassTest, RemovedEntityClassIsHidden)
{
    // Create a temporary DEF file
    TemporaryFile tempFile(_context.getTestProjectPath() + "def/temporary_file.def");

    tempFile.setContents(R"(
entityDef someDefThatIsGoingToBeRemoved
{
    "groundlevel" "11"
}
)");

    // Reload the decls to find the new entityDef
    GlobalDeclarationManager().reloadDeclarations();

    auto eclass = GlobalEntityClassManager().findClass("someDefThatIsGoingToBeRemoved");
    EXPECT_TRUE(eclass) << "Cannot find someDefThatIsGoingToBeRemoved";

    // Remove the def from the file
    tempFile.setContents(R"(
entityDef aReplacementDef
{
    "groundlevel" "11"
}
)");

    GlobalDeclarationManager().reloadDeclarations();

    // the decl "someDefThatIsGoingToBeRemoved" should be there, but hidden
    eclass = GlobalEntityClassManager().findClass("someDefThatIsGoingToBeRemoved");

    EXPECT_TRUE(eclass) << "entityDef should still be registered after reloadDecls";
    EXPECT_TRUE(eclass->getBlockSyntax().contents.empty()) << "entityDef should be empty after reloadDecls";
    EXPECT_EQ(eclass->getVisibility(), vfs::Visibility::HIDDEN) << "entityDef should be hidden after reloadDecls";
}

TEST_F(EntityClassTest, VisibilityIsReevaluatedAfterReloadDecls)
{
    // Create a temporary DEF file
    TemporaryFile tempFile(_context.getTestProjectPath() + "def/temporary_file.def");

    tempFile.setContents(R"(
entityDef changingVisibility
{
    "editor_visibility" "hidden"
}
)");

    // Reload the decls to find the new entityDef
    GlobalDeclarationManager().reloadDeclarations();

    auto eclass = GlobalEntityClassManager().findClass("changingVisibility");
    EXPECT_TRUE(eclass) << "Cannot find changingVisibility";
    EXPECT_EQ(eclass->getVisibility(), vfs::Visibility::HIDDEN) << "Should be hidden";

    // Change the visibility and reload
    tempFile.setContents(R"(
entityDef changingVisibility
{
    "editor_visibility" "visible"
}
)");

    GlobalDeclarationManager().reloadDeclarations();

    eclass = GlobalEntityClassManager().findClass("changingVisibility");
    EXPECT_TRUE(eclass) << "Cannot find changingVisibility after reloadDecls";
    EXPECT_EQ(eclass->getVisibility(), vfs::Visibility::NORMAL) << "Should be visible now";
}

TEST_F(EntityClassTest, GetAttributeValue)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // Missing attribute is an empty string
    EXPECT_EQ(eclass->getAttributeValue("non_existent"), "");

    // Defined on the actual class
    EXPECT_EQ(eclass->getAttributeValue("ordinary_key"), "Test");

    // Inherited attributes should only appear if the includeInherited bool is set
    EXPECT_EQ(eclass->getAttributeValue("base_defined_bool", false), "");
    EXPECT_EQ(eclass->getAttributeValue("base_defined_bool", true), "1");
}

TEST_F(EntityClassTest, GetDefaultAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The default type is empty
    EXPECT_EQ(eclass->getAttributeType("ordinary_key"), "");
}

TEST_F(EntityClassTest, GetDefaultAttributeDescription)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The default description is empty
    EXPECT_EQ(eclass->getAttributeDescription("ordinary_key"), "");
}

TEST_F(EntityClassTest, GetNonInheritedAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The "defined_bool" is defined on the eclass, next to its editor_bool descriptor
    EXPECT_EQ(eclass->getAttributeType("defined_bool"), "bool");

    // The "undefined_bool" is not directly set on the eclass
    EXPECT_EQ(eclass->getAttributeType("undefined_bool"), "bool");
}

TEST_F(EntityClassTest, GetInheritedAttributeType)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The "base_defined_bool" is described in the base, as is the key
    EXPECT_EQ(eclass->getAttributeType("base_defined_bool"), "bool");

    // The "bool_not_defined_in_base" is set on the subclass, the description is in base
    EXPECT_EQ(eclass->getAttributeType("bool_not_defined_in_base"), "bool");

    // The "bool_not_defined_anywhere" is not set anywhere, only the description is there
    EXPECT_EQ(eclass->getAttributeType("bool_not_defined_anywhere"), "bool");
}

TEST_F(EntityClassTest, GetNonInheritedAttributeDescription)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The "defined_bool" is defined on the eclass, next to its editor_bool descriptor
    EXPECT_EQ(eclass->getAttributeDescription("defined_bool"), "Some bool description 2");

    // The "undefined_bool" is not directly set on the eclass
    EXPECT_EQ(eclass->getAttributeDescription("undefined_bool"), "Some bool description 1");
}

TEST_F(EntityClassTest, GetInheritedAttributeDescription)
{
    auto eclass = GlobalEntityClassManager().findClass("attribute_type_test");

    // The "base_defined_bool" is described in the base, as is the key
    EXPECT_EQ(eclass->getAttributeDescription("base_defined_bool"), "Some bool description");

    // The "bool_not_defined_in_base" is set on the subclass, the description is in base
    EXPECT_EQ(eclass->getAttributeDescription("bool_not_defined_in_base"), "Some bool description 12");

    // The "bool_not_defined_anywhere" is not set anywhere, only the description is there
    EXPECT_EQ(eclass->getAttributeDescription("bool_not_defined_anywhere"), "Some bool description 23");
}

TEST_F(EntityClassTest, GetVariousAttributeTypes)
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

TEST_F(EntityClassTest, FindModelDef)
{
    auto model = GlobalEntityClassManager().findModel("just_a_model");
    EXPECT_TRUE(model) << "ModelDef lookup failed";
    EXPECT_EQ(model->getMesh(), "just_an_md5.md5mesh");
    EXPECT_EQ(model->getBlockSyntax().fileInfo.fullPath(), "def/entity_with_model.def");

    EXPECT_TRUE(GlobalEntityClassManager().findModel("some_other_model"));
    EXPECT_TRUE(GlobalEntityClassManager().findModel("a_cooler_model"));

    EXPECT_EQ(GlobalEntityClassManager().findModel("just_a_model"),
        GlobalEntityClassManager().findModel("Just_a_MODEl")) << "Lookup should be case-insensitive";
}

TEST_F(EntityClassTest, ForeachModelDef)
{
    std::set<std::string> visitedModels;

    GlobalEntityClassManager().forEachModelDef([&](const IModelDef::Ptr& model)
        {
            visitedModels.insert(model->getDeclName());
        });

    EXPECT_GT(visitedModels.count("just_a_model"), 0) << "ModelDef visit failed";
    EXPECT_GT(visitedModels.count("some_other_model"), 0) << "ModelDef visit failed";
    EXPECT_GT(visitedModels.count("a_cooler_model"), 0) << "ModelDef visit failed";
}

TEST_F(EntityClassTest, CyclicInheritance)
{
    auto model = GlobalEntityClassManager().findModel("recursive_inheritance");

    EXPECT_TRUE(model) << "ModelDef lookup failed";
    EXPECT_EQ(model->getParent()->getDeclName(), "recursive_inheritance3");

    model = GlobalEntityClassManager().findModel("recursive_inheritance2");
    EXPECT_TRUE(model) << "ModelDef lookup failed";
    EXPECT_EQ(model->getParent()->getDeclName(), "recursive_inheritance");

    model = GlobalEntityClassManager().findModel("recursive_inheritance3");
    EXPECT_TRUE(model) << "ModelDef lookup failed";
    EXPECT_EQ(model->getParent()->getDeclName(), "recursive_inheritance2");
}

TEST_F(EntityClassTest, MeshInheritance)
{
    auto just_a_model = GlobalEntityClassManager().findModel("just_a_model");

    EXPECT_EQ(just_a_model->getMesh(), "just_an_md5.md5mesh");

    auto some_other_model = GlobalEntityClassManager().findModel("some_other_model");

    // some_other_model inherits the mesh from just_a_model
    EXPECT_EQ(some_other_model->getMesh(), just_a_model->getMesh());

    auto a_cooler_model = GlobalEntityClassManager().findModel("a_cooler_model");

    // a_cooler_model defines its own mesh
    EXPECT_EQ(a_cooler_model->getMesh(), "an_overridden_mesh.md5mesh");
}

TEST_F(EntityClassTest, AnimInheritance)
{
    auto just_a_model = GlobalEntityClassManager().findModel("just_a_model");

    EXPECT_EQ(just_a_model->getAnim("af_pose"), "models/md5/af_pose.md5anim");
    EXPECT_EQ(just_a_model->getAnim("idle"), "models/md5/idle.md5anim");

    auto some_other_model = GlobalEntityClassManager().findModel("some_other_model");

    // some_other_model inherits idle, redefines af_pose, introduces new_anim
    EXPECT_EQ(some_other_model->getAnim("af_pose"), "models/md5/some_other_af_pose.md5anim");
    EXPECT_EQ(some_other_model->getAnim("idle"), just_a_model->getAnim("idle"));
    EXPECT_EQ(some_other_model->getAnim("new_anim"), "models/md5/new_anim.md5anim");

    auto a_cooler_model = GlobalEntityClassManager().findModel("a_cooler_model");

    // a_cooler_model overrides idle, inherits the rest
    EXPECT_EQ(a_cooler_model->getAnim("af_pose"), some_other_model->getAnim("af_pose"));
    EXPECT_EQ(a_cooler_model->getAnim("idle"), "models/md5/a_cooler_idle.md5anim");
    EXPECT_EQ(a_cooler_model->getAnim("new_anim"), some_other_model->getAnim("new_anim"));
}

// Loading an entity of unknown class shouldn't dismiss any primitives
TEST_F(EntityClassTest, MissingEntityClassPreservesPrimitives)
{
    std::string mapName = "missing_entitydef.map";
    GlobalCommandSystem().executeCommand("OpenMap", mapName);

    auto mapTextBeforeSaving = algorithm::loadTextFromVfsFile("maps/" + mapName);

    // Find the entity whose class is unknown
    auto entity = algorithm::getEntityByName(GlobalMapModule().getRoot(), "missing_entityclass_1");
    EXPECT_TRUE(entity) << "Couldn't find the entity 'missing_entityclass_1' in this map";

    EXPECT_EQ(algorithm::getChildCount(entity, algorithm::brushHasMaterial("textures/common/caulk")), 2)
        << "Expected 2 brushes to carry the caulk material";
    EXPECT_EQ(algorithm::getChildCount(entity, algorithm::patchHasMaterial("textures/common/caulk")), 1)
        << "Expected 1 patch to carry the caulk material";

    // Remove the .darkradiant file (that is going to be created) after this test.
    TemporaryFile tempDarkRadiantFile(_context.getTestProjectPath() + "maps/missing_entitydef.darkradiant");

    GlobalCommandSystem().executeCommand("SaveMap");

    EXPECT_EQ(algorithm::loadTextFromVfsFile("maps/" + mapName), mapTextBeforeSaving)
        << "Map file contents are different after saving the map with the missing entityDef reference";
}

}
