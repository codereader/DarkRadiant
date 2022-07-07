#include "RadiantTest.h"

#include "ideclmanager.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using DeclManagerTest = RadiantTest;

class TestDeclaration :
    public decl::IDeclaration
{
private:
    decl::Type _type;
    std::string _name;
    decl::DeclarationBlockSyntax _block;
    std::size_t _parseStamp;

public:
    TestDeclaration(decl::Type type, const std::string& name) :
        _type(type),
        _name(name),
        _parseStamp(0)
    {}

    const std::string& getDeclName() const override
    {
        return _name;
    }

    decl::Type getDeclType() const override
    {
        return _type;
    }

    const decl::DeclarationBlockSyntax& getBlockSyntax() const override
    {
        return _block;
    }

    void setBlockSyntax(const decl::DeclarationBlockSyntax& block) override
    {
        _block = block;
    }

    std::size_t getParseStamp() const override
    {
        return _parseStamp;
    }

    void setParseStamp(std::size_t parseStamp) override
    {
        _parseStamp = parseStamp;
    }

    std::string getModName() const override
    {
        return _block.getModName();
    }

    std::string getDeclFilePath() const override
    {
        return _block.fileInfo.fullPath();
    }
};

class TestDeclarationCreator :
    public decl::IDeclarationCreator
{
public:
    bool processingDisabled = false;

    decl::Type getDeclType() const override
    {
        return decl::Type::Material;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        while (processingDisabled)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(20ms);
        }

        return std::make_shared<TestDeclaration>(getDeclType(), name);
    }
};

class TestDeclaration2Creator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::Model;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<TestDeclaration>(getDeclType(), name);
    }
};

TEST_F(DeclManagerTest, DeclTypeRegistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    EXPECT_NO_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator));

    // Registering the same type name twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator), std::logic_error);

    // Passing a new creator instance doesn't help either
    auto creator2 = std::make_shared<TestDeclarationCreator>();
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator2), std::logic_error);
}

TEST_F(DeclManagerTest, DeclTypeUnregistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    // Unregistering the creator should succeed
    EXPECT_NO_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"));

    // Trying to unregister it twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"), std::logic_error);
}

inline std::set<std::string> getAllDeclNames(decl::Type type)
{
    // Iterate over all decls and collect the names
    std::set<std::string> foundNames;

    GlobalDeclarationManager().foreachDeclaration(type, [&](const decl::IDeclaration::Ptr& declaration)
    {
        foundNames.insert(declaration->getDeclName());
    });

    return foundNames;
}

inline void checkKnownTestDeclNames()
{
    auto foundNames = getAllDeclNames(decl::Type::Material);

    // Check a few known ones
    EXPECT_TRUE(foundNames.count("decl/exporttest/guisurf1") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/0") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/1") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/2") > 0);

    // decltables should not be listed
    EXPECT_FALSE(foundNames.count("decltable1") > 0);
    EXPECT_FALSE(foundNames.count("decltable2") > 0);
    EXPECT_FALSE(foundNames.count("decltable3") > 0);
}

inline void checkKnownTestDecl2Names()
{
    auto foundNames = getAllDeclNames(decl::Type::Model);

    // Assume testdecls are not listed
    EXPECT_FALSE(foundNames.count("decl/exporttest/guisurf1") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/0") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/1") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/2") > 0);

    // testdecl2 should be listed
    EXPECT_TRUE(foundNames.count("decltable1") > 0);
    EXPECT_TRUE(foundNames.count("decltable2") > 0);
    EXPECT_TRUE(foundNames.count("decltable3") > 0);
}

TEST_F(DeclManagerTest, DeclFolderRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls/", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutSlash)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Omit the trailing slash, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithExtensionDot)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Add the dot to the file extension, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
}

// Test a second decl creator
TEST_F(DeclManagerTest, DeclTypeCreatorRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
    checkKnownTestDecl2Names();
}

