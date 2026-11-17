#include "WaveformVisualizer.h"
#include <cmath>

WaveformVisualizer::WaveformVisualizer(WaveformBuffer& buffer)
    : waveformBuffer(buffer)
{
    beforeDisplay.resize(4096, 0.0f);
    afterDisplay.resize(4096, 0.0f);
    grDisplay.resize(4096, 0.0f);

    setOpaque(true);
    startTimerHz(60); // 60 FPS smooth rendering
}

WaveformVisualizer::~WaveformVisualizer()
{
    stopTimer();
}

void WaveformVisualizer::setViewMode(ViewMode mode)
{
    viewMode = mode;
    repaint();
}

void WaveformVisualizer::setTimeWindowMs(float ms)
{
    timeWindowMs = juce::jlimit(20.0f, 5000.0f, ms);
}

void WaveformVisualizer::setTriggered(bool triggered)
{
    isTriggered = triggered;
}

void WaveformVisualizer::setFrozen(bool frozen)
{
    isFrozen = frozen;
    waveformBuffer.setFrozen(frozen);
    repaint();
}

void WaveformVisualizer::timerCallback()
{
    int numSamplesToRead = juce::roundToInt((timeWindowMs / 1000.0f) * float(sampleRate));
    numSamplesToRead = juce::jlimit(128, WaveformBuffer::BUFFER_SIZE / 2, numSamplesToRead);

    int pixelWidth = juce::jmax(64, getWidth());
    int points = std::min(pixelWidth * 2, 4096);

    if ((int)beforeDisplay.size() < points)
    {
        beforeDisplay.resize(points, 0.0f);
        afterDisplay.resize(points, 0.0f);
        grDisplay.resize(points, 0.0f);
    }

    std::vector<float> rawBefore(numSamplesToRead);
    std::vector<float> rawAfter(numSamplesToRead);
    std::vector<float> rawGR(numSamplesToRead);

    waveformBuffer.copyLatestSamples(rawBefore.data(), rawAfter.data(), rawGR.data(),
                                     numSamplesToRead, isTriggered && !isFrozen, 0.02f);

    // Decimate or interpolate raw samples into points
    float samplesPerPoint = float(numSamplesToRead) / float(points);

    float maxB = 1e-5f;
    float maxA = 1e-5f;
    double sumSqB = 0.0;
    double sumSqA = 0.0;
    float maxGR = 0.0f;

    for (int p = 0; p < points; ++p)
    {
        int startSample = juce::roundToInt(p * samplesPerPoint);
        int endSample = juce::roundToInt((p + 1) * samplesPerPoint);
        startSample = juce::jlimit(0, numSamplesToRead - 1, startSample);
        endSample = juce::jlimit(startSample + 1, numSamplesToRead, endSample);

        float minValB = 1.0f, maxValB = -1.0f;
        float minValA = 1.0f, maxValA = -1.0f;

        for (int s = startSample; s < endSample; ++s)
        {
            float b = rawBefore[s];
            float a = rawAfter[s];

            if (b < minValB) minValB = b;
            if (b > maxValB) maxValB = b;

            if (a < minValA) minValA = a;
            if (a > maxValA) maxValA = a;

            float absB = std::fabs(b);
            float absA = std::fabs(a);
            if (absB > maxB) maxB = absB;
            if (absA > maxA) maxA = absA;
            sumSqB += double(b * b);
            sumSqA += double(a * a);

            if (rawGR[s] > maxGR) maxGR = rawGR[s];
        }

        // Store peak envelope sample
        beforeDisplay[p] = (std::fabs(maxValB) >= std::fabs(minValB)) ? maxValB : minValB;
        afterDisplay[p] = (std::fabs(maxValA) >= std::fabs(minValA)) ? maxValA : minValA;
        grDisplay[p] = maxGR;
    }

    // Update smoothed meters
    float instPeakB = 20.0f * std::log10(maxB);
    float instPeakA = 20.0f * std::log10(maxA);
    float instRmsB = 20.0f * std::log10(std::max(1e-5f, float(std::sqrt(sumSqB / std::max(1, numSamplesToRead)))));
    float instRmsA = 20.0f * std::log10(std::max(1e-5f, float(std::sqrt(sumSqA / std::max(1, numSamplesToRead)))));

    peakBeforeDb = peakBeforeDb * 0.7f + instPeakB * 0.3f;
    peakAfterDb = peakAfterDb * 0.7f + instPeakA * 0.3f;
    rmsBeforeDb = rmsBeforeDb * 0.7f + instRmsB * 0.3f;
    rmsAfterDb = rmsAfterDb * 0.7f + instRmsA * 0.3f;
    currentGRDb = currentGRDb * 0.7f + maxGR * 0.3f;

    repaint();
}

