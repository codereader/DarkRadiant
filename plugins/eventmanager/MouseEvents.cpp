#include "MouseEvents.h"

#include "iregistry.h"
#include "iradiant.h"
#include "iselection.h"

#include "stream/textstream.h"

#include "gdk/gdkkeys.h"

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
		
		const float DEFAULT_STRAFE_SPEED = 0.65f;
		const int DEFAULT_MIN_SELECTION_COUNT = -1;
	}

MouseEventManager::MouseEventManager(Modifiers& modifiers) : 
	_modifiers(modifiers),
	_selectionSystem(NULL),
	_activeFlags(0)
{}

void MouseEventManager::initialise() {
	loadButtonDefinitions();
		
	loadXYViewEventDefinitions();
	loadObserverEventDefinitions();
	
	loadCameraEventDefinitions();
	loadCameraStrafeDefinitions();
}

void MouseEventManager::connectSelectionSystem(SelectionSystem* selectionSystem) {
	_selectionSystem = selectionSystem;
}

unsigned int MouseEventManager::getButtonId(const std::string& buttonName) {
	ButtonIdMap::iterator it = _buttonId.find(buttonName);
   	if (it != _buttonId.end()) {
   		return it->second;
   	}
   	else {
   		globalOutputStream() << "MouseEventManager: Warning: Button " << buttonName.c_str() << " not found, returning ID=0\n";
   		return 0;
   	}
}

MouseEventManager::ConditionStruc MouseEventManager::getCondition(xml::Node node) {
	const std::string button = node.getAttributeValue("button");
	const std::string modifiers = node.getAttributeValue("modifiers");
	const std::string minSelectionCount = node.getAttributeValue("minSelectionCount");
	
	ConditionStruc returnValue;
	
	returnValue.buttonId = getButtonId(button);
	returnValue.modifierFlags = _modifiers.getModifierFlags(modifiers);
	
	try {
		returnValue.minSelectionCount = boost::lexical_cast<int>(minSelectionCount);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue.minSelectionCount = DEFAULT_MIN_SELECTION_COUNT;
	}
		
	return returnValue;
}

void MouseEventManager::loadCameraStrafeDefinitions() {
	// Find all the camera strafe definitions
	xml::NodeList strafeList = GlobalRegistry().findXPath("user/ui/input/cameraview/strafemode");
	
	if (strafeList.size() > 0) {
		// Get the strafe condition flags
		_toggleStrafeCondition.modifierFlags = _modifiers.getModifierFlags(strafeList[0].getAttributeValue("toggle"));
		_toggleForwardStrafeCondition.modifierFlags = _modifiers.getModifierFlags(strafeList[0].getAttributeValue("forward"));
		
		try {
			_strafeSpeed = boost::lexical_cast<float>(strafeList[0].getAttributeValue("speed"));
		}
		catch (boost::bad_lexical_cast e) {
			_strafeSpeed = DEFAULT_STRAFE_SPEED;
		}
		
		try {
			_forwardStrafeFactor = boost::lexical_cast<float>(strafeList[0].getAttributeValue("forwardFactor"));
		}
		catch (boost::bad_lexical_cast e) {
			_forwardStrafeFactor = 1.0f;
		}
	}
	else {
		// No Camera strafe definitions found!
		globalOutputStream() << "MouseEventManager: Critical: No camera strafe definitions found!\n";
	}
}

void MouseEventManager::loadCameraEventDefinitions() {
	xml::NodeList camviews = GlobalRegistry().findXPath("user/ui/input//cameraview");
	
	if (camviews.size() > 0) {
	
		// Find all the camera definitions
		xml::NodeList eventList = camviews[0].getNamedChildren("event");
		
		if (eventList.size() > 0) {
			globalOutputStream() << "MouseEventManager: Camera Definitions found: " 
								 << static_cast<int>(eventList.size()) << "\n";
			for (unsigned int i = 0; i < eventList.size(); i++) {
				// Get the event name
				const std::string eventName = eventList[i].getAttributeValue("name");
				
				// Check if any recognised event names are found and construct the according condition.
				if (eventName == "EnableFreeLookMode") {
					_cameraConditions[ui::camEnableFreeLookMode] = getCondition(eventList[i]);
				}
				else if (eventName == "DisableFreeLookMode") {
					_cameraConditions[ui::camDisableFreeLookMode] = getCondition(eventList[i]);
				}
				else {
					globalOutputStream() << "MouseEventManager: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
				}
			}
		}
		else {
			// No Camera definitions found!
			globalOutputStream() << "MouseEventManager: Critical: No camera event definitions found!\n";
		}
	}
	else {
		// No Camera definitions found!
		globalOutputStream() << "MouseEventManager: Critical: No camera event definitions found!\n";
	}
}

