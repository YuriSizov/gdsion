/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_EVENT_H
#define SION_EVENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

using namespace godot;

class SiONData;
class SiONDriver;

class SiONEvent : public RefCounted {
	GDCLASS(SiONEvent, RefCounted)

	String _event_type;

	SiONDriver *_driver = nullptr;
	PackedVector2Array _stream_buffer;

protected:
	static void _bind_methods();

public:
	// Event types, doubling as signal names.

	// Emitted when executing queued jobs.
	// Contains the SiONDriver instance (with the processed MML string when compiling) and the SiONData instance (only when rendering).
	static const char *QUEUE_EXECUTING;

	// Emitted when all queued jobs are completed.
	// Contains the SiONDriver instance (with the processed MML string when compiling) and the SiONData instance.
	static const char *QUEUE_COMPLETED;

	// Emitted when all queued jobs are cancelled.
	// Contains the SiONDriver instance.
	static const char *QUEUE_CANCELLED;

	// Emitted while streaming. This signal is emitted repeatedly according to the streaming timing.
	// Contains the SiONDriver instance, the SiONData instance (unless SiONDriver::play was called without data), and the streaming buffer as a byte array.
	static const char *STREAMING;

	// Emitted when the streaming is started. This signal is emitted before the first STREAMING signal.
	// Contains the SiONDriver instance and the SiONData instance (unless SiONDriver::play was called without data).
	static const char *STREAM_STARTED;

	// Emitted when the streaming is stopped.
	// Contains the SiONDriver instance and the SiONData instance (unless SiONDriver::play was called without data).
	static const char *STREAM_STOPPED;

	// Emitted when all sequences have finished executing.
	// Contains the SiONDriver instance and the SiONData instance.
	static const char *SEQUENCE_FINISHED;

	// Emitted when fading. This signal is emitted after the STREAMING signal.
	// Contains the SiONDriver instance and the SiONData instance (unless SiONDriver::play was called without data).
	static const char *FADING;

	// Emitted when the fade in is complete. This signal is emitted after the STREAMING signal.
	// Contains the SiONDriver instance, the SiONData instance (unless SiONDriver::play was called without data), and the streaming buffer as a byte array.
	static const char *FADE_IN_COMPLETED;

	// Emitted when the fade out is complete. This signal is emitted after the STREAMING signal.
	// Contains the SiONDriver instance, the SiONData instance (unless SiONDriver::play was called without data), and the streaming buffer as a byte array.
	static const char *FADE_OUT_COMPLETED;

	// Properties.

	String get_event_type() const { return _event_type; }

	SiONDriver *get_driver() const { return _driver; }
	SiONData *get_data() const;
	PackedVector2Array get_stream_buffer() const { return _stream_buffer; }

	//

	SiONEvent(String p_type = String(), SiONDriver *p_driver = nullptr, PackedVector2Array p_stream_buffer = PackedVector2Array());
	~SiONEvent() {}
};

#endif // SION_EVENT_H
