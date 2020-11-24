#include "RadiantTest.h"

#include <fstream>
#include "ieclass.h"
#include "icolourscheme.h"
#include "ientity.h"
#include "imap.h"
#include "ibrush.h"
#include "ifilesystem.h"
#include "iselection.h"
#include "scenelib.h"

namespace test
{

// Test context setting up a temporary game folder
class Quake4TestContext :
    public radiant::TestContext
{
private:
    std::string _projectFolder;

public:
    Quake4TestContext()
    {
        // Set up the temporary settings folder
        _projectFolder = (os::getTemporaryPath() / "empty_q4_project/").string();

        os::removeDirectory(_projectFolder);
        os::makeDirectory(_projectFolder);
    }

    ~Quake4TestContext()
    {
        if (!_projectFolder.empty())
        {
            os::removeDirectory(_projectFolder);
        }
    }

    virtual std::string getTestProjectPath() const override
    {
        return os::standardPathWithSlash(_projectFolder);
    }
};

// Test setup using an empty Q4 project config
// containing just an empty Quake4.exe and a q4base/ folder
class EmptyQuake4Setup :
    public RadiantTest
{
private:
    Quake4TestContext _context;

public:

protected:
    virtual void setupGameFolder() override
    {
        // Create the q4base folder
        os::makeDirectory(_context.getTestProjectPath() + "q4base");

        std::ofstream outfile(_context.getTestProjectPath() + "Quake4.exe");
        outfile << " " << std::endl;
        outfile.close();
    }

    virtual void handleGameConfigMessage(game::ConfigurationNeeded& message) override
    {
        game::GameConfiguration config;

        config.gameType = "Quake 4";
        config.enginePath = _context.getTestProjectPath();

        message.setConfig(config);
        message.setHandled(true);
    }
};

using Quake4WorldspawnColourTest = EmptyQuake4Setup;

TEST_F(Quake4WorldspawnColourTest, SchemeBrushColourIsUsed)
{
    // File system should be loaded and ready
    EXPECT_TRUE(GlobalFileSystem().isInitialised());

    // This setup doesn't have a worldspawn
    EXPECT_FALSE(GlobalEntityClassManager().findClass("worldspawn"));

    // Create a worldspawn
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // The eclass wire shader should match the colour defined in the scheme
    const auto& schemeColour = GlobalColourSchemeManager().getActiveScheme().getColour("default_brush").getColour();
    auto schemeWireShader = fmt::format("<{0:f} {1:f} {2:f}>", schemeColour[0], schemeColour[1], schemeColour[2]);

    EXPECT_EQ(Node_getEntity(worldspawn)->getEntityClass()->getWireShader(), schemeWireShader);
}

TEST_F(RadiantTest, SchemeBrushColourIsUsed)
{
    // This setup does have a worldspawn
    EXPECT_TRUE(GlobalEntityClassManager().findOrInsert("worldspawn", true));

    // Create a worldspawn
    auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();

    // The eclass wire shader should match the colour defined in the scheme
    const auto& schemeColour = GlobalColourSchemeManager().getActiveScheme().getColour("default_brush").getColour();
    auto schemeWireShader = fmt::format("<{0:f} {1:f} {2:f}>", schemeColour[0], schemeColour[1], schemeColour[2]);

    EXPECT_EQ(Node_getEntity(worldspawn)->getEntityClass()->getWireShader(), schemeWireShader);
}

}
