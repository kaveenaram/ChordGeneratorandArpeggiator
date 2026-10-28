/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    const juce::Colour backgroundColour { 0xFFDDC4F1 };
    const juce::Colour inkColour        { 0xFF4C365E };
    const juce::Colour selectedColour   { 0xFF6C4C8D };
    const juce::Colour buttonColour     { 0xFFFFF9FF };
    const juce::Colour hoverColour      { 0xFFECE0F8 };
    const juce::Colour borderColour     { 0xFFB89CD4 };

    const juce::String headingFont = "Helvetica Neue";
    const juce::String bodyFont = "Helvetica";
}

// CUSTOM LOOK AND FEEL

void PluginLookAndFeel::drawToggleButton(
    juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    const bool isSelected = button.getToggleState();

    juce::Colour fill = isSelected ? selectedColour : buttonColour;

    if (shouldDrawButtonAsHighlighted && !isSelected)
        fill = hoverColour;

    if (shouldDrawButtonAsDown)
        fill = fill.darker(0.06f);

    g.setColour(fill);
    g.fillRoundedRectangle(bounds, 9.0f);

    g.fillRoundedRectangle(bounds, 12.0f);
    g.drawRoundedRectangle(bounds, 12.0f, 1.2f);

    g.setColour(isSelected ? juce::Colours::white : inkColour);

    // Use a slightly larger font for the toggle labels so they are easier to read.
    g.setFont(juce::Font(bodyFont, 18.0f, juce::Font::bold));

    g.drawFittedText(
        button.getButtonText(),
        button.getLocalBounds().reduced(5, 2),
        juce::Justification::centred,
        1);
}

//==============================================================================
// CUSTOM ARP SPEED SLIDER

void PluginLookAndFeel::drawLinearSlider(
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPos,
    float minSliderPos,
    float maxSliderPos,
    juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider(
            g, x, y, width, height,
            sliderPos, minSliderPos, maxSliderPos,
            style, slider);
        return;
    }

    const float trackWidth = 7.0f;
    const float centreX = static_cast<float>(x) + width * 0.5f;
    const float top = static_cast<float>(y) + 10.0f;
    const float bottom = static_cast<float>(y + height) - 10.0f;

    const float trackHeight = juce::jmax(1.0f, bottom - top);
    const float thumbY = juce::jlimit(top, bottom, sliderPos);

    g.setColour(buttonColour);
    g.fillRoundedRectangle(
        centreX - trackWidth * 0.5f,
        top,
        trackWidth,
        trackHeight,
        trackWidth * 0.5f);

    g.setColour(selectedColour);
    g.fillRoundedRectangle(
        centreX - trackWidth * 0.5f,
        thumbY,
        trackWidth,
        juce::jmax(0.0f, bottom - thumbY),
        trackWidth * 0.5f);

    constexpr float thumbRadius = 11.0f;

    g.setColour(selectedColour);
    g.fillEllipse(
        centreX - thumbRadius,
        thumbY - thumbRadius,
        thumbRadius * 2.0f,
        thumbRadius * 2.0f);

    g.setColour(buttonColour);
    g.drawEllipse(
        centreX - thumbRadius,
        thumbY - thumbRadius,
        thumbRadius * 2.0f,
        thumbRadius * 2.0f,
        2.0f);
}

//==============================================================================
// VERTICAL SWITCH

