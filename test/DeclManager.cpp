#include "RadiantTest.h"

#include "ideclmanager.h"
#include "testutil/TemporaryFile.h"
#include "testutil/ThreadUtils.h"
#include "decl/EditableDeclaration.h"
#include "algorithm/FileUtils.h"
#include "os/path.h"
#include "parser/DefBlockTokeniser.h"
#include "string/case_conv.h"

namespace test
{

class DeclManagerTest :
    public RadiantTest
{
public:
    const char* const TEST_DECL_FILE = "numbers.decl";
    const char* const TEST_DECL_FOLDER = "testdecls/";

    void preStartup() override
    {
        // Remove the physical override_test.decl file
        fs::remove(_context.getTestProjectPath() + "testdecls/override_test.decl");

        // Create a backup of the numbers file
        fs::path testFile = _context.getTestProjectPath() + TEST_DECL_FOLDER + TEST_DECL_FILE;
        fs::path bakFile = testFile;
        bakFile.replace_extension(".bak");
        fs::remove(bakFile);
        fs::copy(testFile, bakFile);
    }

    void preShutdown() override
    {
        fs::path testFile = _context.getTestProjectPath() + TEST_DECL_FOLDER + TEST_DECL_FILE;
        fs::remove(testFile);
        fs::path bakFile = testFile;
        bakFile.replace_extension(".bak");
        fs::rename(bakFile, testFile);
    }
};

class ITestDeclaration :
    public decl::IDeclaration
{
public:
    using Ptr = std::shared_ptr<ITestDeclaration>;

    virtual ~ITestDeclaration() {}

    // Interface methods for testing purposes

    virtual std::string getKeyValue(const std::string& key) = 0;
    virtual void setKeyValue(const std::string& key, const std::string& value) = 0;
    virtual void foreachKeyValue(std::function<void(std::pair<std::string, std::string>)> functor) = 0;
};

class TestDeclaration :
    public decl::EditableDeclaration<ITestDeclaration>
{
private:
    std::map<std::string, std::string> _keyValues;

public:
    TestDeclaration(decl::Type type, const std::string& name) :
        EditableDeclaration<ITestDeclaration>(type, name)
    {}

    std::string getKeyValue(const std::string& key) override
    {
        ensureParsed();
        return _keyValues.count(key) > 0 ? _keyValues.at(key) : "";
    }

    // API method to simulate a change of the declaration contents
    void setKeyValue(const std::string& key, const std::string& value) override
    {
        _keyValues[key] = value;
        onParsedContentsChanged();
    }

    void foreachKeyValue(std::function<void(std::pair<std::string, std::string>)> functor) override
    {
        for (auto& pair : _keyValues)
        {
            functor(pair);
        }
    }

    int generateSyntaxInvocationCount = 0;

protected:
    std::string generateSyntax() override
    {
        ++generateSyntaxInvocationCount;

        std::stringstream stream;

        stream << "\n";

        for (const auto& [key, value] : _keyValues)
        {
            stream << "\"" << key << "\" \"" << value << "\"\n";
        }

        stream << "\n";

        return stream.str();
    }

    void parseFromTokens(parser::DefTokeniser& tokeniser) override
    {
        while (tokeniser.hasMoreTokens())
        {
            // Read key/value pairs until end of decl
            auto key = tokeniser.nextToken();
            auto value = tokeniser.nextToken();

            _keyValues.emplace(key, value);
        }
    }
};

class TestDeclarationCreator :
    public decl::IDeclarationCreator
{
public:
    bool processingDisabled = false;

    decl::Type getDeclType() const override
    {
        return decl::Type::TestDecl;
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
        return decl::Type::TestDecl2;
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
    auto foundNames = getAllDeclNames(decl::Type::TestDecl);

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
    auto foundNames = getAllDeclNames(decl::Type::TestDecl2);

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

    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutSlash)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Omit the trailing slash, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, "testdecls", ".decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutExtensionDot)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Add no dot to the file extension, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, "decl");

    checkKnownTestDeclNames();
}

// Test a second decl creator
TEST_F(DeclManagerTest, DeclTypeCreatorRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    checkKnownTestDeclNames();
    checkKnownTestDecl2Names();
}

// Test that a creator coming late to the party is immediately fed with the buffered decl blocks
TEST_F(DeclManagerTest, LateCreatorRegistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    // Let the testdecl creator finish its work
    getAllDeclNames(decl::Type::TestDecl);

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::TestDecl2);
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::TestDecl2);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Register the testdecl2 creator now, it should be used by the decl manager to parse the missing pieces
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // The first thread is still running, so we didn't get any unrecognised decls reported
    foundTestDecl2Names = getAllDeclNames(decl::Type::TestDecl2);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Let the testdecl creator finish its work
    creator->processingDisabled = false;
    getAllDeclNames(decl::Type::TestDecl);

    // Everything should be registered now
    checkKnownTestDecl2Names();
}

