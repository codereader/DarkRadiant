#pragma once

#include <string>
#include "version.h"
#include "imodule.h"

#include "MajorMinorVersion.h"
#include "os/dir.h"
#include "os/path.h"

namespace settings
{

/**
 * Settings file manager, dealing with saving and loading settings files
 * to the user's settings path. DarkRadiant 2.15+ or higher stores its settings
 * in folders specific to their major and minor versions:
 *
 * $HOME/.config/darkradiant/2.15/
 * $HOME/.config/darkradiant/3.0/
 *
 * When loading settings files, this manager class will search the config folders
 * of the current version and all previous versions, making it possible to use
 * or upgrade settings saved by older versions. The most recent versions will
 * take precedence over older files.
 * It will ignore any files stored in higher or future version folders.
 * For backwards compatibility with DarkRadiant versions that have not been
 * using the version-specific settings folders, files in the base settings path
 * will be considered too, with the lowest priority.
 *
 * When saving settings files, there's only a single target folder, which is
 * the one corresponding to the current application version.
 */
class SettingsManager
{
private:
    const IApplicationContext& _context;
    MajorMinorVersion _currentVersion;

    // The path where this version writes its settings files to
    std::string _currentVersionSettingsFolder;

    // All existing version folders found in the settings folder
    std::set<MajorMinorVersion> _existingVersionFolders;

public:
    // Construct a settings manager for this version of DarkRadiant
    // Will create the settings path for this version if it's not existing yet.
    SettingsManager(const IApplicationContext& context) :
        SettingsManager(context, RADIANT_VERSION)
    {}

    SettingsManager(const SettingsManager& other) = delete;
    SettingsManager& operator=(const SettingsManager& other) = delete;

    // Construct a settings manager instance with a specific version
    // string in the format "Major.Minor.Micro[BuildSuffix]", or "2.14.0pre1".
    // Mainly used for unit test purposes, regular code should use the default constructor.
    SettingsManager(const IApplicationContext& context, const std::string& currentVersion) :
        _context(context),
        _currentVersion(currentVersion)
    {
        // Set up the path to the current version
        _currentVersionSettingsFolder = os::standardPathWithSlash(context.getSettingsPath() + _currentVersion.toString());

        // Make sure the output folder exists
        os::makeDirectory(_currentVersionSettingsFolder);

        // Create an inventory of the existing version folders
        checkExistingVersionFolders();
    }

    // Returns the base path (non-version specific), which is the same as IApplicationContext::getSettingsPath()
    std::string getBaseSettingsPath() const
    {
        return _context.getSettingsPath();
    }

    // Returns the output path (including trailing slash) where all settings files
    // for the current version can be saved to.
    // Matches the pattern IApplicationContext::getSettingsPath()/"major.minor"/
    const std::string& getCurrentVersionSettingsFolder() const
    {
        return _currentVersionSettingsFolder;
    }

    /**
     * Returns the full path to the first matching existing settings file (given by its path
     * relative to the settings folder).
     *
     * This will search any settings folders for the current or any previous
     * application version, returning the first matching file.
     * The version folders are searched in descending order, with the current version having
     * the highest priority. This way the file saved by the most recent known version
     * will be picked, whereas versions saved by "future" application versions will be ignored.
     *
     * For compatibility reasons, files saved in the root settings folder will be
     * considered too (with the lowest priority), as DarkRadiant versions prior to 2.15
     * have not been creating version settings folders.
     *
     * Will return an empty string if no file was matching.
     */
    std::string getExistingSettingsFile(const std::string& relativePath) const
    {
        fs::path settingsPath = _context.getSettingsPath();

        // Go through the existing versions, starting from the highest (usually the current version)
        for (auto v = _existingVersionFolders.rbegin(); v != _existingVersionFolders.rend(); ++v)
        {
            auto fullPath = settingsPath / v->toString() / relativePath;

            if (fs::is_regular_file(fullPath))
            {
                return os::standardPath(fullPath.string()); // found a match
            }
        }

        // Finally, check the base folder if nothing else has been matching
        auto fullPath = settingsPath / relativePath;

        return fs::is_regular_file(fullPath) ? os::standardPath(fullPath.string()) : std::string();
    }

private:
    void checkExistingVersionFolders()
    {
        _existingVersionFolders.clear();

        // Enumerate all existing folders and sort them by version
        os::forEachItemInDirectory(_context.getSettingsPath(), [&](const fs::path& item)
        {
            // Skip non-directories
            if (!fs::is_directory(item)) return;

            try
            {
                // Attempt to parse the version from the folder name and insert into the list of
                // existing versions. Due to operator< they will end up sorted in ascending order
                MajorMinorVersion version(item.filename().string());

                // Ignore all versions higher than the current one
                if (_currentVersion < version) return;

                // Sort this into the set
                _existingVersionFolders.insert(version);
            }
            catch (const std::invalid_argument&)
            {} // ignore invalid folder names
        });
    }
};

}