VerticalSwitch::VerticalSwitch(
    const juce::String& topLabel,
    const juce::String& bottomLabel)
    : topText(topLabel),
      bottomText(bottomLabel)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void VerticalSwitch::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    // Outer switch body.
    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, 15.0f);

    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, 15.0f, 1.0f);

    g.setFont(juce::Font(bodyFont, 14.0f, juce::Font::bold));

    g.setColour(topSelected ? selectedColour : inkColour);
    g.drawFittedText(
        topText,
        getLocalBounds().removeFromTop(37).reduced(3, 1),
        juce::Justification::centred,
        1);

    g.setColour(!topSelected ? selectedColour : inkColour);
    g.drawFittedText(
        bottomText,
        juce::Rectangle<int>(
            3,
            getHeight() - 37,
            getWidth() - 6,
            34),
        juce::Justification::centred,
        1);

    const float trackWidth = 16.0f;
    const float trackHeight = juce::jmax(
        30.0f, static_cast<float>(getHeight()) - 88.0f);

    const float trackX =
        (static_cast<float>(getWidth()) - trackWidth) * 0.5f;

    const float trackY =
        (static_cast<float>(getHeight()) - trackHeight) * 0.5f;

    g.setColour(selectedColour);
    g.fillRoundedRectangle(
        trackX, trackY, trackWidth, trackHeight, trackWidth * 0.5f);

    const float thumbSize = 25.0f;
    const float thumbX =
        (static_cast<float>(getWidth()) - thumbSize) * 0.5f;

    const float thumbY = topSelected
        ? trackY + 3.0f
        : trackY + trackHeight - thumbSize - 3.0f;

    g.setColour(buttonColour);
    g.fillEllipse(thumbX, thumbY, thumbSize, thumbSize);

    g.setColour(borderColour);
    g.drawEllipse(thumbX, thumbY, thumbSize, thumbSize, 1.2f);
}

void VerticalSwitch::mouseDown(const juce::MouseEvent& event)
{
    const bool selectTop = event.position.y < getHeight() * 0.5f;

    if (selectTop != topSelected)
    {
        topSelected = selectTop;
        repaint();

        if (onStateChange)
            onStateChange(topSelected);
    }
}

void VerticalSwitch::setTopSelected(bool shouldSelectTop)
{
    if (topSelected != shouldSelectTop)
    {
        topSelected = shouldSelectTop;
        repaint();
    }
}

bool VerticalSwitch::isTopSelected() const
{
    return topSelected;
}

//==============================================================================
KaveenaRamkissoonAssignment1Editor::KaveenaRamkissoonAssignment1Editor (KaveenaRamkissoonAssignment1& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1200, 600);
    
    // ARP SPEED SLIDER
    rateSlider.setSliderStyle(juce::Slider::LinearVertical);
    rateSlider.setRange(0.0, 1.0, 0.05);
    rateSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 24);
    rateSlider.setPopupDisplayEnabled(true, false, this);
    rateSlider.setTextValueSuffix(" rate");
    rateSlider.setNumDecimalPlacesToDisplay(2);
    
    rateSlider.setColour(juce::Slider::textBoxTextColourId, inkColour);
    rateSlider.setColour(juce::Slider::textBoxBackgroundColourId, backgroundColour);
    rateSlider.setColour(juce::Slider::textBoxOutlineColourId, backgroundColour);
            
    rateSlider.setLookAndFeel(&pluginLookAndFeel);
    addAndMakeVisible(rateSlider);

    rateSlider.setValue(audioProcessor.getFloatParameter("arpSpeed"), juce::dontSendNotification);

    rateSlider.onValueChange = [this]() {
        audioProcessor.setFloatParameter("arpSpeed", static_cast<float>(rateSlider.getValue()));
    };

    // RADIO BUTTON
    
    auto addChoice = [this](juce::ToggleButton& button, const juce::String& text, const juce::String& parameterID, int index, int group){
        button.setButtonText(text);
        button.setRadioGroupId(group);
        button.setClickingTogglesState(true);
        button.setLookAndFeel(&pluginLookAndFeel);

        auto* buttonPtr = &button;

        button.onClick = [this, buttonPtr, parameterID, index]{
            if (buttonPtr->getToggleState())
                audioProcessor.setChoiceParameter(parameterID, index);
        };

        addAndMakeVisible(button);
    };
    
    
    addChoice(modeOFFButton, "OFF", "mode", 0, 1001);
    addChoice(modeCHORDButton, "CHORD", "mode", 1, 1001);
    addChoice(modeCHORDARPButton, "CHORD + ARP", "mode", 2, 1001);
    
    addChoice(chordMAJButton, "MAJ", "chord", 0, 1002);
    addChoice(chordMINButton, "MIN", "chord", 1, 1002);
    addChoice(chordADD9Button, "ADD9", "chord", 2, 1002);

    addChoice(pattern1Button, "1", "pattern", 0, 1003);
    addChoice(pattern2Button, "2", "pattern", 1, 1003);
    addChoice(pattern3Button, "3", "pattern", 2, 1003);

    addChoice(octaveButton1, "+1", "octaves", 1, 1005);
    addChoice(octaveButton2, "+2", "octaves", 2, 1005);
    addChoice(octaveButton3, "+3", "octaves", 3, 1005);

    addAndMakeVisible(latchSwitch);

    latchSwitch.onStateChange = [this](bool topSelected){
        // Top = OFF; bottom = ON.
        audioProcessor.setBooleanParameter("latch", !topSelected);
    };

 
    addAndMakeVisible(orderSwitch);

    orderSwitch.onStateChange = [this](bool topSelected){
        // Top = ascending (1); bottom = descending (0).
        audioProcessor.setChoiceParameter("order", topSelected ? 1 : 0);
    };

    timerCallback();
    startTimerHz(20);
}


