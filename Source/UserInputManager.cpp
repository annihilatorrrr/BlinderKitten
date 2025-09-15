/*
  ==============================================================================

    UserInputManager.cpp
    Created: 14 Nov 2021 10:42:56pm
    Author:  No

  ==============================================================================
*/


#include "UserInputManager.h"
#include "Brain.h"
#include "Definitions/Cuelist/Cuelist.h"
#include "Definitions/Effect/Effect.h"
#include "Definitions/Carousel/Carousel.h"
#include "Definitions/Programmer/Programmer.h"
#include "Definitions/Programmer/ProgrammerManager.h"
#include "Definitions/Command/Command.h"
#include "Definitions/Command/CommandSelectionManager.h"
#include "Definitions/Command/CommandSelection.h"
#include "Definitions/Command/CommandValue.h"
#include "Definitions/ChannelFamily/ChannelType/ChannelType.h"
#include "Definitions/ChannelFamily/ChannelFamily.h"
#include "Definitions/ChannelFamily/ChannelFamilyManager.h"
#include "Definitions/Fixture/FixtureManager.h"
#include "Definitions/Fixture/Fixture.h"
#include "UI/Encoders.h"
#include "UI//EncodersMult/EncodersMult.h"
#include "UI/VirtualButtons/VirtualButtonManager.h"
#include "UI/VirtualFaders/VirtualFaderColManager.h"
#include "UI/VirtualButtons/VirtualButtonGrid.h"
#include "UI/VirtualFaders/VirtualFaderColGrid.h"
#include "UI/InputPanel.h"
#include "Definitions/Assistant/Assistant.h"
#include "UI/BKColorPicker.h"
#include "UI/LayoutViewer.h"

juce_ImplementSingleton(UserInputManager);

UserInputManager::UserInputManager()
{
	currentProgrammer = nullptr;
}

UserInputManager::~UserInputManager() {
}

void UserInputManager::processInput(String s) {
	if (EncodersMult::getInstance()->targetCommandManager == nullptr) {
		EncodersMult::getInstance()->targetChanged();
	}

	if (s.toLowerCase() == "assistant") {
		Assistant::getInstance()->selectThis();
	}
	if (s.toLowerCase() == "label") {
		dynamic_cast<BKEngine*>(Engine::mainEngine)->showLabelAndTime();
	}
	if (s.toLowerCase() == "midilock") {
		InputPanel::getInstance()->paramInLock->setValue(!InputPanel::getInstance()->paramInLock->boolValue());
	}
	if (s.toLowerCase() == "blackout") {
		InputPanel::getInstance()->paramBlackOut->setValue(!InputPanel::getInstance()->paramBlackOut->boolValue());
	}

	getProgrammer(true)->processUserInput(s);
}

