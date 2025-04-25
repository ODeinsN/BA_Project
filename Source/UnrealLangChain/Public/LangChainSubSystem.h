// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "LangChainSubSystem.generated.h"

/**
 * 
 */
UCLASS()
class UNREALLANGCHAIN_API ULangChainSubSystem : public UEngineSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static void Handle(const FString &ProcessOutput);
};
