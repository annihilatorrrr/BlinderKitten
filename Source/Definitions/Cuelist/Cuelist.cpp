/*
  ==============================================================================

	Object.cpp
	Created: 26 Sep 2020 10:02:32am
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"
#include "Cuelist.h"
#include "Brain.h"
#include "../ChannelValue.h"
#include "CuelistManager.h"
#include "UI/CuelistLoadWindow.h"
#include "UI/GridView/CuelistGridView.h"
#include "UI/ConductorInfos.h"
#include "UserInputManager.h"
#include "UI/CuelistSheet/CuelistSheet.h"

int sortCues(Cue* A, Cue* B) {
	String test = A->id->getValue() > B->id->getValue() ? "y" : "n";
	return A->id->floatValue() >= B->id->floatValue() ? 1 : -1;
}

Cuelist::Cuelist(var params) :
	BaseItem(params.getProperty("name", "Cuelist")),
	objectType(params.getProperty("type", "Cuelist").toString()),
	objectData(params),
	conductorInfos("Conductor infos"),
	chaserOptions("Chaser options"),
	chaserGenContainer("Regenerate Chaser"),
	offFadeCurve(),
	chaseGenValue(),
	cues(),
	speedMult("Speed multiplicators"),
	timing()
{
	saveAndLoadRecursiveData = true;
	nameCanBeChangedByUser = false;
	editorIsCollapsed = true;
	itemDataType = "Cuelist";
	canBeDisabled = false;

	cues.parentCuelist = this;

	conductorInfos.editorIsCollapsed = true;
	chaserOptions.editorIsCollapsed = true;
	offFadeCurve.editorIsCollapsed = true;

	id = addIntParameter("ID", "Id of this Cuelist", params.getProperty("ID", 1), 1);
	userName = addStringParameter("Name", "Name of this cuelist", "New cuelist");
	layerId = addIntParameter("Layer", "Higher layer, higer priority", 1, 1);
	updateName();

	conductorCurrentCueId = conductorInfos.addFloatParameter("Current ID", "ID of the current cue.", 0);
	conductorCurrentCueId->isSavable = false;
	conductorCurrentCueName = conductorInfos.addStringParameter("Current cue", "Current Cue name", "", false);
	conductorCurrentCueName->isSavable = false;
	conductorCurrentCueText = conductorInfos.addStringParameter("Current cue text", "What's happening during this cue ?", "", false);
	conductorCurrentCueText->multiline = true;
	conductorCurrentCueText->isSavable = false;
	conductorNextCueId = conductorInfos.addFloatParameter("Next ID", "ID of the current cue.", 0);
	conductorNextCueId->isSavable = false;
	conductorNextCueGo = conductorInfos.addStringParameter("Next go", "action needed to go to next cue", "", false);
	conductorNextCueGo->isSavable = false;
	conductorNextCueName = conductorInfos.addStringParameter("Next Cue", "Next cue name", "", false);
	conductorNextCueName->isSavable = false;
	addChildControllableContainer(&conductorInfos);

	isChaser = chaserOptions.addBoolParameter("Is Chaser", "Turn this cuelist in a wonderful chaser", false);
	chaserDirection = chaserOptions.addEnumParameter("Direction", "");
	chaserDirection->addOption("Normal", "normal");
	chaserDirection->addOption("Reverse", "reverse");
	chaserDirection->addOption("Bounce", "bounce");
	chaserDirection->addOption("Random", "random");
	chaserSpeed = chaserOptions.addFloatParameter("Speed", "in GO / minutes", 60,0.001);
	chaserInFade = chaserOptions.addFloatParameter("In fade", "Fade for incoming steps, not in seconds, but in number of steps !",0,0,1);
	chaserOutFade = chaserOptions.addFloatParameter("Out fade", "Fade for out values, not in seconds, but in number of steps !", 0, 0);
	chaserRunXTimes = chaserOptions.addFloatParameter("Run X times", "number of cycles of the chaser, 0 mean infinite",0,0);
	chaserStepPerTap = chaserOptions.addFloatParameter("Step per hits", "Number of steps per tap tempo hit",1,0.001);
	chaserTapTempo = chaserOptions.addTrigger("Tap tempo", "");

	chaserFadeInCurve.allowKeysOutside = false;
	chaserFadeInCurve.setNiceName("In curve");
	chaserFadeInCurve.isSelectable = false;
	chaserFadeInCurve.length->setValue(1);
	chaserFadeInCurve.addKey(0, 0, false);
	chaserFadeInCurve.items[0]->easingType->setValueWithData(Easing::LINEAR);
	chaserFadeInCurve.addKey(1, 1, false);
	chaserFadeInCurve.selectItemWhenCreated = false;
	chaserFadeInCurve.editorCanBeCollapsed = true;
	chaserFadeInCurve.editorIsCollapsed = true;
	chaserOptions.addChildControllableContainer(&chaserFadeInCurve);

	chaserFadeOutCurve.allowKeysOutside = false;
	chaserFadeOutCurve.setNiceName("Out curve");
	chaserFadeOutCurve.isSelectable = false;
	chaserFadeOutCurve.length->setValue(1);
	chaserFadeOutCurve.addKey(0, 0, false);
	chaserFadeOutCurve.items[0]->easingType->setValueWithData(Easing::LINEAR);
	chaserFadeOutCurve.addKey(1, 1, false);
	chaserFadeOutCurve.selectItemWhenCreated = false;
	chaserFadeOutCurve.editorCanBeCollapsed = true;
	chaserFadeOutCurve.editorIsCollapsed = true;
	chaserOptions.addChildControllableContainer(&chaserFadeOutCurve);

	chaserOptions.saveAndLoadRecursiveData = true;

	chaseGenGroup = chaserGenContainer.addIntParameter("Group ID", "Group used to generate chaser",0,0);
	chaseGenFixturesOnly = chaserGenContainer.addBoolParameter("Fixtures only", "use only fixtures (so don't use subfixtures) for the chaser", false);
	chaseGenBuddying = chaserGenContainer.addIntParameter("Buddying", "fixtures are so friends they can't leave each other", 1, 1);
	chaseGenBlocks = chaserGenContainer.addIntParameter("Blocks", "Repetitions of the chaser",1,1);
	chaseGenWings = chaserGenContainer.addIntParameter("Wings", "repetitions, but symmetrical",1,1);
	chaserGenButton = chaserGenContainer.addTrigger("Regenerate", "Be careful, this will erase current content");
	chaserGenContainer.addChildControllableContainer(&chaseGenValue);
	chaserGenContainer.editorIsCollapsed = true;
	chaserOptions.addChildControllableContainer(&chaserGenContainer);
	chaserGenContainer.saveAndLoadRecursiveData = true;
	chaseGenValue.saveAndLoadRecursiveData = true;

	timing.presetOrValue->removeOption("Cue timing");
	timing.presetOrValue->setValue("Raw Timing");
	addChildControllableContainer(&timing);
	addChildControllableContainer(&moveInBlack);
	addChildControllableContainer(&chaserOptions);

	endAction = addEnumParameter("Loop", "Behaviour of this cuelist at the end of its cues");
	endAction->addOption("Off", "off");
	endAction->addOption("Loop", "loop");

	offIfOverwritten = addBoolParameter("Off If Overwritten", "If checked, this cuelist will kill itself if all parameter are overwritten by other cuelists", false);

	tracking = addEnumParameter("Tracking", "Tracking will keep the values of previous cues enabled");
	tracking->addOption("None", "none");
	tracking->addOption("Cuelist order", "cuelist");
	tracking->addOption("Call order", "call");

	loopTracking = addBoolParameter("Loop Tracking", "Do you want tracking keeps values when coming back to start after loop ?", false);

	autoStart = addBoolParameter("Auto start", "Starts the cuelist if HTP level is set to a different value than 0", true);
	autoStop = addBoolParameter("Auto stop", "Stops the cuelist if HTP level is set to 0", true);
	excludeFromGrandMaster = addBoolParameter("Exclude from grand master", "If checked, the grand master will not have any effect on channels in this cuelist", false);
	excludeFromSwop = addBoolParameter("Exclude from swop", "If checked, swop another cuelist will not have any effect on channels in this cuelist", false);

	goBtn = addTrigger("GO", "Trigger next cue");
	goBackBtn = addTrigger("GO back", "Trigger previous cue");
	// goRandomBtn = addTrigger("GO random", "Trigger a random cue");
	offBtn = addTrigger("OFF", "Off this cuelist");
	killBtn = addTrigger("KILL", "Kill this cuelist and leave no clues");
	toggleBtn = addTrigger("Toggle", "Turns on and off the cuelist");
	loadBtn = addTrigger("Load", "Choose next cue");
	loadAndGoBtn = addTrigger("Load and go", "Choose a cue");
	// loadRandomBtn = addTrigger("Load Random", "Load a random cue");
	// flashOnBtn = addTrigger("Flash ON", "release flash");
	// flashOffBtn = addTrigger("flash Off", "press flash");
	// swopOnBtn = addTrigger("Swop ON", "press swop");
	// swopOffBtn = addTrigger("Swop Off", "release swop");
	tempMergeTrack = addTrigger("Temp merge track", "Merge the content of the programmer in this cue, values will be tracked");
	tempMergeNoTrack = addTrigger("Temp merge no track", "Merge the content of the programmer in this cue, values will not be tracked (off at the next go)");
	cleanAllBtn = addTrigger("Clean all cues", "Delete all unused commands in all cues");
	selectAsMainConductorBtn = addTrigger("Set main conductor", "Sets this cuelist as main conductor in project settings");

	nextCue = addTargetParameter("Next Cue", "Cue triggered when button go pressed", &cues);
	nextCue->maxDefaultSearchLevel = 0;
	nextCue->targetType = TargetParameter::CONTAINER;

	nextCueId = addFloatParameter("Next cue ID", "ID of the cue triggered when go pressed, 0 means next cue",0,0);

	HTPLevel = addFloatParameter("HTP Level", "Level master for HTP channels of this sequence", 1, 0, 1);
	flashLevel = addFloatParameter("Flash Level", "Flash/swop level master for HTP channels of this sequence", 1, 0, 1);
	LTPLevel = addFloatParameter("LTP Level", "Level master for LTP channels of this sequence", 1, 0, 1);
	flashWithLtpLevel = addBoolParameter("Flash with LTP level", "If checked, the LTP level will apply when flashing", false);
	absoluteLTPValue = addBoolParameter("Absolute LTP value", "If checked, LTP value act as a multiplier, if not, it acts like a crossfade.", false);

	isCuelistOn = addBoolParameter("is On", "Is this cuelist on ?", false);
	isCuelistOn->isSavable = false;
	isCuelistOn->isControllableFeedbackOnly = true;

	currentFade = addFloatParameter("Current Xfade", "Current position of the running cue", 0, 0, 1);
	currentFade->isControllableFeedbackOnly = true;
	currentFade->isSavable = false;

	upFadeController = addFloatParameter("up fade", "", 0, 0, 1);
	upFadeController->isSavable = false;
	upFadeController->hideInEditor = true;
	downFadeController = addFloatParameter("down fade", "", 0, 0, 1);
	downFadeController->isSavable = false;
	downFadeController->hideInEditor = true;
	crossFadeController = addFloatParameter("cross fade", "", 0, 0, 1);
	crossFadeController->isSavable = false;
	crossFadeController->hideInEditor = true;

	offFade = addFloatParameter("Off time", "Default fade time used to off the cuelist", 0, 0);

	soloPool = addIntParameter("Solo pool", "If greater than zero, only one element can be activated at a time with this number", 0, 0);
	moveInBlackDelay = addFloatParameter("MIB Delay", "Delay to wait after light goes off to trigger move in black", 0, 0);

	// offFadeCurve = new Automation();
	offFadeCurve.saveAndLoadRecursiveData = true;
	offFadeCurve.setNiceName("Off curve");
	offFadeCurve.allowKeysOutside = false;
	offFadeCurve.isSelectable = false;
	offFadeCurve.length->setValue(1);
	offFadeCurve.addKey(0, 0, false);
	offFadeCurve.items[0]->easingType->setValueWithData(Easing::LINEAR);
	offFadeCurve.addKey(1, 1, false);
	offFadeCurve.selectItemWhenCreated = false;
	offFadeCurve.editorCanBeCollapsed = true;
	addChildControllableContainer(&offFadeCurve);

	addChildControllableContainer(&speedMult);

	//nextCueId = addFloatParameter("Next Cue", "Index of the next cue", - 1, -1);
	cueA = nullptr; // static current cue
	cueB = nullptr; // loaded Cue

	// currentCue = addTargetParameter("Current Cue", "Last cue triggered", cues.get());
	// currentCue->maxDefaultSearchLevel = 0;
	// currentCue->targetType = TargetParameter::CONTAINER;

	renumberCuesBtn = addTrigger("Renumber cues", "Reset all cues IDs");

	addChildControllableContainer(&cues);

	cues.comparator.compareFunc = &sortCues;
	reorderCues();

	fillTexts();

	if (params.isVoid()) {
		cues.addItem();
	}

	Brain::getInstance()->registerCuelist(this, id->getValue());
}

Cuelist::~Cuelist()
{
	isDeleting = true;
	kill(true);
	Brain::getInstance()->unregisterCuelist(this);
	Brain::getInstance()->usingCollections.enter();
	Brain::getInstance()->cuelistPoolWaiting.removeAllInstancesOf(this);
	Brain::getInstance()->cuelistPoolUpdating.removeAllInstancesOf(this);
	for (int i = 0; i < cues.items.size(); i++) {
		Brain::getInstance()->cuePoolWaiting.removeAllInstancesOf(cues.items[i]);
		Brain::getInstance()->cuePoolUpdating.removeAllInstancesOf(cues.items[i]);
	}
	Brain::getInstance()->usingCollections.exit();
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		SubFixtureChannel* sfc = it.getKey();
		sfc->cuelistOutOfStack(this);
		Brain::getInstance()->pleaseUpdate(sfc);
	}
	if (CuelistSheet::getInstanceWithoutCreating() && CuelistSheet::getInstance()->targetCuelist == this) {
		CuelistSheet::getInstance()->targetCuelist = nullptr;
	}

}

void Cuelist::reorderCues() {
	cues.reorderItems();
	cues.correctCueIds();
	sendChangeMessage();
	//cues->queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, cues.get()));
}

void Cuelist::onContainerParameterChangedInternal(Parameter* p) {
	if (Brain::getInstance()->loadingIsRunning) {
		//return;
	}
	if (p == isCuelistOn || p == nextCue || p == nextCueId) {
		Brain::getInstance()->virtualButtonsNeedUpdate = true;
		Brain::getInstance()->virtualFaderButtonsNeedUpdate = true;
		Brain::getInstance()->cuelistGridNeedRefresh = true;
	}
	if (p == HTPLevel) {
		// LOG("coucou");
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		if (!Brain::getInstance()->loadingIsRunning) {
			if (autoStart->getValue() && cueA == nullptr && (float)HTPLevel->getValue() != 0 && lastHTPLevel == 0) {
				userGo();
			}
			else if (autoStop->getValue() && cueA != nullptr && (float)HTPLevel->getValue() == 0) {
				off();
			}
		}
		pleaseUpdateHTPs = true;
		lastHTPLevel = p->getValue();
		Brain::getInstance()->pleaseUpdate(this);
	}
	if (p == flashLevel) {
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		pleaseUpdateHTPs = true;
		Brain::getInstance()->pleaseUpdate(this);
	}
	if (p == LTPLevel) {
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		if (!Brain::getInstance()->loadingIsRunning) {
			if (autoStart->getValue() && cueA == nullptr && (float)LTPLevel->getValue() != 0 && lastLTPLevel == 0) {
				userGo();
			}
			else if (autoStop->getValue() && cueA != nullptr && (float)LTPLevel->getValue() == 0) {
				off();
			}
		}
		pleaseUpdateLTPs = true;
		lastLTPLevel = p->getValue();
		Brain::getInstance()->pleaseUpdate(this);
	}

	if (p == HTPLevel) {
		currentHTPLevelController = nextHTPLevelController;
		nextHTPLevelController = "";
	}
	if (p == LTPLevel) {
		currentLTPLevelController = nextLTPLevelController;
		nextLTPLevelController = "";
	}
	if (p == flashLevel) {
		currentFlashLevelController = nextFlashLevelController;
		nextFlashLevelController = "";
	}

	if (p == id) {
		Brain::getInstance()->registerCuelist(this, id->getValue(), true);
	}
	if (p == userName || p == id) {
		updateName();
	}
	if (p == nextCue || p == nextCueId) {
		fillTexts();
	}
	if (p == upFadeController) {
		currentUpFadeController = nextUpFadeController;
		nextUpFadeController = "";
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		if ((float)p->lastValue == 0 || upFadeCanMove) {
			float in = upFadeController->floatValue();
			float out = -1;

			if (crossFadeCanMove) { in = jmax(in, crossFadeController->floatValue()); }

			manualTransition(in, out);
			upFadeCanMove = true;
		}
	}
	if (p == downFadeController) {
		currentDownFadeController = nextDownFadeController;
		nextDownFadeController = "";
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		if ((float)p->lastValue == 0 || downFadeCanMove) {
			float in = -1;
			float out = downFadeController->floatValue();

			if (crossFadeCanMove) { out = jmax(out, crossFadeController->floatValue()); }

			manualTransition(in, out);
			downFadeCanMove = true;
		}
	}
	if (p == crossFadeController) {
		currentCrossFadeController = nextCrossFadeController;
		nextCrossFadeController = "";
		Brain::getInstance()->virtualFadersNeedUpdate = true;
		if ((float)p->lastValue == 0 || crossFadeCanMove) {
			float in = crossFadeController->floatValue();
			float out = in;

			if (upFadeCanMove) { in = jmax(in, upFadeController->floatValue()); }
			if (downFadeCanMove) { out = jmax(out, downFadeController->floatValue()); }

			manualTransition(in, out);
			crossFadeCanMove = true;
		}
	}
}

void Cuelist::triggerTriggered(Trigger* t) {
	if (t == goBtn) {
		userGo();
	}
	else if (t == goBackBtn) {
		goBack();
	}
	else if (t == goRandomBtn) {
		goRandom();
	}
	else if(t == offBtn) {
		off();
	}
	else if (t == killBtn) {
		kill();
	}
	else if (t == flashOnBtn) {
		flash(true, true);
	}
	else if (t == flashOffBtn) {
		flash(false, true);
	}
	else if (t == swopOnBtn) {
		flash(true, true, true);
	}
	else if (t == swopOffBtn) {
		flash(false, true);
	}
	else if (t == renumberCuesBtn) {
		renumberCues();
	}
	else if (t == loadBtn) {
		showLoad();
	}
	else if (t == loadAndGoBtn) {
		showLoadAndGo();
	}
	else if (t == loadRandomBtn) {
		loadRandom();
	}
	else if (t == tempMergeTrack) {
		Programmer* p = UserInputManager::getInstance()->getProgrammer(false);
		tempMergeProgrammer(p, true);
	}
	else if (t == tempMergeNoTrack) {
		Programmer* p = UserInputManager::getInstance()->getProgrammer(false);
		tempMergeProgrammer(p, false);
	}
	else if (t == cleanAllBtn) {
		for (Cue* c : cues.items) {
			c->cleanUnused();
		}
	}
	else if (t == selectAsMainConductorBtn) {
		selectAsMainConductor();
	}
	else {}
}

void Cuelist::onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c) {
	if (c == chaserSpeed) {
		chaserStepDuration = 60000.0/(double)chaserSpeed->getValue();
		chaserFadeInDuration = chaserStepDuration * (double)chaserInFade->getValue();
		chaserFadeOutDuration = chaserStepDuration * (double)chaserOutFade->getValue();;
		Brain::getInstance()->virtualFadersNeedUpdate = true;
	}
	else if (c == chaserInFade) {
		chaserFadeInDuration = chaserStepDuration * (double)chaserInFade->getValue();
	}
	else if (c == chaserOutFade) {
		chaserFadeOutDuration = chaserStepDuration * (double)chaserOutFade->getValue();;
	}
	else if (c == chaserTapTempo) {
		tapTempo();
	}
	else if (c == chaserGenButton) {
		autoCreateChaser();
	}

}


void Cuelist::userGo()
{
	userPressedGo = true;
	if (isChaser->boolValue() && cueA != nullptr) {
		cueA->TSAutoFollowEnd = 0;
	}
	if (soloPool->intValue() > 0) Brain::getInstance()->soloPoolCuelistStarted(soloPool->intValue(), this);
	go(-1, -1);
}

void Cuelist::userGo(Cue* c)
{
	userPressedGo = true;
	if (isChaser->boolValue() && cueA != nullptr) {
		cueA->TSAutoFollowEnd = 0;
	}
	if (soloPool->intValue() > 0) Brain::getInstance()->soloPoolCuelistStarted(soloPool->intValue(), this);
	go(c, -1, -1);
}

void Cuelist::userGo(float delay, float fade)
{
	userPressedGo = true;
	if (isChaser->boolValue() && cueA != nullptr) {
		cueA->TSAutoFollowEnd = 0;
	}
	if (soloPool->intValue() > 0) Brain::getInstance()->soloPoolCuelistStarted(soloPool->intValue(), this);
	go(delay, fade);
}


void Cuelist::userGo(Cue* c, float delay, float fade)
{
	userPressedGo = true;
	if (isChaser->boolValue() && cueA != nullptr) {
		cueA->TSAutoFollowEnd = 0;
	}
	if (soloPool->intValue() > 0) Brain::getInstance()->soloPoolCuelistStarted(soloPool->intValue(), this);
	go(c, delay, fade);
}


void Cuelist::go()
{
	go(-1, -1);
}

void Cuelist::go(Cue* c)
{
	go(c, -1, -1);
}

void Cuelist::go(float forcedDelay, float forcedFade) {
	cueB = dynamic_cast<Cue*>(nextCue->targetContainer.get());
	float nextId = nextCueId->getValue();
	if (nextId > 0) {
		for (int i = 0; i < cues.items.size() && cueB == nullptr; i++) {
			if ((float)cues.items[i]->id->getValue() >= nextId) {
				cueB = cues.items[i];
			}
		}
	}
	if (cueB == nullptr) {
		if (isChaser->getValue()) {
			cueB = getNextChaserCue();
		}
		else {
			autoLoadCueB();
		}
	}
	go(cueB, forcedDelay, forcedFade);
}

void Cuelist::go(Cue* c, float forcedDelay, float forcedFade) {
	//const MessageManagerLock mmLock;
	double now = Time::getMillisecondCounterHiRes();
	lastGoTS = now;
	if (TSLateCompensation > 0) {
		bool compensate = false;
		if (isChaser->boolValue()) {
			compensate = chaserStepDuration > TSLateCompensation;
		}
		else {
			compensate = true;
		}
		if (compensate) {
			now -= TSLateCompensation;
		}
		TSLateCompensation = 0;
	}

	isComputing.enter();
	TSTransitionStart = now;
	TSTransitionEnd = now;
	currentManualInTransition= 0;
	currentManualOutTransition = 0;
	stopTransition = false;
	stuckUpFade = 0;
	stuckDownFade = 0;
	transitionRunning = true;
	currentFade->setValue(0, false);
	upFadeCanMove = false;
	downFadeCanMove = false;
	crossFadeCanMove = false;

	float speedMultVal = speedMult.getValue();
	if (speedMultVal < 0.00001) { speedMultVal = 0.00001; }
	speedMultVal = 1/speedMultVal;

	if (cueA != nullptr) {
		cueA->TSAutoFollowEnd = 0;
	} 
	

	if (isChaser->boolValue() && (float)chaserRunXTimes->getValue() > 0) {
		if (cueA == nullptr && isChaser->getValue()) {
			if (chaserDirection->getValueData() == "bounce" && cues.items.size()>1) {
				int nItems = cues.items.size();
				nItems = (nItems*2)-2;
				chaserRemainingSteps = round((float)chaserRunXTimes->getValue() * nItems);
			}
			else {
				chaserRemainingSteps = round((float)chaserRunXTimes->getValue() * cues.items.size());
				chaserRemainingSteps-=1;
			}
		}
		else if (chaserRemainingSteps <= 0 && !wannaOff) {
			wannaOff = true;
			c = nullptr;
		}
		else {
			chaserRemainingSteps -= 1;
		}

	}

	bool needRebuildTracking = false;
	String trackingType = tracking->getValue();

	int currentIndex = -1;
	int nextIndex = -1;
	if (c != nullptr && trackingType == "cuelist") {
		nextIndex = cues.items.indexOf(c);
		if (cueA != nullptr) {
			currentIndex = cues.items.indexOf(cueA);
		}

		if (nextIndex != currentIndex + 1) {
			needRebuildTracking = true;
		}
	}

	if (cueA != nullptr) {
		cueA->off(forcedDelay, forcedFade);
	}

	cueA = c;
	cueB = c;

	if (c != nullptr) {
		isCuelistOn->setValue(true);
		wannaOff = false;
	}
	else {
		wannaOff = true;
		Brain::getInstance()->pleaseUpdate(this);
	}

	HashMap<SubFixtureChannel*, std::shared_ptr<ChannelValue>> newActiveValues;

	if (needRebuildTracking && !c->releaseCurrentTracking->boolValue()) {
		for (int i = 0; i <= nextIndex-1; i++) {
			Cue* tempCue = cues.items[i];
			if (tempCue->releaseCurrentTracking->boolValue()) {
				newActiveValues.clear();
			}
			tempCue->computeValues();
			tempCue->csComputing.enter();
			for (auto it = tempCue->computedValues.begin(); it != tempCue->computedValues.end(); it.next()) {
				std::shared_ptr<ChannelValue> temp = it.getValue();
				if (newActiveValues.contains(it.getKey())) {
					//ChannelValue* current = newActiveValues.getReference(it.getKey());
					temp->values.set(0,it.getKey()->postCuelistValue);
				}
				else {
					temp->values.set(0, -1);
				}
				float delay = forcedDelay != -1 ? forcedDelay : temp->delay;
				float fade = forcedFade != -1 ? forcedFade : temp->fade;
				delay *= speedMultVal;
				fade *= speedMultVal;
				temp->TSInit = now;
				temp->TSStart = now;//+ (delay);
				temp->TSEnd = now;//temp->TSStart + (fade);
				TSTransitionEnd = jmax(TSTransitionEnd, (double)temp->TSEnd);
				temp->isEnded = true;
				newActiveValues.set(it.getKey(), temp);
				it.getKey()->cuelistOnTopOfStack(this);
			}
			tempCue->csComputing.exit();
			tempCue->runTasks(0,0);
			tempCue->runOffTasks(0,0);
			//tempCue->go();
			//tempCue->off();
		}
	}

	if (c != nullptr) {
		c->computeValues();
		c->csComputing.enter();
		for (auto it = c->computedValues.begin(); it != c->computedValues.end(); it.next()) {
			std::shared_ptr<ChannelValue> temp = it.getValue();
			if (activeValues.contains(it.getKey())) {
				std::shared_ptr<ChannelValue> current = activeValues.getReference(it.getKey());
				if (it.getKey()->isHTP && !temp->htpOverride) {
					if (current != nullptr) {
						double fader = HTPLevel->getValue();
						double currentTiming = jmap(now, (double)current->TSStart, (double)current->TSEnd,0.0,1.0);
						if (stopTransition) {
							currentTiming = currentManualInTransition;
						}
						if (current->isEnded) {
							currentTiming = 1;
						}
						currentTiming = jlimit(0.0,1.0,currentTiming);
						temp->values.set(0, current->valueAt(currentTiming, 0)*fader);
					}
					else {
						temp->values.set(0, 0);
					}
				}
				else {
					temp->values.set(0, it.getKey()->postCuelistValue);
				}
			}
			else {
				temp->values.set(0, -1);
			}
			if (isChaser->getValue()) {
				temp->TSInit = now;
				temp->TSStart = now;
				temp->TSEnd = temp->TSStart + (chaserFadeInDuration * speedMultVal);
				temp->fadeCurve = &chaserFadeInCurve;
			}
			else {
				temp->TSInit = now;
				float delay = forcedDelay != -1 ? forcedDelay : temp->delay;
				float fade = forcedFade != -1 ? forcedFade : temp->fade;
				delay *= speedMultVal;
				fade *= speedMultVal;
				temp->TSStart = now + (delay);
				temp->TSEnd = temp->TSStart + (fade);
			}
			temp->isEnded = false;
			TSTransitionEnd = jmax(TSTransitionEnd, (double)temp->TSEnd);
			newActiveValues.set(it.getKey(), temp);
			it.getKey()->cuelistOnTopOfStack(this);
		}
		c->csComputing.exit();
		c->go(forcedDelay, forcedFade);

		for (int i = 0; i < c->commandHistory.size(); i++) {
			commandHistory.removeAllInstancesOf(c->commandHistory[i]);
			commandHistory.add(c->commandHistory[i]);
		}
	}
	
	bool releaseTracking = trackingType == "none" || c == nullptr || needRebuildTracking || isChaser->getValue() || c->releaseCurrentTracking->boolValue();

	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		if (!newActiveValues.contains(it.getKey())) {
			std::shared_ptr<ChannelValue> temp = it.getValue();
			if (temp != nullptr && temp -> endValue() != -1 && (releaseTracking || temp->canBeTracked == false)) {
				temp->isTransitionOut = true;
				temp->parentCommand = nullptr;
				float fadeTime = 0;
				float delayTime = 0;
				if (c == nullptr) {
					fadeTime = (float)offFade->getValue() * 1000;
					temp->fadeCurve = &offFadeCurve;
				} 
				else if (isChaser->getValue()) {
					fadeTime = chaserFadeOutDuration;
					temp->fadeCurve = &chaserFadeOutCurve;
				}
				else {
					if (it.getKey()->isHTP) {
						if (c->htpOutFade->floatValue() >= 0) { fadeTime = c->htpOutFade->floatValue() * 1000.; }
						else if (c->htpInFade->floatValue() >= 0) { fadeTime = c->htpInFade->floatValue() * 1000.; }
						else { fadeTime = (float)offFade->getValue() * 1000; }

						if (c->htpOutDelay->floatValue() >= 0) { delayTime = c->htpOutDelay->floatValue() * 1000.; }
						else if (c->htpInDelay->floatValue() >= 0) { delayTime = c->htpInDelay->floatValue() * 1000.; }
						else { delayTime = 0; }
					}
					else {
						if (c->ltpFade->floatValue() >= 0) { fadeTime = c->ltpFade->floatValue() * 1000.; }
						else if (c->htpInFade->floatValue() >= 0) { fadeTime = c->htpInFade->floatValue() * 1000.; }
						else { fadeTime = (float)offFade->getValue() * 1000; }

						if (c->ltpDelay->floatValue() >= 0) { delayTime = c->ltpDelay->floatValue() * 1000.; }
						else if (c->htpInDelay->floatValue() >= 0) { delayTime = c->htpInDelay->floatValue() * 1000.; }
						else { delayTime = 0; }
					}
				}

				if (it.getKey()->snapOnly) {
					fadeTime = 0;
				}

				float delay = forcedDelay != -1 ? forcedDelay : delayTime;
				float fade = forcedFade != -1 ? forcedFade : fadeTime;
				delay *= speedMultVal;
				fade *= speedMultVal;
				temp->TSInit = now;
				temp->TSStart = now + delay;
				temp->TSEnd = now + fade + delay;
				TSTransitionEnd = jmax(TSTransitionEnd, (double)temp->TSEnd);
				temp->values.clear();
				temp->values.set(0, temp->value);
				temp->values.set(1, -1);
				temp->isEnded = false;

				newActiveValues.set(it.getKey(), temp);
			}

		}
	}
	
	// move in black
	if (!isChaser->boolValue() && c != nullptr) {
		Cue* MIBNextCue = getNextCue();
		if (MIBNextCue != nullptr) {
			MIBNextCue->computeValues();
			for (auto it = MIBNextCue->computedValues.begin(); it != MIBNextCue->computedValues.end(); it.next()) {
				std::shared_ptr<ChannelValue> cv = it.getValue();
				if (cv->moveInBlack) {
					SubFixtureChannel* sfc = it.getKey();
					BKEngine* engine = (BKEngine*)BKEngine::mainEngine;
					ChannelType* dimmerCT = dynamic_cast<ChannelType*>(engine->IntensityChannel->targetContainer.get());
					if (sfc->parentSubFixture->channelsMap.contains(dimmerCT)) {
						SubFixtureChannel* sfcDimmer = sfc->parentSubFixture->channelsMap.getReference(dimmerCT);
						bool canMove = true;
						int64 moveAfter = now;
						if (activeValues.contains(sfcDimmer) && activeValues.getReference(sfcDimmer)!= nullptr) {
							if (activeValues.getReference(sfcDimmer)->endValue() >= 0) canMove = false;
						}
						if (newActiveValues.contains(sfcDimmer) && newActiveValues.getReference(sfcDimmer) != nullptr) {
							std::shared_ptr<ChannelValue> cvDim = newActiveValues.getReference(sfcDimmer);
							if (cvDim->endValue() > 0) {
								canMove = false;
							}
							else {
								canMove = true;
								moveAfter = cvDim->TSEnd;
							}
						}
						if (canMove) {
							moveAfter += cv->moveInBlackDelay*1000.0;
							cv->TSInit = moveAfter;
							cv->TSStart = moveAfter;
							cv->TSEnd = moveAfter;

							if (activeValues.contains(sfc)) {
								std::shared_ptr<ChannelValue> currentCV = activeValues.getReference(sfc);
								cv->values.set(0,currentCV->endValue());
							}
							newActiveValues.set(sfc, cv);
							sfc->cuelistOnTopOfStack(this);
						}
					}
				}
			}
		}
	}

	TSTransitionDuration = TSTransitionEnd - TSTransitionStart;
	Array<SubFixtureChannel*> toUpdate;
	
	for (auto it = newActiveValues.begin(); it != newActiveValues.end(); it.next()) {
		activeValues.set(it.getKey(), it.getValue());
		toUpdate.add(it.getKey());
	}

	Array<Command*> usefulCommands;
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		if (it.getValue() != nullptr) {
			Command* cmd = it.getValue()->parentCommand;
			if (cmd != nullptr) usefulCommands.addIfNotAlreadyThere(cmd);
		}
	}

	for (int i = commandHistory.size() - 1; i >= 0; i--) {
		Command* cmd = commandHistory[i];
		if (!usefulCommands.contains(cmd)) {
			commandHistory.removeRange(i,1);
		}
	}

	isComputing.exit();

	if (isChaser->getValue() && c != nullptr) {
		c->TSAutoFollowStart = now;
		c->TSAutoFollowEnd = now + (chaserStepDuration * speedMultVal);
		Brain::getInstance()->pleaseUpdate(c);

	}

	for (int i = 0; i < toUpdate.size(); i++) {
		Brain::getInstance()->pleaseUpdate(toUpdate[i]);
	}

	if (cues.items.size() > 1 && !isChaser->boolValue()) {
		Brain::getInstance()->reconstructVirtuals = true;
	}

	if (!isChaser->boolValue() && c != nullptr) {
		c->writeTimeStamp();
	}

	if (CuelistSheet::getInstance()->targetCuelist == this) {
		MessageManager::callAsync([this](){
			CuelistSheet::getInstance()->updateRunningCue();
		});
	}

	const MessageManagerLock mmLock;
	nextCue->resetValue();
	nextCueId->resetValue();
	fillTexts();

	return ;
}

void Cuelist::goBack() {
	goBack(-1,-1);
}

void Cuelist::goBack(float forcedDelay, float forcedFade)
{
	if (cueA == nullptr) {
		return;
	}
	Array<Cue*> currentCues = cues.getItemsWithType<Cue>();
	for (int i = 0; i < currentCues.size() - 1; i++) {
		if (currentCues[i + 1] == cueA) {
			userGo(currentCues[i], forcedDelay, forcedFade);
			return;
		}
	}
}

void Cuelist::flash(bool setOn, bool withTiming, bool swop) {
	if (!isComputing.tryEnter()) {return; }
	if (setOn) {
		TSOffFlash = 0;
		TSOffFlashEnd = 0;
		isFlashing = true;
		if (cueA == nullptr) {
			if (withTiming) {
				go();
			}
			else {
				go(0,0);
			}
		}
		if (swop) {
			isSwopping = true;
			Brain::getInstance()->swoppedCuelist(this);
		}
		if (cueA != nullptr) {
			cueA->csComputing.enter();
			for (auto it = cueA->computedValues.begin(); it != cueA->computedValues.end(); it.next()) {
				Brain::getInstance()->pleaseUpdate(it.getKey());
			}
			cueA->csComputing.exit();
		}
	}
	else {
		isFlashing = false;
		TSOffFlash = Time::getMillisecondCounterHiRes();
		TSOffFlashEnd = TSOffFlash + (offFade->floatValue()*1000.0);
		if (swop || isSwopping) {
			isSwopping = false;
			Brain::getInstance()->unswoppedCuelist(this);
		}
		if (cueA != nullptr) {
			cueA->csComputing.enter();
			for (auto it = cueA->computedValues.begin(); it != cueA->computedValues.end(); it.next()) {
				Brain::getInstance()->pleaseUpdate(it.getKey());
			}
			cueA->csComputing.exit();
		}
		if (!userPressedGo) {
			if (withTiming) {
				off();
			}
			else {
				off(0,0);
				kill();
			}
		}
	}
	isComputing.exit();
	/*
	
	if (setOn) {
		isFlashing = true;
		double now = Time::getMillisecondCounterHiRes();

		Cue* flashingCue = cueA;
		if (flashingCue == nullptr) {
			autoLoadCueB();
			flashingCue = cueB;
		}

		cueB = nullptr;

		if (swop) {
			isSwopping = true;
			Brain::getInstance()->swoppedCuelist(this);
		}

		if (flashingCue != nullptr) {
			ScopedLock locK(Brain::getInstance()->usingCollections);
			flashingCue->computeValues();
			for (auto it = flashingCue->computedValues.begin(); it != flashingCue->computedValues.end(); it.next()) {
				ChannelValue* temp = it.getValue();
				if (flashingValues.contains(it.getKey())) {
					// ChannelValue* current = flashingValues.getReference(it.getKey());
					temp->startValue = it.getKey()->value;
				}
				else {
					temp->startValue = -1;
				}
				temp->TSInit = now;
				temp->TSStart = withTiming ? now + (temp->delay) : now;
				temp->TSEnd = withTiming ? temp->TSStart + (temp->fade) : now;
				temp->isEnded = false;
				flashingValues.set(it.getKey(), temp);
				it.getKey()->cuelistOnTopOfFlashStack(this);
				Brain::getInstance()->pleaseUpdate(it.getKey());
			}
		}
	}
	else {
		wannaOffFlash = true;
		isSwopping = false;
		Brain::getInstance()->unswoppedCuelist(this);
		for (auto it = flashingValues.begin(); it != flashingValues.end(); it.next()) {
			Brain::getInstance()->pleaseUpdate(it.getKey());
		}
		Brain::getInstance()->pleaseUpdate(this);
	}
	*/
}


