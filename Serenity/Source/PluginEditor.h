#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <memory>
#include <vector>

// Dark monospace UI, same house style as Quaternion's editor: a calm
// "breathing" orb (reacts to output level, no goniometer/spectrum needed
// for a generative source) on top, scrollable control rows below.

class BreathingOrb : public juce::Component, private juce::Timer
{
public:
    explicit BreathingOrb(SerenityAudioProcessor& p) : proc(p) { startTimerHz(30); }
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override { repaint(); }
    SerenityAudioProcessor& proc;
    float hue = 210.0f;
};

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

class SerenityAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SerenityAudioProcessorEditor(SerenityAudioProcessor&);
    ~SerenityAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SerenityAudioProcessor& proc;
    BreathingOrb orb;

    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<juce::Component>> rows;

    void addSection(const juce::String& text);
    void addRow(const juce::String& paramId, const juce::String& text);
    void addChoiceRow(const juce::String& paramId, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SerenityAudioProcessorEditor)
};
