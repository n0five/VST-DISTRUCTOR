#include "PluginEditor.h"

using namespace juce;

// ================================================================ look des potards
DrankLnF::DrankLnF()
{
    setColour (ComboBox::backgroundColourId,  Colour (0xff1c1030));
    setColour (ComboBox::textColourId,        Colour (0xfff0dcff));
    setColour (ComboBox::outlineColourId,     Colour (0x66d946ef));
    setColour (ComboBox::arrowColourId,       Colour (0xffd946ef));
    setColour (PopupMenu::backgroundColourId, Colour (0xff150b24));
    setColour (PopupMenu::textColourId,       Colour (0xfff0dcff));
    setColour (PopupMenu::headerTextColourId, Colour (0xffd946ef));
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff7a2bd0));
    setColour (PopupMenu::highlightedTextColourId,       Colours::white);
    setColour (TextButton::buttonColourId,    Colour (0xff1c1030));
    setColour (TextButton::textColourOffId,   Colour (0xfff0dcff));
    setColour (Slider::textBoxTextColourId,       Colour (0xffe9c7ff));
    setColour (Slider::textBoxOutlineColourId,    Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Label::textColourId,           Colour (0xfff0dcff));
}

void DrankLnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                 float startAngle, float endAngle, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float arcR = r - 3.f;
    const float ang = startAngle + pos * (endAngle - startAngle);

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (Colour (0x33ffffff));
    g.strokePath (track, PathStrokeType (4.f, PathStrokeType::curved, PathStrokeType::rounded));

    Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startAngle, ang, true);
    g.setColour (Colour (0x55d946ef));
    g.strokePath (val, PathStrokeType (10.f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient (Colour (0xffff7ad9), c.x - arcR, c.y,
                                       Colour (0xff8a4dff), c.x + arcR, c.y, false));
    g.strokePath (val, PathStrokeType (4.f, PathStrokeType::curved, PathStrokeType::rounded));

    const float br = r - 12.f;
    g.setGradientFill (ColourGradient (Colour (0xff2e1a4a), c.x, c.y - br,
                                       Colour (0xff0d0717), c.x, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (Colour (0x33ffffff));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.f);

    Path p;
    p.addRoundedRectangle (-1.8f, -br + 3.f, 3.6f, br * 0.45f, 1.5f);
    g.setColour (Colours::white);
    g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
}

// ================================================================ editeur
DrankEditor::DrankEditor (DrankAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setSize (760, 500);

    const char* ids[]   = { "drive", "hard", "fold", "bias", "rate", "bits",
                            "muffle", "wobble", "room", "out", "mix" };
    const char* names[] = { "DRIVE", "HARD CLIP", "FOLD", "ASYM", "CRUSH RATE", "CRUSH BITS",
                            "MUFFLE", "WOBBLE", "ROOM", "OUTPUT", "MIX" };

    for (int i = 0; i < 11; ++i)
    {
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 18);
        k->label.setText (names[i], dontSendNotification);
        k->label.setJustificationType (Justification::centred);
        k->label.setFont (Font (FontOptions (12.f, Font::bold)));
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k->slider);
        knobs.push_back (std::move (k));
    }

    for (int i = 0; i < DrankAudioProcessor::numPresets(); ++i)
    {
        if (i == 0) presetBox.addSectionHeading ("ENCLOSED");
        if (i == DrankAudioProcessor::kEnclosedPresets) presetBox.addSectionHeading ("DISTORTION");
        presetBox.addItem (proc.getProgramName (i), i + 1);
    }
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    for (int i = 0; i < 26; ++i)
        bubbles.push_back ({ rnd.nextFloat() * 760.f, rnd.nextFloat() * 500.f,
                             3.f + rnd.nextFloat() * 10.f, 0.2f + rnd.nextFloat() * 0.6f,
                             rnd.nextFloat() * 6.28f });

    startTimerHz (24);
}

DrankEditor::~DrankEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DrankEditor::stepPreset (int delta)
{
    const int n = DrankAudioProcessor::numPresets();
    const int idx = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram (idx);
    presetBox.setSelectedId (idx + 1, dontSendNotification);
}

void DrankEditor::timerCallback()
{
    wavePhase += 0.12f;
    for (auto& b : bubbles)
    {
        b.y -= b.v;
        b.ph += 0.05f;
        b.x += std::sin (b.ph) * 0.2f;
        if (b.y < -b.r)
        {
            b.y = (float) getHeight() + b.r;
            b.x = rnd.nextFloat() * (float) getWidth();
        }
    }

    const int want = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != want)
        presetBox.setSelectedId (want, dontSendNotification);

    repaint();
}

