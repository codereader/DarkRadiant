#pragma once

#include <wx/stopwatch.h>
#include "string/string.h"

namespace render
{

class RenderStatistics
{
	std::string _statStr;

	std::size_t _countPrims;
	std::size_t _countStates;
	std::size_t _countTransforms;

	wxStopWatch _timer;
public:
	const std::string& getStatString()
    {
        _statStr = "prims: " + string::to_string(_countPrims) +
				  " | states: " + string::to_string(_countStates) +
				  " | transforms: "	+ string::to_string(_countTransforms) +
				  " | msec: " + string::to_string(_timer.Time());

		return _statStr;
	}

	void resetStats() 
    {
		_countPrims = 0;
		_countStates = 0;
		_countTransforms = 0;

		_timer.Start();
	}

	static RenderStatistics& Instance()
    {
		static RenderStatistics _instance;
		return _instance;
	}
};

} // namespace render
