#include "BetterSF.h"
#include "IPlug_include_in_plug_src.h"
#include "Fluidsynth.h"
#include "CustomControls.h"
#include "IPlugPaths.h"
#include <filesystem>
#include <fstream>

BetterSF::BetterSF(const InstanceInfo& info)
	: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
	//EnsureDefaultPreset();

	for (int i = 0; i < kNumParams; i++)
		mParamAdditionalData[i] = "";

	GetParam(kParamCurrentChannel)->InitInt("channel", 0, 0, 15);

	GetParam(kParamGain)->InitDouble("gain", 1.0, 0.0, 10.0, 0);
	mParamAdditionalData[kParamGain] = "synth.gain";

	GetParam(kParamReverbActive)->InitBool("reverb_active", false);
	GetParam(kParamReverbDamp)->InitDouble("reverb_damp", 0.3, 0.0, 1.0, 0);
	GetParam(kParamReverbLevel)->InitDouble("reverb_level", 0.7, 0.0, 1.0, 0);
	GetParam(kParamReverbRoomSize)->InitDouble("reverb_room_size", 0.5, 0.0, 1.0, 0);
	GetParam(kParamReverbWidth)->InitDouble("reverb_width", 0.8, 0.0, 1.0, 0);

	GetParam(kParamChorusActive)->InitBool("chorus_active", false);
	GetParam(kParamChorusDepth)->InitDouble("chorus_depth", 4.25, 0.0, 256.0, 0);
	GetParam(kParamChorusLevel)->InitDouble("chorus_level", 0.6, 0.0, 1.0, 0);
	GetParam(kParamChorusSpeed)->InitDouble("chorus_speed", 0.2, 0.0, 5.0, 0);
	GetParam(kParamChorusVoiceCount)->InitInt("chorus_voice_count", 3, 0, 99);

	GetParam(kParamFilterCutoff)->InitDouble("filter_cutoff", 1.0, 0.0, 1.0, 0);
	GetParam(kParamFilterResonance)->InitDouble("filter_resonance", 0.0, 0.0, 1.0, 0);

	GetParam(kParamEnvA)->InitDouble("env_a", 0.0, 0.0, 1.0, 0);
	GetParam(kParamEnvD)->InitDouble("env_d", 0.0, 0.0, 1.0, 0);
	GetParam(kParamEnvS)->InitDouble("env_s", 0.0, 0.0, 1.0, 0);
	GetParam(kParamEnvR)->InitDouble("env_r", 0.0, 0.0, 1.0, 0);

	GetParam(kParamInterpMode)->InitInt("interpolation_method", 2, 0, 3);

	for (int i = 0; i < 16; i++)
		GetParam(kParamPreset1 + i)->InitInt((std::string("preset_channel") + std::to_string(i)).c_str(), 0, 0, INT_MAX);


