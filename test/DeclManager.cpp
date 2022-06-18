#include "RadiantTest.h"

#include "ideclmanager.h"

namespace test
{

using DeclManagerTest = RadiantTest;

class TestDeclaration :
    public decl::IDeclaration
{
public:
    const std::string& getName() const override
    {
        static std::string _testName("TestName");
        return _testName;
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
        return std::make_shared<TestDeclaration>();
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

}