void KaveenaRamkissoonAssignment1Editor::setChoiceParameter(const juce::String& parameterID, int choiceIndex){
    audioProcessor.setChoiceParameter(parameterID, choiceIndex);
}

int KaveenaRamkissoonAssignment1Editor::getChoiceParameter(const juce::String& parameterID) const {
    return audioProcessor.getChoiceParameter(parameterID);
}

void KaveenaRamkissoonAssignment1Editor::setBooleanParameter(const juce::String& parameterID, bool newValue){
    audioProcessor.setBooleanParameter(parameterID, newValue);
}

bool KaveenaRamkissoonAssignment1Editor::getBooleanParameter(const juce::String& parameterID) const{
    return audioProcessor.getBooleanParameter(parameterID);
}

void KaveenaRamkissoonAssignment1Editor::timerCallback()
{
    const auto syncChoice =
        [](juce::ToggleButton& button, bool shouldBeOn)
    {
        button.setToggleState(
            shouldBeOn, juce::dontSendNotification);
    };

    const int mode = getChoiceParameter("mode");

    syncChoice(modeOFFButton, mode == 0);
    syncChoice(modeCHORDButton, mode == 1);
    syncChoice(modeCHORDARPButton, mode == 2);

    const int chord = getChoiceParameter("chord");

    syncChoice(chordMAJButton, chord == 0);
    syncChoice(chordMINButton, chord == 1);
    syncChoice(chordADD9Button, chord == 2);

    const int pattern = getChoiceParameter("pattern");

    syncChoice(pattern1Button, pattern == 0);
    syncChoice(pattern2Button, pattern == 1);
    syncChoice(pattern3Button, pattern == 2);

    const int octave = getChoiceParameter("octaves");

    syncChoice(octaveButton1, octave == 1);
    syncChoice(octaveButton2, octave == 2);
    syncChoice(octaveButton3, octave == 3);

    // Latch: OFF is at the top; ON is at the bottom.
    latchSwitch.setTopSelected(
        !getBooleanParameter("latch"));

    // Order: ascending is at the top; descending is at the bottom.
    orderSwitch.setTopSelected(
        getChoiceParameter("order") == 1);
}

