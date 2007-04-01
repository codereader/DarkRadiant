#ifndef RESPONSEEFFECT_H_
#define RESPONSEEFFECT_H_

#include <vector>
#include <string>

class ResponseEffect
{
public:
	struct Argument {
		// The type of this property (entity, vector, float, etc.)
		int type;
		
		// The argument value
		std::string value;
	};
	
private:
	// The name of this effect (e.g. "effect_teleport")
	std::string _effectName;
	
	typedef std::map<int, Argument> ArgumentList;
	
	// The list of arguments this effect needs
	ArgumentList _args;

public:
	ResponseEffect() 
	{}

	// Copy constructor
	ResponseEffect(const ResponseEffect& other) :
		_effectName(other._effectName),
		_args(other._args)
	{}

	std::string getName() const {
		return _effectName;
	}

	void setName(const std::string& name) {
		_effectName = name;
	}
	
	std::string getArgument(unsigned int index) const {
		ArgumentList::const_iterator i = _args.find(index);
		
		// Return "" if the argument was not found
		return (i != _args.end()) ? i->second.value : "";
	}
	
	void setArgument(unsigned int index, const std::string& value) {
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
};

#endif /*RESPONSEEFFECT_H_*/
