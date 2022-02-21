/*
  ==============================================================================

    DataTransferManager.cpp
    Created: 29 Jan 2019 3:52:46pm
    Author:  no

  ==============================================================================
*/
#include "JuceHeader.h"
#include "DataTransferManager.h"
#include "../ChannelValue.h"
#include "../../Brain.h"
#include "../SubFixture/SubFixture.h"
#include "../Group/GroupManager.h"
#include "../Group/Group.h"
#include "../Preset/PresetManager.h"
#include "../Preset/Preset.h"
#include "../Cuelist/CuelistManager.h"
#include "../Cuelist/Cuelist.h"
#include "../Cue/Cue.h"
#include "../Programmer/Programmer.h"
#include "../Command/CommandSelectionManager.h"
#include "../Command/CommandSelection.h"
#include "../ChannelFamily/ChannelFamilyManager.h"
#include "../ChannelFamily/ChannelFamily.h"
#include "../TimingPreset/TimingPresetManager.h"
#include "../TimingPreset/TimingPreset.h"

juce_ImplementSingleton(DataTransferManager)

DataTransferManager::DataTransferManager() :
	BaseItem("Data Transfer Manager")
{
    sourceType = addEnumParameter("Source Type", "Type of the data source");
    sourceType->addOption("Group", "Group");
    sourceType->addOption("Preset", "Preset");
    // sourceType->addOption("Timing Preset", "TimingPreset");
    sourceType->addOption("Cuelist", "Cuelist");
    sourceType->addOption("Programmer", "Programmer");
    sourceId = addIntParameter("Source Id", "ID of the source", 0, 0);
    targetType = addEnumParameter("Target Type", "Type of the data target");
    targetType->addOption("Group", "Group");
    targetType->addOption("Preset", "Preset");
    // targetType->addOption("Timing Preset", "TimingPreset");
    targetType->addOption("Cuelist", "Cuelist");
    targetType->addOption("Programmer", "Programmer");

    targetUserId = addIntParameter("Target Id", "ID of the target", 0, 0);

    paramfilter = addTargetParameter("Param filter", "Filter recorded values in preset by parameter family", ChannelFamilyManager::getInstance());
    paramfilter->targetType = TargetParameter::CONTAINER;
    paramfilter->maxDefaultSearchLevel = 0;

    groupCopyMode = addEnumParameter("Group merge mode", "Group record mode");
    groupCopyMode->addOption("Merge", "merge");
    groupCopyMode->addOption("Replace", "replace");

    presetCopyMode = addEnumParameter("Preset merge mode", "Preset record mode");
    presetCopyMode->addOption("Merge", "merge");
    presetCopyMode->addOption("Replace", "replace");

    cuelistCopyMode = addEnumParameter("Cuelist merge mode", "Cuelist record mode");
    cuelistCopyMode->addOption("Add new cue", "add");
    cuelistCopyMode->addOption("Update current cue", "update");
    cuelistCopyMode->addOption("Replace current cue", "replace");

    go = addTrigger("Transfer Data", "Run the data transfer");
    updateDisplay();
}