void UserInputManager::processMessage(const juce::OSCMessage& m, const juce::String& clientId)
{
	StringArray aList;
	String address = m.getAddressPattern().toString().toLowerCase();
	//LOG(address);
	aList.addTokens(m.getAddressPattern().toString().toLowerCase(), "/", "\"");
	if (aList.size() < 2) return;
	aList.removeString("");

	/*
	cuelist
		go
		goBack
		goRandom
		off
		kill
		load
		flash
		swop
		loadandgo
		loadrandom
		htpLevel
		ltplevel
		flashLevel
		taptempo
	effect
		start
		stop
		speed
		size
		tap tempo
	carousel
		start
		stop
		speed
		size
		tap tempo
	mapper
		start
		stop
		size

	virtbutton page col row
	virtabove page col row
	virtbelow page col row
	virtfader page col row
	virtrotary page col row

	
		/vfader/3/rotary/2
		/vbutton/4/5
	*/

	String firstWord = aList[0];
	if (firstWord == "key") {
		processInput(aList[1]);
	}
	else if (firstWord == "grandmaster" && m.size() > 0) {
		float val = OSCHelpers::getFloatArg(m[0]);
		InputPanel::getInstance()->paramGrandMaster->setValue(val);
	}
	else if (firstWord == "cuelist" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Cuelist* target = Brain::getInstance()->getCuelistById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
			if (action == "go") { 
				if (aList.size() > 3) {
					float targetCue = (float)((var)aList[3]);
					target->nextCueId->setValue(targetCue);
					target->userGo();
				}
				else {
					target->userGo();
				}
			}
			else if (action == "goback") { target->goBack(); }
			else if (action == "gorandom") { target->goRandom(); }
			else if (action == "off") { target->off(); }
			else if (action == "kill") { target->kill(); }
			else if (action == "toggle") { target->toggle(); }
			else if (action == "load") {
				if (aList.size() > 3) {
					float targetCue = (float)((var)aList[3]);
					target->nextCueId->setValue(targetCue);
				}
				else {
					target->showLoad();
				}
			}
			else if (action == "loadandgo") { 
				if (aList.size() > 3) {
					float targetCue = (float)((var)aList[3]);
					target->nextCueId->setValue(targetCue);
					target->userGo();
				}
				else {
					target->showLoadAndGo();
				}
			}
			else if (action == "loadrandom") { target->loadRandom(); }
			else if (action == "taptempo") { target->tapTempo(); }
			else if (action == "flash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "swop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "timedflash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "timedswop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "htplevel" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->HTPLevel->setValue(val);
			}
			else if (action == "ltplevel" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->LTPLevel->setValue(val);
			}
			else if (action == "flashlevel" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->flashLevel->setValue(val);
			}
			else if (action == "chaserspeed" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->chaserSpeed->setValue(val);
			}
			else if (action == "doublespeed") {
				target->chaserSpeed->setValue(target->chaserSpeed->floatValue() * 2);
			}
			else if (action == "halfspeed") {
				target->chaserSpeed->setValue(target->chaserSpeed->floatValue() / 2);
			}

		}
		else {
			LOGWARNING("Cuelist " + String(targetNumber) + " doesn't exist");
		}
	}
	else if (firstWord == "effect" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Effect* target = Brain::getInstance()->getEffectById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
				if (action == "start") { target->userStart(); }
			else if (action == "stop") { target->stop(); }
			else if (action == "taptempo") { target->tapTempo(); }
			else if (action == "size" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->sizeValue->setValue(val);
			}
			else if (action == "speed" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->speed->setValue(val);
			}
			else if (action == "flash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "swop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "timedflash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "timedswop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "doublespeed") {
					target->speed->setValue(target->speed->floatValue() * 2);
				}
			else if (action == "halfspeed") {
					target->speed->setValue(target->speed->floatValue() / 2);
				}
		}
		else {
			LOGWARNING("Effect " + String(targetNumber) + " doesn't exist");
		}
	}
	else if (firstWord == "carousel" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Carousel* target = Brain::getInstance()->getCarouselById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
			if (action == "start") { target->userStart(); }
			else if (action == "stop") { target->stop(); }
			else if (action == "taptempo") { target->tapTempo(); }
			else if (action == "size" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->sizeValue->setValue(val);
			}
			else if (action == "speed" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->speed->setValue(val);
			}
			else if (action == "flash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "swop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "timedflash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false);
				}
				else {
					target->flash(false, false);
				}
			}
			else if (action == "timedswop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true);
				}
				else {
					target->flash(false, true);
				}
			}
			else if (action == "doublespeed") {
				target->speed->setValue(target->speed->floatValue() * 2);
			}
			else if (action == "halfspeed") {
				target->speed->setValue(target->speed->floatValue() / 2);
			}
		}
		else {
			LOGWARNING("Carousel " + String(targetNumber) + " doesn't exist");
		}
	}
	else if (firstWord == "mapper" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Mapper* target = Brain::getInstance()->getMapperById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
			if (action == "start") { target->start(); }
			else if (action == "stop") { target->stop(); }
			else if (action == "size" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->sizeValue->setValue(val);
			}
		}
		else {
			LOGWARNING("Mapper " + String(targetNumber) + " doesn't exist");
		}
	}

	else if (firstWord == "tracker" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Tracker* target = Brain::getInstance()->getTrackerById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
			if (action == "start") { target->start(); }
			else if (action == "stop") { target->stop(); }
			else if (action == "size" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->sizeValue->setValue(val);
			}
			else if (action == "xyz" && m.size()>2) { 
				float x = OSCHelpers::getFloatArg(m[0]);
				float y = OSCHelpers::getFloatArg(m[1]);
				float z = OSCHelpers::getFloatArg(m[2]);
				target->targetPosition->setVector(x, y, z);
			}
		}
		else {
			LOGWARNING("Tracker " + String(targetNumber) + " doesn't exist");
		}
	}
	else if (firstWord == "bundle" && aList.size() > 2) {
		int targetNumber = (int)((var)aList[1]);
		Bundle* target = Brain::getInstance()->getBundleById(targetNumber);
		if (target != nullptr) {
			String action = aList[2].toLowerCase();
			if (action == "start") { target->start(); }
			else if (action == "stop") { target->stop(); }
			else if (action == "taptempo") { target->tapTempo(); }
			else if (action == "size" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				bool size = m.size() > 1 ? OSCHelpers::getBoolArg(m[1]) : true;
				bool HTP = m.size() > 2 ? OSCHelpers::getBoolArg(m[2]) : true;
				bool LTP = m.size() > 3 ? OSCHelpers::getBoolArg(m[3]) : false;
				bool flash = m.size() > 4 ? OSCHelpers::getBoolArg(m[4]) : false;
				target->setSize(val, size, HTP, LTP, flash);
			}
			else if (action == "speed" && m.size() > 0) {
				float val = OSCHelpers::getFloatArg(m[0]);
				target->setSpeed(val, true);
			}
			else if (action == "flash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false, false);
				}
				else {
					target->flash(false, false, false);
				}
			}
			else if (action == "swop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, false, true);
				}
				else {
					target->flash(false, false, true);
				}
			}
			else if (action == "timedflash" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true, false);
				}
				else {
					target->flash(false, true, false);
				}
			}
			else if (action == "timedswop" && m.size() > 0) {
				int val = OSCHelpers::getIntArg(m[0]);
				if (val == 1) {
					target->flash(true, true, true);
				}
				else {
					target->flash(false, true, true);
				}
			}
			else if (action == "doublespeed") {
				target->speedMult(2);
			}
			else if (action == "halfspeed") {
				target->speedMult(0.5);
			}
		}
		else {
			LOGWARNING("Bundle " + String(targetNumber) + " doesn't exist");
		}
	}

	else if (firstWord == "virtbutton" && aList.size() > 2 && m.size() > 0) {
		int page = VirtualButtonGrid::getInstance()->page;
		int col = (int)((var)aList[1]);
		int row = (int)((var)aList[2]);
		float value = OSCHelpers::getFloatArg(m[0]);
		if (aList.size() == 4) {
			page = (int)((var)aList[1]);
			page = page == 0 ? VirtualButtonGrid::getInstance()->page : page;
			col = (int)((var)aList[2]);
			row = (int)((var)aList[3]);
		}
		VirtualButtonManager::getInstance()->setButtonValue(page, col, row, value, "");
	}

	else if (firstWord == "virtabove" && aList.size() > 2 && m.size() > 0) {
		int page = VirtualFaderColGrid::getInstance()->page;
		int col = (int)((var)aList[1]);
		int row = (int)((var)aList[2]);
		float value = OSCHelpers::getFloatArg(m[0]);
		if (aList.size() == 4) {
			page = (int)((var)aList[1]);
			page = page == 0 ? VirtualFaderColGrid::getInstance()->page : page;
			col = (int)((var)aList[2]);
			row = (int)((var)aList[3]);
		}
		VirtualFaderColManager::getInstance()->setAboveButtonValue(page, col, row, value, "");
	}

	else if (firstWord == "virtbelow" && aList.size() > 2 && m.size() > 0) {
		int page = VirtualFaderColGrid::getInstance()->page;
		int col = (int)((var)aList[1]);
		int row = (int)((var)aList[2]);
		float value = OSCHelpers::getFloatArg(m[0]);
		if (aList.size() == 4) {
			page = (int)((var)aList[1]);
			page = page == 0 ? VirtualFaderColGrid::getInstance()->page : page;
			col = (int)((var)aList[2]);
			row = (int)((var)aList[3]);
		}
		VirtualFaderColManager::getInstance()->setBelowButtonValue(page, col, row, value, "");
	}

	else if (firstWord == "virtrotary" && aList.size() > 2 && m.size() > 0) {
		int page = VirtualFaderColGrid::getInstance()->page;
		int col = (int)((var)aList[1]);
		int row = (int)((var)aList[2]);
		float value = OSCHelpers::getFloatArg(m[0]);
		if (aList.size() == 4) {
			page = (int)((var)aList[1]);
			page = page == 0 ? VirtualFaderColGrid::getInstance()->page : page;
			col = (int)((var)aList[2]);
			row = (int)((var)aList[3]);
		}
		VirtualFaderColManager::getInstance()->setRotaryValue(page, col, row, value, "", 0, false);
	}

	else if (firstWord == "virtfader" && aList.size() > 1 && m.size()>0) {
		int page = VirtualFaderColGrid::getInstance()->page;
		int col = (int)((var)aList[1]);
		float value = OSCHelpers::getFloatArg(m[0]);
		if (aList.size() == 3) {
			page = (int)((var)aList[1]);
			page = page == 0 ? VirtualFaderColGrid::getInstance()->page : page;
			col = (int)((var)aList[2]);
		}
		VirtualFaderColManager::getInstance()->setFaderValue(page, col, value, "", 0, false);
	}

	else if (firstWord == "encoders" && aList.size() > 1) {
		if (aList[1] == "hl") { Encoders::getInstance()->paramHighLight->setValue(!Encoders::getInstance()->paramHighLight->boolValue()); }
		else if (aList[1] == "blind") { Encoders::getInstance()->paramBlind->setValue(!Encoders::getInstance()->paramBlind->boolValue()); }
		else if (aList[1] == "mode") { Encoders::getInstance()->paramMode->trigger(); }
		else if (aList[1] == "encoderrange") { Encoders::getInstance()->paramEncoderRange->trigger(); }
		else if (aList[1] == "numbersornames") { Encoders::getInstance()->paramNumbersOrNames->trigger(); }
		else if (aList[1] == "littlemoveleft") { Encoders::getInstance()->paramLittleMoveLeft->trigger(); }
		else if (aList[1] == "littlemoveright") { Encoders::getInstance()->paramLittleMoveRight->trigger(); }
		else if (aList[1] == "bigmoveleft") { Encoders::getInstance()->paramBigMoveLeft->trigger(); }
		else if (aList[1] == "bigmoveright") { Encoders::getInstance()->paramBigMoveRight->trigger(); }
		else if (aList[1] == "commanddown") { Encoders::getInstance()->paramCommandDown->trigger(); }
		else if (aList[1] == "commandup") { Encoders::getInstance()->paramCommandUp->trigger(); }
		else if (aList[1] == "explodecommand") { Encoders::getInstance()->paramExplodeCommand->trigger(); }
		else if (aList[1] == "value" && aList.size() > 2 && m.size()>0) {
			int encoderNum = aList[2].getIntValue()-1;
			float val = OSCHelpers::getFloatArg(m[0]);
			if (encoderNum >= 0 && encoderNum < Encoders::getInstance()->encoders.size()) {
				MessageManager::callAsync([encoderNum, val](){
					Encoders::getInstance()->encoders[encoderNum]->setValue(val);
				});
			}
		}
	}

	else if (firstWord == "virtbuttonsgrid" && aList.size() > 1) {
		if (aList[1] == "pageplus") VirtualButtonGrid::getInstance()->goToPage(VirtualButtonGrid::getInstance()->page + 1);
		else if (aList[1] == "pageminus") VirtualButtonGrid::getInstance()->goToPage(VirtualButtonGrid::getInstance()->page - 1);
	}

	else if (firstWord == "virtfadersgrid" && aList.size() > 1) {
		if (aList[1] == "pageplus") VirtualFaderColGrid::getInstance()->goToPage(VirtualFaderColGrid::getInstance()->page + 1);
		else if (aList[1] == "pageminus") VirtualFaderColGrid::getInstance()->goToPage(VirtualFaderColGrid::getInstance()->page - 1);
		}

		/*
	if (aList[1] == "model")
	{
		String modelName = OSCHelpers::getStringArg(m[0]);
		LightBlockModel* lm = LightBlockModelLibrary::getInstance()->getModelWithName(modelName);

		if (lm != nullptr)
		{
			if (aList[1] == "assign")
			{
				if (m.size() >= 2)
				{
					int id = OSCHelpers::getIntArg(m[1]);

					LightBlockModelPreset* mp = nullptr;
					if (m.size() >= 3)
					{
						String presetName = OSCHelpers::getStringArg(m[2]);
						mp = lm->presetManager.getItemWithName(presetName);
					}

					LightBlockColorProvider* providerToAssign = mp != nullptr ? mp : (LightBlockColorProvider*)lm;
					if (id == -1)
					{
						for (auto& p : PropManager::getInstance()->items)  p->activeProvider->setValueFromTarget(providerToAssign);
					}
					else
					{
						Prop* p = PropManager::getInstance()->getPropWithId(id);
						if (p != nullptr) p->activeProvider->setValueFromTarget(providerToAssign);
					}

				}


			}
		}

	}
	else if (aList[1] == "prop")
	{
		int id = aList[1] == "all" ? -1 : aList[1].getIntValue();

		String localAddress = "/" + aList.joinIntoString("/", 3);
		OSCMessage lm(localAddress);
		lm.addString(""); //fake ID
		for (auto& a : m) lm.addArgument(a);
		lm.setAddressPattern(localAddress);

		if (id == -1)
		{
			for (auto& p : PropManager::getInstance()->items)  p->handleOSCMessage(lm);
		}
		else
		{
			if (Prop* p = PropManager::getInstance()->getPropWithId(id)) p->handleOSCMessage(lm);
		}
	}
	*/
}

