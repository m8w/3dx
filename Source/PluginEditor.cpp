#include "PluginEditor.h"
#include "Params.h"
#include "Presets.h"

namespace Colours2
{
    static const juce::Colour bg { 0xff0a0a0c };
    static const juce::Colour panel { 0xff101014 };
    static const juce::Colour line { 0xff1d1d24 };
    static const juce::Colour fg { 0xffc9c9ce };
    static const juce::Colour dim { 0xff5c5c66 };
    static const juce::Colour faint { 0xff33333c };
    static const juce::Colour acc { 0xffffb347 };
    static const juce::Colour acc2 { 0xff47d7ff };
}

//==============================================================================
void GoniometerView::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(Colours2::panel);
    g.setColour(Colours2::line);
    g.drawLine(b.getCentreX(), 0, b.getCentreX(), b.getHeight());
    g.drawLine(0, b.getCentreY(), b.getWidth(), b.getCentreY());
    g.drawEllipse(b.reduced(b.getWidth() * 0.12f), 1.0f);

    float cx = b.getCentreX(), cy = b.getCentreY();
    float R = juce::jmin(b.getWidth(), b.getHeight()) * 0.42f;

    int wi = proc.scope.writeIndex.load(std::memory_order_relaxed);
    g.setColour(Colours2::acc);
    for (int i = 0; i < QuaternionAudioProcessor::kScopeSize; i += 2)
    {
        int idx = (wi - 1 - i + QuaternionAudioProcessor::kScopeSize * 4) % QuaternionAudioProcessor::kScopeSize;
        float x = cx + juce::jlimit(-1.0f, 1.0f, proc.scope.compA[static_cast<size_t>(idx)]) * R;
        float y = cy - juce::jlimit(-1.0f, 1.0f, proc.scope.compB[static_cast<size_t>(idx)]) * R;
        g.setOpacity(0.12f + 0.7f * (1.0f - static_cast<float>(i) / QuaternionAudioProcessor::kScopeSize));
        g.fillRect(x, y, 1.6f, 1.6f);
    }
    g.setOpacity(1.0f);
}

//==============================================================================
SpectrumView::SpectrumView(QuaternionAudioProcessor& p) : proc(p) { startTimerHz(24); }

void SpectrumView::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(Colours2::panel);

    int wi = proc.scope.writeIndex.load(std::memory_order_relaxed);
    fftBuf.fill(0.0f);
    for (int i = 0; i < QuaternionAudioProcessor::kScopeSize; ++i)
    {
        int idx = (wi - 1 - i + QuaternionAudioProcessor::kScopeSize * 4) % QuaternionAudioProcessor::kScopeSize;
        float window = 0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i)
                                               / static_cast<float>(QuaternionAudioProcessor::kScopeSize - 1));
        fftBuf[static_cast<size_t>(i)] = proc.scope.mono[static_cast<size_t>(idx)] * window;
    }
    fft.performFrequencyOnlyForwardTransform(fftBuf.data());

    float w = b.getWidth(), h = b.getHeight();
    juce::Path path;
    bool started = false;
    for (int x = 0; x < static_cast<int>(w); ++x)
    {
        float prop = static_cast<float>(x) / w;
        int bin = juce::jlimit(0, 511, static_cast<int>(std::pow(prop, 2.2) * 511));
        float mag = fftBuf[static_cast<size_t>(bin)] / 30.0f;
        float y = h - juce::jlimit(0.0f, 1.0f, mag) * (h - 4.0f) - 2.0f;
        if (!started) { path.startNewSubPath(static_cast<float>(x), y); started = true; }
        else path.lineTo(static_cast<float>(x), y);
    }
    g.setColour(Colours2::acc);
    g.strokePath(path, juce::PathStrokeType(1.0f));
}

//==============================================================================
ParamRow::ParamRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    label.setColour(juce::Label::textColourId, Colours2::dim);
    addAndMakeVisible(label);

    slider.setColour(juce::Slider::trackColourId, Colours2::faint);
    slider.setColour(juce::Slider::thumbColourId, Colours2::acc);
    slider.setColour(juce::Slider::textBoxTextColourId, Colours2::fg);
    slider.setColour(juce::Slider::textBoxOutlineColourId, Colours2::line);
    slider.setColour(juce::Slider::backgroundColourId, Colours2::panel);
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
}

void ParamRow::resized()
{
    auto b = getLocalBounds().reduced(2);
    label.setBounds(b.removeFromLeft(96));
    slider.setBounds(b);
}

//==============================================================================
ParamChoiceRow::ParamChoiceRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    label.setColour(juce::Label::textColourId, Colours2::dim);
    addAndMakeVisible(label);

    box.setColour(juce::ComboBox::backgroundColourId, Colours2::panel);
    box.setColour(juce::ComboBox::textColourId, Colours2::fg);
    box.setColour(juce::ComboBox::outlineColourId, Colours2::line);
    addAndMakeVisible(box);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId)))
    {
        int i = 1;
        for (auto& choice : choiceParam->choices)
            box.addItem(choice, i++);
    }
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, box);
}

