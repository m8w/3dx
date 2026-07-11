#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include "Theme.h"
#include <array>
#include <memory>
#include <vector>

// Dark monospace UI in the spirit of the two originals' house style: a
// goniometer + spectrum scope pair on top, scrollable control sections
// (VOICES / SEQUENCE / ROTOR / SHIFT / MEMORY / OUTPUT) below. A THEME row
// lets the whole thing slide from that dark monochrome look to a
// high-contrast funhouse palette (and three stops between).

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

class GoniometerView : public juce::Component, private juce::Timer, public Themeable
{
public:
    explicit GoniometerView(QuaternionAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics& g) override;
    void setPalette(const Palette& p) override { pal = p; repaint(); }

private:
    void timerCallback() override { repaint(); }
    QuaternionAudioProcessor& proc;
    Palette pal = themes()[0];
};

class SpectrumView : public juce::Component, private juce::Timer, public Themeable
{
public:
    explicit SpectrumView(QuaternionAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void setPalette(const Palette& p) override { pal = p; repaint(); }

private:
    void timerCallback() override { repaint(); }
    QuaternionAudioProcessor& proc;
    juce::dsp::FFT fft { 10 }; // 1024-point
    std::array<float, 2048> fftBuf {};
    Palette pal = themes()[0];
};

// One labelled slider row, self-laying-out; height is fixed by the parent.
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

class QuaternionAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit QuaternionAudioProcessorEditor(QuaternionAudioProcessor&);
    ~QuaternionAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    QuaternionAudioProcessor& proc;
    ThemeRow themeRow;
    GoniometerView gonio;
    SpectrumView spectrum;
    Palette currentPalette = themes()[0];

    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<juce::Component>> rows; // owns Param/Choice/SectionHeader rows, in layout order

    void addSection(const juce::String& text);
    void addRow(const juce::String& paramId, const juce::String& text);
    void addChoiceRow(const juce::String& paramId, const juce::String& text);
    void applyPalette(int themeIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuaternionAudioProcessorEditor)
};
