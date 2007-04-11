#ifndef STIMRESPONSE_H_
#define STIMRESPONSE_H_

#include <map>
#include <list>
#include <string>
#include "ResponseEffect.h"

	namespace {
		const std::string RKEY_STIM_RESPONSE_PREFIX = 
				"game/stimResponseSystem/stimResponsePrefix";
				
		enum {
			EFFECT_INDEX_COL,
			EFFECT_CAPTION_COL,
			EFFECT_ARGS_COL,
			EFFECT_NUM_COLS,
		};
	}

// Forward declaration
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeIter GtkTreeIter;

struct SRKey {
	// The key name
	std::string key;
	
	// A string defining the classes this applies for
	// e.g. "R" for responses only, "SR" for both
	std::string classes;
};

/** greebo: A simple stim/response representation structure
 * (a SREntity can hold a bunch of these)
 */
class StimResponse
{
public:
	// The effect map
	typedef std::map<unsigned int, ResponseEffect> EffectMap;

private:
	struct Property {
		// The actual value (this is used for write-accesses)
		std::string value;
		
		// The "original" value (this is considered read-only)
		// Is used for inherited values to check if they were
		// "overruled" by non-inherited values (on the target entity)
		std::string origValue;
	};

	// The key/value mapping
	typedef std::map<std::string, Property> PropertyMap;

	// TRUE, if this stems from an inherited eclass. Makes this object read-only	
	bool _inherited;
	
	// The list of named properties
	PropertyMap _properties;
	
	/** greebo: The index of this object. The StimResponse objects themselves
	 * 			are already mapped by the SREntity class, although it's
	 * 			useful to set an index of this object by hand to allow
	 * 			it to override an inherited stim.
	 */
	int _index;
	
	// The list of ResponseEffects for this response (does not apply for stims)
	EffectMap _effects;
	
public:
	StimResponse();
	
	// Copy constructor
	StimResponse(const StimResponse& other);
	
	/** greebo: Returns / sets the "inherited" flag
	 */
	bool inherited() const;
	void setInherited(bool inherited);
	
	/** greebo: Gets/Sets the index of this object (only for non-inherited)
	 */
	int getIndex() const;
	void setIndex(int index);
	
	/** greebo: Gets the property value string or "" if not defined/empty
	 */
	std::string get(const std::string& key);
	
	/** greebo: Sets the given <key> to <value>.
	 * 
	 * @load: This is indicating the origin of this value and can be used
	 * 		  to determine whether this is an "override" operation or just
	 * 		  an ordinary property population (during parsing the entityclass).
	 */
	void set(const std::string& key, const std::string& value, bool inherited = false);
	
	/** greebo: Returns TRUE, if the given key has been overridden.
	 */
	bool isOverridden(const std::string& key);
	
	/** greebo: Retrieves the response effect with the given id.
	 * 			This creates the response effect if it doesn't exist yet.
	 */
	ResponseEffect& getResponseEffect(const unsigned int index);
	
	/** greebo: Returns the reference to the interally stored map
	 */
	EffectMap& getEffects();
	
	/** greebo: Re-indexes the effects to avoid gaps in the numbering.
	 * 			Should be called right before saving the effects to the
	 * 			spawnargs.
	 */
	void sortEffects();
	
	/** greebo: Returns the highest used effect index.
	 */
	unsigned int highestEffectIndex();
	
	/** greebo: Swaps the two effects with the specified indices. 
	 */
	void moveEffect(const unsigned int fromIndex, const unsigned int toIndex);
	
	/** greebo: Removes the effect with the given index.
	 * 			This re-sorts the items after deletion.
	 */
	void deleteEffect(const unsigned int index);
	
	/** greebo: Adds a new effect before the given index.
	 * 
	 * @index: the index of the effect the new one should be 
	 * 		   inserted before. If this is negative, the item
	 * 		   is appended at the end of the list.
	 */
	void addEffect(const unsigned int index);
	
	/** greebo: Constructs the GtkListStore using the effects stored in this
	 * 			response.
	 */
	GtkListStore* getEffectStore();
	
private:
	/** greebo: Write the values of the passed ResponseEffect to the 
	 * 			GtkListStore using the passed GtkTreeIter.
	 * 			The ID stays untouched. 
	 * 
	 * @store: The ListStore
	 * @iter: The TreeIter pointing at the row where the data should be inserted
	 * @sr: the ResponseEffect object containing the source data
	 */
	void writeToListStore(GtkListStore* store, GtkTreeIter* iter, ResponseEffect& effect);
};

#endif /*STIMRESPONSE_H_*/