void WaveformVisualizer::resized()
{
}

void WaveformVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(juce::Colour(0xff0d0f14));

    drawGrid(g, bounds);

    int pixelWidth = juce::jmax(64, getWidth());
    int points = std::min(pixelWidth * 2, 4096);

    switch (viewMode)
    {
        case ViewMode::Overlay:
            drawWaveformOverlay(g, bounds, points);
            break;
        case ViewMode::Split:
            drawWaveformSplit(g, bounds, points);
            break;
        case ViewMode::Delta:
            drawWaveformDelta(g, bounds, points);
            break;
    }

    drawHUD(g, bounds);

    // Outer border
    g.setColour(juce::Colour(0xff2a2d39));
    g.drawRect(bounds, 1.0f);
}

void WaveformVisualizer::drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    float centerY = bounds.getCentreY();
    float height = bounds.getHeight();
    float width = bounds.getWidth();

    // Horizontal amplitude lines: 0 dB (top/bottom), -6 dB, -12 dB, -18 dB
    const float dbLevels[] = { -6.0f, -12.0f, -18.0f };
    const char* dbLabels[] = { "-6 dB", "-12 dB", "-18 dB" };

    g.setFont(juce::Font(10.0f));

    // Center zero line
    g.setColour(juce::Colour(0x33ffffff));
    g.drawHorizontalLine(juce::roundToInt(centerY), bounds.getX(), bounds.getRight());

    float halfH = (height - 30.0f) * 0.5f;

    for (int i = 0; i < 3; ++i)
    {
        float amp = std::pow(10.0f, dbLevels[i] / 20.0f);
        float yPosTop = centerY - amp * halfH;
        float yPosBtm = centerY + amp * halfH;

        g.setColour(juce::Colour(0x18ffffff));
        g.drawHorizontalLine(juce::roundToInt(yPosTop), bounds.getX(), bounds.getRight());
        g.drawHorizontalLine(juce::roundToInt(yPosBtm), bounds.getX(), bounds.getRight());

        g.setColour(juce::Colour(0x66ffffff));
        g.drawText(dbLabels[i], bounds.getX() + 6.0f, yPosTop - 12.0f, 40.0f, 10.0f, juce::Justification::left);
    }

    // Vertical time grid lines
    int numTimeDivs = 8;
    float timeStep = width / float(numTimeDivs);
    for (int t = 1; t < numTimeDivs; ++t)
    {
        float x = bounds.getX() + t * timeStep;
        g.setColour(juce::Colour(0x14ffffff));
        g.drawVerticalLine(juce::roundToInt(x), bounds.getY(), bounds.getBottom());
    }
}