void UserInputManager::programmerCommandStructureChanged()
{
	EncodersMult::getInstance()->reconstructSubComponents();
	Encoders::getInstance()->updateChannels();
	Brain::getInstance()->layoutViewerNeedRepaint = true;
}

void UserInputManager::programmerCommandStructureChanged(Programmer* p)
{
	if (p == getProgrammer(false)) {
		EncodersMult::getInstance()->reconstructSubComponents();
		Encoders::getInstance()->updateChannels();
	}
	Brain::getInstance()->layoutViewerNeedRepaint = true;
}

void UserInputManager::programmerCommandValueChanged(Programmer* p)
{
	if (p == getProgrammer(false)) {
		EncodersMult::getInstance()->reconstructNames();
	}
	Brain::getInstance()->layoutViewerNeedRepaint = true;
}

void UserInputManager::commandSelectionChanged(Command* c) {
	Programmer* p = getProgrammer(false);
	if (p != nullptr && p->currentUserCommand == c) {
		Encoders::getInstance()->updateChannels();
		commandValueChanged(c);
	}
	Brain::getInstance()->layoutViewerNeedRepaint = true;
}

void UserInputManager::redrawEncoders() {
}

void UserInputManager::commandValueChanged(Command* c) {
	Programmer* p = getProgrammer(false);
	if (p != nullptr && p->autoSelectCommand && p->currentUserCommand != c && p->commands.items.indexOf(c) >= 0) {
		p->selectCommand(c);
	}
	if (p != nullptr && p->currentUserCommand == c) {
		Encoders::getInstance()->updateEncodersValues();
		Encoders::getInstance()->updateCommandLine();
		MessageManager::callAsync([this]() {
			BKColorPicker::getInstance()->repaint(); 
		});
	}
}

