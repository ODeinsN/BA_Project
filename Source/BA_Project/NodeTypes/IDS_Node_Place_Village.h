// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDS_Node_Place.h"
#include "IDS_Node_Place_Village.generated.h"

/**
 * 
 */
UCLASS()
class BA_PROJECT_API UIDS_Node_Place_Village : public UIDS_Node_Place
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category=Property)
	int64 Founded;

	UPROPERTY(EditAnywhere, Category=Property)
	int64 CitizenCount;
};