void Cuelist::goRandom() {
	Array<Cue*> childCues = cues.getItemsWithType<Cue>();
	Array<Cue*> allowedCues;

	for (int i = 0; i< childCues.size(); i++) {
		if (childCues.getReference(i) !=cueA && childCues[i]->canBeRandomlyCalled->getValue()) {
			allowedCues.add(childCues.getReference(i));
		}
	}

	int s = allowedCues.size();
	if (s > 0) {
		int r = Brain::getInstance()->mainRandom.nextInt(s);
		userGo(allowedCues[r]);
	}
}

void Cuelist::toggle()
{
	if (cueA == nullptr) {
		userGo();
	}
	else {
		off();
	}
}

void Cuelist::off(float forcedDelay, float forcedFade)
{
	wannaOff = true;
	go(nullptr, forcedDelay, forcedFade);
}

void Cuelist::off() {
	off(-1, -1);
}






void Cuelist::update() {
	isComputing.enter();
	if (pleaseUpdateHTPs) {
		updateHTPs();
	}
	if (pleaseUpdateLTPs) {
		updateLTPs();
	}
	float tempPosition = 1;
	float isUseFul = false;
	float isOverWritten = true;
	bool isEnded = true;
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		std::shared_ptr<ChannelValue> cv = it.getValue();
		if (cv != nullptr) {
			tempPosition = jmin(tempPosition, cv->currentPosition);
			isUseFul = isUseFul || cv->endValue() != -1 || !cv->isEnded;
			isEnded = isEnded && cv->isEnded;
			isOverWritten = isOverWritten && cv->isOverWritten;
		}
	}
	currentFade->setValue(tempPosition);
	transitionRunning = !isEnded;
	if (tempPosition == 1 && cueA != nullptr) {
		cueA->endTransition();
	}
	if (isOverWritten) {
		isUseFul = false;
	}
	if (!isUseFul && offIfOverwritten->getValue()) {
		kill(false);
	} 
	if (isEnded && wannaOff) {
		kill(false);
	}
	if (isEnded) {
		upFadeCanMove = false;
		downFadeCanMove = false;
		crossFadeCanMove = false;
	}

	if (wannaOffFlash) {
		bool canStopFlash = true;
		for (auto it = flashingValues.begin(); it != flashingValues.end(); it.next()) {
			std::shared_ptr<ChannelValue> cv = it.getValue();
			canStopFlash = canStopFlash && cv->isEnded;
		}

		if (canStopFlash) {
			for (auto it = flashingValues.begin(); it != flashingValues.end(); it.next()) {
				it.getKey()->cuelistOutOfFlashStack(this);
				Brain::getInstance()->pleaseUpdate(it.getKey());
			}
			isFlashing = false;
			wannaOffFlash = false;
		}
	}

	isComputing.exit();
}