// Test that a creator coming late to the party is immediately fed with the buffered decl blocks
TEST_F(DeclManagerTest, LateCreatorRegistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    // Let the testdecl creator finish its work
    getAllDeclNames(decl::Type::Material);

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable2") > 0);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable3") > 0);

    // Register the testdecl2 creator now, it should be used by the decl manager to parse the missing pieces
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // Everything should be registered now
    checkKnownTestDecl2Names();
}

TEST_F(DeclManagerTest, CreatorRegistrationDuringRunningThread)
{
    auto creator = std::make_shared<TestDeclarationCreator>();

    // Hold back this creator until we let it go in this fixture
    creator->processingDisabled = true;

    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Register the testdecl2 creator now, it should be used by the decl manager to parse the missing pieces
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // The first thread is still running, so we didn't get any unrecognised decls reported
    foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Let the testdecl creator finish its work
    creator->processingDisabled = false;
    getAllDeclNames(decl::Type::Material);

    // Everything should be registered now
    checkKnownTestDecl2Names();
}

TEST_F(DeclManagerTest, DeclsReloadedSignalAfterInitialParse)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    bool materialSignalFired = false;
    bool modelSignalFired = false;
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material).connect(
        [&]() { materialSignalFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Model).connect(
        [&]() { modelSignalFired = true; }
    );

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    // Force the thread to be finished
    GlobalDeclarationManager().foreachDeclaration(decl::Type::Material, [](const decl::IDeclaration::Ptr&) {});

    EXPECT_TRUE(materialSignalFired) << "Material signal should have fired by the time parsing has finished";
    EXPECT_FALSE(modelSignalFired) << "Model-type Signal should not have been fired";
}

TEST_F(DeclManagerTest, DeclsReloadedSignals)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    // Force the threads to be finished
    GlobalDeclarationManager().foreachDeclaration(decl::Type::Material, [](const decl::IDeclaration::Ptr&) {});
    GlobalDeclarationManager().foreachDeclaration(decl::Type::Model, [](const decl::IDeclaration::Ptr&) {});

    bool materialsReloadingFired = false;
    bool modelsReloadingFired = false;
    GlobalDeclarationManager().signal_DeclsReloading(decl::Type::Material).connect(
        [&]() { materialsReloadingFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloading(decl::Type::Model).connect(
        [&]() { modelsReloadingFired = true; }
    );

    bool materialsReloadedFired = false;
    bool modelsReloadedFired = false;
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material).connect(
        [&]() { materialsReloadedFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Model).connect(
        [&]() { modelsReloadedFired = true; }
    );

    GlobalDeclarationManager().reloadDeclarations();

    EXPECT_TRUE(materialsReloadingFired) << "Material signal should have before reloadDecls";
    EXPECT_TRUE(modelsReloadingFired) << "Model signal should have fired before reloadDecls";
    EXPECT_TRUE(materialsReloadedFired) << "Material signal should have fired after reloadDecls";
    EXPECT_TRUE(modelsReloadedFired) << "Model signal should have fired after reloadDecls";
}

TEST_F(DeclManagerTest, FindDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1"));
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/nonexistent"));

    // Find declaration is case-insensitive
    EXPECT_EQ(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1"),
              GlobalDeclarationManager().findDeclaration(decl::Type::Material, "Decl/eXporTTest/gUISURF1"));
}

TEST_F(DeclManagerTest, FindOrCreateDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    EXPECT_TRUE(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Material, "decl/exporttest/guisurf1"));
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/nonexistent")) <<
        "decl/nonexistent should not be present in this test setup";

    auto defaultDecl = GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Material, "decl/nonexistent");
    EXPECT_TRUE(defaultDecl) << "Declaration manager didn't create a defaulted declaration";

    EXPECT_EQ(defaultDecl->getDeclType(), decl::Type::Material);
    EXPECT_EQ(defaultDecl->getDeclName(), "decl/nonexistent");
    EXPECT_EQ(defaultDecl->getBlockSyntax().contents, std::string());
    EXPECT_EQ(defaultDecl->getBlockSyntax().fileInfo.visibility, vfs::Visibility::HIDDEN);

    EXPECT_EQ(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Material, "decl/nonexistent"), defaultDecl)
        << "We expect the created declaration to be persistent";
    EXPECT_EQ(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/nonexistent"), defaultDecl)
        << "We expect the created declaration to be persistent";
}

