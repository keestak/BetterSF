
#include <TargetConditionals.h>
#if TARGET_OS_IOS == 1 || TARGET_OS_VISION == 1
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

#define IPLUG_AUVIEWCONTROLLER IPlugAUViewController_vBetterSF
#define IPLUG_AUAUDIOUNIT IPlugAUAudioUnit_vBetterSF
#import <BetterSFAU/IPlugAUViewController.h>
#import <BetterSFAU/IPlugAUAudioUnit.h>

//! Project version number for BetterSFAU.
FOUNDATION_EXPORT double BetterSFAUVersionNumber;

//! Project version string for BetterSFAU.
FOUNDATION_EXPORT const unsigned char BetterSFAUVersionString[];

@class IPlugAUViewController_vBetterSF;