void UserInputManager::encoderValueChanged(int index, float newValue, String origin) {
	const MessageManagerLock mmLock;
	if (Encoders::getInstance()->channels.size() <= index) { return; }
	int mode = Encoders::getInstance()->mode;
	
	//index += Encoders::getInstance()->encodersOffset;
	int offset = Encoders::getInstance()->encodersOffset;
	targetCommand = getProgrammer(true)->currentUserCommand;
	if (targetCommand == nullptr) {return;}
	if (index >=0 && mode < 2 && Encoders::getInstance()->channels.size() > index+offset) { 
		ChannelType* c = Encoders::getInstance()->channels.getReference(index+offset);
		if (c != nullptr) {
			String oldOrigin = "";
			
			float value = Encoders::getInstance()->encoders[index]->getValue();
			if (Encoders::getInstance()->encoderRange == 1) value /= 100.;
			if (Encoders::getInstance()->encoderRange == 2) value /= 255.;


			if (Encoders::getInstance()->lastOrigin.contains(c)) {
				oldOrigin = Encoders::getInstance()->lastOrigin.getReference(c);
			}
			if (origin == "" || origin == oldOrigin || abs(value - newValue) <= 0.009) {
				Encoders::getInstance()->lastOrigin.set(c, origin);
				UserInputManager::getInstance()->feedback("/encoder/" + String(index + 1), newValue, "");
				changeChannelValue(c, newValue);
				Brain::getInstance()->virtualFadersNeedUpdate = true;
			}
		}
	}
	else {
		// timing
	}
}

