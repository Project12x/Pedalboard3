/**
 * @file tone_generator_test.cpp
 * @brief Headless tests for ToneGenerator frequency/pitch math
 *
 * These tests verify the mathematical correctness of:
 * 1. MIDI note to frequency conversion
 * 2. Frequency to MIDI note conversion
 * 3. Cents calculation (boundary conditions)
 *
 * NOTE: These are pure math tests - no JUCE AudioProcessor instantiation
 * to avoid needing the full app dependencies.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

// ============================================================================
// Standalone Frequency Math Functions (mirrors ToneGeneratorProcessor)
// ============================================================================

constexpr float A4_FREQ = 440.0f;
constexpr int A4_MIDI = 69;

float midiNoteToFrequency(int midiNote)
{
    return A4_FREQ * std::pow(2.0f, (midiNote - A4_MIDI) / 12.0f);
}

int frequencyToMidiNote(float frequency)
{
    if (frequency <= 0.0f)
        return -1;
    return static_cast<int>(std::round(12.0f * std::log2(frequency / A4_FREQ) + A4_MIDI));
}

float frequencyToCents(float frequency, int targetNote)
{
    float targetFreq = midiNoteToFrequency(targetNote);
    return 1200.0f * std::log2(frequency / targetFreq);
}

// ============================================================================
// Frequency Conversion Tests
// ============================================================================

TEST_CASE("MIDI note to frequency - Standard Notes", "[tonegen][math]")
{
    SECTION("A4 = 440Hz")
    {
        float freq = midiNoteToFrequency(69);
        REQUIRE_THAT(freq, WithinAbs(440.0f, 0.01f));
    }

    SECTION("A3 = 220Hz (octave below)")
    {
        float freq = midiNoteToFrequency(57);
        REQUIRE_THAT(freq, WithinAbs(220.0f, 0.01f));
    }

    SECTION("A5 = 880Hz (octave above)")
    {
        float freq = midiNoteToFrequency(81);
        REQUIRE_THAT(freq, WithinAbs(880.0f, 0.01f));
    }

    SECTION("C4 (middle C) = 261.63Hz")
    {
        float freq = midiNoteToFrequency(60);
        REQUIRE_THAT(freq, WithinAbs(261.63f, 0.1f));
    }

    SECTION("A0 = 27.5Hz (lowest piano A)")
    {
        float freq = midiNoteToFrequency(21);
        REQUIRE_THAT(freq, WithinAbs(27.5f, 0.01f));
    }

    SECTION("C8 = 4186Hz (near top of piano)")
    {
        float freq = midiNoteToFrequency(108);
        REQUIRE_THAT(freq, WithinAbs(4186.01f, 1.0f));
    }
}

TEST_CASE("Frequency to MIDI note", "[tonegen][math]")
{
    SECTION("440Hz = A4 (MIDI 69)")
    {
        int note = frequencyToMidiNote(440.0f);
        REQUIRE(note == 69);
    }

    SECTION("220Hz = A3 (MIDI 57)")
    {
        int note = frequencyToMidiNote(220.0f);
        REQUIRE(note == 57);
    }

    SECTION("Invalid frequency returns -1")
    {
        REQUIRE(frequencyToMidiNote(0.0f) == -1);
        REQUIRE(frequencyToMidiNote(-100.0f) == -1);
    }
}

// ============================================================================
// BOUNDARY CONDITION TESTS
// These are the critical tests that verify tool accuracy rather than
// guaranteed-to-pass trivial cases.
// ============================================================================

TEST_CASE("Frequency to MIDI note - Boundary at semitone", "[tonegen][boundary]")
{
    // A4 = 440Hz, A#4 = 466.16Hz
    // Boundary is at 50 cents from either note

    SECTION("452Hz is ~47 cents sharp of A4 -> should round to A4")
    {
        // 452Hz = 440 * 2^(x/1200), solve: x = 1200 * log2(452/440) = 46.5 cents
        int note = frequencyToMidiNote(452.0f);
        REQUIRE(note == 69); // Still A4
    }

    SECTION("453Hz is ~51 cents sharp of A4 -> should round to A#4")
    {
        // 453Hz = 50.4 cents sharp, rounds to A#4
        int note = frequencyToMidiNote(453.0f);
        REQUIRE(note == 70); // A#4
    }

    SECTION("428Hz is ~47 cents flat of A4 -> should round to A4")
    {
        int note = frequencyToMidiNote(428.0f);
        REQUIRE(note == 69); // Still A4
    }

    SECTION("427Hz is ~52 cents flat of A4 -> should round to G#4")
    {
        int note = frequencyToMidiNote(427.0f);
        REQUIRE(note == 68); // G#4
    }
}

TEST_CASE("Cents calculation - Boundary values", "[tonegen][boundary]")
{
    SECTION("Exact match = 0 cents")
    {
        float cents = frequencyToCents(440.0f, 69);
        REQUIRE_THAT(cents, WithinAbs(0.0f, 0.1f));
    }

    SECTION("+99 cents (near boundary)")
    {
        // 99 cents sharp of A4: 440 * 2^(99/1200) = 466.03 Hz
        float sharpFreq = 440.0f * std::pow(2.0f, 99.0f / 1200.0f);
        float cents = frequencyToCents(sharpFreq, 69);
        REQUIRE_THAT(cents, WithinAbs(99.0f, 0.5f));
    }

    SECTION("-99 cents (near boundary)")
    {
        // 99 cents flat of A4
        float flatFreq = 440.0f * std::pow(2.0f, -99.0f / 1200.0f);
        float cents = frequencyToCents(flatFreq, 69);
        REQUIRE_THAT(cents, WithinAbs(-99.0f, 0.5f));
    }

    SECTION("+50 cents = quarter tone")
    {
        float quarterToneFreq = 440.0f * std::pow(2.0f, 50.0f / 1200.0f);
        float cents = frequencyToCents(quarterToneFreq, 69);
        REQUIRE_THAT(cents, WithinAbs(50.0f, 0.5f));
    }

    SECTION("-50 cents = quarter tone flat")
    {
        float flatFreq = 440.0f * std::pow(2.0f, -50.0f / 1200.0f);
        float cents = frequencyToCents(flatFreq, 69);
        REQUIRE_THAT(cents, WithinAbs(-50.0f, 0.5f));
    }
}

TEST_CASE("Edge cases for tuner display", "[tonegen][boundary]")
{
    SECTION("+100 cents should equal +1 semitone frequency")
    {
        // A4 + 100 cents = A#4 frequency
        float expectedAsSharpFreq = 440.0f * std::pow(2.0f, 100.0f / 1200.0f);
        float aSharp4 = midiNoteToFrequency(70); // A#4

        REQUIRE_THAT(expectedAsSharpFreq, WithinAbs(aSharp4, 0.01f));
    }

    SECTION("Frequency exactly between notes")
    {
        // Exactly 50 cents sharp of A4
        float midPoint = 440.0f * std::pow(2.0f, 50.0f / 1200.0f);

        // Could round either way - implementation dependent
        int note = frequencyToMidiNote(midPoint);
        // With standard rounding, 50 cents should round UP
        REQUIRE((note == 69 || note == 70)); // Either is acceptable
    }
}

TEST_CASE("Octave consistency", "[tonegen][math]")
{
    SECTION("Each octave doubles frequency")
    {
        float a4 = midiNoteToFrequency(69);
        float a5 = midiNoteToFrequency(81);
        float a3 = midiNoteToFrequency(57);

        REQUIRE_THAT(a5, WithinAbs(a4 * 2.0f, 0.01f));
        REQUIRE_THAT(a3, WithinAbs(a4 / 2.0f, 0.01f));
    }

    SECTION("12 semitones = 1 octave = 2x frequency")
    {
        for (int baseNote = 36; baseNote <= 84; baseNote += 12)
        {
            float baseFreq = midiNoteToFrequency(baseNote);
            float octaveUp = midiNoteToFrequency(baseNote + 12);
            REQUIRE_THAT(octaveUp, WithinAbs(baseFreq * 2.0f, 0.1f));
        }
    }
}

TEST_CASE("Roundtrip MIDI->Freq->MIDI", "[tonegen][math]")
{
    // Every MIDI note should survive a roundtrip
    for (int midiNote = 24; midiNote <= 108; ++midiNote)
    {
        float freq = midiNoteToFrequency(midiNote);
        int recovered = frequencyToMidiNote(freq);
        REQUIRE(recovered == midiNote);
    }
}