#if IPLUG_EDITOR // http://bit.ly/2S64BDd
	mMakeGraphicsFunc = [&]() {
		return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
		};

	mLayoutFunc = [&](IGraphics* pGraphics) {

		pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
		const IRECT b = pGraphics->GetBounds().GetPadded(-5);

		pGraphics->EnableTooltips(true);

		pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
		pGraphics->EnableMouseOver(true);

		pGraphics->AttachPanelBackground(bgcol_dark);
		pGraphics->AttachControl(new FancyDotsDecoration(pGraphics->GetBounds().GetFromTop(b.H() - 50), bgcol_verydark, 1.0f, 5.0f));
		pGraphics->AttachControl(new RoundRectBgPanel(pGraphics->GetBounds().GetFromTop(b.H() - 50).GetFromBottom(b.H() - 50 - 60), bgcol_light, 10.0f));

		KsKnobControl* gainKnob = new KsKnobControl(b.GetFromTLHC(50, 50).GetTranslated(20, 0), kParamGain);
		pGraphics->AttachControl(gainKnob);

		IVKeyboardControl* keyboard = new IVKeyboardControl(b.GetFromBottom(50), 21, 108, true);
		pGraphics->AttachControl(keyboard);

		mListViewControl = new KsListViewControl(b.GetFromLeft(450).GetVPadded(-100).GetHShifted(20).GetVShifted(20), [&](IControl* pCaller) {
			int currentChannel = GetParam(kParamCurrentChannel)->Int();
			GetParam(kParamPreset1 + currentChannel)->Set(pCaller->As<KsListViewControl>()->LastClickedIndex());
			OnParamChange(kParamPreset1 + currentChannel);
			SendCurrentParamValuesFromDelegate();
			});
		pGraphics->AttachControl(mListViewControl);

		KsDropdownList* interpChanger = new KsDropdownList(b.GetFromTRHC(250, 25).GetTranslated(-30, 80), kParamInterpMode, "Interpolation: ");
		interpChanger->AddItem("None");
		interpChanger->AddItem("Linear");
		interpChanger->AddItem("4th Order");
		interpChanger->AddItem("7Th Order");
		interpChanger->SetSelectedIndex(2); //default
		pGraphics->AttachControl(interpChanger);

		mFileLoaderDisplay = new FileLoaderDisplay(b.GetFromTLHC(600, 30).GetTranslated(100, 10), [&](IControl* pCaller) { PromptChangeSoundfont(); });
		mFileLoaderDisplay->mLoadFileCallback = [&](const char* file) {
			LoadSoundFontFromPath(file, !mKeepSoundfontProgramIdxBetweenLoads);

			GetParam(kParamCurrentChannel)->Set(0);

			mListViewControl->SelectIndex(0);
			};
		pGraphics->AttachControl(mFileLoaderDisplay);

		pGraphics->AttachControl(new KsTextButton(b.GetFromTRHC(70, 30).GetTranslated(-10, 10), "Settings", [&](IControl* pCaller) {

			IPopupMenu menu = IPopupMenu("Items", {}, [&](IPopupMenu* pMenu) {
				int idx = pMenu->GetChosenItemIdx();
				if (idx == 0) mDefaultSoundfontFilePath = mCurrentSoundfontFilePath;
				if (idx == 1) mKeepSoundfontProgramIdxBetweenLoads = !mKeepSoundfontProgramIdxBetweenLoads;
				if (idx == 2) GetUI()->ShowMessageBox("BetterSF\nMade by keestak\nBuilt with Iplug2 and Fluidsynth", "About BetterSF", EMsgBoxType::kMB_OK);
				SaveUserSettings();
				});
			menu.AddItem("Set as default soundfont", 0);
			menu.AddItem("Keep selected preset index on file change", 1, mKeepSoundfontProgramIdxBetweenLoads ? IPopupMenu::Item::kChecked : 0);
			menu.AddItem("About...", 2);
			GetUI()->CreatePopupMenu(*pCaller, menu, pCaller->GetRECT());

			}));

		mChannelSelector = new KsChannelSelector(b.GetFromTLHC(25 * 16, 25).GetTranslated(20, 60), [&](IControl* pCaller) {
			int currentSelectedChannel = pCaller->As<KsChannelSelector>()->SelectedChannel();

			GetParam(kParamCurrentChannel)->Set(currentSelectedChannel);

			fluid_preset_t* preset = fluid_synth_get_channel_preset(mSynth, currentSelectedChannel);

			if (preset == nullptr)
				return;

			std::string bank = std::to_string(fluid_preset_get_banknum(preset));
			std::string num = std::to_string(fluid_preset_get_num(preset));

			mListViewControl->SelectWithPrefix(bank + " " + num);
			});
		pGraphics->AttachControl(mChannelSelector);
		pGraphics->AttachControl(new KsEditableTextControl(b.GetFromTLHC(25 * 16, 25).GetTranslated(20, 90), [&](IControl* pCaller) {
			const char* text_val = pCaller->As<KsEditableTextControl>()->GetStr();
			mListViewControl->SetSearchFilter(text_val);
			}));

		IRECT effectssRect = b.GetFromRight(300).GetPadded(0, -120, 0, -80).GetHShifted(-20);
		pGraphics->AttachControl(new RoundRectBgPanel(effectssRect, bgcol_dark, 5.0f, true, bgcol_verydark));
		IRECT toggleRect = effectssRect.GetFromTLHC(290, 20).GetTranslated(10, 10);
		IRECT knobRect = effectssRect.GetFromTLHC(50, 50).GetTranslated(10, 35);
		IRECT knobLabelRect = effectssRect.GetFromTLHC(50, 20).GetTranslated(10, 90);

		auto addFluidControlKnob = [&](int col, int row, const char* labelText, const char* fluidSetting = "", int paramIdx = -1, float range = 1.0f) -> KsKnobControl*
			{
				const int rowOffset = 110;
				const int colOffset = 60;
				KsKnobControl* control = new KsKnobControl(knobRect.GetTranslated(col * colOffset, row * rowOffset), paramIdx);
				if (strlen(fluidSetting) > 0 && paramIdx >= 0)
					mParamAdditionalData[paramIdx] = fluidSetting;
				pGraphics->AttachControl(control);
				pGraphics->AttachControl(new IVLabelControl(knobLabelRect.GetTranslated(col * colOffset, row * rowOffset), labelText));
				return control;
			};

		//chorus controls
		pGraphics->AttachControl(new KsControlActiveToggle(toggleRect, "Chorus", true, kParamChorusActive));

		addFluidControlKnob(0, 0, "Depth", "synth.chorus.depth", kParamChorusDepth, 256.0f);
		addFluidControlKnob(1, 0, "Level", "synth.chorus.level", kParamChorusLevel);
		addFluidControlKnob(2, 0, "Speed", "synth.chorus.speed", kParamChorusSpeed, 5.0f);
		addFluidControlKnob(3, 0, "Voices", "synth.chorus.nr", kParamChorusVoiceCount, 99.0f);

		//reverb controls
		pGraphics->AttachControl(new KsControlActiveToggle(toggleRect.GetTranslated(0, 110), "Reverb", true, kParamReverbActive));

		addFluidControlKnob(0, 1, "Damp", "synth.reverb.damp", kParamReverbDamp);
		addFluidControlKnob(1, 1, "Level", "synth.reverb.level", kParamReverbLevel);
		addFluidControlKnob(2, 1, "Size", "synth.reverb.room-size", kParamReverbRoomSize);
		addFluidControlKnob(3, 1, "Width", "synth.reverb.width", kParamReverbWidth);


		//filter controls
		pGraphics->AttachControl(new KsControlActiveToggle(toggleRect.GetTranslated(0, 220), "Filter"));

		KsKnobControl* cutoffKnob = addFluidControlKnob(0, 2, "Cutoff", "", kParamFilterCutoff);
		cutoffKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_FILTER_CUTOFF, 127 * (1.0 - pCaller->GetValue())); });
		KsKnobControl* resonanceKnob = addFluidControlKnob(1, 2, "Res", "", kParamFilterResonance);
		resonanceKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_FILTER_RESONANCE, 127 * pCaller->GetValue()); });

		//envelope
		pGraphics->AttachControl(new KsControlActiveToggle(toggleRect.GetTranslated(0, 330), "Envelope"));

		KsKnobControl* attackKnob = addFluidControlKnob(0, 3, "A", "", kParamEnvA);
		attackKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_ENV_A, 127 * pCaller->GetValue()); });
		KsKnobControl* decayKnob = addFluidControlKnob(1, 3, "D", "", kParamEnvD);
		decayKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_ENV_D, 127 * pCaller->GetValue()); });
		KsKnobControl* susKnob = addFluidControlKnob(2, 3, "S", "", kParamEnvS);
		susKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_ENV_S, 127 * pCaller->GetValue()); });
		KsKnobControl* relKnob = addFluidControlKnob(3, 3, "R", "", kParamEnvR);
		relKnob->SetActionFunction([&](IControl* pCaller) { fluid_synth_cc(mSynth, 0, FluidSynthOptionalCC::FLUID_ENV_R, 127 * pCaller->GetValue()); });

		if (!mCurrentSoundfontFilePath.empty())
			UpdateUI();
		else
			mUiNeedsRefresh = true;
		};