void UserInputManager::changeChannelValue(ChannelType* c, float newValue)
{
	MessageManagerLock mmlock;
	targetCommand = getProgrammer(true)->currentUserCommand;
	if (targetCommand == nullptr) {
		return;
	}
	if (c == nullptr) {
		return;
	}
	if (Encoders::getInstance()->channels.indexOf(c) == -1) {
		return;
	}

	int mode = Encoders::getInstance()->mode;
	CommandValue* t = nullptr;
	CommandValue* empty = nullptr;
	int commandIndex = -1;
	for (int i = 0; t == nullptr && i < targetCommand->values.items.size(); i++) {
		CommandValue* temp = targetCommand->values.items[i];
		if (temp->presetOrValue->getValue() == "value") {
			ChannelType* valCT = dynamic_cast<ChannelType*>(temp->channelType->targetContainer.get());
			if (valCT == c) {
				t = temp;
				commandIndex = i;
			}
			else if (empty == nullptr && valCT == nullptr) {
				empty = temp;
			}
		}
	}
	if (t == nullptr && newValue == -2) {
		return;
	}
	if (t == nullptr) {
		if (empty != nullptr) {
			t = empty;
			t->channelType->setValueFromTarget(c);
		}
		else {
			t = targetCommand->values.addItem();
			t->channelType->setValueFromTarget(c);
		}
	}

	if (mode == 0) {
		if (newValue == -2) {
			t->parentContainer->removeChildControllableContainer(t);
			targetCommand->values.items.remove(commandIndex);
			//getProgrammer()->computeValues();
			getProgrammer(true)->go();
			//t->valueFrom->setValue(newValue, false);
		}
		else {
			t->valueFrom->setValue(newValue, false);
		}
	}
	else {
		if (newValue == -2) {
			t->thru->setValue(false);
		}
		else {
			if (!t->thru->getValue()) {
				t->thru->setValue(true);
			}
			t->valueTo->setValue(newValue, false);
		}

	}
}