void MouseEventManager::loadObserverEventDefinitions() {
	xml::NodeList observers = GlobalRegistry().findXPath("user/ui/input//observer");
	
	if (observers.size() > 0) {
	
		// Find all the observer definitions
		xml::NodeList eventList = observers[0].getNamedChildren("event");
		
		if (eventList.size() > 0) {
			globalOutputStream() << "MouseEventManager: Observer Definitions found: " 
								 << static_cast<int>(eventList.size()) << "\n";
			for (unsigned int i = 0; i < eventList.size(); i++) {
				// Get the event name
				const std::string eventName = eventList[i].getAttributeValue("name");
				
				// Check if any recognised event names are found and construct the according condition.
				if (eventName == "Manipulate") {
					_observerConditions[ui::obsManipulate] = getCondition(eventList[i]);
				}
				else if (eventName == "Select") {
					_observerConditions[ui::obsSelect] = getCondition(eventList[i]);
				}
				else if (eventName == "ToggleSelection") {
					_observerConditions[ui::obsToggle] = getCondition(eventList[i]);
				}
				else if (eventName == "ToggleFaceSelection") {
					_observerConditions[ui::obsToggleFace] = getCondition(eventList[i]);
				}
				else if (eventName == "CycleSelection") {
					_observerConditions[ui::obsReplace] = getCondition(eventList[i]);
				}
				else if (eventName == "CycleFaceSelection") {
					_observerConditions[ui::obsReplaceFace] = getCondition(eventList[i]);
				}
				else if (eventName == "CopyTexture") {
					_observerConditions[ui::obsCopyTexture] = getCondition(eventList[i]);
				}
				else if (eventName == "PasteTextureProjected") {
					_observerConditions[ui::obsPasteTextureProjected] = getCondition(eventList[i]);
				}
				else if (eventName == "PasteTextureNatural") {
					_observerConditions[ui::obsPasteTextureNatural] = getCondition(eventList[i]);
				}
				else if (eventName == "PasteTextureCoordinates") {
					_observerConditions[ui::obsPasteTextureCoordinates] = getCondition(eventList[i]);
				}
				else if (eventName == "PasteTextureToBrush") {
					_observerConditions[ui::obsPasteTextureToBrush] = getCondition(eventList[i]);
				}
				else if (eventName == "JumpToObject") {
					_observerConditions[ui::obsJumpToObject] = getCondition(eventList[i]);
				}
				else {
					globalOutputStream() << "MouseEventManager: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
				}
			}
		}
		else {
			// No observer definitions found!
			globalOutputStream() << "MouseEventManager: Critical: No observer event definitions found!\n";
		}
	}
	else {
		// No observer definitions found!
		globalOutputStream() << "MouseEventManager: Critical: No observer event definitions found!\n";
	}
}

