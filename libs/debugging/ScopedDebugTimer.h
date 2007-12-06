#ifndef SCOPEDDEBUGTIMER_H_
#define SCOPEDDEBUGTIMER_H_

#include <sys/time.h>
#include <string>
#include "stream/textfilestream.h"
#include "stream/stringstream.h"

namespace {
	
	const double MILLION = 1000000.0;

	/**
	 * Operator- for timeval structures.
	 * 
	 * @returns
	 * Double-precision float representing the difference in seconds between
	 * the two times.
	 */
	double operator-(const timeval& l, const timeval& r) {
		
		// Convert timevals to double
		double dl = (double) l.tv_sec + ((double) l.tv_usec / MILLION);
		double dr = (double) r.tv_sec + ((double) r.tv_usec / MILLION);
		
		return dl - dr;
	}
}

/**
 * Debugging class to time a particular event. The clock is saved during
 * construction, and the time difference calculated at destruction.
 */
class ScopedDebugTimer
{
private:
	
	// Start time
	timeval _s;
	
	// Name of operation
	std::string _op;
	
	// Show FPS?
	bool _fps;
	
public:
	
	/**
	 * Constructor. Set the name of the operation to be printed out on 
	 * destruction.
	 * 
	 * @param name
	 * The name of the operation.
	 * 
	 * @param showFps
	 * If true, a nominal FPS value will be calculated for the given operation
	 * time.
	 */
	ScopedDebugTimer(const std::string& name, bool showFps = false)
	: _op(name), _fps(showFps)
	{
		// Save start time
		gettimeofday(&_s, NULL);
	}
	
	/**
	 * Destructor. Prints out the time of the operation.
	 */
	~ScopedDebugTimer() {
		
		// Get the current time
		timeval end;
		gettimeofday(&end, NULL);
		
		// Calculate duration
		double duration = end - _s;
		
		std::cout << "[ScopedDebugTimer] \"" << _op 
				  << "\" in " << duration << " seconds.";
		globalOutputStream() << "[ScopedDebugTimer] \"" << _op.c_str() 
		  					 << "\" in " << duration << " seconds.";
		if (_fps) {
			std::cout << " (" << (1.0 / duration) << " FPS)";
			globalOutputStream() << " (" << (1.0 / duration) << " FPS)";
		}
		std::cout << std::endl;
	}
};

#endif /*SCOPEDDEBUGTIMER_H_*/
