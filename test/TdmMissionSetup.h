#pragma once

#include "RadiantTest.h"

namespace test
{

// Test context setting up a temporary FM folder
class TdmMissionContext :
    public radiant::TestContext
{
private:
    std::string _projectFolder;

public:
    TdmMissionContext()
    {
        // Set up the temporary settings folder
        _projectFolder = (os::getTemporaryPath() / "tdm_fm/").string();

        os::removeDirectory(_projectFolder);
        os::makeDirectory(_projectFolder);
    }

    ~TdmMissionContext()
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

// Test setup using an empty FM to be used as TDM project config
class TdmMissionSetup :
    public RadiantTest
{
protected:
    TdmMissionContext _context;

    constexpr static const char* TestMissionName = "testfm";
    constexpr static const char* MissionBasePath = "fms";

private:
    std::string _testFmPath;
    std::string _fmBasePath;

protected:
    const std::string& getTestMissionPath()
    {
        return _testFmPath;
    }

    virtual void setupGameFolder() override
    {
        _fmBasePath = _context.getTestProjectPath() + MissionBasePath + "/";
        _testFmPath = os::standardPathWithSlash(_fmBasePath + TestMissionName);

        os::makeDirectory(_fmBasePath);
        os::makeDirectory(_testFmPath);
    }

    virtual void TearDown() override
    {
        os::removeDirectory(_fmBasePath);
    }

    virtual void handleGameConfigMessage(game::ConfigurationNeeded& message) override
    {
        game::GameConfiguration config;

        config.gameType = "The Dark Mod 2.0 (Standalone)";
        config.enginePath = _context.getTestProjectPath();
        config.modPath = std::string(MissionBasePath) + "/" + TestMissionName;

        message.setConfig(config);
        message.setHandled(true);
    }
};

}