void MouseEventManager::loadXYViewEventDefinitions() {
	
	xml::NodeList xyviews = GlobalRegistry().findXPath("user/ui/input//xyview");
	
	if (xyviews.size() > 0) {
	
		// Find all the xy view definitions
		xml::NodeList eventList = xyviews[0].getNamedChildren("event");
	
		if (eventList.size() > 0) {
			globalOutputStream() << "MouseEventManager: XYView Definitions found: " 
								 << static_cast<int>(eventList.size()) << "\n";
			for (unsigned int i = 0; i < eventList.size(); i++) {
				// Get the event name
				const std::string eventName = eventList[i].getAttributeValue("name");
				
				// Check if any recognised event names are found and construct the according condition.
				if (eventName == "MoveView") {
					_xyConditions[ui::xyMoveView] = getCondition(eventList[i]);
				}
				else if (eventName == "Select") {
					_xyConditions[ui::xySelect] = getCondition(eventList[i]);
				}
				else if (eventName == "Zoom") {
					_xyConditions[ui::xyZoom] = getCondition(eventList[i]);
				}
				else if (eventName == "CameraMove") {
					_xyConditions[ui::xyCameraMove] = getCondition(eventList[i]);
				}
				else if (eventName == "CameraAngle") {
					_xyConditions[ui::xyCameraAngle] = getCondition(eventList[i]);
				}
				else if (eventName == "NewBrushDrag") {
					_xyConditions[ui::xyNewBrushDrag] = getCondition(eventList[i]);
				}
				else {
					globalOutputStream() << "MouseEventManager: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
				}
			}
		}
		else {
			// No event definitions found!
			globalOutputStream() << "MouseEventManager: Critical: No XYView event definitions found!\n";
		}
	}
	else {
		// No event definitions found!
		globalOutputStream() << "MouseEventManager: Critical: No XYView event definitions found!\n";
	}
}

void MouseEventManager::loadButtonDefinitions() {
	xml::NodeList buttons = GlobalRegistry().findXPath("user/ui/input//buttons");
	
	if (buttons.size() > 0) {
	
		// Find all the button definitions
		xml::NodeList buttonList = buttons[0].getNamedChildren("button");
	
		if (buttonList.size() > 0) {
			globalOutputStream() << "MouseEventManager: Buttons found: " 
								 << static_cast<int>(buttonList.size()) << "\n";
			for (unsigned int i = 0; i < buttonList.size(); i++) {
				const std::string name = buttonList[i].getAttributeValue("name");
				
				unsigned int id;
				try {
					id = boost::lexical_cast<unsigned int>(buttonList[i].getAttributeValue("id"));
				}
				catch (boost::bad_lexical_cast e) {
					id = 0;
				}
				
				if (name != "" && id > 0) {
					//std::cout << "MouseEventManager: Found button definition " << name.c_str() << " with ID " << id << "\n";
					
					// Save the button ID into the map
					_buttonId[name] = id;
				} 
				else {
					globalOutputStream() << "MouseEventManager: Warning: Invalid button definition found.\n";
				}
			}
		}
		else {
			// No Button definitions found!
			globalOutputStream() << "MouseEventManager: Critical: No button definitions found!\n";
		}
	}
	else {
		// No Button definitions found!
		globalOutputStream() << "MouseEventManager: Critical: No button definitions found!\n";
	}
}

// Retrieves the button from an GdkEventMotion state 
unsigned int MouseEventManager::getButtonFlags(const unsigned int& state) {
	if ((state & GDK_BUTTON1_MASK) != 0) return 1;
	if ((state & GDK_BUTTON2_MASK) != 0) return 2;
	if ((state & GDK_BUTTON3_MASK) != 0) return 3;
	if ((state & GDK_BUTTON4_MASK) != 0) return 4;
	if ((state & GDK_BUTTON5_MASK) != 0) return 5;

	return 0;
}

ui::CamViewEvent MouseEventManager::findCameraViewEvent(const unsigned int& button, const unsigned int& modifierFlags) {
	
	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return ui::camNothing;
	}
	
	for (CameraConditionMap::iterator it = _cameraConditions.begin(); it != _cameraConditions.end(); it++) {
		ui::CamViewEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId && modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return ui::camNothing;
}

ui::XYViewEvent MouseEventManager::findXYViewEvent(const unsigned int& button, const unsigned int& modifierFlags) {
	
	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return ui::xyNothing;
	}
	
	for (XYConditionMap::iterator it = _xyConditions.begin(); it != _xyConditions.end(); it++) {
		ui::XYViewEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId 
			&& modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return ui::xyNothing;
}

ui::ObserverEvent MouseEventManager::findObserverEvent(const unsigned int& button, const unsigned int& modifierFlags) {

	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return ui::obsNothing;
	}
	
	for (ObserverConditionMap::iterator it = _observerConditions.begin(); it != _observerConditions.end(); it++) {
		ui::ObserverEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId 
			&& modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return ui::obsNothing;
}

