#pragma once

#include <stdexcept>
#include "imessagebus.h"
#include "imapformat.h"

namespace radiant
{

/**
 * Message sent when the backend code needs the user to 
 * browse and select a file.
 */
class FileSelectionRequest :
    public radiant::IMessage
{
public:
    enum class Mode
    {
        Open,
        Save,
    };

    class Result
    {
    public:
        // The full path of the selected file
        std::string fullPath;

        // For save dialogs, a specific map format might have been selected
        std::string mapFormatName;

        // The reference to the named mapformat above. Can be an empty pointer.
        map::MapFormatPtr mapFormat;
    };

private:
    Mode _mode;
    std::string _type;
    std::string _title;
    std::string _defaultFile;
    Result _result;

public:
    /**
     * Asks the UI code to select a file to load or save.
     *
     * @param mode
     * Whether this is an open operation or a save operation (changes file
     * dialog behaviour).
     *
     * @param title
     * The title to display on the dialog, such as "Open map" or "Export
     * selection".
     *
     * @param type: the file type to be loaded ("map", "prefab" or "region")
     *
     * @returns
     * The info structure of the file selection, the member fullPath of which will
     * be empty string if no selection was made.
     */
    FileSelectionRequest(Mode mode, const std::string& title, const std::string& type) :
        FileSelectionRequest(mode, title, type, std::string())
    {}

    /**
     * Asks the UI code to select a file to load or save.
     *
     * @param mode
     * Whether this is an open operation or a save operation (changes file
     * dialog behaviour).
     *
     * @param title
     * The title to display on the dialog, such as "Open map" or "Export
     * selection".
     *
     * @param type: the file type to be loaded ("map", "prefab" or "region")
     *
     * @param defaultFile
     * The default file path which should already be selected when the dialog
     * is displayed.
     *
     * @returns
     * The info structure of the file selection, the member fullPath of which will
     * be empty string if no selection was made.
     */
    FileSelectionRequest(Mode mode, const std::string& title,
                         const std::string& type, const std::string& defaultFile) :
        _mode(mode),
        _type(type),
        _title(title),
        _defaultFile(defaultFile)
    {}

    std::size_t getId() const override
    {
        return IMessage::Type::FileSelectionRequest;
    }

    const std::string& getType() const
    {
        return _type;
    }

    const std::string& getTitle() const
    {
        return _title;
    }

    Mode getMode() const
    {
        return _mode;
    }

    const std::string& getDefaultFile() const
    {
        return _defaultFile;
    }

    // Returns the result of the file selection operation
    // The file path is empty if nothing was selected or the
    // selection has been cancelled
    const Result& getResult() const
    {
        return _result;
    }

    void setResult(const Result& result)
    {
        _result = result;
    }
};

}
