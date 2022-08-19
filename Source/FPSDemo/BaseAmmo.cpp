// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseAmmo.h"

// Sets default values
ABaseAmmo::ABaseAmmo()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//set variables.
	ammoType = EAmmoType::E_WeaponM4;
	ammoAmount = 20;

}

// Called when the game starts or when spawned
void ABaseAmmo::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABaseAmmo::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


