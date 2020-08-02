/*  h9_modules.h
    This file is part of libh9, a library for remotely managing Eventide H9
    effects pedals.

    Copyright (C) 2020 Daniel Collins

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef algorithms_h
#define algorithms_h

#include "libh9.h"

h9_module modules[] = {{"TimeFactor",
                        1,
                        0,
                        {{0, 0, "Digital Delay", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Xfade", "Mod Depth", "Mod Speed", "Filter", "Repeat"},
                         {1, 0, "Vintage Delay", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Bits", "Mod Depth", "Mod Speed", "Filter", "Repeat"},
                         {2, 0, "Tape Echo", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Saturation", "Wow", "Flutter", "Filter", "Repeat"},
                         {3, 0, "Mod Delay", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Mod Shape", "Mod Depth", "Mod Speed", "Filter", "Repeat"},
                         {4, 0, "Ducked Delay", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Ratio", "Threshold", "Release", "Filter", "Repeat"},
                         {5, 0, "Band Delay", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Resonance", "Mod Depth", "Mod Speed", "Filter", "Repeat"},
                         {6, 0, "Filter Pong", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Slur", "Mod Shape", "Mod Depth", "Mod Speed", "Filter", "Repeat"},
                         {7, 0, "MultiTap", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Slur", "Taper", "Spread", "Filter", "Repeat"},
                         {8, 0, "Reverse", "Mix", "Delay Mix", "Delay A", "Delay B", "Fdbk A", "Fdbk B", "Xfade", "Depth Mod", "Speed", "Filter", "Repeat"},
                         {9, 0, "Looper", "Mix", "Max Length", "Ply-Start", "Ply-Length", "Decay", "Dubmode", "Playmode", "Resolution", "Rec-Speed", "Filter", "Repeat"}},
                        10},
                       {"ModFactor",
                        2,
                        0,
                        {{0, 1, "Chorus", "Intensity", "Type", "Depth", "Speed", "Shape", "Filter", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {1, 1, "Phaser", "Intensity", "Type", "Depth", "Speed", "Shape", "Stages", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {2, 1, "Q-Wah", "Q-Intensity", "Type", "Vowel", "Speed", "Shape", "Bottom", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {3, 1, "Flanger", "Intensity", "Type", "Depth", "Speed", "Shape", "Modfy Dly O/P", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {4, 1, "ModFilter", "Intensity", "Type", "Depth", "Sensitivity", "Shape", "Width", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {5, 1, "Rotary", "Mix", "Type", "Rotor Spd", "Horn Spd", "Rot/Hrn Mix", "Tone", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {6, 1, "TremoloPan", "Edge", "Type", "Depth", "Speed", "Shape", "Width", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {7, 1, "Vibrato", "Intensity", "Type", "Depth", "Speed", "Shape", "Width", "Depth Mod", "Speed Mod", "Mod Sens", "Mod Source", "Slow/Fast"},
                         {8, 1, "Undulator", "Intensity", "Type", "Depth", "Speed", "Shape", "Feedback", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"},
                         {9, 1, "RingMod", "Intensity", "Type", "Un-Used", "Speed", "Shape", "Tone", "Depth Mod", "Speed Mod", "Mod Rate", "Mod Source", "Slow/Fast"}},
                        10},
                       {"PitchFactor",
                        3,
                        0,
                        {{0, 2, "Diatonic", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Delay A", "Delay B", "Key", "Scale", "Feedback A", "Feedback B", "Flex"},
                         {1, 2, "Quadravox", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Delay D", "Delay Grp", "Key", "Mode", "Pitch C", "Pitch D", "Flex"},
                         {2, 2, "HarModulator", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Delay A", "Delay B", "Mod Depth", "Mod Sens", "Shape", "Feedback", "Flex"},
                         {3, 2, "MicroPitch", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Delay A", "Delay B", "Mod Depth", "Mod Rate", "Feedback", "Tone", "Flex"},
                         {4, 2, "H910 949", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Delay A", "Delay B", "Type", "Pitch Cntrl", "Feedback A", "Feedback B", "Flex"},
                         {5, 2, "PitchFlex", "Mix", "Pitch Mix", "Heel A", "Heel B", "H-T Gliss", "T-H Gliss", "LP Filter", "Shape", "Toe A", "Toe B", "Flex"},
                         {6, 2, "Octaver", "Mix", "Pitch Mix", "Filter A", "Filter B", "Resnce A", "Resnce B", "Envelope", "Sensitivity", "Fuzz", "Oct-Fuzz Mix", "Flex"},
                         {7, 2, "Crystals", "Mix", "Pitch Mix", "Pitch A", "Pitch B", "Rev Delay A", "Rev Delay B", "Verb Mix", "Verb Decay", "Feedback A", "Feedback B", "Flex"},
                         {8, 2, "HarPeggiator", "Mix", "Arp Mix", "Sequence A", "Sequence B", "Rhythm A", "Rhythm B", "Dynamics", "Length", "Effects A", "Effects B", "Flex"},
                         {9, 2, "Synthonizer", "Mix", "Vox Mix", "Wave Mix A", "Octave B", "Attack A", "Attack B", "Verb Level", "Verb Decay", "Shape A", "Sweep B", "Flex"}},
                        10},
                       {"Space",
                        4,
                        0,
                        {{0, 3, "Hall", "Mix", "Decay", "Size", "Pre Delay", "Low EQ", "High EQ", "Low Decay", "High Decay", "Mod-Level", "Mid EQ", "HotSwitch"},
                         {1, 3, "Room", "Mix", "Decay", "Size", "Pre Delay", "Low EQ", "High EQ", "Reflection", "Diffusion", "Mod-Level", "High Freq", "HotSwitch"},
                         {2, 3, "Plate", "Mix", "Decay", "Size", "Pre Delay", "Low-damp", "High-damp", "Distance", "Diffusion", "Mod Level", "Tone", "HotSwitch"},
                         {3, 3, "Spring", "Mix", "Decay", "Tension", "Num Springs", "Low-damp", "High-damp", "Reflection", "Trem Inten", "Trem Speed", "Resonance", "HotSwitch"},
                         {4, 3, "DualVerb", "Mix", "A-Decay", "Size", "A Predelay", "A Tone", "B Tone", "B Decay", "B Predelay", "ABMix", "Resonance", "HotSwitch"},
                         {5, 3, "Reverse Reverb", "Mix", "Decay", "Size", "Feedback", "Low EQ", "High EQ", "Late Dry", "Diffusion", "Mod Level", "Contour", "HotSwitch"},
                         {6, 3, "ModEchoVerb", "Mix", "Decay", "Size", "Echo", "Low EQ", "High EQ", "Echo Fdbk", "Modrate", "Chorus Mix", "Echotone", "HotSwitch"},
                         {7, 3, "Blackhole", "Mix", "Gravity", "Size", "Pre Delay", "Low EQ", "High EQ", "Mod-Depth", "Modrate", "Feedback", "Resonance", "HotSwitch"},
                         {8, 3, "MangledVerb", "Mix", "Decay", "Size", "Pre Delay", "Low EQ", "High EQ", "Overdrive", "Output", "Wobble", "Mid EQ", "HotSwitch"},
                         {9, 3, "TremoloVerb", "Mix", "Decay", "Size", "Pre Delay", "Low EQ", "High EQ", "Shape", "Speed", "Mono Depth", "High Freq", "HotSwitch"},
                         {10, 3, "DynaVerb", "Mix", "Decay", "Size", "Attack", "Low EQ", "High EQ", "Omni-Ratio", "Release", "Threshold", "Sidechain", "HotSwitch"},
                         {11, 3, "Shimmer", "Mix", "Decay", "Size", "Delay", "Low-decay", "High-decay", "A-Pitch", "B-Pitch", "Pitch-Decay", "Mid-Decay", "HotSwitch"}},
                        12},
                       {"H9",
                        5,
                        0,
                        {{0, 4, "UltraTap", "Mix", "Length", "Taps", "Pre-Delay", "Spread", "Taper", "Tone", "Slurm", "Chop", "Un-Used", "HotSwitch"},
                         {1, 4, "Resonator", "Mix", "Length", "Rhythm", "Feedback", "Resonance", "Reverb", "Note 1", "Note 2", "Note 3", "Note 4", "HotSwitch"},
                         {2, 4, "EQ Compressor", "Gain 1", "Frequency 1", "Width 1", "Gain 2", "Frequency 2", "Width 2", "Bass", "Treble", "Compressor", "Trim", "HotSwitch"},
                         {3, 4, "SpaceTime", "Mix", "Mod Amt", "Mod Rate", "Verb Lvl", "Decay", "Color", "Delay Lvl", "Delay A", "Delay B", "Feedback", "HotSwitch"},
                         {4, 4, "Sculpt", "Mix", "Bandmix", "XOver", "Low-Drive", "High-Drive", "Compressor", "Low Boost", "Filter-Pre", "Filter-Post", "Env-Filter", "HotSwitch"},
                         {5, 4, "CrushStation", "Mix", "Drive", "Sustain", "Sag", "Octaves", "Grit", "Bass", "Mids", "Mids Freq", "Treble", "HotSwitch"},
                         {6, 4, "PitchFuzz", "Fuzz", "Fuzz Tone", "Pitch Amt", "Pitch A", "Pitch B", "Pitch C", "Arp Level", "Delay B", "Delay C", "Feedback", "HotSwitch"},
                         {7, 4, "HotSawz", "Mix", "Osc Depth", "Cutoff", "Resonance", "LFO/Speed", "LFO Amount", "Attack", "Decay", "Gate", "Envelope", "HotSwitch"},
                         {8, 4, "Harmadillo", "Depth", "Rate", "Shape", "X-Over", "X-Overlap", "Drive", "Env Depth", "Env Rate", "Env X-Over", "Tone", "HotSwitch"},
                         {9, 4, "Tricerachorus", "Ch/Vib Mix", "Rate", "Depth L", "Depth C", "Depth R", "Delay", "Detune Mix", "Detune", "Env Mix/Rate", "Tone", "HotSwitch"}},
                        10}};

#endif /* algorithms_h */
