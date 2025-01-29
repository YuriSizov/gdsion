/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_base.h"

#include <godot_cpp/classes/audio_stream_wav.hpp>

SiOPMWaveBase::SiOPMWaveBase(SiONModuleType p_module_type) {
	_module_type = p_module_type;
}

Vector<double> SiOPMWaveBase::_extract_wave_data(const Ref<AudioStream> &p_stream, int *r_channel_count) {
	if (p_stream.is_null()) {
		return Vector<double>();
	}

	Ref<AudioStreamWAV> wav_stream = p_stream;
	if (wav_stream.is_valid()) {
		AudioStreamWAV::Format data_format = wav_stream->get_format();
		if (data_format != AudioStreamWAV::FORMAT_8_BITS && data_format != AudioStreamWAV::FORMAT_16_BITS) {
			ERR_FAIL_V_MSG(Vector<double>(), vformat("SiOPMWaveBase: Unsupported WAV file format (%d).", data_format));
		}

		*r_channel_count = (wav_stream->is_stereo() ? 2 : 1);
		PackedByteArray wav_data = wav_stream->get_data();
		Vector<double> raw_data;

		int offset = 0;
		while (offset < wav_data.size()) {
			switch (data_format) {
				case AudioStreamWAV::FORMAT_8_BITS: {
					int value = wav_data.decode_s8(offset);
					double sample = float(value) / 127.0; // Max int8.
					raw_data.push_back(sample);

					offset += 1;
				} break;

				case AudioStreamWAV::FORMAT_16_BITS: {
					int value = wav_data.decode_s16(offset);
					double sample = float(value) / 32767.0; // Max int16.
					raw_data.push_back(sample);

					offset += 2;
				} break;

				default:
					offset += 1;
					break;
			}
		}

		return raw_data;
	}

	ERR_FAIL_V_MSG(Vector<double>(), "SiOPMWaveBase: Unsupported audio stream format.");
}