void WaveformVisualizer::drawWaveformOverlay(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints)
{
    if (numPoints <= 1) return;

    float centerY = bounds.getCentreY();
    float halfH = (bounds.getHeight() - 30.0f) * 0.5f;
    float stepX = bounds.getWidth() / float(numPoints - 1);

    // 1. Shaded Transformation Difference (Gain Reduction / Clipping Squashed Area)
    juce::Path diffPath;
    bool hasDiff = false;

    for (int i = 0; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        float b = beforeDisplay[i];
        float a = afterDisplay[i];

        // If Before amplitude is greater than After, compressor or clipper squashed it!
        if (std::fabs(b) > std::fabs(a) + 0.01f)
        {
            float yBefore = centerY - b * halfH;
            float yAfter  = centerY - a * halfH;

            if (!hasDiff)
            {
                diffPath.startNewSubPath(x, yBefore);
                hasDiff = true;
            }
            else
            {
                diffPath.lineTo(x, yBefore);
            }
        }
    }

    // 2. Draw Before Waveform (Dry / Cyan Neon)
    juce::Path beforePath;
    juce::Path beforeFill;
    beforePath.startNewSubPath(bounds.getX(), centerY - beforeDisplay[0] * halfH);
    beforeFill.startNewSubPath(bounds.getX(), centerY);

    for (int i = 0; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        float y = centerY - beforeDisplay[i] * halfH;
        beforePath.lineTo(x, y);
        beforeFill.lineTo(x, y);
    }
    beforeFill.lineTo(bounds.getRight(), centerY);
    beforeFill.closeSubPath();

    // Subtle cyan fill for Before
    g.setColour(juce::Colour(0x1a00e5ff));
    g.fillPath(beforeFill);

    // Neon cyan stroke for Before
    g.setColour(juce::Colour(0x8000e5ff));
    g.strokePath(beforePath, juce::PathStrokeType(1.2f));

    // 3. Draw After Waveform (Wet / Amber Neon)
    juce::Path afterPath;
    juce::Path afterFill;
    afterPath.startNewSubPath(bounds.getX(), centerY - afterDisplay[0] * halfH);
    afterFill.startNewSubPath(bounds.getX(), centerY);

    for (int i = 0; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        float y = centerY - afterDisplay[i] * halfH;
        afterPath.lineTo(x, y);
        afterFill.lineTo(x, y);
    }
    afterFill.lineTo(bounds.getRight(), centerY);
    afterFill.closeSubPath();

    // Warm amber fill for After
    g.setColour(juce::Colour(0x33ffab00));
    g.fillPath(afterFill);

    // Vivid electric amber stroke for After
    g.setColour(juce::Colour(0xffffb300));
    g.strokePath(afterPath, juce::PathStrokeType(2.0f));

    // 4. Highlight Difference Verticals (Red / Crimson for squashed peaks)
    for (int i = 0; i < numPoints; i += 2)
    {
        float b = beforeDisplay[i];
        float a = afterDisplay[i];
        if (std::fabs(b) > std::fabs(a) + 0.02f)
        {
            float x = bounds.getX() + i * stepX;
            float yB = centerY - b * halfH;
            float yA = centerY - a * halfH;
            g.setColour(juce::Colour(0x66ff3366)); // Crimson squash highlight
            g.drawLine(x, yB, x, yA, 1.5f);
        }
    }
}

void WaveformVisualizer::drawWaveformSplit(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints)
{
    if (numPoints <= 1) return;

    float height = bounds.getHeight();
    float paneH = height * 0.48f;
    float centerTop = bounds.getY() + paneH * 0.5f;
    float centerBtm = bounds.getBottom() - paneH * 0.5f;
    float stepX = bounds.getWidth() / float(numPoints - 1);

    // Divider line
    g.setColour(juce::Colour(0x33ffffff));
    g.drawHorizontalLine(juce::roundToInt(bounds.getY() + height * 0.5f), bounds.getX(), bounds.getRight());

    // Top Pane: BEFORE
    juce::Path topPath;
    topPath.startNewSubPath(bounds.getX(), centerTop - beforeDisplay[0] * paneH * 0.45f);
    for (int i = 1; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        topPath.lineTo(x, centerTop - beforeDisplay[i] * paneH * 0.45f);
    }
    g.setColour(juce::Colour(0xff00e5ff));
    g.strokePath(topPath, juce::PathStrokeType(1.5f));

    // Bottom Pane: AFTER
    juce::Path btmPath;
    btmPath.startNewSubPath(bounds.getX(), centerBtm - afterDisplay[0] * paneH * 0.45f);
    for (int i = 1; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        btmPath.lineTo(x, centerBtm - afterDisplay[i] * paneH * 0.45f);
    }
    g.setColour(juce::Colour(0xffffb300));
    g.strokePath(btmPath, juce::PathStrokeType(1.8f));
}

