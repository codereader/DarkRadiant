#ifndef VECTORLIGHTLIST_H_
#define VECTORLIGHTLIST_H_

#include "irender.h"
#include "generic/callback.h"

namespace model {

/**
 * Implementation of the LightList interface using a std::vector of pointers.
 */
class VectorLightList 
: public LightList
{
	// Vector of lights
	typedef std::vector<const RendererLight*> Lights;
	Lights _lights;
	
public:

	void addLight(const RendererLight& light) {
    	_lights.push_back(&light);
	}
	
	void clear() {
		_lights.clear();
	}
	
	void evaluateLights() const { }

	void lightsChanged() const { }

	void forEachLight(const RendererLightCallback& callback) const {
		for (Lights::const_iterator i = _lights.begin(); 
			 i != _lights.end(); 
			 ++i)
		{
			callback(*(*i));
		}
	}
};

} // namespace model

#endif /*VECTORLIGHTLIST_H_*/
