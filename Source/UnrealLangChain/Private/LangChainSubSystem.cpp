// Fill out your copyright notice in the Description page of Project Settings.


#include "LangChainSubSystem.h"

#include "Misc/MonitoredProcess.h"

void ULangChainSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("ULangChainSubsystem Initializing"));
	
	const FString ExecPath = FPaths::ProjectDir() + "ThirdParty/LangChain/main.exe";

	auto PythonTest = FMonitoredProcess(ExecPath, TEXT("Dirk"), "D:", true, true);
	
	PythonTest.OnOutput().BindLambda(&ULangChainSubSystem::Handle);

	
	PythonTest.OnCanceled().BindLambda([]()
	{
		UE_LOG(LogTemp, Warning, TEXT("exe got canceled"));
	});
	
	PythonTest.OnCompleted().BindLambda([](const int32 Code)
	{
		UE_LOG(LogTemp, Warning, TEXT("exe completed with code %d"), Code);
	});
	
	if(!PythonTest.Launch())
	{
		UE_LOG(LogTemp, Error, TEXT("Error occured while executing exe!"));
	}

	while(PythonTest.Update())
	{
		FPlatformProcess::Sleep(1);
	}
}

void ULangChainSubSystem::Handle(const FString &ProcessOutput)
{
	UE_LOG(LogTemp, Log, TEXT("Exe Output: %s"), *ProcessOutput);
}
