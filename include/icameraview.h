#pragma once

#include "imodule.h"
#include "iinteractiveview.h"
#include "math/Vector3.h"

// Abstract class used when handling mouse events
// see also: class IOrthoView in iorthoview.h
class ICameraView :
	public virtual IInteractiveView
{
public:
	typedef std::shared_ptr<ICameraView> Ptr;

	virtual ~ICameraView() {}

	// Move the camera's origin
	virtual const Vector3& getCameraOrigin() const = 0;
	virtual void setCameraOrigin(const Vector3& newOrigin) = 0;

	virtual const Vector3& getCameraAngles() const = 0;
	virtual void setCameraAngles(const Vector3& newAngles) = 0;

	// Returns the vector pointing to the "right"
	virtual const Vector3& getRightVector() const = 0;
	// Returns the vector pointing "up"
	virtual const Vector3& getUpVector() const = 0;
	// Returns the vector pointing "forward"
	virtual const Vector3& getForwardVector() const = 0;
};

class IFreeMoveView :
	public virtual IInteractiveView
{
public:
	virtual ~IFreeMoveView() {}

    // Freemove mode
    virtual void enableFreeMove() = 0;
    virtual void disableFreeMove() = 0;
    virtual bool freeMoveEnabled() const = 0;
};

class ICameraViewManager :
	public RegisterableModule
{
public:
	virtual ~ICameraViewManager() {}


};