TEST_F(DeclManagerTest, DeclsReloadedSignalAfterInitialParse)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    bool testdeclSignalFired = false;
    bool testdecl2SignalFired = false;
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::TestDecl).connect(
        [&]() { testdeclSignalFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::TestDecl2).connect(
        [&]() { testdecl2SignalFired = true; }
    );

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    // Force the threads to be finished
    GlobalDeclarationManager().foreachDeclaration(decl::Type::TestDecl, [](const decl::IDeclaration::Ptr&) {});

    EXPECT_TRUE(algorithm::waitUntil([&]() { return testdeclSignalFired; })) << "Time out waiting for the flag";

    EXPECT_TRUE(testdeclSignalFired) << "testdecl signal should have fired by the time parsing has finished";
    EXPECT_FALSE(testdecl2SignalFired) << "testdecl2-type Signal should not have been fired";
}

TEST_F(DeclManagerTest, DeclsReloadedSignals)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    // Force the threads to be finished
    GlobalDeclarationManager().foreachDeclaration(decl::Type::TestDecl, [](const decl::IDeclaration::Ptr&) {});
    GlobalDeclarationManager().foreachDeclaration(decl::Type::TestDecl2, [](const decl::IDeclaration::Ptr&) {});

    bool testdeclsReloadingFired = false;
    bool testdecl2sReloadingFired = false;
    GlobalDeclarationManager().signal_DeclsReloading(decl::Type::TestDecl).connect(
        [&]() { testdeclsReloadingFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloading(decl::Type::TestDecl2).connect(
        [&]() { testdecl2sReloadingFired = true; }
    );

    auto callingThreadId = std::this_thread::get_id();
    std::thread::id signalThreadId;

    std::size_t testdeclsReloadedFireCount = 0;
    bool testdecl2sReloadedFired = false;
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::TestDecl).connect(
        [&]()
        {
            ++testdeclsReloadedFireCount;
            signalThreadId = std::this_thread::get_id();
        }
    );
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::TestDecl2).connect(
        [&]() { testdecl2sReloadedFired = true; }
    );

    GlobalDeclarationManager().reloadDeclarations();

    EXPECT_TRUE(testdeclsReloadingFired) << "testdecl signal should have fired before reloadDecls";
    EXPECT_TRUE(testdecl2sReloadingFired) << "testdecl2 signal should have fired before reloadDecls";
    EXPECT_EQ(testdeclsReloadedFireCount, 1) << "testdecl signal should have fired once after reloadDecls";
    EXPECT_TRUE(testdecl2sReloadedFired) << "testdecl2 signal should have fired after reloadDecls";

    // The signal has to be fire on the same thread as the calling code
    EXPECT_EQ(callingThreadId, signalThreadId) << "Reloaded Signal should have been fired on the calling thread.";
}

TEST_F(DeclManagerTest, FindDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1"));
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/nonexistent"));

    // Find declaration is case-insensitive
    EXPECT_EQ(GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1"),
              GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "Decl/eXporTTest/gUISURF1"));
}

TEST_F(DeclManagerTest, FindOrCreateDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    EXPECT_TRUE(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1"));
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/nonexistent")) <<
        "decl/nonexistent should not be present in this test setup";

    auto defaultDecl = GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "decl/nonexistent");
    EXPECT_TRUE(defaultDecl) << "Declaration manager didn't create a defaulted declaration";

    EXPECT_EQ(defaultDecl->getDeclType(), decl::Type::TestDecl);
    EXPECT_EQ(defaultDecl->getDeclName(), "decl/nonexistent");
    EXPECT_EQ(defaultDecl->getBlockSyntax().contents, std::string());
    EXPECT_EQ(defaultDecl->getBlockSyntax().fileInfo.visibility, vfs::Visibility::HIDDEN);

    EXPECT_EQ(GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "decl/nonexistent"), defaultDecl)
        << "We expect the created declaration to be persistent";
    EXPECT_EQ(GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/nonexistent"), defaultDecl)
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1");

    EXPECT_TRUE(decl);
    EXPECT_EQ(decl->getDeclType(), decl::Type::TestDecl);
    EXPECT_EQ(decl->getDeclFilePath(), "testdecls/exporttest.decl");
    EXPECT_EQ(decl->getModName(), RadiantTest::DefaultGameType);
}

