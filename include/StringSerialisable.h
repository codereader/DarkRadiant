#pragma once

#include <string>

/**
 * \brief
 * Interface for an object which can serialise itself to/from a string.
 */
class StringSerialisable
{
public:

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

