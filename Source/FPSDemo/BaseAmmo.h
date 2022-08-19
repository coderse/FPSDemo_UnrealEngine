// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseAmmo.generated.h"


UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	E_WeaponM4 UMETA(DisplayName = "Weapon_M4")
};


UCLASS()
class FPSDEMO_API ABaseAmmo : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseAmmo();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int ammoAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EAmmoType ammoType;

};
