// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FPSDemoHUD.generated.h"

UCLASS()
class AFPSDemoHUD : public AHUD
{
	GENERATED_BODY()

public:
	AFPSDemoHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

