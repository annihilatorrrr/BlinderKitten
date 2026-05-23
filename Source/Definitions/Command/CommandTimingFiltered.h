/*
  ==============================================================================

    Object.h
    Created: 26 Sep 2020 10:02:32am
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "Definitions/Multiplicator/MultiplicatorLinkManager.h"

class CommandTimingFiltered :
    public BaseItem
{
public:
    CommandTimingFiltered(var params = var());
    virtual ~CommandTimingFiltered();

    String objectType;
    var objectData;

    // TargetParameter* timePreset;
    TargetParameter* filter;

    FloatParameter* fadeFrom;
    FloatParameter* delayFrom;
    FloatParameter* delayTo;
    BoolParameter* symmetryDelay;
    BoolParameter* randomizeDelay;

    BoolParameter* thruDelay;
    BoolParameter* thruFade;
    FloatParameter* fadeTo;
    BoolParameter* symmetryFade;
    BoolParameter* randomizeFade;

    Automation curveFade;
    Automation curveDelayRepart;
    Automation curveFadeRepart;

    MultiplicatorLinkManager delayMult;
    MultiplicatorLinkManager fadeMult;

    // String getTypeString() const override { return objectType; }
    void parameterValueChanged(Parameter* p);
    void afterLoadJSONDataInternal();
    void updateDisplay();

    static CommandTimingFiltered* create(var params) { return new CommandTimingFiltered(params); }
};
