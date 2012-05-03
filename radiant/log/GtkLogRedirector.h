#ifndef _GTK_LOG_REDIRECTOR_H_
#define _GTK_LOG_REDIRECTOR_H_

#include "LogStream.h"
#include <boost/shared_ptr.hpp>
#include <map>
#include <glib.h>

namespace applog {

// shared_ptr forward declaration
class GtkLogRedirector;
typedef boost::shared_ptr<GtkLogRedirector> GtkLogRedirectorPtr;

class GtkLogRedirector
{
private:
	std::map<std::string, guint> _handles;

public:
	GtkLogRedirector();
	~GtkLogRedirector();

	// A call to init() will redirect the std::cout output to the log
	static void init();

	// A call to destroy() will stop redirecting std::cout
	static void destroy();

private:
	static GtkLogRedirectorPtr& InstancePtr();

	static void handleLogMessage(const gchar* log_domain,
								GLogLevelFlags log_level,
								const gchar* message,
								gpointer user_data);
};

} // namespace applog

#endif /* _GTK_LOG_REDIRECTOR_H_ */
