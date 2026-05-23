/*
  ==============================================================================

	Object.cpp
	Created: 26 Sep 2020 10:02:32am
	Author:  No

  ==============================================================================
*/

#include "JuceHeader.h"
#include "CommandTimingFiltered.h"
#include "Definitions/ChannelFamily/ChannelFamilyManager.h"

CommandTimingFiltered::CommandTimingFiltered(var params) :
	BaseItem("Timing"),
	objectData(params),
	delayMult("Delay multiplicators"),
	fadeMult("Fade multiplicators")
{
	saveAndLoadRecursiveData = true;
	editorIsCollapsed = false;

	filter = addTargetParameter("Filter", "Select a family or a channel type that will be targeted by this timing, leave empty applies for all.", ChannelFamilyManager::getInstance());
	filter-> targetType = TargetParameter::CONTAINER;
	filter->typesFilter.add("ChannelFamily");
	filter->typesFilter.add("ChannelType");

	// to add a manager with defined data
	delayFrom = addFloatParameter("Delay", "delay of the first element (in seconds)", 0, 0);
	thruDelay = addBoolParameter("Thru delay", "Do you want to apply multiples delays ?", false);
	delayTo = addFloatParameter("Delay To", "delay of the last element (in seconds)", 0, 0);
	symmetryDelay = addBoolParameter("Delay Symmetry", "Apply this delay in symmetry", false);
	randomizeDelay = addBoolParameter("Delay random", "Randomize the delay values", false);

	fadeFrom = addFloatParameter("Fade", "fade of the first element (in seconds)", 0, 0);
	thruFade = addBoolParameter("Thru fade", "Do you want to apply multiple fades", false);
	fadeTo = addFloatParameter("Fade To", "fade of the last element (in seconds)", 0, 0);
	symmetryFade = addBoolParameter("Fade Symmetry", "Apply this fade in symmetry", false);
	randomizeFade = addBoolParameter("Fade random", "Randomize the fade values", false);

	curveFade.saveAndLoadRecursiveData = true;
	curveFade.setNiceName("Fade curve");
	curveFade.editorIsCollapsed = true;
	curveFade.allowKeysOutside = false;
	curveFade.isSelectable = false;
	curveFade.length->setValue(1);
	curveFade.addKey(0, 0, false);
	curveFade.items[0]->easingType->setValueWithData(Easing::BEZIER);
	curveFade.addKey(1, 1, false);
	curveFade.selectItemWhenCreated = false;
	curveFade.editorCanBeCollapsed = true;

	curveDelayRepart.saveAndLoadRecursiveData = true;
	curveDelayRepart.editorIsCollapsed = true;
	curveDelayRepart.setNiceName("Delay repartition");
	curveDelayRepart.allowKeysOutside = false;
	curveDelayRepart.isSelectable = false;
	curveDelayRepart.length->setValue(1);
	curveDelayRepart.addKey(0, 0, false);
	curveDelayRepart.items[0]->easingType->setValueWithData(Easing::LINEAR);
	curveDelayRepart.addKey(1, 1, false);
	curveDelayRepart.selectItemWhenCreated = false;
	curveDelayRepart.editorCanBeCollapsed = true;

	curveFadeRepart.saveAndLoadRecursiveData = true;
	curveFadeRepart.editorIsCollapsed = true;
	curveFadeRepart.setNiceName("Fade repartition");
	curveFadeRepart.allowKeysOutside = false;
	curveFadeRepart.isSelectable = false;
	curveFadeRepart.length->setValue(1);
	curveFadeRepart.addKey(0, 0, false);
	curveFadeRepart.items[0]->easingType->setValueWithData(Easing::LINEAR);
	curveFadeRepart.addKey(1, 1, false);
	curveFadeRepart.selectItemWhenCreated = false;
	curveFadeRepart.editorCanBeCollapsed = true;

	addChildControllableContainer(&curveFade);
	addChildControllableContainer(&curveDelayRepart);
	addChildControllableContainer(&curveFadeRepart);

	addChildControllableContainer(&delayMult);
	addChildControllableContainer(&fadeMult);

	updateDisplay();
}

CommandTimingFiltered::~CommandTimingFiltered()
{
}

void CommandTimingFiltered::updateDisplay()
{
	bool raw = true;
	bool thd = thruDelay->getValue();
	bool thf = thruFade->getValue();

	// to add a manager with defined data
	delayFrom->hideInEditor = !raw;
	thruDelay->hideInEditor = !raw;
	delayTo->hideInEditor = !raw || !thd;
	symmetryDelay->hideInEditor = !raw || !thd;
	randomizeDelay->hideInEditor = !thd;

	fadeFrom->hideInEditor = !raw;
	thruFade->hideInEditor = !raw;
	fadeTo->hideInEditor = !raw || !thf;
	symmetryFade->hideInEditor = !raw || !thf;
	randomizeFade->hideInEditor = !thf;

	curveFade.hideInEditor = !raw;
	curveDelayRepart.hideInEditor = !raw || !thd;
	curveFadeRepart.hideInEditor = !raw || !thf;

	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

void CommandTimingFiltered::parameterValueChanged(Parameter* p) {
	ControllableContainer::parameterValueChanged( p);
	if (p == thruDelay || p == thruFade) {
		updateDisplay();
	}
}

void CommandTimingFiltered::afterLoadJSONDataInternal() {
	updateDisplay();
}

