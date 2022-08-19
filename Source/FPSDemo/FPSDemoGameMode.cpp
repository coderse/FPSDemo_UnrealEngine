// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSDemoGameMode.h"
#include "FPSDemoHUD.h"
#include "FPSDemoCharacter.h"
#include "UObject/ConstructorHelpers.h"

AFPSDemoGameMode::AFPSDemoGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AFPSDemoHUD::StaticClass();
}
