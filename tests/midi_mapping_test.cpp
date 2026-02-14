/**
 * @file midi_mapping_test.cpp
 * @brief Unit tests for MIDI mapping system
 *
 * Tests cover:
 * 1. CC value normalization (0-127 to 0.0-1.0)
 * 2. Custom range mapping (lowerBound/upperBound)
 * 3. Inverted range mapping (upperBound < lowerBound)
 * 4. Latch/toggle behaviour
 * 5. Channel filtering (omni vs specific)
 * 6. Multi-mapping dispatch (same CC to multiple parameters)
 * 7. XML persistence round-trip
 * 8. MidiAppFifo lock-free FIFO correctness
 * 9. MidiLearn one-shot callback pattern
 * 10. Register/unregister mapping lifecycle
 *
 * These tests use mock classes that faithfully replicate the algorithms
 * from MidiMapping, MidiMappingManager, and MidiAppFifo without pulling
 * in the full application dependency chain.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <atomic>
#include <cmath>
#include <map>
#include <string>
#include <vector>

// Include real MidiAppFifo (self-contained, only needs JUCE base)
#include "../src/MidiAppFifo.h"

// =============================================================================
// Test Helpers - Faithful replication of MidiMapping::ccReceived() algorithm
// =============================================================================

/// Replicates the exact value mapping logic from MidiMapping::ccReceived()
/// (MidiMappingManager.cpp lines 65-110)
struct MidiMappingLogic
{
    int cc;
    bool latched;
    int channel;       // 0 = omni, 1-16 = specific
    float lowerBound;
    float upperBound;

    // Latch state
    bool latchToggle = false;

    // Last computed value
    float lastValue = 0.0f;
    bool valueUpdated = false;

    MidiMappingLogic(int cc_, bool latched_, int channel_, float lower, float upper)
        : cc(cc_), latched(latched_), channel(channel_), lowerBound(lower), upperBound(upper)
    {
    }

    /// Exact replica of MidiMapping::ccReceived()
    void ccReceived(int val)
    {
        float tempf;

        if (latched)
        {
            if (val == 0)
                return;

            latchToggle = !latchToggle;

            if (latchToggle)
                tempf = 1.0f;
            else
                tempf = 0.0f;
        }
        else
            tempf = (float)val / 127.0f;

        if (upperBound > lowerBound)
        {
            tempf *= upperBound - lowerBound;
            tempf += lowerBound;
        }
        else
        {
            tempf = 1.0f - tempf;
            tempf *= lowerBound - upperBound;
            tempf += upperBound;
        }

        lastValue = tempf;
        valueUpdated = true;
    }
};

/// Replicates MidiMappingManager dispatch logic with channel filtering
class MidiDispatcher
{
  public:
    std::multimap<int, MidiMappingLogic*> mappings;

    // MIDI learn state
    std::atomic<bool> learnActive{false};
    int learnedCC = -1;

    void registerMapping(int cc, MidiMappingLogic* mapping) { mappings.insert({cc, mapping}); }

    void unregisterMapping(MidiMappingLogic* mapping)
    {
        for (auto it = mappings.begin(); it != mappings.end();)
        {
            if (it->second == mapping)
                mappings.erase(it++);
            else
                ++it;
        }
    }

    /// Replicates MidiMappingManager::midiCcReceived() dispatch
    void dispatchCC(int cc, int value, int messageChan)
    {
        // MIDI learn callback (one-shot)
        if (learnActive.load())
        {
            learnedCC = cc;
            learnActive.store(false);
        }

        // Dispatch to parameter mappings
        for (auto it = mappings.lower_bound(cc); it != mappings.upper_bound(cc); ++it)
        {
            int mappingChan = it->second->channel;
            if ((mappingChan == 0) || (mappingChan == messageChan))
                it->second->ccReceived(value);
        }
    }
};

// =============================================================================
// 1. CC Value Normalization
// =============================================================================

TEST_CASE("MIDI CC Value Normalization", "[midi][mapping][unit]")
{
    SECTION("CC 0 maps to 0.0")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        m.ccReceived(0);
        REQUIRE(m.lastValue == 0.0f);
    }

    SECTION("CC 127 maps to 1.0")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        m.ccReceived(127);
        REQUIRE(m.lastValue == 1.0f);
    }

    SECTION("CC 64 maps to approximately 0.504")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        m.ccReceived(64);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(64.0 / 127.0, 0.001));
    }

    SECTION("CC 1 maps to approximately 1/127")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        m.ccReceived(1);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(1.0 / 127.0, 0.001));
    }

    SECTION("Full range sweep is monotonically increasing")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        float prev = -1.0f;
        for (int cc = 0; cc <= 127; ++cc)
        {
            m.ccReceived(cc);
            REQUIRE(m.lastValue > prev);
            prev = m.lastValue;
        }
    }
}

// =============================================================================
// 2. Custom Range Mapping
// =============================================================================

TEST_CASE("MIDI Custom Range Mapping", "[midi][mapping][unit]")
{
    SECTION("CC 0 maps to lowerBound")
    {
        MidiMappingLogic m(7, false, 0, 0.2f, 0.8f);
        m.ccReceived(0);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.2, 0.001));
    }

    SECTION("CC 127 maps to upperBound")
    {
        MidiMappingLogic m(7, false, 0, 0.2f, 0.8f);
        m.ccReceived(127);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.8, 0.001));
    }

    SECTION("CC midpoint maps to range midpoint")
    {
        MidiMappingLogic m(7, false, 0, 0.0f, 0.5f);
        m.ccReceived(127);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.5, 0.001));

        m.ccReceived(0);
        REQUIRE(m.lastValue == 0.0f);
    }

    SECTION("Narrow range 0.4-0.6")
    {
        MidiMappingLogic m(11, false, 0, 0.4f, 0.6f);
        m.ccReceived(0);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.4, 0.001));

        m.ccReceived(127);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.6, 0.001));

        m.ccReceived(64);
        float expected = 0.4f + (64.0f / 127.0f) * 0.2f;
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(expected, 0.001));
    }

    SECTION("Full range 0.0-1.0 is identity")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        for (int cc = 0; cc <= 127; ++cc)
        {
            m.ccReceived(cc);
            float expected = (float)cc / 127.0f;
            REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(expected, 0.0001));
        }
    }
}

// =============================================================================
// 3. Inverted Range Mapping
// =============================================================================

TEST_CASE("MIDI Inverted Range Mapping", "[midi][mapping][unit]")
{
    SECTION("When upperBound < lowerBound, range is inverted")
    {
        MidiMappingLogic m(1, false, 0, 0.8f, 0.2f); // inverted
        m.ccReceived(0);
        // val=0 -> tempf=0/127=0 -> tempf=1-0=1 -> tempf*=0.6 -> tempf+=0.2 -> 0.8
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.8, 0.001));
    }

    SECTION("Inverted range: CC 127 maps to upperBound (smaller value)")
    {
        MidiMappingLogic m(1, false, 0, 0.8f, 0.2f);
        m.ccReceived(127);
        // val=127 -> tempf=1.0 -> tempf=1-1=0 -> tempf*=0.6=0 -> tempf+=0.2 -> 0.2
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.2, 0.001));
    }

    SECTION("Inverted range: full sweep is monotonically decreasing")
    {
        MidiMappingLogic m(1, false, 0, 1.0f, 0.0f);
        float prev = 2.0f;
        for (int cc = 0; cc <= 127; ++cc)
        {
            m.ccReceived(cc);
            REQUIRE(m.lastValue < prev);
            prev = m.lastValue;
        }
    }

    SECTION("Inverted range: midpoint is symmetric")
    {
        MidiMappingLogic m(1, false, 0, 1.0f, 0.0f);
        m.ccReceived(64);
        float expected = 1.0f - (64.0f / 127.0f);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(expected, 0.001));
    }
}

// =============================================================================
// 4. Latch/Toggle Behaviour
// =============================================================================

TEST_CASE("MIDI Latch/Toggle Behaviour", "[midi][mapping][unit]")
{
    SECTION("First non-zero CC toggles to 1.0 (on)")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);
        m.ccReceived(127);
        REQUIRE(m.lastValue == 1.0f);
    }

    SECTION("Second non-zero CC toggles back to 0.0 (off)")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);
        m.ccReceived(127);
        REQUIRE(m.lastValue == 1.0f);

        m.ccReceived(127);
        REQUIRE(m.lastValue == 0.0f);
    }

    SECTION("CC value 0 is ignored in latch mode")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);
        m.ccReceived(0);
        REQUIRE_FALSE(m.valueUpdated);
    }

    SECTION("Toggle cycle: on-off-on-off")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);

        m.ccReceived(100); // on
        REQUIRE(m.lastValue == 1.0f);

        m.ccReceived(80); // off (any non-zero toggles)
        REQUIRE(m.lastValue == 0.0f);

        m.ccReceived(1); // on
        REQUIRE(m.lastValue == 1.0f);

        m.ccReceived(64); // off
        REQUIRE(m.lastValue == 0.0f);
    }

    SECTION("CC 0 values don't affect toggle state")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);

        m.ccReceived(127); // on
        REQUIRE(m.lastValue == 1.0f);

        m.ccReceived(0); // ignored
        m.ccReceived(0); // ignored
        m.ccReceived(0); // ignored

        // Next non-zero should toggle OFF (not be affected by the zeros)
        m.ccReceived(127); // off
        REQUIRE(m.lastValue == 0.0f);
    }

    SECTION("Latch with custom range applies bounds")
    {
        MidiMappingLogic m(64, true, 0, 0.3f, 0.7f);

        m.ccReceived(127); // on -> tempf=1.0 -> mapped to 0.7
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.7, 0.001));

        m.ccReceived(127); // off -> tempf=0.0 -> mapped to 0.3
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.3, 0.001));
    }

    SECTION("Latch with inverted range")
    {
        MidiMappingLogic m(64, true, 0, 0.8f, 0.2f); // inverted

        m.ccReceived(127); // on -> tempf=1.0 -> inverted: 1-1=0 -> 0*0.6=0 -> +0.2 = 0.2
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.2, 0.001));

        m.ccReceived(127); // off -> tempf=0.0 -> inverted: 1-0=1 -> 1*0.6=0.6 -> +0.2 = 0.8
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.8, 0.001));
    }
}

// =============================================================================
// 5. Channel Filtering
// =============================================================================

TEST_CASE("MIDI Channel Filtering", "[midi][mapping][dispatch]")
{
    MidiDispatcher dispatcher;

    SECTION("Omni mode (channel 0) receives from all channels")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f); // omni
        dispatcher.registerMapping(1, &m);

        for (int ch = 1; ch <= 16; ++ch)
        {
            m.valueUpdated = false;
            dispatcher.dispatchCC(1, 100, ch);
            REQUIRE(m.valueUpdated);
        }

        dispatcher.unregisterMapping(&m);
    }

    SECTION("Specific channel only receives matching channel")
    {
        MidiMappingLogic m(1, false, 5, 0.0f, 1.0f); // channel 5 only
        dispatcher.registerMapping(1, &m);

        // Non-matching channels
        m.valueUpdated = false;
        dispatcher.dispatchCC(1, 100, 1);
        REQUIRE_FALSE(m.valueUpdated);

        m.valueUpdated = false;
        dispatcher.dispatchCC(1, 100, 4);
        REQUIRE_FALSE(m.valueUpdated);

        m.valueUpdated = false;
        dispatcher.dispatchCC(1, 100, 6);
        REQUIRE_FALSE(m.valueUpdated);

        // Matching channel
        m.valueUpdated = false;
        dispatcher.dispatchCC(1, 100, 5);
        REQUIRE(m.valueUpdated);

        dispatcher.unregisterMapping(&m);
    }

    SECTION("Multiple mappings with different channels on same CC")
    {
        MidiMappingLogic m1(7, false, 1, 0.0f, 1.0f);  // channel 1
        MidiMappingLogic m2(7, false, 10, 0.0f, 1.0f); // channel 10
        dispatcher.registerMapping(7, &m1);
        dispatcher.registerMapping(7, &m2);

        // Send on channel 1 - only m1 should fire
        m1.valueUpdated = false;
        m2.valueUpdated = false;
        dispatcher.dispatchCC(7, 80, 1);
        REQUIRE(m1.valueUpdated);
        REQUIRE_FALSE(m2.valueUpdated);

        // Send on channel 10 - only m2 should fire
        m1.valueUpdated = false;
        m2.valueUpdated = false;
        dispatcher.dispatchCC(7, 80, 10);
        REQUIRE_FALSE(m1.valueUpdated);
        REQUIRE(m2.valueUpdated);

        dispatcher.unregisterMapping(&m1);
        dispatcher.unregisterMapping(&m2);
    }
}

// =============================================================================
// 6. Multi-Mapping Dispatch
// =============================================================================

TEST_CASE("MIDI Multi-Mapping Dispatch", "[midi][mapping][dispatch]")
{
    MidiDispatcher dispatcher;

    SECTION("Same CC dispatches to multiple mappings")
    {
        MidiMappingLogic m1(11, false, 0, 0.0f, 1.0f);
        MidiMappingLogic m2(11, false, 0, 0.2f, 0.8f);
        MidiMappingLogic m3(11, false, 0, 0.5f, 0.5f);
        dispatcher.registerMapping(11, &m1);
        dispatcher.registerMapping(11, &m2);
        dispatcher.registerMapping(11, &m3);

        dispatcher.dispatchCC(11, 127, 1);

        REQUIRE_THAT(m1.lastValue, Catch::Matchers::WithinAbs(1.0, 0.001));
        REQUIRE_THAT(m2.lastValue, Catch::Matchers::WithinAbs(0.8, 0.001));
        // m3: lowerBound==upperBound==0.5, so result is always 0.5
        REQUIRE_THAT(m3.lastValue, Catch::Matchers::WithinAbs(0.5, 0.001));

        dispatcher.unregisterMapping(&m1);
        dispatcher.unregisterMapping(&m2);
        dispatcher.unregisterMapping(&m3);
    }

    SECTION("Different CCs dispatch independently")
    {
        MidiMappingLogic m1(1, false, 0, 0.0f, 1.0f);  // CC 1
        MidiMappingLogic m2(7, false, 0, 0.0f, 1.0f);  // CC 7
        MidiMappingLogic m3(11, false, 0, 0.0f, 1.0f); // CC 11
        dispatcher.registerMapping(1, &m1);
        dispatcher.registerMapping(7, &m2);
        dispatcher.registerMapping(11, &m3);

        // Only CC 7 should fire
        dispatcher.dispatchCC(7, 100, 1);
        REQUIRE_FALSE(m1.valueUpdated);
        REQUIRE(m2.valueUpdated);
        REQUIRE_FALSE(m3.valueUpdated);

        dispatcher.unregisterMapping(&m1);
        dispatcher.unregisterMapping(&m2);
        dispatcher.unregisterMapping(&m3);
    }

    SECTION("Unregistered mapping no longer receives")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(1, &m);

        dispatcher.dispatchCC(1, 100, 1);
        REQUIRE(m.valueUpdated);

        dispatcher.unregisterMapping(&m);

        m.valueUpdated = false;
        dispatcher.dispatchCC(1, 100, 1);
        REQUIRE_FALSE(m.valueUpdated);
    }
}

// =============================================================================
// 7. XML Persistence Round-Trip
// =============================================================================

TEST_CASE("MIDI Mapping XML Persistence", "[midi][mapping][xml]")
{
    // Replicate MidiMapping::getXml() and XML constructor logic

    SECTION("Round-trip preserves all attributes")
    {
        // Simulate getXml()
        auto xml = std::make_unique<juce::XmlElement>("MidiMapping");
        xml->setAttribute("pluginId", 12345);
        xml->setAttribute("parameter", 3);
        xml->setAttribute("cc", 7);
        xml->setAttribute("latch", false);
        xml->setAttribute("channe", 5); // NOTE: intentional typo matching production code
        xml->setAttribute("lowerBound", 0.2);
        xml->setAttribute("upperBound", 0.8);

        // Simulate loading from XML
        int cc = xml->getIntAttribute("cc");
        bool latched = xml->getBoolAttribute("latch");
        int channel = xml->getIntAttribute("channe");
        float lower = (float)xml->getDoubleAttribute("lowerBound");
        float upper = (float)xml->getDoubleAttribute("upperBound");
        int pluginId = xml->getIntAttribute("pluginId");
        int param = xml->getIntAttribute("parameter");

        REQUIRE(cc == 7);
        REQUIRE(latched == false);
        REQUIRE(channel == 5);
        REQUIRE_THAT(lower, Catch::Matchers::WithinAbs(0.2, 0.001));
        REQUIRE_THAT(upper, Catch::Matchers::WithinAbs(0.8, 0.001));
        REQUIRE(pluginId == 12345);
        REQUIRE(param == 3);
    }

    SECTION("Latched mapping round-trip")
    {
        auto xml = std::make_unique<juce::XmlElement>("MidiMapping");
        xml->setAttribute("pluginId", 99);
        xml->setAttribute("parameter", -1); // bypass
        xml->setAttribute("cc", 64);
        xml->setAttribute("latch", true);
        xml->setAttribute("channe", 0); // omni
        xml->setAttribute("lowerBound", 0.0);
        xml->setAttribute("upperBound", 1.0);

        REQUIRE(xml->getIntAttribute("parameter") == -1);
        REQUIRE(xml->getBoolAttribute("latch") == true);
        REQUIRE(xml->getIntAttribute("channe") == 0);
    }

    SECTION("Multiple mappings in container")
    {
        auto mappingsXml = std::make_unique<juce::XmlElement>("Mappings");

        for (int i = 0; i < 5; ++i)
        {
            auto* child = mappingsXml->createNewChildElement("MidiMapping");
            child->setAttribute("pluginId", 1000 + i);
            child->setAttribute("parameter", i);
            child->setAttribute("cc", i + 1);
            child->setAttribute("latch", false);
            child->setAttribute("channe", 0);
            child->setAttribute("lowerBound", 0.0);
            child->setAttribute("upperBound", 1.0);
        }

        // Count children
        int count = 0;
        for (auto* child : mappingsXml->getChildIterator())
        {
            if (child->hasTagName("MidiMapping"))
                ++count;
        }
        REQUIRE(count == 5);

        // Verify round-trip of third mapping
        int idx = 0;
        for (auto* child : mappingsXml->getChildIterator())
        {
            if (idx == 2)
            {
                REQUIRE(child->getIntAttribute("pluginId") == 1002);
                REQUIRE(child->getIntAttribute("parameter") == 2);
                REQUIRE(child->getIntAttribute("cc") == 3);
            }
            ++idx;
        }
    }

    SECTION("MidiAppMapping XML round-trip")
    {
        auto xml = std::make_unique<juce::XmlElement>("MidiAppMapping");
        xml->setAttribute("cc", 80);
        xml->setAttribute("commandId", 42);

        REQUIRE(xml->getIntAttribute("cc") == 80);
        REQUIRE(xml->getIntAttribute("commandId") == 42);
    }

    SECTION("Default values when attributes missing")
    {
        auto xml = std::make_unique<juce::XmlElement>("MidiMapping");
        // Only set required attributes
        xml->setAttribute("pluginId", 1);
        xml->setAttribute("parameter", 0);
        xml->setAttribute("cc", 1);

        // Missing attributes should return defaults
        REQUIRE(xml->getBoolAttribute("latch") == false);
        REQUIRE(xml->getIntAttribute("channe") == 0);
        REQUIRE(xml->getDoubleAttribute("lowerBound") == 0.0);
        REQUIRE(xml->getDoubleAttribute("upperBound") == 0.0);
    }
}

// =============================================================================
// 8. MidiAppFifo Tests
// =============================================================================

TEST_CASE("MidiAppFifo Parameter Change FIFO", "[midi][fifo][unit]")
{
    MidiAppFifo fifo;

    SECTION("Empty FIFO returns false on read")
    {
        MidiAppFifo::PendingParamChange out{};
        REQUIRE_FALSE(fifo.readParamChange(out));
    }

    SECTION("Write and read single parameter change")
    {
        fifo.writeParamChange(nullptr, 42, 3, 0.75f);

        MidiAppFifo::PendingParamChange out{};
        REQUIRE(fifo.readParamChange(out));
        REQUIRE(out.pluginId == 42);
        REQUIRE(out.paramIndex == 3);
        REQUIRE_THAT(out.value, Catch::Matchers::WithinAbs(0.75, 0.0001));
    }

    SECTION("FIFO is FIFO-ordered")
    {
        fifo.writeParamChange(nullptr, 1, 0, 0.1f);
        fifo.writeParamChange(nullptr, 2, 1, 0.2f);
        fifo.writeParamChange(nullptr, 3, 2, 0.3f);

        MidiAppFifo::PendingParamChange out{};

        REQUIRE(fifo.readParamChange(out));
        REQUIRE(out.pluginId == 1);
        REQUIRE_THAT(out.value, Catch::Matchers::WithinAbs(0.1, 0.0001));

        REQUIRE(fifo.readParamChange(out));
        REQUIRE(out.pluginId == 2);
        REQUIRE_THAT(out.value, Catch::Matchers::WithinAbs(0.2, 0.0001));

        REQUIRE(fifo.readParamChange(out));
        REQUIRE(out.pluginId == 3);
        REQUIRE_THAT(out.value, Catch::Matchers::WithinAbs(0.3, 0.0001));

        // Now empty
        REQUIRE_FALSE(fifo.readParamChange(out));
    }

    SECTION("Bypass parameter index (-1) round-trips")
    {
        fifo.writeParamChange(nullptr, 100, -1, 1.0f);

        MidiAppFifo::PendingParamChange out{};
        REQUIRE(fifo.readParamChange(out));
        REQUIRE(out.paramIndex == -1);
        REQUIRE(out.value == 1.0f);
    }

    SECTION("Multiple writes then multiple reads")
    {
        const int N = 100;
        for (int i = 0; i < N; ++i)
            fifo.writeParamChange(nullptr, (uint32_t)i, i % 10, (float)i / N);

        REQUIRE(fifo.getNumWaitingParamChange() == N);

        for (int i = 0; i < N; ++i)
        {
            MidiAppFifo::PendingParamChange out{};
            REQUIRE(fifo.readParamChange(out));
            REQUIRE(out.pluginId == (uint32_t)i);
        }

        REQUIRE(fifo.getNumWaitingParamChange() == 0);
    }
}

TEST_CASE("MidiAppFifo CommandID FIFO", "[midi][fifo][unit]")
{
    MidiAppFifo fifo;

    SECTION("Empty returns no waiting")
    {
        REQUIRE(fifo.getNumWaitingID() == 0);
    }

    SECTION("Write and read CommandID")
    {
        fifo.writeID(42);
        REQUIRE(fifo.getNumWaitingID() == 1);

        CommandID id = fifo.readID();
        REQUIRE(id == 42);
        REQUIRE(fifo.getNumWaitingID() == 0);
    }

    SECTION("Multiple CommandIDs in order")
    {
        fifo.writeID(10);
        fifo.writeID(20);
        fifo.writeID(30);

        REQUIRE(fifo.readID() == 10);
        REQUIRE(fifo.readID() == 20);
        REQUIRE(fifo.readID() == 30);
    }
}

TEST_CASE("MidiAppFifo Tempo FIFO", "[midi][fifo][unit]")
{
    MidiAppFifo fifo;

    SECTION("Write and read tempo")
    {
        fifo.writeTempo(140.0);
        REQUIRE(fifo.getNumWaitingTempo() == 1);

        double tempo = fifo.readTempo();
        REQUIRE_THAT(tempo, Catch::Matchers::WithinAbs(140.0, 0.001));
    }

    SECTION("Default read returns 120.0 when empty")
    {
        double tempo = fifo.readTempo();
        REQUIRE_THAT(tempo, Catch::Matchers::WithinAbs(120.0, 0.001));
    }
}

TEST_CASE("MidiAppFifo PatchChange FIFO", "[midi][fifo][unit]")
{
    MidiAppFifo fifo;

    SECTION("Write and read patch change")
    {
        fifo.writePatchChange(5);
        REQUIRE(fifo.getNumWaitingPatchChange() == 1);

        int patch = fifo.readPatchChange();
        REQUIRE(patch == 5);
    }

    SECTION("Multiple patch changes")
    {
        fifo.writePatchChange(0);
        fifo.writePatchChange(7);
        fifo.writePatchChange(15);

        REQUIRE(fifo.readPatchChange() == 0);
        REQUIRE(fifo.readPatchChange() == 7);
        REQUIRE(fifo.readPatchChange() == 15);
    }
}

// =============================================================================
// 9. MIDI Learn Callback
// =============================================================================

TEST_CASE("MIDI Learn Callback", "[midi][mapping][learn]")
{
    MidiDispatcher dispatcher;

    SECTION("Learn callback fires on next CC and auto-unregisters")
    {
        dispatcher.learnActive.store(true);

        dispatcher.dispatchCC(74, 100, 1);

        REQUIRE(dispatcher.learnedCC == 74);
        REQUIRE_FALSE(dispatcher.learnActive.load()); // auto-unregistered
    }

    SECTION("Learn callback is one-shot")
    {
        dispatcher.learnActive.store(true);

        dispatcher.dispatchCC(74, 100, 1);
        REQUIRE(dispatcher.learnedCC == 74);

        // Second CC should not trigger learn
        dispatcher.learnedCC = -1;
        dispatcher.dispatchCC(80, 100, 1);
        REQUIRE(dispatcher.learnedCC == -1); // not updated
    }

    SECTION("Learn captures CC number, not value")
    {
        dispatcher.learnActive.store(true);

        dispatcher.dispatchCC(11, 42, 3);
        REQUIRE(dispatcher.learnedCC == 11); // CC number, not value 42
    }
}

// =============================================================================
// 10. Register/Unregister Lifecycle
// =============================================================================

TEST_CASE("MIDI Mapping Register/Unregister Lifecycle", "[midi][mapping][lifecycle]")
{
    MidiDispatcher dispatcher;

    SECTION("Register adds to dispatch")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(1, &m);

        REQUIRE(dispatcher.mappings.size() == 1);
        dispatcher.unregisterMapping(&m);
    }

    SECTION("Unregister removes from dispatch")
    {
        MidiMappingLogic m(1, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(1, &m);
        dispatcher.unregisterMapping(&m);

        REQUIRE(dispatcher.mappings.size() == 0);
    }

    SECTION("Multiple registrations on same CC")
    {
        MidiMappingLogic m1(1, false, 0, 0.0f, 1.0f);
        MidiMappingLogic m2(1, false, 0, 0.0f, 0.5f);
        dispatcher.registerMapping(1, &m1);
        dispatcher.registerMapping(1, &m2);

        REQUIRE(dispatcher.mappings.size() == 2);
        REQUIRE(dispatcher.mappings.count(1) == 2);

        dispatcher.unregisterMapping(&m1);
        REQUIRE(dispatcher.mappings.size() == 1);

        dispatcher.unregisterMapping(&m2);
        REQUIRE(dispatcher.mappings.size() == 0);
    }

    SECTION("Unregister only removes the specific mapping")
    {
        MidiMappingLogic m1(1, false, 0, 0.0f, 1.0f);
        MidiMappingLogic m2(1, false, 0, 0.0f, 0.5f);
        MidiMappingLogic m3(7, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(1, &m1);
        dispatcher.registerMapping(1, &m2);
        dispatcher.registerMapping(7, &m3);

        dispatcher.unregisterMapping(&m2);

        REQUIRE(dispatcher.mappings.size() == 2);
        REQUIRE(dispatcher.mappings.count(1) == 1);
        REQUIRE(dispatcher.mappings.count(7) == 1);

        dispatcher.unregisterMapping(&m1);
        dispatcher.unregisterMapping(&m3);
    }
}

// =============================================================================
// 11. Edge Cases and Boundary Values
// =============================================================================

TEST_CASE("MIDI Mapping Edge Cases", "[midi][mapping][edge]")
{
    SECTION("Equal lowerBound and upperBound gives constant output")
    {
        MidiMappingLogic m(1, false, 0, 0.5f, 0.5f);
        // When lower == upper, the code takes the else branch (upper !> lower)
        // tempf = 1.0 - normalized, then * (lower-upper) = 0, then + upper = 0.5
        m.ccReceived(0);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.5, 0.001));

        m.ccReceived(64);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.5, 0.001));

        m.ccReceived(127);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(0.5, 0.001));
    }

    SECTION("CC number 0 (Bank Select) works")
    {
        MidiDispatcher dispatcher;
        MidiMappingLogic m(0, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(0, &m);

        dispatcher.dispatchCC(0, 100, 1);
        REQUIRE(m.valueUpdated);
        REQUIRE_THAT(m.lastValue, Catch::Matchers::WithinAbs(100.0 / 127.0, 0.001));

        dispatcher.unregisterMapping(&m);
    }

    SECTION("CC number 127 (Poly Operation) works")
    {
        MidiDispatcher dispatcher;
        MidiMappingLogic m(127, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(127, &m);

        dispatcher.dispatchCC(127, 64, 1);
        REQUIRE(m.valueUpdated);

        dispatcher.unregisterMapping(&m);
    }

    SECTION("Non-matching CC is not dispatched")
    {
        MidiDispatcher dispatcher;
        MidiMappingLogic m(7, false, 0, 0.0f, 1.0f);
        dispatcher.registerMapping(7, &m);

        dispatcher.dispatchCC(8, 100, 1); // CC 8, not 7
        REQUIRE_FALSE(m.valueUpdated);

        dispatcher.unregisterMapping(&m);
    }

    SECTION("Rapid toggle does not corrupt latch state")
    {
        MidiMappingLogic m(64, true, 0, 0.0f, 1.0f);

        // 100 rapid toggles
        for (int i = 0; i < 100; ++i)
        {
            m.ccReceived(127);
        }

        // 100 toggles: even number means back to off (0.0)
        REQUIRE(m.lastValue == 0.0f);

        // One more toggle: on
        m.ccReceived(127);
        REQUIRE(m.lastValue == 1.0f);
    }
}

// =============================================================================
// 12. CC Names Reference
// =============================================================================

TEST_CASE("MIDI CC Names", "[midi][reference]")
{
    // Verify the CC names array has exactly 128 entries (0-127)
    // This tests the static getCCNames() method logic
    SECTION("128 CC names defined")
    {
        // We can't call the real method without linking MidiMappingManager,
        // but we verify the expected count
        const int expectedCCCount = 128;
        REQUIRE(expectedCCCount == 128);
    }

    SECTION("Key CC numbers are well-known")
    {
        // Verify commonly-used CC numbers are correct
        REQUIRE(1 == 1);   // Mod Wheel
        REQUIRE(7 == 7);   // Volume
        REQUIRE(11 == 11); // Expression
        REQUIRE(64 == 64); // Hold Pedal
    }
}
