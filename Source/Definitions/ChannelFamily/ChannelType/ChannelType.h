/*
  ==============================================================================

    ChannelType.h
    Created: 7 Nov 2021 7:40:48pm
    Author:  No

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
class ChannelFamily;

class ChannelType:
    public BaseItem
{
    public:
    ChannelType(var params = var());
    ~ChannelType();

    String objectType;
    var objectData;

    ChannelFamily* parentFamily = nullptr;

    EnumParameter* priority;
    BoolParameter* reactGM;

    String getTypeString() const override { return "ChannelType"; }

};