#include "RadiantTest.h"

#include "ideclmanager.h"

namespace test
{

using DeclManagerTest = RadiantTest;

class TestDeclaration :
    public decl::IDeclaration
{
private:
    decl::Type _type;
    std::string _name;

public:
    TestDeclaration(decl::Type type, const std::string& name) :
        _type(type),
        _name(name)
    {}

    const std::string& getName() const override
    {
        return _name;
    }

    decl::Type getType() const override
    {
        return _type;
    }
};

class TestDeclarationParser :
    public decl::IDeclarationParser
{
public:
    // Returns the declaration type this parser can handle
    decl::Type getDeclType() const override
    {
        return decl::Type::Material;
    }

    // Create a new declaration instance from the given block
    decl::IDeclaration::Ptr parseFromBlock(const decl::DeclarationBlockSyntax& block) override
    {
        return std::make_shared<TestDeclaration>(getDeclType(), block.name);
    }
};

class TestDeclaration2Parser :
    public decl::IDeclarationParser
{
public:
    // Returns the declaration type this parser can handle
    decl::Type getDeclType() const override
    {
        return decl::Type::Model;
    }

    // Create a new declaration instance from the given block
    decl::IDeclaration::Ptr parseFromBlock(const decl::DeclarationBlockSyntax& block) override
    {
        return std::make_shared<TestDeclaration>(getDeclType(), block.name);
    }
};

TEST_F(DeclManagerTest, DeclTypeRegistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();
    EXPECT_NO_THROW(GlobalDeclarationManager().registerDeclType("testdecl", parser));

    // Registering the same type name twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", parser), std::logic_error);

    // Passing a new parser instance doesn't help either
    auto parser2 = std::make_shared<TestDeclarationParser>();
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", parser2), std::logic_error);
}

TEST_F(DeclManagerTest, DeclTypeUnregistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();
    GlobalDeclarationManager().registerDeclType("testdecl", parser);

    // Unregistering the parser should succeed
    EXPECT_NO_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"));

    // Trying to unregister it twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"), std::logic_error);
}

inline std::set<std::string> getAllDeclNames(decl::Type type)
{
    // Iterate over all decls and collect the names
    std::set<std::string> foundNames;

    GlobalDeclarationManager().foreachDeclaration(type, [&](const decl::IDeclaration& declaration)
    {
        foundNames.insert(declaration.getName());
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
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationParser>());

    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls/", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutSlash)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationParser>());

    // Omit the trailing slash, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithExtensionDot)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationParser>());

    // Add the dot to the file extension, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
}

// Test a second decl parser
TEST_F(DeclManagerTest, DeclTypeParserRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationParser>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Parser>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
    checkKnownTestDecl2Names();
}

// Test that a parser coming late to the party is immediately fed with the buffered decl blocks
TEST_F(DeclManagerTest, LateDeclTypeRegistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();


}

}