void Cuelist::autoLoadCueB() {
	bool valid = false;
	if (cueA == nullptr) {
		if (cues.getItemsWithType<Cue>().size() > 0) {
			cueB = cues.getItemsWithType<Cue>()[0];
			valid = true;
		}
	}
	else {
		Array<Cue*> currentCues = cues.getItemsWithType<Cue>();
		for (int i = 1; i < currentCues.size() && !valid; i++) {
			if (currentCues[i - 1] == cueA) {
				valid = true;
				cueB = currentCues[i];
			}
		}
		if (!valid) { // cueA is the last cue
			String end = endAction->getValue();
			if (end == "loop") {
				cueB = currentCues[0];
			}
			else {
				cueB = nullptr;
			}
		}
	}
	fillTexts();
}

float Cuelist::applyToChannel(SubFixtureChannel* fc, float currentVal, double now, bool& isApplied, bool flashValues) {
	float val = currentVal;
	bool HTP = fc->parentParamDefinition->priority->getValue() == "HTP";
	
	bool keepUpdate = false;
	isApplied = false;

	float localValue = 0;
	std::shared_ptr<ChannelValue> cv;
	float faderLevel = 0;
	if (!activeValues.contains(fc)) {return currentVal;}
	cv = activeValues.getReference(fc);
	if (cv == nullptr) {
		return currentVal;
	}

	if (stopTransition) {
		double ratio = cv->isTransitionOut ? currentManualOutTransition : currentManualInTransition;
		now = TSTransitionStart + (TSTransitionDuration * ratio);
	}

	faderLevel = (float)HTPLevel->getValue();

	if (TSOffFlashEnd > now) {
		double flashLvl = jmax(faderLevel, (float)flashLevel->getValue());
		faderLevel = jmap(now, TSOffFlash, TSOffFlashEnd, flashLvl, (double)faderLevel);
		keepUpdate = true;
	}
	else if (isFlashing) {
		faderLevel = jmax(faderLevel,(float)flashLevel->getValue());
	}
	else {
	}


	float valueFrom = currentVal;
	float valueTo = currentVal;

	if (cv->startValue() >= 0) {
		valueFrom = cv->startValue(); 
	}
	if (cv->endValue() >= 0) { 
		valueTo = cv->endValue(); 
	}

	float totTime = (cv->TSEnd - cv->TSInit);
	if (totTime > 0) {
		cv->currentPosition = (now - cv->TSInit) / (totTime);
	}
	else {
		cv->currentPosition = 1;
	}

	float fade = 0;
	if (flashValues) {
		localValue = valueTo;
		fade = 1;
	}
	else if (cv -> TSStart > now) {
		fade = 0;
		localValue = valueFrom; 
		keepUpdate = true;
		Brain::getInstance()->pleaseUpdate(this);
	}
	else if (cv -> TSEnd <= now) {
		localValue = valueTo;
		fade = 1;
		if (!cv->isEnded) {
			cv->isEnded = true;
			Brain::getInstance()->pleaseUpdate(this);
		}
	}
	else {
		fade = double(now - cv->TSStart) / double(cv->TSEnd - cv->TSStart);
		if (cv->fadeCurve != nullptr) fade = cv->fadeCurve->getValueAtPosition(fade);
		//localValue = jmap(fade, valueFrom, valueTo);
		localValue = cv->valueAt(fade, currentVal);
		keepUpdate = true;
		Brain::getInstance()->pleaseUpdate(this);
	}
	cv->value = localValue;

	if (HTP) {
		localValue *= faderLevel;
	}
	else {
		if (!isFlashing || flashWithLtpLevel->boolValue()) {
			float localValueNoFade = localValue;
			if (absoluteLTPValue->boolValue()) {
				localValue *= (double)LTPLevel->getValue();
			}
			else {
				localValue = jmap(LTPLevel->floatValue(), currentVal, localValue);
			}
			localValue = jmap(fade,0.f,1.f,localValueNoFade, localValue);
		}
	}



	if (HTP && !cv->htpOverride) {
		isApplied = localValue >= val;
		val = jmax(val, localValue);
	}
	else {
		isApplied = true;
		val = localValue;
	}

	if (keepUpdate && !stopTransition) {
		Brain::getInstance()->pleaseUpdate(fc);
	}
	return val;
}

