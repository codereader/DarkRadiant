#ifndef RESPONSEEFFECT_H_
#define RESPONSEEFFECT_H_

#include <vector>
#include <string>
#include "ResponseEffectTypes.h"

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

	// The reference to the entityDef this response effect is basing on
	IEntityClassPtr _eclass;

public:
	// Default constructor
	ResponseEffect();

	// Copy constructor
	ResponseEffect(const ResponseEffect& other);

	// Returns the effect name ("effect_damage")
	std::string getName() const;

	/** greebo: Updates the name of this object. This triggers
	 * 			an update of the contained IEntityClassPtr as well.
	 */
	void setName(const std::string& name);
	
	/** greebo: Retrieves the argument with the given index.
	 */
	std::string getArgument(unsigned int index) const;
	
	/** greebo: Sets the argument with the given ID to the given value.
	 */
	void setArgument(unsigned int index, const std::string& value);
	
	/** greebo: Returns the "display name" of this effect (e.g. "Damage")
	 */
	std::string getCaption() const;
};

#endif /*RESPONSEEFFECT_H_*/
