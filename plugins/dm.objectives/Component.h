#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <string>

namespace objectives
{

/**
 * A component of an objective. 
 * 
 * Each objective may have a number of components, which are combined using 
 * boolean operations to determine whether the objective is satisfied or not.
 * A component is essentially a condition which needs to be met in order to
 * satisfy the overall objective.
 */
class Component
{
public:

	/**
	 * Enumeration of valid specifier types.
	 * 
	 * Each Component can have up to 2 <b>specifiers</b>, which provide
	 * information about the target of the component depending on the component
	 * type. For example, a KILL component must specify an AI or group of AI
	 * to be killed.
	 * 
	 * Each specifier consists of two parts, a specifier <b>type</b> and a 
	 * specifier <b>value</b>. This enumeration provides the available specifier
	 * types, which are used by the game to determine the meaning of the 
	 * associated specifier value.
	 */
	enum Specifier
	{
		SPEC_NONE, /**< No specifier. */
		SPEC_NAME, /**< Name of a specific entity. */
		SPEC_OVERALL, /**< Overall loot or AI specifier. */
		SPEC_GROUP, /**< Type-dependent group specifier. */
		SPEC_CLASSNAME, /**< Name of an DEF-based entity class. */
		SPEC_SPAWNCLASS, /**< Name of an SDK-level spawn class. */
		SPEC_AI_TYPE, /**< Name of AI type, such as "human". */
		SPEC_AI_TEAM, /**< Integer AI team number. */
		SPEC_AI_INNOCENCE /**< Value is 1 for combat, 0 for non-combat AI. */
	};
	
public:
	
	/**
	 * Flag to indicate that this component has been satisfied.
	 */
	bool state;
	
	/**
	 * Flag to indicate that the sense of this component is inverted.
	 * 
	 * If true, this component is logically <b>NOT</b>ed, so when the conditions 
	 * described by the type and specifiers are not met, the component state is 
	 * true, and when they are met, it is false.
	 */
	bool inverted;
	
	/**
	 * Flag to indicate that this component changes state once then latches,
	 * even if its in-game condition is no longer satisfied.
	 */
	bool irreversible;
	
	/**
	 * Flag to indicate that the player must be responsible for satisfying this
	 * component.
	 * 
	 * If this flag is set, the component will not be satisfied by an event
	 * which is not <i>directly</i> caused by the player. For example, if the
	 * component requires the killing of an AI, it will not be satisfied if the
	 * AI is killed by another AI rather than the player.
	 */
	bool playerResponsible;
	
	/**
	 * The type of this component ("kill", "ko" etc).
	 */
	std::string type;
	
	/**
	 * The first specifier type.
	 */
	Specifier spec1_type;
	
	/**
	 * The first specifier value.
	 */
	std::string spec1_val;
	
	/**
	 * The second specifier type.
	 */
	Specifier spec2_type;
	
	/**
	 * The second specifier value.
	 */
	std::string spec2_val;
	
	/**
	 * Construct a Component with default settings. All flags are set to false,
	 * and the type is set to the empty string.
	 */
	Component()
	: state(false), inverted(false), irreversible(false), type("")
	{ }
};

}

#endif /*COMPONENT_H_*/
