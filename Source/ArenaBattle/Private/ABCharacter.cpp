// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacter.h"
#include "ABAnimInstance.h"
#include "DrawDebugHelpers.h"
#include "ABWeapon.h"
#include "ABCharacterStatComponent.h"
#include "Components/WidgetComponent.h"
#include "ABCharacterWidget.h"
#include "ABAIController.h"
#include "ABCharacterSetting.h"
#include "ABGameInstance.h"
#include "ABPlayerController.h"
#include "ABPlayerState.h"
#include "ABHUDWidget.h"

// Sets default values
AABCharacter::AABCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = GetCapsuleComponent();
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("ABCharacter"));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));
	CharacterStat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("CHARACTERSTAT"));
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBARWIDGET"));

	SpringArm->SetupAttachment(GetCapsuleComponent());
	Camera->SetupAttachment(SpringArm);
	HPBarWidget->SetupAttachment(GetMesh());
	
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
	SpringArm->TargetArmLength = 400.f;
	SpringArm->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	HPBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	static ConstructorHelpers::FClassFinder<UUserWidget> UI_HUD(TEXT("WidgetBlueprint'/Game/UI/UI_HPBar.UI_HPBar_C'"));
	if (UI_HUD.Succeeded())
	{
		HPBarWidget->SetWidgetClass(UI_HUD.Class);
		HPBarWidget->SetDrawSize(FVector2D(150.f, 50.f));
	}
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh>SK_CARDBOARD(TEXT("SkeletalMesh'/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard'"));

	if (SK_CARDBOARD.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(SK_CARDBOARD.Object);
	}
	
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	static ConstructorHelpers::FClassFinder<UAnimInstance> WARRIOR_ANIM(TEXT("AnimBlueprint'/Game/BP_Class/ABP_WarriorAnimBlueprint.ABP_WarriorAnimBlueprint_C'"));
	if (WARRIOR_ANIM.Succeeded())
	{
		// ???????? ???????? ????????.
		GetMesh()->SetAnimInstanceClass(WARRIOR_ANIM.Class);
	}
	
	// SetControlMode(0);
	SetControlMode(EControlMode::GTA);

	ArmLengthSpeed = 3.f;
	ArmRotationSpeed = 10.f;
	GetCharacterMovement()->JumpZVelocity = 800.f;

	IsAttacking = false;

	// Combo
	MaxCombo = 4;
	AttackEndComboState();

	// DrawDebug
	AttackRange = 80.f;
	AttackRadius = 50.f;

	// UI

	// AI
	AIControllerClass = AABAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// GameState
	AssetIndex = 4;

	SetActorHiddenInGame(true);
	HPBarWidget->SetHiddenInGame(true);
	SetCanBeDamaged(false);
	// bCanbeDamaged???? ??????.

	DeadTimer = 5.f;
}

void AABCharacter::SetCharacterState(ECharacterState NewState)
{
	ABCHECK(CurrentState != NewState);
	CurrentState = NewState;

	switch (CurrentState)
	{
		case ECharacterState::LOADING:
		{
			if (bIsPlayer)
			{
				DisableInput(ABPlayerController);

				ABPlayerController->GetHUDWidget()->BindCharacterStat(CharacterStat);
				
				auto ABPlayerState = Cast<AABPlayerState>(GetPlayerState());
				ABCHECK(nullptr != ABPlayerState);
				CharacterStat->SetNewLevel(ABPlayerState->GetCharacterLevel());
			}

			SetActorHiddenInGame(true);
			HPBarWidget->SetHiddenInGame(true);
			SetCanBeDamaged(true);
		}
		break;
		case ECharacterState::READY:
		{
			SetActorHiddenInGame(false);
			HPBarWidget->SetHiddenInGame(false);
			SetCanBeDamaged(true);
			
			CharacterStat->OnHPIsZero.AddLambda
			(
				[this]() -> void
				{
					SetCharacterState(ECharacterState::DEAD);
				}
			);

			auto CharacterWidget = Cast<UABCharacterWidget>(HPBarWidget->GetUserWidgetObject());
			ABCHECK(nullptr != CharacterWidget);
			CharacterWidget->BindCharacterStat(CharacterStat);

			if (bIsPlayer)
			{
				SetControlMode(EControlMode::GTA);
				GetCharacterMovement()->MaxWalkSpeed = 600.f;
				EnableInput(ABPlayerController);
			}
			else
			{
				SetControlMode(EControlMode::NPC);
				GetCharacterMovement()->MaxWalkSpeed = 300.f;
				ABAIController->RunAI();
			}
		}
		break;
		case ECharacterState::DEAD:
		{
			SetActorEnableCollision(false);
			GetMesh()->SetHiddenInGame(false);
			HPBarWidget->SetHiddenInGame(true);
			ABAnim->SetDeadAnim();
			SetCanBeDamaged(false);

			if (bIsPlayer)
			{
				DisableInput(ABPlayerController);
			}
			else
			{
				ABAIController->StopAI();
			}

			GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda
			(
				[this]() -> void
				{
					if (bIsPlayer) 
						ABPlayerController->RestartLevel();
					else 
						Destroy();
				}
			), DeadTimer, false);
		}
		break;
	}
	return;
}