TEST_F(DeclManagerTest, FindOrCreateUnknownDeclarationType)
{
    // Unknown types should yield an exception
    EXPECT_THROW(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::None, "decl/nonexistent"), std::invalid_argument);
    EXPECT_THROW(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Undetermined, "decl/nonexistent"), std::invalid_argument);
}

TEST_F(DeclManagerTest, DeclarationMetadata)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1");

    EXPECT_TRUE(decl);
    EXPECT_EQ(decl->getDeclType(), decl::Type::Material);
    EXPECT_EQ(decl->getDeclFilePath(), "testdecls/exporttest.decl");
    EXPECT_EQ(decl->getModName(), RadiantTest::DefaultGameType);
}

inline void expectMaterialIsPresent(decl::Type type, const std::string& declName)
{
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(type, declName))
        << declName << " should be present";
}

inline void expectMaterialIsNotPresent(decl::Type type, const std::string& declName)
{
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(type, declName))
        << declName << " should not be present";
}

inline void expectMaterialContains(decl::Type type, const std::string& declName, const std::string& expectedContents)
{
    auto decl = GlobalDeclarationManager().findDeclaration(type, declName);
    EXPECT_TRUE(decl) << declName << " should be present";
    EXPECT_NE(decl->getBlockSyntax().contents.find(expectedContents), std::string::npos)
        << declName << " should contain the expected contents " << expectedContents;
}

// Material must still be present, but not contain the given string
inline void expectMaterialDoesNotContain(decl::Type type, const std::string& declName, const std::string& contents)
{
    auto decl = GlobalDeclarationManager().findDeclaration(type, declName);
    EXPECT_TRUE(decl) << declName << " should be present";
    EXPECT_EQ(decl->getBlockSyntax().contents.find(contents), std::string::npos)
        << declName << " should NOT contain " << contents;
}

// EntityDef is treated the same as EnTiTyDeF and entitydef
TEST_F(DeclManagerTest, DeclTypenamesAreCaseInsensitive)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/numbers/3");
    expectMaterialContains(decl::Type::Material, "decl/numbers/3", "diffusemap textures/numbers/3");
}

TEST_F(DeclManagerTest, ReloadDeclarationDetectsChangedFile)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");
    tempFile.setContents(R"(

decl/temporary/11
{
    diffusemap textures/temporary/11
}

decl/temporary/12
{
    diffusemap textures/temporary/12
}

)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialContains(decl::Type::Material, "decl/temporary/12", "diffusemap textures/temporary/12");
    expectMaterialIsNotPresent(decl::Type::Material, "decl/temporary/13");

    // Change the file, change temp12, remove temp11 and add temp13 instead
    tempFile.setContents(R"(

decl/temporary/12
{
    diffusemap textures/changed_temporary/12
}

testdecl decl/temporary/13
{
    diffusemap textures/temporary/13
}

)");

    GlobalDeclarationManager().reloadDeclarations();

    // Check the changes in temp12
    expectMaterialContains(decl::Type::Material, "decl/temporary/12", "diffusemap textures/changed_temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/13");
    expectMaterialDoesNotContain(decl::Type::Material, "decl/temporary/11", "diffusemap textures/temporary/11");
}