void UserInputManager::updateCommandLine() {
	Encoders::getInstance()->updateCommandLine();
}

void UserInputManager::toggleHightlight()
{
	if (currentProgrammer != nullptr) {
		bool hl = !currentProgrammer->highlightCurrentCommand->boolValue();
		currentProgrammer->highlightCurrentCommand->setValue(hl);
	}
}

void UserInputManager::toggleBlind()
{
	if (currentProgrammer != nullptr) {
		String val = currentProgrammer->editionMode->getValueData();
		val = val == "blind" ? "notTimed" : "blind";
		currentProgrammer->editionMode->setValueWithData(val);
	}
}

void UserInputManager::goMainCuelist()
{
	BKEngine* engine = (BKEngine*) Engine::mainEngine;
	int targetCueId = engine->conductorCuelistId->intValue();
	Cuelist* target = Brain::getInstance()->getCuelistById(targetCueId);
	if (target == nullptr) return;
	target->userGo();
}

void UserInputManager::goBackMainCuelist()
{
	BKEngine* engine = (BKEngine*) Engine::mainEngine;
	int targetCueId = engine->conductorCuelistId->intValue();
	Cuelist* target = Brain::getInstance()->getCuelistById(targetCueId);
	if (target == nullptr) return;
	target->goBack();
}

