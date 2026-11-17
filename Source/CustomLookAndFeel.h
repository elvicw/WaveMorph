#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff12141c));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffffb300));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffffb300));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff252936));
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe0e4f0));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, juce::Colour(0xff9ea8c2));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();

        // Background Track Arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff222634));
        g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Dial Body
        float dialRadius = radius - 8.0f;
        juce::ColourGradient dialGrad(juce::Colour(0xff232836), centre.x - dialRadius, centre.y - dialRadius,
                                     juce::Colour(0xff161822), centre.x + dialRadius, centre.y + dialRadius, false);
        g.setGradientFill(dialGrad);
        g.fillEllipse(centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

        g.setColour(juce::Colour(0xff333a4d));
        g.drawEllipse(centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.2f);

        // Active Arc
        if (slider.isEnabled())
        {
            juce::Path activeArc;
            activeArc.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f,
                                    rotaryStartAngle, toAngle, true);

            juce::Colour arcColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
            g.setColour(arcColour);
            g.strokePath(activeArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Pointer indicator
        juce::Path pointer;
        float pointerLength = dialRadius * 0.75f;
        float thumbWidth = 2.5f;
        pointer.addRoundedRectangle(-thumbWidth * 0.5f, -dialRadius, thumbWidth, pointerLength, 1.0f);
        pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));

        g.setColour(juce::Colour(0xffffffff));
        g.fillPath(pointer);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 4.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

        auto baseColour = backgroundColour;
        if (button.getToggleState())
            baseColour = baseColour.brighter(0.2f);
        if (shouldDrawButtonAsDown)
            baseColour = baseColour.darker(0.15f);
        else if (shouldDrawButtonAsHighlighted)
            baseColour = baseColour.brighter(0.15f);

        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(button.getToggleState() ? juce::Colour(0xff00e5ff) : baseColour.brighter(0.25f));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
};