TEST_F(DeclManagerTest, ReloadDeclarationDetectsNewFile)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");

    tempFile.setContents(R"(
testdecl   decl/temporary/11 { diffusemap textures/temporary/11 }
testdecl    decl/temporary/12 { diffusemap textures/temporary/12 }
)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialIsNotPresent(decl::Type::Material, "decl/temporary/13");

    // Create a new file and reload decls
    TemporaryFile tempFile2(_context.getTestProjectPath() + "testdecls/temp_file2.decl");
    tempFile2.setContents(R"(
testdecl   decl/temporary/13 { diffusemap textures/temporary/13 }
testdecl    decl/temporary/14 { diffusemap textures/temporary/14 }
)");

    GlobalDeclarationManager().reloadDeclarations();

    // All the decls should be present now
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/13");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/14");
}

TEST_F(DeclManagerTest, ReloadDeclarationDetectsRemovedFile)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");
    tempFile.setContents(R"(
testdecl   decl/temporary/11 { diffusemap textures/temporary/11 }
testdecl    decl/temporary/12 { diffusemap textures/temporary/12 }
)");

    auto tempFile2 = std::make_shared<TemporaryFile>(_context.getTestProjectPath() + "testdecls/temp_file2.decl");
    tempFile2->setContents(R"(
testdecl   decl/temporary/13 { diffusemap textures/temporary/13 }
testdecl    decl/temporary/14 { diffusemap textures/temporary/14 }
)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/13");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/14");

    // Remove one file and reload decls
    tempFile2.reset();
    GlobalDeclarationManager().reloadDeclarations();

    // All decls should still be present
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/12");
    expectMaterialDoesNotContain(decl::Type::Material, "decl/temporary/13", "diffusemap textures/temporary/13");
    expectMaterialDoesNotContain(decl::Type::Material, "decl/temporary/14", "diffusemap textures/temporary/14");
}

// Have one declaration moved from one existing file to another
TEST_F(DeclManagerTest, ReloadDeclarationDetectsMovedDecl)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");
    TemporaryFile tempFile2(_context.getTestProjectPath() + "testdecls/temp_file2.decl");

    tempFile.setContents(R"(
testdecl   decl/temporary/11 { diffusemap textures/temporary/11 }
testdecl    decl/temporary/12 { diffusemap textures/temporary/12 }
)");

    tempFile2.setContents(R"(
testdecl   decl/temporary/13 { diffusemap textures/temporary/13 }
)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/13");

    // Move a decl from the first file to the second
    tempFile.setContents(R"(
testdecl   decl/temporary/11 { diffusemap textures/temporary/11 }
)");
    tempFile2.setContents(R"(
testdecl   decl/temporary/13 { diffusemap textures/temporary/13 }
testdecl    decl/temporary/12 { diffusemap textures/changed_temporary/12 } // changed the decl
)");

    GlobalDeclarationManager().reloadDeclarations();

    // All decls should still be present, contents of 12 should have changed
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/11");
    expectMaterialContains(decl::Type::Material, "decl/temporary/12", "diffusemap textures/changed_temporary/12");
    expectMaterialIsPresent(decl::Type::Material, "decl/temporary/13");
}

TEST_F(DeclManagerTest, ReloadDeclarationsIncreasesParseStamp)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1");
    EXPECT_TRUE(decl) << "Couldn't find the declaration decl/exporttest/guisurf1";

    auto firstParseStamp = decl->getParseStamp();

    GlobalDeclarationManager().reloadDeclarations();

    decl = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1");
    EXPECT_TRUE(decl) << "Couldn't find the declaration decl/exporttest/guisurf1";

    EXPECT_NE(decl->getParseStamp(), firstParseStamp) << "Parse stamp should have changed on reload";
}

TEST_F(DeclManagerTest, DeclarationPrecedence)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    expectMaterialIsPresent(decl::Type::Material, "decl/precedence_test/1");

    // The first decl in the file precedence_test1.decl takes precedence over any decls
    // declared in the same file or other files sorted after precedence_test1.decl
    expectMaterialContains(decl::Type::Material, "decl/precedence_test/1", "diffusemap textures/numbers/1");
}

}
