#ifndef CAMERASETTINGS_H_
#define CAMERASETTINGS_H_

#include "iregistry.h"

/* greebo: This is the home of all the camera settings. As this class derives
 * from a RegistryKeyObserver, it can be connected to the according registry keys
 * and gets notified if any of the observed keys are changed.*/

namespace {
	const int MAX_CUBIC_SCALE = 23;
}

class CameraSettings : public RegistryKeyObserver {
	bool _callbackActive;
	
	int _movementSpeed;
	
	bool _invertMouseVerticalAxis;
	bool _discreteMovement;
	
	int _cubicScale;
	
public:
	CameraSettings();

	// The callback that gets called on registry key changes
	void keyChanged();
	
	int getMovementSpeed() const {
		return _movementSpeed;
	}
	
	bool invertMouseVerticalAxis() const {
		return _invertMouseVerticalAxis;
	}
	
	bool discreteMovement() const {
		return _discreteMovement;
	}
	
	// Gets/Sets the cubic scale member variable (is automatically constrained [1..MAX_CUBIC_SCALE])
	int& getCubicScale();
	void setCubicScale(const int& scale);
	
private:

	void initDiscreteMovement();

}; // class CameraSettings

CameraSettings* getCameraSettings();

#endif /*CAMERASETTINGS_H_*/