String UserInputManager::toUserText(String s) {
	if (s.containsOnly("1234567890.")) {return s;}
	if (s == "group") { return "Group"; }
	if (s == "fixture") { return "Fixture"; }
	if (s == "subfixture") { return "Subfixture"; }
	if (s == "thru") { return "Thru"; }
	if (s == "preset") { return "Preset"; }
	if (s == "add") { return "+"; }
	if (s == "remove") { return "-"; }

	return s;
}

Programmer* UserInputManager::getProgrammer(bool createIfNotThere) {
	currentProgrammer = Brain::getInstance()->getProgrammerById(1);

	if (currentProgrammer == nullptr && createIfNotThere) {
		currentProgrammer = ProgrammerManager::getInstance()->addItem();
		currentProgrammer->id->setValue(1);
	}
	return currentProgrammer;
}


int UserInputManager::backspaceOnInt(var v) {
	String s = v.toString();
	if (s.length() == 1) {
		return 0;
	}
	else {
		s = s.substring(0, s.length() - 1);
		return s.getIntValue();
	}
}

float UserInputManager::backspaceOnFloat(var v) {
	String s = v.toString();
	if (s.length() == 1) {
		return 0;
	}
	else {
		s = s.substring(0, s.length() - 1);
		return s.getFloatValue();
	}
}

