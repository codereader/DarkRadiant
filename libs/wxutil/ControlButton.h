#pragma once

#include <wx/bmpbuttn.h>
#include <wx/timer.h>

namespace wxutil
{

namespace
{
	// The delay between the first "click" and the second "click" event
	const int DELAY_INITIAL = 200;
	// The delay between all following "click" events
	const int DELAY_PERIODIC = 20;
}

/**
 * A button containing a single icon that keeps periodically emitting the
 * "clicked" event as long as the user keeps the mouse button pressed.
 * Used for Surface Inspector controls, for example.
 */
class ControlButton :
	public wxBitmapButton
{
private:
	wxTimer _timer;

public:

	ControlButton(wxWindow* parent, const wxBitmap& bitmap) :
		wxBitmapButton(parent, wxID_ANY, bitmap),
		_timer(this)
	{
		// Connect the pressed/released signals
		Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ControlButton::onPress), NULL, this);
		Connect(wxEVT_LEFT_UP, wxMouseEventHandler(ControlButton::onRelease), NULL, this);

		Connect(wxEVT_TIMER, wxTimerEventHandler(ControlButton::onIntervalReached), NULL, this);

		_timer.Stop();
	}

	void onIntervalReached(wxTimerEvent& ev)
	{
		// Fire the "clicked" signal
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		ProcessEvent(event);

		// Set the interval to a smaller value
		_timer.Stop();
		_timer.Start(DELAY_PERIODIC);
	}

	void onPress(wxMouseEvent& ev)
	{
		// Trigger a first click
		wxCommandEvent event(wxEVT_BUTTON, GetId());
		event.SetEventObject(this);
		ProcessEvent(event);

		// Start the timer using the initial value
		_timer.Start(DELAY_INITIAL);
	}

	void onRelease(wxMouseEvent& ev)
	{
		// Disconnect the timing event
		_timer.Stop();
	}
};

} // namespace