// ---------------------------------------------------------------- gobelet de lean
void DrankEditor::drawCup (Graphics& g, Rectangle<float> r)
{
    const float cx = r.getCentreX();
    const float topY = r.getY() + r.getHeight() * 0.22f;
    const float botY = r.getBottom();
    const float topW = r.getWidth() * 0.92f;
    const float botW = r.getWidth() * 0.62f;

    Path cup;
    cup.startNewSubPath (cx - topW * 0.5f, topY);
    cup.lineTo (cx + topW * 0.5f, topY);
    cup.lineTo (cx + botW * 0.5f, botY);
    cup.lineTo (cx - botW * 0.5f, botY);
    cup.closeSubPath();

    // paille rose
    g.setColour (Colour (0xffff6fb5));
    g.drawLine (cx + 6.f, topY - 4.f, cx + 24.f, r.getY() - 4.f, 4.f);

    // liquide violet qui ondule
    g.saveState();
    g.reduceClipRegion (cup);
    const float liqY = topY + r.getHeight() * 0.18f;
    Path liq;
    liq.startNewSubPath (cx - topW * 0.5f - 2.f, botY + 2.f);
    liq.lineTo (cx - topW * 0.5f - 2.f, liqY);
    for (float xx = -topW * 0.5f - 2.f; xx <= topW * 0.5f + 2.f; xx += 3.f)
        liq.lineTo (cx + xx, liqY + 2.5f * std::sin (wavePhase + xx * 0.25f));
    liq.lineTo (cx + topW * 0.5f + 2.f, botY + 2.f);
    liq.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xffe35bff), cx, liqY,
                                       Colour (0xff4a1a8c), cx, botY, false));
    g.fillPath (liq);
    g.restoreState();

    // contour + couvercle
    g.setColour (Colour (0xccffffff));
    g.strokePath (cup, PathStrokeType (2.f, PathStrokeType::curved, PathStrokeType::rounded));
    g.fillRoundedRectangle (cx - topW * 0.5f - 3.f, topY - 5.f, topW + 6.f, 6.f, 3.f);
}

// ---------------------------------------------------------------- dessin
void DrankEditor::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (ColourGradient (Colour (0xff200a3d), 0.f, 0.f,
                                       Colour (0xff06020c), 0.f, b.getBottom(), false));
    g.fillAll();

    g.setGradientFill (ColourGradient (Colour (0x55d946ef), 190.f, 50.f,
                                       Colours::transparentBlack, 190.f, 320.f, true));
    g.fillRect (b);

    // bulles qui montent
    for (auto& bb : bubbles)
    {
        g.setColour (Colour (0xffd946ef).withAlpha (0.10f + 0.05f * std::sin (bb.ph)));
        g.fillEllipse (bb.x - bb.r, bb.y - bb.r, bb.r * 2.f, bb.r * 2.f);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.drawEllipse (bb.x - bb.r, bb.y - bb.r, bb.r * 2.f, bb.r * 2.f, 1.f);
    }

    // panneaux
    auto panel = [&g] (Rectangle<int> r, const String& title)
    {
        g.setColour (Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (r.toFloat(), 16.f);
        g.setColour (Colour (0x55d946ef));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 16.f, 1.2f);
        g.setColour (Colour (0xffe9c7ff));
        g.setFont (Font (FontOptions (13.f, Font::bold)));
        g.drawText (title, r.removeFromTop (28).withTrimmedLeft (16), Justification::centredLeft);
    };
    panel ({ 24, 124, 200, 352 },  "ENCLOSED");
    panel ({ 236, 124, 336, 352 }, "DISTORTION");
    panel ({ 584, 124, 152, 352 }, "OUTPUT");

    // gobelet + titre
    drawCup (g, { 24.f, 14.f, 66.f, 92.f });

    const Rectangle<int> titleArea (110, 6, 300, 64);
    g.setFont (Font (FontOptions (54.f, Font::bold)));
    for (int i = 4; i > 0; --i)
    {
        g.setColour (Colour (0xffd946ef).withAlpha (0.07f));
        g.drawText ("DRANK", titleArea.expanded (i * 2), Justification::centredLeft);
    }
    g.setGradientFill (ColourGradient (Colour (0xffff7ad9), 110.f, 10.f,
                                       Colour (0xff8a4dff), 300.f, 70.f, false));
    g.drawText ("DRANK", titleArea, Justification::centredLeft);

    g.setColour (Colour (0xffb9a0d8));
    g.setFont (Font (FontOptions (14.f)));
    g.drawText ("sip slow. distort loud.", Rectangle<int> (112, 68, 320, 22), Justification::centredLeft);
}

void DrankEditor::resized()
{
    prevBtn.setBounds (440, 30, 30, 30);
    presetBox.setBounds (476, 30, 220, 30);
    nextBtn.setBounds (702, 30, 34, 30);

    auto place = [] (Knob& k, Rectangle<int> cell)
    {
        cell = cell.withSizeKeepingCentre (jmin (cell.getWidth(), 112), jmin (cell.getHeight(), 124));
        k.label.setBounds (cell.removeFromTop (16));
        k.slider.setBounds (cell);
    };

    // ENCLOSED : muffle, wobble, room (6,7,8)
    {
        auto r = Rectangle<int> (24, 124, 200, 352).reduced (8);
        r.removeFromTop (24);
        const int rh = r.getHeight() / 3;
        for (int i = 0; i < 3; ++i)
            place (*knobs[(size_t) (6 + i)], r.removeFromTop (rh));
    }

    // DISTORTION : drive, hard, fold / asym, rate, bits (0..5)
    {
        auto r = Rectangle<int> (236, 124, 336, 352).reduced (8);
        r.removeFromTop (24);
        const int rh = r.getHeight() / 2;
        for (int row = 0; row < 2; ++row)
        {
            auto line = r.removeFromTop (rh);
            const int cw = line.getWidth() / 3;
            for (int col = 0; col < 3; ++col)
                place (*knobs[(size_t) (row * 3 + col)], line.removeFromLeft (cw));
        }
    }

    // OUTPUT : out, mix (9,10)
    {
        auto r = Rectangle<int> (584, 124, 152, 352).reduced (8);
        r.removeFromTop (24);
        const int rh = r.getHeight() / 2;
        for (int i = 0; i < 2; ++i)
            place (*knobs[(size_t) (9 + i)], r.removeFromTop (rh));
    }
}