void DataTransferManager::updateDisplay() {
    String tgt = targetType->getValue();
    paramfilter->hideInEditor = !(tgt == "Preset");
    groupCopyMode->hideInEditor = !(tgt == "Group");
    presetCopyMode->hideInEditor = !(tgt == "Preset");
    cuelistCopyMode->hideInEditor = !(tgt == "Cuelist");
    queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

void DataTransferManager::onContainerParameterChangedInternal(Parameter* p) {
    updateDisplay();
}

DataTransferManager::~DataTransferManager()
{
}

void DataTransferManager::triggerTriggered(Trigger* t) {
    if (t == go) {
        execute();
    }
}


void DataTransferManager::execute() {
    String srcType = sourceType->getValue();
    String trgType = targetType->getValue();
    bool valid = false;

    int tId = targetUserId->getValue();

    if (srcType == "Programmer") {
        Programmer* source = Brain::getInstance()->getProgrammerById(sourceId->getValue());
        if (source == nullptr) { LOG("Invalid Programmer ID"); return; }
        if (trgType == "Group") {
            valid = true;
            Group* target = Brain::getInstance()->getGroupById(tId);
            if (target == nullptr) {
                target = GroupManager::getInstance()->addItem(new Group());
                target->id->setValue(tId);
                target->setNiceName("Group " + String(int(target->id->getValue())));
            }

            if (groupCopyMode->getValue() == "replace") {
                target->selection.clear(); // erase data
            }

            for (int commandIndex = 0; commandIndex < source->commands.items.size(); commandIndex++) {
                CommandSelectionManager* selections = &source->commands.items[commandIndex]->selection;
                for (int selectionIndex = 0; selectionIndex < selections->items.size(); selectionIndex++) {
                    CommandSelection* selection = selections->items[selectionIndex];
                    CommandSelection* newSel = target->selection.addItem();
                    newSel->loadJSONData(selection->getJSONData());
                }
            }
        }
        else if (trgType == "Preset") {
            valid = true;
            Preset* target = Brain::getInstance()->getPresetById(tId);
            if (target == nullptr) {
                target = PresetManager::getInstance()->addItem(new Preset());
                target->id->setValue(tId);
                target->setNiceName("Preset " + String(int(target->id->getValue())));
                target->updateName();
                target->subFixtureValues.clear();
            }

            if (presetCopyMode->getValue() == "replace") {
                target->subFixtureValues.clear(); // erase data
            }

            ChannelFamily* filter = dynamic_cast<ChannelFamily*>(paramfilter->targetContainer.get());

            source->computeValues();
            for (auto it = source->computedValues.begin(); it != source->computedValues.end(); it.next()) {
                // HashMap<SubFixtureChannel*, ChannelValue*> computedValues;
                SubFixtureChannel* chan = it.getKey();
                ChannelValue* cValue = it.getValue();
                ChannelFamily* chanType = dynamic_cast<ChannelFamily*>(chan->channelType->parentContainer->parentContainer.get());

                if (cValue->endValue != -1 && (filter == nullptr || filter == chanType)) {

                    int subfixtId = chan->parentSubFixture->subId;
                    int fixtId = dynamic_cast<Fixture*>(chan->parentSubFixture->parentFixture)->id->getValue();
                    PresetSubFixtureValues* pfv = nullptr;

                    for (int i = 0; i < target->subFixtureValues.items.size(); i++) {
                        PresetSubFixtureValues* temp = target->subFixtureValues.items[i];
                        if ((int)temp->targetFixtureId->getValue() == fixtId && (int)temp->targetSubFixtureId->getValue() == subfixtId) {
                            pfv = target->subFixtureValues.items[i];
                        }
                    }
                    if (pfv == nullptr) {
                        pfv = target->subFixtureValues.addItem();
                        pfv->targetFixtureId->setValue(fixtId);
                        pfv->targetSubFixtureId->setValue(subfixtId);
                        pfv->values.clear();
                    }

                    PresetValue* pv = nullptr;
                    for (int i = 0; i < pfv->values.items.size(); i++) {
                        if (dynamic_cast<ChannelType*>(pfv->values.items[i]->param->targetContainer.get()) == chan->channelType) {
                            pv = pfv->values.items[i];
                        }
                    }
                    if (pv == nullptr) {
                        pv = pfv->values.addItem();
                        pv->param->setValueFromTarget(chan->channelType);
                    }
                    pv->paramValue->setValue(cValue->endValue);
                }
            target->updateDisplay();
            }
        }
        else if (trgType == "Cuelist") {
            valid = true;
            Cuelist* target = Brain::getInstance()->getCuelistById(tId);
            if (target == nullptr) {
                target = CuelistManager::getInstance()->addItem(new Cuelist());

                target->id->setValue(tId);
                target->userName->setValue("Cuelist "+target->id->getValue().toString());
                target->cues.clear();
            }

            String copyMode = cuelistCopyMode->getValue();
            Cue* targetCue;

            if (copyMode == "add") {
                targetCue = target->cues.addItem();
                targetCue->commands.clear();
            }
            else {
                targetCue = target->cueA;
                if (targetCue == nullptr) {
                    targetCue = target->cues.items[0];
                    if (targetCue == nullptr){
                        targetCue = target->cues.addItem();
                        targetCue->commands.clear();
                    }
                }
            }
            if (copyMode == "replace") {
                targetCue->commands.clear();
            }

            for (int i = 0; i < source->commands.items.size(); i++) {
                Command* c = targetCue->commands.addItem();
                c->loadJSONData(source->commands.items[i]->getJSONData());
            }
        }

    }


    else if (srcType == "Cuelist") {

    }
    else if (srcType == "Preset") {

    }
    else if (srcType == "Group") {

    }


    if (!valid) {
        LOGWARNING("target type and source type are not compatible");
    }
    else {
    }


}