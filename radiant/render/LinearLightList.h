#ifndef LINEARLIGHTLIST_H_
#define LINEARLIGHTLIST_H_

#include "irender.h"
#include <list>
#include "generic/callback.h"

namespace render {

typedef std::set<RendererLight*> RendererLights;

class LinearLightList : 
	public LightList
{
	LightCullable& m_cullable;
	RendererLights& m_allLights;
	Callback m_evaluateChanged;

	typedef std::list<RendererLight*> Lights;
	mutable Lights m_lights;
	mutable bool m_lightsChanged;
public:
	
	LinearLightList(LightCullable& cullable, 
					RendererLights& lights,
					const Callback& evaluateChanged) :
		m_cullable(cullable), 
		m_allLights(lights),
		m_evaluateChanged(evaluateChanged)
	{
		m_lightsChanged = true;
	}
	
	void evaluateLights() const {
		m_evaluateChanged();
		
		if (m_lightsChanged) {
			m_lightsChanged = false;

			m_lights.clear();
			m_cullable.clearLights();
			
			for (RendererLights::const_iterator i = m_allLights.begin(); 
				 i != m_allLights.end(); ++i)
			{
				if (lightEnabled(*(*i), m_cullable)) {
					m_lights.push_back(*i);
					m_cullable.insertLight(*(*i));
				}
			}
		}
	}
	
	void forEachLight(const RendererLightCallback& callback) const {
		evaluateLights();

		for (Lights::const_iterator i = m_lights.begin(); i != m_lights.end(); ++i) {
			callback(*(*i));
		}
	}
	
	void lightsChanged() const {
		m_lightsChanged = true;
	}
	
private:
	inline bool lightEnabled(const RendererLight& light, const LightCullable& cullable) const {
		return cullable.testLight(light);
	}
};

} // namespace render

#endif /*LINEARLIGHTLIST_H_*/
