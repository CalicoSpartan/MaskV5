// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSProjectGameModeBase.h"
#include "FPSCharacter.h"
#include "PlayerHUD.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/GameFramework/GameMode.h"
#include "FPSPlayerState.h"

#include "FPSPlayerController.h"
#include "Engine.h"

AFPSProjectGameModeBase::AFPSProjectGameModeBase()
{

	
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_FPSCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	
	// set the type of HUD used in the game
	static ConstructorHelpers::FClassFinder<AHUD> PlayerHUDClass(TEXT("/Game/Blueprints/BP_PlayerHUD"));
	if (PlayerHUDClass.Class != NULL) {
		HUDClass = PlayerHUDClass.Class;
	}


	Team1Players = 0;
	Team2Players = 0;
	Team3Players = 0;
	Team4Players = 0;
	Team5Players = 0;
	SafeSpawnDistance = 1500;
	HasLoggedIn = false;
	NumTeamA = 0;
	NumTeamB = 0;
	SpawnDelay = 2;

	AddKillFeedEntry = false;
	//set the type of gamestate used in the game
	GameStateClass = AFPSGameState::StaticClass();

	PlayerControllerClass = AFPSPlayerController::StaticClass();

	PlayerStateClass = AFPSPlayerState::StaticClass();
	NumberOfTeams = 2;
}

void AFPSProjectGameModeBase::RespawnPlayer(APlayerController* NewPlayer)
{
	UE_LOG(LogClass, Log, TEXT("Respawning"));
	AFPSPlayerController* PlayerController = Cast<AFPSPlayerController>(NewPlayer);
	TArray<AFPSPlayerStart*> PreferredStarts;
	bool checkforsafety = true;
	for (int32 i = 0; i < 5; ++i) {
		for (TActorIterator<AFPSPlayerStart> PlayerStart(GetWorld()); PlayerStart; ++PlayerStart)
		{
			if (PlayerController)
			{
				if (PlayerStart->Tags.Contains("RespawnPoint"))
				{
					for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
					{
						AFPSPlayerController* PlayerController = Cast<AFPSPlayerController>(*Iterator);
						if (PlayerController)
						{
							if (PlayerController != NewPlayer) {
								if (AFPSPlayerController* NewFPSPlayer = Cast<AFPSPlayerController>(NewPlayer))
								{


									if (PlayerController->GetPlayerTeam() != NewFPSPlayer->GetPlayerTeam())
									{
										AFPSCharacter* PlayerCharacter = Cast<AFPSCharacter>(PlayerController->GetPawn());
										if (PlayerCharacter) {
											UCameraComponent* PlayerCamera = Cast<UCameraComponent>(PlayerCharacter->FPSCameraComponent);
											if (PlayerCamera)
											{
												FVector CameraLocation = PlayerCamera->GetComponentLocation();
												FVector SpawnPointLocation = PlayerStart->GetActorLocation();
												FVector DirectionBetween = (SpawnPointLocation - CameraLocation).GetSafeNormal();
												FVector CameraDirection = PlayerCamera->GetForwardVector().GetSafeNormal();
												float dotvalue = FGenericPlatformMath::Abs(FVector::DotProduct(DirectionBetween, CameraDirection));

												if (dotvalue > .68f) {
			

													FHitResult hit;
													
													if (GetWorld()->LineTraceSingleByChannel(hit, CameraLocation, CameraLocation + (DirectionBetween * 10000), ECollisionChannel::ECC_Camera))
													{


														if (AFPSPlayerStart* spawnpoint = Cast<AFPSPlayerStart>(hit.GetActor())) {
		
														}
														else {

	
															PreferredStarts.AddUnique(*PlayerStart);
														}
													}

													if (i >= 3) {
														UE_LOG(LogClass, Log, TEXT("Respawning anyway"));
														PreferredStarts.AddUnique(*PlayerStart);
													}

												}
												else
												{
													if (FVector::Dist(CameraLocation, SpawnPointLocation) > SafeSpawnDistance || i >= 3) {
														PreferredStarts.AddUnique(*PlayerStart);
													}

												}

											}
										}
									}
								}
							}
						}

					}
				}
			}
		}
		if (PreferredStarts.Num() == 0) {

		}
		else
		{
			break;
		}
	}
	if (PreferredStarts.Num() > 1) {
		AFPSPlayerStart* BestStart = PreferredStarts[FMath::RandRange(0, PreferredStarts.Num() - 1)];
		float SpawnDistance = 0.0f;
		for (int32 i = 0; i < PreferredStarts.Num(); ++i) {
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				
				AFPSPlayerController* PlayerController = Cast<AFPSPlayerController>(*Iterator);
				if (PlayerController)
				{
					if (PlayerController != NewPlayer) {
						if (AFPSPlayerController* NewFPSPlayer = Cast<AFPSPlayerController>(NewPlayer))
						{
							if (PlayerController->GetPlayerTeam() != NewFPSPlayer->GetPlayerTeam())
							{


								AFPSCharacter* PlayerCharacter = Cast<AFPSCharacter>(PlayerController->GetPawn());
								if (PlayerCharacter) {
									UCameraComponent* PlayerCamera = Cast<UCameraComponent>(PlayerCharacter->FPSCameraComponent);
									if (PlayerCamera)
									{
										FVector CameraLocation = PlayerCamera->GetComponentLocation();
										FVector SpawnPointLocation = PreferredStarts[i]->GetActorLocation();
										if (FVector::Dist(CameraLocation, SpawnPointLocation) > SpawnDistance)
										{
											BestStart = PreferredStarts[i];
											SpawnDistance = FVector::Dist(CameraLocation, SpawnPointLocation);
										}
									}
								}
							}
						}
					}
				}
			}

		}
		int32 PlayerStartIndex = FMath::RandRange(0, PreferredStarts.Num() - 1);
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, BestStart));

		RestartPlayer(NewPlayer);
		AFPSCharacter* Character = Cast<AFPSCharacter>(NewPlayer->GetPawn());
		if (Character)
		{
			Character->TriggerAddUI();

		}
	}
	else {
		int32 PlayerStartIndex = 0;
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, PreferredStarts[PlayerStartIndex]));

		RestartPlayer(NewPlayer);
		AFPSCharacter* Character = Cast<AFPSCharacter>(NewPlayer->GetPawn());
		if (Character)
		{
			Character->TriggerAddUI();

		}
	}

}



