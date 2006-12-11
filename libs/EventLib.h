#ifndef EVENTLIB_H_
#define EVENTLIB_H_

namespace ui {

	/* greebo: Below are the actual events that are "read" by the views/observers to 
	 * interpret the mouseclicks. */
	
	// The possible modes when in "component manipulation mode"
	enum XYViewEvent {
		xyNothing,		// unrecognised event
		xyMoveView,		// drag the view around
		xySelect,		// selection / clip
		xyZoom,			// drag-zoom operator
		xyCameraMove,	// the button used to drag the camera around 
		xyCameraAngle,	// the button used to change camera angle
		xyNewBrushDrag,	// used to create new brushes
	};
	
	// These are the buttons for the camera view
	enum CamViewEvent {
		camNothing,				// nothing special, event can be passed to the windowobservers
		camEnableFreeLookMode,	// used to enable the free look mode in the camera view
		camDisableFreeLookMode,	// used to disable the free look mode in the camera view
	};
	
	// If the click is passed to the windowobservers, these are the possibilites
	enum ObserverEvent {
		obsNothing,		// any uninterpreted/unsupported combination
		obsManipulate,	// manipulate an object by drag or click
		obsSelect,		// selection toggle 
		obsToggle,		// selection toggle
		obsToggleFace,	// selection toggle (face)
		obsReplace,		// replace/cycle selection through possible candidates
		obsReplaceFace,	// replace/cycle selection through possible face candidates
		obsCopyTexture,	// copy texture from object
		obsPasteTexture,	// paste texture to object
	};

} // namespace ui

#endif /*EVENTLIB_H_*/
