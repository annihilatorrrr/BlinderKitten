/*
  ==============================================================================

    Assistant.h
    Created: 29 Jan 2019 3:52:46pm
    Author:  bkupe

  ==============================================================================
*/

#pragma once
#include "JuceHeader.h"
#include "Definitions/Command/CommandValueManager.h"

class Assistant :
	public BaseItem,
	Thread
{
public:
	juce_DeclareSingleton(Assistant, true);

	Assistant();
	~Assistant();

	void run() override;

	ControllableContainer patcherCC;
	TargetParameter * patcherFixtureType;
	IntParameter* patcherAmount;
	StringParameter * patcherName;
	IntParameter* patcherFirstId;
	TargetParameter* patcherInterface;
	IntParameter* patcherFirstAddress;
	IntParameter* patcherAddressInterval;
	BoolParameter* patcherMakeGroup;
	Trigger* patcherBtn;

	ControllableContainer paletteMakerCC;
	IntParameter* paletteGroupId;
	IntParameter* paletteFirstPresetId;
	IntParameter* paletteLastPresetId;
	IntParameter* paletteTimingPresetId;
	IntParameter* paletteCuelistId;
	StringParameter* paletteName;
	BoolParameter* paletteKeepEmpty;
	Trigger* paletteBtn;

	ControllableContainer soloPaletteMakerCC;
	IntParameter* soloPalettePoolId;
	IntParameter* soloPaletteCuelistId;
	StringParameter* soloPaletteName;
	Trigger* soloPaletteBtn;

	ControllableContainer masterMakerCC;
	IntParameter* masterFirstGroupId;
	IntParameter* masterLastGroupId;
	CommandValueManager masterValue;
	Trigger* masterBtn;

	ControllableContainer fixtureSwapperCC;
	TargetParameter* swapperOld;
	TargetParameter* swapperNew;
	Trigger* swapperBtn;

	ControllableContainer midiMapperCC;
	TargetParameter* midiMapperTargetInterface;
	EnumParameter* midiMapperTargetType = nullptr;
	IntParameter* midiMapperTargetId;
	IntParameter* midiMapperPageNumber;
	Trigger* midiMapperBtn;

	ControllableContainer asciiCC;
	BoolParameter* asciiPatch;
	BoolParameter* asciiGroups;
	BoolParameter* asciiGroupValuesAsPreset;
	BoolParameter* asciiCues;
	BoolParameter* asciiSubs;
	BoolParameter* asciiRespectCueNumbers;
	BoolParameter* asciiEraseCuelist;
	IntParameter* asciiCuelistId;
	TargetParameter* asciiChannelFixtureType;
	TargetParameter* asciiDimmerChannel;
	Trigger* importAsciiBtn;
	Trigger* exportAsciiBtn;

	ControllableContainer controlsCC;
	Trigger* offCuelistsBtn;
	Trigger* killCuelistsBtn;
	Trigger* stopEffectsBtn;
	Trigger* stopCarouselsBtn;
	IntParameter* randomSeed;
	Trigger* resetRandomBtn;
	Trigger* loadRunningCuelistsBtn;

	bool pleasePatchFixtures = false;
	bool pleaseCreatePalette = false;
	bool pleaseCreateMasters = false;
	bool pleaseSwapFixtures = false;
	bool pleaseCreateSoloPalette = false;
	bool pleaseCreateMidiMappings = false;
	bool pleaseImportAscii = false;

	void triggerTriggered(Trigger* t);
	void onContainerParameterChangedInternal(Parameter* p);
	void updateDisplay();
	void onControllableFeedbackUpdateInternal(ControllableContainer* cc, Controllable* c);

	void patchFixtures();
	void createPalette();
	void createMasters();
	void createMidiMappings();
	void swapFixtures();
	void createSoloPalette();

	void importAscii();
	void exportAscii();

	float asciiLevelToFloat(String asciiLevel);
};