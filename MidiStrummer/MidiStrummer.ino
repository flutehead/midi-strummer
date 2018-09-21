// -----------------------------------------------------------------------------
// MidiStrummer - Jörg Ockel - September 2018
//
// Converts 2-tone intervals into strummed chords:
//   second      => m9 chord
//   minor third => m7 chord
//   major third => Maj7 chord
//   fourth      => Maj13 chord
//   fifth       => V7 chord
// Alternates between strum up and strum down, velo converts to arpeggio delay
// Two variants are provided per chord, see tables below for details
//
// Built for SparkFun Midi Shield, use "Midi output" jumper setting

#include <avr/pgmspace.h>
#include <MIDI.h>
#include "noteList.h"   // From the Midi shield example, to compute interval

MIDI_CREATE_DEFAULT_INSTANCE();

#define MIDI_OUTPUT_CHANNEL 1

// -----------------------------------------------------------------------------

bool bChordReady = false;

MidiNoteList<16> midiNotes;
int numNotes = 0;

// Define chords: MAX_NOTES per chord, MAX_TYPES of variants, for
// MAX_STEPS * MAX_OCTAVES of scale root notes

#define MAX_NOTES 6

#define TYPE_MJ7  0
#define TYPE_7    1
#define TYPE_M7   2
#define TYPE_M9   3
#define TYPE_MJ13 4
#define MAX_TYPES 5
#define TYPE_NONE 255

#define MAX_STEPS    12
#define MAX_OCTAVES  2

// Split chord fingering to get more flexibility (note number 55 = g')
#define OCTAVE_SPLIT_NOTE  55

