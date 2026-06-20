#include "../src/dsp/ReverbSC.h"
#include "../src/BypassableInstance.h"
#include "../src/ReverbSCProcessor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{
bool isFiniteBuffer(const std::vector<float>& buffer)
{
    return std::all_of(buffer.begin(), buffer.end(), [](const float sample) { return std::isfinite(sample); });
}

float maxAbs(const std::vector<float>& buffer)
{
    float maximum = 0.0f;
    for (const auto sample : buffer)
        maximum = std::max(maximum, std::abs(sample));
    return maximum;
}

std::string readTextFileForReverbScTest(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.is_open());

    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("ReverbSC core keeps silence silent", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 64);

    std::vector<float> inL(64, 0.0f);
    std::vector<float> inR(64, 0.0f);
    std::vector<float> outL(64, 1.0f);
    std::vector<float> outR(64, 1.0f);

    reverb.process(inL.data(), inR.data(), outL.data(), outR.data(), 64);

    for (int i = 0; i < 64; ++i)
    {
        REQUIRE(outL[static_cast<size_t>(i)] == 0.0f);
        REQUIRE(outR[static_cast<size_t>(i)] == 0.0f);
    }
}

TEST_CASE("ReverbSC core produces a finite stereo impulse tail", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 512);
    reverb.setFeedback(0.97f);
    reverb.setDampingHz(10000.0f);

    constexpr int totalSamples = 48000;
    std::vector<float> inL(totalSamples, 0.0f);
    std::vector<float> inR(totalSamples, 0.0f);
    std::vector<float> outL(totalSamples, 0.0f);
    std::vector<float> outR(totalSamples, 0.0f);

    inL[0] = 1.0f;
    inR[0] = 1.0f;

    for (int offset = 0; offset < totalSamples; offset += 512)
    {
        const int blockSize = std::min(512, totalSamples - offset);
        reverb.process(inL.data() + offset, inR.data() + offset, outL.data() + offset, outR.data() + offset, blockSize);
    }

    REQUIRE(isFiniteBuffer(outL));
    REQUIRE(isFiniteBuffer(outR));
    REQUIRE(maxAbs(outL) > 0.001f);
    REQUIRE(maxAbs(outR) > 0.001f);
    REQUIRE(maxAbs(outL) <= 2.0f);
    REQUIRE(maxAbs(outR) <= 2.0f);
}

TEST_CASE("ReverbSC core remains finite across sample rates and block sizes", "[reverbsc][dsp]")
{
    const double sampleRates[] = {44100.0, 48000.0, 96000.0, 192000.0};
    const int blockSizes[] = {1, 17, 256};

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            pedalboard3::dsp::ReverbSC reverb;
            reverb.prepare(sampleRate, blockSize);
            reverb.setFeedback(0.99f);
            reverb.setDampingHz(20000.0f);

            std::vector<float> inL(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> inR(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> outL(static_cast<size_t>(blockSize), 0.0f);
            std::vector<float> outR(static_cast<size_t>(blockSize), 0.0f);

            inL[0] = 0.5f;
            inR[0] = -0.5f;

            for (int i = 0; i < 1000; ++i)
                reverb.process(inL.data(), inR.data(), outL.data(), outR.data(), blockSize);

            REQUIRE(isFiniteBuffer(outL));
            REQUIRE(isFiniteBuffer(outR));
            REQUIRE(maxAbs(outL) <= 2.0f);
            REQUIRE(maxAbs(outR) <= 2.0f);
        }
    }
}

TEST_CASE("ReverbSC core clamps feedback and damping parameters", "[reverbsc][dsp]")
{
    pedalboard3::dsp::ReverbSC reverb;
    reverb.prepare(48000.0, 64);

    reverb.setFeedback(-1.0f);
    REQUIRE(reverb.getFeedback() == 0.0f);
    reverb.setFeedback(2.0f);
    REQUIRE(reverb.getFeedback() == 0.99f);

    reverb.setDampingHz(-10.0f);
    REQUIRE(reverb.getDampingHz() == 20.0f);
    reverb.setDampingHz(40000.0f);
    REQUIRE(reverb.getDampingHz() == 20000.0f);
}

TEST_CASE("ReverbSCProcessor exposes stable host metadata", "[reverbsc][processor]")
{
    ReverbSCProcessor processor;

    REQUIRE(processor.getName() == "ReverbSC");
    REQUIRE_FALSE(processor.acceptsMidi());
    REQUIRE_FALSE(processor.producesMidi());
    REQUIRE(processor.getNumParameters() == ReverbSCProcessor::NumParameters);
    REQUIRE(processor.getParameterName(ReverbSCProcessor::MixParam) == "Mix");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::FeedbackParam) == "Feedback");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::DampingParam) == "Damping");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::WidthParam) == "Width");
    REQUIRE(processor.getParameterName(ReverbSCProcessor::OutputParam) == "Output");
}