void AFPSProjectGameModeBase::StartNewPlayerClient(APlayerController* NewPlayer)
{
	NewPlayer->GetPawn()->Destroy();
	AFPSPlayerController* TestPlayerController = Cast<AFPSPlayerController>(NewPlayer);
	TArray<AFPSPlayerStart*> PreferredStarts;
	bool BlockCheck = true;
	for (int32 i = 0; i < 2; ++i) {

		for (TActorIterator<AFPSPlayerStart> PlayerStart(GetWorld()); PlayerStart; ++PlayerStart)
		{

			if (TestPlayerController)
			{
				if (TestPlayerController->GetPlayerTeam() == 1 && PlayerStart->Tags.Contains("Team1")) //&& PlayerStart->PlayerStartTag != FName(TEXT("Blocked")))
				{
					if (PlayerStart->PlayerStartTag == FName(TEXT("Blocked"))) {
						if (BlockCheck == false) {
							
							PreferredStarts.AddUnique(*PlayerStart);
						}
						else {
							
						}
						
					}
					else
					{
		
						PreferredStarts.AddUnique(*PlayerStart);
					}
					// Player should spawn on CT.
					

				}
				else if (TestPlayerController->GetPlayerTeam() == 2 && PlayerStart->Tags.Contains("Team2")) //&& PlayerStart->PlayerStartTag != FName(TEXT("Blocked")))
				{
					if (PlayerStart->PlayerStartTag == FName(TEXT("Blocked"))) {
						if (BlockCheck == false) {
							
							PreferredStarts.AddUnique(*PlayerStart);
						}
						else {
							
						}
						
					}
					else {
						
						PreferredStarts.AddUnique(*PlayerStart);
					}
					// Player should spawn on Suspects
					

				}

			}

		}
		if (PreferredStarts.Num() == 0) {
			BlockCheck = false;
		}
		else {
			break;
		}
		
	}
	if (TestPlayerController && TestPlayerController->GetPlayerTeam() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Player is a spectator");
		return;
	}
	else
	{
		//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Player has selected a Team");
		int32 PlayerStartIndex = FMath::RandRange(0, PreferredStarts.Num() - 1);
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, PreferredStarts[PlayerStartIndex]));
		PreferredStarts[PlayerStartIndex]->PlayerStartTag = FName(TEXT("Blocked"));

		RestartPlayer(NewPlayer);
		AFPSCharacter* Character = Cast<AFPSCharacter>(NewPlayer->GetPawn());
		if (Character)
		{
			Character->TriggerAddUI();

		}
	}
}


