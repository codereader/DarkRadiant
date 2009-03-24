#pragma once

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

   // Time until next event
   unsigned long _timeUntilNext;

private:

   // Decrement the timer (platform-specific)
   void decrementTimer()
   {
      _timeUntilNext--;
   }

public:

   /**
    * \brief
    * Construct an EventRateLimiter with the given event separation time.
    *
    * \param separationTime
    * Value representing the number of calls to readyForEvent() which should
    * occur between events.
    *
    * \todo
    * On supported platforms, make the separation time work as an actual time in
    * milliseconds.
    */
   EventRateLimiter(unsigned long separationTime)
   : _separationTime(separationTime),
     _timeUntilNext(separationTime)
   { }

   /**
    * \brief
    * Test whether the event is ready to be triggered.
    *
    * If the return value is false, the timer is decremented and the calling
    * code should not trigger the event. If the return value is true, the timer
    * is reset and the event may be triggered by the calling code.
    */
   bool readyForEvent()
   {
      if (_timeUntilNext == 0)
      {
         _timeUntilNext = _separationTime;
         return true;
      }
      else
      {
         decrementTimer();
         return false;
      }
   }
};
