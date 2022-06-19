#include "RadiantTest.h"

#include "ideclmanager.h"

namespace test
{

using DeclManagerTest = RadiantTest;

class TestDeclaration :
    public decl::IDeclaration
{
private:
    std::string _name;

public:
    TestDeclaration(const std::string& name) :
        _name(name)
    {}

    const std::string& getName() const override
    {
        return _name;
    }

    decl::Type getType() const override
    {
        return decl::Type::Material;
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
        return std::make_shared<TestDeclaration>(block.name);
    }
};

TEST_F(DeclManagerTest, DeclTypeRegistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();
    EXPECT_NO_THROW(GlobalDeclarationManager().registerDeclType("sometype", parser));

    // Registering the same type name twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("sometype", parser), std::logic_error);

    // Passing a new parser instance doesn't help either
    auto parser2 = std::make_shared<TestDeclarationParser>();
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("sometype", parser2), std::logic_error);
}

TEST_F(DeclManagerTest, DeclTypeUnregistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();
    GlobalDeclarationManager().registerDeclType("sometype", parser);

    // Unregistering the parser should succeed
    EXPECT_NO_THROW(GlobalDeclarationManager().unregisterDeclType("sometype"));

    // Trying to unregister it twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().unregisterDeclType("sometype"), std::logic_error);
}

inline void checkKnownMaterialDeclarations()
{
    // Iterate over all materials and collect the names
    std::set<std::string> foundNames;

    GlobalDeclarationManager().foreachDeclaration(decl::Type::Material, [&](const decl::IDeclaration& declaration)
        {
            foundNames.insert(declaration.getName());
        });

    // Check a few known ones
    EXPECT_TRUE(foundNames.count("textures/exporttest/guisurf1") > 0);
    EXPECT_TRUE(foundNames.count("textures/numbers/1") > 0);

    // Tables should not be listed
    EXPECT_FALSE(foundNames.count("sinTable") > 0);
}

TEST_F(DeclManagerTest, DeclFolderRegistration)
{
    GlobalDeclarationManager().registerDeclType("material", std::make_shared<TestDeclarationParser>());

    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "materials/", "mtr");

    checkKnownMaterialDeclarations();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutSlash)
{
    GlobalDeclarationManager().registerDeclType("material", std::make_shared<TestDeclarationParser>());

    // Omit the trailing slash, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "materials", "mtr");

    checkKnownMaterialDeclarations();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithExtensionDot)
{
    GlobalDeclarationManager().registerDeclType("material", std::make_shared<TestDeclarationParser>());

    // Add the dot to the file extension, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "materials", ".mtr");

    checkKnownMaterialDeclarations();
}

// Test that a parser coming late to the party is immediately fed with the buffered decl blocks
TEST_F(DeclManagerTest, LateDeclTypeRegistration)
{
    auto parser = std::make_shared<TestDeclarationParser>();


}

}
