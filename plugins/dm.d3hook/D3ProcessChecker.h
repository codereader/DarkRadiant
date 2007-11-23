#ifndef D3PROCESSCHECKER_H_
#define D3PROCESSCHECKER_H_

class D3ProcessChecker {
public:
	// Success flag
	static bool processFound;
		
	// Platform-dependent process check routine (searches for gamex86.dll)
	static bool D3IsRunning();
};

#endif /*D3PROCESSCHECKER_H_*/