ui::CamViewEvent MouseEventManager::getCameraViewEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = _modifiers.getKeyboardFlags(event->state);
	
	return findCameraViewEvent(button, modifierFlags);
}

ui::XYViewEvent MouseEventManager::getXYViewEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = _modifiers.getKeyboardFlags(event->state);
	
	return findXYViewEvent(button, modifierFlags);
}

// The same as above, just with a state as argument rather than a GdkEventButton
ui::XYViewEvent MouseEventManager::getXYViewEvent(const unsigned int& state) {
	unsigned int button = getButtonFlags(state);
	unsigned int modifierFlags = _modifiers.getKeyboardFlags(state);
	
	return findXYViewEvent(button, modifierFlags);
}

bool MouseEventManager::matchXYViewEvent(const ui::XYViewEvent& xyViewEvent, 
										 const unsigned int& button, 
										 const unsigned int& modifierFlags) 
{
	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return false;
	}
	
	XYConditionMap::iterator it = _xyConditions.find(xyViewEvent);
   	if (it != _xyConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
   		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "MouseEventManager: Warning: Query for event " << xyViewEvent << ": not found.\n";
   		return false;
   	}
}

bool MouseEventManager::matchObserverEvent(const ui::ObserverEvent& observerEvent, 
										   const unsigned int& button, 
										   const unsigned int& modifierFlags) 
{
	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return false;
	}
	
	ObserverConditionMap::iterator it = _observerConditions.find(observerEvent);
   	if (it != _observerConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "MouseEventManager: Warning: Query for event " << observerEvent << ": not found.\n";
   		return false;
   	}
}

bool MouseEventManager::matchCameraViewEvent(const ui::CamViewEvent& camViewEvent, 
											 const unsigned int& button, 
											 const unsigned int& modifierFlags) 
{
	if (_selectionSystem == NULL) {
		globalErrorStream() << "MouseEventManager: No connection to SelectionSystem\n";
		return false;
	}
	
	CameraConditionMap::iterator it = _cameraConditions.find(camViewEvent);
   	if (it != _cameraConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
   		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(_selectionSystem->countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "MouseEventManager: Warning: Query for event " << camViewEvent << ": not found.\n";
   		return false;
   	}
}

bool MouseEventManager::stateMatchesXYViewEvent(const ui::XYViewEvent& xyViewEvent, GdkEventButton* event) {
	return matchXYViewEvent(xyViewEvent, event->button, _modifiers.getKeyboardFlags(event->state));
}

// The same as above, just with a state as argument rather than a GdkEventButton
bool MouseEventManager::stateMatchesXYViewEvent(const ui::XYViewEvent& xyViewEvent, const unsigned int& state) {
	return matchXYViewEvent(xyViewEvent, getButtonFlags(state), _modifiers.getKeyboardFlags(state));
}

bool MouseEventManager::stateMatchesObserverEvent(const ui::ObserverEvent& observerEvent, GdkEventButton* event) {
	return matchObserverEvent(observerEvent, event->button, _modifiers.getKeyboardFlags(event->state));
}

bool MouseEventManager::stateMatchesCameraViewEvent(const ui::CamViewEvent& camViewEvent, GdkEventButton* event) {
	return matchCameraViewEvent(camViewEvent, event->button, _modifiers.getKeyboardFlags(event->state));
}

ui::ObserverEvent MouseEventManager::getObserverEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = _modifiers.getKeyboardFlags(event->state);
	
	return findObserverEvent(button, modifierFlags);
}

ui::ObserverEvent MouseEventManager::getObserverEvent(const unsigned int& state) {
	unsigned int button = getButtonFlags(state);
	unsigned int modifierFlags = _modifiers.getKeyboardFlags(state);
	
	return findObserverEvent(button, modifierFlags);
}