#endif
}

#if IPLUG_DSP
void BetterSF::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
	if (!mSynth) return;

	int currentSample = 0;
	IMidiMsg msg;

	while (!mMidiQueue.Empty())
	{
		msg = mMidiQueue.Peek();
		if (msg.mOffset >= nFrames) break;

		int sliceLen = msg.mOffset - currentSample;
		if (sliceLen > 0)
		{
			fluid_synth_write_float(mSynth, sliceLen,
				mLeftBuffer.data() + currentSample, 0, 1,
				mRightBuffer.data() + currentSample, 0, 1);
			currentSample += sliceLen;
		}

		ProcessMidiMsgFromBlock(msg);

		mMidiQueue.Remove();

		for (int s = 0; s < nFrames; s++)
		{
			if (NOutChansConnected() >= 2)
			{
				outputs[0][s] = (sample)mLeftBuffer[s];
				outputs[1][s] = (sample)mRightBuffer[s];
			}
			else
			{
				outputs[0][s] = (sample)((mLeftBuffer[s] + mRightBuffer[s]) * 0.5f);
			}
		}
	}

	int remainingSamples = nFrames - currentSample;
	if (remainingSamples > 0)
	{
		fluid_synth_write_float(mSynth, remainingSamples,
			mLeftBuffer.data() + currentSample, 0, 1,
			mRightBuffer.data() + currentSample, 0, 1);
	}

	mMidiQueue.Flush(nFrames);

	for (int s = 0; s < nFrames; s++)
	{
		if (NOutChansConnected() >= 2)
		{
			outputs[0][s] = (sample)mLeftBuffer[s];
			outputs[1][s] = (sample)mRightBuffer[s];
		}
		else
		{
			outputs[0][s] = (sample)((mLeftBuffer[s] + mRightBuffer[s]) * 0.5f);
		}
	}
}

