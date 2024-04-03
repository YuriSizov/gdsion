/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_TRACK_EVENT_H
#define SION_TRACK_EVENT_H

#include "events/sion_event.h"

class SiMMLTrack;

class SiONTrackEvent : public SiONEvent {
	GDCLASS(SiONTrackEvent, SiONEvent)

	SiMMLTrack *_track = nullptr;
	int _event_trigger_id = 0;
	int _note = 0;
	int _buffer_index = 0;
	double _frame_trigger_delay = 0;
	int _frame_trigger_timer = 0;

protected:
	static void _bind_methods();

public:
	// Event types, doubling as signal names.

	// Emitted with the "note on" event in a sequence with the "%t" command.
	// Event trigger ID contains the ID specified in the 1st argument of the command.
	static const char *NOTE_ON_STREAM;

	// Emitted with the "note off" event in a sequence with the "%t" command.
	// Event trigger ID contains the ID specified in the 1st argument of the command.
	static const char *NOTE_OFF_STREAM;

	// Emitted when the sound starts.
	// Event trigger ID contains the ID specified in the 1st argument of the command.
	static const char *NOTE_ON_FRAME;

	// Emitted when the sound ends.
	// Event trigger ID contains the ID specified in the 1st argument of the command.
	static const char *NOTE_OFF_FRAME;

	// Emitted on each beat while streaming.
	// Event trigger ID contains a counter in 16th beat.
	static const char *STREAMING_BEAT;

	// Emitted when the BPM changes.
	static const char *BPM_CHANGED;

	// Emitted when a user defined track event is dispatched.
	// Event trigger ID and note number are configured as arguments of the dispatched event.
	static const char *USER_DEFINED;

	// Properties.

	SiMMLTrack *get_track() const { return _track; }
	int get_event_trigger_id() const { return _event_trigger_id; }
	int get_note() const { return _note; }
	int get_buffer_index() const { return _buffer_index; }
	double get_frame_trigger_delay() const { return _frame_trigger_delay; }

	bool decrement_timer(int p_frame_rate);

	//

	SiONTrackEvent(String p_type = String(), SiONDriver *p_driver = nullptr, SiMMLTrack *p_track = nullptr, int p_buffer_index = 0, int p_note = 0, int p_event_trigger_id = 0);
	~SiONTrackEvent() {}
};

#endif // SION_TRACK_EVENT_H
