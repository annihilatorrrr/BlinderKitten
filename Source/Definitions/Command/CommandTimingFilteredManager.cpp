#include "CommandTimingFilteredManager.h"
#include "../SubFixture/SubFixture.h"
#include "../../Brain.h"

CommandTimingFilteredManager::CommandTimingFilteredManager(const juce::String& name) :
    BaseManager(name)
{
    itemDataType = "CommandTimingFiltered";
    selectItemWhenCreated = false;

}

CommandTimingFilteredManager::~CommandTimingFilteredManager()
{
}

void CommandTimingFilteredManager::showStepSize(bool should)
{
    shouldShowStepSize = should;
    //for (CommandValue* cmdVal : items) {
    //    cmdVal->shouldShowStepSize = should;
    //    cmdVal->updateDisplay();
    //}
}

void CommandTimingFilteredManager::addItemInternal(CommandTimingFiltered* cv, juce::var data)
{
    //cv->shouldShowStepSize = shouldShowStepSize;
    //cv->updateDisplay();
}