void AFPSProjectGameModeBase::StartNewPlayer(APlayerController* NewPlayer)
{
	
	AFPSPlayerController* TestPlayerController = Cast<AFPSPlayerController>(NewPlayer);

	TArray<AFPSPlayerStart*> PreferredStarts;



	for (TActorIterator<AFPSPlayerStart> PlayerStart(GetWorld()); PlayerStart; ++PlayerStart)
	{

		
		if (TestPlayerController)
		{

			if (TestPlayerController->GetPlayerTeam() == 1 && PlayerStart->Tags.Contains("Team1"))
			{
				
				// Player should spawn on CT.
				PreferredStarts.Add(*PlayerStart);

			}
			else if (TestPlayerController->GetPlayerTeam() == 2 && PlayerStart->Tags.Contains("Team2"))
			{
				
				// Player should spawn on Suspects
				PreferredStarts.Add(*PlayerStart);

			}
			
		}
	}
	if (TestPlayerController && TestPlayerController->GetPlayerTeam() == 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, "Player is a spectator");
		return;
	}
	else
	{
		if (TestPlayerController->GetPlayerTeam() == 1) {
			Team1Players += 1;

		}
		if (TestPlayerController->GetPlayerTeam() == 2) {
			Team2Players += 1;

		}

		NewPlayer->GetPawn()->Destroy();
		int32 StartSpotIndex = FMath::RandRange(0, PreferredStarts.Num() - 1);
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, PreferredStarts[StartSpotIndex]));
		for (TActorIterator<AFPSPlayerStart> ClearPlayerStart(GetWorld()); ClearPlayerStart; ++ClearPlayerStart) {
			ClearPlayerStart->PlayerStartTag = FName(TEXT("Open"));
		}

		PreferredStarts[StartSpotIndex]->PlayerStartTag = FName(TEXT("Blocked"));

		RestartPlayer(NewPlayer);
		AFPSCharacter* Character = Cast<AFPSCharacter>(NewPlayer->GetPawn());
		if (Character)
		{
			Character->TriggerAddUI();

		}
	}


	//TArray<AFPSPlayerController> PlayerControllers;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFPSPlayerController::StaticClass(), PlayerControllers);

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AFPSPlayerController* PlayerController = Cast<AFPSPlayerController>(*Iterator);
		if (PlayerController) {
			if (PlayerController != NewPlayer) {
				if (Team1Players > Team2Players) {

					Team2Players++;
					PlayerController->ServerSetPlayerTeamClient(2);
				}
				else {
	
					Team1Players++;
					PlayerController->ServerSetPlayerTeamClient(1);
				}
			}
			
		}
		
		
	}

}







void AFPSProjectGameModeBase::PostLogin(APlayerController * NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
}


void AFPSProjectGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	check(World);
	AFPSGameState* MyGameState = Cast<AFPSGameState>(GameState);
	check(MyGameState);
	HandleNewState(EGamePlayState::EPlaying);

	



}

void AFPSProjectGameModeBase::Update()
{


}

void AFPSProjectGameModeBase::HandleNewState(EGamePlayState NewState)
{
	UWorld* World = GetWorld();
	check(World);
	AFPSGameState* MyGameState = Cast<AFPSGameState>(GameState);
	check(MyGameState);

	if (NewState != MyGameState->GetCurrentState())
	{
		//update the state, so clients know about the transition
		MyGameState->SetCurrentState(NewState);

		switch (NewState)
		{
		case EGamePlayState::EWaiting:

			break;
		case EGamePlayState::EPlaying:

			//start draining power
			GetWorldTimerManager().SetTimer(UpdateTimer, this, &AFPSProjectGameModeBase::Update, UpdateDelay, true);
			break;
		case EGamePlayState::EGameOver:

			//stop draining power
			GetWorldTimerManager().ClearTimer(UpdateTimer);
			break;
		default:
		case EGamePlayState::EUnknown:
			break;
		}
	}

}



