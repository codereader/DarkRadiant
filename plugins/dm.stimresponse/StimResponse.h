#ifndef STIMRESPONSE_H_
#define STIMRESPONSE_H_

#include <map>
#include <string>

/** greebo: A simple stim/response representation structure
 * (a SREntity can hold a bunch of these)
 */

class StimResponse
{
public:
	enum SRClass {
		typeStim,
		typeResponse,
	};

private:
	// The key/value mapping
	typedef std::map<std::string, std::string> PropertyMap;

	// The S/R class
	SRClass _class;
	
	// The list of named properties
	PropertyMap _properties;
	
public:
	StimResponse();
	
	// Copy constructor
	StimResponse(const StimResponse& other);
	
	/** greebo: Gets the property value string or "" if not defined/empty
	 */
	std::string get(const std::string& key);
	
	/** greebo: Sets the given <key> to <value>
	 */
	void set(const std::string& key, const std::string& value);
};

#endif /*STIMRESPONSE_H_*/