std::string MouseEventManager::printXYViewEvent(const ui::XYViewEvent& xyViewEvent) {
	
	switch (xyViewEvent) {
		case ui::xyNothing: return "<b>Nothing</b>";
		case ui::xyMoveView: return "<b>Move View</b>";
		case ui::xySelect: return "<b>Select</b>";
		case ui::xyZoom: return "<b>Zoom View</b>";
		case ui::xyCameraMove: return "<b>Drag Camera</b>";
		case ui::xyCameraAngle: return "<b>Point Camera</b>";
		case ui::xyNewBrushDrag: return "<b>Create New Brush</b>";
		default: return "<b>Unknown event</b>";
	}
}

std::string MouseEventManager::printObserverEvent(const ui::ObserverEvent& observerEvent) {
	
	switch (observerEvent) {
		case ui::obsNothing: return "<b>Nothing</b>";
		case ui::obsManipulate: return "<b>Manipulate</b>";
		case ui::obsSelect: return "<b>Select</b>"; 
		case ui::obsToggle: return "<b>Toggle Selection</b>";
		case ui::obsToggleFace: return "<b>Toggle Face Selection</b>";
		case ui::obsReplace: return "<b>Cycle Selection</b>";
		case ui::obsReplaceFace: return "<b>Cycle Face Selection</b>";
		case ui::obsCopyTexture: return "<b>Copy Texture</b>";
		case ui::obsPasteTextureProjected: return "<b>PasteTexture Projected</b>";		
		case ui::obsPasteTextureNatural: return "<b>PasteTexture Natural</b>";
		case ui::obsPasteTextureCoordinates: return "<b>PasteTexture Coordinates</b>";
		case ui::obsPasteTextureToBrush: return "<b>Paste Texture to all Brush Faces</b>";
		case ui::obsJumpToObject: return "<b>Jump to Object</b>";
		default: return "<b>Unknown event</b>";
	}
}

float MouseEventManager::getCameraStrafeSpeed() {
	return _strafeSpeed;
}

float MouseEventManager::getCameraForwardStrafeFactor() {
	return _forwardStrafeFactor;
}

bool MouseEventManager::strafeActive(unsigned int& state) {
	return ((_modifiers.getKeyboardFlags(state) & _toggleStrafeCondition.modifierFlags) != 0);
}

bool MouseEventManager::strafeForwardActive(unsigned int& state) {
	return ((_modifiers.getKeyboardFlags(state) & _toggleForwardStrafeCondition.modifierFlags) != 0);
}

std::string MouseEventManager::getShortButtonName(const std::string& longName) {
	if (longName == "MOUSE_LEFT") {
		return "LMB";
	}
	else if (longName == "MOUSE_RIGHT") {
		return "RMB";
	}
	else if (longName == "MOUSE_MIDDLE") {
		return "MMB";
	}
	else if (longName == "MOUSE_THUMB") {
		return "MB4";
	}
	else if (longName == "MOUSE_FIVE") {
		return "MB5";
	}
	else {
		return "";
	} 
}

void MouseEventManager::updateStatusText(GdkEventKey* event) {
	_activeFlags = _modifiers.getKeyboardFlags(event->state);
	
	std::string statusText("");
	
	if (_activeFlags != 0) {
		for (ButtonIdMap::iterator it = _buttonId.begin(); it != _buttonId.end(); it++) {
			// Look up an event with this button ID and the given modifier			
			ui::XYViewEvent xyEvent = findXYViewEvent(it->second, _activeFlags);
			
			if (xyEvent != ui::xyNothing) {
				statusText += _modifiers.getModifierStr(_activeFlags, true) + "-";
				statusText += getShortButtonName(it->first) + ": ";
				statusText += printXYViewEvent(xyEvent);
				statusText += " ";
			}
			
			// Look up an event with this button ID and the given modifier			
			ui::ObserverEvent obsEvent = findObserverEvent(it->second, _activeFlags);
			
			if (obsEvent != ui::obsNothing) {
				statusText += _modifiers.getModifierStr(_activeFlags, true) + "-";
				statusText += getShortButtonName(it->first) + ": ";
				statusText += printObserverEvent(obsEvent);
				statusText += " ";
			}
		}
	}
	
	GlobalRadiant().setStatusText(statusText);
}
