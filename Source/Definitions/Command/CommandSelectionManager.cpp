#include "CommandSelectionManager.h"
#include "../SubFixture/SubFixture.h"
#include "../../Brain.h"
#include "../../UserInputManager.h"
#include "Command.h"
#include "Definitions/Layout/Layout.h"

CommandSelectionManager::CommandSelectionManager() :
    BaseManager("Selections")
{
    itemDataType = "CommandSelection";
    selectItemWhenCreated = false; 

	getProgButton = addTrigger("Get from prog.", "");
	setProgButton = addTrigger("Set in prog.", "");
	getProgButton->hideInEditor = true;
	setProgButton->hideInEditor = true;

}

CommandSelectionManager::~CommandSelectionManager()
{
	computing.enter();
	computedSelectedSubFixtures.clear();
	subFixtureToPosition.clear();
	computing.exit();
}

void CommandSelectionManager::computeSelection() {
	Array<int> histo;
	computeSelection(histo);
}

void CommandSelectionManager::computeSelection(Array<int> groupHistory) {
	ScopedLock lock(computing);
	computedSelectedSubFixtures.clear();
	computedSelectedFixtures.clear();
	subFixtureToPosition.clear();
	subFixtureToPositionNoGap.clear();
	Brain* b = Brain::getInstance();
	Array<CommandSelection*> selections = getItemsWithType<CommandSelection>();
	for (int selId = 0; selId < selections.size(); selId++) {
		if (selections[selId]->enabled->boolValue()) {
			Array<SubFixture*> tempSelection;
			int idFrom = selections[selId]->valueFrom->getValue();
			int idThru = selections[selId]->thru->getValue() ? selections[selId]->valueTo->intValue() : idFrom;
			int mod = idFrom <= idThru ? 1 : -1;

			if (selections[selId]->targetType->getValue() == "fixture") {
				for (int id = idFrom; id != idThru + mod; id = id + mod) {
					Fixture* fixt = b->getFixtureById(id, true);
					if (fixt != nullptr) {
						if (selections[selId]->subSel->getValue()) {
							int subFrom = selections[selId]->subFrom->getValue();
							int subThru = selections[selId]->subThru->getValue() ? selections[selId]->subTo->intValue() : subFrom;
							int subMod = subFrom <= subThru ? 1 : -1;
							for (int subId = subFrom; subId != subThru + subMod; subId = subId + subMod) {
								SubFixture* s = fixt->getSubFixture(subId);
								if (s != nullptr) {
									tempSelection.add(s);
								}
							}
						}
						else {
							tempSelection.addArray(fixt->getAllSubFixtures());
						}
					}
				}
			}
			else if (selections[selId]->targetType->stringValue() == "group") {
				for (int id = idFrom; id != idThru + mod; id = id + mod) {
					Group* g = b->getGroupById(id, true);
					if (g!=nullptr) {
						int count = 0;
						int groupId = g->id->getValue();
						for (int histIndex = 0; histIndex < groupHistory.size(); histIndex++) {
							if (groupHistory[histIndex] == groupId) {
								count++;
							}
						}
						//if (groupHistory.indexOf(g->id->getValue()) == -1) {
						if (count < 10) {
							groupHistory.add(id);
							g->selection.computeSelection(groupHistory);
							g->selection.computing.enter();
							tempSelection.addArray(g->selection.computedSelectedSubFixtures);
							for (auto it = g->selection.subFixtureToPosition.begin(); it != g->selection.subFixtureToPosition.end(); it.next()) {
								subFixtureToPosition.set(it.getKey(), it.getValue());
							}
							g->selection.computing.exit();
						}
					}
				}
				if (selections[selId]->subSel->getValue()) {
					int subFrom = selections[selId]->subFrom->getValue();
					int subThru = selections[selId]->subThru->getValue() ? selections[selId]->subTo->intValue() : subFrom;

					int min = jmin(subFrom, subThru);
					int max = jmax(subFrom, subThru);

					Array<SubFixture*> tempSelectionFiltered;
					for (int i = 0; i < tempSelection.size(); i++) {
						if (tempSelection[i]->subId >= min && tempSelection[i]->subId <= max) {
							tempSelectionFiltered.add(tempSelection[i]);
						}
					}

					tempSelection.clear();
					tempSelection.addArray(tempSelectionFiltered);
				}

			}
			else {}
			
			// filter with divide and pattern...
			String pattern = selections[selId]->pattern->getValue();
			bool sym = selections[selId]->symmetry->getValue();
			int patternLength = pattern.length();
			int tempSelectionSize = tempSelection.size();

			if (selections[selId]->filter->getValue() == "reverse") {
				Array<SubFixture*> temp;
				temp.addArray(tempSelection);
				tempSelection.clear();
				for (int i = temp.size()-1; i >= 0; i--) {
					tempSelection.add(temp[i]);
				}
			}
			else if (selections[selId]->filter->getValue() == "pattern" && patternLength > 0) {
				Array<SubFixture*> filteredSelection;
				for (int i = 0; i < tempSelection.size(); i++) {
					int patternIndex = i % patternLength; 
					if (sym && i >= tempSelectionSize/2) {
						patternIndex = tempSelectionSize - 1 - i;
						patternIndex = patternIndex % patternLength;
					}
					char c = pattern[patternIndex];
					if (c == '1') {
						filteredSelection.add(tempSelection[i]);
					}
				}
				tempSelection = filteredSelection;
			}
			else if (selections[selId]->filter->getValue() == "divide" && patternLength > 0) {
				if (sym) {
					for (int i = patternLength - 1; i >= 0; i--) {
						pattern += pattern[i];
					}
					patternLength *= 2;
				}
				Array<SubFixture*> filteredSelection;
				for (int i = 0; i < tempSelection.size(); i++) {
					float patternIndex = 0;

					patternIndex = jmap(i,0,tempSelection.size(), 0, patternLength);
					char c = pattern[patternIndex];
					if (c == '1') {
						filteredSelection.add(tempSelection[i]);
					}
				}
				tempSelection = filteredSelection;

			}
			else if (selections[selId]->filter->getValue() == "shuffle") {
				Array<SubFixture*> filteredSelection;

				Random randObj;
				Random* r = &randObj;
				if ((int)selections[selId]->randomSeed->getValue() == 0) {
					r = &Brain::getInstance()->mainRandom;
				}
				else {
					r->setSeed((int)selections[selId]->randomSeed->getValue());
				}
			
				while (tempSelection.size() > 0) {
					int randIndex = r->nextInt(tempSelection.size());
					filteredSelection.add(tempSelection[randIndex]);
					tempSelection.remove(randIndex);
				}

				tempSelection = filteredSelection;

			}
			else if (selections[selId]->filter->getValue() == "begin") {
				Array<SubFixture*> filteredSelection;
				int n = jmin(selections[selId]->randomNumber->intValue(), tempSelection.size());
				for (int i = 0; i < n; i++) {
					filteredSelection.add(tempSelection[i]);
				}
				tempSelection = filteredSelection;
			}
			else if (selections[selId]->filter->getValue() == "end") {
				Array<SubFixture*> filteredSelection;
				int n = jmin(selections[selId]->randomNumber->intValue(), tempSelection.size());
				for (int i = tempSelectionSize-n; i < tempSelectionSize; i++) {
					filteredSelection.add(tempSelection[i]);
				}
				tempSelection = filteredSelection;
			}
			else if (selections[selId]->filter->getValue() == "random") {
				Array<SubFixture*> filteredSelection;
				HashMap<int, std::shared_ptr<Array<SubFixture*>>> indexToSubFixtures;
				HashMap<SubFixture*, int>subfixtureToIndex;

				int nBuddy = selections[selId]->randomBuddy->getValue();
				int nBlocks = selections[selId]->randomBlock->getValue();
				int nWings = selections[selId]->randomWing->getValue();
				int to = jmin(tempSelection.size(), (int)selections[selId]->randomNumber->getValue());
				int maxIndex = 0;
				Array<int> indexes;

				int realTot = ceil(tempSelection.size() / (float)nBuddy);
				float wingSize = realTot / (float)nWings;
				realTot = ceil(realTot / (float)nWings);
				int roundedWingSize = round(wingSize);
				roundedWingSize = jmax(roundedWingSize, 1);

				for (int chanIndex = 0; chanIndex < tempSelection.size(); chanIndex++) {
					int realIndex = chanIndex / nBuddy;

					int nWing = realIndex / wingSize;
					if (nWing % 2 == 1) {
						realIndex = realIndex % roundedWingSize;
						realIndex = wingSize - 1 - realIndex;
					}
					realIndex = realIndex * nBlocks;
					realIndex = realIndex % roundedWingSize;
					maxIndex = jmax(maxIndex, realIndex);
					indexes.addIfNotAlreadyThere(realIndex);
					if (!indexToSubFixtures.contains(realIndex)) {
						indexToSubFixtures.set(realIndex, std::make_shared<Array<SubFixture*>>());
					}
					indexToSubFixtures.getReference(realIndex)->add(tempSelection[chanIndex]);
					subfixtureToIndex.set(tempSelection[chanIndex], realIndex);
				}

				Random randObject;
				Random* r = &randObject;
				if (selections[selId]->randomSeed->intValue() == 0) {
					r = &Brain::getInstance()->mainRandom;
					if (tempSelection.size() - selections[selId]->lastRandom.size() > to) {
						for (int i = 0; i < selections[selId]->lastRandom.size(); i++) {
							tempSelection.removeAllInstancesOf(selections[selId]->lastRandom[i]);
						}
					}
				}
				else {
					r->setSeed((int)selections[selId]->randomSeed->getValue());
				}

				if (indexes.size() > to * 2 && selections[selId]->randomSeed->intValue() == 0) {
					for (int i = 0; i < selections[selId]->lastRandom.size(); i++) {
						if (subfixtureToIndex.contains(selections[selId]->lastRandom[i])) {
							indexes.removeAllInstancesOf(subfixtureToIndex.getReference(selections[selId]->lastRandom[i]));
						}
					}
				}

				for (int i = 0; i < to && indexes.size()>0; i++) {
					int randI = r->nextInt(indexes.size());
					//randI = r.nextInt(indexes.size());
					int index = indexes[randI];
					if (indexToSubFixtures.contains(index)) {
						auto a = indexToSubFixtures.getReference(index);
						for (int i2 = 0; i2 < a->size(); i2++) {
							filteredSelection.add(a->getRawDataPointer()[i2]);
						}
						indexToSubFixtures.remove(index);
						indexes.remove(randI);
					}
				}

				tempSelection = filteredSelection;
				selections[selId]->lastRandom.clear();
				selections[selId]->lastRandom.addArray(filteredSelection);
			}
			else if (selections[selId]->filter->getValue() == "bbw") {
				Array<SubFixture*> filteredSelection;

				int nBuddy = selections[selId]->randomBuddy->getValue();
				int nBlocks = selections[selId]->randomBlock->getValue();
				int nWings = selections[selId]->randomWing->getValue();
				int maxIndex = 0;
				Array<int> indexes;

				int realTot = ceil(tempSelection.size() / (float)nBuddy);
				float wingSize = realTot / (float)nWings;
				realTot = ceil(realTot / (float)nWings);
				int roundedWingSize = round(wingSize);
				roundedWingSize = jmax(roundedWingSize, 1);

				HashMap<SubFixture*, int> subFixtToIndex;

				for (int chanIndex = 0; chanIndex < tempSelection.size(); chanIndex++) {
					int realIndex = chanIndex / nBuddy;

					int nWing = realIndex / wingSize;
					if (nWing % 2 == 1) {
						realIndex = realIndex % roundedWingSize;
						realIndex = wingSize - 1 - realIndex;
					}
					realIndex = realIndex * nBlocks;
					realIndex = realIndex % roundedWingSize;
					maxIndex = jmax(maxIndex, realIndex);
					subFixtToIndex.set(tempSelection[chanIndex], realIndex);
				}

				for (auto it = subFixtToIndex.begin(); it != subFixtToIndex.end(); it.next()) {
					SubFixture* sf = it.getKey();
					int index = it.getValue();

					float vWithGap = jmap((float)index, (float)0, (float)(maxIndex+1), 0.f, 1.f);
					subFixtureToPosition.set(sf, vWithGap);
					float vNoGap = jmap((float)index, (float)0, (float)(maxIndex), 0.f, 1.f);
					subFixtureToPositionNoGap.set(sf, vNoGap);
				}

				selections[selId]->lastRandom.clear();
				selections[selId]->lastRandom.addArray(filteredSelection);
			}
			else if (selections[selId]->filter->getValue() == "outcondition") {
				Array<SubFixture*> filteredSelection;
				ChannelType* chan = dynamic_cast<ChannelType*>(selections[selId]->conditionChannel->targetContainer.get());
				float val = selections[selId]->conditionValue->floatValue();
				CommandSelection::ConditionTest test = selections[selId]->conditionTest->getValueDataAsEnum<CommandSelection::ConditionTest>();
				if (chan != nullptr) {
					for (int i = 0; i < tempSelection.size(); i++) {
						bool valid = false;
						SubFixture* sf = tempSelection[i];
						if (sf->channelsMap.contains(chan)) {
							SubFixtureChannel* sfc = sf->channelsMap.getReference(chan);
							if (sfc != nullptr) {
								float sfcVal = sfc->currentValue;
								switch (test) {
									case CommandSelection::EQUAL: valid = sfcVal == val; break;
									case CommandSelection::DIFFERENT: valid = sfcVal != val; break;
									case CommandSelection::LESS: valid = sfcVal < val; break;
									case CommandSelection::MORE: valid = sfcVal > val; break;
									case CommandSelection::LESSEQ: valid = sfcVal <= val; break;
									case CommandSelection::MOREEQ: valid = sfcVal >= val; break;
								}
							}
						}
						if (valid) {
							filteredSelection.add(sf);
						}
					}

				}
				tempSelection = filteredSelection;

			}

			else if (selections[selId]->filter->getValue() == "layoutdir") {
				Layout* l = Brain::getInstance()->getLayoutById(selections[selId]->layoutId->intValue());
				if (l != nullptr) {
					auto sfToPos = l->getSubfixturesRatioFromDirection(selections[selId]->layoutDirection->floatValue());
					float min = 0;
					float max = 1;
					min = 1; max = 0;
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							min = jmin(min, sfToPos->getReference(tempSelection[i]));
							max = jmax(max, sfToPos->getReference(tempSelection[i]));
						}
					}
					float maxGap = 0;
					float currentOffset = 0;
					float currentNextOffset = 1;
					bool search = true;
					while (search) {
						for (int i = 0; i < tempSelection.size(); i++) {
							if (sfToPos->contains(tempSelection[i])) {
								float thisVal = sfToPos->getReference(tempSelection[i]);
								if (thisVal > currentOffset && thisVal < currentNextOffset) {
									currentNextOffset = thisVal;
								}
							}
						}
						maxGap = jmax(maxGap, currentNextOffset-currentOffset);
						if (currentOffset == currentNextOffset) {
							search = false;
						}
						else {
							currentOffset = currentNextOffset;
							currentNextOffset = max;
						}
					}
					float maxNoGap = max;
					max += maxGap;
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							float v = sfToPos->getReference(tempSelection[i]);
							float vWithGap = jmap(v, min, max, 0.f, 1.f);
							subFixtureToPosition.set(tempSelection[i], vWithGap);
							float vNoGap = jmap(v, min, maxNoGap, 0.f, 1.f);
							subFixtureToPositionNoGap.set(tempSelection[i], vNoGap);
						}
					}
				}
			}
			else if (selections[selId]->filter->getValue() == "layoutcircle") {
				Layout* l = Brain::getInstance()->getLayoutById(selections[selId]->layoutId->intValue());
				if (l != nullptr) {
					Point<float> origin((float)selections[selId]->layoutCircleOrigin->getValue()[0], (float)selections[selId]->layoutCircleOrigin->getValue()[1]);
					auto sfToPos = l->getSubfixturesRatioFromOriginAndAngle(&origin, selections[selId]->layoutCircleStartAngle->floatValue(), !selections[selId]->layoutCircleCompleteRevolution->boolValue(), selections[selId]->layoutCircleCCW->boolValue());
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							float v = sfToPos->getReference(tempSelection[i]);
							subFixtureToPosition.set(tempSelection[i], v);
						}
					}
				}
			}
			else if (selections[selId]->filter->getValue() == "layoutpoint") {
				Layout* l = Brain::getInstance()->getLayoutById(selections[selId]->layoutId->intValue());
				if (l != nullptr) {
					Point<float> origin((float)selections[selId]->layoutCircleOrigin->getValue()[0], (float)selections[selId]->layoutCircleOrigin->getValue()[1]);
					auto sfToPos = l->getSubfixturesRatioFromOrigin(&origin);
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							float v = sfToPos->getReference(tempSelection[i]);
							subFixtureToPosition.set(tempSelection[i], v);
						}
					}
				}
			}
			else if (selections[selId]->filter->getValue() == "layoutperlin") {
				Layout* l = Brain::getInstance()->getLayoutById(selections[selId]->layoutId->intValue());
				if (l != nullptr) {
					auto sfToPos = l->getSubfixturesRatioPerlin(selections[selId]->layoutPerlinScale->floatValue(), selections[selId]->layoutPerlinSeed->floatValue());
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							float v = sfToPos->getReference(tempSelection[i]);
							subFixtureToPosition.set(tempSelection[i], v);
						}
					}
				}
			}
			else if (selections[selId]->filter->getValue() == "layoutwake") {
				Layout* l = Brain::getInstance()->getLayoutById(selections[selId]->layoutId->intValue());
				if (l != nullptr) {
					Point<float> origin((float)selections[selId]->layoutWakeAnchor->getValue()[0], (float)selections[selId]->layoutWakeAnchor->getValue()[1]);
					auto sfToPos = l->getSubfixturesRatioFromWake(&origin, selections[selId]->layoutDirection->floatValue(), selections[selId]->layoutWakeAngle->floatValue(), true);
					float min = 0;
					float max = 1;
					min = 1; max = 0;
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							min = jmin(min, sfToPos->getReference(tempSelection[i]));
							max = jmax(max, sfToPos->getReference(tempSelection[i]));
						}
					}
					float maxGap = 0;
					float currentOffset = 0;
					float currentNextOffset = 1;
					bool search = true;
					while (search) {
						for (int i = 0; i < tempSelection.size(); i++) {
							if (sfToPos->contains(tempSelection[i])) {
								float thisVal = sfToPos->getReference(tempSelection[i]);
								if (thisVal > currentOffset && thisVal < currentNextOffset) {
									currentNextOffset = thisVal;
								}
							}
						}
						maxGap = jmax(maxGap, currentNextOffset - currentOffset);
						if (currentOffset == currentNextOffset) {
							search = false;
						}
						else {
							currentOffset = currentNextOffset;
							currentNextOffset = max;
						}
					}
					float maxNoGap = max;
					max += maxGap;
					for (int i = 0; i < tempSelection.size(); i++) {
						if (sfToPos->contains(tempSelection[i])) {
							float v = sfToPos->getReference(tempSelection[i]);
							float vWithGap = jmap(v, min, max, 0.f, 1.f);
							subFixtureToPosition.set(tempSelection[i], vWithGap);
							float vNoGap = jmap(v, min, maxNoGap, 0.f, 1.f);
							subFixtureToPositionNoGap.set(tempSelection[i], vNoGap);
						}
					}
				}
			}

			if (selections[selId]->plusOrMinus->getValue() == "add") {
				computedSelectedSubFixtures.addArray(tempSelection);
			}
			else if (selections[selId]->plusOrMinus->getValue() == "remove") {
				computedSelectedSubFixtures.removeValuesIn(tempSelection);
			}
			else {}
		}
	}

	for (SubFixture* sf : computedSelectedSubFixtures) {
		if (sf->parentFixture != nullptr) computedSelectedFixtures.addIfNotAlreadyThere(sf->parentFixture);
	}
	//int n = subFixtureToPosition.size();
	//auto t = this;
	//LOG("coucou");
}

