#include "PluginEditor.h"
#include "Params.h"

namespace Colours2
{
    static const juce::Colour bg { 0xff0a0a0c };
    static const juce::Colour panel { 0xff101014 };
    static const juce::Colour line { 0xff1d1d24 };
    static const juce::Colour fg { 0xffc9c9ce };
    static const juce::Colour dim { 0xff5c5c66 };
    static const juce::Colour faint { 0xff33333c };
    static const juce::Colour acc { 0xffffb347 };
}

//==============================================================================
void BreathingOrb::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(Colours2::panel);

    float level = proc.outputLevel.load(std::memory_order_relaxed);
    float cx = b.getCentreX(), cy = b.getCentreY();
    float base = juce::jmin(b.getWidth(), b.getHeight()) * (0.16f + juce::jlimit(0.0f, 1.0f, level * 3.0f) * 0.34f);

    hue += 0.15f + level * 4.0f;
    if (hue > 360.0f) hue -= 360.0f;

    juce::ColourGradient grad(juce::Colour::fromHSV(hue / 360.0f, 0.55f, 0.75f, 0.55f), cx, cy,
                               Colours2::panel.withAlpha(0.0f), cx, cy - base * 2.2f, true);
    grad.addColour(0.5, juce::Colour::fromHSV(std::fmod(hue + 30.0f, 360.0f) / 360.0f, 0.55f, 0.6f, 0.18f));
    g.setGradientFill(grad);
    g.fillEllipse(cx - base * 2.2f, cy - base * 2.2f, base * 4.4f, base * 4.4f);

    g.setColour(juce::Colours::white.withAlpha(0.25f));
    g.drawEllipse(cx - base, cy - base, base * 2.0f, base * 2.0f, 1.0f);
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
SerenityAudioProcessorEditor::SerenityAudioProcessorEditor(SerenityAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), orb(p)
{
    addAndMakeVisible(orb);

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);

    addSection("PADS -- major/minor pentatonic, dorian, lydian");
    addChoiceRow(Params::scale, "SCALE");
    addRow(Params::brightness, "BRIGHTNESS");
    addRow(Params::drift, "DRIFT");

    addSection("CHIMES -- random FM bells");
    addRow(Params::density, "DENSITY");

    addSection("SPACE / OUTPUT");
    addRow(Params::space, "SPACE");
    addRow(Params::output, "OUTPUT");

    setResizable(true, true);
    setSize(420, 480);
    setResizeLimits(340, 380, 800, 1000);
}

void SerenityAudioProcessorEditor::addSection(const juce::String& text)
{
    auto row = std::make_unique<SectionHeaderRow>(text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void SerenityAudioProcessorEditor::addRow(const juce::String& paramId, const juce::String& text)
{
    auto row = std::make_unique<ParamRow>(proc.apvts, paramId, text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void SerenityAudioProcessorEditor::addChoiceRow(const juce::String& paramId, const juce::String& text)
{
    auto row = std::make_unique<ParamChoiceRow>(proc.apvts, paramId, text);
    content.addAndMakeVisible(*row);
    rows.push_back(std::move(row));
}

void SerenityAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Colours2::bg);
}

void SerenityAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(10);

    orb.setBounds(b.removeFromTop(160));
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
