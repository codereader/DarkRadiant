#pragma once

#include "ieventmanager.h"

#include <list>
#include <iostream>

namespace ui
{

/* greebo: An Accelerator consists of a key/modifier combination plus a connected Event object.
 *
 * Use the match() method to test if the accelerator matches a certain key/modifier combination.
 * Use the connectCommand() method to assign a command to this accelerator.
 * Use the keyUp()/keyDown() methods to trigger the keyup/keydown command callbacks.
 */
class Accelerator :
    public IAccelerator
{
private:
    // The internally stored key/modifier combination
    unsigned int _key;
    unsigned int _modifiers;

    // The connected event
    IEventPtr _event;

    // Alternative if event is null: the command to execute
    std::string _statement;

    bool _isEmpty;

    // Private constructor creating an empty accelerator
    Accelerator();

public:
    typedef std::shared_ptr<Accelerator> Ptr;

    // Construct an accelerator out of the key/modifier plus a command
    Accelerator(const unsigned int key, const unsigned int modifiers, const IEventPtr& event);

    // Returns true if the key/modifier combination matches this accelerator
    bool match(const unsigned int key, const unsigned int modifiers) const;

    // Returns true if the event is attached to this Accelerator
    bool match(const IEventPtr& event) const;

    // Reads out the interal key/modifier combination of this Accelerator
    unsigned int getKey() const override;
    unsigned int getModifiers() const override;

    // Returns the statement associated to this accelerator, or an empty string
    // in case it's tied to an Event (check getEvent() in that case)
    const std::string& getStatement() const;
    void setStatement(const std::string& statement);

    // Make the accelerator use this key/modifier
    void setKey(const unsigned int key) override;
    void setModifiers(const unsigned int modifiers) override;

    // Retrieve the contained event pointer
    const IEventPtr& getEvent();
    // Connect this modifier to the specified command
    void setEvent(const IEventPtr& ev);

    std::string getString(bool forMenu) const override;

    bool isEmpty() const;

    /**
     * Converts a string representation of a key to the corresponding
     * wxKeyCode, e.g. "A" => 65, "TAB" => wxKeyCode::WXK_TAB.
     *
     * Note that "a" will also result in the uppercase value of 'A' == 65.
     *
     * In GDK we had gdk_keyval_from_name, which seems to be missing
     * in wxWidgets.
     *
     * Will return WXK_NONE if no conversion exists.
     */
    static unsigned int getKeyCodeFromName(const std::string& name);

    /**
     * Converts the given keycode to a string to serialize it into
     * a settings or XML file. The given keycode will be converted
     * to its uppercase pendant if possible. 'a' => 'A'
     */
    static std::string getNameFromKeyCode(unsigned int keyCode);

    // Create an empty accelerator
    static Accelerator CreateEmpty();
};

typedef std::list<Accelerator::Ptr> AcceleratorList;

}
