#ifndef RENDERSTATISTICS_H_
#define RENDERSTATISTICS_H_

namespace render {

#include "timer.h"
#include "string/string.h"

class RenderStatistics {
	std::string _statStr;
	
	std::size_t _countPrims;
	std::size_t _countStates;
	std::size_t _countTransforms;
	
	Timer _timer;
public:
	const std::string& getStatString() {
		_statStr.clear();
		_statStr = "prims: " + sizetToStr(_countPrims) +
				  " | states: " + sizetToStr(_countStates) +
				  " | transforms: "	+ sizetToStr(_countTransforms) + 
				  " | msec: " + intToStr(_timer.elapsed_msec());
		return _statStr;
	}
	
	void resetStats() {
		_countPrims = 0;
		_countStates = 0;
		_countTransforms = 0;
		_timer.start();
	}
	
	static RenderStatistics& Instance() {
		static RenderStatistics _instance;
		return _instance;
	}
};

} // namespace render

#endif /*RENDERSTATISTICS_H_*/
