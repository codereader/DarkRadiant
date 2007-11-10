#ifndef DOOM3MODELSKIN_H_
#define DOOM3MODELSKIN_H_

#include "modelskin.h"
#include "moduleobservers.h"

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

namespace skins
{

/** 
 * A single instance of a Doom 3 model skin. This structure stores a set of 
 * maps between an existing texture and a new texture, and possibly the name of
 * the model that this skin is associated with.
 */
class Doom3ModelSkin
: public ModelSkin
{
	// Map of texture switches
	typedef std::map<std::string, std::string> StringMap;
	StringMap _remaps;
	
	std::string _name;
	std::string _skinFileName;
	
public:
	Doom3ModelSkin(const std::string& name) :	
		_name(name)
	{}

	std::string getName() const {
		return _name;
	}

	void setSkinFileName(const std::string& fileName) {
		_skinFileName = fileName;
	}
	
	std::string getSkinFileName() const {
		return _skinFileName;
	}

	// Get this skin's remap for the provided material name (if any).
	std::string getRemap(const std::string& name) const {
		StringMap::const_iterator i = _remaps.find(name);
		if(i != _remaps.end()) {
			return i->second;
		}
		else { // none found
			return "";
		}
	}
  
	// Add a remap pair to this skin
	void addRemap(const std::string& src, const std::string& dst) {
		_remaps.insert(StringMap::value_type(src, dst));
	}
  
};
typedef boost::shared_ptr<Doom3ModelSkin> Doom3ModelSkinPtr;


}

#endif /*DOOM3MODELSKIN_H_*/
