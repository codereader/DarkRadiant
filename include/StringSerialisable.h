#pragma once

#include <string>
#include <boost/shared_ptr.hpp>

/**
 * \brief
 * Interface for an object which can serialise itself to/from a string.
 */
class StringSerialisable
{
public:
   /**
	* \brief
	* Destructor
	*/
   virtual ~StringSerialisable() {}

   /**
    * \brief
    * Export this object's state to a string.
    */
   virtual std::string exportToString() const = 0;

   /**
    * \brief
    * Import this object's state from a given string.
    */
   virtual void importFromString(const std::string& str) = 0;
};

typedef boost::shared_ptr<StringSerialisable> StringSerialisablePtr;

