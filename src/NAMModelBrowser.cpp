/*
  ==============================================================================

    NAMModelBrowser.cpp
    Browser window for selecting and loading NAM model files

  ==============================================================================
*/

#include "NAMModelBrowser.h"

#include "ColourScheme.h"
#include "FontManager.h"
#include "IconManager.h"
#include "NAMOnlineBrowser.h"
#include "NAMProcessor.h"

#include <melatonin_blur/melatonin_blur.h>
#include <nlohmann/json.hpp>
#include <array>
#include <set>
#include <spdlog/spdlog.h>

namespace
{
String describeBrowserCount(const File& primaryDirectory, const String& secondarySource, int totalCount,
                            int filteredCount, const String& singular, const String& plural, const String& query)
{
    String status = primaryDirectory.getFullPathName();
    if (secondarySource.isNotEmpty())
        status += " + " + secondarySource;

    const auto trimmedQuery = query.trim();
    if (totalCount <= 0)
        return status + " - No " + plural + " found";

    if (trimmedQuery.isNotEmpty())
        return status + " - " + String(filteredCount) + " of " + String(totalCount) + " " + plural + " match \"" +
               trimmedQuery + "\"";

    return status + " - " + String(totalCount) + " " + (totalCount == 1 ? singular : plural);
}

String makeEmptyStateCopy(const String& title, const String& action, const String& query)
{
    if (query.trim().isNotEmpty())
        return "No matches for \"" + query.trim() + "\"\n\nTry a broader search or clear the search field.";

    return title + "\n\n" + action;
}

String formatNAMSampleRate(double sampleRate)
{
    if (sampleRate <= 0.0)
        return "Unknown";

    if (sampleRate >= 1000.0)
        return String(sampleRate / 1000.0, sampleRate >= 10000.0 ? 1 : 2) + " kHz";

    return String(static_cast<int>(sampleRate)) + " Hz";
}

String formatNAMLoudness(const NAMModelInfo& model)
{
    return model.hasLoudness ? String(model.loudness, 1) + " dB" : "N/A";
}

String formatNAMFileSize(const NAMModelInfo& model)
{
    const auto size = File(String(model.filePath)).getSize();
    if (size <= 0)
        return "-";

    if (size >= 1024 * 1024)
        return String(size / (1024.0 * 1024.0), 2) + " MB";

    return String((size + 1023) / 1024) + " KB";
}

String formatIRDuration(double durationSeconds)
{
    if (durationSeconds <= 0.0)
        return "-";

    if (durationSeconds >= 1.0)
        return String(durationSeconds, 3) + " s";

    return String(static_cast<int>(durationSeconds * 1000.0)) + " ms";
}

String formatIRSampleRate(double sampleRate)
{
    if (sampleRate <= 0.0)
        return "-";

    if (sampleRate >= 1000.0)
        return String(sampleRate / 1000.0, sampleRate >= 10000.0 ? 1 : 2) + " kHz";

    return String(static_cast<int>(sampleRate)) + " Hz";
}

String formatIRChannels(int numChannels)
{
    if (numChannels <= 0)
        return "-";

    if (numChannels == 1)
        return "Mono";
    if (numChannels == 2)
        return "Stereo";

    return String(numChannels) + " ch";
}

String formatIRFileSize(int64_t fileSize)
{
    if (fileSize <= 0)
        return "-";

    if (fileSize >= 1024 * 1024)
        return String(fileSize / (1024.0 * 1024.0), 2) + " MB";

    return String((fileSize + 1023) / 1024) + " KB";
}

String makeIRPreviewSummary(const IRFileInfo& ir)
{
    StringArray parts;
    if (ir.durationSeconds > 0.0)
        parts.add(formatIRDuration(ir.durationSeconds));
    if (ir.sampleRate > 0.0)
        parts.add(formatIRSampleRate(ir.sampleRate));
    if (ir.numChannels > 0)
        parts.add(formatIRChannels(ir.numChannels));

    return parts.isEmpty() ? "Cabinet impulse response" : parts.joinIntoString("  |  ");
}

bool isReadableIRFile(const IRFileInfo& ir)
{
    return File(String(ir.filePath)).existsAsFile();
}

bool isReadableNAMModel(const NAMModelInfo& model)
{
    return File(String(model.filePath)).existsAsFile();
}

void drawMagnifierGlyph(Graphics& g, Rectangle<float> area, Colour colour, float thickness)
{
    const auto size = jmin(area.getWidth(), area.getHeight()) * 0.58f;
    const auto circle = Rectangle<float>(area.getCentreX() - size * 0.55f, area.getCentreY() - size * 0.6f, size, size);
    g.setColour(colour);
    g.drawEllipse(circle, thickness);
    g.drawLine(circle.getRight() - size * 0.08f, circle.getBottom() - size * 0.08f, circle.getRight() + size * 0.36f,
               circle.getBottom() + size * 0.36f, thickness);
}

void drawModelGlyph(Graphics& g, Rectangle<float> tile, Colour accent, bool active)
{
    const auto base = active ? accent.withAlpha(0.24f) : accent.withAlpha(0.13f);
    g.setColour(base);
    g.fillRoundedRectangle(tile, 9.0f);
    g.setColour(accent.withAlpha(active ? 0.7f : 0.42f));
    g.drawRoundedRectangle(tile.reduced(0.5f), 9.0f, 1.0f);

    const auto speaker = tile.reduced(tile.getWidth() * 0.26f, tile.getHeight() * 0.22f);
    g.setColour(accent.withAlpha(active ? 0.95f : 0.7f));
    g.drawEllipse(speaker, 1.4f);
    g.fillEllipse(speaker.withSizeKeepingCentre(speaker.getWidth() * 0.26f, speaker.getHeight() * 0.26f));
    g.drawLine(tile.getX() + tile.getWidth() * 0.24f, tile.getY() + tile.getHeight() * 0.25f,
               tile.getX() + tile.getWidth() * 0.42f, tile.getY() + tile.getHeight() * 0.25f, 1.2f);
}

void drawIRGlyph(Graphics& g, Rectangle<float> tile, Colour accent, bool active)
{
    g.setColour(active ? accent.withAlpha(0.22f) : accent.withAlpha(0.12f));
    g.fillRoundedRectangle(tile, 9.0f);
    g.setColour(accent.withAlpha(active ? 0.7f : 0.42f));
    g.drawRoundedRectangle(tile.reduced(0.5f), 9.0f, 1.0f);

    const auto cell = jmin(tile.getWidth(), tile.getHeight()) * 0.2f;
    for (int y = 0; y < 2; ++y)
    {
        for (int x = 0; x < 2; ++x)
        {
            auto dot = Rectangle<float>(tile.getX() + tile.getWidth() * (0.32f + x * 0.28f) - cell * 0.5f,
                                        tile.getY() + tile.getHeight() * (0.33f + y * 0.28f) - cell * 0.5f, cell,
                                        cell);
            g.setColour(accent.withAlpha(active ? 0.82f : 0.58f));
            g.drawEllipse(dot, 1.2f);
            g.setColour(accent.withAlpha(active ? 0.22f : 0.12f));
            g.fillEllipse(dot.reduced(3.0f));
        }
    }
}

void drawStatusLed(Graphics& g, Rectangle<float> area, Colour colour, bool active)
{
    const auto dot = area.withSizeKeepingCentre(8.0f, 8.0f);
    if (active)
    {
        g.setColour(colour.withAlpha(0.22f));
        g.fillEllipse(dot.expanded(6.0f));
        g.setColour(colour.withAlpha(0.34f));
        g.fillEllipse(dot.expanded(3.5f));
    }

    ColourGradient ledGradient(colour.brighter(0.35f), dot.getX(), dot.getY(), colour.darker(0.35f), dot.getRight(),
                               dot.getBottom(), false);
    g.setGradientFill(ledGradient);
    g.fillEllipse(dot);
    g.setColour(Colours::black.withAlpha(0.45f));
    g.drawEllipse(dot, 1.0f);
}

struct BrowserPalette
{
    Colour top;
    Colour bottom;
    Colour accent;
    Colour accent2;
    Colour led;
    Colour text;
    Colour face;
    Colour face2;
    Colour inset;
    Colour edge;
    Colour edgeHi;
};

BrowserPalette makeBrowserPalette()
{
    auto& colours = ::ColourScheme::getInstance().colours;
    const auto preset = ::ColourScheme::getInstance().presetName;

    auto palette = [](uint32 top, uint32 bottom, uint32 face, uint32 face2, uint32 inset, uint32 edge, uint32 edgeHi,
                      uint32 accent, uint32 accent2, uint32 led, uint32 text)
    {
        return BrowserPalette{Colour(top),    Colour(bottom), Colour(accent), Colour(accent2), Colour(led),
                              Colour(text),   Colour(face),   Colour(face2),  Colour(inset),   Colour(edge),
                              Colour(edgeHi)};
    };

    if (preset == "Midnight")
        return palette(0xFF211A2B, 0xFF140F1B, 0xFF271F33, 0xFF30273D, 0xFF0E0A14, 0xFF473A57, 0xFF5B4C6E,
                       0xFFFFB020, 0xFF36C8FF, 0xFF3DDC84, 0xFFF4ECDD);
    if (preset == "Deep Ocean")
        return palette(0xFF102029, 0xFF08131B, 0xFF142A36, 0xFF1B3543, 0xFF07121A, 0xFF2C5563, 0xFF3C6B7A,
                       0xFFFF9E3D, 0xFF2BD4FF, 0xFF00E0AD, 0xFFEAF3F1);
    if (preset == "Synthwave")
        return palette(0xFF1E0A28, 0xFF0F0518, 0xFF2A1139, 0xFF351747, 0xFF0C0414, 0xFF5A2D72, 0xFF76439A,
                       0xFFFF8A3D, 0xFFFF45FF, 0xFF1FFFA0, 0xFFF6EBFF);
    if (preset == "Forest")
        return palette(0xFF1C1D13, 0xFF10110A, 0xFF26281A, 0xFF2F3120, 0xFF0E0F08, 0xFF4A4D2E, 0xFF5F633D,
                       0xFFE6AD36, 0xFF79D479, 0xFF7CE87C, 0xFFF1EEDA);
    if (preset == "Daylight")
        return palette(0xFF3B332A, 0xFF2B241C, 0xFF473E33, 0xFF52483B, 0xFF241F18, 0xFF615648, 0xFF796B58,
                       0xFFFFB43A, 0xFF3AA6EC, 0xFF4DDC84, 0xFFF5EDDE);

    const auto accent = colours["Warning Colour"];
    const auto accent2 = colours["Audio Connection"];
    const auto top = colours["Window Background"].interpolatedWith(accent, 0.08f);
    const auto bottom = colours["Window Background"].darker(0.16f).interpolatedWith(accent, 0.04f);
    const auto face = colours["Dialog Inner Background"].interpolatedWith(accent, 0.055f);
    const auto face2 = colours["Dialog Inner Background"].brighter(0.08f).interpolatedWith(accent, 0.075f);
    const auto inset = colours["Window Background"].darker(0.06f).interpolatedWith(accent, 0.025f);
    const auto edge = colours["Plugin Border"].interpolatedWith(accent, 0.12f);

    return {top, bottom, accent, accent2, colours["Success Colour"], colours["Text Colour"], face, face2, inset, edge,
            edge.brighter(0.2f)};
}

Colour toneColourForTag(const String& tone)
{
    if (tone.equalsIgnoreCase("Clean"))
        return Colour(0xFF38BDF8);
    if (tone.equalsIgnoreCase("Crunch"))
        return makeBrowserPalette().accent;
    if (tone.equalsIgnoreCase("Hi-Gain"))
        return Colour(0xFFFF5D5D);
    if (tone.equalsIgnoreCase("Lead"))
        return Colour(0xFFC084FC);

    return makeBrowserPalette().text.withAlpha(0.5f);
}

Colour colourForNAMArchitecture(const String& architecture, const BrowserPalette& palette)
{
    if (architecture.containsIgnoreCase("LSTM"))
        return Colour(0xFFE8A838);
    if (architecture.containsIgnoreCase("WaveNet"))
        return Colour(0xFF38C8E8);
    if (architecture.containsIgnoreCase("ConvNet"))
        return Colour(0xFF58D868);
    if (architecture.containsIgnoreCase("Linear"))
        return Colour(0xFFB088E8);

    return palette.text.withAlpha(0.48f);
}

String normaliseNAMModelType(String modelType)
{
    modelType = modelType.trim();
    if (modelType.isEmpty() || modelType == "-")
        return "Model";

    const auto lower = modelType.toLowerCase();
    if (lower.contains("preamp") || lower.contains("pre-amp"))
        return "Preamp";
    if (lower.contains("full") || lower.contains("chain") || lower.contains("rig"))
        return "Full Rig";
    if (lower.contains("pedal"))
        return "Pedal";
    if (lower.contains("amp"))
        return "Amp";
    if (lower.contains("cab"))
        return "Cab";

    return modelType.length() > 12 ? modelType.substring(0, 12) : modelType;
}

Colour colourForNAMModelType(const String& modelType, const BrowserPalette& palette)
{
    const auto lower = modelType.toLowerCase();
    if (lower.contains("preamp") || lower.contains("pre-amp") || lower.contains("amp"))
        return Colour(0xFFE8A838);
    if (lower.contains("full") || lower.contains("chain") || lower.contains("rig"))
        return Colour(0xFF58D868);
    if (lower.contains("pedal"))
        return Colour(0xFF38C8E8);
    if (lower.contains("cab"))
        return palette.accent2;

    return palette.text.withAlpha(0.58f);
}

String getJsonStringField(const nlohmann::json& meta, const char* key)
{
    if (!meta.contains(key) || meta[key].is_null())
        return {};

    if (meta[key].is_string())
        return String(meta[key].get<std::string>());
    if (meta[key].is_number_integer())
        return String(static_cast<int64>(meta[key].get<int64_t>()));
    if (meta[key].is_number_float())
        return String(meta[key].get<double>(), 2);
    if (meta[key].is_boolean())
        return meta[key].get<bool>() ? "Yes" : "No";

    return {};
}

String getGearStringField(const nlohmann::json& meta, const char* key)
{
    if (!meta.contains("gear") || !meta["gear"].is_object())
        return {};

    const auto& gear = meta["gear"];
    return getJsonStringField(gear, key);
}

struct NAMPreviewFields
{
    String author = "-";
    String modelType = "-";
    String rig;
    String cabinet;
    String microphone;
    String note;
};

NAMPreviewFields extractNAMPreviewFields(const NAMModelInfo& model)
{
    NAMPreviewFields fields;

    if (model.metadata.empty())
        return fields;

    try
    {
        const auto meta = nlohmann::json::parse(model.metadata);

        fields.author = getJsonStringField(meta, "author");
        if (fields.author.isEmpty())
            fields.author = getJsonStringField(meta, "modeled_by");
        if (fields.author.isEmpty())
            fields.author = "-";

        fields.modelType = getJsonStringField(meta, "model_type");
        if (fields.modelType.isEmpty())
            fields.modelType = getJsonStringField(meta, "type");
        if (fields.modelType.isEmpty())
            fields.modelType = getJsonStringField(meta, "category");
        if (fields.modelType.isEmpty())
            fields.modelType = getJsonStringField(meta, "capture");
        if (fields.modelType.isEmpty())
            fields.modelType = getJsonStringField(meta, "gear_type");
        if (fields.modelType.isEmpty())
            fields.modelType = "-";

        fields.rig = getGearStringField(meta, "amp");
        if (fields.rig.isEmpty())
            fields.rig = getJsonStringField(meta, "amp");
        if (fields.rig.isEmpty())
            fields.rig = getJsonStringField(meta, "gear");
        if (fields.rig.isEmpty())
            fields.rig = getJsonStringField(meta, "name");

        fields.cabinet = getGearStringField(meta, "cabinet");
        if (fields.cabinet.isEmpty())
            fields.cabinet = getJsonStringField(meta, "cab");

        fields.microphone = getGearStringField(meta, "mic");
        if (fields.microphone.isEmpty())
            fields.microphone = getJsonStringField(meta, "mic");

        fields.note = getJsonStringField(meta, "description");
        if (fields.note.isEmpty())
            fields.note = getJsonStringField(meta, "notes");
    }
    catch (const std::exception&)
    {
        // Keep defaults when metadata is absent or not JSON.
    }

    return fields;
}

String makeNAMPreviewSummary(const NAMModelInfo& model, const NAMPreviewFields& fields)
{
    StringArray parts;
    if (fields.rig.isNotEmpty())
        parts.add(fields.rig);
    else if (fields.modelType != "-")
        parts.add(normaliseNAMModelType(fields.modelType));

    if (model.architecture.empty())
        parts.add("NAM");
    else
        parts.add(String(model.architecture));

    if (model.expectedSampleRate > 0.0)
        parts.add(formatNAMSampleRate(model.expectedSampleRate));
    const auto fileSize = formatNAMFileSize(model);
    if (fileSize != "-")
        parts.add(fileSize);

    return parts.joinIntoString("  |  ");
}

String makeNAMDetailDescription(const NAMModelInfo& model, const NAMPreviewFields& fields)
{
    if (fields.note.isNotEmpty())
        return fields.note.replaceCharacter('\n', ' ').trim();

    StringArray parts;
    if (fields.rig.isNotEmpty())
        parts.add(fields.rig);
    if (fields.cabinet.isNotEmpty())
        parts.add(fields.cabinet);
    if (fields.microphone.isNotEmpty())
        parts.add(fields.microphone);

    if (parts.isEmpty())
        return makeNAMPreviewSummary(model, fields);

    return parts.joinIntoString("  |  ");
}

String inferToneTag(const String& text)
{
    const auto haystack = text.toLowerCase();
    if (haystack.contains("rect") || haystack.contains("5150") || haystack.contains("metal") ||
        haystack.contains("high gain") || haystack.contains("hi-gain") || haystack.contains("rat") ||
        haystack.contains("muff") || haystack.contains("fuzz"))
        return "Hi-Gain";

    if (haystack.contains("lead") || haystack.contains("slo") || haystack.contains("soldano") ||
        haystack.contains("dumble") || haystack.contains("solo"))
        return "Lead";

    if (haystack.contains("clean") || haystack.contains("twin") || haystack.contains("jc-120") ||
        haystack.contains("jazz chorus") || haystack.contains("blackface"))
        return "Clean";

    if (haystack.contains("crunch") || haystack.contains("jcm") || haystack.contains("plexi") ||
        haystack.contains("klon") || haystack.contains("808") || haystack.contains("drive") ||
        haystack.contains("tweed") || haystack.contains("ac30"))
        return "Crunch";

    return {};
}

void drawBrowserChip(Graphics& g, Rectangle<float> bounds, const String& text, Colour colour, bool strong)
{
    if (bounds.isEmpty() || text.isEmpty())
        return;

    g.setColour(colour.withAlpha(strong ? 0.2f : 0.13f));
    g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.5f);
    g.setColour(colour.withAlpha(strong ? 0.68f : 0.48f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), bounds.getHeight() * 0.5f, 1.0f);
    g.setFont(FontManager::getInstance().getBadgeFont());
    g.setColour(colour.withAlpha(strong ? 0.94f : 0.78f));
    g.drawText(text, bounds, Justification::centred, true);
}

