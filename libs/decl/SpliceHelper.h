#pragma once

#include <istream>
#include <regex>

namespace decl
{

// Helper class which is able to cut out an existing declaration from the given
// input stream. The first line of the declaration is identified using the given regex.
// The part of the input stream leading up to the def is piped to the output stream in unmodified form,
// excluding the declaration block itself.
// Piping will either stop once the declaration is found (and omitted) or the input stream is exhausted.
class SpliceHelper
{
public:
    static void PipeStreamUntilInsertionPoint(std::istream& input, std::ostream& output, const std::string& typeName, const std::string& declName)
    {
        // Write the file to the output stream, up to the point the decl should be written to
        // The typename is optional and compared case-sensitively
        std::regex pattern("^[\\s]*(" + typeName + "[\\s]+" + declName + "|" + declName + ")\\s*\\{*.*$",
            std::regex_constants::icase);

        std::string line;

        while (std::getline(input, line))
        {
            std::smatch matches;

            // See if this line contains the def in question
            if (std::regex_match(line, matches, pattern))
            {
                // Line matches, march from opening brace to the other one
                std::size_t openBraces = 0;
                bool blockStarted = false;

                // Forward the input stream till we find the closing brace
                do
                {
                    for (std::size_t i = 0; i < line.length(); ++i)
                    {
                        if (line[i] == '{')
                        {
                            openBraces++;
                            blockStarted = true;
                        }
                        else if (line[i] == '}')
                        {
                            openBraces--;
                        }
                    }

                    if (blockStarted && openBraces == 0)
                    {
                        break;
                    }
                } while (std::getline(input, line));

                return; // stop right here, return to caller
            }
            
            // No particular match, add line to output
            output << line;

            // Append a newline in any case
            output << std::endl;
        }
    }

};

}