ECharacterState AABCharacter::GetChracterState() const
{
	return CurrentState;
}

int32 AABCharacter::GetExp() const
{
	return CharacterStat->GetDropExp();
}

float AABCharacter::GetFinalAttackRange() const
{
	return (nullptr != CurrentWeapon) ? CurrentWeapon->GetAttackRange() : AttackRange;
}

// Called when the game starts or when spawned
void AABCharacter::BeginPlay()
{
	Super::BeginPlay();	

	bIsPlayer = IsPlayerControlled();
	if (bIsPlayer)
	{
		ABPlayerController = Cast<AABPlayerController>(GetController());
		ABCHECK(nullptr != ABPlayerController);
	}
	else
	{
		ABAIController = Cast<AABAIController>(GetController());
		ABCHECK(nullptr != ABAIController);
	}

	auto DefaultSetting = GetDefault<UABCharacterSetting>();
	
	if (bIsPlayer) AssetIndex = 4;
	else AssetIndex = FMath::RandRange(0, DefaultSetting->CharacterAssets.Num() - 1);

	CharacterAssetToLoad = DefaultSetting->CharacterAssets[AssetIndex];
	auto ABGameInstance = Cast<UABGameInstance>(GetGameInstance());
	ABCHECK(nullptr != ABGameInstance);
	AssetStreamingHandle = ABGameInstance->StreamableManager.RequestAsyncLoad(CharacterAssetToLoad, FStreamableDelegate::CreateUObject(this, &AABCharacter::OnAssetLoadCompleted));
	SetCharacterState(ECharacterState::LOADING);
}

void AABCharacter::OnAssetLoadCompleted()
{
	USkeletalMesh* AssetLoaded = Cast<USkeletalMesh>(AssetStreamingHandle->GetLoadedAsset());
	AssetStreamingHandle.Reset();

	ABCHECK(nullptr != AssetLoaded);

	GetMesh()->SetSkeletalMesh(AssetLoaded);
	SetCharacterState(ECharacterState::READY);
}

void AABCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ABAnim = Cast<UABAnimInstance>(GetMesh()->GetAnimInstance());
	ABCHECK(nullptr != ABAnim);

	ABAnim->OnAttackHitCheck.AddUObject(this, &AABCharacter::AttackCheck);

	if (nullptr != ABAnim)
	{
		ABAnim->OnMontageEnded.AddDynamic(this, &AABCharacter::OnAttackMontageEnded);
	}

	ABAnim->OnNextAttackCheck.AddLambda 
	(
		[this]() -> void
		{
			ABLOG(Warning, TEXT("OnNextAttackCheck"));
			CanNextCombo = false;

			if (IsComboInputOn)
			{
				AttackStartComboState();
				ABAnim->JumpToAttackMontageSection(CurrentCombo);
			}
		}
	);

	// New Trace Channel?? ???? ???? ???????? ????.
	ABAnim->OnAttackHitCheck.AddUObject(this, &AABCharacter::AttackCheck);

	CharacterStat->OnHPIsZero.AddLambda
	(
		[this]() -> void
		{
			ABLOG(Warning, TEXT("OnHPIsZero"));
			ABAnim->SetDeadAnim();
			SetActorEnableCollision(false);
		}
	);
}

float AABCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CharacterStat->SetDamage(FinalDamage);

	if (CurrentState == ECharacterState::DEAD)
	{
		if (EventInstigator->IsPlayerController())
		{
			auto PlayerController = Cast<AABPlayerController>(EventInstigator);
			ABCHECK(nullptr != PlayerController, 0.0f);
			PlayerController->NPCKill(this);
		}
	}
	return FinalDamage;
}

void AABCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (IsPlayerControlled())
	{
		SetControlMode(EControlMode::DIABLO);
		GetCharacterMovement()->MaxWalkSpeed = 600.f;
	}
	else
	{
		SetControlMode(EControlMode::NPC);
		GetCharacterMovement()->MaxWalkSpeed = 300.f;
	}
}

void AABCharacter::SetControlMode(int32 ControlMode)
{
	if (ControlMode == 0)
	{
		SpringArm->TargetArmLength = 450.f;
		SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bInheritPitch = true;
		SpringArm->bInheritYaw = true;
		SpringArm->bInheritRoll = true;
		SpringArm->bDoCollisionTest = true;
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	}
}

void AABCharacter::SetControlMode(EControlMode NewControlMode)
{
	CurrentControlMode = NewControlMode;
	
	switch (CurrentControlMode)
	{
	case EControlMode::GTA:
		{
			// SpringArm->TargetArmLength = 450.f;
			// SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
			ArmLengthTo = 450.f;
			SpringArm->bUsePawnControlRotation = true;
			SpringArm->bInheritPitch = true;
			SpringArm->bInheritYaw = true;
			SpringArm->bInheritRoll = true;
			SpringArm->bDoCollisionTest = true;
			bUseControllerRotationYaw = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
		}
		break;
	case EControlMode::DIABLO:
		{
			// SpringArm->TargetArmLength = 800.f;
			// SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.f, 0.f));
			ArmLengthTo = 800.f;
			ArmRotationTo = FRotator(-45.f, 0.f, 0.f);
			SpringArm->bUsePawnControlRotation = false;
			SpringArm->bInheritPitch = false;
			SpringArm->bInheritYaw = false;
			SpringArm->bInheritRoll = false;
			SpringArm->bDoCollisionTest = false;
			bUseControllerRotationYaw = false;
			GetCharacterMovement()->bOrientRotationToMovement = false;
			GetCharacterMovement()->bUseControllerDesiredRotation = true;
			GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
		}
		break;
	case EControlMode::NPC:
		{
			bUseControllerRotationYaw = false;
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
		}
		break;
	}
}

// Called every frame
void AABCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, ArmLengthTo, DeltaTime, ArmLengthSpeed);

	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), ArmRotationTo, DeltaTime, ArmRotationSpeed));

			if (DirectionToMove.SizeSquared() > 0.f)
			{
				GetController()->SetControlRotation(FRotationMatrix::MakeFromX(DirectionToMove).Rotator());
				AddMovementInput(DirectionToMove);
			}
		}
		break;
	}

	switch (CurrentControlMode)
	{
		case EControlMode::DIABLO:
		{
			if (DirectionToMove.SizeSquared() > 0.f)
			{
				GetController()->SetControlRotation(FRotationMatrix::MakeFromX(DirectionToMove).Rotator());
				AddMovementInput(DirectionToMove);
			}
		}
		break;
	}
}

// Called to bind functionality to input
void AABCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("UpDown"), this, &AABCharacter::UpDown);
	PlayerInputComponent->BindAxis(TEXT("LeftRight"), this, &AABCharacter::LeftRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AABCharacter::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AABCharacter::Turn);

	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &AABCharacter::Jump);

	// View
	PlayerInputComponent->BindAction(TEXT("ViewChange"), EInputEvent::IE_Pressed, this, &AABCharacter::ViewChange );

	// Attack
	PlayerInputComponent->BindAction(TEXT("Attack"), EInputEvent::IE_Pressed, this, &AABCharacter::Attack);
}

bool AABCharacter::CanSetWeapon()
{
	return true;
}

void AABCharacter::SetWeapon(AABWeapon* NewWeapon)
{
	ABCHECK(nullptr != NewWeapon);
	
	if (nullptr != CurrentWeapon)
	{
		CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FName WeaponSocket(TEXT("hand_rSocket"));
	if (nullptr != NewWeapon)
	{
		NewWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WeaponSocket);
		NewWeapon->SetOwner(this);
		CurrentWeapon = NewWeapon;
	}
}

void AABCharacter::UpDown(float NewAxisValue)
{
	if (NewAxisValue == 0.f) return;

	// AddMovementInput(FRotationMatrix(GetControlRotation()).GetUnitAxis(EAxis::X), NewAxisValue);

	// AddMovementInput(FRotationMatrix(GetControlRotation()).GetScaledAxis(EAxis::X), NewAxisValue);

	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X), NewAxisValue);
		}
		break;
		case EControlMode::DIABLO:
		{
			DirectionToMove.X = NewAxisValue;
		}
		break;
	}
}

