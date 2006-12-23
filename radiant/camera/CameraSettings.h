#ifndef CAMERASETTINGS_H_
#define CAMERASETTINGS_H_

#include "iregistry.h"

/* greebo: This is the home of all the camera settings. As this class derives
 * from a RegistryKeyObserver, it can be connected to the according registry keys
 * and gets notified if any of the observed keys are changed.*/

class CameraSettings : public RegistryKeyObserver {
	bool _callbackActive;
	
	int _movementSpeed;
	
	bool _invertMouseVerticalAxis;
	bool _discreteMovement;
	
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
	
private:

	void initDiscreteMovement();

}; // class CameraSettings

CameraSettings* getCameraSettings();

#endif /*CAMERASETTINGS_H_*/