Array<ChannelType *> CommandSelectionManager::getControllableChannelsTypes() {
	Array<ChannelType*> chans;
	ScopedLock lock(computing);
	for (int i = 0; i < computedSelectedSubFixtures.size(); i++) {
		for (auto it = computedSelectedSubFixtures[i]->channelsMap.begin(); it != computedSelectedSubFixtures[i]->channelsMap.end(); it.next()) {
			if (it.getKey()!=nullptr && !chans.contains(it.getKey())) {
				chans.add(it.getKey());
			}
		}
	}
	return chans;
}

InspectableEditor* CommandSelectionManager::getEditorInternal(bool isRoot, Array<Inspectable*> inspectables)
{
	return new CommandSelectionManagerEditor(this, isRoot);
}

void CommandSelectionManager::triggerTriggered(Trigger* t)
{
	if (t == getProgButton) {
		UserInputManager::getInstance()->getProgrammer(true)->checkCurrentUserCommand();
		Command* currentCommand = UserInputManager::getInstance()->getProgrammer(true)->currentUserCommand;
		if (currentCommand != nullptr) {
			loadJSONData(currentCommand->selection.getJSONData());
			UserInputManager::getInstance()->commandSelectionChanged(currentCommand);
		}
	}
	if (t == setProgButton) {
		UserInputManager::getInstance()->getProgrammer(true)->checkCurrentUserCommand();
		Command* currentCommand = UserInputManager::getInstance()->targetCommand;
		if (currentCommand != nullptr) {
			currentCommand->selection.loadJSONData(getJSONData());
			UserInputManager::getInstance()->commandSelectionChanged(currentCommand);
		}
	}
}




CommandSelectionManagerEditor::CommandSelectionManagerEditor(CommandSelectionManager* item, bool isRoot) :
	GenericManagerEditor(item, isRoot),
	commandSel(item)
{
	getProgBT.reset(commandSel->getProgButton->createButtonUI());
	setProgBT.reset(commandSel->setProgButton->createButtonUI());
	addAndMakeVisible(getProgBT.get());
	addAndMakeVisible(setProgBT.get());
}

CommandSelectionManagerEditor::~CommandSelectionManagerEditor()
{
}

void CommandSelectionManagerEditor::resizedInternalHeader(Rectangle<int>& r)
{
	GenericManagerEditor::resizedInternalHeader(r);
	getProgBT->setBounds(r.withTrimmedRight(80).removeFromRight(80).reduced(2));
	setProgBT->setBounds(r.removeFromRight(80).reduced(2));
}
