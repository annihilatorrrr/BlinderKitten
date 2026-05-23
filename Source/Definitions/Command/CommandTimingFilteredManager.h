/*
  ==============================================================================

    ObjectManager.h
    Created: 26 Sep 2020 10:02:28am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "CommandTimingFiltered.h"
// #include "../ChannelFamily/ChannelType/ChannelType.h"

class CommandTimingFilteredManager :
    public BaseManager<CommandTimingFiltered>
{
public:
    CommandTimingFilteredManager(const juce::String& name = "CommandTimingFiltered");
    ~CommandTimingFilteredManager();

    bool shouldShowStepSize = false;
    void showStepSize(bool should);
    void addItemInternal(CommandTimingFiltered*, juce::var data) override;
};