void Cuelist::insertProgCueBefore()
{
	int index = cueA == nullptr ? 0 : cues.items.indexOf(cueA);
	insertProgCueAtIndex(index);
}

void Cuelist::insertProgCueAfter()
{
	int index = cueA == nullptr ? cues.items.size() : cues.items.indexOf(cueA)+1;
	insertProgCueAtIndex(index);
}

void Cuelist::insertProgCueBefore(Cue* c)
{
	int index = cues.items.indexOf(c);
	insertProgCueAtIndex(index);
}

void Cuelist::insertProgCueAfter(Cue* c)
{
	int index = cues.items.indexOf(c);
	if (index == -1) index = cues.items.size()-1;
	insertProgCueAtIndex(index+1);
}

void Cuelist::insertProgCueAtIndex(int index)
{
	//const MessageManagerLock mmlock;
	Programmer* p = UserInputManager::getInstance()->getProgrammer(false);
	if (p != nullptr) {
		Cue * cue = cues.addItem();
		cues.setItemIndex(cue, index);	
		cue->commands.clear();
		for (int i = 0; i < p->commands.items.size(); i++) {
			Command* com = cue->commands.addItem();
			com->loadJSONData(p->commands.items[i]->getJSONData());
		}
		go(cue);
	}

}

void Cuelist::updateHTPs() {
	pleaseUpdateHTPs = false;
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		SubFixtureChannel* chan = it.getKey();
		if (chan!=nullptr && chan->isHTP) {
			Brain::getInstance()->pleaseUpdate(chan);
		}
	}
	for (auto it = flashingValues.begin(); it != flashingValues.end(); it.next()) {
		SubFixtureChannel* chan = it.getKey();
		if (chan != nullptr && chan->isHTP) {
			Brain::getInstance()->pleaseUpdate(chan);
		}
	}
}

