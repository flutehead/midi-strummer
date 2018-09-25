# midi-strummer
Converts MIDI input to strummed chords as MIDI output.
Built for SparkFun Midi Shield, use "Midi output" jumper setting.

Conversion is as follows:
* second      => m9 chord
* minor third => m7 chord
* major third => Maj7 chord
* fourth      => Maj13 chord
* fifth       => V7 chord

MidiStrummer alternates between strum up and strum down, velo is converted into arpeggio delay.
Two variants are provided per chord.

To-do:
* Replace delay() with async timer
* Add footswitch for stopping chords / ghost notes