void drawLibraryRail(Graphics& g, Rectangle<float> bounds, const String& heading, const String& primary,
                     const String& secondary, const String& folder, int totalCount, int filteredCount, bool online)
{
    if (bounds.isEmpty())
        return;

    auto& colours = ::ColourScheme::getInstance().colours;
    auto& fonts = ::FontManager::getInstance();

    const auto palette = makeBrowserPalette();

    g.setColour(palette.face.withAlpha(0.94f));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(palette.edge);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);

    auto inner = bounds.reduced(12.0f, 12.0f);
    g.setFont(fonts.getBadgeFont());
    g.setColour(palette.text.withAlpha(0.46f));
    g.drawText(heading.toUpperCase(), inner.removeFromTop(18.0f), Justification::centredLeft, true);
    inner.removeFromTop(6.0f);

    auto drawRailRow = [&](const String& label, const String& value, bool active, Colour accent)
    {
        auto row = inner.removeFromTop(36.0f);
        g.setColour(active ? accent.withAlpha(0.15f) : palette.text.withAlpha(0.045f));
        g.fillRoundedRectangle(row, 8.0f);
        g.setColour(active ? accent.withAlpha(0.58f) : palette.text.withAlpha(0.055f));
        g.drawRoundedRectangle(row.reduced(0.5f), 8.0f, 1.0f);
        auto text = row.reduced(9.0f, 0.0f);
        g.setFont(fonts.getLabelFont());
        g.setColour(active ? accent : palette.text.withAlpha(0.66f));
        g.drawText(label, text.removeFromLeft(text.getWidth() - 38.0f), Justification::centredLeft, true);
        g.setFont(fonts.getMonoFont(11.0f));
        g.setColour(palette.text.withAlpha(active ? 0.9f : 0.46f));
        g.drawText(value, text, Justification::centredRight, true);
        inner.removeFromTop(6.0f);
    };

    drawRailRow(primary, String(totalCount), true, palette.accent);
    drawRailRow(secondary, String(filteredCount), filteredCount != totalCount, palette.accent2);

    inner.removeFromTop(8.0f);
    if (primary.equalsIgnoreCase("Models"))
    {
        g.setFont(fonts.getBadgeFont());
        g.setColour(palette.text.withAlpha(0.42f));
        g.drawText("TONE", inner.removeFromTop(18.0f), Justification::centredLeft, true);
        inner.removeFromTop(5.0f);

        const std::array<String, 4> tones = {"Clean", "Crunch", "Hi-Gain", "Lead"};
        for (const auto& tone : tones)
        {
            auto row = inner.removeFromTop(26.0f);
            const auto toneColour = toneColourForTag(tone);
            auto dot = row.removeFromLeft(20.0f).withSizeKeepingCentre(8.0f, 8.0f);
            g.setColour(toneColour.withAlpha(0.68f));
            g.fillEllipse(dot);
            g.setColour(toneColour.withAlpha(0.22f));
            g.drawEllipse(dot.expanded(2.0f), 1.0f);

            g.setFont(fonts.getLabelFont());
            g.setColour(palette.text.withAlpha(0.62f));
            g.drawText(tone, row, Justification::centredLeft, true);
        }

        inner.removeFromTop(8.0f);
    }

    g.setFont(fonts.getBadgeFont());
    g.setColour(palette.text.withAlpha(0.42f));
    g.drawText("SOURCE", inner.removeFromTop(18.0f), Justification::centredLeft, true);
    inner.removeFromTop(5.0f);

    auto status = inner.removeFromTop(30.0f);
    drawStatusLed(g, status.removeFromLeft(20.0f), online ? palette.led : palette.accent2, online);
    g.setFont(fonts.getLabelFont());
    g.setColour(palette.text.withAlpha(0.7f));
    g.drawText(online ? "Online" : "Local folder", status, Justification::centredLeft, true);

    inner.removeFromTop(6.0f);
    g.setFont(fonts.getMonoFont(10.5f));
    g.setColour(palette.text.withAlpha(0.38f));
    g.drawFittedText(folder, inner.toNearestInt(), Justification::topLeft, 3);
}
} // namespace

//==============================================================================
// Custom LookAndFeel for browser windows — rounded corners + dark title bar
//==============================================================================
class BrowserWindowLookAndFeel : public LookAndFeel_V4
{
  public:
    BrowserWindowLookAndFeel()
    {
        const auto palette = makeBrowserPalette();
        auto bg = palette.bottom;
        auto text = palette.text;

        // DocumentWindow colours
        setColour(DocumentWindow::backgroundColourId, bg);
        setColour(DocumentWindow::textColourId, text);

        // ResizableWindow background
        setColour(ResizableWindow::backgroundColourId, bg);
    }

    void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW,
                                    const Image* /*icon*/, bool /*drawTitleTextOnLeft*/) override
    {
        const auto palette = makeBrowserPalette();
        auto bg = palette.face;
        auto text = palette.text;

        // Title bar background with rounded top corners
        Path titlePath;
        titlePath.addRoundedRectangle(0.0f, 0.0f, (float)w, (float)h + cornerRadius, cornerRadius, cornerRadius, true,
                                      true, false, false);
        ColourGradient titleGradient(bg.brighter(0.08f), 0.0f, 0.0f, bg.darker(0.12f), 0.0f, (float)h, false);
        g.setGradientFill(titleGradient);
        g.fillPath(titlePath);

        // Subtle bottom border and accent trace
        g.setColour(text.withAlpha(0.08f));
        g.drawHorizontalLine(h - 1, 0.0f, (float)w);
        g.setColour(palette.accent.withAlpha(0.42f));
        g.drawLine((float)titleSpaceX, (float)h - 2.0f, (float)jmin(w - 34, titleSpaceX + titleSpaceW / 2),
                   (float)h - 2.0f, 1.0f);

        // Title text
        g.setColour(text.withAlpha(0.9f));
        g.setFont(FontManager::getInstance().getSubheadingFont());
        g.drawText(window.getName(), titleSpaceX, 0, titleSpaceW, h, Justification::centredLeft, true);
    }

    Button* createDocumentWindowButton(int buttonType) override
    {
        if (buttonType == DocumentWindow::closeButton)
        {
            auto* btn = new CloseButton();
            return btn;
        }
        return LookAndFeel_V4::createDocumentWindowButton(buttonType);
    }

    void drawResizableWindowBorder(Graphics& /*g*/, int /*w*/, int /*h*/, const BorderSize<int>& /*border*/,
                                   ResizableWindow& /*window*/) override
    {
        // No border — we draw rounded corners in paint() instead
    }

    void drawCornerResizer(Graphics& g, int w, int h, bool /*isMouseOver*/, bool /*isMouseDragging*/) override
    {
        const auto palette = makeBrowserPalette();
        g.setColour(palette.text.withAlpha(0.15f));
        // Small grip dots
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3 - i; ++j)
                g.fillEllipse((float)(w - 4 - i * 5), (float)(h - 4 - j * 5), 3.0f, 3.0f);
    }

    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& editor) override
    {
        const auto palette = makeBrowserPalette();
        auto bounds = Rectangle<float>(0, 0, (float)width, (float)height);
        float cr = editor.isMultiLine() ? 8.0f : height / 2.0f;

        ColourGradient fill(palette.inset, bounds.getX(), bounds.getY(), palette.face2, bounds.getX(),
                            bounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, cr);

        if (editor.isMultiLine())
        {
            g.setColour(Colours::black.withAlpha(0.12f));
            g.drawRoundedRectangle(bounds.reduced(2.0f), cr - 2.0f, 1.0f);
        }
    }

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& editor) override
    {
        const auto palette = makeBrowserPalette();
        auto bounds = Rectangle<float>(0, 0, (float)width, (float)height);
        float cr = editor.isMultiLine() ? 8.0f : height / 2.0f;

        // Subtle pill border — brighter when focused
        float alpha = editor.hasKeyboardFocus(true) ? 0.5f : 0.2f;
        g.setColour((editor.hasKeyboardFocus(true) ? palette.accent : palette.text).withAlpha(alpha));
        g.drawRoundedRectangle(bounds.reduced(0.5f), cr, 1.0f);
    }

    static constexpr float cornerRadius = 10.0f;

  private:
    // Custom close button with themed X
    class CloseButton : public Button
    {
      public:
        CloseButton() : Button("Close") {}

        void paintButton(Graphics& g, bool isMouseOverButton, bool isButtonDown) override
        {
            const auto palette = makeBrowserPalette();
            auto area = getLocalBounds().toFloat().reduced(4.0f);

            if (isButtonDown)
            {
                g.setColour(Colours::red.withAlpha(0.6f));
                g.fillEllipse(area);
            }
            else if (isMouseOverButton)
            {
                g.setColour(Colours::red.withAlpha(0.3f));
                g.fillEllipse(area);
            }

            // X symbol
            auto cross = area.reduced(area.getWidth() * 0.25f);
            g.setColour(isMouseOverButton ? palette.text : palette.text.withAlpha(0.7f));
            g.drawLine(cross.getX(), cross.getY(), cross.getRight(), cross.getBottom(), 1.5f);
            g.drawLine(cross.getRight(), cross.getY(), cross.getX(), cross.getBottom(), 1.5f);
        }
    };
};

//==============================================================================
// NAMModelListModel
//==============================================================================

void NAMModelListModel::setModels(const std::vector<NAMModelInfo>& newModels)
{
    allModels = newModels;
    rebuildFilteredList();
}

void NAMModelListModel::setFilter(const String& filter)
{
    currentFilter = filter.toLowerCase();
    rebuildFilteredList();
}

int NAMModelListModel::getNumRows()
{
    return static_cast<int>(filteredIndices.size());
}

void NAMModelListModel::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto palette = makeBrowserPalette();
    const int margin = 6;
    const bool compact = width < 330 || height <= 50;
    const float cornerRadius = 8.0f;
    Rectangle<float> itemBounds(static_cast<float>(margin), 3.0f, static_cast<float>(width - margin * 2),
                                static_cast<float>(height - 6));

    // Card background for every item
    if (rowIsSelected)
    {
        ColourGradient selectedFill(palette.inset.interpolatedWith(palette.accent, 0.14f), itemBounds.getX(),
                                    itemBounds.getY(), palette.inset.interpolatedWith(palette.accent, 0.06f),
                                    itemBounds.getX(), itemBounds.getBottom(), false);
        g.setGradientFill(selectedFill);
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent.withAlpha(0.74f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);

        // Left-edge accent stripe (DAW-style selection indicator)
        Rectangle<float> stripe(itemBounds.getX(), itemBounds.getY() + 4.0f, 3.0f, itemBounds.getHeight() - 8.0f);
        g.setColour(palette.accent);
        g.fillRoundedRectangle(stripe, 1.5f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(palette.text.withAlpha(0.08f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent.withAlpha(0.28f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);
    }
    else
    {
        // Subtle card background for non-selected items
        g.setColour(palette.inset.interpolatedWith(palette.face, 0.18f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.edge.withAlpha(0.5f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 0.5f);
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& model = allModels[filteredIndices[rowNumber]];
        const int glyphSize = compact ? 30 : 38;
        const int textX = margin + (compact ? 46 : 58);

        // Extract rig type and model type from metadata
        String rigType;
        String modelType;
        if (!model.metadata.empty())
        {
            try
            {
                auto meta = nlohmann::json::parse(model.metadata);

                // Try to get amp/gear info in order of preference
                if (meta.contains("gear") && meta["gear"].is_object())
                {
                    const auto& gear = meta["gear"];
                    if (gear.contains("amp") && gear["amp"].is_string())
                        rigType = String(gear["amp"].get<std::string>());
                }
                if (rigType.isEmpty() && meta.contains("amp") && meta["amp"].is_string())
                    rigType = String(meta["amp"].get<std::string>());
                if (rigType.isEmpty() && meta.contains("gear") && meta["gear"].is_string())
                    rigType = String(meta["gear"].get<std::string>());
                if (rigType.isEmpty() && meta.contains("name") && meta["name"].is_string())
                    rigType = String(meta["name"].get<std::string>());

                // Try to get model type (preamp/amp/full chain)
                if (meta.contains("model_type") && meta["model_type"].is_string())
                    modelType = String(meta["model_type"].get<std::string>());
                else if (meta.contains("type") && meta["type"].is_string())
                    modelType = String(meta["type"].get<std::string>());
                else if (meta.contains("category") && meta["category"].is_string())
                    modelType = String(meta["category"].get<std::string>());
                else if (meta.contains("capture") && meta["capture"].is_string())
                    modelType = String(meta["capture"].get<std::string>());
                else if (meta.contains("gear_type") && meta["gear_type"].is_string())
                    modelType = String(meta["gear_type"].get<std::string>());
            }
            catch (const std::exception&)
            {
                // Ignore parse errors
            }
        }

        // Badge layout - rightmost is architecture, then model type and inferred tone.
        const int badgeHeight = 18;
        const int badgeSpacing = 5;
        int badgeX = width - margin - 10;

        // Architecture badge (rightmost)
        const int archBadgeWidth = 50;
        badgeX -= archBadgeWidth;

        String archShort(model.architecture);
        const auto archColour = colourForNAMArchitecture(archShort, palette);

        drawModelGlyph(g,
                       Rectangle<float>((float)(margin + 12), (height - static_cast<float>(glyphSize)) * 0.5f,
                                        static_cast<float>(glyphSize), static_cast<float>(glyphSize)),
                       archColour, rowIsSelected);

        if (!compact)
        {
            Rectangle<float> archBadgeBounds(static_cast<float>(badgeX), (height - badgeHeight) / 2.0f,
                                             static_cast<float>(archBadgeWidth), static_cast<float>(badgeHeight));
            drawBrowserChip(g, archBadgeBounds, archShort, archColour, rowIsSelected);

            // Model type badge (left of architecture badge, if we have type info)
            if (modelType.isNotEmpty())
            {
                badgeX -= badgeSpacing;

                const auto typeDisplay = normaliseNAMModelType(modelType);
                const auto typeColour = colourForNAMModelType(modelType, palette);

                int typeBadgeWidth =
                    static_cast<int>(FontManager::getInstance().getBadgeFont().getStringWidthFloat(typeDisplay)) + 12;
                badgeX -= typeBadgeWidth;

                Rectangle<float> typeBadgeBounds(static_cast<float>(badgeX), (height - badgeHeight) / 2.0f,
                                                 static_cast<float>(typeBadgeWidth), static_cast<float>(badgeHeight));
                drawBrowserChip(g, typeBadgeBounds, typeDisplay, typeColour, rowIsSelected);
            }

            const auto toneTag = inferToneTag(String(model.name) + " " + rigType + " " + modelType);
            if (toneTag.isNotEmpty())
            {
                badgeX -= badgeSpacing;
                const auto toneColour = toneColourForTag(toneTag);
                const int toneBadgeWidth =
                    static_cast<int>(FontManager::getInstance().getBadgeFont().getStringWidthFloat(toneTag)) + 14;
                badgeX -= toneBadgeWidth;
                Rectangle<float> toneBadgeBounds(static_cast<float>(badgeX), (height - badgeHeight) / 2.0f,
                                                 static_cast<float>(toneBadgeWidth), static_cast<float>(badgeHeight));
                drawBrowserChip(g, toneBadgeBounds, toneTag, toneColour, rowIsSelected);
            }
        }

        // Model name (top line) - adjust width to not overlap badges
        const int textEndX = compact ? width - margin - 10 : jmax(textX + 80, badgeX - 8);
        const int topPad = compact ? 7 : 10;
        const int nameH = compact ? 17 : 20;
        g.setColour(rowIsSelected ? palette.text : palette.text.withAlpha(0.95f));
        g.setFont(compact ? FontManager::getInstance().getBodyBoldFont()
                          : FontManager::getInstance().getSubheadingFont());
        g.drawText(String(model.name).replace("_", " "), textX, topPad, textEndX - textX, nameH,
                   Justification::centredLeft, true);

        // Rig type and sample rate info on second line
        String infoLine;
        if (rigType.isNotEmpty())
        {
            // Truncate long rig names
            if (rigType.length() > 50)
                rigType = rigType.substring(0, 47) + "...";
            infoLine = rigType;
        }
        if (model.expectedSampleRate > 0)
        {
            if (infoLine.isNotEmpty())
                infoLine += "  |  ";
            infoLine += formatNAMSampleRate(model.expectedSampleRate);
        }

        if (infoLine.isNotEmpty())
        {
            g.setColour(palette.text.withAlpha(0.45f));
            g.setFont(compact ? FontManager::getInstance().getCaptionFont() : FontManager::getInstance().getLabelFont());
            g.drawText(infoLine, textX, topPad + nameH + 1, textEndX - textX, compact ? 15 : 16,
                       Justification::centredLeft, true);
        }
    }

    // Subtle bottom separator
    if (!rowIsSelected)
    {
        g.setColour(palette.text.withAlpha(0.05f));
        g.drawLine(static_cast<float>(margin + 4), static_cast<float>(height - 1),
                   static_cast<float>(width - margin - 4), static_cast<float>(height - 1), 1.0f);
    }
}

const NAMModelInfo* NAMModelListModel::getModelAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(filteredIndices.size()))
    {
        return &allModels[filteredIndices[index]];
    }
    return nullptr;
}

void NAMModelListModel::rebuildFilteredList()
{
    filteredIndices.clear();

    for (size_t i = 0; i < allModels.size(); ++i)
    {
        if (currentFilter.isEmpty())
        {
            filteredIndices.push_back(i);
        }
        else
        {
            String name = String(allModels[i].name).toLowerCase();
            String arch = String(allModels[i].architecture).toLowerCase();
            String metadata = String(allModels[i].metadata).toLowerCase();
            String filePath = String(allModels[i].filePath).toLowerCase();

            if (name.contains(currentFilter) || arch.contains(currentFilter) || metadata.contains(currentFilter) ||
                filePath.contains(currentFilter))
            {
                filteredIndices.push_back(i);
            }
        }
    }
}

//==============================================================================
// IRListModel
//==============================================================================

void IRListModel::setFiles(const std::vector<IRFileInfo>& newFiles)
{
    allFiles = newFiles;
    rebuildFilteredList();
}

void IRListModel::setFilter(const String& filter)
{
    currentFilter = filter.toLowerCase();
    rebuildFilteredList();
}

int IRListModel::getNumRows()
{
    return static_cast<int>(filteredIndices.size());
}

void IRListModel::paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected)
{
    const auto palette = makeBrowserPalette();
    const bool compact = width < 300 || height <= 48;
    const int margin = compact ? 5 : 6;
    const float cornerRadius = 6.0f;
    Rectangle<float> itemBounds(static_cast<float>(margin), 2.0f, static_cast<float>(width - margin * 2),
                                static_cast<float>(height - 4));

    // Background with rounded corners
    if (rowIsSelected)
    {
        ColourGradient selectedFill(palette.accent2.withAlpha(0.2f), itemBounds.getX(), itemBounds.getY(),
                                    palette.face2.withAlpha(0.85f), itemBounds.getX(), itemBounds.getBottom(), false);
        g.setGradientFill(selectedFill);
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent2.withAlpha(0.55f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);

        // Left-edge accent stripe (DAW-style selection indicator)
        Rectangle<float> stripe(itemBounds.getX(), itemBounds.getY() + 2.0f, 3.0f, itemBounds.getHeight() - 4.0f);
        g.setColour(palette.accent2);
        g.fillRoundedRectangle(stripe, 1.5f);
    }
    else if (rowNumber == hoveredRow)
    {
        g.setColour(palette.text.withAlpha(0.05f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.accent2.withAlpha(0.24f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 1.0f);
    }
    else
    {
        g.setColour(palette.face.withAlpha(0.24f));
        g.fillRoundedRectangle(itemBounds, cornerRadius);
        g.setColour(palette.edge.withAlpha(0.45f));
        g.drawRoundedRectangle(itemBounds, cornerRadius, 0.5f);
    }

    if (rowNumber >= 0 && rowNumber < static_cast<int>(filteredIndices.size()))
    {
        const auto& ir = allFiles[filteredIndices[rowNumber]];
        const int glyphSize = compact ? 26 : 30;
        const int textX = margin + (compact ? 42 : 52);
        const int badgeWidth = compact ? 46 : 50;
        const int badgeHeight = compact ? 17 : 18;
        const bool showDurationBadge = width >= (compact ? 235 : 250);
        const int badgeX = showDurationBadge ? width - margin - badgeWidth - 8 : width - margin - 8;
        const Colour irAccent = palette.accent2;

        drawIRGlyph(g, Rectangle<float>((float)(margin + (compact ? 8 : 10)),
                                        (height - static_cast<float>(glyphSize)) * 0.5f,
                                        static_cast<float>(glyphSize), static_cast<float>(glyphSize)), irAccent,
                    rowIsSelected);

        // Duration badge
        String durationText;
        if (ir.durationSeconds > 0)
        {
            if (ir.durationSeconds >= 1.0)
                durationText = String(ir.durationSeconds, 2) + "s";
            else
                durationText = String(static_cast<int>(ir.durationSeconds * 1000)) + "ms";
        }

        if (durationText.isNotEmpty() && showDurationBadge)
        {
            Rectangle<float> badgeBounds(static_cast<float>(badgeX), (height - badgeHeight) / 2.0f,
                                         static_cast<float>(badgeWidth), static_cast<float>(badgeHeight));
            Colour badgeColour = irAccent; // bright cyan for IR duration
            g.setColour(badgeColour.withAlpha(0.2f));
            g.fillRoundedRectangle(badgeBounds, badgeHeight / 2.0f);
            g.setColour(badgeColour);
            g.drawRoundedRectangle(badgeBounds, badgeHeight / 2.0f, 1.0f);

            g.setFont(FontManager::getInstance().getCaptionFont());
            g.setColour(badgeColour);
            g.drawText(durationText, badgeBounds, Justification::centred, true);
        }

        // IR name
        g.setColour(rowIsSelected ? palette.text : palette.text.withAlpha(0.95f));
        g.setFont(compact ? FontManager::getInstance().getBodyBoldFont().withHeight(12.0f)
                          : FontManager::getInstance().getBodyBoldFont());
        const int nameHeight = compact ? jmax(18, height / 2) : height / 2;
        g.drawText(String(ir.name), textX, compact ? 5 : 4, badgeX - textX - 8, nameHeight,
                   Justification::centredLeft, true);

        // Sample rate and channels on bottom line
        String details;
        if (ir.sampleRate > 0)
            details = formatIRSampleRate(ir.sampleRate);
        if (ir.numChannels > 0)
        {
            if (details.isNotEmpty())
                details += "  |  ";
            details += formatIRChannels(ir.numChannels);
        }

        if (details.isNotEmpty())
        {
            g.setColour(palette.text.withAlpha(0.5f));
            g.setFont(FontManager::getInstance().getMonoFont(compact ? 10.0f : 11.0f));
            g.drawText(details, textX, compact ? height / 2 - 1 : height / 2, badgeX - textX - 8,
                       height / 2 - (compact ? 2 : 4), Justification::centredLeft, true);
        }
    }

    // Subtle bottom separator
    if (!rowIsSelected)
    {
        g.setColour(palette.text.withAlpha(0.05f));
        g.drawLine(static_cast<float>(margin + 4), static_cast<float>(height - 1),
                   static_cast<float>(width - margin - 4), static_cast<float>(height - 1), 1.0f);
    }
}

const IRFileInfo* IRListModel::getFileAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(filteredIndices.size()))
    {
        return &allFiles[filteredIndices[index]];
    }
    return nullptr;
}

void IRListModel::rebuildFilteredList()
{
    filteredIndices.clear();

    for (size_t i = 0; i < allFiles.size(); ++i)
    {
        if (currentFilter.isEmpty())
        {
            filteredIndices.push_back(i);
        }
        else
        {
            String name = String(allFiles[i].name).toLowerCase();

            if (name.contains(currentFilter))
            {
                filteredIndices.push_back(i);
            }
        }
    }
}

//==============================================================================
// PillTabLookAndFeel - Custom look for pill-style tab buttons
//==============================================================================

class PillTabLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/, bool isMouseOverButton,
                              bool isButtonDown) override
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        const auto palette = makeBrowserPalette();
        auto bounds = button.getLocalBounds().toFloat().reduced(2);
        float cornerRadius = bounds.getHeight() / 2;

        if (button.getToggleState())
        {
            // Active: raised faceplate tab with amber text, matching the mockup browser.
            auto fillTop = palette.face2.brighter(0.08f);
            auto fillBottom = palette.face.darker(0.04f);
            if (isButtonDown)
            {
                fillTop = fillTop.darker(0.1f);
                fillBottom = fillBottom.darker(0.1f);
            }
            else if (isMouseOverButton)
            {
                fillTop = fillTop.brighter(0.07f);
                fillBottom = fillBottom.brighter(0.04f);
            }

            ColourGradient active(fillTop, bounds.getX(), bounds.getY(), fillBottom, bounds.getX(), bounds.getBottom(),
                                  false);
            g.setGradientFill(active);
            g.fillRoundedRectangle(bounds, cornerRadius);
            g.setColour(palette.accent.withAlpha(0.5f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
            g.setColour(Colours::white.withAlpha(0.1f));
            g.drawLine(bounds.getX() + 7.0f, bounds.getY() + 1.5f, bounds.getRight() - 7.0f, bounds.getY() + 1.5f,
                       1.0f);
        }
        else
        {
            // Inactive: subtle hover effect only
            if (isMouseOverButton || isButtonDown)
            {
                g.setColour(palette.accent.withAlpha(0.1f));
                g.fillRoundedRectangle(bounds, cornerRadius);
            }
        }
    }

    void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        const auto palette = makeBrowserPalette();
        auto bounds = button.getLocalBounds().toFloat();

        g.setFont(FontManager::getInstance().getBodyBoldFont());

        if (button.getToggleState())
            g.setColour(palette.accent.withAlpha(0.94f));
        else
            g.setColour(palette.text.withAlpha(0.7f));

        g.drawText(button.getButtonText(), bounds, Justification::centred);
    }
};