void Cuelist::updateLTPs() {
	pleaseUpdateLTPs = false;
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		SubFixtureChannel* chan = it.getKey();
		if (chan != nullptr && !chan->isHTP) {
			Brain::getInstance()->pleaseUpdate(chan);
		}
	}
}

void Cuelist::updateAllChannels()
{
	isComputing.enter();
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		SubFixtureChannel* chan = it.getKey();
		if (chan != nullptr) {
			Brain::getInstance()->pleaseUpdate(chan);
		}
	}
	isComputing.exit();
}


void Cuelist::kill(bool forceRefreshChannels) {
	wannaOff = true;
	for (auto it = activeValues.begin(); it != activeValues.end(); it.next()) {
		SubFixtureChannel* chan = it.getKey();
		chan->cuelistOutOfStack(this);
		if (forceRefreshChannels) {
			Brain::getInstance()->pleaseUpdate(chan);
		}
	}
	isComputing.enter();
	if (cueA != nullptr) {
		cueA->computedValues.clear();
	}
	activeValues.clear();
	isComputing.exit();
	cueA = nullptr;
	cueB = nullptr;
	userPressedGo = false;
	isCuelistOn->setValue(false);
	fillTexts();
}

void Cuelist::setHTPLevel(float level) {
	HTPLevel->setValue(level);
}

