#include "XMLRegistry.h"

#include <iostream>
#include <stdexcept>
#include "itextstream.h"

#include "os/file.h"
#include "os/path.h"

#include "version.h"
#include "string/string.h"
#include "string/encoding.h"
#include "module/StaticModule.h"

namespace registry
{

XMLRegistry::XMLRegistry() :
    _queryCounter(0),
    _changesSinceLastSave(0),
    _shutdown(false)
{}

void XMLRegistry::shutdown()
{
    rMessage() << "XMLRegistry Shutdown: " << _queryCounter << " queries processed." << std::endl;

    saveToDisk();

    _shutdown = true;
    _autosaveTimer.reset();
}

void XMLRegistry::saveToDisk()
{
    // Save the user tree to the settings path, this contains all
    // settings that have been modified during runtime
    if (!get(RKEY_SKIP_REGISTRY_SAVE).empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lock(_writeLock);

    // Make a deep copy of the user tree by copy-constructing it
    RegistryTree copiedTree(_userTree);

    // Application-relative on other OS
    std::string settingsPath = module::GlobalModuleRegistry().getApplicationContext().getSettingsPath();

    // Replace the version tag and set it to the current DarkRadiant version
    copiedTree.deleteXPath("user//version");
    copiedTree.set("user/version", RADIANT_VERSION);

    // Export the user-defined filter definitions to a separate file
    copiedTree.exportToFile("user/ui/filtersystem/filters", settingsPath + "filters.xml");
    copiedTree.deleteXPath("user/ui/filtersystem/filters");

    // Export the colour schemes and remove them from the registry
    copiedTree.exportToFile("user/ui/colourschemes", settingsPath + "colours.xml");
    copiedTree.deleteXPath("user/ui/colourschemes");

    // Export the input definitions into the user's settings folder and remove them as well
    copiedTree.exportToFile("user/ui/input", settingsPath + "input.xml");
    copiedTree.deleteXPath("user/ui/input");

    // Delete all nodes marked as "transient", they are NOT exported into the user's xml file
    copiedTree.deleteXPath("user/*[@transient='1']");

    // Remove any remaining upgradePaths (from older registry files)
    copiedTree.deleteXPath("user/upgradePaths");
    // Remove legacy <interface> node
    copiedTree.deleteXPath("user/ui/interface");

    // Save the remaining /darkradiant/user tree to user.xml so that the current settings are preserved
    copiedTree.exportToFile("user", settingsPath + "user.xml");

    _changesSinceLastSave = 0;
}

xml::NodeList XMLRegistry::findXPath(const std::string& path)
{
    // Query the user tree first
    xml::NodeList results = _userTree.findXPath(path);
    xml::NodeList stdResults = _standardTree.findXPath(path);

    // Append the stdResults to the results
    std::copy(stdResults.begin(), stdResults.end(), std::back_inserter(results));

    _queryCounter++;

    return results;
}

void XMLRegistry::dump() const
{
    rConsole() << "User Tree:" << std::endl;
    _userTree.dump();
    rConsole() << "Default Tree:" << std::endl;
    _standardTree.dump();
}

void XMLRegistry::exportToFile(const std::string& key, const std::string& filename)
{
    // Only the usertree should be exported, so pass the call to this tree
    _userTree.exportToFile(key, filename);
}

sigc::signal<void> XMLRegistry::signalForKey(const std::string& key) const
{
    return _keySignals[key]; // will return existing or default-construct
}

bool XMLRegistry::keyExists(const std::string& key)
{
    // Pass the query on to findXPath which queries the subtrees
    xml::NodeList result = findXPath(key);
    return !result.empty();
}

void XMLRegistry::deleteXPath(const std::string& path)
{
    std::lock_guard<std::mutex> lock(_writeLock);

    assert(!_shutdown);

    // Add the toplevel node to the path if required
    xml::NodeList nodeList = findXPath(path);

    if (!nodeList.empty())
    {
        _changesSinceLastSave++;
    }

    for (xml::Node& node : nodeList)
    {
        // unlink and delete the node
        node.erase();
    }
}

xml::Node XMLRegistry::createKeyWithName(const std::string& path,
                                         const std::string& key,
                                         const std::string& name)
{
    std::lock_guard<std::mutex> lock(_writeLock);

    assert(!_shutdown);

    _changesSinceLastSave++;

    // The key will be created in the user tree (the default tree is read-only)
    return _userTree.createKeyWithName(path, key, name);
}

xml::Node XMLRegistry::createKey(const std::string& key)
{
    std::lock_guard<std::mutex> lock(_writeLock);

    assert(!_shutdown);

    _changesSinceLastSave++;

    return _userTree.createKey(key);
}

void XMLRegistry::setAttribute(const std::string& path,
    const std::string& attrName, const std::string& attrValue)
{
    std::lock_guard<std::mutex> lock(_writeLock);

    assert(!_shutdown);

    _changesSinceLastSave++;

    _userTree.setAttribute(path, attrName, attrValue);
}

std::string XMLRegistry::getAttribute(const std::string& path,
                                      const std::string& attrName)
{
    // Pass the query to the findXPath method, which queries the user tree first
    xml::NodeList nodeList = findXPath(path);

    if (nodeList.empty())
    {
        return std::string();
    }

    return nodeList[0].getAttributeValue(attrName);
}

std::string XMLRegistry::get(const std::string& key)
{
    // Pass the query to the findXPath method, which queries the user tree first
    xml::NodeList nodeList = findXPath(key);

    // Does it even exist?
    // It may well be the case that this returns two or more nodes that match the key criteria
    // This function always uses the first one, as the user tree should override the default tree
    if (!nodeList.empty())
    {
        // Convert the UTF-8 string back to locale and return
        return string::utf8_to_mb(nodeList[0].getAttributeValue("value"));
    }

    return std::string();
}

void XMLRegistry::set(const std::string& key, const std::string& value)
{
    {
        std::lock_guard<std::mutex> lock(_writeLock);

        assert(!_shutdown);

        // Create or set the value in the user tree, the default tree stays untouched
        // Convert the string to UTF-8 before storing it into the RegistryTree
        _userTree.set(key, string::mb_to_utf8(value));

        _changesSinceLastSave++;
    }

    // Notify the observers
    emitSignalForKey(key);
}

void XMLRegistry::import(const std::string& importFilePath, const std::string& parentKey, Tree tree)
{
    std::lock_guard<std::mutex> lock(_writeLock);

    assert(!_shutdown);

    switch (tree)
    {
        case treeUser:
            _userTree.importFromFile(importFilePath, parentKey);
            break;
        case treeStandard:
            _standardTree.importFromFile(importFilePath, parentKey);
            break;
    }

    _changesSinceLastSave++;
}

void XMLRegistry::emitSignalForKey(const std::string& changedKey)
{
    // Do not default-construct a signal, just emit if there is one already
    KeySignals::const_iterator i = _keySignals.find(changedKey);

    if (i != _keySignals.end())
    {
        i->second.emit();
    }
}

void XMLRegistry::loadUserFileFromSettingsPath(const IApplicationContext& ctx,
    const std::string& filename, const std::string& baseXPath)
{
    std::string userSettingsFile = ctx.getSettingsPath() + filename;

    if (os::fileOrDirExists(userSettingsFile))
    {
        try
        {
            import(userSettingsFile, baseXPath, Registry::treeUser);
        }
        catch (const std::exception& e)
        {
            // User files may become corrupted, in which case we should just
            // skip them and move on (as if the user-modified file simply did
            // not exist).
            rError() << "XMLRegistry: user settings file " << filename
                     << " could not be parsed and was skipped (" << e.what() << ")" << std::endl;
        }
    }
    else
    {
        rMessage() << "XMLRegistry: file " << filename << " not present in "
            << ctx.getSettingsPath() << std::endl;
    }
}

// RegisterableModule implementation
const std::string& XMLRegistry::getName() const
{
    static std::string _name(MODULE_XMLREGISTRY);
    return _name;
}

const StringSet& XMLRegistry::getDependencies() const
{
    static StringSet _dependencies; // no dependencies
    return _dependencies;
}

void XMLRegistry::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << "XMLRegistry::initialiseModule called" << std::endl;

    // Load the XML files from the runtime data directory
    std::string base = ctx.getRuntimeDataPath();

    rMessage() << "XMLRegistry: looking for XML files in " << base << std::endl;

    try
    {
        // Load all of the required XML files
        import(base + "user.xml", "", Registry::treeStandard);
        import(base + "colours.xml", "user/ui", Registry::treeStandard);
        import(base + "input.xml", "user/ui", Registry::treeStandard);
        import(base + "menu.xml", "user/ui", Registry::treeStandard);
        import(base + "commandsystem.xml", "user/ui", Registry::treeStandard);

        // Load the debug.xml file only if the relevant key is set in user.xml
        if (get("user/debug") == "1")
        {
            import(base + "debug.xml", "", Registry::treeStandard);
        }
    }
    catch (std::runtime_error& e)
    {
        rConsoleError() << "XML registry population failed:\n\n" << e.what() << std::endl;
    }

    // Load user preferences, these overwrite any values that have defined before

    loadUserFileFromSettingsPath(ctx, "user.xml", "");
    loadUserFileFromSettingsPath(ctx, "colours.xml", "user/ui");
    loadUserFileFromSettingsPath(ctx, "input.xml", "user/ui");
    loadUserFileFromSettingsPath(ctx, "filters.xml", "user/ui/filtersystem");

    // Subscribe to the post-module-shutdown signal to save changes to disk
    module::GlobalModuleRegistry().signal_allModulesUninitialised().connect(
        sigc::mem_fun(this, &XMLRegistry::shutdown));

    _autosaveTimer.reset(new util::Timer(2000,
        sigc::mem_fun(this, &XMLRegistry::onAutoSaveTimerIntervalReached)));

    module::GlobalModuleRegistry().signal_allModulesInitialised().connect([this]()
    {
        _autosaveTimer->start();
    });
}

void XMLRegistry::shutdownModule()
{
    _autosaveTimer->stop();
}

void XMLRegistry::onAutoSaveTimerIntervalReached()
{
    {
        std::lock_guard<std::mutex> lock(_writeLock);

        if (_changesSinceLastSave == 0)
        {
            return;
        }
    }

    rMessage() << "Auto-saving registry to user settings path." << std::endl;

    saveToDisk();
}

// Static module instance
module::StaticModuleRegistration<XMLRegistry> xmlRegistryModule;

}