TEST_CASE("ReverbSCProcessor exposes compact stereo graph pins", "[reverbsc][ui]")
{
    ReverbSCProcessor processor;

    REQUIRE(processor.getInputChannelName(0) == "L");
    REQUIRE(processor.getInputChannelName(1) == "R");
    REQUIRE(processor.getInputChannelName(2).isEmpty());
    REQUIRE(processor.getOutputChannelName(0) == "L");
    REQUIRE(processor.getOutputChannelName(1) == "R");
    REQUIRE(processor.getOutputChannelName(2).isEmpty());

    const auto inputLayout = processor.getInputPinLayout();
    const auto outputLayout = processor.getOutputPinLayout();
    REQUIRE(inputLayout.pinY.size() == 2);
    REQUIRE(outputLayout.pinY.size() == 2);
    REQUIRE(inputLayout.pinY[0] < inputLayout.pinY[1]);
    REQUIRE(outputLayout.pinY[0] < outputLayout.pinY[1]);
}

TEST_CASE("ReverbSC wrapper preserves compact stereo graph pin labels", "[reverbsc][ui]")
{
    std::unique_ptr<BypassableInstance> wrapped(new BypassableInstance(new ReverbSCProcessor()));

    REQUIRE(wrapped->getCachedInputChannelName(0) == "L");
    REQUIRE(wrapped->getCachedInputChannelName(1) == "R");
    REQUIRE(wrapped->getCachedOutputChannelName(0) == "L");
    REQUIRE(wrapped->getCachedOutputChannelName(1) == "R");
}

TEST_CASE("ReverbSCProcessor provides embedded node controls for every parameter", "[reverbsc][ui]")
{
    ReverbSCProcessor processor;
    std::unique_ptr<Component> controls(processor.getControls());

    REQUIRE(controls != nullptr);
    REQUIRE(processor.getSize().getX() == 308);
    REQUIRE(processor.getSize().getY() == 154);
    REQUIRE(controls->getNumChildComponents() >= ReverbSCProcessor::NumParameters);

    for (int parameter = 0; parameter < ReverbSCProcessor::NumParameters; ++parameter)
    {
        const auto parameterName = processor.getParameterName(parameter);
        bool foundSlider = false;
        for (int childIndex = 0; childIndex < controls->getNumChildComponents(); ++childIndex)
        {
            auto* child = controls->getChildComponent(childIndex);
            if (child != nullptr && child->getName() == parameterName)
                foundSlider = dynamic_cast<Slider*>(child) != nullptr;
        }

        REQUIRE(foundSlider);
    }

    const auto source = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/ReverbSCProcessor.cpp");
    const auto pluginComponentSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginComponent.cpp");
    REQUIRE(source.find("class ReverbSCControl") != std::string::npos);
    REQUIRE(source.find("paintParameterLane") != std::string::npos);
    REQUIRE(source.find("paintParameterTile") != std::string::npos);
    REQUIRE(pluginComponentSource.find("usesEmbeddedParameterSurface") != std::string::npos);
    REQUIRE(pluginComponentSource.find("pluginName == \"ReverbSC\"") != std::string::npos);
}

TEST_CASE("ReverbSC embedded controls suppress redundant editor and param pin affordances", "[reverbsc][ui]")
{
    const auto pluginComponentSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginComponent.cpp");

    const auto hostPinStart = pluginComponentSource.find("bool shouldCreateHostMidiOrParamPin");
    REQUIRE(hostPinStart != std::string::npos);
    const auto hostPinEnd = pluginComponentSource.find("int getEmbeddedNodeControlTopOffset", hostPinStart);
    REQUIRE(hostPinEnd != std::string::npos);
    const auto hostPinBody = pluginComponentSource.substr(hostPinStart, hostPinEnd - hostPinStart);
    REQUIRE(hostPinBody.find("usesEmbeddedParameterSurface(pluginName)") != std::string::npos);

    const auto editorStart = pluginComponentSource.find("const bool suppressHostEditorButton");
    REQUIRE(editorStart != std::string::npos);
    const auto editorEnd = pluginComponentSource.find(";", editorStart);
    REQUIRE(editorEnd != std::string::npos);
    const auto editorExpression = pluginComponentSource.substr(editorStart, editorEnd - editorStart);
    REQUIRE(editorExpression.find("usesEmbeddedParameterSurface(pluginName)") != std::string::npos);
}

