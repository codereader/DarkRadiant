#pragma once

#include <ctime>

/**
 * \brief
 * Helper class to restrict the frequency of an event, such as updating a GTK
 * dialog in response to rapidly-changing data.
 *
 * An EventRateLimiter provides a simple means for the frequency of an arbitrary
 * event to be limited. It provides a single method, readyForEvent(), which
 * returns a boolean indicating whether the event should be triggered or not.
 * The EventRateLimiter is initalised with a value indicating how often the
 * events may be triggered.
 *
 * Note that the EventRateLimiter has no knowledge of the underlying event, it
 * simply returns a value from the readyForEvent() function, assuming that the
 * calling code will fire the event if the return value is true.
 */
class EventRateLimiter
{
   // Separation time provided at construction
   unsigned long _separationTime;

   // Last clock invocation
   clock_t _lastClock;

public:

   /**
    * \brief
    * Construct an EventRateLimiter with the given event separation time.
    *
    * \param separationTime
    * Number of milliseconds that should elapse between subsequent events.
    */
   EventRateLimiter(unsigned long separationTimeMillis)
   : _separationTime(separationTimeMillis),
     _lastClock(clock())
   { }

   /**
    * \brief
    * Test whether the event is ready to be triggered.
    *
    * If the return value is false, the calling code should not trigger the
    * event. If the return value is true, the timer is reset and the event may
    * be triggered by the calling code.
    */
   bool readyForEvent()
   {
      clock_t currentClk = clock();
      float diffMillis = (currentClk - _lastClock) / (0.001f * CLOCKS_PER_SEC);
      if (diffMillis >= _separationTime)
      {
         _lastClock = currentClk;
         return true;
      }
      else
      {
         return false;
      }
   }
};