void WaveformVisualizer::drawWaveformDelta(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints)
{
    if (numPoints <= 1) return;

    float centerY = bounds.getCentreY();
    float halfH = (bounds.getHeight() - 30.0f) * 0.5f;
    float stepX = bounds.getWidth() / float(numPoints - 1);

    juce::Path deltaPath;
    float initialDelta = (afterDisplay[0] - beforeDisplay[0]);
    deltaPath.startNewSubPath(bounds.getX(), centerY - initialDelta * halfH);

    for (int i = 1; i < numPoints; ++i)
    {
        float x = bounds.getX() + i * stepX;
        float delta = (afterDisplay[i] - beforeDisplay[i]);
        deltaPath.lineTo(x, centerY - delta * halfH);
    }

    g.setColour(juce::Colour(0xffff4081)); // Pink/Magenta Delta
    g.strokePath(deltaPath, juce::PathStrokeType(2.0f));
}

void WaveformVisualizer::drawHUD(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::bold));

    // Top-Left Badges: BEFORE (Cyan) and AFTER (Amber)
    float badgeY = bounds.getY() + 8.0f;

    // BEFORE Badge
    juce::Rectangle<float> beforeBadge(bounds.getX() + 10.0f, badgeY, 190.0f, 22.0f);
    g.setColour(juce::Colour(0xdd121a24));
    g.fillRoundedRectangle(beforeBadge, 4.0f);
    g.setColour(juce::Colour(0xaa00e5ff));
    g.drawRoundedRectangle(beforeBadge, 4.0f, 1.0f);

    juce::String bText = "PRE (DRY): " + juce::String(peakBeforeDb, 1) + " dB | RMS: " + juce::String(rmsBeforeDb, 1) + " dB";
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawText(bText, beforeBadge, juce::Justification::centred);

    // AFTER Badge
    juce::Rectangle<float> afterBadge(beforeBadge.getRight() + 8.0f, badgeY, 190.0f, 22.0f);
    g.setColour(juce::Colour(0xdd241b12));
    g.fillRoundedRectangle(afterBadge, 4.0f);
    g.setColour(juce::Colour(0xaaffb300));
    g.drawRoundedRectangle(afterBadge, 4.0f, 1.0f);

    juce::String aText = "POST (WET): " + juce::String(peakAfterDb, 1) + " dB | RMS: " + juce::String(rmsAfterDb, 1) + " dB";
    g.setColour(juce::Colour(0xffffb300));
    g.drawText(aText, afterBadge, juce::Justification::centred);

    // GAIN REDUCTION Badge (if GR > 0.1 dB)
    if (currentGRDb > 0.1f)
    {
        juce::Rectangle<float> grBadge(afterBadge.getRight() + 8.0f, badgeY, 110.0f, 22.0f);
        g.setColour(juce::Colour(0xdd2e1218));
        g.fillRoundedRectangle(grBadge, 4.0f);
        g.setColour(juce::Colour(0xaaff3366));
        g.drawRoundedRectangle(grBadge, 4.0f, 1.0f);

        juce::String grText = "GR: -" + juce::String(currentGRDb, 1) + " dB";
        g.setColour(juce::Colour(0xffff3366));
        g.drawText(grText, grBadge, juce::Justification::centred);
    }

    // Top-Right: FROZEN or LIVE indicator
    juce::Rectangle<float> statusBadge(bounds.getRight() - 120.0f, badgeY, 110.0f, 22.0f);
    if (isFrozen)
    {
        g.setColour(juce::Colour(0xee4a154b));
        g.fillRoundedRectangle(statusBadge, 4.0f);
        g.setColour(juce::Colour(0xffff55ff));
        g.drawRoundedRectangle(statusBadge, 4.0f, 1.2f);
        g.drawText("FROZEN", statusBadge, juce::Justification::centred);
    }
    else
    {
        g.setColour(juce::Colour(0xaa14231a));
        g.fillRoundedRectangle(statusBadge, 4.0f);
        g.setColour(juce::Colour(0xaa00e676));
        g.drawRoundedRectangle(statusBadge, 4.0f, 1.0f);
        g.drawText("LIVE STREAM", statusBadge, juce::Justification::centred);
    }

    // Bottom-Right: Time Window
    juce::String winText = "Window: " + juce::String(juce::roundToInt(timeWindowMs)) + " ms";
    g.setColour(juce::Colour(0x88ffffff));
    g.drawText(winText, bounds.getRight() - 130.0f, bounds.getBottom() - 22.0f, 120.0f, 16.0f, juce::Justification::right);
}
