#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include <memory>
#include <vector>

// Dark monospace UI, same house style as Quaternion's editor: a calm
// "breathing" orb (reacts to output level, no goniometer/spectrum needed
// for a generative source) on top, scrollable control rows below. A THEME
// row lets the whole thing slide from dark monochrome to a high-contrast
// funhouse palette (and three stops between).

class ThemeSwatchButton : public juce::Component
{
public:
    ThemeSwatchButton(const Palette& p, int idx) : palette(p), index(idx) {}
    void paint(juce::Graphics& g) override;
    void mouseUp(const juce::MouseEvent&) override { if (onClick) onClick(index); }
    void setSelected(bool s) { selected = s; repaint(); }

    std::function<void(int)> onClick;

private:
    const Palette& palette;
    int index;
    bool selected = false;
};

class ThemeRow : public juce::Component
{
public:
    ThemeRow();
    void resized() override;
    void setSelectedIndex(int idx);

    std::function<void(int)> onThemeSelected;

private:
    std::vector<std::unique_ptr<ThemeSwatchButton>> swatches;
};

class BreathingOrb : public juce::Component, private juce::Timer, public Themeable
{
public:
    explicit BreathingOrb(SerenityAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics& g) override;
    void setPalette(const Palette& p) override { pal = p; repaint(); }

private:
    void timerCallback() override { repaint(); }
    SerenityAudioProcessor& proc;
    Palette pal = themes()[0];
    float hue = 210.0f;
};

class ParamRow : public juce::Component, public Themeable
{
public:
    ParamRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text);
    void resized() override;
    void setPalette(const Palette& p) override;

private:
    juce::Label label;
    juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ParamChoiceRow : public juce::Component, public Themeable
{
public:
    ParamChoiceRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text);
    void resized() override;
    void setPalette(const Palette& p) override;

private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class SectionHeaderRow : public juce::Component, public Themeable
{
public:
    explicit SectionHeaderRow(const juce::String& text);
    void paint(juce::Graphics& g) override;
    void setPalette(const Palette& p) override { pal = p; repaint(); }

private:
    juce::String text;
    Palette pal = themes()[0];
};

class SerenityAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SerenityAudioProcessorEditor(SerenityAudioProcessor&);
    ~SerenityAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SerenityAudioProcessor& proc;
    ThemeRow themeRow;
    BreathingOrb orb;
    Palette currentPalette = themes()[0];

    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<juce::Component>> rows;

    void addSection(const juce::String& text);
    void addRow(const juce::String& paramId, const juce::String& text);
    void addChoiceRow(const juce::String& paramId, const juce::String& text);
    void applyPalette(int themeIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerenityAudioProcessorEditor)
};