void BetterSF::ProcessMidiMsgFromBlock(const IMidiMsg& msg)
{
	int status = msg.StatusMsg();
	IMidiMsg::EControlChangeMsg ccidx = msg.ControlChangeIdx();

	DBGMSG("NoteOn: ch=%i, note=%i, vel=%i, cc=%i, cc val=%f, pitch=%f\n", msg.Channel(), msg.NoteNumber(), msg.Velocity(), ccidx, msg.ControlChange(ccidx), (msg.PitchWheel() + 1.0) * 0.5);

	switch (status)
	{
	case IMidiMsg::kNoteOn:
		if (msg.Velocity() == 0)
		{
			fluid_synth_noteoff(mSynth, msg.Channel(), msg.NoteNumber());
			break;
		}
		fluid_synth_noteon(mSynth, msg.Channel(), msg.NoteNumber(), msg.Velocity());
		break;
	case IMidiMsg::kNoteOff:
		fluid_synth_noteoff(mSynth, msg.Channel(), msg.NoteNumber());
		break;
	case IMidiMsg::kControlChange:
		fluid_synth_cc(mSynth, msg.Channel(), ccidx, (int)std::round(std::clamp(msg.ControlChange(ccidx), 0.0, 1.0) * 127.0));
		break;
	case IMidiMsg::kPitchWheel:
		fluid_synth_pitch_bend(mSynth, msg.Channel(), (int)((msg.PitchWheel() + 1.0) * 8191.5));
		break;
	case IMidiMsg::kPolyAftertouch:
		fluid_synth_channel_pressure(mSynth, msg.Channel(), msg.ChannelAfterTouch());
		break;
	case IMidiMsg::kProgramChange:
	case IMidiMsg::kChannelAftertouch:
	default:
		break;
	}
}

void BetterSF::OnIdle()
{
	if (!mUiNeedsRefresh) return;
	mUiNeedsRefresh = false;

	UpdateUI();
}

void BetterSF::OnUIOpen()
{
	IEditorDelegate::OnUIOpen(); //must call this (which calls SendCurrentParamValuesFromDelegate()) otherwise ui becomes totally broken
	UpdateUI();
}

void BetterSF::UpdateUI()
{
	if (!GetUI() || !mListViewControl)
	{
		mUiNeedsRefresh = true;
		return;
	}

	mListViewControl->ClearListItems();
	mFileLoaderDisplay->AddFilePath(mCurrentSoundfontFilePath, true, false);

	if (!mSoundfont)
	{
		DBGMSG("No soundfont loaded (ID=%d)\n", mSoundFontID);
		return;
	}

	const char* sname = fluid_sfont_get_name(mSoundfont);
	DBGMSG("SoundFont ID %d: %s\n", mSoundFontID, sname ? sname : "(null)");

	for (int i = 0; i < mCurrentPresets.size(); i++)
	{
		PresetInfo pi = mCurrentPresets[i];
		mListViewControl->AddListItem(std::to_string(pi.bank) + " " + std::to_string(pi.number) + " " + pi.name);
	}

	int currentChannel = GetParam(kParamCurrentChannel)->Int();

	mChannelSelector->SelectChannel(currentChannel, false);

	mListViewControl->SelectIndex(GetParam(kParamPreset1 + currentChannel)->Int(), false, false);

	GetUI()->SetAllControlsDirty();
}

