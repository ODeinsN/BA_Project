// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "base/nodes/IDS_Node.h"

#include "IDS_Node_Profession.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class BA_PROJECT_API UIDS_Node_Profession : public UIDS_Node
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Property)
	FString Name;
};
