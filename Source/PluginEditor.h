#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

// Dark monospace UI in the spirit of the two originals' house style: a
// goniometer + spectrum scope pair on top, scrollable control sections
// (VOICES / SEQUENCE / ROTOR / SHIFT / MEMORY / OUTPUT) below.

class GoniometerView : public juce::Component, private juce::Timer
{
public:
    explicit GoniometerView(QuaternionAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }
    QuaternionAudioProcessor& proc;
};

class SpectrumView : public juce::Component, private juce::Timer
{
public:
    explicit SpectrumView(QuaternionAudioProcessor& p);
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }
    QuaternionAudioProcessor& proc;
    juce::dsp::FFT fft { 10 }; // 1024-point
    std::array<float, 2048> fftBuf {};
};

// One labelled slider row, self-laying-out; height is fixed by the parent.
class ParamRow : public juce::Component
{
public:
    ParamRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text);
    void resized() override;

private:
    juce::Label label;
    juce::Slider slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ParamChoiceRow : public juce::Component
{
public:
    ParamChoiceRow(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& text);
    void resized() override;

private:
    juce::Label label;
    juce::ComboBox box;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class SectionHeaderRow : public juce::Component
{
public:
    explicit SectionHeaderRow(const juce::String& text);
    void paint(juce::Graphics& g) override;

private:
    juce::String text;
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
    GoniometerView gonio;
    SpectrumView spectrum;

    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<juce::Component>> rows; // owns Param/Choice/SectionHeader rows, in layout order

    void addSection(const juce::String& text);
    void addRow(const juce::String& paramId, const juce::String& text);
    void addChoiceRow(const juce::String& paramId, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuaternionAudioProcessorEditor)
};