void BetterSF::PopulateCurrentPresetsList()
{
	mCurrentPresets.clear();
	fluid_sfont_iteration_start(mSoundfont);
	fluid_preset_t* preset = nullptr;
	while ((preset = fluid_sfont_iteration_next(mSoundfont)) != NULL)
	{
		int bank = fluid_preset_get_banknum(preset);
		int num = fluid_preset_get_num(preset);
		std::string name = std::string(fluid_preset_get_name(preset));

		PresetInfo pi = PresetInfo();
		pi.bank = bank;
		pi.name = name;
		pi.number = num;

		mCurrentPresets.push_back(pi);

		DBGMSG("Bank: %i, Program: %i, Name: %s\n", bank, num, name.c_str());
	}
}

void BetterSF::OnReset()
{
	LoadUserSettings();

	mLeftBuffer.resize(GetBlockSize());
	mRightBuffer.resize(GetBlockSize());

	if (mLastBlockSize != GetBlockSize())
	{
		mMidiQueue.Resize(GetBlockSize());
		mLastBlockSize = GetBlockSize();
	}
	mMidiQueue.Flush(GetBlockSize());

	if (mSynth)
	{
		delete_fluid_synth(mSynth);
		mSynth = nullptr;
	}
	if (mFluidSettings)
	{
		delete_fluid_settings(mFluidSettings);
		mFluidSettings = nullptr;
	}

	mFluidSettings = new_fluid_settings();
	fluid_settings_setnum(mFluidSettings, "synth.sample-rate", GetSampleRate());
	fluid_settings_setnum(mFluidSettings, "synth.polyphony", 256);
	mSynth = new_fluid_synth(mFluidSettings);

	fluid_synth_set_gain(mSynth, GetParam(kParamGain)->Value());

	SetUpFluidModulators();

	if (!mCurrentSoundfontFilePath.empty())
		LoadSoundFontFromPath(mCurrentSoundfontFilePath);
	else if (!mDefaultSoundfontFilePath.empty())
		LoadSoundFontFromPath(mDefaultSoundfontFilePath);
}

void BetterSF::ProcessMidiMsg(const IMidiMsg& msg)
{
	mMidiQueue.Add(msg);
}

void BetterSF::OnParamChange(int paramIdx)
{
	if (!mSynth) return;

	switch (paramIdx)
	{
	case kParamCurrentChannel:
	{
		if (mListViewControl && GetUI())
		{
			int channel = kParamPreset1 + GetParam(kParamCurrentChannel)->Int();
			mListViewControl->SelectIndex(GetParam(channel)->Int(), false);
		}
	}
	break;
	case kParamChorusActive:
	{
		bool on = GetParam(kParamChorusActive)->Bool();
		for (int ch = 0; ch < 16; ch++)
			fluid_synth_cc(mSynth, ch, FluidSynthOptionalCC::FLUID_CHORUS_SEND, on ? 127 : 0);
		fluid_synth_chorus_on(mSynth, -1, on);
	}
	break;
	case kParamReverbActive:
	{
		bool on = GetParam(kParamReverbActive)->Bool();
		for (int ch = 0; ch < 16; ch++)
			fluid_synth_cc(mSynth, ch, FluidSynthOptionalCC::FLUID_REVERB_SEND, on ? 127 : 0);
		fluid_synth_reverb_on(mSynth, -1, on);
	}
	break;
	case kParamInterpMode:
	{
		const int interpModes[] = { FLUID_INTERP_NONE, FLUID_INTERP_LINEAR, FLUID_INTERP_4THORDER, FLUID_INTERP_7THORDER };
		int mode = interpModes[GetParam(paramIdx)->Int()];
		fluid_synth_set_interp_method(mSynth, -1, mode);
		break;
	}
	case kParamGain:
	case kParamChorusDepth:
	case kParamChorusLevel:
	case kParamChorusVoiceCount:
	case kParamChorusSpeed:
	case kParamReverbDamp:
	case kParamReverbLevel:
	case kParamReverbRoomSize:
	case kParamReverbWidth:
	case kParamFilterResonance:
	case kParamFilterCutoff:
	case kParamEnvA:
	case kParamEnvD:
	case kParamEnvS:
	case kParamEnvR:
		if (!mParamAdditionalData[paramIdx].empty() && mFluidSettings)
		{
			fluid_settings_setnum(mFluidSettings, mParamAdditionalData[paramIdx].c_str(), GetParam(paramIdx)->Value());
		}
		break;
	default:
		if (paramIdx >= kParamPreset1 && paramIdx <= kParamPreset16 && mCurrentPresets.size() > 0)
		{
			int channel = paramIdx - kParamPreset1;
			int selectedPreset = GetParam(paramIdx)->Int();
			if (selectedPreset < 0 || selectedPreset >= mCurrentPresets.size())
				selectedPreset = 0;
			fluid_synth_program_select(mSynth, channel, mSoundFontID, mCurrentPresets[selectedPreset].bank, mCurrentPresets[selectedPreset].number);
			//SetCurrentPresetIdx(0);
			//ModifyCurrentPreset(mCurrentPresets[selectedPreset].name.c_str());
			//InformHostOfPresetChange();
		}
		break;
	} 
}

