/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_PARSER_SETTINGS_H
#define MML_PARSER_SETTINGS_H

class MMLParserSettings {

	// Offset from MML notes to MIDI note numbers. Calculated from the default octave.
	int _mml_to_note_number = 0;
	int _default_octave = 0;

public:
	// Resolution of the note length. 'resolution/4' is a length of a beat.
	int resolution = 1920;
	double default_bpm = 120;

	// Default value of the l command.
	int default_l_value = 4;

	// Minimum ratio of the q command.
	int min_quant_ratio = 0;
	// Maximum ratio of the q command.
	int max_quant_ratio = 8;
	// Default value of the q command.
	int default_quant_ratio = 10;
	// Minimum value of the @q command.
	int min_quant_count = -192;
	// Maximum value of the @q command.
	int max_quant_count = 192;
	// Default value of the @q command.
	int default_quant_count = 0;

	// Maximum value of the v command.
	int max_volume = 15;
	// Default value of the v command.
	int default_volume = 10;
	// Maximum value of the @v command.
	int max_fine_volume = 127;
	// Default value of the @v command.
	int default_fine_volume = 127;

	// Minimum value of the o command.
	int min_octave = 0;
	// Maximum value of the o command.
	int max_octave = 9;

	// Polarization of the ( and ) command. 1=x68k/-1=pc98.
	int volume_polarization = 1;
	// Polarization of the < and > command. 1=x68k/-1=pc98.
	int octave_polarization = 1;

	int get_mml_to_note_offset() const { return _mml_to_note_number; }

	// Default value of length in MML event.
	int get_default_length() const;

	int get_default_octave() const { return _default_octave; }
	void set_default_octave(int p_value);

	// Note that the original method takes an initialization object (which is also used but initialize/update);
	// but there is nothing using this feature. So it's been removed for now.
	MMLParserSettings();
	~MMLParserSettings() {}
};

#endif // MML_PARSER_SETTINGS_H