KaveenaRamkissoonAssignment1Editor::~KaveenaRamkissoonAssignment1Editor()
{
    rateSlider.setLookAndFeel(nullptr);

    modeOFFButton.setLookAndFeel(nullptr);
    modeCHORDButton.setLookAndFeel(nullptr);
    modeCHORDARPButton.setLookAndFeel(nullptr);

    chordMAJButton.setLookAndFeel(nullptr);
    chordMINButton.setLookAndFeel(nullptr);
    chordADD9Button.setLookAndFeel(nullptr);

    octaveButton1.setLookAndFeel(nullptr);
    octaveButton2.setLookAndFeel(nullptr);
    octaveButton3.setLookAndFeel(nullptr);

    pattern1Button.setLookAndFeel(nullptr);
    pattern2Button.setLookAndFeel(nullptr);
    pattern3Button.setLookAndFeel(nullptr);
}

// THIS HANDLES ANY ON SCREEN GUI CONTROLS OR VISUALISATIONS
//==============================================================================

void KaveenaRamkissoonAssignment1Editor::paint(juce::Graphics& g)
{
    const int margin = 24;
    const int headerHeight = 82;
    const int contentY = 100;
    const int contentHeight = getHeight() - contentY - margin;

    const int leftWidth = 230;
    const int rightWidth = 130;
    const int panelGap = 16;

    const int leftX = margin;
    const int centerX = leftX + leftWidth + panelGap;
    const int rightX = getWidth() - margin - rightWidth;
    const int centerWidth = rightX - panelGap - centerX;

    // Main background
    g.fillAll(backgroundColour);

    // Header
    g.setColour(inkColour);
    g.setFont(juce::Font(headingFont, 30.0f, juce::Font::bold));
    g.drawFittedText(
        "CHORD GENERATOR + ARPEGGIATOR",
        margin, 12, getWidth() - margin * 2, 46,
        juce::Justification::centred, 1);

    g.setColour(borderColour);
    g.fillRoundedRectangle(
        getWidth() * 0.40f, 66.0f,
        getWidth() * 0.20f, 3.0f, 1.5f);

    const juce::Rectangle<float> leftPanel(
        (float) leftX, (float) contentY,
        (float) leftWidth, (float) contentHeight);

    const juce::Rectangle<float> centerPanel(
        (float) centerX, (float) contentY,
        (float) centerWidth, (float) contentHeight);

    const juce::Rectangle<float> rightPanel(
        (float) rightX, (float) contentY,
        (float) rightWidth, (float) contentHeight);

    const auto panelColour = backgroundColour.brighter(0.12f);

    for (const auto& panel : { leftPanel, centerPanel, rightPanel })
    {
        g.setColour(panelColour);
        g.fillRoundedRectangle(panel, 16.0f);

        g.setColour(borderColour.withAlpha(0.65f));
        g.drawRoundedRectangle(panel, 16.0f, 1.0f);
    }

    // Section heading style
    g.setColour(inkColour);
    g.setFont(juce::Font(headingFont, 15.0f, juce::Font::bold));

    // Left panel headings
    g.drawFittedText(
        "ARP SPEED",
        leftX + 8, contentY + 15, 78, 24,
        juce::Justification::centred, 1);

    g.drawFittedText(
        "OCTAVE RANGE",
        leftX + 88, contentY + 15, leftWidth - 96, 24,
        juce::Justification::centred, 1);

    // Center section headings
    const int labelHeight = 24;

    auto drawRowHeading = [&](const juce::String& text,
                              juce::Component& first,
                              juce::Component& last)
    {
        const int rowLeft = first.getX();
        const int rowRight = last.getRight();

        g.drawFittedText(
            text,
            rowLeft, first.getY() - 29,
            rowRight - rowLeft, labelHeight,
            juce::Justification::centred, 1);
    };

    drawRowHeading("MODE", modeOFFButton, modeCHORDARPButton);
    drawRowHeading("CHORD TYPE", chordMAJButton, chordADD9Button);
    drawRowHeading("PATTERN", pattern1Button, pattern3Button);

    // Right panel headings
    g.drawFittedText(
        "LATCH",
        rightX + 4, contentY + 15, rightWidth - 8, 24,
        juce::Justification::centred, 1);

    g.drawFittedText(
        "ORDER",
        rightX + 4, contentY + contentHeight / 2 + 5,
        rightWidth - 8, 24,
        juce::Justification::centred, 1);
}