#endif

void BetterSF::PromptChangeSoundfont()
{

	IFileDialogCompletionHandlerFunc completionFunc = [&](WDL_String file, WDL_String path)
		{
			if (file.GetLength() > 0)
			{
				DBGMSG("SELECTED SOUNDFONT: %s, %s\n", file.Get(), path.Get());
				LoadSoundFontFromPath(file.Get(), !mKeepSoundfontProgramIdxBetweenLoads);
			}
		};

	WDL_String file, path;
	GetUI()->PromptForFile(file, path, EFileAction::Open, "sf2", completionFunc);
}

bool BetterSF::LoadSoundFontFromPath(std::string file, bool resetPresets)
{
	if (!mSynth) return false;
	if (file.empty()) return false;

	if (mSoundFontID)
	{
		fluid_synth_sfunload(mSynth, mSoundFontID, 1);
		mSoundFontID = 0;
		mSoundfont = nullptr;
	}

	mSoundFontID = fluid_synth_sfload(mSynth, file.c_str(), 1);
	if (mSoundFontID <= 0)
	{
		mCurrentSoundfontFilePath = "";
		mSoundFontID = 0;
		DBGMSG("Failed to load soundfont '%s' (returned %d)\n", file.c_str(), mSoundFontID);
		return false;
	}

	mCurrentSoundfontFilePath = file;
	mSoundfont = fluid_synth_get_sfont_by_id(mSynth, mSoundFontID);

	PopulateCurrentPresetsList();

	if (resetPresets)
	{
		for (int ch = 0; ch < 16; ch++)
			GetParam(kParamPreset1 + ch)->Set(0);
	}

	//SendCurrentParamValuesFromDelegate() doesn't seem to work so I'm doing this
	for (int p = 0; p < kNumParams; p++)
		OnParamChange(p);

	DBGMSG("Loaded soundfont '%s'\n", file.c_str(), mSoundFontID);
	UpdateUI();
	return true;
}

