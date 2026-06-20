#include "../src/NAMCore.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using Catch::Matchers::WithinAbs;

namespace
{
class TempNamFile
{
public:
    explicit TempNamFile(const std::string& json)
    {
        const auto uniqueId = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() / ("pedalboard3_nam_runtime_" + std::to_string(uniqueId) + ".nam");

        std::ofstream file(path, std::ios::binary);
        REQUIRE(file.is_open());
        file << json;
    }

    ~TempNamFile()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    std::string string() const
    {
        return path.string();
    }

private:
    std::filesystem::path path;
};

std::string makeLinearNamJson(const std::string& version,
                              bool architectureVersion2,
                              double sampleRate,
                              int inChannels = 1,
                              int outChannels = 1)
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"version\": \"" << version << "\",\n";
    json << "  \"architecture\": \"Linear\",\n";
    if (architectureVersion2)
        json << "  \"architecture_version\": 2,\n";
    json << "  \"sample_rate\": " << sampleRate << ",\n";
    json << "  \"config\": {\n";
    json << "    \"receptive_field\": 1,\n";
    json << "    \"bias\": false,\n";
    json << "    \"in_channels\": " << inChannels << ",\n";
    json << "    \"out_channels\": " << outChannels << "\n";
    json << "  },\n";
    json << "  \"metadata\": {},\n";
    json << "  \"weights\": [1.0]\n";
    json << "}\n";
    return json.str();
}

void requireIdentityProcessing(NAMCore& core)
{
    std::vector<float> input{0.25f, -0.5f, 0.75f, 0.0f, 0.125f, -0.25f};
    std::vector<float> output(input.size(), 0.0f);

    core.process(input.data(), output.data(), static_cast<int>(input.size()));
    core.finalize(static_cast<int>(input.size()));

    for (size_t i = 0; i < input.size(); ++i)
    {
        REQUIRE(std::isfinite(output[i]));
        REQUIRE_THAT(output[i], WithinAbs(input[i], 0.0001f));
    }
}
} // namespace

TEST_CASE("NAMCore runtime loads a generated legacy Linear NAM model", "[nam][runtime][legacy]")
{
    TempNamFile model(makeLinearNamJson("0.5.0", false, 48000.0));

    NAMCore core;
    core.prepare(48000.0, 16);

    REQUIRE(core.loadModel(model.string()));
    REQUIRE(core.isModelLoaded());
    REQUIRE_FALSE(core.isSlimmableModel());

    requireIdentityProcessing(core);
}

TEST_CASE("NAMCore runtime loads a generated explicit A2 Linear NAM model", "[nam][runtime][a2]")
{
    TempNamFile model(makeLinearNamJson("0.5.0", true, 48000.0));

    NAMCore core;
    core.prepare(48000.0, 16);

    REQUIRE(core.loadModel(model.string()));
    REQUIRE(core.isModelLoaded());
    REQUIRE_FALSE(core.isSlimmableModel());

    requireIdentityProcessing(core);
}

TEST_CASE("NAMCore runtime clears an A2 model when prepare sees a sample-rate mismatch", "[nam][runtime][a2]")
{
    TempNamFile model(makeLinearNamJson("0.5.0", true, 48000.0));

    NAMCore core;
    REQUIRE(core.loadModel(model.string()));
    REQUIRE(core.isModelLoaded());

    core.prepare(44100.0, 16);

    REQUIRE_FALSE(core.isModelLoaded());
}

TEST_CASE("NAMCore runtime keeps an existing model when unsupported A2 load fails", "[nam][runtime][a2][compat]")
{
    TempNamFile legacyModel(makeLinearNamJson("0.5.0", false, 48000.0));
    TempNamFile unsupportedA2Model(makeLinearNamJson("0.5.0", true, 48000.0, 1, 2));

    NAMCore core;
    core.prepare(48000.0, 16);

    REQUIRE(core.loadModel(legacyModel.string()));
    REQUIRE(core.isModelLoaded());

    REQUIRE_FALSE(core.loadModel(unsupportedA2Model.string()));
    REQUIRE(core.isModelLoaded());

    requireIdentityProcessing(core);
}