static PillTabLookAndFeel pillTabLookAndFeel;

class BrowserActionButtonLookAndFeel : public LookAndFeel_V4
{
  public:
    void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton,
                              bool isButtonDown) override
    {
        auto& colours = ::ColourScheme::getInstance().colours;
        const auto palette = makeBrowserPalette();
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const auto buttonText = button.findColour(TextButton::textColourOffId);
        const bool danger = button.findColour(TextButton::textColourOffId).getHue() > 0.95f ||
                            button.getButtonText().containsIgnoreCase("delete");
        const bool primary = !danger && buttonText == palette.accent;
        auto base = danger ? colours["Danger Colour"].withAlpha(0.25f)
                           : (primary ? palette.inset.withAlpha(0.96f) : palette.face2.withAlpha(0.92f));

        if (!button.isEnabled())
            base = base.withMultipliedSaturation(0.35f).withAlpha(0.34f);
        else if (isButtonDown)
            base = base.darker(0.12f);
        else if (isMouseOverButton)
            base = base.brighter(0.1f);

        const auto radius = juce::jmin(9.0f, bounds.getHeight() * 0.28f);
        if (button.isEnabled())
        {
            g.setColour(Colours::black.withAlpha(isButtonDown ? 0.12f : 0.26f));
            g.fillRoundedRectangle(bounds.translated(0.0f, isButtonDown ? 0.8f : 2.0f), radius);
        }

        ColourGradient fill(base.brighter(primary ? 0.16f : 0.1f), bounds.getX(), bounds.getY(),
                            base.darker(primary ? 0.18f : 0.12f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill(fill);
        g.fillRoundedRectangle(bounds, radius);

        g.setColour(Colours::white.withAlpha(primary ? 0.16f : 0.07f));
        g.drawLine(bounds.getX() + 5.0f, bounds.getY() + 1.5f, bounds.getRight() - 5.0f, bounds.getY() + 1.5f, 1.0f);
        g.setColour((danger ? colours["Danger Colour"] : (primary ? palette.accent : palette.text))
                        .withAlpha(primary ? 0.62f : 0.22f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), radius, primary ? 1.4f : 1.0f);
    }

    void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
    {
        const auto palette = makeBrowserPalette();
        auto text = button.findColour(button.getToggleState() ? TextButton::textColourOnId : TextButton::textColourOffId);
        if (!button.isEnabled())
            text = palette.text.withAlpha(0.32f);

        g.setFont(FontManager::getInstance().getBodyBoldFont());
        g.setColour(text);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(7, 2), Justification::centred, 1);
    }
};

static BrowserActionButtonLookAndFeel browserActionButtonLookAndFeel;

//==============================================================================
// NAMModelBrowserComponent
//==============================================================================

NAMModelBrowserComponent::NAMModelBrowserComponent(NAMProcessor* processor, std::function<void()> onModelLoaded)
    : namProcessor(processor), onModelLoadedCallback(std::move(onModelLoaded))
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto palette = makeBrowserPalette();

    // Title
    titleLabel = std::make_unique<Label>("title", "NAM Library");
    titleLabel->setFont(FontManager::getInstance().getHeadingFont());
    titleLabel->setColour(Label::textColourId, palette.text);
    addAndMakeVisible(titleLabel.get());

    // Tab buttons with pill-style look
    localTabButton = std::make_unique<TextButton>("Local");
    localTabButton->setClickingTogglesState(true);
    localTabButton->setToggleState(true, dontSendNotification);
    localTabButton->setRadioGroupId(1);
    localTabButton->setLookAndFeel(&pillTabLookAndFeel);
    localTabButton->addListener(this);
    addAndMakeVisible(localTabButton.get());

    onlineTabButton = std::make_unique<TextButton>("Online");
    onlineTabButton->setClickingTogglesState(true);
    onlineTabButton->setRadioGroupId(1);
    onlineTabButton->setLookAndFeel(&pillTabLookAndFeel);
    onlineTabButton->addListener(this);
    addAndMakeVisible(onlineTabButton.get());

    irTabButton = std::make_unique<TextButton>("IRs");
    irTabButton->setClickingTogglesState(true);
    irTabButton->setRadioGroupId(1);
    irTabButton->setLookAndFeel(&pillTabLookAndFeel);
    irTabButton->addListener(this);
    addAndMakeVisible(irTabButton.get());

    // Online browser component (created but initially hidden)
    onlineBrowser = std::make_unique<NAMOnlineBrowserComponent>(processor, onModelLoaded);
    onlineBrowser->setVisible(false);
    addAndMakeVisible(onlineBrowser.get());

    // Search box with rounded styling
    searchBox = std::make_unique<TextEditor>("search");
    searchBox->setTextToShowWhenEmpty("Search models, makers, authors...", palette.text.withAlpha(0.5f));
    searchBox->addListener(this);
    searchBox->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
    searchBox->setColour(TextEditor::textColourId, palette.text);
    searchBox->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    searchBox->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    searchBox->setIndents(28, 6); // Left indent for search icon, top indent to center text
    searchBox->setFont(FontManager::getInstance().getSubheadingFont()); // 15px fills the pill better
    addAndMakeVisible(searchBox.get());

    // Buttons with styled colors
    auto styleButton = [&colours, &palette](TextButton* btn, bool isPrimary = false)
    {
        btn->setLookAndFeel(&browserActionButtonLookAndFeel);
        if (isPrimary)
        {
            // Primary action button (accent color)
            btn->setColour(TextButton::buttonColourId, palette.accent);
            btn->setColour(TextButton::buttonOnColourId, palette.accent.brighter(0.2f));
            btn->setColour(TextButton::textColourOffId, palette.accent);
            btn->setColour(TextButton::textColourOnId, palette.accent.brighter(0.12f));
        }
        else
        {
            // Secondary button
            btn->setColour(TextButton::buttonColourId, palette.face2);
            btn->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
            btn->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
            btn->setColour(TextButton::textColourOnId, palette.text);
        }
    };

    refreshButton = std::make_unique<TextButton>("Refresh");
    refreshButton->addListener(this);
    styleButton(refreshButton.get());
    addAndMakeVisible(refreshButton.get());

    browseFolderButton = std::make_unique<TextButton>("Browse Folder...");
    browseFolderButton->addListener(this);
    styleButton(browseFolderButton.get());
    addAndMakeVisible(browseFolderButton.get());

    loadButton = std::make_unique<TextButton>("Load Model");
    loadButton->addListener(this);
    styleButton(loadButton.get(), true); // Primary action
    addAndMakeVisible(loadButton.get());

    closeButton = std::make_unique<TextButton>("Close");
    closeButton->addListener(this);
    styleButton(closeButton.get());
    addAndMakeVisible(closeButton.get());

    // Model list - transparent background for custom rounded painting
    modelList = std::make_unique<ListBox>("models", &listModel);
    modelList->setRowHeight(58);
    modelList->setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    modelList->setColour(ListBox::outlineColourId, Colours::transparentBlack);
    modelList->setOutlineThickness(0);
    modelList->setMultipleSelectionEnabled(false);
    modelList->addMouseListener(this, true);
    addAndMakeVisible(modelList.get());

    // Details panel
    detailsTitle = std::make_unique<Label>("detailsTitle", "Selected Model");
    detailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    detailsTitle->setColour(Label::textColourId, palette.text);
    addAndMakeVisible(detailsTitle.get());

    auto createLabelPair = [&palette, this](std::unique_ptr<Label>& labelPtr, std::unique_ptr<Label>& valuePtr,
                                            const String& labelText, const String& valueText)
    {
        labelPtr = std::make_unique<Label>("", labelText);
        labelPtr->setFont(FontManager::getInstance().getLabelFont());
        labelPtr->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
        addAndMakeVisible(labelPtr.get());

        valuePtr = std::make_unique<Label>("", valueText);
        valuePtr->setFont(FontManager::getInstance().getBodyBoldFont());
        valuePtr->setColour(Label::textColourId, palette.text.withAlpha(0.9f));
        addAndMakeVisible(valuePtr.get());
    };

    createLabelPair(nameLabel, nameValue, "Name:", "-");
    nameLabel->setText("Selected:", dontSendNotification);
    nameValue->setFont(FontManager::getInstance().getSubheadingFont());
    createLabelPair(authorLabel, authorValue, "Author:", "-");
    createLabelPair(modelTypeLabel, modelTypeValue, "Type:", "-");
    createLabelPair(architectureLabel, architectureValue, "Architecture:", "-");
    createLabelPair(sampleRateLabel, sampleRateValue, "Sample Rate:", "-");
    createLabelPair(loudnessLabel, loudnessValue, "Loudness:", "-");

    modelDescriptionLabel = std::make_unique<Label>("modelDescription", "Select a model to preview its capture notes.");
    modelDescriptionLabel->setFont(FontManager::getInstance().getLabelFont());
    modelDescriptionLabel->setColour(Label::textColourId, palette.text.withAlpha(0.66f));
    modelDescriptionLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(modelDescriptionLabel.get());

    metadataLabel = std::make_unique<Label>("", "Metadata:");
    metadataLabel->setFont(FontManager::getInstance().getLabelFont());
    metadataLabel->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
    addAndMakeVisible(metadataLabel.get());

    metadataDisplay = std::make_unique<TextEditor>("metadata");
    metadataDisplay->setMultiLine(true);
    metadataDisplay->setReadOnly(true);
    metadataDisplay->setColour(TextEditor::backgroundColourId, palette.inset);
    metadataDisplay->setColour(TextEditor::textColourId, palette.text.withAlpha(0.84f));
    metadataDisplay->setColour(TextEditor::outlineColourId, palette.edge);
    metadataDisplay->setFont(FontManager::getInstance().getMonoFont(11.0f));
    addAndMakeVisible(metadataDisplay.get());

    // File path in details
    createLabelPair(filePathLabel, filePathValue, "File:", "-");
    filePathValue->setMinimumHorizontalScale(0.5f);

    // Delete button
    deleteButton = std::make_unique<TextButton>("Delete Model");
    deleteButton->addListener(this);
    deleteButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    // Delete button — secondary style (outline, not filled)
    deleteButton->setColour(TextButton::buttonColourId, palette.face2);
    deleteButton->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    deleteButton->setColour(TextButton::textColourOffId, colours["Danger Colour"]);
    deleteButton->setColour(TextButton::textColourOnId, colours["Danger Colour"]);
    addAndMakeVisible(deleteButton.get());

    // Status bar
    statusLabel = std::make_unique<Label>("status", "");
    statusLabel->setFont(FontManager::getInstance().getCaptionFont());
    statusLabel->setColour(Label::textColourId, palette.text.withAlpha(0.6f));
    addAndMakeVisible(statusLabel.get());

    // Empty state message
    emptyStateLabel = std::make_unique<Label>(
        "emptyState",
        makeEmptyStateCopy("No NAM models found", "Use Browse Folder to select a folder or switch to Online.",
                           String()));
    emptyStateLabel->setFont(FontManager::getInstance().getBodyFont());
    emptyStateLabel->setColour(Label::textColourId, palette.text.withAlpha(0.4f));
    emptyStateLabel->setJustificationType(Justification::centred);
    emptyStateLabel->setVisible(false);
    addAndMakeVisible(emptyStateLabel.get());

    // IR browser components
    irList = std::make_unique<ListBox>("irs", &irListModel);
    irList->setRowHeight(52);
    irList->setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    irList->setColour(ListBox::outlineColourId, Colours::transparentBlack);
    irList->setOutlineThickness(0);
    irList->setMultipleSelectionEnabled(false);
    irList->addMouseListener(this, true);
    irList->setVisible(false);
    addAndMakeVisible(irList.get());

    irEmptyStateLabel = std::make_unique<Label>(
        "irEmptyState", makeEmptyStateCopy("No impulse responses found", "Use Browse IR Folder to choose a cabinet folder.",
                                           String()));
    irEmptyStateLabel->setFont(FontManager::getInstance().getBodyFont());
    irEmptyStateLabel->setColour(Label::textColourId, palette.text.withAlpha(0.4f));
    irEmptyStateLabel->setJustificationType(Justification::centred);
    irEmptyStateLabel->setVisible(false);
    addAndMakeVisible(irEmptyStateLabel.get());

    irBrowseFolderButton = std::make_unique<TextButton>("Browse IR Folder...");
    irBrowseFolderButton->addListener(this);
    irBrowseFolderButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    irBrowseFolderButton->setColour(TextButton::buttonColourId, palette.face2);
    irBrowseFolderButton->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    irBrowseFolderButton->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
    irBrowseFolderButton->setColour(TextButton::textColourOnId, palette.text);
    irBrowseFolderButton->setVisible(false);
    addAndMakeVisible(irBrowseFolderButton.get());

    irLoadButton = std::make_unique<TextButton>("Load IR");
    irLoadButton->addListener(this);
    irLoadButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    irLoadButton->setColour(TextButton::buttonColourId, palette.accent);
    irLoadButton->setColour(TextButton::buttonOnColourId, palette.accent.brighter(0.2f));
    irLoadButton->setColour(TextButton::textColourOffId, palette.accent);
    irLoadButton->setColour(TextButton::textColourOnId, palette.accent.brighter(0.12f));
    irLoadButton->setVisible(false);
    addAndMakeVisible(irLoadButton.get());

    // IR details panel
    irDetailsTitle = std::make_unique<Label>("irDetailsTitle", "IR Details");
    irDetailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    irDetailsTitle->setColour(Label::textColourId, palette.text);
    irDetailsTitle->setVisible(false);
    addAndMakeVisible(irDetailsTitle.get());

    auto createIRLabelPair = [&palette, this](std::unique_ptr<Label>& labelPtr, std::unique_ptr<Label>& valuePtr,
                                              const String& labelText, const String& valueText)
    {
        labelPtr = std::make_unique<Label>("", labelText);
        labelPtr->setFont(FontManager::getInstance().getLabelFont());
        labelPtr->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
        labelPtr->setVisible(false);
        addAndMakeVisible(labelPtr.get());

        valuePtr = std::make_unique<Label>("", valueText);
        valuePtr->setFont(FontManager::getInstance().getLabelFont());
        valuePtr->setColour(Label::textColourId, palette.text.withAlpha(0.9f));
        valuePtr->setVisible(false);
        addAndMakeVisible(valuePtr.get());
    };

    createIRLabelPair(irNameLabel, irNameValue, "Name:", "-");
    createIRLabelPair(irDurationLabel, irDurationValue, "Duration:", "-");
    createIRLabelPair(irSampleRateLabel, irSampleRateValue, "Sample Rate:", "-");
    createIRLabelPair(irChannelsLabel, irChannelsValue, "Channels:", "-");
    createIRLabelPair(irFileSizeLabel, irFileSizeValue, "File Size:", "-");
    createIRLabelPair(irFilePathLabel, irFilePathValue, "File:", "-");
    irFilePathValue->setMinimumHorizontalScale(0.5f);

    // Start with NAM download directory (same as Tone3000DownloadManager uses)
    currentDirectory =
        File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("Pedalboard3").getChildFile("NAM Models");

    // Create NAM Models directory if it doesn't exist
    if (!currentDirectory.isDirectory())
        currentDirectory.createDirectory();

    // IR directory (separate from NAM models)
    irDirectory = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("Pedalboard3").getChildFile("IR");

    // Create IR directory if it doesn't exist
    if (!irDirectory.isDirectory())
        irDirectory.createDirectory();

    setSize(700, 500);

    // Auto-scan on creation
    scanDirectory(currentDirectory);
}

