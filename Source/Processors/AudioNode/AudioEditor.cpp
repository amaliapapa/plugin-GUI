/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "AudioEditor.h"
#include "../../Audio/AudioComponent.h"
#include "../../AccessClass.h"

MuteButton::MuteButton()
    : ImageButton ("MuteButton")
{
    Image offimage = ImageCache::getFromMemory  (BinaryData::muteoff_png, BinaryData::muteoff_pngSize);
    Image onimage = ImageCache::getFromMemory   (BinaryData::muteon_png,  BinaryData::muteon_pngSize);

    setImages (false, true, true,
               offimage, 1.0f, Colours::white.withAlpha (0.0f),
               offimage, 1.0f, Colours::black.withAlpha (0.0f),
               onimage,  1.0f, Colours::white.withAlpha (0.0f));

    setClickingTogglesState (true);

    setTooltip ("Mute audio");
}


MuteButton::~MuteButton()
{
}


AudioWindowButton::AudioWindowButton()
    : Button ("AudioWindowButton")
{
    setClickingTogglesState (true);

    font = Font ("Small Text", 12, Font::plain);
    textString = "AUDIO";
    setTooltip ("Change the buffer size");
}


AudioWindowButton::~AudioWindowButton()
{
}


void AudioWindowButton::paintButton (Graphics& g, bool isMouseOver, bool isButtonDown)
{
    if (getToggleState())
        g.setColour (Colours::yellow);
    else
        g.setColour (Colours::lightgrey);

    g.setFont (font);
    g.drawSingleLineText (textString, 0, 15);
}


void AudioWindowButton::setText (const String& newText)
{
    textString = newText;
    repaint();
}


AudioEditor::AudioEditor (AudioNode* owner)
    : AudioProcessorEditor (owner)
    , lastValue                 (1.0f)
    , isEnabled                 (true)
    , audioConfigurationWindow  (nullptr)
{
    muteButton = new MuteButton();
    muteButton->addListener (this);
    muteButton->setToggleState (false, dontSendNotification);
    addAndMakeVisible (muteButton);

    audioWindowButton = new AudioWindowButton();
    audioWindowButton->addListener (this);
    audioWindowButton->setToggleState (false, dontSendNotification);
    addAndMakeVisible (audioWindowButton);

    volumeSlider = new Slider ("Volume Slider");
    volumeSlider->setRange (0,100,1);
    volumeSlider->addListener (this);
    volumeSlider->setTextBoxStyle (Slider::NoTextBox,
                                   false, 0, 0);
    volumeSlider->setColour (Slider::trackColourId, Colours::yellow);
    addAndMakeVisible (volumeSlider);

    noiseGateSlider = new Slider ("Noise Gate Slider");
    noiseGateSlider->setRange (0,100,1);
    noiseGateSlider->addListener (this);
    noiseGateSlider->setTextBoxStyle (Slider::NoTextBox,
                                      false, 0, 0);
    addAndMakeVisible (noiseGateSlider);
}


AudioEditor::~AudioEditor()
{
}


void AudioEditor::resized()
{
    const int height        = getHeight();
    const int sliderWidth   = 50;
    const int sliderHeight  = height - 5;

    muteButton->setBounds           (0, 5, 30, 25);
    volumeSlider->setBounds         (35, 8, sliderWidth, sliderHeight);
    noiseGateSlider->setBounds      (85, 8, sliderWidth, sliderHeight);
    audioWindowButton->setBounds    (140, 5, 200, height);
}


bool AudioEditor::keyPressed (const KeyPress& key)
{
    //std::cout << name << " received " << key.getKeyCode() << std::endl;
    return false;
}


void AudioEditor::updateBufferSizeText()
{
    String t = String (AccessClass::getAudioComponent()->getBufferSizeMs());
    t += " ms";

    audioWindowButton->setText (t);
}


void AudioEditor::enable()
{
    isEnabled = true;
    audioWindowButton->setClickingTogglesState (true);
}


void AudioEditor::disable()
{
    isEnabled = false;

    if (audioConfigurationWindow)
    {
        audioConfigurationWindow->setVisible (false);
        audioWindowButton->setToggleState (false, dontSendNotification);
    }

    audioWindowButton->setClickingTogglesState (false);
}


