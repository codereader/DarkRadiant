#include "ResponseEffect.h"

#include "string/string.h"

ResponseEffect::ResponseEffect() :
	_eclass(IEntityClassPtr())
{}

// Copy constructor
ResponseEffect::ResponseEffect(const ResponseEffect& other) :
	_effectName(other._effectName),
	_args(other._args),
	_eclass(other._eclass) 
{}

std::string ResponseEffect::getName() const {
	return _effectName;
}

void ResponseEffect::setName(const std::string& name) {
	_effectName = name;
	// Update the entityclass pointer
	_eclass = ResponseEffectTypes::Instance().getEClassForName(name);
	
	// Clear and rebuild the argument list 
	buildArgumentList();
}

std::string ResponseEffect::getArgument(unsigned int index) const {
	ArgumentList::const_iterator i = _args.find(index);
	
	// Return "" if the argument was not found
	return (i != _args.end()) ? i->second.value : "";
}

void ResponseEffect::setArgument(unsigned int index, const std::string& value) {
	ArgumentList::const_iterator i = _args.find(index);
	
	if (i == _args.end()) {
		// Argument doesn't exist, create it
		Argument newArgument;
		newArgument.value = value;
		
		// Store the argument in the map
		_args[index] = newArgument;
	}
	else {
		// Argument exists
		_args[index].value = value;
	}
}

std::string ResponseEffect::getCaption() const {
	return (_eclass != NULL) ? _eclass->getValueForKey("editor_caption") : ""; 
}

IEntityClassPtr ResponseEffect::getEClass() const {
	return _eclass;
}

ResponseEffect::ArgumentList& ResponseEffect::getArguments() {
	return _args;
}

void ResponseEffect::buildArgumentList() {
	if (_eclass == NULL) return;
	
	for (int i = 1; i < 1000; i++) {
		std::string argType = _eclass->getValueForKey("editor_argType" + intToStr(i));
		std::string argDesc = _eclass->getValueForKey("editor_argDesc" + intToStr(i));
		std::string argTitle = _eclass->getValueForKey("editor_argTitle" + intToStr(i));
		std::string optional = _eclass->getValueForKey("editor_argOptional" + intToStr(i));
		
		if (argType != "") {
			// Check if the argument exists
			ArgumentList::iterator found = _args.find(i);
			if (found == _args.end()) {
				Argument newArgument;
				_args[i] = newArgument;
			}
			
			// Load the values into the structure
			_args[i].type = argType;
			_args[i].desc = argDesc;
			_args[i].title = argTitle;
			_args[i].optional = (optional == "1");
		}
		else {
			// Empty argument type found, break the loop
			break;
		}
	}
}
