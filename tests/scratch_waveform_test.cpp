#include "ScratchWaveformView.h"
#include "ScratchTestFiles.h"
#include <catch2/catch_approx.hpp>

namespace
{
std::shared_ptr<ScratchWaveformPair> loaded(ScratchWaveformSource& source)
{
    const auto deadline = juce::Time::getMillisecondCounter() + 5000;
    do
    {
        auto data = source.snapshot();
        if (data && data->raw.isFullyLoaded() && data->wet.isFullyLoaded()) return data;
        juce::Thread::sleep(1);
    } while (juce::Time::getMillisecondCounter() < deadline);
    return {};
}
ScratchTakeLoadResult fixture(ScratchTestFiles& files)
{
    auto take = files.makeTake();
    take.durationSamples = 48000;
    take.rawDurationSamples = take.wetDurationSamples = 48000;
    take.complete = true;
    take.captureStatus = "complete";
    ScratchTestFiles::wav(take.rawFile, 1, 48000, 48000, false);
    ScratchTestFiles::wav(take.wetFile, 2, 48000);
    REQUIRE(take.writeMetadata());
    return loadScratchTake(take.takeDirectory);
}
}

TEST_CASE("Scratch waveforms reflect decoded raw and wet samples", "[scratch][scratch-waveform]")
{
    ScratchTestFiles files;
    auto take = fixture(files);
    ScratchWaveformCache cache;
    ScratchWaveformSource source(cache);
    source.setTake(take);
    auto data = loaded(source);
    REQUIRE(data != nullptr);
    REQUIRE(data->raw.getNumChannels() == 1);
    REQUIRE(data->wet.getNumChannels() == 2);
    REQUIRE(data->wet.getTotalLength() == Catch::Approx(1.0).margin(1.0 / 48000.0));
    float minimum = 0, maximum = 0;
    data->raw.getApproximateMinMax(0, 1, 0, minimum, maximum);
    REQUIRE(minimum == 0);
    REQUIRE(maximum == 1.0f / 128.0f); // JUCE expands equal quantized bounds by one step.
    data->wet.getApproximateMinMax(0.70, 0.80, 0, minimum, maximum);
    REQUIRE(maximum == Catch::Approx(0.75f).margin(0.02f));
    data->wet.getApproximateMinMax(0, 0.5, 0, minimum, maximum);
    REQUIRE(maximum == 1.0f / 128.0f);
    data->wet.getApproximateMinMax(0, 1, 1, minimum, maximum);
    REQUIRE(maximum == 1.0f / 128.0f);
}

TEST_CASE("Scratch waveform selection and changed file identity do not reuse stale audio", "[scratch][scratch-waveform]")
{
    ScratchTestFiles files;
    auto first = fixture(files), second = fixture(files);
    ScratchWaveformCache cache;
    ScratchWaveformSource source(cache);
    source.setTake(first);
    source.setTake(second);
    auto data = loaded(source);
    REQUIRE(data != nullptr);
    REQUIRE(data->directory == second.take.takeDirectory);
    const auto hash = data->wet.getHashCode();
    source.clear();
    data.reset(); // releases the open reader before replacing on Windows
    REQUIRE(second.take.wetFile.deleteFile());
    ScratchTestFiles::wav(second.take.wetFile, 2, 96000);
    source.setTake(second);
    data = loaded(source);
    REQUIRE(data != nullptr);
    REQUIRE(data->wet.getHashCode() != hash);
    REQUIRE(data->wet.getTotalLength() == Catch::Approx(2.0));
    source.clear();
    REQUIRE(source.snapshot() == nullptr);
}