void AABCharacter::LeftRight(float NewAxisValue)
{
	if (NewAxisValue == 0.f) return;
	// AddMovementInput(FRotationMatrix(GetControlRotation()).GetUnitAxis(EAxis::Y), NewAxisValue);

	// AddMovementInput(FRotationMatrix(GetControlRotation()).GetScaledAxis(EAxis::Y), NewAxisValue);
	// FRotationMatrix : ?????? ???? ?????? ???????? ????
	// GetControlRotation : ???????? ??????
	// ?? GetScaledAxis(Eaxis::X) : X?????? ?????? ??(?????)

	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), NewAxisValue);
		}
		break;
		case EControlMode::DIABLO:
		{
			DirectionToMove.Y = NewAxisValue;
		}
	break;
	}
}

void AABCharacter::LookUp(float NewAxisValue)
{
	// Pitch : Y
	// AddControllerPitchInput(NewAxisValue);

	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			AddControllerPitchInput(NewAxisValue);
		}
		break;
	}
}

void AABCharacter::Turn(float NewAxisValue)
{
	// Yaw : Z
	// AddControllerYawInput(NewAxisValue);

	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			AddControllerYawInput(NewAxisValue);
		}
		break;
	}
}

void AABCharacter::ViewChange()
{
	switch (CurrentControlMode)
	{
		case EControlMode::GTA:
		{
			GetController()->SetControlRotation(GetActorRotation());
			SetControlMode(EControlMode::DIABLO);
		}
		break;
		case EControlMode::DIABLO:
		{
			GetController()->SetControlRotation(SpringArm->GetRelativeRotation());
			SetControlMode(EControlMode::GTA);
		}
		break;
	}
}

void AABCharacter::Attack()
{
	if (IsAttacking)
	{
		ABCHECK(FMath::IsWithinInclusive<int32>(CurrentCombo, 1, MaxCombo));
		if (CanNextCombo)
		{
			IsComboInputOn = true;
		}
	}
	else
	{
		ABCHECK(CurrentCombo == 0);
		AttackStartComboState();
		ABAnim->PlayAttackMontage();
		ABAnim->JumpToAttackMontageSection(CurrentCombo);
		IsAttacking = true;
	}
}

void AABCharacter::AttackCheck()
{
	float FinalAttackRange = GetFinalAttackRange();

	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bResult = GetWorld()->SweepSingleByChannel
	(
		HitResult,
		GetActorLocation(),
		GetActorLocation() + GetActorForwardVector() * FinalAttackRange,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel2,
		FCollisionShape::MakeSphere(AttackRadius),
		Params
	);

#// if ENALBE_DRAW_DEBUG

	FVector TraceVec = GetActorForwardVector() * FinalAttackRange;
	FVector Center = GetActorLocation() + TraceVec * 0.5f;
	float HalfHeight = FinalAttackRange * 0.5f + AttackRadius;
	FQuat CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
	FColor DrawColor = bResult ? FColor::Green : FColor::Red;
	float DebugLifeTime = 2.0f;

	DrawDebugCapsule
	(
		GetWorld(),
		Center,
		HalfHeight,
		AttackRadius,
		CapsuleRot,
		DrawColor,
		false,
		DebugLifeTime
	);

// #endif

	if (bResult)
	{
		if (HitResult.Actor.IsValid())
		{
			ABLOG(Warning, TEXT("Hit Actor Name : %s"), *HitResult.Actor->GetName());
		}

		// Attack Damage
		FDamageEvent DamageEvent;
		HitResult.Actor->TakeDamage(CharacterStat->GetAttack(), DamageEvent, GetController(), this);
	}
}

void AABCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ABCHECK(IsAttacking);
	ABCHECK(CurrentCombo > 0);
	ABLOG_S(Warning);
	IsAttacking = false;
	AttackEndComboState();

	OnAttackEnd.Broadcast();
}

void AABCharacter::AttackStartComboState()
{
	CanNextCombo = true;
	IsComboInputOn = false;
	ABCHECK(FMath::IsWithinInclusive<int32>(CurrentCombo, 0, MaxCombo - 1));
	CurrentCombo = FMath::Clamp<int32>(CurrentCombo + 1, 1, MaxCombo);
}

void AABCharacter::AttackEndComboState()
{
	IsComboInputOn = false;
	CanNextCombo = false;
	CurrentCombo = 0;
}
