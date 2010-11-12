#ifndef CAMERAOBSERVER_H_
#define CAMERAOBSERVER_H_

#include <list>

class CameraObserver
{
public:
    // destructor
	virtual ~CameraObserver() {}
	// This gets called as soon as the camera is moved
	virtual void cameraMoved() = 0;

}; // class CameraObserver

typedef std::list<CameraObserver*> CameraObserverList;

#endif /*CAMERAOBSERVER_H_*/
