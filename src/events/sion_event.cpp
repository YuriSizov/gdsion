/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "sion_event.h"

#include "sion_data.h"
#include "sion_driver.h"

const char *SiONEvent::QUEUE_EXECUTING = "queue_executing";
const char *SiONEvent::QUEUE_COMPLETED = "queue_completed";
const char *SiONEvent::QUEUE_CANCELLED = "queue_cancelled";

const char *SiONEvent::STREAMING = "streaming";
const char *SiONEvent::STREAM_STARTED = "stream_started";
const char *SiONEvent::STREAM_STOPPED = "stream_stopped";
const char *SiONEvent::SEQUENCE_FINISHED = "sequence_finished";

const char *SiONEvent::FADING = "fading";
const char *SiONEvent::FADE_IN_COMPLETED = "fade_in_completed";
const char *SiONEvent::FADE_OUT_COMPLETED = "fade_out_completed";

Ref<SiONData> SiONEvent::get_data() const {
	ERR_FAIL_NULL_V(_driver, nullptr);
	return _driver->get_data();
}

//

void SiONEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_event_type"), &SiONEvent::get_event_type);
	ClassDB::bind_method(D_METHOD("get_driver"), &SiONEvent::get_driver);
	ClassDB::bind_method(D_METHOD("get_data"), &SiONEvent::get_data);
	ClassDB::bind_method(D_METHOD("get_stream_buffer"), &SiONEvent::get_stream_buffer);
}

SiONEvent::SiONEvent(String p_type, SiONDriver *p_driver, PackedVector2Array p_stream_buffer) {
	_event_type = p_type;
	_driver = p_driver;
	_stream_buffer = p_stream_buffer;
}
