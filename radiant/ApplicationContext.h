#pragma once

#include "module/ApplicationContextBase.h"
#include "log/PopupErrorHandler.h"

namespace radiant
{

/**
 * Application context implementation of the RadiantApp
 */
class ApplicationContext :
    public ApplicationContextBase
{
public:
    // Custom initialise method, setting up the wxWidgets popup error handler
    void initialise(int argc, char* argv[]) override
    {
        ApplicationContextBase::initialise(argc, argv);

        initErrorHandler();
    }

    void initErrorHandler()
    {
#ifdef _DEBUG
        // Use the PopupErrorHandler, which displays a popup box
        setErrorHandlingFunction(radiant::PopupErrorHandler::HandleError);

        // Initialise the function pointer in our binary's scope
        GlobalErrorHandler() = getErrorHandlingFunction();
#endif
    }
};

}