const byte chordFingering[MAX_STEPS*MAX_OCTAVES][MAX_TYPES][MAX_NOTES] PROGMEM = {
  // ----  maj7                      V 7                       m7                        m9                      maj13
  /*C */ { { 255, 3, 2, 0, 0, 0 },   { 255, 3, 2, 3, 1, 0 },   { 255, 255, 1, 3, 1, 3 }, { 255, 3, 0, 3, 4, 3 }, { 255, 3, 3, 2, 0, 0 } },
  /*Db*/ { { 1, 4, 3, 1, 1, 1 },     { 255, 4, 3, 4, 2, 255 }, { 255, 4, 2, 4, 5, 255 }, { 255, 4, 1, 1, 0, 0 }, { 1, 4, 4, 3, 1, 1 } },
  /*D */ { { 255, 5, 4, 2, 2, 2  },  { 255, 255, 0, 2, 1, 2 }, { 255, 255, 0, 2, 1, 1 }, { 255, 5, 3, 2, 1, 0 }, { 255, 5, 4, 6, 0, 3 } },
  /*Eb*/ { { 255, 255, 1, 3, 3, 3 }, { 255, 1, 1, 3, 2, 3 },   { 255, 255, 1, 3, 2, 2 }, { 1, 1, 1, 3, 2, 2 },   { 3, 3, 0, 0, 4, 4 } },
  /*E */ { { 0, 2, 1, 1, 0, 0  },    { 0, 2, 0, 1, 0, 0 },     { 0, 2, 2, 0, 3, 0 },     { 0, 5, 4, 0, 0, 0 },   { 0, 0, 1, 1, 2, 0 } },
  /*F */ { { 255, 0, 3, 2, 1, 0 },   { 1, 0, 1, 2, 1, 255 },   { 1, 3, 1, 1, 1, 1 },     { 1, 3, 1, 1, 1, 3 },   { 1, 0, 0, 3, 3, 0 } },
  /*Gb*/ { { 255, 255, 4, 3, 2, 1 }, { 255, 255, 4, 3, 2, 0 }, { 2, 4, 2, 2, 2, 2 },     { 2, 0, 2, 1, 2, 255 }, { 2, 1, 1, 4, 4, 1 } },
  /*G */ { { 3, 2, 0, 0, 0, 2 },     { 3, 2, 0, 0, 0, 1 },     { 1, 1, 0, 0, 3, 255 },   { 3, 0, 0, 3, 3, 1 },   { 3, 3, 4, 0, 0, 0 } },
  /*Ab*/ { { 255, 255, 1, 1, 1, 3 }, { 255, 255, 1, 1, 1, 2 }, { 255, 255, 1, 1, 0, 2 }, { 4, 6, 4, 4, 0, 6 },   { 4, 3, 3, 6, 6, 3 } },
  /*A */ { { 255, 0, 2, 1, 2, 0 },   { 255, 0, 2, 0, 2, 0 },   { 255, 0, 2, 0, 1, 0 },   { 5, 3, 5, 0, 0, 0 },   { 255, 0, 0, 1, 2, 2 } },
  /*Bb*/ { { 255, 1, 3, 2, 3, 1 },   { 255, 1, 3, 1, 3, 1 },   { 255, 1, 3, 1, 2, 1 },   { 6, 8, 6, 6, 6, 8 },   { 255, 1, 0, 1, 4, 3 } },
  /*H */ { { 255, 2, 4, 3, 4, 2 },   { 255, 2, 1, 2, 0, 2 },   { 255, 2, 4, 2, 3, 2 },   { 255, 2, 0, 2, 2, 2 }, { 255, 2, 6, 3, 4, 0 } },

  /*C */ { { 255, 3, 2, 4, 0, 3 },   { 8, 7, 8, 0, 5, 0 },     { 255, 3, 5, 3, 4, 6  },  { 8, 6, 0, 0, 8, 6 },   { 8, 8, 7, 5, 0, 0 } },
  /*Db*/ { { 255, 4, 6, 5, 6, 4 },   { 255, 4, 6, 4, 6, 4 },   { 255, 4, 6, 4, 5 ,4 },   { 255, 4, 6, 4, 4, 0 }, { 4, 4, 4, 5, 6, 6 } },
  /*D */ { { 255, 5, 0, 6, 7, 5 },   { 255, 255, 0, 5, 7, 5 }, { 255, 5, 7, 5, 6, 5 },   { 255, 8, 0, 5, 5, 5 }, { 255, 9, 0, 6, 8, 7 } },
  /*Eb*/ { { 255, 6, 5, 0, 3, 6 },   { 255, 6, 8, 6, 8, 6 },   { 255, 6, 8, 6, 7, 6 },   { 6, 9, 8, 8, 6, 9 },   { 255, 6, 10, 0, 9, 10 } },
  /*E */ { { 0, 7, 6, 4, 0, 0  },    { 0, 5, 0, 4, 0, 4 },     { 0, 5, 0, 0, 0, 3 },     { 0, 5, 4, 0, 0, 3 },   { 0, 0, 4, 6, 4, 4 } },
  /*F */ { { 255, 8, 7, 5, 5, 5 },   { 1, 0, 3, 2, 4, 255 },   { 255, 8, 6, 8, 6, 8 },   { 1, 3, 5, 0, 4, 4 },   { 1, 0, 0, 3, 5, 5 } },
  /*Gb*/ { { 255, 255, 4, 6, 6, 6 }, { 255, 255, 4, 6, 5, 6 }, { 255, 255, 4, 6, 5, 5 }, { 2, 0, 2, 1, 2, 255 }, { 2, 4, 2, 2, 5, 4 } },
  /*G */ { { 3, 2, 4, 0, 0, 3 },     { 3, 5, 3, 0, 0, 3 },     { 3, 5, 3, 3, 3 , 6 },    { 3, 0, 0, 0, 6, 6 },   { 3, 0, 4, 5, 0, 0 } },
  /*Ab*/ { { 255, 255, 6, 8, 8, 8 }, { 255, 255, 6, 5, 7, 4 }, { 4, 6, 4, 4, 4, 4 },     { 7, 6, 6, 8, 7, 6 },   { 4, 4, 5, 5, 6, 4 } },
  /*A */ { { 6, 0, 2, 2, 2, 4 },     { 255, 0, 2, 0, 2, 0 },   { 255, 0, 2, 2, 2, 3 },   { 5, 3, 5, 0, 0, 0 },   { 255, 0, 5, 5, 5, 7 } },
  /*Bb*/ { { 255, 1, 0, 3, 3, 5 },   { 255, 1, 0, 3, 3, 4 },   { 6, 8, 6, 6, 6, 6 },     { 6, 8, 8, 6, 9, 8 },   { 6, 5, 0, 0, 4, 5 } },
  /*H */ { { 255, 255, 9, 8, 7, 6 }, { 255, 2, 1, 4, 0, 5 },   { 7, 9, 7, 7, 7, 7 },     { 7, 9, 7, 7, 7, 9 },   { 7, 7, 8, 4, 4, 4 } }  
};

// For operation, convert the fingering into arrays of MIDI notes:

const byte chordTune[MAX_NOTES] = { 
  40, // E
  45, // A
  50, // d
  55, // g
  59, // h
  64  // e'
};

byte chordNotes[MAX_STEPS*MAX_OCTAVES][MAX_TYPES][MAX_NOTES];

byte chordRoot = 0;
byte chordVelo = 0;
byte chordType = 0;
byte aChordPitch[MAX_NOTES];

// Convert 2 tone interval into chord type
const byte interval2type[MAX_STEPS] = {
  TYPE_NONE,  // C-C:  0 is not possible
  TYPE_NONE,  // C-Db: 1 half-tone is impractical
  TYPE_M9,    // C-D:  second => m9 chord
  TYPE_M7,    // C-Eb: minor third => m7 chord
  TYPE_MJ7,   // C-E:  major third => Maj7 chord
  TYPE_MJ13,  // C-F:  fourth => Maj13 chord
  TYPE_NONE,  // C-Gb: unused
  TYPE_7,     // C-G:  fifth => V7 chord
  TYPE_NONE,  // C-Ab: unused
  TYPE_NONE,  // C-A:  unused  
  TYPE_NONE,  // C-Bb: unused
  TYPE_NONE   // C-B:  unused
};