NAMModelBrowserComponent::~NAMModelBrowserComponent()
{
    // Clear custom LookAndFeel before destruction
    localTabButton->setLookAndFeel(nullptr);
    onlineTabButton->setLookAndFeel(nullptr);
    irTabButton->setLookAndFeel(nullptr);
    refreshButton->setLookAndFeel(nullptr);
    browseFolderButton->setLookAndFeel(nullptr);
    loadButton->setLookAndFeel(nullptr);
    closeButton->setLookAndFeel(nullptr);
    deleteButton->setLookAndFeel(nullptr);
    irBrowseFolderButton->setLookAndFeel(nullptr);
    irLoadButton->setLookAndFeel(nullptr);
}

void NAMModelBrowserComponent::refreshColours()
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto palette = makeBrowserPalette();

    // Title
    titleLabel->setColour(Label::textColourId, palette.text);

    // Search box
    searchBox->setColour(TextEditor::backgroundColourId, palette.inset);
    searchBox->setColour(TextEditor::textColourId, palette.text);
    searchBox->setColour(TextEditor::outlineColourId, palette.edge);

    // Style helper (mirrors constructor logic)
    auto styleButton = [&palette](TextButton* btn, bool isPrimary = false)
    {
        btn->setLookAndFeel(&browserActionButtonLookAndFeel);
        if (isPrimary)
        {
            btn->setColour(TextButton::buttonColourId, palette.accent);
            btn->setColour(TextButton::buttonOnColourId, palette.accent.brighter(0.2f));
            btn->setColour(TextButton::textColourOffId, palette.accent);
            btn->setColour(TextButton::textColourOnId, palette.accent.brighter(0.12f));
        }
        else
        {
            btn->setColour(TextButton::buttonColourId, palette.face2);
            btn->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
            btn->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
            btn->setColour(TextButton::textColourOnId, palette.text);
        }
    };

    styleButton(refreshButton.get());
    styleButton(browseFolderButton.get());
    styleButton(loadButton.get(), true);
    styleButton(closeButton.get());

    // Details panel labels
    detailsTitle->setColour(Label::textColourId, palette.text);
    auto refreshLabelPair = [&palette](Label* label, Label* value)
    {
        label->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
        value->setColour(Label::textColourId, palette.text.withAlpha(0.9f));
    };
    refreshLabelPair(nameLabel.get(), nameValue.get());
    refreshLabelPair(authorLabel.get(), authorValue.get());
    refreshLabelPair(modelTypeLabel.get(), modelTypeValue.get());
    refreshLabelPair(architectureLabel.get(), architectureValue.get());
    refreshLabelPair(sampleRateLabel.get(), sampleRateValue.get());
    refreshLabelPair(loudnessLabel.get(), loudnessValue.get());
    refreshLabelPair(filePathLabel.get(), filePathValue.get());
    if (modelDescriptionLabel)
    {
        modelDescriptionLabel->setFont(FontManager::getInstance().getLabelFont());
        modelDescriptionLabel->setColour(Label::textColourId, palette.text.withAlpha(0.64f));
    }

    metadataLabel->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
    metadataDisplay->setColour(TextEditor::backgroundColourId, palette.inset);
    metadataDisplay->setColour(TextEditor::textColourId, palette.text.withAlpha(0.84f));
    metadataDisplay->setColour(TextEditor::outlineColourId, palette.edge);

    // Status and empty state
    statusLabel->setColour(Label::textColourId, palette.text.withAlpha(0.6f));
    emptyStateLabel->setColour(Label::textColourId, palette.text.withAlpha(0.4f));
    if (irEmptyStateLabel)
        irEmptyStateLabel->setColour(Label::textColourId, palette.text.withAlpha(0.4f));

    // IR browser buttons
    styleButton(irBrowseFolderButton.get());
    irLoadButton->setColour(TextButton::buttonColourId, palette.accent);
    irLoadButton->setColour(TextButton::buttonOnColourId, palette.accent.brighter(0.2f));
    irLoadButton->setColour(TextButton::textColourOffId, palette.accent);
    irLoadButton->setColour(TextButton::textColourOnId, palette.accent.brighter(0.12f));

    // IR details labels
    irDetailsTitle->setColour(Label::textColourId, palette.text);
    refreshLabelPair(irNameLabel.get(), irNameValue.get());
    refreshLabelPair(irDurationLabel.get(), irDurationValue.get());
    refreshLabelPair(irSampleRateLabel.get(), irSampleRateValue.get());
    refreshLabelPair(irChannelsLabel.get(), irChannelsValue.get());
    refreshLabelPair(irFileSizeLabel.get(), irFileSizeValue.get());
    refreshLabelPair(irFilePathLabel.get(), irFilePathValue.get());

    // Rebuild fonts (FontManager may have changed)
    auto rebuildFonts = [](Label* label, Label* value)
    {
        label->setFont(FontManager::getInstance().getLabelFont());
        value->setFont(FontManager::getInstance().getLabelFont());
    };
    detailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    rebuildFonts(nameLabel.get(), nameValue.get());
    rebuildFonts(authorLabel.get(), authorValue.get());
    rebuildFonts(modelTypeLabel.get(), modelTypeValue.get());
    rebuildFonts(architectureLabel.get(), architectureValue.get());
    rebuildFonts(sampleRateLabel.get(), sampleRateValue.get());
    rebuildFonts(loudnessLabel.get(), loudnessValue.get());
    rebuildFonts(filePathLabel.get(), filePathValue.get());
    metadataLabel->setFont(FontManager::getInstance().getLabelFont());
    metadataDisplay->setFont(FontManager::getInstance().getMonoFont(11.0f));
    statusLabel->setFont(FontManager::getInstance().getCaptionFont());
    emptyStateLabel->setFont(FontManager::getInstance().getBodyFont());
    if (irEmptyStateLabel)
        irEmptyStateLabel->setFont(FontManager::getInstance().getBodyFont());
    searchBox->setFont(FontManager::getInstance().getSubheadingFont());

    irDetailsTitle->setFont(FontManager::getInstance().getSubheadingFont());
    rebuildFonts(irNameLabel.get(), irNameValue.get());
    rebuildFonts(irDurationLabel.get(), irDurationValue.get());
    rebuildFonts(irSampleRateLabel.get(), irSampleRateValue.get());
    rebuildFonts(irChannelsLabel.get(), irChannelsValue.get());
    rebuildFonts(irFileSizeLabel.get(), irFileSizeValue.get());
    rebuildFonts(irFilePathLabel.get(), irFilePathValue.get());

    repaint();
}

