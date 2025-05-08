// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "base/nodes/IDS_Node.h"

#include "IDS_Node_Family.generated.h"

/**
 * 
 */
UCLASS()
class BA_PROJECT_API UIDS_Node_Family : public UIDS_Node
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category=Property)
	FString Name;
};