void ParamChoiceRow::resized()
{
    auto b = getLocalBounds().reduced(2);
    label.setBounds(b.removeFromLeft(96));
    box.setBounds(b);
}

//==============================================================================
SectionHeaderRow::SectionHeaderRow(const juce::String& t) : text(t) {}

void SectionHeaderRow::paint(juce::Graphics& g)
{
    g.setColour(Colours2::faint);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(text, getLocalBounds().withTrimmedLeft(2), juce::Justification::centredLeft);
    g.drawLine(0, static_cast<float>(getHeight()) - 1, static_cast<float>(getWidth()), static_cast<float>(getHeight()) - 1);
}

//==============================================================================
QuaternionAudioProcessorEditor::QuaternionAudioProcessorEditor(QuaternionAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), gonio(p), spectrum(p)
{
    addAndMakeVisible(gonio);
    addAndMakeVisible(spectrum);

    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    presetLabel.setColour(juce::Label::textColourId, Colours2::dim);
    addAndMakeVisible(presetLabel);

    presetBox.setColour(juce::ComboBox::backgroundColourId, Colours2::panel);
    presetBox.setColour(juce::ComboBox::textColourId, Colours2::fg);
    presetBox.setColour(juce::ComboBox::outlineColourId, Colours2::line);
    int i = 1;
    for (auto& preset : Presets::factoryPresets())
        presetBox.addItem(preset.name, i++);
    presetBox.setSelectedItemIndex(proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        proc.setCurrentProgram(presetBox.getSelectedItemIndex());
    };
    addAndMakeVisible(presetBox);

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);

    addSection("VOICES -- sine -> triangle -> saw -> vocoder");
    addRow(Params::wave, "WAVE");
    addRow(Params::fan, "FAN");
    addRow(Params::formant, "FORMANT");
    addRow(Params::scMix, "SC MIX");

    addSection("SEQUENCE -- harmonic-series patterns");
    addChoiceRow(Params::pattern, "PATTERN");
    addRow(Params::rate, "RATE");
    addRow(Params::range, "RANGE");
    addRow(Params::glide, "GLIDE");
    addRow(Params::gate, "GATE");
    addRow(Params::env, "ENV");

    addSection("QUATERNION -- Z = Y . X");
    addChoiceRow(Params::algMode, "ALGEBRA");
    addRow(Params::spin, "SPIN");
    addRow(Params::drift, "DRIFT");
    addRow(Params::depth, "DEPTH");
    addRow(Params::fmAmount, "FM AMOUNT");

    addSection("SHIFT -- Re[(I+jQ) . e^{jwt}]");
    addRow(Params::shiftHz, "SHIFT HZ");
    addRow(Params::balance, "BALANCE");
    addRow(Params::feedback, "FEEDBACK");
    addRow(Params::fbTime, "FB TIME");
    addRow(Params::shiftMix, "SHIFT MIX");

    addSection("MEMORY / OUTPUT");
    addRow(Params::haunt, "HAUNT");
    addRow(Params::output, "OUTPUT");

    setResizable(true, true);
    setSize(480, 752);
    setResizeLimits(380, 512, 900, 1400);
}

void QuaternionAudioProcessorEditor::addSection(const juce::String& text)
{
    auto row = std::make_unique<SectionHeaderRow>(text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void QuaternionAudioProcessorEditor::addRow(const juce::String& paramId, const juce::String& text)
{
    auto row = std::make_unique<ParamRow>(proc.apvts, paramId, text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void QuaternionAudioProcessorEditor::addChoiceRow(const juce::String& paramId, const juce::String& text)
{
    auto row = std::make_unique<ParamChoiceRow>(proc.apvts, paramId, text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void QuaternionAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours2::bg);
}

void QuaternionAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(10);

    auto presetArea = b.removeFromTop(24);
    presetLabel.setBounds(presetArea.removeFromLeft(56));
    presetBox.setBounds(presetArea);
    b.removeFromTop(8);

    auto scopesArea = b.removeFromTop(160);
    gonio.setBounds(scopesArea.removeFromLeft(160));
    scopesArea.removeFromLeft(8);
    spectrum.setBounds(scopesArea);

    b.removeFromTop(8);
    viewport.setBounds(b);

    int y = 0;
    const int rowH = 30;
    const int headerH = 22;
    for (auto& c : rows)
    {
        bool isHeader = dynamic_cast<SectionHeaderRow*>(c.get()) != nullptr;
        int h = isHeader ? headerH : rowH;
        c->setBounds(0, y, b.getWidth() - 10, h);
        y += h;
    }
    content.setSize(b.getWidth() - 10, y + 8);
}