// 8 steps for 3 highest bits of velo (velo>>4)
int delayTimes [8] = {
  250, 250, 200, 100, 50, 30, 15, 10
};
int strumDelay = 10;

bool bStrumDown = true;

// -----------------------------------------------------------------------------

void handleNoteOn(byte chan, byte pitch, byte velo)
{
  midiNotes.add(MidiNote(pitch,velo));
  numNotes++;

  if (2 == numNotes) {
    byte lowest = 0;
    byte highest = 0;
    midiNotes.getLow(lowest);
    midiNotes.getHigh(highest);
    
    chordRoot = lowest % MAX_STEPS;
    if (lowest >= OCTAVE_SPLIT_NOTE) {
      chordRoot += MAX_STEPS; // use next set of fingering
    }

    bChordReady = false;
    chordType = interval2type[(highest-lowest) % MAX_STEPS];
    if (TYPE_NONE != chordType ) {
      for (byte i=0; i<MAX_NOTES; i++) {
        aChordPitch[i] = chordNotes[chordRoot][chordType][i];
      }
      bChordReady = true;
    }
    
    chordVelo = velo;
    strumDelay = delayTimes[velo>>4];
  }
}


void handleNoteOff(byte chan, byte pitch, byte velo)
{
  bChordReady = false;
  
  midiNotes.remove(pitch);
  numNotes--;
  if (midiNotes.empty()) {
    numNotes = 0;
  }
}

// -----------------------------------------------------------------------------

inline void sendNoteStart( byte pitch ) {
  MIDI.sendNoteOn( pitch, chordVelo, MIDI_OUTPUT_CHANNEL );
}

inline void sendNoteEnd( byte pitch ) {
  MIDI.sendNoteOff( pitch, 0, MIDI_OUTPUT_CHANNEL );
}

// -----------------------------------------------------------------------------

void strumChord() {
  bool bHasChord4 = (0 != aChordPitch[4]);
  bool bHasChord5 = (0 != aChordPitch[5]);
  
  if (bStrumDown) {
    sendNoteStart( aChordPitch[0] );
    delay(strumDelay);
    sendNoteStart( aChordPitch[1] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[0] );
    sendNoteStart( aChordPitch[2] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[1] );
    sendNoteStart( aChordPitch[3] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[2] );

    if (bHasChord4) {
      sendNoteStart( aChordPitch[4] );
      delay(strumDelay);
    }
            sendNoteEnd( aChordPitch[3] );
    if (bHasChord5) {
      sendNoteStart( aChordPitch[5] );
      delay(strumDelay);
    }
    if (bHasChord4) {
            sendNoteEnd( aChordPitch[4] );
    }
    if (bHasChord5) {
            sendNoteEnd( aChordPitch[5] );
    }
    bStrumDown = false;
  } else {  // strum up
    if (bHasChord5) {
      sendNoteStart( aChordPitch[5] );
      delay(strumDelay);
    }
    if (bHasChord4) {
      sendNoteStart( aChordPitch[4] );
      delay(strumDelay);
    }
    if (bHasChord5) {
            sendNoteEnd( aChordPitch[5] );
    }
    sendNoteStart( aChordPitch[3] );
    delay(strumDelay);
    if (bHasChord4) {
            sendNoteEnd( aChordPitch[4] );
    }
    sendNoteStart( aChordPitch[2] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[3] );
    sendNoteStart( aChordPitch[1] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[2] );
    sendNoteStart( aChordPitch[0] );
    delay(strumDelay);
            sendNoteEnd( aChordPitch[1] );
            sendNoteEnd( aChordPitch[0] );
    bStrumDown = true;
  }
}

// -----------------------------------------------------------------------------

void setup()
{
  int i=0;  // index into the target notes array
  int n=0;  // index in the source fingering array
  for (int s=0; s<MAX_STEPS*MAX_OCTAVES; s++) {
    for (int t=0; t<MAX_TYPES; t++) {
      i=0;
      n=0;
      for (n=0; n<MAX_NOTES; n++) {
        byte f = pgm_read_byte( &(chordFingering[s][t][n]) );
        // copy only chords that are used, skip the rest (= set to 0 afterwards)
        if (255 != f) {
          chordNotes[s][t][i] = f + chordTune[n];
          i++;
        }
      }
      for (n=i; n<MAX_NOTES; n++) {
        chordNotes[s][t][n] = 0;
      }
    }
  }
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.turnThruOff();
  MIDI.begin();
}


void loop()
{
  MIDI.read();

  if (bChordReady) {
    strumChord();
    bChordReady = false;  
  }
}

// EOF
// -----------------------------------------------------------------------------