void KaveenaRamkissoonAssignment1Editor::resized()
{
    const int margin = 24;
    const int contentY = 100;
    const int contentBottom = getHeight() - margin;
    const int contentHeight = juce::jmax(300, contentBottom - contentY);

    // Three-column layout
    const int leftWidth = 230;
    const int rightWidth = 130;
    const int panelGap = 16;

    const int leftX = margin;
    const int centerX = leftX + leftWidth + panelGap;
    const int rightX = getWidth() - margin - rightWidth;
    const int centerWidth = rightX - panelGap - centerX;

    // --------------------------------------------------
    // LEFT COLUMN: speed and octave range
    // --------------------------------------------------

    const int leftInnerX = leftX + 12;
    const int sliderWidth = 48;
    const int sliderY = contentY + 54;
    const int sliderHeight = contentHeight - 100;

    rateSlider.setBounds(
        leftInnerX + 4,
        sliderY,
        sliderWidth,
        sliderHeight);

    const int octaveX = leftX + 82;
    const int octaveWidth = leftWidth - 98;
    const int octaveHeight = 48;
    const int octaveGap = 14;
    const int octaveStartY = contentY + 58;

    octaveButton1.setBounds(
        octaveX, octaveStartY, octaveWidth, octaveHeight);

    octaveButton2.setBounds(
        octaveX, octaveStartY + octaveHeight + octaveGap,
        octaveWidth, octaveHeight);

    octaveButton3.setBounds(
        octaveX, octaveStartY + 2 * (octaveHeight + octaveGap),
        octaveWidth, octaveHeight);

    // --------------------------------------------------
    // CENTER COLUMN: three rows of three buttons
    // --------------------------------------------------

    const int innerMargin = 22;
    const int buttonGap = 14;
    const int buttonWidth = juce::jmax(
        70, (centerWidth - 2 * innerMargin - 2 * buttonGap) / 3);

    const int groupWidth = 3 * buttonWidth + 2 * buttonGap;
    const int groupX = centerX + (centerWidth - groupWidth) / 2;

    const int buttonHeight = 62;
    const int firstRowY = contentY + 65;
    const int availableRowsHeight = contentHeight - 105;

    // Space the rows evenly while preserving room for headings.
    const int rowGap = juce::jmax(
        24, (availableRowsHeight - 3 * buttonHeight) / 2);

    auto placeRow = [&](int y,
                        juce::ToggleButton& a,
                        juce::ToggleButton& b,
                        juce::ToggleButton& c)
    {
        a.setBounds(groupX, y, buttonWidth, buttonHeight);
        b.setBounds(groupX + buttonWidth + buttonGap,
                    y, buttonWidth, buttonHeight);
        c.setBounds(groupX + 2 * (buttonWidth + buttonGap),
                    y, buttonWidth, buttonHeight);
    };

    placeRow(firstRowY,
             modeOFFButton, modeCHORDButton, modeCHORDARPButton);

    placeRow(firstRowY + buttonHeight + rowGap,
             chordMAJButton, chordMINButton, chordADD9Button);

    placeRow(firstRowY + 2 * (buttonHeight + rowGap),
             pattern1Button, pattern2Button, pattern3Button);

    // --------------------------------------------------
    // RIGHT COLUMN: latch and order switches
    // --------------------------------------------------

    const int switchWidth = 86;
    const int switchHeight = 145;
    const int switchX = rightX + (rightWidth - switchWidth) / 2;

    const int latchY = contentY + 52;
    latchSwitch.setBounds(
        switchX, latchY, switchWidth, switchHeight);

    const int orderY = contentY + contentHeight / 2 + 30;
    orderSwitch.setBounds(
        switchX, orderY, switchWidth, switchHeight);
}




