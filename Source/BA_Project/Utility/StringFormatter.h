// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StringFormatter.generated.h"

/**
 * 
 */
UCLASS()
class BA_PROJECT_API UStringFormatter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, BlueprintPure)
	static void ReduceSpaces(const FString& InputText, FString &ReplacementString);
};
