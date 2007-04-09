#ifndef RESPONSEEFFECT_H_
#define RESPONSEEFFECT_H_

#include <vector>
#include <string>
#include "ResponseEffectTypes.h"

class ResponseEffect
{
public:
	struct Argument {
		// The type string of this property ("e" => entity)
		std::string type;
		
		// TRUE, if this argument can be omitted
		bool optional;
		
		// The caption of this argument
		std::string title;
		
		// The description string
		std::string desc;
		
		// The argument value
		std::string value;
	};
	typedef std::map<int, Argument> ArgumentList;
	
private:
	// The name of this effect (e.g. "effect_teleport")
	std::string _effectName;
	
	// The state of this response effect
	bool _state;
	
	// The list of arguments this effect needs
	ArgumentList _args;

	// The reference to the entityDef this response effect is basing on
	IEntityClassPtr _eclass;
	
	// Stays false until the eclass has been parsed for effect argument types
	bool _argumentListBuilt;

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
	
	bool isActive() const;
	void setActive(bool active);
	
	/** greebo: Retrieves the argument with the given index.
	 */
	std::string getArgument(unsigned int index) const;
	
	/** greebo: Sets the argument with the given ID to the given value.
	 */
	void setArgument(unsigned int index, const std::string& value);
	
	/** greebo: Returns a reference to the internal list
	 */
	ArgumentList& getArguments();
	
	/** greebo: Returns a "fabricated" argument string
	 * 			in human-readable format to pack into the effects list
	 */
	std::string getArgumentStr();
	
	/** greebo: Returns the "display name" of this effect (e.g. "Damage")
	 */
	std::string getCaption() const;
	
	/** greebo: Returns the entity class pointer (as the name states)
	 */
	IEntityClassPtr getEClass() const;
	
	/** greebo: Clears and rebuilds the argument list from
	 * 			the information found in the entity class.
	 */
	void buildArgumentList();
	
	/** greebo: Clears the argument list (removes all map entries)
	 */
	void clearArgumentList();
};

#endif /*RESPONSEEFFECT_H_*/
