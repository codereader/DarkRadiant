#ifndef COUNTERMANAGER_H_
#define COUNTERMANAGER_H_

#include "iradiant.h"
#include "scenelib.h"
#include "generic/callback.h"
#include "qe3.h"
#include <boost/shared_ptr.hpp>

namespace map {

class SimpleCounter : 
	public Counter
{
	Callback m_countChanged;
	std::size_t m_count;
public:
	SimpleCounter() : 
		m_count(0)
	{}
	
	void setCountChangedCallback(const Callback& countChanged) {
		m_countChanged = countChanged;
	}
	
	void increment() {
		++m_count;
		m_countChanged();
	}
	
	void decrement() {
		--m_count;
		m_countChanged();
	}
	
	std::size_t get() const {
		return m_count;
	}
};

class CounterManager
{
	typedef boost::shared_ptr<SimpleCounter> CounterPtr;
	typedef std::map<CounterType, CounterPtr> CounterMap;
	CounterMap _counters;
public:
	CounterManager() {
		_counters[counterBrushes] = CounterPtr(new SimpleCounter);
		_counters[counterPatches] = CounterPtr(new SimpleCounter);
		_counters[counterEntities] = CounterPtr(new SimpleCounter);
		
		_counters[counterBrushes]->setCountChangedCallback(
			MemberCaller<CounterManager, &CounterManager::brushCountChanged>(*this)
		);
		_counters[counterPatches]->setCountChangedCallback(
			MemberCaller<CounterManager, &CounterManager::patchCountChanged>(*this)
		);
		_counters[counterEntities]->setCountChangedCallback(
			MemberCaller<CounterManager, &CounterManager::entityCountChanged>(*this)
		);
	}

	Counter& get(CounterType counter) {
		if (_counters.find(counter) == _counters.end()) {
			throw std::runtime_error("Counter ID not found.");
		}
		return *_counters[counter];
	}
	
	void brushCountChanged() {
		std::cout << "Brush count changed.\n";
		//g_numbrushes = ;
		updateStatusBar();
	}
	
	void patchCountChanged() {
		std::cout << "Patch count changed.\n";
		//g_numbrushes = int(_counters[counterPatches]->get());
		updateStatusBar();
	}
	
	void entityCountChanged() {
		std::cout << "Entity count changed.\n";
		updateStatusBar();
	}
	
	void updateStatusBar() {
		int brushCount(_counters[counterBrushes]->get());
		int patchCount(_counters[counterPatches]->get());
		int entityCount(_counters[counterEntities]->get());
		std::string text = "Brushes: " + intToStr(brushCount);
		text += " Patches: " + intToStr(patchCount);
		text += " Entities: " + intToStr(entityCount);
		
		g_pParentWnd->SetStatusText(g_pParentWnd->m_brushcount_status, text);
	}
};

} // namespace map

#endif /*COUNTERMANAGER_H_*/
