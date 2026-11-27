#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

// Values are in real units (Hz, seconds, %, dB, octave, voices, delay-time index).
struct Preset
{
    const char* category;
    const char* name;
    std::vector<std::pair<const char*, float>> values; // only what differs from Init
};

// Delay time index: 0=1/16 1=1/8 2=1/8D 3=1/4 4=1/4D 5=1/2
inline const std::vector<Preset>& getPresets()
{
    static const std::vector<Preset> presets
    {
        { "INIT", "Init", {} },

        // ---------- PADS ----------
        { "PAD", "Warm Strings", { {"saw",0.8f},{"square",0.0f},{"voices",6},{"detune",30},{"spread",80},
                                   {"cutoff",3000},{"reso",10},{"envamt",0},{"attack",0.8f},{"decay",1.0f},{"sustain",85},{"release",1.8f},
                                   {"dtime",3},{"dfb",35},{"dmix",15},{"rsize",85},{"rmix",45} } },
        { "PAD", "Hollow Glass", { {"saw",0.0f},{"square",0.8f},{"pw",30},{"voices",4},{"detune",20},{"spread",70},
                                   {"cutoff",2200},{"reso",20},{"attack",1.2f},{"decay",1.5f},{"sustain",80},{"release",2.5f},
                                   {"dtime",4},{"dfb",40},{"dmix",20},{"rsize",90},{"rmix",50} ,{"volume",-3}} },
        { "PAD", "Dream Wash",   { {"saw",0.7f},{"square",0.4f},{"pw",45},{"voices",8},{"detune",45},{"spread",100},
                                   {"cutoff",5000},{"reso",5},{"attack",2.0f},{"decay",1.0f},{"sustain",100},{"release",3.5f},
                                   {"dtime",5},{"dfb",50},{"dmix",25},{"rsize",100},{"rmix",60},{"volume",3} } },

        // ---------- PLUCKS ----------
        { "PLUCK", "Trance Pluck", { {"saw",0.8f},{"voices",5},{"detune",25},{"spread",70},
                                     {"cutoff",350},{"reso",25},{"envamt",70},{"attack",0.001f},{"decay",0.25f},{"sustain",0},{"release",0.25f},
                                     {"dtime",2},{"dfb",40},{"dmix",30},{"rsize",60},{"rmix",25} } },
        { "PLUCK", "Square Bell",  { {"saw",0.0f},{"square",0.8f},{"pw",25},{"octave",1},{"voices",2},{"detune",10},{"spread",50},
                                     {"cutoff",700},{"reso",30},{"envamt",60},{"attack",0.001f},{"decay",0.4f},{"sustain",0},{"release",0.4f},
                                     {"dtime",1},{"dfb",35},{"dmix",25},{"rsize",70},{"rmix",30} ,{"volume",-2}} },
        { "PLUCK", "Soft Keys",    { {"saw",0.5f},{"square",0.5f},{"pw",50},{"voices",3},{"detune",12},{"spread",60},
                                     {"cutoff",600},{"reso",10},{"envamt",50},{"attack",0.002f},{"decay",0.8f},{"sustain",20},{"release",0.5f},
                                     {"dtime",3},{"dfb",30},{"dmix",15},{"rsize",50},{"rmix",25} ,{"volume",3}} },

        // ---------- LEADS ----------
        { "LEAD", "Supersaw Lead", { {"saw",1.0f},{"voices",8},{"detune",40},{"spread",90},
                                     {"cutoff",9000},{"reso",5},{"attack",0.005f},{"decay",0.3f},{"sustain",100},{"release",0.3f},
                                     {"dtime",2},{"dfb",35},{"dmix",25},{"rsize",60},{"rmix",25},{"volume",0} } },
        { "LEAD", "Square Lead",   { {"saw",0.0f},{"square",0.9f},{"pw",40},{"voices",2},{"detune",8},{"spread",40},
                                     {"cutoff",4000},{"reso",15},{"attack",0.003f},{"decay",0.2f},{"sustain",90},{"release",0.2f},
                                     {"dtime",1},{"dfb",30},{"dmix",20},{"rsize",50},{"rmix",15} ,{"volume",-3}} },
        { "LEAD", "Bright Saw",    { {"saw",0.9f},{"square",0.3f},{"voices",3},{"detune",15},{"spread",50},
                                     {"cutoff",2500},{"reso",30},{"envamt",30},{"attack",0.003f},{"decay",0.4f},{"sustain",80},{"release",0.25f},
                                     {"dtime",3},{"dfb",35},{"dmix",30},{"rsize",55},{"rmix",20} ,{"volume",3}} },

        // ---------- BASSES ----------
        { "BASS", "Saw Bass",      { {"saw",1.0f},{"octave",-1},{"voices",1},{"cutoff",900},{"reso",20},{"envamt",40},
                                     {"attack",0.001f},{"decay",0.3f},{"sustain",70},{"release",0.08f},{"rmix",0} } },
        { "BASS", "Square Sub",    { {"saw",0.0f},{"square",1.0f},{"pw",50},{"octave",-2},{"voices",1},{"cutoff",400},{"reso",0},{"envamt",10},
                                     {"attack",0.002f},{"decay",0.2f},{"sustain",100},{"release",0.08f},{"rmix",0} ,{"volume",-2}} },
        { "BASS", "Reese",         { {"saw",1.0f},{"octave",-1},{"voices",4},{"detune",20},{"spread",30},{"cutoff",1500},{"reso",15},{"envamt",10},
                                     {"attack",0.003f},{"decay",0.3f},{"sustain",100},{"release",0.15f},{"rmix",5} } },
    };
    return presets;
}
