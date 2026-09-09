#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include "IplugMidi.h"
#include "fluidsynth.h"
#include "CustomControls.h"
#include <mutex>

#define StringFormatted(fmt, ...) \
    ([&]() { \
        WDL_String _str; \
        _str.SetFormatted(4096, fmt, __VA_ARGS__); \
        return std::string(_str.Get()); \
    }())

const int kNumPresets = 1;

enum EParams
{
	kParamCurrentChannel = 0,

	kParamGain,

	kParamPreset1,
	kParamPreset2,
	kParamPreset3,
	kParamPreset4,
	kParamPreset5,
	kParamPreset6,
	kParamPreset7,
	kParamPreset8,
	kParamPreset9,
	kParamPreset10,
	kParamPreset11,
	kParamPreset12,
	kParamPreset13,
	kParamPreset14,
	kParamPreset15,
	kParamPreset16,

	kParamChorusActive,
	kParamChorusDepth,
	kParamChorusLevel,
	kParamChorusVoiceCount,
	kParamChorusSpeed,

	kParamReverbActive,
	kParamReverbDamp,
	kParamReverbLevel,
	kParamReverbRoomSize,
	kParamReverbWidth,

	kParamFilterResonance,
	kParamFilterCutoff,

	kParamEnvA,
	kParamEnvD,
	kParamEnvS,
	kParamEnvR,

	kParamInterpMode,

	kNumParams,
};

enum FluidSynthOptionalCC
{
	FLUID_FILTER_RESONANCE = 71,	// Filter resonance
	FLUID_ENV_R = 72,				// Release time
	FLUID_ENV_A = 73,				// Attack time
	FLUID_FILTER_CUTOFF = 74,		// Filter cutoff
	FLUID_ENV_D = 75,				// Decay time
	FLUID_ENV_S = 79,				// Sustain
	FLUID_REVERB_SEND = 91,
	FLUID_CHORUS_SEND = 93,
};

//enum EControlTags
//{
//	kCtrlTagGain = 0,
//	kNumCtrlTags
//};

using namespace iplug;
using namespace igraphics;

class BetterSF final : public Plugin
{
public:
	BetterSF(const InstanceInfo& info);
	~BetterSF();

#if IPLUG_DSP // http://bit.ly/2S64BDd
public:
	void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
	void ProcessMidiMsg(const IMidiMsg& msg) override;
	void ProcessMidiMsgFromBlock(const IMidiMsg& msg);
	void OnReset() override;
	void OnParamChange(int paramIdx) override;
	void OnIdle() override;
	void UpdateUI();
	void OnUIOpen() override;
	void PopulateCurrentPresetsList();
	void PromptChangeSoundfont();
	bool LoadSoundFontFromPath(std::string path, bool resetPresets = false);
	void SetUpFluidModulators();
	bool SerializeState(IByteChunk& chunk) const override;
	int UnserializeState(const IByteChunk& chunk, int startPos) override;
	void SaveUserSettings();
	void LoadUserSettings();
	void LogMessage(const std::string& message, bool trunc = false) const;
	
private:
	fluid_settings_t* mFluidSettings = nullptr;
	fluid_synth_t* mSynth = nullptr;
	fluid_sfont_t* mSoundfont = nullptr;
	int mSoundFontID = -1;
	std::vector<float> mLeftBuffer, mRightBuffer;
	std::string mCurrentSoundfontFilePath = "";
	std::string mDefaultSoundfontFilePath = "";
	std::mutex mSynthMutex;
	bool mKeepSoundfontProgramIdxBetweenLoads = true;
	std::atomic<bool> mUiNeedsRefresh{ false };
	bool mEnableLogs = false;

	KsListViewControl* mListViewControl = nullptr;
	FileLoaderDisplay* mFileLoaderDisplay = nullptr;
	KsChannelSelector* mChannelSelector = nullptr;

	struct PresetInfo
	{
		std::string name;
		int number;
		int bank;
	};

	std::vector<PresetInfo> mCurrentPresets;

	std::string mParamAdditionalData[kNumParams];

	IMidiQueue mMidiQueue;
	int mLastBlockSize = 0;

#endif
};
