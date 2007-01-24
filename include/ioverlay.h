#ifndef IOVERLAY_H_
#define IOVERLAY_H_

#include "generic/constant.h"
#include <string>

/**
 * Abstract base class for the background image overlay module 
 */
class IOverlay {
	
public:
	INTEGER_CONSTANT(Version, 1);
	STRING_CONSTANT(Name, "overlay");

	virtual void show(bool shown) = 0;

	// Saves the specified node and all its children into the file <filename>
	virtual void setImage(const std::string& imageName) = 0;
	
	// Sets the image transparency (0 = completely transparent, 1 = opaque)
	virtual void setTransparency(const float& transparency) = 0;
	
	virtual void setImageScale(const float& scale) = 0;
	virtual void setImagePosition(const unsigned int& x, const unsigned int& y) = 0;
};

// Module definitions

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<IOverlay> GlobalOverlayModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<IOverlay> GlobalOverlayModuleRef;

// This is the accessor for the overlay module
inline IOverlay& GlobalOverlay() {
	return GlobalOverlayModule::getTable();
}

#endif /*IOVERLAY_H_*/