void UserInputManager::gridViewCellPressed(String type, int id) {
	type = type.toLowerCase();
	Programmer *p = getProgrammer(true);
	if (p->cliActionType->getValue() != "") {
		if (p->userCanPressTargetType) {
			p->processUserInput(type);
			p->processUserInput(String(id));
			if (p->userCanPressGo) {
				p->processUserInput("enter");
			}
		}
		else if (p->userCanPressNumber) {
			
		}
	} 
	else if (type == "fixture") {
		p->checkCurrentUserCommand();
		p->getTextCommand();
		//String test = p->currentUserCommand->userCanHaveAnotherCommand ? "oui" : "non";
		if (p->currentUserCommand->userCanPressSelectionType) {
			p->processUserInput("fixture");
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->userCanHaveAnotherCommand) {
			p->processUserInput("fixture");
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->lastTarget == "selectionThru") {
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->userCanPressPlusOrMinus) {
			bool pleaseAdd = true;
			for (int i = p->currentUserCommand->selection.items.size() - 1; i >= 0; i--) {
				//CommandSelection* s = p->currentUserCommand->selection.items[i];
				//if (false && s->targetType->getValue().toString() == "fixture" && (int)s->valueFrom->getValue() == id) {
				//	pleaseAdd = false;
				//	p->currentUserCommand->selection.removeItem(s);
				//}
			}
			if (pleaseAdd) {
				p->processUserInput("+");
				p->processUserInput("fixture");
				p->processUserInput(String(id));
			}
			else if (p->currentUserCommand->selection.items.size() == 0) {
				p->currentUserCommand->selection.addItem();
			}
		}
	}	else if (type == "group") {
		p->checkCurrentUserCommand();
		p->getTextCommand();
		if (p->currentUserCommand->userCanPressSelectionType) {
			p->processUserInput("group");
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->userCanHaveAnotherCommand) {
			p->processUserInput("group");
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->lastTarget == "selectionThru") {
			p->processUserInput(String(id));
		}
		else if (p->currentUserCommand->userCanPressPlusOrMinus) {
			bool pleaseAdd = true;
			for (int i = p->currentUserCommand->selection.items.size() - 1; i >= 0; i--) {
				CommandSelection* s = p->currentUserCommand->selection.items[i];
				if (s->targetType->getValue() == "group" && (int)s->valueFrom->getValue() == id) {
					pleaseAdd = false;
					p->currentUserCommand->selection.removeItem(s);
				}
			}
			if (pleaseAdd) {
				p->processUserInput("+");
				p->processUserInput("group");
				p->processUserInput(String(id));
			}
			else if (p->currentUserCommand->selection.items.size() == 0) {
				p->currentUserCommand->selection.addItem();
			}
		}
	}
	else if (type == "preset") {
		p->checkCurrentUserCommand();
		p->getTextCommand();;
		if (p->currentUserCommand->userCanPressValueType) {
			p->processUserInput("preset");
			p->processUserInput(String(id));
			p->computing.enter();
			p->currentUserCommand->cleanUnused();
			p->computing.exit();
		} else if (p->currentUserCommand->lastTarget == "valuePresetThru") {
			p->processUserInput(String(id));
			p->computing.enter();
			p->currentUserCommand->cleanUnused();
			p->computing.exit();
		}

	}
	else if (type == "cuelist") {
		Cuelist* trg = Brain::getInstance()->getCuelistById(id);
		if (trg != nullptr) {
			trg->selectThis();
		}
	}
	else if (type == "effect") {
		Effect* trg = Brain::getInstance()->getEffectById(id);
		if (trg != nullptr) {
			trg->selectThis();
		}
	}
	else if (type == "carousel") {
		Carousel* trg = Brain::getInstance()->getCarouselById(id);
		if (trg != nullptr) {
			trg->selectThis();
		}
	}
	else if (type == "mapper") {
		Mapper* trg = Brain::getInstance()->getMapperById(id);
		if (trg != nullptr) {
			trg->selectThis();
		}
	}
}

void UserInputManager::testPreset(Preset* p)
{
	if (p == nullptr) { return; }
	Programmer* prg = getProgrammer(true);
	if (prg == nullptr) { return; }
	int fixtMax = 1;
	for (int i = 0; i < FixtureManager::getInstance()->items.size(); i++) {
		int id = FixtureManager::getInstance()->items[i]->id->getValue();
		fixtMax = jmax(fixtMax, id);
	}
	// prg->clearAll();
	processInput("Fixture");
	processInput("1");
	processInput("Thru");
	processInput(String(fixtMax));
	processInput("Preset");
	processInput(p->id->getValue());
}

void UserInputManager::resetFocus()
{
	// Component::unfocusAllComponents();
	InputPanel::getInstance()->setFocusContainerType(Component::FocusContainerType::keyboardFocusContainer);
}

void UserInputManager::feedback(String address, var value, String origin="")
{
	InterfaceManager::getInstance()->feedback(address, value, origin);
}

void UserInputManager::loadContentConductor()
{
	Programmer* p = getProgrammer(true);
	if (p== nullptr) return;
	BKEngine* e = dynamic_cast<BKEngine*>(BKEngine::mainEngine);
	if (e == nullptr) return;
	Cuelist* c = Brain::getInstance()->getCuelistById(e->conductorCuelistId->intValue());
	if (c == nullptr ) return;
	c->loadContent(p);
}