inline void expectDeclIsPresent(decl::Type type, const std::string& declName)
{
    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(type, declName))
        << declName << " should be present";
}

inline void expectDeclIsNotPresent(decl::Type type, const std::string& declName)
{
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(type, declName))
        << declName << " should not be present";
}

inline void expectDeclContains(decl::Type type, const std::string& declName, const std::string& expectedContents)
{
    auto decl = GlobalDeclarationManager().findDeclaration(type, declName);
    EXPECT_TRUE(decl) << declName << " should be present";
    EXPECT_NE(decl->getBlockSyntax().contents.find(expectedContents), std::string::npos)
        << declName << " should contain the expected contents " << expectedContents;
}

// Decl must still be present, but not contain the given string
inline void expectDeclDoesNotContain(decl::Type type, const std::string& declName, const std::string& contents)
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/numbers/3");
    expectDeclContains(decl::Type::TestDecl, "decl/numbers/3", "diffusemap textures/numbers/3");
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclContains(decl::Type::TestDecl, "decl/temporary/12", "diffusemap textures/temporary/12");
    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/temporary/13");

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
    expectDeclContains(decl::Type::TestDecl, "decl/temporary/12", "diffusemap textures/changed_temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/13");
    expectDeclDoesNotContain(decl::Type::TestDecl, "decl/temporary/11", "diffusemap textures/temporary/11");
}

TEST_F(DeclManagerTest, ReloadDeclarationDetectsNewFile)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");

    tempFile.setContents(R"(
testdecl   decl/temporary/11 { diffusemap textures/temporary/11 }
testdecl    decl/temporary/12 { diffusemap textures/temporary/12 }
)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/temporary/13");

    // Create a new file and reload decls
    TemporaryFile tempFile2(_context.getTestProjectPath() + "testdecls/temp_file2.decl");
    tempFile2.setContents(R"(
testdecl   decl/temporary/13 { diffusemap textures/temporary/13 }
testdecl    decl/temporary/14 { diffusemap textures/temporary/14 }
)");

    GlobalDeclarationManager().reloadDeclarations();

    // All the decls should be present now
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/13");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/14");
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/13");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/14");

    // Remove one file and reload decls
    tempFile2.reset();
    GlobalDeclarationManager().reloadDeclarations();

    // All decls should still be present
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/12");
    expectDeclDoesNotContain(decl::Type::TestDecl, "decl/temporary/13", "diffusemap textures/temporary/13");
    expectDeclDoesNotContain(decl::Type::TestDecl, "decl/temporary/14", "diffusemap textures/temporary/14");
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
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/13");

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
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/11");
    expectDeclContains(decl::Type::TestDecl, "decl/temporary/12", "diffusemap textures/changed_temporary/12");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/temporary/13");
}

TEST_F(DeclManagerTest, ReloadDeclarationsIncreasesParseStamp)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1");
    EXPECT_TRUE(decl) << "Couldn't find the declaration decl/exporttest/guisurf1";

    auto firstParseStamp = decl->getParseStamp();

    GlobalDeclarationManager().reloadDeclarations();

    decl = GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/exporttest/guisurf1");
    EXPECT_TRUE(decl) << "Couldn't find the declaration decl/exporttest/guisurf1";

    EXPECT_NE(decl->getParseStamp(), firstParseStamp) << "Parse stamp should have changed on reload";
}

TEST_F(DeclManagerTest, DeclarationPrecedence)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/precedence_test/1");

    // The first decl in the file precedence_test1.decl takes precedence over any decls
    // declared in the same file or other files sorted after precedence_test1.decl
    expectDeclContains(decl::Type::TestDecl, "decl/precedence_test/1", "diffusemap textures/numbers/1");
}

