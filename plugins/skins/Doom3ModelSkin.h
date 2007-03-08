#ifndef DOOM3MODELSKIN_H_
#define DOOM3MODELSKIN_H_

#include "modelskin.h"
#include "moduleobservers.h"

#include <string>
#include <map>

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
	
	// Associated model
	std::string _model;
	
	// List of observers
	ModuleObservers _observers;
	
public:

	// Constructor
	Doom3ModelSkin()
	: _model("")
	{}

	void attach(ModuleObserver& observer) {
		_observers.attach(observer);
		observer.realise();
	}
	
	void detach(ModuleObserver& observer) {
		observer.unrealise();
		_observers.detach(observer);
	}
	
	bool realised() const {
		return true;
	}

	// Get this skin's remap for the provided material name (if any).
	const char* getRemap(const std::string& name) const {
		StringMap::const_iterator i = _remaps.find(name);
		if(i != _remaps.end()) {
			return i->second.c_str();
		}
		else { // none found
			return "";
		}
	}
  
	// Return the model associated with this skin.
	std::string getModel() {
		return _model;
	}
	
	// Set the model associated with this skin.
	void setModel(const std::string& model) {
		_model = model;
	}
	
	// Add a remap pair to this skin
	void addRemap(const std::string& src, const std::string& dst) {
		_remaps.insert(StringMap::value_type(src, dst));
	}
  
};


}

#endif /*DOOM3MODELSKIN_H_*/
