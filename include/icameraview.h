#pragma once

#include "iinteractiveview.h"
#include "math/Vector3.h"

// Abstract class used when handling mouse events
// see also: class IOrthoView in iorthoview.h
class ICameraView :
    public IInteractiveView
{
public:
    virtual ~ICameraView() {}

	// Move the camera's origin
	virtual Vector3 getCameraOrigin() const = 0;
	virtual void setCameraOrigin(const Vector3& newOrigin) = 0;

	// Returns the vector pointing to the "right"
	virtual Vector3 getRightVector() const = 0;
	// Returns the vector pointing "up"
	virtual Vector3 getUpVector() const = 0;
	// Returns the vector pointing "forward"
	virtual Vector3 getForwardVector() const = 0;

    // Freemove mode
    virtual void enableFreeMove() = 0;
    virtual void disableFreeMove() = 0;
    virtual bool freeMoveEnabled() const = 0;
};
