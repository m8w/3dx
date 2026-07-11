#include "PluginEditor.h"
#include "Params.h"

//==============================================================================
void ThemeSwatchButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(palette.acc);
    g.fillRect(b);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    g.drawText(palette.name, getLocalBounds(), juce::Justification::centred);
    if (selected)
    {
        g.setColour(palette.acc3);
        g.drawRect(getLocalBounds(), 2);
    }
}

//==============================================================================
ThemeRow::ThemeRow()
{
    int i = 0;
    for (auto& p : themes())
    {
        auto sw = std::make_unique<ThemeSwatchButton>(p, i);
        sw->onClick = [this](int idx) { if (onThemeSelected) onThemeSelected(idx); };
        addAndMakeVisible(*sw);
        swatches.push_back(std::move(sw));
        ++i;
    }
}

void ThemeRow::resized()
{
    auto b = getLocalBounds();
    int n = static_cast<int>(swatches.size());
    int w = n > 0 ? b.getWidth() / n : b.getWidth();
    for (int i = 0; i < n; ++i)
        swatches[static_cast<size_t>(i)]->setBounds(b.getX() + i * w, b.getY(), (i == n - 1 ? b.getWidth() - i * w : w), b.getHeight());
}

void ThemeRow::setSelectedIndex(int idx)
{
    for (int i = 0; i < static_cast<int>(swatches.size()); ++i)
        swatches[static_cast<size_t>(i)]->setSelected(i == idx);
}

//==============================================================================
void BreathingOrb::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.fillAll(pal.panel);

    float level = proc.outputLevel.load(std::memory_order_relaxed);
    float cx = b.getCentreX(), cy = b.getCentreY();
    float base = juce::jmin(b.getWidth(), b.getHeight()) * (0.16f + juce::jlimit(0.0f, 1.0f, level * 3.0f) * 0.34f);

    hue += 0.15f + level * 4.0f;
    if (hue > 360.0f) hue -= 360.0f;

    juce::ColourGradient grad(juce::Colour::fromHSV(hue / 360.0f, 0.55f, 0.75f, 0.55f), cx, cy,
                               pal.panel.withAlpha(0.0f), cx, cy - base * 2.2f, true);
    grad.addColour(0.5, juce::Colour::fromHSV(std::fmod(hue + 30.0f, 360.0f) / 360.0f, 0.55f, 0.6f, 0.18f));
    g.setGradientFill(grad);
    g.fillEllipse(cx - base * 2.2f, cy - base * 2.2f, base * 4.4f, base * 4.4f);

    g.setColour(pal.fg.withAlpha(0.35f));
    g.drawEllipse(cx - base, cy - base, base * 2.0f, base * 2.0f, 1.0f);
}

//==============================================================================
ParamRow::ParamRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(label);
    addAndMakeVisible(slider);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramId, slider);
    setPalette(themes()[0]);
}

void ParamRow::resized()
{
    auto b = getLocalBounds().reduced(2);
    label.setBounds(b.removeFromLeft(96));
    slider.setBounds(b);
}

void ParamRow::setPalette(const Palette& p)
{
    label.setColour(juce::Label::textColourId, p.dim);
    slider.setColour(juce::Slider::trackColourId, p.faint);
    slider.setColour(juce::Slider::thumbColourId, p.acc);
    slider.setColour(juce::Slider::textBoxTextColourId, p.fg);
    slider.setColour(juce::Slider::textBoxOutlineColourId, p.line);
    slider.setColour(juce::Slider::backgroundColourId, p.panel);
    repaint();
}

//==============================================================================
ParamChoiceRow::ParamChoiceRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(label);
    addAndMakeVisible(box);

    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId)))
    {
        int i = 1;
        for (auto& choice : choiceParam->choices)
            box.addItem(choice, i++);
    }
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, paramId, box);
    setPalette(themes()[0]);
}

void ParamChoiceRow::resized()
{
    auto b = getLocalBounds().reduced(2);
    label.setBounds(b.removeFromLeft(96));
    box.setBounds(b);
}

void ParamChoiceRow::setPalette(const Palette& p)
{
    label.setColour(juce::Label::textColourId, p.dim);
    box.setColour(juce::ComboBox::backgroundColourId, p.panel);
    box.setColour(juce::ComboBox::textColourId, p.fg);
    box.setColour(juce::ComboBox::outlineColourId, p.line);
    repaint();
}

//==============================================================================
SectionHeaderRow::SectionHeaderRow(const juce::String& t) : text(t) {}

void SectionHeaderRow::paint(juce::Graphics& g)
{
    g.setColour(pal.faint);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(text, getLocalBounds().withTrimmedLeft(2), juce::Justification::centredLeft);
    g.drawLine(0, static_cast<float>(getHeight()) - 1, static_cast<float>(getWidth()), static_cast<float>(getHeight()) - 1);
}

//==============================================================================
SerenityAudioProcessorEditor::SerenityAudioProcessorEditor(SerenityAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p), orb(p)
{
    addAndMakeVisible(themeRow);
    themeRow.onThemeSelected = [this](int idx)
    {
        proc.setThemeIndex(idx);
        applyPalette(idx);
    };

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

    applyPalette(juce::jlimit(0, static_cast<int>(themes().size()) - 1, proc.getThemeIndex()));

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

void SerenityAudioProcessorEditor::applyPalette(int themeIndex)
{
    currentPalette = themes()[static_cast<size_t>(themeIndex)];
    for (auto& c : rows)
        if (auto* themeable = dynamic_cast<Themeable*>(c.get()))
            themeable->setPalette(currentPalette);
    orb.setPalette(currentPalette);
    themeRow.setSelectedIndex(themeIndex);
    repaint();
}

void SerenityAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(currentPalette.bg);
}

void SerenityAudioProcessorEditor::resized()
{
    auto b = getLocalBounds().reduced(10);

    themeRow.setBounds(b.removeFromTop(26));
    b.removeFromTop(8);

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