void NAMModelBrowserComponent::paint(Graphics& g)
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto palette = makeBrowserPalette();

    // Gradient background
    ColourGradient bgGradient(palette.top, 0, 0, palette.bottom, 0, static_cast<float>(getHeight()), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Subtle dot-grid pattern on background for visual character
    {
        g.setColour(palette.text.withAlpha(0.05f));
        const int gridStep = 16;
        for (int gy = 0; gy < getHeight(); gy += gridStep)
            for (int gx = 0; gx < getWidth(); gx += gridStep)
                g.fillEllipse((float)gx, (float)gy, 2.0f, 2.0f);
    }

    // Mockup-informed faceplate header: title, tabs, and active/offline status live on one raised chassis strip.
    {
        auto headerBounds = titleLabel->getBounds()
                                .getUnion(localTabButton->getBounds())
                                .getUnion(onlineTabButton->getBounds())
                                .getUnion(irTabButton->getBounds())
                                .expanded(10, 8)
                                .toFloat();
        headerBounds.setRight((float)getWidth() - 16.0f);

        Path headerPath;
        headerPath.addRoundedRectangle(headerBounds, 10.0f);
        melatonin::DropShadow shadow(Colours::black.withAlpha(0.24f), 9, {1, 2});
        shadow.render(g, headerPath);

        ColourGradient face(palette.face2, headerBounds.getX(), headerBounds.getY(), palette.face,
                            headerBounds.getX(), headerBounds.getBottom(), false);
        g.setGradientFill(face);
        g.fillPath(headerPath);
        g.setColour(palette.edge);
        g.strokePath(headerPath, PathStrokeType(1.0f));

        g.setColour(palette.accent.withAlpha(0.75f));
        const auto accentRight = jmin(headerBounds.getRight() - 18.0f, headerBounds.getX() + 205.0f);
        g.drawLine(headerBounds.getX() + 14.0f, headerBounds.getBottom() - 7.0f, accentRight,
                   headerBounds.getBottom() - 7.0f, 2.0f);

        if (headerBounds.getWidth() >= 560.0f)
        {
            auto statusPill = Rectangle<float>(headerBounds.getRight() - 116.0f, headerBounds.getCentreY() - 13.0f,
                                               88.0f, 26.0f);
            g.setColour(palette.inset);
            g.fillRoundedRectangle(statusPill, 8.0f);
            g.setColour(palette.edge);
            g.drawRoundedRectangle(statusPill.reduced(0.5f), 8.0f, 1.0f);
            drawStatusLed(g, statusPill.removeFromLeft(24.0f), currentTab == 1 ? palette.led : palette.accent2,
                          currentTab == 1);
            g.setFont(FontManager::getInstance().getBadgeFont());
            g.setColour(currentTab == 1 ? palette.led : palette.text.withAlpha(0.55f));
            g.drawText(currentTab == 1 ? "ONLINE" : "LOCAL", statusPill, Justification::centredLeft, true);
        }

        for (auto corner : {headerBounds.getTopLeft(), headerBounds.getTopRight(), headerBounds.getBottomLeft(),
                            headerBounds.getBottomRight()})
        {
            auto screw = Rectangle<float>(corner.x - (corner.x == headerBounds.getX() ? -6.0f : 10.0f),
                                          corner.y - (corner.y == headerBounds.getY() ? -6.0f : 10.0f), 5.0f,
                                          5.0f);
            g.setColour(Colours::black.withAlpha(0.45f));
            g.fillEllipse(screw);
            g.setColour(palette.edgeHi.withAlpha(0.72f));
            g.drawEllipse(screw, 1.0f);
        }
    }

    // Draw pill-container capsule behind tab buttons
    {
        auto tabBg = localTabButton->getBounds()
                         .getUnion(onlineTabButton->getBounds())
                         .getUnion(irTabButton->getBounds())
                         .toFloat()
                         .expanded(4.0f, 2.0f);
        float cr = tabBg.getHeight() / 2.0f;
        g.setColour(palette.inset.withAlpha(0.88f));
        g.fillRoundedRectangle(tabBg, cr);
        g.setColour(palette.edge);
        g.drawRoundedRectangle(tabBg, cr, 1.0f);
    }

    // Draw panels for Local and IR tabs
    if (currentTab == 0 || currentTab == 2)
    {
        const auto listBounds =
            (currentTab == 0 && modelList) ? modelList->getBounds().toFloat()
                                           : (irList ? irList->getBounds().toFloat() : Rectangle<float>());
        const auto detailsBounds = detailsPanelBounds.toFloat();

        const int total = currentTab == 0 ? listModel.getTotalCount() : irListModel.getTotalCount();
        const int filtered = currentTab == 0 ? listModel.getFilteredCount() : irListModel.getFilteredCount();
        const auto folder = currentTab == 0 ? currentDirectory.getFileName() : irDirectory.getFileName();
        drawLibraryRail(g, libraryRailBounds.toFloat(), currentTab == 0 ? "Type" : "Cabinet",
                        currentTab == 0 ? "Models" : "IRs", "Visible", folder.isNotEmpty() ? folder : "No folder",
                        total, filtered, false);

        // Draw rounded list background with subtle inner shadow
        g.setColour(palette.inset);
        g.fillRoundedRectangle(listBounds, 8.0f);
        g.setColour(Colours::black.withAlpha(0.18f));
        g.drawRoundedRectangle(listBounds.reduced(2.0f), 6.0f, 1.0f);
        g.setColour(palette.edge);
        g.drawRoundedRectangle(listBounds.reduced(0.5f), 8.0f, 1.0f);

        // Draw card-style details panel with shadow
        Path detailsPath;
        detailsPath.addRoundedRectangle(detailsBounds, 8.0f);

        // Drop shadow
        melatonin::DropShadow shadow(Colours::black.withAlpha(0.25f), 10, {2, 3});
        shadow.render(g, detailsPath);

        // Card fill with subtle gradient
        ColourGradient cardGrad(palette.face2, detailsBounds.getX(), detailsBounds.getY(), palette.face,
                                detailsBounds.getX(), detailsBounds.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillPath(detailsPath);

        // Card border with glow effect
        g.setColour((currentTab == 0 ? palette.accent : palette.accent2).withAlpha(0.14f));
        g.strokePath(detailsPath, PathStrokeType(2.0f));
        g.setColour(palette.edge);
        g.strokePath(detailsPath, PathStrokeType(1.0f));

        if (detailsBounds.getWidth() >= 250.0f)
        {
            if (currentTab == 0)
            {
                const auto* selectedModel = modelList ? listModel.getModelAt(modelList->getSelectedRow()) : nullptr;
                const bool selectedReady = selectedModel != nullptr && isReadableNAMModel(*selectedModel);
                const auto fields = selectedModel != nullptr ? extractNAMPreviewFields(*selectedModel) : NAMPreviewFields{};

                auto previewCard = nameValue->getBounds()
                                       .getUnion(authorLabel->getBounds())
                                       .getUnion(authorValue->getBounds())
                                       .getUnion(modelDescriptionLabel->getBounds())
                                       .toFloat()
                                       .expanded(10.0f, 5.0f);
                previewCard.setLeft(detailsBounds.getX() + 14.0f);
                previewCard.setRight(detailsBounds.getRight() - 14.0f);
                previewCard.setBottom(previewCard.getBottom() + 24.0f);

                g.setColour(selectedModel ? palette.accent.withAlpha(selectedReady ? 0.13f : 0.08f)
                                          : palette.text.withAlpha(0.045f));
                g.fillRoundedRectangle(previewCard, 9.0f);
                g.setColour(selectedModel ? (selectedReady ? palette.accent : colours["Warning Colour"]).withAlpha(0.5f)
                                          : palette.edge.withAlpha(0.42f));
                g.drawRoundedRectangle(previewCard.reduced(0.5f), 9.0f, 1.0f);

                const bool showPreviewGlyph = detailsBounds.getWidth() >= 330.0f;
                if (showPreviewGlyph)
                {
                    auto glyph =
                        Rectangle<float>(previewCard.getRight() - 60.0f, previewCard.getY() + 12.0f, 44.0f, 44.0f);
                    const auto archColour = selectedModel ? colourForNAMArchitecture(String(selectedModel->architecture), palette)
                                                          : palette.accent2;
                    drawModelGlyph(g, glyph, selectedReady ? archColour : colours["Warning Colour"],
                                   selectedModel != nullptr);
                }

                auto chipRow = previewCard.reduced(10.0f, 0.0f).removeFromBottom(24.0f);
                const auto stateChipWidth = selectedModel && !selectedReady ? 70.0f : 58.0f;
                if (chipRow.getWidth() >= stateChipWidth)
                    drawBrowserChip(g, chipRow.removeFromLeft(stateChipWidth),
                                    selectedModel ? (selectedReady ? "READY" : "MISSING") : "EMPTY",
                                    selectedModel ? (selectedReady ? palette.led : colours["Warning Colour"])
                                                  : palette.text.withAlpha(0.45f),
                                    selectedModel != nullptr);

                if (selectedModel != nullptr)
                {
                    if (chipRow.getWidth() > 56.0f)
                    {
                        chipRow.removeFromLeft(6.0f);
                        const auto typeChip = normaliseNAMModelType(fields.modelType);
                        const auto typeChipWidth = jmin(chipRow.getWidth(),
                                                        jlimit(48.0f, 82.0f,
                                                               FontManager::getInstance().getBadgeFont().getStringWidthFloat(typeChip) +
                                                                   16.0f));
                        drawBrowserChip(g, chipRow.removeFromLeft(typeChipWidth), typeChip,
                                        colourForNAMModelType(fields.modelType, palette), true);
                    }

                    const auto toneTag = inferToneTag(String(selectedModel->name) + " " + fields.rig + " " + fields.modelType);
                    if (toneTag.isNotEmpty() && chipRow.getWidth() > 48.0f)
                    {
                        chipRow.removeFromLeft(6.0f);
                        const auto toneChipWidth = jmin(chipRow.getWidth(),
                                                        FontManager::getInstance().getBadgeFont().getStringWidthFloat(toneTag) + 18.0f);
                        drawBrowserChip(g, chipRow.removeFromLeft(toneChipWidth), toneTag, toneColourForTag(toneTag),
                                        true);
                    }
                }

            }
            else
            {
                auto glyph =
                    Rectangle<float>(detailsBounds.getRight() - 82.0f, detailsBounds.getY() + 20.0f, 58.0f, 58.0f);
                drawIRGlyph(g, glyph, palette.accent2, irList && irList->getSelectedRow() >= 0);

                g.setColour(palette.accent2.withAlpha(0.7f));
                g.fillRoundedRectangle(detailsBounds.withTrimmedTop(96.0f).withHeight(2.0f).reduced(16.0f, 0.0f),
                                       1.0f);
            }
        }

        auto drawDetailRowBacking = [&](Label* label, Component* value, int rowIndex)
        {
            if (label == nullptr || value == nullptr || !label->isVisible() || !value->isVisible())
                return;

            auto row = label->getBounds().getUnion(value->getBounds()).expanded(7, 3).toFloat();
            row.setRight(detailsBounds.getRight() - (detailsBounds.getWidth() >= 250.0f && row.getY() < detailsBounds.getY() + 124.0f ? 96.0f : 16.0f));
            row.setLeft(jmax(detailsBounds.getX() + 8.0f, (float)label->getX() - 6.0f));

            g.setColour((rowIndex % 2 == 0 ? palette.inset : palette.face).withAlpha(0.72f));
            g.fillRoundedRectangle(row, 6.0f);
            g.setColour(palette.edge.withAlpha(0.45f));
            g.drawRoundedRectangle(row.reduced(0.5f), 6.0f, 1.0f);
        };

        if (currentTab == 0)
        {
            drawDetailRowBacking(modelTypeLabel.get(), modelTypeValue.get(), 2);
            drawDetailRowBacking(architectureLabel.get(), architectureValue.get(), 3);
            drawDetailRowBacking(sampleRateLabel.get(), sampleRateValue.get(), 4);
            drawDetailRowBacking(loudnessLabel.get(), loudnessValue.get(), 5);
            drawDetailRowBacking(filePathLabel.get(), filePathValue.get(), 6);
        }
        else
        {
            drawDetailRowBacking(irNameLabel.get(), irNameValue.get(), 0);
            drawDetailRowBacking(irDurationLabel.get(), irDurationValue.get(), 1);
            drawDetailRowBacking(irSampleRateLabel.get(), irSampleRateValue.get(), 2);
            drawDetailRowBacking(irChannelsLabel.get(), irChannelsValue.get(), 3);
            drawDetailRowBacking(irFileSizeLabel.get(), irFileSizeValue.get(), 4);
            drawDetailRowBacking(irFilePathLabel.get(), irFilePathValue.get(), 5);
        }
    }

    // Draw section separators in details panel
    if ((currentTab == 0 || currentTab == 2) && !detailsSeparatorPositions.empty())
    {
        auto detailsX = nameLabel ? nameLabel->getX() : 0;
        auto detailsRight = nameValue ? nameValue->getRight() : getWidth();

        g.setColour(palette.edge.withAlpha(0.5f));
        for (auto y : detailsSeparatorPositions)
        {
            float yf = static_cast<float>(y) + 4.0f;
            g.drawLine(static_cast<float>(detailsX), yf, static_cast<float>(detailsRight), yf, 1.0f);
        }
    }

    // Draw empty state icon (magnifying glass) when no models found
    if (currentTab == 0 && emptyStateLabel && emptyStateLabel->isVisible())
    {
        auto emptyBounds = emptyStateLabel->getBounds();
        float cx = emptyBounds.getCentreX();
        float iconTop = emptyBounds.getY() - 40.0f;

        // Large magnifying glass
        float r = 14.0f;
        g.setColour(palette.text.withAlpha(0.15f));
        g.drawEllipse(cx - r, iconTop, r * 2.0f, r * 2.0f, 2.0f);
        float hx = cx + r * 0.7f;
        float hy = iconTop + r * 2.0f - r * 0.3f;
        g.drawLine(hx, hy, hx + r * 0.8f, hy + r * 0.8f, 2.0f);
    }
    else if (currentTab == 2 && irEmptyStateLabel && irEmptyStateLabel->isVisible())
    {
        auto emptyBounds = irEmptyStateLabel->getBounds().toFloat();
        drawIRGlyph(g, emptyBounds.withSizeKeepingCentre(44.0f, 44.0f).translated(0.0f, -52.0f),
                    palette.accent2.withAlpha(0.8f), false);
    }
}

void NAMModelBrowserComponent::paintOverChildren(Graphics& g)
{
    // Draw magnifying glass icon centered in the search pill
    if (currentTab == 0 || currentTab == 2)
    {
        const auto palette = makeBrowserPalette();
        auto searchBounds = searchBox->getBounds().toFloat();

        float iconSize = 13.0f;
        float radius = iconSize * 0.35f;
        float iconX = searchBounds.getX() + 10.0f;
        float iconCentreY = searchBounds.getCentreY();

        g.setColour(palette.text.withAlpha(0.45f));
        // Circle part — centered vertically
        g.drawEllipse(iconX, iconCentreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);
        // Handle
        float handleStartX = iconX + radius + radius * 0.7f;
        float handleStartY = iconCentreY + radius * 0.7f;
        g.drawLine(handleStartX, handleStartY, handleStartX + radius * 0.8f, handleStartY + radius * 0.8f, 1.5f);
    }
}

void NAMModelBrowserComponent::resized()
{
    const bool compactLayout = getWidth() < 780 || getHeight() < 620;
    const int outerPadding = compactLayout ? 10 : 16;
    auto bounds = getLocalBounds().reduced(outerPadding);
    libraryRailBounds = {};
    detailsPanelBounds = {};

    auto& fonts = FontManager::getInstance();
    titleLabel->setFont(compactLayout ? fonts.getSubheadingFont() : fonts.getHeadingFont());
    searchBox->setFont(compactLayout ? fonts.getBodyBoldFont() : fonts.getSubheadingFont());
    detailsTitle->setFont(compactLayout ? fonts.getBodyBoldFont() : fonts.getSubheadingFont());
    nameValue->setFont(compactLayout ? fonts.getBodyBoldFont() : fonts.getSubheadingFont());
    modelList->setRowHeight(compactLayout ? 46 : 58);

    // Title row with tab buttons
    auto titleRow = bounds.removeFromTop(compactLayout ? 28 : 34);
    const int titleGap = compactLayout ? 8 : 16;
    const int localTabWidth = compactLayout ? 58 : 70;
    const int onlineTabWidth = compactLayout ? 64 : 70;
    const int irTabWidth = compactLayout ? 44 : 55;
    const int tabStripWidth = localTabWidth + 2 + onlineTabWidth + 2 + irTabWidth;
    const int reservedTabWidth = tabStripWidth + titleGap;
    const int availableTitleWidth = titleRow.getWidth() - reservedTabWidth;
    const int titleWidth = availableTitleWidth >= (compactLayout ? 96 : 150)
                               ? jlimit(compactLayout ? 96 : 150, compactLayout ? 190 : 220, availableTitleWidth)
                               : 0;
    titleLabel->setBounds(titleRow.removeFromLeft(titleWidth));
    titleLabel->setVisible(titleWidth > 0);

    // The tabs are primary navigation and must remain visible at high app scale.
    if (titleWidth > 0)
        titleRow.removeFromLeft(titleGap);
    localTabButton->setBounds(titleRow.removeFromLeft(localTabWidth));
    titleRow.removeFromLeft(2);
    onlineTabButton->setBounds(titleRow.removeFromLeft(onlineTabWidth));
    titleRow.removeFromLeft(2);
    irTabButton->setBounds(titleRow.removeFromLeft(irTabWidth));

    bounds.removeFromTop(compactLayout ? 6 : 8);

    // Hide all IR components by default
    auto hideIRComponents = [this]()
    {
        irList->setVisible(false);
        if (irEmptyStateLabel)
            irEmptyStateLabel->setVisible(false);
        irBrowseFolderButton->setVisible(false);
        irLoadButton->setVisible(false);
        irDetailsTitle->setVisible(false);
        irNameLabel->setVisible(false);
        irNameValue->setVisible(false);
        irDurationLabel->setVisible(false);
        irDurationValue->setVisible(false);
        irSampleRateLabel->setVisible(false);
        irSampleRateValue->setVisible(false);
        irChannelsLabel->setVisible(false);
        irChannelsValue->setVisible(false);
        irFileSizeLabel->setVisible(false);
        irFileSizeValue->setVisible(false);
        irFilePathLabel->setVisible(false);
        irFilePathValue->setVisible(false);
    };

    // Hide all local NAM components by default
    auto hideLocalComponents = [this]()
    {
        searchBox->setVisible(false);
        refreshButton->setVisible(false);
        browseFolderButton->setVisible(false);
        loadButton->setVisible(false);
        modelList->setVisible(false);
        detailsTitle->setVisible(false);
        nameLabel->setVisible(false);
        nameValue->setVisible(false);
        authorLabel->setVisible(false);
        authorValue->setVisible(false);
        modelTypeLabel->setVisible(false);
        modelTypeValue->setVisible(false);
        architectureLabel->setVisible(false);
        architectureValue->setVisible(false);
        sampleRateLabel->setVisible(false);
        sampleRateValue->setVisible(false);
        loudnessLabel->setVisible(false);
        loudnessValue->setVisible(false);
        modelDescriptionLabel->setVisible(false);
        metadataLabel->setVisible(false);
        metadataDisplay->setVisible(false);
        filePathLabel->setVisible(false);
        filePathValue->setVisible(false);
        deleteButton->setVisible(false);
        statusLabel->setVisible(false);
        emptyStateLabel->setVisible(false);
    };

    // Online browser takes the full remaining area (minus close button row)
    if (currentTab == 1)
    {
        // Button row at bottom for close button only
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(8);
        closeButton->setBounds(buttonRow.removeFromRight(70));

        onlineBrowser->setBounds(bounds);

        // Hide local and IR browser elements
        hideLocalComponents();
        hideIRComponents();
        return;
    }

    // IR browser tab
    if (currentTab == 2)
    {
        hideLocalComponents();
        onlineBrowser->setVisible(false);

        // Search row with IR-specific browse button
        auto searchRow = bounds.removeFromTop(32);
        refreshButton->setBounds(searchRow.removeFromRight(70));
        refreshButton->setVisible(true);
        searchRow.removeFromRight(8);
        irBrowseFolderButton->setBounds(searchRow.removeFromRight(120));
        irBrowseFolderButton->setVisible(true);
        searchRow.removeFromRight(8);
        searchBox->setBounds(searchRow);
        searchBox->setVisible(true);
        bounds.removeFromTop(8);

        // Status bar at bottom
        auto statusRow = bounds.removeFromBottom(20);
        statusLabel->setBounds(statusRow);
        statusLabel->setVisible(true);
        bounds.removeFromBottom(4);

        // Button row at bottom
        auto buttonRow = bounds.removeFromBottom(36);
        bounds.removeFromBottom(8);
        closeButton->setBounds(buttonRow.removeFromRight(70));
        buttonRow.removeFromRight(8);
        irLoadButton->setBounds(buttonRow.removeFromRight(80));
        irLoadButton->setVisible(true);

        // Split remaining area: library rail, list, and focused inspector card.
        const bool showRail = bounds.getWidth() >= 720;
        if (showRail)
        {
            const int railWidth = jlimit(132, 168, bounds.getWidth() / 7);
            libraryRailBounds = bounds.removeFromLeft(railWidth);
            bounds.removeFromLeft(12);
        }

        const int detailsWidth = jlimit(240, 330, juce::roundToInt(bounds.getWidth() * 0.33f));
        auto detailsArea = bounds.removeFromRight(detailsWidth);
        detailsPanelBounds = detailsArea;
        bounds.removeFromRight(12);
        auto listArea = bounds;

        // IR list
        irList->setBounds(listArea);
        if (irEmptyStateLabel)
            irEmptyStateLabel->setBounds(listArea);
        updateIRBrowserState();

        // IR details panel
        detailsArea.reduce(14, 12);

        irDetailsTitle->setBounds(detailsArea.removeFromTop(22));
        irDetailsTitle->setVisible(true);
        detailsArea.removeFromTop(82);

        auto layoutIRLabelValue = [&detailsArea](Label* label, Label* value)
        {
            auto row = detailsArea.removeFromTop(22);
            label->setBounds(row.removeFromLeft(90));
            label->setVisible(true);
            value->setBounds(row);
            value->setVisible(true);
            detailsArea.removeFromTop(4);
        };

        layoutIRLabelValue(irNameLabel.get(), irNameValue.get());
        layoutIRLabelValue(irDurationLabel.get(), irDurationValue.get());
        layoutIRLabelValue(irSampleRateLabel.get(), irSampleRateValue.get());
        layoutIRLabelValue(irChannelsLabel.get(), irChannelsValue.get());
        layoutIRLabelValue(irFileSizeLabel.get(), irFileSizeValue.get());

        detailsArea.removeFromTop(8);

        // File path row
        auto fileRow = detailsArea.removeFromTop(22);
        irFilePathLabel->setBounds(fileRow.removeFromLeft(40));
        irFilePathLabel->setVisible(true);
        irFilePathValue->setBounds(fileRow);
        irFilePathValue->setVisible(true);

        return;
    }

    // Local NAM tab - hide IR components and online browser
    hideIRComponents();
    onlineBrowser->setVisible(false);

    // Show local browser elements
    searchBox->setVisible(true);
    refreshButton->setVisible(true);
    browseFolderButton->setVisible(true);
    loadButton->setVisible(true);
    detailsTitle->setVisible(true);
    nameLabel->setVisible(true);
    nameValue->setVisible(true);
    authorLabel->setVisible(true);
    authorValue->setVisible(true);
    modelTypeLabel->setVisible(true);
    modelTypeValue->setVisible(true);
    architectureLabel->setVisible(true);
    architectureValue->setVisible(true);
    sampleRateLabel->setVisible(true);
    sampleRateValue->setVisible(true);
    loudnessLabel->setVisible(true);
    loudnessValue->setVisible(true);
    modelDescriptionLabel->setVisible(true);
    metadataLabel->setVisible(true);
    metadataDisplay->setVisible(true);
    filePathLabel->setVisible(true);
    filePathValue->setVisible(true);
    deleteButton->setVisible(true);
    statusLabel->setVisible(true);

    // Show list or empty state based on model count
    updateLocalBrowserState();

    // Search and refresh row
    const int rowGap = compactLayout ? 6 : 8;
    auto searchRow = bounds.removeFromTop(compactLayout ? 30 : 32);
    refreshButton->setBounds(searchRow.removeFromRight(compactLayout ? 62 : 70));
    searchRow.removeFromRight(rowGap);
    browseFolderButton->setBounds(searchRow.removeFromRight(compactLayout ? 96 : 110));
    searchRow.removeFromRight(rowGap);
    searchBox->setBounds(searchRow);
    bounds.removeFromTop(compactLayout ? 6 : 8);

    // Status bar at bottom
    auto statusRow = bounds.removeFromBottom(compactLayout ? 18 : 20);
    statusLabel->setBounds(statusRow);
    bounds.removeFromBottom(compactLayout ? 3 : 4);

    // Button row at bottom
    auto buttonRow = bounds.removeFromBottom(compactLayout ? 34 : 36);
    bounds.removeFromBottom(compactLayout ? 6 : 8);

    closeButton->setBounds(buttonRow.removeFromRight(compactLayout ? 64 : 70));
    buttonRow.removeFromRight(rowGap);
    deleteButton->setBounds(buttonRow.removeFromRight(compactLayout ? 92 : 100));
    buttonRow.removeFromRight(rowGap);
    loadButton->setBounds(buttonRow.removeFromRight(compactLayout ? 92 : 100));

    // Split remaining area: library rail, list, and focused inspector card.
    const bool showRail = !compactLayout && bounds.getWidth() >= 720;
    if (showRail)
    {
        const int railWidth = jlimit(132, 168, bounds.getWidth() / 7);
        libraryRailBounds = bounds.removeFromLeft(railWidth);
        bounds.removeFromLeft(12);
    }

    const int splitGap = compactLayout ? 8 : 12;
    const int minimumListWidth = compactLayout ? 150 : 220;
    const int desiredDetailsWidth = compactLayout ? jlimit(170, 230, juce::roundToInt(bounds.getWidth() * 0.44f))
                                                  : jlimit(250, 350, juce::roundToInt(bounds.getWidth() * 0.34f));
    const int maxDetailsWidth = jmax(120, bounds.getWidth() - splitGap - minimumListWidth);
    const int detailsWidth = jmin(desiredDetailsWidth, maxDetailsWidth);
    auto detailsArea = bounds.removeFromRight(detailsWidth);
    detailsPanelBounds = detailsArea;
    bounds.removeFromRight(splitGap);
    auto listArea = bounds;

    // Model list or empty state
    modelList->setBounds(listArea);
    emptyStateLabel->setBounds(listArea);

    // Details panel with section grouping
    detailsArea.reduce(compactLayout ? 10 : 14, compactLayout ? 9 : 12);
    const int labelWidth = compactLayout ? 72 : 90;
    const int sectionGap = compactLayout ? 6 : 10;
    const int rowH = compactLayout ? 19 : 22;
    const int detailRowGap = compactLayout ? 2 : 4;
    const bool showFullTechnicalDetails = !compactLayout;

    sampleRateLabel->setVisible(showFullTechnicalDetails);
    sampleRateValue->setVisible(showFullTechnicalDetails);
    loudnessLabel->setVisible(showFullTechnicalDetails);
    loudnessValue->setVisible(showFullTechnicalDetails);
    metadataLabel->setVisible(showFullTechnicalDetails);
    metadataDisplay->setVisible(showFullTechnicalDetails);

    detailsTitle->setBounds(detailsArea.removeFromTop(compactLayout ? 20 : 22));
    detailsArea.removeFromTop(compactLayout ? 5 : 8);

    // Store separator positions for paint()
    detailsSeparatorPositions.clear();

    auto layoutLabelValue = [&detailsArea, labelWidth, rowH, detailRowGap](Label* label, Label* value)
    {
        auto row = detailsArea.removeFromTop(rowH);
        label->setBounds(row.removeFromLeft(labelWidth));
        value->setBounds(row);
        detailsArea.removeFromTop(detailRowGap);
    };

    // -- Identity / preview section --
    auto heroArea = detailsArea.removeFromTop(compactLayout ? 84 : 118);
    auto heroText = heroArea;
    if (!compactLayout && detailsPanelBounds.getWidth() >= 330)
        heroText.removeFromRight(78);
    nameLabel->setVisible(false);
    nameLabel->setBounds({});
    nameValue->setBounds(heroText.removeFromTop(compactLayout ? 24 : 34));
    auto authorRow = heroText.removeFromTop(compactLayout ? 19 : 22);
    authorLabel->setBounds(authorRow.removeFromLeft(labelWidth));
    authorValue->setBounds(authorRow);
    heroText.removeFromTop(compactLayout ? 2 : 4);
    modelDescriptionLabel->setBounds(heroText.removeFromTop(compactLayout ? 31 : 34));
    detailsArea.removeFromTop(compactLayout ? 4 : 8);

    // Separator after Identity
    detailsSeparatorPositions.push_back(detailsArea.getY());
    detailsArea.removeFromTop(sectionGap);

    // -- Technical section --
    layoutLabelValue(modelTypeLabel.get(), modelTypeValue.get());
    layoutLabelValue(architectureLabel.get(), architectureValue.get());
    if (showFullTechnicalDetails)
    {
        layoutLabelValue(sampleRateLabel.get(), sampleRateValue.get());
        layoutLabelValue(loudnessLabel.get(), loudnessValue.get());
    }

    // Separator after Technical
    detailsSeparatorPositions.push_back(detailsArea.getY());
    detailsArea.removeFromTop(sectionGap);

    // -- File section --
    auto fileRow = detailsArea.removeFromTop(rowH);
    filePathLabel->setBounds(fileRow.removeFromLeft(40));
    filePathValue->setBounds(fileRow);
    detailsArea.removeFromTop(detailRowGap);

    if (showFullTechnicalDetails)
    {
        metadataLabel->setBounds(detailsArea.removeFromTop(20));
        detailsArea.removeFromTop(4);
        metadataDisplay->setBounds(detailsArea);
    }
}

void NAMModelBrowserComponent::buttonClicked(Button* button)
{
    if (button == localTabButton.get())
    {
        switchToTab(0);
    }
    else if (button == onlineTabButton.get())
    {
        switchToTab(1);
    }
    else if (button == irTabButton.get())
    {
        switchToTab(2);
    }
    else if (button == refreshButton.get())
    {
        if (currentTab == 0)
            scanDirectory(currentDirectory);
        else if (currentTab == 2)
            scanIRDirectory(irDirectory);
    }
    else if (button == browseFolderButton.get())
    {
        folderChooser = std::make_unique<FileChooser>("Select NAM Models Folder", currentDirectory, "", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        folderChooser->launchAsync(chooserFlags,
                                   [this](const FileChooser& fc)
                                   {
                                       auto result = fc.getResult();
                                       if (result.isDirectory())
                                       {
                                           currentDirectory = result;
                                           scanDirectory(currentDirectory);
                                       }
                                   });
    }
    else if (button == irBrowseFolderButton.get())
    {
        irFolderChooser = std::make_unique<FileChooser>("Select IR Folder", irDirectory, "", true);

        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        irFolderChooser->launchAsync(chooserFlags,
                                     [this](const FileChooser& fc)
                                     {
                                         auto result = fc.getResult();
                                         if (result.isDirectory())
                                         {
                                             irDirectory = result;
                                             scanIRDirectory(irDirectory);
                                         }
                                     });
    }
    else if (button == loadButton.get())
    {
        loadSelectedModel();
    }
    else if (button == irLoadButton.get())
    {
        loadSelectedIR();
    }
    else if (button == closeButton.get())
    {
        if (auto* window = findParentComponentOfClass<NAMModelBrowser>())
            window->closeButtonPressed();
    }
    else if (button == deleteButton.get())
    {
        deleteSelectedModel();
    }
}

void NAMModelBrowserComponent::deleteSelectedModel()
{
    int selectedRow = modelList->getSelectedRow();
    const auto* model = listModel.getModelAt(selectedRow);

    if (model == nullptr)
        return;

    File modelFile(model->filePath);
    if (!modelFile.existsAsFile())
        return;

    // Confirm deletion
    auto options = MessageBoxOptions::makeOptionsOk(MessageBoxIconType::QuestionIcon, "Delete Model?",
                                                    "Are you sure you want to delete \"" + String(model->name) +
                                                        "\"?\n\nThis cannot be undone.");

    AlertWindow::showAsync(options,
                           [this, modelFile](int result)
                           {
                               if (result == 1)
                               {
                                   // Delete the file
                                   if (modelFile.deleteFile())
                                   {
                                       spdlog::info("[NAMModelBrowser] Deleted model: {}",
                                                    modelFile.getFullPathName().toStdString());

                                       // Also delete parent folder if it's empty (TONE3000 creates a folder per
                                       // model)
                                       File parentDir = modelFile.getParentDirectory();
                                       if (parentDir != currentDirectory)
                                       {
                                           auto remainingFiles = parentDir.findChildFiles(File::findFiles, false);
                                           if (remainingFiles.isEmpty())
                                           {
                                               parentDir.deleteRecursively();
                                               spdlog::info("[NAMModelBrowser] Deleted empty folder: {}",
                                                            parentDir.getFullPathName().toStdString());
                                           }
                                       }

                                       // Refresh the list
                                       scanDirectory(currentDirectory);
                                   }
                                   else
                                   {
                                       spdlog::error("[NAMModelBrowser] Failed to delete model: {}",
                                                     modelFile.getFullPathName().toStdString());
                                   }
                               }
                           });
}

void NAMModelBrowserComponent::switchToTab(int tabIndex)
{
    if (currentTab == tabIndex)
        return;

    currentTab = tabIndex;

    // Update tab button states
    localTabButton->setToggleState(tabIndex == 0, dontSendNotification);
    onlineTabButton->setToggleState(tabIndex == 1, dontSendNotification);
    irTabButton->setToggleState(tabIndex == 2, dontSendNotification);

    // Show/hide appropriate content
    onlineBrowser->setVisible(tabIndex == 1);

    // Refresh content when switching tabs (each uses its own directory)
    if (tabIndex == 0)
        scanDirectory(currentDirectory);
    else if (tabIndex == 2)
        scanIRDirectory(irDirectory);

    // Trigger layout update
    resized();
    repaint();

    const char* tabNames[] = {"Local", "Online", "IRs"};
    spdlog::info("[NAMModelBrowser] Switched to {} tab", tabNames[tabIndex]);
}

void NAMModelBrowserComponent::textEditorTextChanged(TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        if (currentTab == 0)
        {
            listModel.setFilter(searchBox->getText());
            modelList->updateContent();
            modelList->repaint();
            syncLocalSelectionAfterListChange();
        }
        else if (currentTab == 2)
        {
            irListModel.setFilter(searchBox->getText());
            irList->updateContent();
            irList->repaint();
            syncIRSelectionAfterListChange();
        }
    }
}

void NAMModelBrowserComponent::scanDirectory(const File& directory)
{
    models.clear();

    if (!directory.isDirectory())
    {
        listModel.setModels(models);
        modelList->updateContent();
        syncLocalSelectionAfterListChange();
        return;
    }

    // Show scanning indicator
    isScanning = true;
    statusLabel->setText("Scanning for NAM models...", dontSendNotification);
    statusLabel->repaint();

    spdlog::info("[NAMModelBrowser] Scanning directory: {}", directory.getFullPathName().toStdString());

    // Find all .nam files recursively
    auto namFiles = directory.findChildFiles(File::findFiles, true, "*.nam");

    for (const auto& file : namFiles)
    {
        NAMModelInfo info;
        if (NAMCore::getModelInfo(file.getFullPathName().toStdString(), info))
        {
            models.push_back(std::move(info));
        }
    }

    isScanning = false;
    spdlog::info("[NAMModelBrowser] Found {} NAM models", models.size());

    // Sort by name
    std::sort(models.begin(), models.end(),
              [](const NAMModelInfo& a, const NAMModelInfo& b) { return a.name < b.name; });

    listModel.setModels(models);
    modelList->updateContent();
    modelList->repaint();

    syncLocalSelectionAfterListChange();
}

void NAMModelBrowserComponent::refreshModelList()
{
    scanDirectory(currentDirectory);
}

void NAMModelBrowserComponent::updateDetailsPanel(const NAMModelInfo* model)
{
    if (model)
    {
        const auto fields = extractNAMPreviewFields(*model);
        nameValue->setText(String(model->name), dontSendNotification);
        architectureValue->setText(String(model->architecture), dontSendNotification);
        modelDescriptionLabel->setText(makeNAMDetailDescription(*model, fields), dontSendNotification);

        sampleRateValue->setText(formatNAMSampleRate(model->expectedSampleRate), dontSendNotification);
        loudnessValue->setText(formatNAMLoudness(*model), dontSendNotification);

        // Show file path (just the filename, with tooltip for full path)
        File modelFile(model->filePath);
        filePathValue->setText(modelFile.getFileName(), dontSendNotification);
        filePathValue->setTooltip(String(model->filePath));

        // Extract author and model type from metadata, format remaining metadata
        String formattedMetadata;

        if (!model->metadata.empty())
        {
            try
            {
                auto meta = nlohmann::json::parse(model->metadata);

                // Extract and format remaining fields
                auto addField = [&](const char* label, const char* key)
                {
                    if (meta.contains(key) && !meta[key].is_null())
                    {
                        String value;
                        if (meta[key].is_string())
                            value = String(meta[key].get<std::string>());
                        else if (meta[key].is_number())
                            value = String(meta[key].get<double>());
                        else if (meta[key].is_boolean())
                            value = meta[key].get<bool>() ? "Yes" : "No";

                        if (value.isNotEmpty())
                            formattedMetadata += String(label) + ": " + value + "\n";
                    }
                };

                // Common NAM metadata fields (skip author/type since shown separately)
                addField("Name", "name");
                addField("Date", "date");
                addField("Gear", "gear");
                addField("Amp", "amp");
                addField("Cab", "cab");
                addField("Mic", "mic");
                addField("Description", "description");
                addField("Notes", "notes");
                addField("License", "license");
                addField("Version", "version");

                // Handle nested gear object if present
                if (meta.contains("gear") && meta["gear"].is_object())
                {
                    const auto& gear = meta["gear"];
                    if (gear.contains("amp") && !gear["amp"].is_null())
                        formattedMetadata += "Amp: " + String(gear["amp"].get<std::string>()) + "\n";
                    if (gear.contains("cabinet") && !gear["cabinet"].is_null())
                        formattedMetadata += "Cabinet: " + String(gear["cabinet"].get<std::string>()) + "\n";
                    if (gear.contains("mic") && !gear["mic"].is_null())
                        formattedMetadata += "Mic: " + String(gear["mic"].get<std::string>()) + "\n";
                }
            }
            catch (const std::exception&)
            {
                // Fall back to raw metadata if parsing fails
                formattedMetadata = String(model->metadata);
            }
        }

        authorValue->setText(fields.author, dontSendNotification);
        modelTypeValue->setText(fields.modelType, dontSendNotification);

        metadataDisplay->setText(formattedMetadata.trimEnd(), dontSendNotification);
        statusLabel->setText("Selected " + String(model->name) + " - " + makeNAMPreviewSummary(*model, fields) +
                                 " - double-click or Load Model to use it.",
                             dontSendNotification);
        if (loadButton)
            loadButton->setEnabled(isReadableNAMModel(*model));
        if (deleteButton)
            deleteButton->setEnabled(isReadableNAMModel(*model));
    }
    else
    {
        nameValue->setText("-", dontSendNotification);
        authorValue->setText("-", dontSendNotification);
        modelTypeValue->setText("-", dontSendNotification);
        architectureValue->setText("-", dontSendNotification);
        sampleRateValue->setText("-", dontSendNotification);
        loudnessValue->setText("-", dontSendNotification);
        filePathValue->setText("-", dontSendNotification);
        filePathValue->setTooltip("");
        modelDescriptionLabel->setText("Select a model to preview its capture notes.", dontSendNotification);
        metadataDisplay->setText("", dontSendNotification);
        if (loadButton)
            loadButton->setEnabled(false);
        if (deleteButton)
            deleteButton->setEnabled(false);
        updateLocalBrowserState();
    }
}

void NAMModelBrowserComponent::updateLocalBrowserState()
{
    if (!statusLabel || !emptyStateLabel || !modelList)
        return;

    const auto query = searchBox ? searchBox->getText() : String();
    const int total = listModel.getTotalCount();
    const int filtered = listModel.getFilteredCount();
    const bool hasFilteredRows = filtered > 0;

    statusLabel->setText(describeBrowserCount(currentDirectory, String(), total, filtered, "model", "models", query),
                         dontSendNotification);
    emptyStateLabel->setText(makeEmptyStateCopy("No NAM models found",
                                                "Use Browse Folder to select a folder or switch to Online.", query),
                             dontSendNotification);
    modelList->setVisible(hasFilteredRows);
    emptyStateLabel->setVisible(!hasFilteredRows);
    const int selectedRow = modelList->getSelectedRow();
    const auto* selectedModel = selectedRow >= 0 ? listModel.getModelAt(selectedRow) : nullptr;
    const bool canUseSelectedModel = hasFilteredRows && selectedModel != nullptr && isReadableNAMModel(*selectedModel);
    loadButton->setEnabled(canUseSelectedModel);
    deleteButton->setEnabled(canUseSelectedModel);
    repaint();
}

void NAMModelBrowserComponent::syncLocalSelectionAfterListChange()
{
    updateLocalBrowserState();

    if (!modelList)
        return;

    const int filtered = listModel.getFilteredCount();
    if (filtered <= 0)
    {
        modelList->deselectAllRows();
        updateDetailsPanel(nullptr);
        return;
    }

    int row = modelList->getSelectedRow();
    if (row < 0 || row >= filtered)
        row = 0;

    modelList->selectRow(row, dontSendNotification);
    updateDetailsPanel(listModel.getModelAt(row));
    modelList->repaint();
}

void NAMModelBrowserComponent::loadSelectedModel()
{
    auto selectedRow = modelList->getSelectedRow();
    if (selectedRow >= 0)
    {
        const auto* model = listModel.getModelAt(selectedRow);
        if (model && namProcessor)
        {
            File modelFile(model->filePath);
            if (!modelFile.existsAsFile())
            {
                updateLocalBrowserState();
                statusLabel->setText("Selected model file is missing. Refresh or choose another folder.",
                                     dontSendNotification);
                return;
            }

            if (namProcessor->loadModel(modelFile))
            {
                spdlog::info("[NAMModelBrowser] Loaded model: {}", model->name);

                if (onModelLoadedCallback)
                    onModelLoadedCallback();
            }
            else
            {
                spdlog::error("[NAMModelBrowser] Failed to load model: {}", model->name);
            }
        }
    }
}

void NAMModelBrowserComponent::onListSelectionChanged()
{
    auto selectedRow = modelList->getSelectedRow();
    const auto* model = listModel.getModelAt(selectedRow);
    updateDetailsPanel(model);
}

void NAMModelBrowserComponent::mouseUp(const MouseEvent& event)
{
    // Handle clicks on the model list to update selection
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() { onListSelectionChanged(); });
    }
    // Handle clicks on the IR list
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() { onIRListSelectionChanged(); });
    }
}