void Cuelist::setFlashLevel(float level) {
	flashLevel->setValue(level);
}

void Cuelist::setLTPLevel(float level) {
	LTPLevel->setValue(level);
}

void Cuelist::setChaserSpeed(float level) {
	chaserSpeed->setValue(level);
}

void Cuelist::manualTransition(float ratioIn, float ratioOut)
{
	if (ratioIn == -1 && ratioOut == -1) {return;}
	if (!transitionRunning) {
		userGo();
	}
	if (!stopTransition) {
		if (ratioIn > currentFade->floatValue() || ratioOut > currentFade->floatValue()) {
		stopTransition = true;
		stuckUpFade = currentFade->floatValue();
		stuckDownFade = currentFade->floatValue();
		currentManualInTransition = stuckUpFade;
		currentManualOutTransition = stuckDownFade;
		}
	}
	if (ratioIn != -1 && ratioIn > stuckUpFade) { 
		currentManualInTransition = ratioIn; 
		stuckUpFade = 0;
	}
	if (ratioOut != -1 && ratioOut > stuckDownFade) { 
		currentManualOutTransition = ratioOut; 
		stuckDownFade = 0;
	}
	updateAllChannels();
}

void Cuelist::showLoad()
{
	CuelistLoadWindow::getInstance()->loadCuelist(this, false);
}

