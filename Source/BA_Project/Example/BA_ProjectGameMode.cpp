// Copyright Epic Games, Inc. All Rights Reserved.

#include "BA_ProjectGameMode.h"
#include "BA_ProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

ABA_ProjectGameMode::ABA_ProjectGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