void AudioEditor::buttonClicked (Button* button)
{
    if (button == muteButton)
    {
        if (muteButton->getToggleState())
        {
            lastValue = volumeSlider->getValue();
            getAudioProcessor()->setParameter (1,0.0f);
            std::cout << "Mute on." << std::endl;
        }
        else
        {
            getAudioProcessor()->setParameter (1,lastValue);
            std::cout << "Mute off." << std::endl;
        }
    }
    else if (button == audioWindowButton && isEnabled)
    {
        if (audioWindowButton->getToggleState())
        {
            if (! audioConfigurationWindow)
            {
                audioConfigurationWindow = new AudioConfigurationWindow (AccessClass::getAudioComponent()->deviceManager,
                                                                         audioWindowButton);
            }

            AccessClass::getAudioComponent()->restartDevice();
            audioConfigurationWindow->setVisible (true);
        }
        else
        {
            updateBufferSizeText();
            audioConfigurationWindow->setVisible (false);
            AccessClass::getAudioComponent()->stopDevice();
        }
    }

}


void AudioEditor::sliderValueChanged (Slider* slider)
{
    if (slider == volumeSlider)
        getAudioProcessor()->setParameter (1, slider->getValue());
    else if (slider == noiseGateSlider)
        getAudioProcessor()->setParameter (2, slider->getValue());
}


void AudioEditor::paint (Graphics& g)
{
    g.setColour (Colours::grey);
    g.setFont (10);
    g.drawText ("VOLUME:", 40, 1, 50, 10, Justification::left, false);
    g.drawText ("GATE:", 90, 1, 50, 10, Justification::left, false);
}


void AudioEditor::saveStateToXml (XmlElement* xml)
{
    XmlElement* audioEditorState = xml->createNewChildElement ("AUDIOEDITOR");
    audioEditorState->setAttribute ("isMuted",   muteButton->getToggleState());
    audioEditorState->setAttribute ("volume",    volumeSlider->getValue());
    audioEditorState->setAttribute ("noiseGate", noiseGateSlider->getValue());
}


void AudioEditor::loadStateFromXml (XmlElement* xml)
{
    forEachXmlChildElement (*xml, xmlNode)
    {
        if (xmlNode->hasTagName ("AUDIOEDITOR"))
        {
            muteButton->setToggleState  (xmlNode->getBoolAttribute ("isMuted", false), dontSendNotification);

            volumeSlider->setValue    (xmlNode->getDoubleAttribute ("volume",    0.0f), NotificationType::sendNotification);
            noiseGateSlider->setValue (xmlNode->getDoubleAttribute ("noiseGate", 0.0f), NotificationType::sendNotification);
        }
    }

    updateBufferSizeText();
}


AudioConfigurationWindow::AudioConfigurationWindow (AudioDeviceManager& adm, AudioWindowButton* cButton)
    : DocumentWindow ("Audio Settings",
                      Colours::red,
                      DocumentWindow::closeButton)
    , controlButton (cButton)

{
    centreWithSize (360,300);
    setUsingNativeTitleBar (true);
    setResizable (false,false);

    //std::cout << "Audio CPU usage:" << adm.getCpuUsage() << std::endl;

    AudioDeviceSelectorComponent* adsc = new AudioDeviceSelectorComponent
        (adm,
         0, // minAudioInputChannels
         2, // maxAudioInputChannels
         0, // minAudioOutputChannels
         2, // maxAudioOutputChannels
         false, // showMidiInputOptions
         false, // showMidiOutputSelector
         false, // showChannelsAsStereoPairs
         false); // hideAdvancedOptionsWithButton

    adsc->setBounds (0, 0, 450, 240);

    setContentOwned (adsc, true);
    setVisible (false);
}


AudioConfigurationWindow::~AudioConfigurationWindow()
{
}


void AudioConfigurationWindow::closeButtonPressed()
{
    controlButton->setToggleState (false, dontSendNotification);

    String t = String (AccessClass::getAudioComponent()->getBufferSizeMs());
    t += " ms";
    controlButton->setText (t);
    AccessClass::getAudioComponent()->stopDevice();
    setVisible (false);
}


void AudioConfigurationWindow::resized()
{
}


void AudioConfigurationWindow::paint (Graphics& g)
{
    g.fillAll (Colours::darkgrey);
}