TEST_F(DeclManagerTest, RemoveDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/precedence_test/1");

    GlobalDeclarationManager().removeDeclaration(decl::Type::TestDecl, "decl/precedence_test/1");

    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/precedence_test/1");
}

TEST_F(DeclManagerTest, RenameDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/precedence_test/1");
    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/renamed/1");

    auto oldSyntax = GlobalDeclarationManager().findDeclaration(
        decl::Type::TestDecl, "decl/precedence_test/1")->getBlockSyntax();

    auto result = GlobalDeclarationManager().renameDeclaration(
        decl::Type::TestDecl, "decl/precedence_test/1", "decl/renamed/1");

    EXPECT_TRUE(result) << "Rename operation should have succeeded";

    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/precedence_test/1");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/renamed/1");

    auto newSyntax = GlobalDeclarationManager().findDeclaration(
        decl::Type::TestDecl, "decl/renamed/1")->getBlockSyntax();

    // Check that the syntax of the renamed declaration is the same as before
    EXPECT_EQ(newSyntax.name, oldSyntax.name);
    EXPECT_EQ(newSyntax.contents, oldSyntax.contents);
    EXPECT_EQ(newSyntax.getModName(), oldSyntax.getModName());
    EXPECT_EQ(newSyntax.typeName, oldSyntax.typeName);
    EXPECT_EQ(newSyntax.fileInfo.fullPath(), oldSyntax.fileInfo.fullPath());
}

TEST_F(DeclManagerTest, RenameDeclarationFailsIfNotExisting)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsNotPresent(decl::Type::TestDecl, "decl/notexisting/1");

    auto result = GlobalDeclarationManager().renameDeclaration(
        decl::Type::TestDecl, "decl/notexisting/1", "decl/renamed/1");

    EXPECT_FALSE(result) << "Rename operation should have failed";
}

TEST_F(DeclManagerTest, RenameDeclarationFailsIfNameIsTheSame)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/precedence_test/1");

    auto result = GlobalDeclarationManager().renameDeclaration(
        decl::Type::TestDecl, "decl/precedence_test/1", "decl/precedence_test/1");

    EXPECT_FALSE(result) << "Rename operation should have failed";
}

TEST_F(DeclManagerTest, RenameDeclarationFailsIfNameIsInUse)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    expectDeclIsPresent(decl::Type::TestDecl, "decl/numbers/1");
    expectDeclIsPresent(decl::Type::TestDecl, "decl/numbers/2");

    auto result = GlobalDeclarationManager().renameDeclaration(
        decl::Type::TestDecl, "decl/numbers/1", "decl/numbers/2");

    EXPECT_FALSE(result) << "Rename operation should have failed";
}

TEST_F(DeclManagerTest, SyntaxGeneration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<TestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/0"));

    EXPECT_EQ(decl->getKeyValue("diffusemap"), "textures/numbers/0");

    // Expect the unchanged block to be present
    EXPECT_NE(decl->getBlockSyntax().contents.find("textures/numbers/0"), std::string::npos);
    decl->generateSyntaxInvocationCount = 0;

    // Change the declaration
    decl->setKeyValue("diffusemap", "some/other/texture");

    // The old material name should be gone now, the new one should be there
    EXPECT_EQ(decl->generateSyntaxInvocationCount, 0) << "No call to generateSyntax should have been recorded yet";
    EXPECT_EQ(decl->getBlockSyntax().contents.find("textures/numbers/0"), std::string::npos);
    EXPECT_EQ(decl->generateSyntaxInvocationCount, 1) << "A call to generateSyntax should have been recorded";
    EXPECT_NE(decl->getBlockSyntax().contents.find("some/other/texture"), std::string::npos);
    EXPECT_EQ(decl->generateSyntaxInvocationCount, 1) << "Only one call to generateSyntax should have been recorded";
}