TEST_CASE("ReverbSC embedded controls do not paint a nested node shell", "[reverbsc][ui]")
{
    const auto source = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/ReverbSCProcessor.cpp");
    const auto pluginComponentSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginComponent.cpp");

    const auto directPaintStart = pluginComponentSource.find("bool isDirectPaintedEmbeddedNodeName");
    REQUIRE(directPaintStart != std::string::npos);
    const auto directPaintEnd = pluginComponentSource.find("bool usesEmbeddedParameterSurface", directPaintStart);
    REQUIRE(directPaintEnd != std::string::npos);
    const auto directPaintBody = pluginComponentSource.substr(directPaintStart, directPaintEnd - directPaintStart);
    REQUIRE(directPaintBody.find("\"ReverbSC\"") == std::string::npos);

    const auto controlStart = source.find("class ReverbSCControl");
    REQUIRE(controlStart != std::string::npos);
    const auto controlEnd = source.find("} // namespace", controlStart);
    REQUIRE(controlEnd != std::string::npos);
    const auto controlBody = source.substr(controlStart, controlEnd - controlStart);

    REQUIRE(controlBody.find("fillRoundedRectangle(bounds") == std::string::npos);
    REQUIRE(controlBody.find("drawRoundedRectangle(bounds") == std::string::npos);
}

TEST_CASE("ReverbSC embedded controls do not duplicate host title chrome", "[reverbsc][ui]")
{
    const auto source = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/ReverbSCProcessor.cpp");

    const auto controlStart = source.find("class ReverbSCControl");
    REQUIRE(controlStart != std::string::npos);
    const auto controlEnd = source.find("} // namespace", controlStart);
    REQUIRE(controlEnd != std::string::npos);
    const auto controlBody = source.substr(controlStart, controlEnd - controlStart);

    REQUIRE(controlBody.find("\"SC REVERB\"") == std::string::npos);
    REQUIRE(controlBody.find("\"STEREO\"") == std::string::npos);
    REQUIRE(controlBody.find("paintHeader") == std::string::npos);
}

TEST_CASE("ReverbSC embedded controls use polished direct-surface primitives", "[reverbsc][ui]")
{
    const auto source = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/ReverbSCProcessor.cpp");
    const auto header = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/ReverbSCProcessor.h");

    const auto controlStart = source.find("class ReverbSCControl");
    REQUIRE(controlStart != std::string::npos);
    const auto controlEnd = source.find("} // namespace", controlStart);
    REQUIRE(controlEnd != std::string::npos);
    const auto controlBody = source.substr(controlStart, controlEnd - controlStart);

    REQUIRE(controlBody.find("paintDiffusionTexture") != std::string::npos);
    REQUIRE(controlBody.find("paintValueChip") != std::string::npos);
    REQUIRE(controlBody.find("paintPanelLighting") != std::string::npos);
    REQUIRE(controlBody.find("paintReverbGlyph") != std::string::npos);
    REQUIRE(controlBody.find("glyphArea") != std::string::npos);
    REQUIRE(source.find("setSize(308, 154);") != std::string::npos);
    REQUIRE(header.find("Point<int>(308, 154)") != std::string::npos);
    REQUIRE(controlBody.find("jmin(10.8f") != std::string::npos);
}

TEST_CASE("ReverbSC embedded controls use compact host footer spacing", "[reverbsc][ui]")
{
    const auto pluginComponentSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginComponent.cpp");

    const auto paddingStart = pluginComponentSource.find("int getEmbeddedNodeControlHeightPadding");
    REQUIRE(paddingStart != std::string::npos);
    const auto paddingEnd = pluginComponentSource.find("Point<int> getDefaultRackNodeSize", paddingStart);
    REQUIRE(paddingEnd != std::string::npos);
    const auto paddingBody = pluginComponentSource.substr(paddingStart, paddingEnd - paddingStart);

    REQUIRE(paddingBody.find("if (pluginName == \"ReverbSC\")") != std::string::npos);
    REQUIRE(paddingBody.find("return 60;") != std::string::npos);
}