void NAMModelBrowserComponent::mouseDoubleClick(const MouseEvent& event)
{
    // Double-click on list item loads the model
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() { loadSelectedModel(); });
    }
    // Double-click on IR list item loads the IR
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        juce::MessageManager::callAsync([this]() { loadSelectedIR(); });
    }
}

void NAMModelBrowserComponent::mouseMove(const MouseEvent& event)
{
    if (modelList != nullptr && modelList->isParentOf(event.eventComponent))
    {
        auto localPoint = modelList->getLocalPoint(event.eventComponent, event.position);
        int row = modelList->getRowContainingPosition(static_cast<int>(localPoint.x), static_cast<int>(localPoint.y));
        if (row != listModel.getHoveredRow())
        {
            listModel.setHoveredRow(row);
            modelList->repaint();
        }
    }
    if (irList != nullptr && irList->isParentOf(event.eventComponent))
    {
        auto localPoint = irList->getLocalPoint(event.eventComponent, event.position);
        int row = irList->getRowContainingPosition(static_cast<int>(localPoint.x), static_cast<int>(localPoint.y));
        if (row != irListModel.getHoveredRow())
        {
            irListModel.setHoveredRow(row);
            irList->repaint();
        }
    }
}

void NAMModelBrowserComponent::mouseExit(const MouseEvent& /*event*/)
{
    if (listModel.getHoveredRow() != -1)
    {
        listModel.setHoveredRow(-1);
        modelList->repaint();
    }
    if (irListModel.getHoveredRow() != -1)
    {
        irListModel.setHoveredRow(-1);
        irList->repaint();
    }
}

//==============================================================================
// IR Browser Methods
//==============================================================================

