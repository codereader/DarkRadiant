#ifndef STIMTYPES_H_
#define STIMTYPES_H_

#include <map>
#include <string>

/** greebo: A simple Stim representation.
 */
struct Stim {
	std::string name;			// The name (STIM_FIRE)
	std::string caption;		// Nice format ("Fire")
	std::string description;	// Unused at the moment
};
typedef std::map<int, Stim> StimTypeMap;

class StimTypes
{
	// The list of available stims 
	StimTypeMap _stims;
	
	// The empty stim.
	Stim _emptyStim;

public:
	/** greebo: Constructor, loads the Stim types from the registry.
	 */
	StimTypes();
	
	/** greebo: Returns the Stim with the given ID
	 */
	Stim get(int id);
};


#endif /*STIMTYPES_H_*/
