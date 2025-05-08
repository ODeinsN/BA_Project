// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MutexBool.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class BA_PROJECT_API UMutexBool : public UObject
{
	GENERATED_BODY()
	mutable FCriticalSection DataGuard {};

	bool Value;

public:
	UFUNCTION(BlueprintCallable)
	void Lock() const
	{
		DataGuard.Lock();
	}

	UFUNCTION(BlueprintCallable)
	bool Get() const
	{
		return Value;
	}

	UFUNCTION(BlueprintCallable)
	void SetValue(const bool UpdatedValue)
	{
		Value = UpdatedValue;
	}

	UFUNCTION(BlueprintCallable)
	void Unlock() const
	{
		DataGuard.Unlock();
	}
};