void NAMModelBrowserComponent::scanIRDirectory(const File& directory)
{
    irFiles.clear();

    // Show scanning indicator
    isScanning = true;
    statusLabel->setText("Scanning for IR files...", dontSendNotification);
    statusLabel->repaint();

    // Track seen file paths to avoid duplicates
    std::set<String> seenPaths;

    auto scanDir = [this, &seenPaths](const File& dir)
    {
        if (!dir.isDirectory())
            return;

        spdlog::info("[NAMModelBrowser] Scanning IR directory: {}", dir.getFullPathName().toStdString());

        // Find all IR files recursively (.wav, .aiff, .aif)
        auto wavFiles = dir.findChildFiles(File::findFiles, true, "*.wav");
        auto aiffFiles = dir.findChildFiles(File::findFiles, true, "*.aiff");
        auto aifFiles = dir.findChildFiles(File::findFiles, true, "*.aif");

        for (const auto& file : wavFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
        for (const auto& file : aiffFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
        for (const auto& file : aifFiles)
        {
            if (seenPaths.find(file.getFullPathName()) == seenPaths.end())
            {
                seenPaths.insert(file.getFullPathName());
                addIRFileInfo(file);
            }
        }
    };

    // Scan primary IR directory
    scanDir(directory);

    // Also scan NAM Models directory (TONE3000 downloads IRs there too)
    if (currentDirectory.isDirectory() && currentDirectory != directory)
    {
        scanDir(currentDirectory);
    }

    isScanning = false;
    spdlog::info("[NAMModelBrowser] Found {} IR files total", irFiles.size());

    // Sort by name
    std::sort(irFiles.begin(), irFiles.end(), [](const IRFileInfo& a, const IRFileInfo& b) { return a.name < b.name; });

    irListModel.setFiles(irFiles);
    irList->updateContent();
    irList->repaint();

    syncIRSelectionAfterListChange();
}

void NAMModelBrowserComponent::addIRFileInfo(const File& file)
{
    IRFileInfo info;
    info.name = file.getFileNameWithoutExtension().toStdString();
    info.filePath = file.getFullPathName().toStdString();
    info.fileSize = file.getSize();

    // Read audio file metadata
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader)
    {
        info.sampleRate = reader->sampleRate;
        info.numChannels = static_cast<int>(reader->numChannels);
        if (reader->sampleRate > 0)
            info.durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    }

    irFiles.push_back(std::move(info));
}

void NAMModelBrowserComponent::updateIRDetailsPanel(const IRFileInfo* irInfo)
{
    if (irInfo)
    {
        irNameValue->setText(String(irInfo->name), dontSendNotification);

        // Duration
        if (irInfo->durationSeconds > 0)
        {
            if (irInfo->durationSeconds >= 1.0)
                irDurationValue->setText(String(irInfo->durationSeconds, 3) + " s", dontSendNotification);
            else
                irDurationValue->setText(String(static_cast<int>(irInfo->durationSeconds * 1000)) + " ms",
                                         dontSendNotification);
        }
        else
        {
            irDurationValue->setText("-", dontSendNotification);
        }

        // Sample rate
        if (irInfo->sampleRate > 0)
            irSampleRateValue->setText(String(static_cast<int>(irInfo->sampleRate)) + " Hz", dontSendNotification);
        else
            irSampleRateValue->setText("-", dontSendNotification);

        // Channels
        if (irInfo->numChannels > 0)
        {
            String chText = irInfo->numChannels == 1
                                ? "Mono"
                                : (irInfo->numChannels == 2 ? "Stereo" : String(irInfo->numChannels) + " channels");
            irChannelsValue->setText(chText, dontSendNotification);
        }
        else
        {
            irChannelsValue->setText("-", dontSendNotification);
        }

        // File size
        if (irInfo->fileSize > 0)
        {
            String sizeText;
            if (irInfo->fileSize > 1024 * 1024)
                sizeText = String(irInfo->fileSize / (1024 * 1024)) + " MB";
            else if (irInfo->fileSize > 1024)
                sizeText = String(irInfo->fileSize / 1024) + " KB";
            else
                sizeText = String(irInfo->fileSize) + " bytes";
            irFileSizeValue->setText(sizeText, dontSendNotification);
        }
        else
        {
            irFileSizeValue->setText("-", dontSendNotification);
        }

        // File path
        File irFile(irInfo->filePath);
        irFilePathValue->setText(irFile.getFileName(), dontSendNotification);
        irFilePathValue->setTooltip(String(irInfo->filePath));
        statusLabel->setText("Selected " + String(irInfo->name) + " - double-click or Load IR to use it.",
                             dontSendNotification);
    }
    else
    {
        irNameValue->setText("-", dontSendNotification);
        irDurationValue->setText("-", dontSendNotification);
        irSampleRateValue->setText("-", dontSendNotification);
        irChannelsValue->setText("-", dontSendNotification);
        irFileSizeValue->setText("-", dontSendNotification);
        irFilePathValue->setText("-", dontSendNotification);
        irFilePathValue->setTooltip("");
        updateIRBrowserState();
    }
}

void NAMModelBrowserComponent::updateIRBrowserState()
{
    if (!statusLabel || !irList)
        return;

    const auto query = searchBox ? searchBox->getText() : String();
    const int total = irListModel.getTotalCount();
    const int filtered = irListModel.getFilteredCount();
    const bool hasFilteredRows = filtered > 0;
    const auto secondary = currentDirectory != irDirectory ? currentDirectory.getFileName() : String();

    statusLabel->setText(describeBrowserCount(irDirectory, secondary, total, filtered, "IR file", "IR files", query),
                         dontSendNotification);
    if (irEmptyStateLabel)
    {
        irEmptyStateLabel->setText(
            makeEmptyStateCopy("No impulse responses found", "Use Browse IR Folder to choose a cabinet folder.", query),
            dontSendNotification);
        irEmptyStateLabel->setVisible(!hasFilteredRows);
    }

    irList->setVisible(hasFilteredRows);
    irLoadButton->setEnabled(hasFilteredRows);
    repaint();
}

void NAMModelBrowserComponent::syncIRSelectionAfterListChange()
{
    updateIRBrowserState();

    if (!irList)
        return;

    const int filtered = irListModel.getFilteredCount();
    if (filtered <= 0)
    {
        irList->deselectAllRows();
        updateIRDetailsPanel(nullptr);
        return;
    }

    int row = irList->getSelectedRow();
    if (row < 0 || row >= filtered)
        row = 0;

    irList->selectRow(row, dontSendNotification);
    updateIRDetailsPanel(irListModel.getFileAt(row));
    irList->repaint();
}

void NAMModelBrowserComponent::loadSelectedIR()
{
    auto selectedRow = irList->getSelectedRow();
    if (selectedRow >= 0)
    {
        const auto* irInfo = irListModel.getFileAt(selectedRow);
        if (irInfo && namProcessor)
        {
            File irFile(irInfo->filePath);
            if (namProcessor->loadIR(irFile))
            {
                spdlog::info("[NAMModelBrowser] Loaded IR: {}", irInfo->name);

                if (onModelLoadedCallback)
                    onModelLoadedCallback();
            }
            else
            {
                spdlog::error("[NAMModelBrowser] Failed to load IR: {}", irInfo->name);
            }
        }
    }
}

void NAMModelBrowserComponent::onIRListSelectionChanged()
{
    auto selectedRow = irList->getSelectedRow();
    const auto* irInfo = irListModel.getFileAt(selectedRow);
    updateIRDetailsPanel(irInfo);
}

//==============================================================================
// NAMModelBrowser
//==============================================================================

std::unique_ptr<NAMModelBrowser> NAMModelBrowser::instance;
NAMProcessor* NAMModelBrowser::currentProcessor = nullptr;
std::function<void()> NAMModelBrowser::currentCallback;

NAMModelBrowser::NAMModelBrowser(NAMProcessor* processor, std::function<void()> onModelLoaded)
    : DocumentWindow("NAM Model Browser", ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::closeButton)
{
    browserLAF = std::make_unique<BrowserWindowLookAndFeel>();
    setLookAndFeel(browserLAF.get());
    setContentOwned(new NAMModelBrowserComponent(processor, std::move(onModelLoaded)), true);
    setResizable(true, false);
    setUsingNativeTitleBar(false);
    setDropShadowEnabled(true);
    centreWithSize(700, 500);
}

NAMModelBrowser::~NAMModelBrowser()
{
    setLookAndFeel(nullptr);
}

void NAMModelBrowser::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float cr = BrowserWindowLookAndFeel::cornerRadius;
    const auto palette = makeBrowserPalette();

    // Rounded window background
    g.setColour(palette.bottom);
    g.fillRoundedRectangle(bounds, cr);
}

void NAMModelBrowser::showWindow(NAMProcessor* processor, std::function<void()> onModelLoaded)
{
    // If processor changed or window doesn't exist, recreate
    if (!instance || currentProcessor != processor)
    {
        currentProcessor = processor;
        currentCallback = std::move(onModelLoaded);
        instance = std::make_unique<NAMModelBrowser>(processor, currentCallback);
    }

    instance->setVisible(true);
    instance->toFront(true);
}

//==============================================================================
// IRBrowserComponent - Standalone IR browser for IRLoaderProcessor
//==============================================================================

IRBrowserComponent::IRBrowserComponent(std::function<void(const File&)> onIRSelected)
    : onIRSelectedCallback(std::move(onIRSelected))
{
    auto& colours = ColourScheme::getInstance().colours;
    const auto palette = makeBrowserPalette();

    // Title with icon-like styling
    titleLabel = std::make_unique<Label>("title", "IR Browser");
    titleLabel->setFont(FontManager::getInstance().getSubheadingFont());
    titleLabel->setColour(Label::textColourId, palette.text);
    addAndMakeVisible(titleLabel.get());

    // Search box with improved styling
    searchBox = std::make_unique<TextEditor>("search");
    searchBox->setTextToShowWhenEmpty("Search IRs...", palette.text.withAlpha(0.4f));
    searchBox->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
    searchBox->setColour(TextEditor::textColourId, palette.text);
    searchBox->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    searchBox->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    searchBox->setIndents(28, 6); // Left indent for search icon, top indent to center text
    searchBox->setFont(FontManager::getInstance().getSubheadingFont()); // 15px fills the pill better
    searchBox->addListener(this);
    addAndMakeVisible(searchBox.get());

    // Buttons with consistent styling
    refreshButton = std::make_unique<TextButton>("Refresh");
    refreshButton->setTooltip("Rescan IR folders");
    refreshButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    refreshButton->setColour(TextButton::buttonColourId, palette.face2);
    refreshButton->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    refreshButton->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
    refreshButton->setColour(TextButton::textColourOnId, palette.text);
    refreshButton->addListener(this);
    addAndMakeVisible(refreshButton.get());

    browseFolderButton = std::make_unique<TextButton>("Folder...");
    browseFolderButton->setTooltip("Select IR folder to scan");
    browseFolderButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    browseFolderButton->setColour(TextButton::buttonColourId, palette.face2);
    browseFolderButton->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    browseFolderButton->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
    browseFolderButton->setColour(TextButton::textColourOnId, palette.text);
    browseFolderButton->addListener(this);
    addAndMakeVisible(browseFolderButton.get());

    // Load button with amber outline treatment to match the shared browser action style.
    loadButton = std::make_unique<TextButton>("Load IR");
    loadButton->setTooltip("Load selected IR");
    loadButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    loadButton->setColour(TextButton::buttonColourId, palette.accent);
    loadButton->setColour(TextButton::buttonOnColourId, palette.accent.brighter(0.2f));
    loadButton->setColour(TextButton::textColourOffId, palette.accent);
    loadButton->setColour(TextButton::textColourOnId, palette.accent.brighter(0.12f));
    loadButton->addListener(this);
    addAndMakeVisible(loadButton.get());

    closeButton = std::make_unique<TextButton>("Close");
    closeButton->setLookAndFeel(&browserActionButtonLookAndFeel);
    closeButton->setColour(TextButton::buttonColourId, palette.face2);
    closeButton->setColour(TextButton::buttonOnColourId, palette.face2.brighter(0.12f));
    closeButton->setColour(TextButton::textColourOffId, palette.text.withAlpha(0.85f));
    closeButton->setColour(TextButton::textColourOnId, palette.text);
    closeButton->addListener(this);
    addAndMakeVisible(closeButton.get());

    // IR List with improved styling
    irList = std::make_unique<ListBox>("irList", &listModel);
    irList->setRowHeight(52); // Taller rows for better readability
    irList->setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    irList->setColour(ListBox::outlineColourId, Colours::transparentBlack);
    irList->setOutlineThickness(0);
    irList->addMouseListener(this, true); // Receive mouse events from children
    addAndMakeVisible(irList.get());

    emptyStateLabel = std::make_unique<Label>(
        "emptyState", makeEmptyStateCopy("No impulse responses found", "Use Folder to choose a cabinet folder.", String()));
    emptyStateLabel->setFont(FontManager::getInstance().getBodyFont());
    emptyStateLabel->setColour(Label::textColourId, palette.text.withAlpha(0.4f));
    emptyStateLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(emptyStateLabel.get());

    // Details panel labels with improved styling
    detailsTitle = std::make_unique<Label>("detailsTitle", "IR Details");
    detailsTitle->setFont(FontManager::getInstance().getBodyBoldFont());
    detailsTitle->setColour(Label::textColourId, palette.text);
    addAndMakeVisible(detailsTitle.get());

    auto addDetailRow = [&](std::unique_ptr<Label>& label, std::unique_ptr<Label>& value, const String& labelText)
    {
        label = std::make_unique<Label>("", labelText);
        label->setFont(FontManager::getInstance().getCaptionFont());
        label->setColour(Label::textColourId, palette.text.withAlpha(0.62f));
        label->setJustificationType(Justification::centredRight);
        addAndMakeVisible(label.get());

        value = std::make_unique<Label>("", "-");
        value->setFont(FontManager::getInstance().getCaptionFont());
        value->setColour(Label::textColourId, palette.text.withAlpha(0.9f));
        value->setJustificationType(Justification::centredLeft);
        addAndMakeVisible(value.get());
    };

    addDetailRow(nameLabel, nameValue, "Name:");
    addDetailRow(durationLabel, durationValue, "Duration:");
    addDetailRow(sampleRateLabel, sampleRateValue, "Rate:");
    addDetailRow(channelsLabel, channelsValue, "Channels:");
    addDetailRow(fileSizeLabel, fileSizeValue, "Size:");

    // Status bar with path display
    statusLabel = std::make_unique<Label>("status", "");
    statusLabel->setFont(FontManager::getInstance().getCaptionFont());
    statusLabel->setColour(Label::textColourId, palette.text.withAlpha(0.5f));
    statusLabel->setJustificationType(Justification::centredLeft);
    addAndMakeVisible(statusLabel.get());

    // Set default directories
    auto pedalboard3Dir = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("Pedalboard3");

    currentDirectory = pedalboard3Dir.getChildFile("IR");
    namModelsDirectory = pedalboard3Dir.getChildFile("NAM Models");

    if (!currentDirectory.isDirectory())
        currentDirectory = File::getSpecialLocation(File::userDocumentsDirectory);

    scanDirectory(currentDirectory);
}

IRBrowserComponent::~IRBrowserComponent()
{
    refreshButton->setLookAndFeel(nullptr);
    browseFolderButton->setLookAndFeel(nullptr);
    loadButton->setLookAndFeel(nullptr);
    closeButton->setLookAndFeel(nullptr);
}

