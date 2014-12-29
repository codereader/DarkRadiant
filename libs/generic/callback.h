#ifndef _CALLBACK_H_
#define _CALLBACK_H_

#include <functional>

// Typedef to satisfy old code - in many places the "Callback" type is still used
typedef std::function<void()> Callback;

#endif /* _CALLBACK_H_ */