void Cuelist::showLoadAndGo()
{
	CuelistLoadWindow::getInstance()->loadCuelist(this, true);
}


void Cuelist::updateName() {
	String n = userName->getValue();
	if (parentContainer != nullptr) {
		dynamic_cast<CuelistManager*>(parentContainer.get())->reorderItems();
	}
	setNiceName(String((int)id->getValue()) + " - " + n);
	Brain::getInstance()->reconstructVirtuals = true;
}

void Cuelist::renumberCues() {
	Array<Cue*> temp;
	temp.addArray(cues.items);
	for (int i = 0; i < temp.size(); i++) {
		temp[i]->id->setValue(i+1);
	}
}

void Cuelist::loadRandom() {
	Array<Cue*> childCues = cues.getItemsWithType<Cue>();
	Array<Cue*> allowedCues;

	for (int i = 0; i < childCues.size(); i++) {
		if (childCues.getReference(i) != cueA && childCues[i]->canBeRandomlyCalled->getValue()) {
			allowedCues.add(childCues.getReference(i));
		}
	}

	int s = allowedCues.size();
	if (s > 0) {
		int r = Brain::getInstance()->mainRandom.nextInt(s);
		nextCue->setValueFromTarget(allowedCues[r]);
		fillTexts();
	}
}

void Cuelist::fillTexts() {
	if (isDeleting) {return;}
	conductorCurrentCueId->setValue(cueA == nullptr ? -1 : cueA->id->floatValue());
	conductorCurrentCueName->setValue(cueA == nullptr ? "" : cueA->niceName);
	conductorCurrentCueText->setValue(cueA == nullptr ? "" : cueA->cueText->getValue());
	Cue* n = dynamic_cast<Cue*>(nextCue->targetContainer.get());
	float nextId = nextCueId->getValue();
	if (nextId > 0) {
		for (int i = 0; i < cues.items.size() && cueB == nullptr; i++) {
			if ((float)cues.items[i]->id->getValue() >= nextId) {
				cueB = cues.items[i];
			}
		}
	}
	if (n == nullptr) {
		n = getNextCue();
	}
	if (n) {
		conductorNextCueId->setValue(n->id->floatValue());
		conductorNextCueGo->setValue(n->goText->getValue());
		conductorNextCueName->setValue(n->niceName);
	}
	else {
		conductorNextCueId->setValue(-1);
		conductorNextCueGo->setValue("");
		conductorNextCueName->setValue("");
	}
	if (id->intValue() == ConductorInfos::getInstance()->engine->conductorCuelistId->intValue()) {
		MessageManager::callAsync([](){
			ConductorInfos::getInstance()->updateContent();
			ConductorInfos::getInstance()->repaint();
			});
	}
}

Cue* Cuelist::getNextCue() {
	bool valid = false;
	if (cueA == nullptr) {
		if (cues.getItemsWithType<Cue>().size() > 0) {
			return cues.getItemsWithType<Cue>()[0];
		}
	}
	else {
		Array<Cue*> currentCues = cues.getItemsWithType<Cue>();
		for (int i = 1; i < currentCues.size() && !valid; i++) {
			if (currentCues[i - 1] == cueA) {
				return currentCues[i];
			}
		}
		String end = endAction->getValue();
		if (end == "loop") {
			return currentCues[0];
		}
	}
	return nullptr;
}

Cue* Cuelist::getNextChaserCue() {
	bool valid = false;
	if (cues.getItemsWithType<Cue>().size() == 0) {
		return nullptr;
	}
	if (cues.getItemsWithType<Cue>().size() == 1) {
		return cues.getItemsWithType<Cue>()[0];
	}

	if (chaserDirection->getValueData() == "random") {
		Array<Cue*> childCues = cues.getItemsWithType<Cue>();
		Array<Cue*> allowedCues;

		for (int i = 0; i < childCues.size(); i++) {
			if (childCues.getReference(i) != cueA) {
				allowedCues.add(childCues.getReference(i));
			}
		}

		int s = allowedCues.size();
		if (s > 0) {
			int r = Brain::getInstance()->mainRandom.nextInt(s);
			return allowedCues[r];
		}
	}

	if (cueA == nullptr) {
		if ((chaserIsGoingBackward && chaserDirection->getValueData() == "bounce") || chaserDirection->getValueData() == "reverse") {
			return cues.getItemsWithType<Cue>()[cues.getItemsWithType<Cue>().size()-1];
		}
		else {
			return cues.getItemsWithType<Cue>()[0];
		}
	}
	else {
		Array<Cue*> currentCues = cues.getItemsWithType<Cue>();
		if ((chaserIsGoingBackward && chaserDirection->getValueData() == "bounce") || chaserDirection->getValueData() == "reverse") {
			for (int i = 1; i < currentCues.size() && !valid; i++) {
				if (currentCues[i] == cueA) {
					return currentCues[i-1];
				}
			}
		}
		else {
			for (int i = 1; i < currentCues.size() && !valid; i++) {
				if (currentCues[i - 1] == cueA) {
					return currentCues[i];
				}
			}
		}
		
		if (chaserDirection->getValueData() == "normal") {
			return cues.getItemsWithType<Cue>()[0];
		}
		else if (chaserDirection->getValueData() == "reverse") {
			return cues.getItemsWithType<Cue>()[cues.getItemsWithType<Cue>().size() - 1];
		}
		else if (chaserDirection->getValueData() == "bounce") {
			if (chaserIsGoingBackward) {
				chaserIsGoingBackward = false;
				return cues.getItemsWithType<Cue>()[1];
			}
			else {
				chaserIsGoingBackward = true;
				return cues.getItemsWithType<Cue>()[cues.getItemsWithType<Cue>().size() - 2];
			}
		}
		return nullptr;
	}

}

Cue* Cuelist::getCueAfterId(float askedId)
{
	for (int i = 0; i < cues.items.size(); i++) {
		if ((float)cues.items[i]->id->getValue() >= askedId) {
			return cues.items[i];
		}
	}

	return nullptr;
}

void Cuelist::autoCreateChaser()
{
// here we go
	const MessageManagerLock mmlock;
	int nBuddy = chaseGenBuddying->intValue();
	int nBlocks = chaseGenBlocks->intValue();
	int nWings = chaseGenWings->intValue();

	int groupId = chaseGenGroup->intValue();

	Group* g = Brain::getInstance()->getGroupById(groupId);
	if (g == nullptr) {
		LOGERROR("you must specify a valid group ID");
		return;
	}
	g->selection.computeSelection();
	Array<SubFixture*> subfixtures = g->selection.computedSelectedSubFixtures;
	Array<std::shared_ptr<Array<SubFixture*>>> blocks;

	bool fixturesOnly = chaseGenFixturesOnly->boolValue();
	if (fixturesOnly) {
		subfixtures.clear();
		for (int i = 0; i < g->selection.computedSelectedFixtures.size(); i++) {
			Fixture* f = g->selection.computedSelectedFixtures[i];
			if (f->subFixturesContainer.size() > 0) {
				subfixtures.add(f->subFixturesContainer[0]);
			}
		}
	}

	int nsubFixtures = subfixtures.size();

	if (nsubFixtures == 0) {
		LOGERROR("your group is empty !");
		return;
	}
	int maxSize = 0;
	int maxBlock = 0;
	for (int i = 0; i < subfixtures.size(); i++) {
		int wingId = i*nWings/nsubFixtures;
		int blockId = i*(nWings*nBlocks) / nsubFixtures;

		while (blocks.size() - 1 < blockId) { blocks.add(std::make_shared<Array<SubFixture*>>()); }
		if (wingId % 2 == 1) {
			blocks[blockId]->insert(0, subfixtures[i]);
		}
		else {
			blocks[blockId]->add(subfixtures[i]);
		}
		maxSize = jmax(maxSize, blocks[blockId]->size());
		maxBlock = jmax(maxBlock, blockId);
	}

	int currentBuddy = nBuddy;
	Cue* cue = nullptr;
	kill();
	cues.clear();
	Array<Cue*> toAdd;
	CommandSelectionManager* csm = nullptr;
	for (int i = 0; i < maxSize; i++) {
		if (currentBuddy == nBuddy) {
			cue = new Cue();
			toAdd.add(cue);
			cue->commands.items[0]->values.loadJSONData(chaseGenValue.getJSONData());
			cue->setNiceName("empty");
			cue->editorIsCollapsed = true;
			cue->id->setValue(i+1);
			csm = &cue->commands.items[0]->selection;
			csm->clear();
			currentBuddy = 0;
		}
		currentBuddy++;
		for (int b = 0; b <= maxBlock; b++) {
			if (blocks[b]->size() > i) {
				SubFixture* sf = blocks[b]->getRawDataPointer()[i];
				CommandSelection* cs = csm->addItem();
				String name = "";
				cs-> valueFrom ->setValue( sf->parentFixture->id->intValue() );
				name = sf->parentFixture->id->stringValue();
				if (!fixturesOnly && sf->parentFixture->subFixtures.size() > 1) {
					cs->subSel->setValue(true);
					cs->subFrom->setValue(sf->subId);
					name += "."+String(sf->subId);
				}
				if (cue->niceName == "empty") {
					cue->setNiceName(name);
				}
				else {
					cue->setNiceName(cue->niceName +" + "+name);
				}
			}
		}
	}
	cues.addItems(toAdd, var(), false);
	blocks.clear();
}

