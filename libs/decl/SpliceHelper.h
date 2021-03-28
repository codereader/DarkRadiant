#pragma once

#include <iostream>
#include <regex>

namespace decl
{

// Helper class which is able to cut out an existing declaration from the given
// input stream. The first line of the declaration is identified using the given regex.
// The part of the input stream leading up to the def is piped to the output stream in unmodified form,
// excluding the declaration block itself.
// Piping will either stop once the declaration is found (and omitted) or the input stream is exhausted.
// The capture group in the regex is used to identify an opening brace in the same line as the decl name.
class SpliceHelper
{
public:
    static void PipeStreamUntilInsertionPoint(std::istream& input, std::ostream& output, const std::regex& patternToFind)
    {
        std::string line;

        while (std::getline(input, line))
        {
            std::smatch matches;

            // See if this line contains the def in question
            if (std::regex_match(line, matches, patternToFind))
            {
                // Line matches, march from opening brace to the other one
                std::size_t openBraces = 0;
                bool blockStarted = false;

                if (!matches[1].str().empty())
                {
                    // We've had an opening brace in the first line
                    openBraces++;
                    blockStarted = true;
                }

                while (std::getline(input, line))
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
                }

                return; // stop right here, return to caller
            }
            else
            {
                // No particular match, add line to output
                output << line;
            }

            // Append a newline in any case
            output << std::endl;
        }
    }

};

}