inline void expectDeclIsPresentInFile(const ITestDeclaration::Ptr& decl, const std::string& path, bool expectPresent)
{
    auto contents = algorithm::loadTextFromVfsFile(path);

    parser::BasicDefBlockTokeniser<std::string> tokeniser(contents);

    std::vector<parser::BlockTokeniser::Block> foundBlocks;
    auto declName = string::to_lower_copy(decl->getDeclName());

    // Run a check against our custom decl
    auto hasAllKeyValuePairs = true;

    while (tokeniser.hasMoreBlocks())
    {
        auto block = tokeniser.nextBlock();
        auto header = string::to_lower_copy(block.name);

        if (string::ends_with(header, declName))
        {
            foundBlocks.push_back(block);

            // Every key and every value must be present in the file
            decl->foreachKeyValue([&](std::pair<std::string, std::string> pair)
            {
                hasAllKeyValuePairs &= block.contents.find("\"" + pair.first + "\"") != std::string::npos;
                hasAllKeyValuePairs &= block.contents.find("\"" + pair.second + "\"") != std::string::npos;
            });
        }
    }

    if (expectPresent)
    {
        EXPECT_EQ(foundBlocks.size() && hasAllKeyValuePairs, 1) << "Expected exactly one decl " << declName << " in the contents in the file";
    }
    else
    {
        EXPECT_TRUE(foundBlocks.empty() || !hasAllKeyValuePairs) << "Expected no decl " << declName << " in the contents in the file";
    }
}

// Save a new declaration to a file on disk
TEST_F(DeclManagerTest, SaveNewDeclToNewFile)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "some_def"));

    decl->setKeyValue("tork", "some_value");

    // Set the decl to save its contents to a new file that doesn't exist yet
    auto syntax = decl->getBlockSyntax();
    auto fileInfo = vfs::FileInfo(TEST_DECL_FOLDER, "some_new_file.decl", vfs::Visibility::NORMAL);
    decl->setFileInfo(fileInfo);

    auto outputPath = _context.getTestProjectPath() + fileInfo.fullPath();
    EXPECT_FALSE(fs::exists(outputPath)) << "Output file shouldn't exist yet";

    // Auto-remove the file that is going to be written
    TemporaryFile outputFile(outputPath);

    GlobalDeclarationManager().saveDeclaration(decl);

    EXPECT_TRUE(fs::exists(outputPath)) << "Output file should exist now";

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);
}

// Save the decl to a file that already exists (but doesn't contain the def)
TEST_F(DeclManagerTest, SaveNewDeclToExistingFile)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "some_def346"));

    decl->setKeyValue("tork", "myvalue with spaces");

    // Set up the decl to save its contents to an existing file
    auto syntax = decl->getBlockSyntax();
    auto fileInfo = vfs::FileInfo(TEST_DECL_FOLDER, "numbers.decl", vfs::Visibility::NORMAL);
    decl->setFileInfo(fileInfo);

    auto outputPath = _context.getTestProjectPath() + syntax.fileInfo.fullPath();
    EXPECT_TRUE(fs::exists(outputPath)) << "Output file must already exist";

    // Def file should not have that decl yet
    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // the fixture will revert the changes to numbers.decl
}

// Save a decl to the same physical file that originally declared the decl
TEST_F(DeclManagerTest, SaveExistingDeclToExistingFile)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/1"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/1");
    decl->setKeyValue("tork", "new_value");

    // This modified decl should not be present
    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    // Save, it should be there now
    GlobalDeclarationManager().saveDeclaration(decl);
    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// Save decl originally defined in a PK4 file to a new physical file that overrides the PK4
TEST_F(DeclManagerTest, SaveExistingDeclToNewFileOverridingPk4)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/export/1"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/1");
    decl->setKeyValue("tork", "new_value");

    // The overriding file should not be present
    auto outputPath = _context.getTestProjectPath() + decl->getBlockSyntax().fileInfo.fullPath();

    // Let the file be deleted when we're done here
    TemporaryFile outputFile(outputPath);
    EXPECT_FALSE(fs::exists(outputPath));

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), false);

    // Save, it should be there now
    GlobalDeclarationManager().saveDeclaration(decl);
    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // Check if the other decl declaration is still intact in the file (use the same path to check)
    auto export2Decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/export/2"));
    EXPECT_EQ(export2Decl->getDeclFilePath(), decl->getDeclFilePath()) << "The decls should be in the same .decl file";
    expectDeclIsPresentInFile(export2Decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    auto export0Decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/export/0"));
    EXPECT_EQ(export0Decl->getDeclFilePath(), decl->getDeclFilePath()) << "The decls should be in the same .decl file";
    expectDeclIsPresentInFile(export0Decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    auto contents = algorithm::loadTextFromVfsFile(decl->getBlockSyntax().fileInfo.fullPath());

    // Comments need to be left untouched
    EXPECT_NE(contents.find("Some comment before the declaration decl/export/0"), std::string::npos) << "Comments should be left intact";
    EXPECT_NE(contents.find("Some comment before the declaration decl/export/1"), std::string::npos) << "Comments should be left intact";
    EXPECT_NE(contents.find("Some comment before the declaration decl/export/2"), std::string::npos) << "Comments should be left intact";
}

TEST_F(DeclManagerTest, SaveDeclarationWithoutFileInfoThrows)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::TestDecl, "newdecl/0"));

    // Sabotage the decl to ensure an empty file name
    decl->setFileInfo(vfs::FileInfo(TEST_DECL_FOLDER, "", vfs::Visibility::NORMAL));

    // Saving a decl without file info needs to throw
    EXPECT_THROW(GlobalDeclarationManager().saveDeclaration(decl), std::invalid_argument);
}

