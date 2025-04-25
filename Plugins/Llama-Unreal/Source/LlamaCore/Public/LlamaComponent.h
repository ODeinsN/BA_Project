// Copyright 2025-current Getnamo.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LlamaDataTypes.h"

#include "LlamaComponent.generated.h"

/** 
* Actor component API access to LLM. Each component wraps its own Model and context state, allows for multiple parallel LLMs.
* Inherits lifetime from parent, typically in a system-type actor that means it will unload on level exit. If you wish to have
* an LLM that survives level transitions, consider using LlamaSubsystem.
*/
UCLASS(Category = "LLM", BlueprintType, meta = (BlueprintSpawnableComponent))
class LLAMACORE_API ULlamaComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    ULlamaComponent(const FObjectInitializer &ObjectInitializer);
    ~ULlamaComponent();

    virtual void Activate(bool bReset) override;
    virtual void Deactivate() override;
    virtual void TickComponent(float DeltaTime,
                                ELevelTick TickType,
                                FActorComponentTickFunction* ThisTickFunction) override;

    //Main callback, updates for each token generated
    UPROPERTY(BlueprintAssignable)
    FOnTokenGeneratedSignature OnTokenGenerated;

    //Only called when full response has been received (EOS/etc). Usually bandwidth bound operation, TPS given for TGS.
    UPROPERTY(BlueprintAssignable)
    FOnResponseGeneratedSignature OnResponseGenerated;

    //Response split by punctuation emit e.g. sentence level emits. Useful for speech generation type tasks.
    UPROPERTY(BlueprintAssignable)
    FOnPartialSignature OnPartialGenerated;

    //Usually processing bound operation; TPS given for PPS
    UPROPERTY(BlueprintAssignable)
    FOnPromptProcessedSignature OnPromptProcessed;

    //Whenever the model stops generating
    UPROPERTY(BlueprintAssignable)
    FOnEndOfStreamSignature OnEndOfStream;

    UPROPERTY(BlueprintAssignable)
    FVoidEventSignature OnContextReset;

    UPROPERTY(BlueprintAssignable)
    FModelNameSignature OnModelLoaded;

    //Catch internal errors
    UPROPERTY(BlueprintAssignable)
    FOnErrorSignature OnError;

    //Modify these before loading model to apply settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Model Component")
    FLLMModelParams ModelParams;

    //This state gets updated typically after every response
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Model Component")
    FLLMModelState ModelState;

    //Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Model Component")
    bool bDebugLogModelOutput = false;

    //toggle to pay copy cost or not, default true
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LLM Model Component")
    bool bSyncPromptHistory = true;

    //loads model from ModelParams
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void LoadModel();

    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void UnloadModel();

    UFUNCTION(BlueprintPure, Category = "LLM Model Component")
    bool IsModelLoaded();


    //Clears the prompt, allowing a new context - optionally keeping the initial system prompt
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void ResetContextHistory(bool bKeepSystemPrompt = false);

    //removes what the LLM replied
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void RemoveLastAssistantReply();

    //removes what you said and what the LLM replied
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void RemoveLastUserInput();

    //Main input function
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void InsertTemplatedPrompt(UPARAM(meta=(MultiLine=true)) const FString& Text, EChatTemplateRole Role = EChatTemplateRole::User, bool bAddAssistantBOS = false, bool bGenerateReply = true);

    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void InsertFormattedPrompt(UPARAM(meta=(MultiLine=true)) const FString& Text, TMap<FString, FString> InjectedValues, EChatTemplateRole Role = EChatTemplateRole::User, bool bAddAssistantBOS = false, bool bGenerateReply = true);
    
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void InsertTemplatedPromptStruct(const FLlamaChatPrompt& ChatPrompt);

    //does not apply formatting before running inference
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void InsertRawPrompt(UPARAM(meta = (MultiLine = true)) const FString& Text, bool bGenerateReply = true);

    //if you want to manually wrap prompt, if template is empty string, default model template is applied. NB: this function should be thread safe, but this has not be thoroughly tested.
    UFUNCTION(BlueprintPure, Category = "LLM Model Component")
    FString WrapPromptForRole(const FString& Text, EChatTemplateRole Role, const FString& OverrideTemplate);

    //Force stop generating new tokens
    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void StopGeneration();

    UFUNCTION(BlueprintCallable, Category = "LLM Model Component")
    void ResumeGeneration();

    //Obtain the currently formatted context
    UFUNCTION(BlueprintPure, Category = "LLM Model Component")
    FString RawContextHistory();

    UFUNCTION(BlueprintPure, Category = "LLM Model Component")
    FStructuredChatHistory GetStructuredChatHistory();

private:
    class FLlamaNative* LlamaNative;
};
