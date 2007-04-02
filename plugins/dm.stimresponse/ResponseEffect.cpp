#include "ResponseEffect.h"

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