TEST_CASE("ReverbSCProcessor state round-trips parameters", "[reverbsc][processor]")
{
    ReverbSCProcessor source;
    source.setParameter(ReverbSCProcessor::MixParam, 0.25f);
    source.setParameter(ReverbSCProcessor::FeedbackParam, 0.75f);
    source.setParameter(ReverbSCProcessor::DampingParam, 0.5f);
    source.setParameter(ReverbSCProcessor::WidthParam, 0.4f);
    source.setParameter(ReverbSCProcessor::OutputParam, 0.8f);

    juce::MemoryBlock state;
    source.getStateInformation(state);

    ReverbSCProcessor restored;
    restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

    REQUIRE(restored.getParameter(ReverbSCProcessor::MixParam) == Catch::Approx(0.25f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::FeedbackParam) == Catch::Approx(0.75f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::DampingParam) == Catch::Approx(0.5f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::WidthParam) == Catch::Approx(0.4f));
    REQUIRE(restored.getParameter(ReverbSCProcessor::OutputParam) == Catch::Approx(0.8f));
}

TEST_CASE("ReverbSCProcessor processes a finite stereo impulse", "[reverbsc][processor]")
{
    ReverbSCProcessor processor;
    processor.prepareToPlay(48000.0, 4096);
    processor.setParameter(ReverbSCProcessor::MixParam, 1.0f);
    processor.setParameter(ReverbSCProcessor::FeedbackParam, 0.9f);
    processor.setParameter(ReverbSCProcessor::DampingParam, 0.7f);
    processor.setParameter(ReverbSCProcessor::WidthParam, 1.0f);
    processor.setParameter(ReverbSCProcessor::OutputParam, 0.5f);

    juce::AudioBuffer<float> buffer(2, 4096);
    juce::MidiBuffer midi;
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    processor.processBlock(buffer, midi);

    std::vector<float> left(static_cast<size_t>(buffer.getNumSamples()));
    std::vector<float> right(static_cast<size_t>(buffer.getNumSamples()));
    std::copy(buffer.getReadPointer(0), buffer.getReadPointer(0) + buffer.getNumSamples(), left.begin());
    std::copy(buffer.getReadPointer(1), buffer.getReadPointer(1) + buffer.getNumSamples(), right.begin());

    REQUIRE(isFiniteBuffer(left));
    REQUIRE(isFiniteBuffer(right));
    REQUIRE(maxAbs(left) > 0.0001f);
    REQUIRE(maxAbs(right) > 0.0001f);
    REQUIRE(maxAbs(left) <= 2.0f);
    REQUIRE(maxAbs(right) <= 2.0f);
}

TEST_CASE("InternalPluginFormat source registers ReverbSC", "[reverbsc][internal-format]")
{
    const auto header = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/InternalFilters.h");
    const auto source = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/InternalFilters.cpp");
    const auto mainPanelSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/MainPanel.cpp");
    const auto pluginFieldSource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginField.cpp");
    const auto searchOverlaySource = readTextFileForReverbScTest(PEDALBOARD3_SOURCE_DIR "/src/PluginSearchOverlay.cpp");

    REQUIRE(header.find("reverbScProcFilter") != std::string::npos);
    REQUIRE(header.find("PluginDescription reverbScProcDesc") != std::string::npos);
    REQUIRE(source.find("#include \"ReverbSCProcessor.h\"") != std::string::npos);
    REQUIRE(source.find("ReverbSCProcessor p;") != std::string::npos);
    REQUIRE(source.find("reverbScProcDesc.category = \"Effects\"") != std::string::npos);
    REQUIRE(source.find("return new ReverbSCProcessor();") != std::string::npos);
    REQUIRE(source.find("case reverbScProcFilter:") != std::string::npos);

    const auto userFacingStart = source.find("void InternalPluginFormat::getUserFacingTypes");
    REQUIRE(userFacingStart != std::string::npos);
    const auto userFacingBody = source.substr(userFacingStart);
    REQUIRE(userFacingBody.find("reverbScProcFilter") != std::string::npos);

    REQUIRE(mainPanelSource.find("#include \"InternalFilters.h\"") != std::string::npos);
    REQUIRE(mainPanelSource.find("internalFormat.getUserFacingTypes(userFacingInternalTypes)") != std::string::npos);
    REQUIRE(mainPanelSource.find("for (auto* desc : userFacingInternalTypes)") != std::string::npos);

    REQUIRE(pluginFieldSource.find("addPluginDescriptionIfMissing(types, internalFormat.getDescriptionFor("
                                   "InternalPluginFormat::subGraphProcFilter))") != std::string::npos);
    REQUIRE(searchOverlaySource.find("addPluginDescriptionIfMissing(types, internalFormat.getDescriptionFor("
                                     "InternalPluginFormat::subGraphProcFilter))") != std::string::npos);
}