void BetterSF::SetUpFluidModulators() //reference https://github.com/mateusz/fluidadsr/blob/master/src/main.cpp sometimes envelopes just don't work with some soundfonts, not sure if there's a way to fix it
{
	// Set up envelope amount
	float env_amount = 20000.0f;

	// Filter resonance modulator
	std::unique_ptr<fluid_mod_t, decltype(&delete_fluid_mod)> mod{ new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_FILTER_RESONANCE, // MIDI CC 71 Timbre/Harmonic Intensity (filter resonance)
		FLUID_MOD_CC
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_CONCAVE
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_FILTERQ);
	fluid_mod_set_amount(mod.get(), 960.0f);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	// Release time modulator
	mod = { new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_ENV_R, // MIDI CC 72 Release time
		FLUID_MOD_CC
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_LINEAR
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_VOLENVRELEASE);
	fluid_mod_set_amount(mod.get(), env_amount);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	// Attack time modulator
	mod = { new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_ENV_A, // MIDI CC 73 Attack time
		FLUID_MOD_CC
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_LINEAR
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_VOLENVATTACK);
	fluid_mod_set_amount(mod.get(), env_amount);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	// Filter cutoff modulator
	mod = { new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_FILTER_CUTOFF, // MIDI CC 74 Brightness (cutoff frequency, FILTERFC)
		FLUID_MOD_CC
		| FLUID_MOD_LINEAR
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_FILTERFC);
	fluid_mod_set_amount(mod.get(), -5000.0f);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	// Decay time modulator
	mod = { new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_ENV_D, // MIDI CC 75 Decay Time
		FLUID_MOD_CC
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_LINEAR
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_VOLENVDECAY);
	fluid_mod_set_amount(mod.get(), env_amount);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	// Sustain modulator
	mod = { new_fluid_mod(), delete_fluid_mod };
	fluid_mod_set_source1(mod.get(),
		FluidSynthOptionalCC::FLUID_ENV_S, // MIDI CC 79 undefined
		FLUID_MOD_CC
		| FLUID_MOD_UNIPOLAR
		| FLUID_MOD_CONCAVE
		| FLUID_MOD_POSITIVE);
	fluid_mod_set_source2(mod.get(), 0, 0);
	fluid_mod_set_dest(mod.get(), GEN_VOLENVSUSTAIN);
	fluid_mod_set_amount(mod.get(), 1000.0f);
	fluid_synth_add_default_mod(mSynth, mod.get(), FLUID_SYNTH_OVERWRITE);

	for (int channel = 0; channel < 16; channel++)
	{
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_FILTER_RESONANCE, 0);  // Filter resonance
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_ENV_R, 0);  // Release
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_ENV_A, 0);  // Attack
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_FILTER_CUTOFF, 0);  // Filter cutoff
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_ENV_D, 0);  // Decay
		fluid_synth_cc(mSynth, channel, FluidSynthOptionalCC::FLUID_ENV_S, 0); // Sustain
	}
}

bool BetterSF::SerializeState(IByteChunk& chunk) const
{
	chunk.PutStr(mCurrentSoundfontFilePath.c_str());
	return SerializeParams(chunk);
}

int BetterSF::UnserializeState(const IByteChunk& chunk, int startPos)
{
	WDL_String filePath;
	startPos = chunk.GetStr(filePath, startPos);
	startPos = UnserializeParams(chunk, startPos);
	mCurrentSoundfontFilePath = std::string(filePath.Get());
	if (mSynth && !mCurrentSoundfontFilePath.empty())
	{
		LoadSoundFontFromPath(mCurrentSoundfontFilePath);
	}
	return startPos;
}

void BetterSF::SaveUserSettings()
{
	WDL_String settingsPath;
	INIPath(settingsPath, "BetterSF");
	std::filesystem::create_directories(settingsPath.Get());
	WDL_String settingsFile(settingsPath.Get());
	settingsFile.Append("/bettersf.ini");
	
	std::ofstream f(settingsFile.Get());
	if (f) {
		f << "defaultSoundfont=" << mDefaultSoundfontFilePath << "\n";
		f << "keepSoundfontProgramIdxBetweenLoads=" << mKeepSoundfontProgramIdxBetweenLoads << "\n";
	}
}

void BetterSF::LoadUserSettings()
{
	WDL_String settingsPath;
	INIPath(settingsPath, "BetterSF");
	WDL_String settingsFile(settingsPath.Get());
	settingsFile.Append("/bettersf.ini");

	std::ifstream f(settingsFile.Get());
	if (f) {
		std::string line;
		while (std::getline(f, line))
		{
			auto sep = line.find('=');
			if (sep == std::string::npos) continue;
			std::string key = line.substr(0, sep);
			std::string val = line.substr(sep + 1);

			if (key == "defaultSoundfont") mDefaultSoundfontFilePath = val;
			if (key == "keepSoundfontProgramIdxBetweenLoads") mKeepSoundfontProgramIdxBetweenLoads = (val == "1");
		}
	}
}