void Cuelist::loadContent()
{
	Programmer* p = UserInputManager::getInstance()->getProgrammer(true);
	loadContent(p);
}

void Cuelist::loadContent(Programmer *p)
{
	if (cueA != nullptr) {
		cueA->loadContent(p);
	}
	else if (cues.items.size() > 0) {
		cues.items[0]->loadContent(p);
	}
}

void Cuelist::tempMergeProgrammer(Programmer* p, bool trackValues)
{
	if (p == nullptr) {return;}
	Array<SubFixtureChannel*> toUpdate;
	isComputing.enter();
	p->computing.enter();
	for (auto it = p->activeValues.begin(); it != p->activeValues.end(); it.next()) {
		if (it.getValue()->endValue() >= 0) {
			SubFixtureChannel* sfc = it.getKey();
			std::shared_ptr<ChannelValue> cv = std::make_shared<ChannelValue>();
			cv->values.set(1,it.getValue()->endValue());
			cv->canBeTracked = trackValues;
			activeValues.set(sfc, cv);
			sfc->cuelistOnTopOfStack(this);
		}
	}
	p->computing.exit();
	isComputing.exit();
}

void Cuelist::forceCueId(Cue* c, float askedId)
{
	for (int i = 0; i < cues.items.size(); i++) {
		if (cues.items[i] != c && cues.items[i]->id->floatValue() == askedId) {
			float nextId = i < cues.items.size()-2 ? cues.items[i+1]->id->floatValue() : cues.items[i]->id->floatValue()+2;
			float newId = askedId + ((nextId - askedId) / 2);
			cues.items[i]->id->setValue(newId);
		}
	}
	c->id->setValue(askedId);
}

void Cuelist::selectAsMainConductor()
{
	BKEngine* e = dynamic_cast<BKEngine*>(Engine::mainEngine);
	e->conductorCuelistId->setValue(id->intValue());
}

void Cuelist::mergeWithProgrammer(Programmer* p)
{
	Cue* targetCue = nullptr;
	if (cueA != nullptr) targetCue = cueA;
	else if (cues.items.size() > 0) targetCue = cues.items[0];

	if (targetCue != nullptr) targetCue->mergeContent(p);
}

void Cuelist::replaceWithProgrammer(Programmer* p)
{
	Cue* targetCue = nullptr;
	if (cueA != nullptr) targetCue = cueA;
	else if (cues.items.size() > 0) targetCue = cues.items[0];

	if (targetCue != nullptr) targetCue->replaceContent(p);
}

void Cuelist::exportInTextFile()
{
	FileChooser fc("Save conductor as HTML", File::getCurrentWorkingDirectory(), "*.html");
	if (!fc.browseForFileToSave(true)) return;

	String output = "";
	output += "<html><head>";
	output += "<style>";
	output += "h2 {margin-bottom:0;}";
	output += "ul {margin:0;}";
	output += ".cueText {font-style: italic;}";
	output += ".goText {font-weight: bold;margin:10px;}";
	output += "</style>";
	output += "</head><body>";
	output += "<h1>"+niceName+"</h1>";

	StringArray offtasks;

	for (Cue* c : cues.items) {
		if (c->goText->stringValue() != "") {
			output += "<div class='goText'>Event : "+ c->goText->stringValue()+"</div>";
		}
		output += "<h2>" + c->id->stringValue() + " - "+c->niceName+"</h2>";

		float delayIn = c->htpInDelay->floatValue();
		float delayOut = c->htpOutDelay->floatValue();
		float delayLTP = c->ltpDelay->floatValue();

		if (delayIn >= 0 && delayOut < 0 && delayLTP < 0) {
			output += "<div class='timing'>delay : " + c->htpInDelay->stringValue() + "</div>";
		}
		else if (delayIn >= 0 || delayOut >= 0 || delayLTP >= 0) {
			StringArray delays;
			output += "<div class='timing'> delay : ";
			if (delayIn > 0) delays.add("in " + c->htpInDelay->stringValue());
			if (delayOut > 0) delays.add("out " + c->htpOutDelay->stringValue());
			else if (delayIn > 0) delays.add("out " + c->htpInDelay->stringValue());
			if (delayLTP > 0) delays.add("ltp " + c->ltpDelay->stringValue());
			else if (delayIn > 0) delays.add("ltp " + c->htpInDelay->stringValue());
			output += delays.joinIntoString(", ");
			output += "</div>";
		}


		float fadeIn = c->htpInFade->floatValue();
		float fadeOut = c->htpOutFade->floatValue();
		float fadeLTP = c->ltpFade->floatValue();

		if (fadeIn >= 0 && fadeOut < 0 && fadeLTP < 0) {
			output += "<div class='timing'>fade : " + c->htpInFade->stringValue() + "</div>";
		}
		else if (fadeIn >= 0 || fadeOut >= 0 || fadeLTP >= 0) {
			StringArray fades;
			output += "<div class='timing'> fade : ";
			if (fadeIn > 0) fades.add("in " + c->htpInFade->stringValue());
			if (fadeOut > 0) fades.add("out " + c->htpOutFade->stringValue());
			else if (fadeIn > 0) fades.add("out " + c->htpInFade->stringValue());
			if (fadeLTP > 0) fades.add("ltp " + c->ltpFade->stringValue());
			else if (fadeIn > 0) fades.add("ltp " + c->htpInFade->stringValue());
			output += fades.joinIntoString(", ");
			output += "</div>";
		}


		if (c->cueText->stringValue() != "") {
			output += "<div class='cueText'>" + c->cueText->stringValue() + "</div>";
		}

		output += "<ul class='commands'>";
		if (c->releaseCurrentTracking->boolValue()) {
			output += "<li>release tracking</li>";
		}
		for (String cmd : offtasks) {
			output += "<li>" + cmd + "</li>";
		}
		offtasks.clear();
		StringArray commands;
		commands.addTokens(c->getCommandsText(true), "\n", "\"");
		for (String cmd : commands) {
			output += "<li>" + cmd + "</li>";
		}
		for (Task* t : c->tasks.items) {
			output += "<li>" + t->niceName + "</li>";
		}
		for (Task* t : c->tasksOffCue.items) {
			offtasks.add(t->niceName);
		}
		output += "</ul>";
	}

	output += "</body></html>";

	File file = fc.getResult();
	if (file.exists()) file.deleteFile();
	file.create();
	std::unique_ptr<OutputStream> os(file.createOutputStream());
	if (os == nullptr)
	{
		LOGERROR("Error saving document, please try again");
		return;
	}

	os->writeString(output);
	os->flush();

}

void Cuelist::tapTempo() {
	double now = Time::getMillisecondCounterHiRes();
	double delta = now - lastTapTempo;
	lastTapTempo = now;
	if (delta < 3000) {
		BKEngine* e = dynamic_cast<BKEngine*>(BKEngine::mainEngine);
		int historySize = e->tapTempoHistory->intValue();
		delta = delta / (double)chaserStepPerTap->getValue();
		tapTempoHistory.add(delta);
		while (tapTempoHistory.size() > historySize) tapTempoHistory.remove(0);
		delta = 0;
		for (int i = 0; i < tapTempoHistory.size(); i++) delta += tapTempoHistory[i];
		delta = delta / tapTempoHistory.size();
		double cpm = 60000. / delta;
		chaserSpeed->setValue(cpm);
	}
	else {
		tapTempoHistory.clear();
	}
}