void IRBrowserComponent::paint(Graphics& g)
{
    const auto palette = makeBrowserPalette();
    auto bounds = getLocalBounds().toFloat();
    const bool compactLayout = getWidth() < 620 || getHeight() < 500;
    const bool shortLayout = getHeight() < 430;
    const int outerPadding = compactLayout ? 10 : 12;
    const int titleHeight = compactLayout ? 30 : 32;
    const int headerGap = compactLayout ? 6 : 8;
    const int searchHeight = compactLayout ? 30 : 32;
    const int contentGap = compactLayout ? 8 : 12;
    const int statusHeight = compactLayout ? 18 : 20;
    const int bottomGap = compactLayout ? 6 : 8;
    const int detailsWidth = compactLayout ? jlimit(160, 220, roundToInt(getWidth() * 0.30f)) : 210;
    const int detailsInset = compactLayout ? 6 : 8;
    const float previewHeight = shortLayout ? 58.0f : (compactLayout ? 70.0f : 86.0f);
    const bool showPreviewGlyph = !compactLayout || detailsWidth >= 190;
    const bool showLocalChip = !compactLayout || detailsWidth >= 200;

    // Gradient background
    ColourGradient bg(palette.top, 0.0f, 0.0f, palette.bottom, 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Subtle dot-grid pattern on background
    {
        g.setColour(palette.text.withAlpha(0.05f));
        const int gridStep = 16;
        for (int gy = 0; gy < getHeight(); gy += gridStep)
            for (int gx = 0; gx < getWidth(); gx += gridStep)
                g.fillEllipse((float)gx, (float)gy, 2.0f, 2.0f);
    }

    // Header faceplate with gradient
    Rectangle<float> headerArea(static_cast<float>(outerPadding), static_cast<float>(outerPadding - 2),
                                bounds.getWidth() - static_cast<float>(outerPadding * 2),
                                static_cast<float>(titleHeight + (compactLayout ? 10 : 12)));
    ColourGradient headerGradient(palette.face2, headerArea.getX(), headerArea.getY(), palette.face,
                                  headerArea.getX(), headerArea.getBottom(), false);
    g.setGradientFill(headerGradient);
    g.fillRoundedRectangle(headerArea, 9.0f);
    g.setColour(palette.edge);
    g.drawRoundedRectangle(headerArea.reduced(0.5f), 9.0f, 1.0f);
    g.setColour(palette.accent2.withAlpha(0.7f));
    g.drawLine(headerArea.getX() + 12.0f, headerArea.getBottom() - 7.0f,
               jmin(headerArea.getX() + 145.0f, headerArea.getRight() - 12.0f),
               headerArea.getBottom() - 7.0f, 2.0f);

    // Header bottom separator
    g.setColour(palette.edge.withAlpha(0.45f));
    g.drawHorizontalLine(outerPadding + titleHeight + headerGap + 2, outerPadding + 4, bounds.getWidth() - outerPadding - 4);

    // Details panel and list well. Keep this geometry aligned with resized().
    auto contentBounds = getLocalBounds().reduced(outerPadding);
    contentBounds.removeFromTop(titleHeight);
    contentBounds.removeFromTop(headerGap);
    contentBounds.removeFromTop(searchHeight);
    contentBounds.removeFromTop(contentGap);
    contentBounds.removeFromBottom(statusHeight);
    contentBounds.removeFromBottom(bottomGap);
    auto detailsArea = contentBounds.removeFromRight(detailsWidth).reduced(detailsInset);
    contentBounds.removeFromRight(compactLayout ? 8 : 12);
    auto listArea = contentBounds.reduced(4);

    g.setColour(palette.inset);
    g.fillRoundedRectangle(listArea.toFloat(), 8.0f);
    g.setColour(palette.edge);
    g.drawRoundedRectangle(listArea.toFloat().reduced(0.5f), 8.0f, 1.0f);

    // Panel shadow
    g.setColour(Colours::black.withAlpha(0.15f));
    g.fillRoundedRectangle(detailsArea.toFloat().translated(2, 2), 8.0f);

    // Panel background with gradient
    ColourGradient panelGradient(palette.face2, detailsArea.getX(), detailsArea.getY(), palette.face,
                                 detailsArea.getX(), detailsArea.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(detailsArea.toFloat(), 8.0f);

    // Panel border
    g.setColour(palette.edge);
    g.drawRoundedRectangle(detailsArea.toFloat(), 8.0f, 1.0f);

    const auto* selectedIR = irList ? listModel.getFileAt(irList->getSelectedRow()) : nullptr;
    const bool selectedIRReady = selectedIR != nullptr && isReadableIRFile(*selectedIR);
    auto previewArea = detailsArea.toFloat().reduced(compactLayout ? 7.0f : 10.0f);
    previewArea = previewArea.removeFromTop(previewHeight);
    g.setColour(selectedIR ? palette.accent2.withAlpha(selectedIRReady ? 0.12f : 0.08f)
                           : palette.text.withAlpha(0.045f));
    g.fillRoundedRectangle(previewArea, 9.0f);
    g.setColour(selectedIR ? (selectedIRReady ? palette.accent2 : palette.accent).withAlpha(0.46f)
                           : palette.edge.withAlpha(0.42f));
    g.drawRoundedRectangle(previewArea.reduced(0.5f), 9.0f, 1.0f);

    if (showPreviewGlyph)
    {
        auto glyphArea = previewArea.removeFromLeft(compactLayout ? 42.0f : 52.0f)
                             .withSizeKeepingCentre(compactLayout ? 30.0f : 40.0f, compactLayout ? 30.0f : 40.0f);
        drawIRGlyph(g, glyphArea, selectedIRReady ? palette.accent2 : palette.accent, selectedIR != nullptr);
    }

    auto previewText = previewArea.reduced(4.0f, compactLayout ? 5.0f : 7.0f);
    auto chipRow = previewText.removeFromBottom(compactLayout ? 18.0f : 20.0f);
    g.setFont(compactLayout ? FontManager::getInstance().getBodyBoldFont().withHeight(12.0f)
                            : FontManager::getInstance().getBodyBoldFont());
    g.setColour(selectedIR ? palette.text : palette.text.withAlpha(0.54f));
    g.drawText(selectedIR ? String(selectedIR->name) : String("Select an IR"),
               previewText.removeFromTop(compactLayout ? 18.0f : 24.0f),
               Justification::centredLeft, true);

    g.setFont(FontManager::getInstance().getCaptionFont());
    g.setColour(palette.text.withAlpha(selectedIR ? 0.62f : 0.38f));
    g.drawText(selectedIR ? makeIRPreviewSummary(*selectedIR) : String("Double-click a row or use Load IR"),
               previewText, Justification::centredLeft, true);

    drawBrowserChip(g, chipRow.removeFromLeft(selectedIR && !selectedIRReady ? 70.0f : 58.0f),
                    selectedIR ? (selectedIRReady ? "READY" : "MISSING") : "EMPTY",
                    selectedIR ? (selectedIRReady ? palette.led : palette.accent) : palette.text.withAlpha(0.45f),
                    selectedIR != nullptr);
    if (showLocalChip)
    {
        chipRow.removeFromLeft(6.0f);
        drawBrowserChip(g, chipRow.removeFromLeft(56.0f), "LOCAL", palette.accent2, selectedIR != nullptr);
    }

    auto drawDetailRowBacking = [&](Label* label, Label* value, int rowIndex)
    {
        if (label == nullptr || value == nullptr || !label->isVisible() || !value->isVisible())
            return;

        auto row = label->getBounds().getUnion(value->getBounds()).toFloat().expanded(5.0f, 2.0f);
        g.setColour((rowIndex % 2 == 0 ? palette.inset : palette.face).withAlpha(0.44f));
        g.fillRoundedRectangle(row, 6.0f);
        g.setColour(palette.edge.withAlpha(0.32f));
        g.drawRoundedRectangle(row.reduced(0.5f), 6.0f, 0.75f);
    };

    drawDetailRowBacking(nameLabel.get(), nameValue.get(), 0);
    drawDetailRowBacking(durationLabel.get(), durationValue.get(), 1);
    drawDetailRowBacking(sampleRateLabel.get(), sampleRateValue.get(), 2);
    drawDetailRowBacking(channelsLabel.get(), channelsValue.get(), 3);
    drawDetailRowBacking(fileSizeLabel.get(), fileSizeValue.get(), 4);

    if (emptyStateLabel && emptyStateLabel->isVisible())
    {
        auto emptyBounds = emptyStateLabel->getBounds().toFloat();
        drawIRGlyph(g, emptyBounds.withSizeKeepingCentre(42.0f, 42.0f).translated(0.0f, -52.0f),
                    palette.accent2.withAlpha(0.8f), false);
    }

    // Status bar background
    const float statusStripHeight = compactLayout ? 26.0f : 30.0f;
    Rectangle<float> statusArea(0, bounds.getHeight() - statusStripHeight, bounds.getWidth(), statusStripHeight);
    g.setColour(palette.bottom.darker(0.08f));
    g.fillRect(statusArea);
    g.setColour(palette.edge);
    g.drawHorizontalLine(static_cast<int>(bounds.getHeight() - statusStripHeight), 0, bounds.getWidth());
}

void IRBrowserComponent::paintOverChildren(Graphics& g)
{
    // Draw magnifying glass icon centered in the search pill
    const auto palette = makeBrowserPalette();
    auto searchBounds = searchBox->getBounds().toFloat();

    float iconSize = 13.0f;
    float radius = iconSize * 0.35f;
    float iconX = searchBounds.getX() + 10.0f;
    float iconCentreY = searchBounds.getCentreY();

    g.setColour(palette.text.withAlpha(0.45f));
    // Circle part — centered vertically
    g.drawEllipse(iconX, iconCentreY - radius, radius * 2.0f, radius * 2.0f, 1.5f);
    // Handle
    float handleStartX = iconX + radius + radius * 0.7f;
    float handleStartY = iconCentreY + radius * 0.7f;
    g.drawLine(handleStartX, handleStartY, handleStartX + radius * 0.8f, handleStartY + radius * 0.8f, 1.5f);
}

void IRBrowserComponent::resized()
{
    const bool compactLayout = getWidth() < 620 || getHeight() < 500;
    const bool shortLayout = getHeight() < 430;
    const int outerPadding = compactLayout ? 10 : 12;
    const int titleHeight = compactLayout ? 30 : 32;
    const int headerGap = compactLayout ? 6 : 8;
    const int searchHeight = compactLayout ? 30 : 32;
    const int contentGap = compactLayout ? 8 : 12;
    const int statusHeight = compactLayout ? 18 : 20;
    const int bottomGap = compactLayout ? 6 : 8;
    const int detailsWidth = compactLayout ? jlimit(160, 220, roundToInt(getWidth() * 0.30f)) : 210;
    const int detailsInset = compactLayout ? 6 : 8;
    const int previewReserved = shortLayout ? 68 : (compactLayout ? 82 : 96);
    const int rowHeight = compactLayout ? 20 : 22;
    const int rowGap = compactLayout ? 3 : 4;
    const int labelWidth = compactLayout ? 62 : 76;

    titleLabel->setFont(compactLayout ? FontManager::getInstance().getBodyBoldFont()
                                      : FontManager::getInstance().getSubheadingFont());
    searchBox->setFont(compactLayout ? FontManager::getInstance().getBodyFont()
                                     : FontManager::getInstance().getSubheadingFont());
    irList->setRowHeight(compactLayout ? 46 : 52);

    auto bounds = getLocalBounds().reduced(outerPadding);

    // Title row (inside header area)
    auto titleRow = bounds.removeFromTop(titleHeight);
    titleLabel->setBounds(titleRow.removeFromLeft(compactLayout ? 104 : 120));
    closeButton->setBounds(titleRow.removeFromRight(compactLayout ? 58 : 65));
    titleRow.removeFromRight(compactLayout ? 6 : 8);
    loadButton->setBounds(titleRow.removeFromRight(compactLayout ? 72 : 80));

    bounds.removeFromTop(headerGap);

    // Search/button row
    auto searchRow = bounds.removeFromTop(searchHeight);
    if (compactLayout)
    {
        browseFolderButton->setBounds(searchRow.removeFromRight(68));
        searchRow.removeFromRight(5);
        refreshButton->setBounds(searchRow.removeFromRight(62));
        searchRow.removeFromRight(6);
        searchBox->setBounds(searchRow);
    }
    else
    {
        searchBox->setBounds(searchRow.removeFromLeft(180));
        searchRow.removeFromLeft(8);
        refreshButton->setBounds(searchRow.removeFromLeft(65));
        searchRow.removeFromLeft(5);
        browseFolderButton->setBounds(searchRow.removeFromLeft(70));
    }

    bounds.removeFromTop(contentGap);

    // Status bar at bottom
    auto statusRow = bounds.removeFromBottom(statusHeight);
    statusLabel->setBounds(statusRow);

    bounds.removeFromBottom(bottomGap);

    // Details panel on right
    auto detailsArea = bounds.removeFromRight(detailsWidth).reduced(detailsInset);
    bounds.removeFromRight(compactLayout ? 8 : 12);

    detailsArea.removeFromTop(previewReserved);

    detailsTitle->setBounds(detailsArea.removeFromTop(compactLayout ? 20 : 24));
    detailsArea.removeFromTop(compactLayout ? 7 : 10);

    auto addDetailLayout = [&](Label* label, Label* value)
    {
        if (label == nullptr || value == nullptr || !label->isVisible() || !value->isVisible())
            return;

        auto row = detailsArea.removeFromTop(rowHeight);
        label->setBounds(row.removeFromLeft(labelWidth));
        row.removeFromLeft(compactLayout ? 4 : 5);
        value->setBounds(row);
        detailsArea.removeFromTop(rowGap);
    };

    const bool showSampleRateRow = detailsArea.getHeight() >= (compactLayout ? 50 : 74);
    const bool showChannelsRow = !compactLayout || detailsArea.getHeight() >= 78;
    const bool showFileSizeRow = !compactLayout || detailsArea.getHeight() >= 106;
    sampleRateLabel->setVisible(showSampleRateRow);
    sampleRateValue->setVisible(showSampleRateRow);
    channelsLabel->setVisible(showChannelsRow);
    channelsValue->setVisible(showChannelsRow);
    fileSizeLabel->setVisible(showFileSizeRow);
    fileSizeValue->setVisible(showFileSizeRow);

    addDetailLayout(nameLabel.get(), nameValue.get());
    addDetailLayout(durationLabel.get(), durationValue.get());
    addDetailLayout(sampleRateLabel.get(), sampleRateValue.get());
    addDetailLayout(channelsLabel.get(), channelsValue.get());
    addDetailLayout(fileSizeLabel.get(), fileSizeValue.get());

    // IR list takes remaining space with rounded corners visual
    irList->setBounds(bounds);
    emptyStateLabel->setBounds(bounds);
    updateBrowserState();
}

void IRBrowserComponent::buttonClicked(Button* button)
{
    if (button == refreshButton.get())
    {
        scanDirectory(currentDirectory);
    }
    else if (button == browseFolderButton.get())
    {
        folderChooser = std::make_unique<FileChooser>("Select IR Folder", currentDirectory);
        auto flags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        folderChooser->launchAsync(flags,
                                   [this](const FileChooser& fc)
                                   {
                                       auto result = fc.getResult();
                                       if (result.isDirectory())
                                       {
                                           currentDirectory = result;
                                           scanDirectory(currentDirectory);
                                       }
                                   });
    }
    else if (button == loadButton.get())
    {
        loadSelectedIR();
    }
    else if (button == closeButton.get())
    {
        if (auto* window = findParentComponentOfClass<DocumentWindow>())
            window->setVisible(false);
    }
}

void IRBrowserComponent::textEditorTextChanged(TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        listModel.setFilter(searchBox->getText());
        irList->updateContent();
        irList->repaint();
        syncSelectionAfterListChange();
    }
}

void IRBrowserComponent::mouseUp(const MouseEvent& event)
{
    if (irList->isParentOf(event.eventComponent))
    {
        // Use callAsync to ensure ListBox has updated selection before we query it
        juce::MessageManager::callAsync([this]() { onListSelectionChanged(); });
    }
}

void IRBrowserComponent::mouseDoubleClick(const MouseEvent& event)
{
    if (irList->isParentOf(event.eventComponent))
    {
        loadSelectedIR();
    }
}

void IRBrowserComponent::mouseMove(const MouseEvent& event)
{
    if (irList->isParentOf(event.eventComponent))
    {
        auto localPoint = irList->getLocalPoint(event.eventComponent, event.position);
        int row = irList->getRowContainingPosition(static_cast<int>(localPoint.x), static_cast<int>(localPoint.y));
        if (row != listModel.getHoveredRow())
        {
            listModel.setHoveredRow(row);
            irList->repaint();
        }
    }
}

void IRBrowserComponent::mouseExit(const MouseEvent& /*event*/)
{
    if (listModel.getHoveredRow() != -1)
    {
        listModel.setHoveredRow(-1);
        irList->repaint();
    }
}

void IRBrowserComponent::scanDirectory(const File& directory)
{
    irFiles.clear();

    statusLabel->setText("Scanning for IR files...", dontSendNotification);

    std::set<String> seenPaths;
    AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    auto addFile = [&](const File& file)
    {
        if (seenPaths.find(file.getFullPathName()) != seenPaths.end())
            return;
        seenPaths.insert(file.getFullPathName());

        IRFileInfo info;
        info.name = file.getFileNameWithoutExtension().toStdString();
        info.filePath = file.getFullPathName().toStdString();
        info.fileSize = file.getSize();

        std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader)
        {
            info.sampleRate = reader->sampleRate;
            info.numChannels = static_cast<int>(reader->numChannels);
            if (reader->sampleRate > 0)
                info.durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;

            spdlog::debug("[IRBrowser] Loaded IR: {} - {}Hz, {}ch, {:.3f}s", info.name, info.sampleRate,
                          info.numChannels, info.durationSeconds);
        }
        else
        {
            spdlog::warn("[IRBrowser] Failed to read audio file: {}", file.getFullPathName().toStdString());
        }

        irFiles.push_back(std::move(info));
    };

    auto scanDir = [&](const File& dir)
    {
        if (!dir.isDirectory())
            return;

        spdlog::info("[IRBrowser] Scanning directory: {}", dir.getFullPathName().toStdString());

        auto wavFiles = dir.findChildFiles(File::findFiles, true, "*.wav");
        auto aiffFiles = dir.findChildFiles(File::findFiles, true, "*.aiff");
        auto aifFiles = dir.findChildFiles(File::findFiles, true, "*.aif");

        for (const auto& f : wavFiles)
            addFile(f);
        for (const auto& f : aiffFiles)
            addFile(f);
        for (const auto& f : aifFiles)
            addFile(f);
    };

    // Scan primary IR directory
    scanDir(directory);

    // Also scan NAM Models directory (TONE3000 downloads IRs there too)
    if (namModelsDirectory.isDirectory() && namModelsDirectory != directory)
    {
        scanDir(namModelsDirectory);
    }

    spdlog::info("[IRBrowser] Found {} IR files total", irFiles.size());

    std::sort(irFiles.begin(), irFiles.end(), [](const IRFileInfo& a, const IRFileInfo& b) { return a.name < b.name; });

    listModel.setFiles(irFiles);
    irList->updateContent();
    irList->repaint();

    syncSelectionAfterListChange();
}

void IRBrowserComponent::updateDetailsPanel(const IRFileInfo* irInfo)
{
    if (irInfo)
    {
        nameValue->setText(String(irInfo->name), dontSendNotification);

        durationValue->setText(formatIRDuration(irInfo->durationSeconds), dontSendNotification);
        sampleRateValue->setText(formatIRSampleRate(irInfo->sampleRate), dontSendNotification);
        channelsValue->setText(formatIRChannels(irInfo->numChannels), dontSendNotification);
        fileSizeValue->setText(formatIRFileSize(irInfo->fileSize), dontSendNotification);
        statusLabel->setText("Selected " + String(irInfo->name) + " - double-click or Load IR to use it.",
                             dontSendNotification);
        if (loadButton)
            loadButton->setEnabled(isReadableIRFile(*irInfo));
    }
    else
    {
        nameValue->setText("-", dontSendNotification);
        durationValue->setText("-", dontSendNotification);
        sampleRateValue->setText("-", dontSendNotification);
        channelsValue->setText("-", dontSendNotification);
        fileSizeValue->setText("-", dontSendNotification);
        if (loadButton)
            loadButton->setEnabled(false);
        updateBrowserState();
    }
}

void IRBrowserComponent::updateBrowserState()
{
    if (!statusLabel || !irList || !emptyStateLabel)
        return;

    const auto query = searchBox ? searchBox->getText() : String();
    const int total = listModel.getTotalCount();
    const int filtered = listModel.getFilteredCount();
    const bool hasFilteredRows = filtered > 0;
    const auto secondary = namModelsDirectory.isDirectory() && namModelsDirectory != currentDirectory
                               ? namModelsDirectory.getFileName()
                               : String();

    statusLabel->setText(describeBrowserCount(currentDirectory, secondary, total, filtered, "IR file", "IR files", query),
                         dontSendNotification);
    emptyStateLabel->setText(
        makeEmptyStateCopy("No impulse responses found", "Use Folder to choose a cabinet folder.", query),
        dontSendNotification);
    irList->setVisible(hasFilteredRows);
    emptyStateLabel->setVisible(!hasFilteredRows);
    const int selectedRow = irList->getSelectedRow();
    const auto* selectedIR = selectedRow >= 0 ? listModel.getFileAt(selectedRow) : nullptr;
    loadButton->setEnabled(hasFilteredRows && selectedIR != nullptr && isReadableIRFile(*selectedIR));
    repaint();
}

void IRBrowserComponent::syncSelectionAfterListChange()
{
    updateBrowserState();

    if (!irList)
        return;

    const int filtered = listModel.getFilteredCount();
    if (filtered <= 0)
    {
        irList->deselectAllRows();
        updateDetailsPanel(nullptr);
        return;
    }

    int row = irList->getSelectedRow();
    if (row < 0 || row >= filtered)
        row = 0;

    irList->selectRow(row, dontSendNotification);
    updateDetailsPanel(listModel.getFileAt(row));
    irList->repaint();
}

void IRBrowserComponent::loadSelectedIR()
{
    int selectedRow = irList->getSelectedRow();
    const IRFileInfo* ir = listModel.getFileAt(selectedRow);

    if (ir == nullptr)
        return;

    File irFile(ir->filePath);
    if (!irFile.existsAsFile())
        return;

    spdlog::info("[IRBrowser] Loading IR: {}", ir->name);

    if (onIRSelectedCallback)
        onIRSelectedCallback(irFile);

    // Close window after loading
    if (auto* window = findParentComponentOfClass<DocumentWindow>())
        window->setVisible(false);
}

void IRBrowserComponent::onListSelectionChanged()
{
    int selectedRow = irList->getSelectedRow();
    const IRFileInfo* ir = listModel.getFileAt(selectedRow);
    updateDetailsPanel(ir);
}

//==============================================================================
// IRBrowser Window
//==============================================================================

std::unique_ptr<IRBrowser> IRBrowser::instance;
std::function<void(const File&)> IRBrowser::currentCallback;

IRBrowser::IRBrowser(std::function<void(const File&)> onIRSelected)
    : DocumentWindow("IR Browser", ColourScheme::getInstance().colours["Window Background"],
                     DocumentWindow::closeButton)
{
    browserLAF = std::make_unique<BrowserWindowLookAndFeel>();
    setLookAndFeel(browserLAF.get());
    setContentOwned(new IRBrowserComponent(std::move(onIRSelected)), true);
    setResizable(true, true);
    setResizeLimits(500, 350, 1200, 800);
    setUsingNativeTitleBar(false);
    setDropShadowEnabled(true);
    centreWithSize(600, 450);
}

IRBrowser::~IRBrowser()
{
    setLookAndFeel(nullptr);
}

void IRBrowser::paint(Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float cr = BrowserWindowLookAndFeel::cornerRadius;
    auto& colours = ColourScheme::getInstance().colours;

    // Rounded window background
    g.setColour(colours["Window Background"]);
    g.fillRoundedRectangle(bounds, cr);
}

void IRBrowser::showWindow(std::function<void(const File&)> onIRSelected)
{
    currentCallback = std::move(onIRSelected);
    instance = std::make_unique<IRBrowser>(currentCallback);
    instance->setVisible(true);
    instance->toFront(true);
}
