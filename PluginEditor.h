#pragma once
#include "PluginProcessor.h"

// Look des potards : anneau violet/rose lumineux
class DrankLnF : public juce::LookAndFeel_V4
{
public:
    DrankLnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

class DrankEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrankEditor (DrankAudioProcessor&);
    ~DrankEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Bubble { float x, y, r, v, ph; };

    void timerCallback() override;
    void drawCup (juce::Graphics&, juce::Rectangle<float>);
    void stepPreset (int delta);

    DrankAudioProcessor& proc;
    DrankLnF lnf;

    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    std::vector<Bubble> bubbles;
    juce::Random rnd;
    float wavePhase = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrankEditor)
};
