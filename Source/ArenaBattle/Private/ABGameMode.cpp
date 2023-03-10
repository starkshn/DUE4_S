// Fill out your copyright notice in the Description page of Project Settings.


#include "ABGameMode.h"
#include "ABPawn.h"
#include "ABPlayerController.h"
#include "ABCharacter.h"
#include "ABPlayerState.h"
#include "ABGameState.h"

AABGameMode::AABGameMode()
{
	// 게임모드에 폰을 지정하는 코드
	// DefaultPawnClass = AABPawn::StaticClass();
	DefaultPawnClass = AABCharacter::StaticClass();
	PlayerControllerClass = AABPlayerController::StaticClass();
	PlayerStateClass = AABPlayerState::StaticClass();
	GameStateClass = AABGameState::StaticClass();
}

void AABGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ABGameState = Cast<AABGameState>(GameState);
}

void AABGameMode::PostLogin(APlayerController* NewPlayer)
{
	ABLOG(Warning, TEXT("Post Login Begin"));
	Super::PostLogin(NewPlayer);
	ABLOG(Warning, TEXT("PostLogin End"));

	auto ABPlayerState = Cast<AABPlayerState>(NewPlayer->PlayerState);
	ABCHECK(nullptr != ABPlayerState);
	ABPlayerState->InitPlayerData();
}

void AABGameMode::AddScore(AABPlayerController* ScoredPlayer)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const auto ABPlayerController = Cast<AABPlayerController>(It->Get());
		if ((nullptr != ABPlayerController) && (ScoredPlayer == ABPlayerController))
		{
			ABPlayerController->AddGameScore();
			break;
		}
	}

	ABGameState->AddGameScore();
}


