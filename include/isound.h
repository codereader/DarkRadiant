#ifndef ISOUND_H_
#define ISOUND_H_

#include "imodule.h"
#include "ModResource.h"

#include <vector>
#include <boost/function.hpp>

// A list of sound files associated to a shader
typedef std::vector<std::string> SoundFileList;

const float METERS_PER_UNIT = 0.0254f;
const float UNITS_PER_METER = 1/METERS_PER_UNIT;

// The min and max radii of a sound shader
class SoundRadii {
    float minRad, maxRad;
    public:
    //set sound radii either in metres or in inch on initialization might cause a conversion
    SoundRadii (float min = 0, float max = 0, bool inMetres = false) {
        if (inMetres) {
            minRad = min * UNITS_PER_METER;
            maxRad = max * UNITS_PER_METER;
        }
        else {
            minRad = min;
            maxRad = max;
        }
    }

    // set the sound radii in metres or in inch, might cause a conversion
    void setMin(float min, bool inMetres = false) {
        if (inMetres) {
            minRad = min * UNITS_PER_METER;
        }
        else {
            minRad = min;
        }
    }

    void setMax (float max, bool inMetres = false) {
        if (inMetres) {
            maxRad = max * UNITS_PER_METER;
        }
        else {
            maxRad = max;
        }
    }

    float getMin(bool inMetres = false) const {
        return (inMetres) ? minRad * METERS_PER_UNIT : minRad;
    }

    float getMax(bool inMetres = false) const {
        return (inMetres) ? maxRad * METERS_PER_UNIT : maxRad;
    }
};

/// Representation of a single sound or sound shader.
class ISoundShader :
    public ModResource
{
public:

    /// Get the name of the shader
    virtual std::string getName() const = 0;

    /// Get the min and max radii of the shader
    virtual SoundRadii getRadii() const = 0;

    /// Get the list of sound files associated to this shader
    virtual SoundFileList getSoundFileList() const = 0;

	// angua: get the display folder for sorting the sounds in the sound chooser window
	virtual const std::string& getDisplayFolder() const = 0;

};
typedef boost::shared_ptr<ISoundShader> ISoundShaderPtr;

const std::string MODULE_SOUNDMANAGER("SoundManager");

/// Sound manager interface.
class ISoundManager :
    public RegisterableModule
{
public:

    /// Invoke a function for each sound shader
    virtual void forEachShader(boost::function<void(const ISoundShader&)>)
    const = 0;

    /** greebo: Tries to lookup the SoundShader with the given name,
     *          returns a soundshader with an empty name, if the lookup failed.
     */
    virtual ISoundShaderPtr getSoundShader(const std::string& shaderName) = 0;

    /** greebo: Plays the given sound file (defined by its VFS path).
     *
     * @returns: TRUE, if the sound file was found at the given VFS path,
     *           FALSE otherwise
     */
    virtual bool playSound(const std::string& fileName) = 0;

    /** greebo: Stops the currently played sound.
     */
    virtual void stopSound() = 0;
};

// Accessor method
inline ISoundManager& GlobalSoundManager() {
    // Cache the reference locally
    static ISoundManager& _soundManager(
        *boost::static_pointer_cast<ISoundManager>(
            module::GlobalModuleRegistry().getModule(MODULE_SOUNDMANAGER)
        )
    );
    return _soundManager;
}

#endif /*ISOUND_H_*/