// In numbers.decl, decl/numbers/1 is declared without an explicit typename
TEST_F(DeclManagerTest, SaveExistingDeclLackingTypename)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/1"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/1");

    // Save, then it should be present in the file
    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// In numbers.decl, testDecl decl/numbers/2 is declared using an explicit typename (case is matching)
TEST_F(DeclManagerTest, SaveExistingDeclWithMatchingTypename)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/2"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/2");

    // Save, then it should be present in the file
    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// In numbers.decl, TestDecl decl/numbers/3 is declared using an explicit typename that has different casing
TEST_F(DeclManagerTest, SaveExistingDeclWithMixedCaseTypename)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/3"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/3");

    // Save, then it should be present in the file
    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// Looks like this: testdecl decl/numbers/5 { // a comment in the same line as the opening brace
TEST_F(DeclManagerTest, SaveExistingDeclWithCommentInTheSameLine)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/5"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/5");

    // Save, then it should be present in the file
    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

// Decl looks like this: testdecl decl/numbers/6 { diffusemap textures/numbers/6 }
TEST_F(DeclManagerTest, SaveExistingDeclWithEverythingInASingleLine)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<ITestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/6"));

    // Swap some contents of this decl
    decl->setKeyValue("diffusemap", "textures/changed/6");
    decl->setKeyValue("bumpmap", "textures/whatever/6");

    // Save, then it should be present in the file
    GlobalDeclarationManager().saveDeclaration(decl);

    expectDeclIsPresentInFile(decl, decl->getBlockSyntax().fileInfo.fullPath(), true);

    // The test fixture will restore the original file contents in TearDown
}

TEST_F(DeclManagerTest, SetDeclFileInfo)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/3");

    decl->setFileInfo(vfs::FileInfo("materials/", "testfile.mtr", vfs::Visibility::HIDDEN));

    EXPECT_EQ(decl->getBlockSyntax().fileInfo.name, "testfile.mtr");
    EXPECT_EQ(decl->getBlockSyntax().fileInfo.topDir, "materials/");
    EXPECT_EQ(decl->getBlockSyntax().fileInfo.visibility, vfs::Visibility::HIDDEN);
}

// Changed signal should fire on assigning a new syntax block
TEST_F(DeclManagerTest, ChangedSignalOnSyntaxBlockChange)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/3");

    std::size_t changedSignalReceiveCount = 0;
    decl->signal_DeclarationChanged().connect([&] { ++changedSignalReceiveCount; });

    // Assign a new syntax block, this should emit the signal
    auto syntax = decl->getBlockSyntax();
    syntax.contents += "\n";
    decl->setBlockSyntax(syntax);

    EXPECT_EQ(changedSignalReceiveCount, 1) << "Changed signal should have fired once after assigning the syntax block";
}

// Change signal should fire when an EditableDeclaration is modified
TEST_F(DeclManagerTest, ChangedSignalOnEdit)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::TestDecl, TEST_DECL_FOLDER, ".decl");

    auto decl = std::static_pointer_cast<TestDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::TestDecl, "decl/numbers/3"));

    std::size_t changedSignalReceiveCount = 0;
    decl->signal_DeclarationChanged().connect([&] { ++changedSignalReceiveCount; });

    // Change the editable declaration
    decl->setKeyValue("nork", "tork");

    EXPECT_EQ(changedSignalReceiveCount, 1) << "Changed signal should have fired once after editing the decl";
}

}
