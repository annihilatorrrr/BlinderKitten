/*
  ==============================================================================

    CuelistGridView.cpp
    Created: 19 Feb 2022 12:19:42am
    Author:  No

  ==============================================================================
*/

#include <JuceHeader.h>
#include "CuelistGridView.h"
#include "Brain.h"
#include "Definitions/Cuelist/Cuelist.h"
#include "Definitions/Cuelist/CuelistManager.h"
#include "UserInputManager.h"
#include "DataTransferManager/DataTransferManager.h"

//==============================================================================
CuelistGridViewUI::CuelistGridViewUI(const String& contentName):
    ShapeShifterContent(CuelistGridView::getInstance(), contentName)
{
    
}

CuelistGridViewUI::~CuelistGridViewUI()
{
}

juce_ImplementSingleton(CuelistGridView);

CuelistGridView::CuelistGridView()
{
    numberOfCells = 200;
    targetType = "Cuelist";
    CuelistManager::getInstance()->addAsyncManagerListener(this);

}

CuelistGridView::~CuelistGridView()
{
    if (CuelistManager::getInstanceWithoutCreating() != nullptr) CuelistManager::getInstance()->removeAsyncManagerListener(this);
}

void CuelistGridView::updateCells() {
    for (int i = 0; i < numberOfCells; i++) {
        Cuelist* g = Brain::getInstance()->getCuelistById(i+1);
        if (g != nullptr) {
            gridButtons[i]->removeColour(TextButton::buttonColourId);
            gridButtons[i]->removeColour(TextButton::textColourOnId);
            gridButtons[i]->removeColour(TextButton::textColourOffId);

            gridButtons[i]->setButtonText(g->userName->stringValue());
        }
        else {
            gridButtons[i]->setButtonText("");
            gridButtons[i]->setColour(TextButton::buttonColourId, Colour(40, 40, 40));
            gridButtons[i]->setColour(TextButton::textColourOnId, Colour(96, 96, 96));
            gridButtons[i]->setColour(TextButton::textColourOffId, Colour(96, 96, 96));

        }
    }
    updateButtons();
}

void CuelistGridView::updateButtons()
{
    const MessageManagerLock mmLock;
    for (int i = 0; i < numberOfCells; i++) {
        Cuelist* c = Brain::getInstance()->getCuelistById(i+1);
        if (c != nullptr) {
            if (c->isCuelistOn->boolValue()) {
                gridButtons[i]->setColour(TextButton::buttonColourId, juce::Colour(64, 80, 64));
            }
            else {
                gridButtons[i]->removeColour(TextButton::buttonColourId);
            }
        }
    }

}

void CuelistGridView::showContextMenu(int id)
{
    Cuelist* target = Brain::getInstance()->getCuelistById(id);
    PopupMenu p;
    if (target != nullptr) {
        p.addItem("Go", [target]() {target->userGo(); });
        p.addItem("Load", [target]() {target->showLoad(); });
        p.addItem("Load and go", [target]() {target->showLoadAndGo(); });
        if (target->cueA != nullptr) {
            p.addItem("Off", [target]() {target->off(); });
            p.addSeparator();
            p.addItem("Temp merge track", [target]() {target->tempMergeProgrammer(UserInputManager::getInstance()->getProgrammer(true), true); });
            p.addItem("Temp merge no track", [target]() {target->tempMergeProgrammer(UserInputManager::getInstance()->getProgrammer(true), false); });
        }
        p.addSeparator();
        p.addItem("Load content", [target]() {target->loadContent(UserInputManager::getInstance()->getProgrammer(true)); });
        p.addSeparator();
        p.addItem("Merge", [target]() {target->mergeWithProgrammer(UserInputManager::getInstance()->getProgrammer(true)); });
        p.addItem("Replace", [target]() {target->replaceWithProgrammer(UserInputManager::getInstance()->getProgrammer(true)); });
        p.addSeparator();
        p.addItem("Select as main conductor", [target]() {target->selectAsMainConductor(); });

    }
    else {
        p.addItem("Add", [id](){DataTransferManager::getInstance()->editObject("cuelist", id); });
    }
    p.showMenuAsync(PopupMenu::Options(), [this](int result) {});
}

void CuelistGridView::newMessage(const CuelistManager::ManagerEvent& e)
{
    updateCells();
}
