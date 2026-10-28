/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================

class PluginLookAndFeel : public juce::LookAndFeel_V4{
public:
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider) override;
};

class VerticalSwitch : public juce::Component{
public:
    VerticalSwitch(const juce::String& topLabel, const juce::String& bottomLabel);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setTopSelected(bool shouldSelectTop);
    bool isTopSelected() const;

    std::function<void(bool)> onStateChange;

private:
    juce::String topText;
    juce::String bottomText;

    bool topSelected = true;
};


class KaveenaRamkissoonAssignment1Editor  : public juce::AudioProcessorEditor, private juce::Timer {
public:
    KaveenaRamkissoonAssignment1Editor (KaveenaRamkissoonAssignment1&);
    ~KaveenaRamkissoonAssignment1Editor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    KaveenaRamkissoonAssignment1& audioProcessor;
    
    PluginLookAndFeel pluginLookAndFeel;
    
    // rate slider

    juce::Slider rateSlider; // [1]
    
    // mode RADIO BUTTONS
    juce::ToggleButton modeOFFButton;
    juce::ToggleButton modeCHORDButton;
    juce::ToggleButton modeCHORDARPButton;
    
    // chord RADIO BUTTONS
    
    juce::ToggleButton chordMAJButton;
    juce::ToggleButton chordMINButton;
    juce::ToggleButton chordADD9Button;
    
    // octave RADIO BUTTONS
    
    juce::ToggleButton octaveButton1;
    juce::ToggleButton octaveButton2;
    juce::ToggleButton octaveButton3;
    
    // pattern RADIO BUTTONS
    
    juce::ToggleButton pattern1Button;
    juce::ToggleButton pattern2Button;
    juce::ToggleButton pattern3Button;
    
    // switches
    
    VerticalSwitch latchSwitch {"OFF", "ON"};
    VerticalSwitch orderSwitch {"ASCENDING", "DESCENDING"};
    
    std::unique_ptr<SliderAttachment> arpSpeedAttachment;

    void setChoiceParameter(const juce::String& parameterID, int choiceIndex);
    int getChoiceParameter(const juce::String& parameterID) const;
    void setBooleanParameter(const juce::String& parameterID, bool newValue);
    bool getBooleanParameter(const juce::String& parameterID) const;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KaveenaRamkissoonAssignment1Editor)
};
