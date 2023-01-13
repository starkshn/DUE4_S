// Fill out your copyright notice in the Description page of Project Settings.


#include "ABWeapon.h"

// Sets default values
AABWeapon::AABWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WEAPON"));
	RootComponent = Weapon;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_WEAPON(TEXT("SkeletalMesh'/Game/InfinityBladeWeapons/Weapons/Blade/Silly_Weapons/Blade_ChickenBlade/SK_Blade_ChickenBlade.SK_Blade_ChickenBlade'"));

	if (SK_WEAPON.Succeeded())
	{
		Weapon->SetSkeletalMesh(SK_WEAPON.Object);
	}
	
	Weapon->SetCollisionProfileName(TEXT("NoCollision"));

	AttackRange = 150.f;
}

float AABWeapon::GetAttackRange() const
{
	return AttackRange;
}

// Called when the game starts or when spawned
void AABWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

