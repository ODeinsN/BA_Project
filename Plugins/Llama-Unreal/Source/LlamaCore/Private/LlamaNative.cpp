// Copyright 2025-current Getnamo.

#include "LlamaNative.h"
#include "LlamaUtility.h"
#include "Internal/LlamaInternal.h"
#include "Async/TaskGraphInterfaces.h"
#include "Async/Async.h"
#include "Tickable.h"

FLlamaNative::FLlamaNative()
{
    Internal = new FLlamaInternal();

    //Hookup internal listeners - these get called on BG thread
    Internal->OnTokenGenerated = [this](const std::string& TokenPiece)
    {
        const FString Token = FLlamaString::ToUE(TokenPiece);

        //Accumulate
        CombinedPieceText += Token;

        FString Partial;

        //Compute Partials
        if (ModelParams.Advanced.bEmitPartials)
        {
            bool bSplitFound = false;
            //Check new token for separators
            for (const FString& Separator : ModelParams.Advanced.PartialsSeparators)
            {
                if (Token.Contains(Separator))
                {
                    bSplitFound = true;
                }
            }
            if (bSplitFound)
            {
                Partial = FLlamaString::GetLastSentence(CombinedPieceText);
            }
        }

        //Emit token to game thread
        if (OnTokenGenerated)
        {
            EnqueueGTTask([this, Token, Partial]()
            {
                if (OnTokenGenerated)
                {
                    OnTokenGenerated(Token);
                }
                if (OnPartialGenerated && !Partial.IsEmpty())
                {
                    OnPartialGenerated(Partial);
                }
            });
        }
    };

    Internal->OnGenerationComplete = [this](const std::string& Response, float Duration, int32 TokensGenerated, float SpeedTps)
    {
        if (ModelParams.Advanced.bLogGenerationStats)
        {
            UE_LOG(LlamaLog, Log, TEXT("Generated %d tokens in %1.2fs (%1.2ftps)"), TokensGenerated, Duration, SpeedTps);
        }

        int32 UsedContext = UsedContextLength();

        //Sync history data on bg thread
        SyncModelStateToInternal([this, UsedContext, SpeedTps]
        {
            ModelState.ContextUsed = UsedContext;
            ModelState.LastTokenGenerationSpeed = SpeedTps;
        });

        //Clear our partial text parser
        CombinedPieceText.Empty();

        //Emit response generated to general listeners
        FString ResponseString = FLlamaString::ToUE(Response);
        EnqueueGTTask([this, ResponseString]
        {
            if (OnResponseGenerated)
            {
                OnResponseGenerated(ResponseString);
            }
        });
    };

    Internal->OnPromptProcessed = [this](int32 TokensProcessed, EChatTemplateRole RoleProcessed, float SpeedTps)
    {
        int32 UsedContext = UsedContextLength();

        //Sync history data with additional state updates
        SyncModelStateToInternal([this, UsedContext, SpeedTps]
        {
            ModelState.ContextUsed = UsedContext;
            ModelState.LastPromptProcessingSpeed = SpeedTps;
        });

        //Separate enqueue to ensure it happens after modelstate update
        EnqueueGTTask([this, TokensProcessed, RoleProcessed, SpeedTps]
        {
            if (OnPromptProcessed)
            {
                OnPromptProcessed(TokensProcessed, RoleProcessed, SpeedTps);
            }
        });
    };

    Internal->OnError = [this](const FString& ErrorMessage, int32 ErrorCode)
    {
        const FString ErrorMessageGTSafe = ErrorMessage;
        EnqueueGTTask([this, ErrorMessageGTSafe, ErrorCode]
        {
            if (OnError)
            {
                OnError(ErrorMessageGTSafe, ErrorCode);
            }
        });
    };
}

FLlamaNative::~FLlamaNative()
{
    StopGeneration();
    bThreadShouldRun = false;
    
    //Remove ticker if active
    RemoveTicker();

    //Wait for the thread to stop
    while (bThreadIsActive) 
    {
        FPlatformProcess::Sleep(0.01f);
    }
    delete Internal;
}

void FLlamaNative::SyncModelStateToInternal(TFunction<void()> AdditionalGTStateUpdates)
{
    TFunction<void(int64)> BGSyncAction = [this, AdditionalGTStateUpdates](int64 TaskId)
    {
        //Copy states internal states, emit to 
        FStructuredChatHistory ChatHistory;
        FString ContextHistory;
        GetStructuredChatHistory(ChatHistory);
        RawContextHistory(ContextHistory);

        EnqueueGTTask([this, ChatHistory, ContextHistory, AdditionalGTStateUpdates]
        {
            //Update state on gamethread
            ModelState.ChatHistory = ChatHistory;
            ModelState.ContextHistory = ContextHistory;

            //derived state update
            if (ModelState.ChatHistory.History.Num() > 0)
            {
                ModelState.LastRole = ModelState.ChatHistory.History.Last().Role;
            }

            //Run the updates before model state changes happen
            if (AdditionalGTStateUpdates)
            {
                AdditionalGTStateUpdates();
            }

            //Emit model state update to GT listeners
            if (OnModelStateChanged)
            {
                OnModelStateChanged(ModelState);
            }
        }, TaskId);
    };

    if (IsInGameThread())
    {
        EnqueueBGTask(BGSyncAction);
    }
    else
    {
        //Call directly
        BGSyncAction(GetNextTaskId());
    }
    
}

void FLlamaNative::StartLLMThread()
{
    bThreadShouldRun = true;
    Async(EAsyncExecution::Thread, [this]
    {
        bThreadIsActive = true;

        while (bThreadShouldRun)
        {
            //Run all queued tasks
            while (!BackgroundTasks.IsEmpty())
            {
                FLLMThreadTask Task;
                BackgroundTasks.Dequeue(Task);
                if (Task.TaskFunction)
                {
                    //Run Task
                    Task.TaskFunction(Task.TaskId);
                }
            }

            FPlatformProcess::Sleep(ThreadIdleSleepDuration);
        }

        bThreadIsActive = false;
    });
}

int64 FLlamaNative::GetNextTaskId()
{
    //technically returns an int32
    return TaskIdCounter.Increment();
}

void FLlamaNative::EnqueueBGTask(TFunction<void(int64)> TaskFunction)
{
    //Lazy start the thread on first enqueue
    if (!bThreadIsActive)
    {
        StartLLMThread();
    }

    FLLMThreadTask Task;
    Task.TaskId = GetNextTaskId();
    Task.TaskFunction = TaskFunction;

    BackgroundTasks.Enqueue(Task);
}

void FLlamaNative::EnqueueGTTask(TFunction<void()> TaskFunction, int64 LinkedTaskId)
{
    FLLMThreadTask Task;
    
    if (LinkedTaskId == -1)
    {
        Task.TaskId = GetNextTaskId();
    }
    else
    {
        Task.TaskId = LinkedTaskId;
    }

    Task.TaskFunction = [TaskFunction](int64 InTaskId) 
    {
        TaskFunction();
    };

    GameThreadTasks.Enqueue(Task);
}

void FLlamaNative::SetModelParams(const FLLMModelParams& Params)
{
	ModelParams = Params;
}

void FLlamaNative::LoadModel(TFunction<void(const FString&, int32 StatusCode)> ModelLoadedCallback)
{
    EnqueueBGTask([this, ModelLoadedCallback](int64 TaskId)
    {
        //Unload first if any is loaded
        Internal->UnloadModel();

        //Now load it
        bool bSuccess = Internal->LoadModelFromParams(ModelParams);

        //Sync model state
        if (bSuccess)
        {
            const FString TemplateString = FLlamaString::ToUE(Internal->Template);
            const FString TemplateSource = FLlamaString::ToUE(Internal->TemplateSource);

            EnqueueGTTask([this, TemplateString, TemplateSource, ModelLoadedCallback]
            {
                FJinjaChatTemplate ChatTemplate;
                ChatTemplate.TemplateSource = TemplateSource;
                ChatTemplate.Jinja = TemplateString;

                ModelState.ChatTemplateInUse = ChatTemplate;
                ModelState.bModelIsLoaded = true;

                if (OnModelStateChanged)
                {
                    OnModelStateChanged(ModelState);
                }

                if (ModelLoadedCallback)
                {
                    ModelLoadedCallback(ModelParams.PathToModel, 0);
                }
            }, TaskId);
        }
        else
        {
            EnqueueGTTask([this, ModelLoadedCallback]
            {
                //On error will be triggered earlier in the chain, but forward our model loading error status here
                ModelLoadedCallback(ModelParams.PathToModel, 15);
            }, TaskId);
        }
    });
}

void FLlamaNative::UnloadModel(TFunction<void(int32 StatusCode)> ModelUnloadedCallback)
{
    EnqueueBGTask([this, ModelUnloadedCallback](int64 TaskId)
    {
        if (IsModelLoaded())
        {
            Internal->UnloadModel();
        }

        //Reply with code
        EnqueueGTTask([this, ModelUnloadedCallback]
        {
            ModelState.bModelIsLoaded = false;

            if (OnModelStateChanged)
            {
                OnModelStateChanged(ModelState);
            }

            if (ModelUnloadedCallback)
            {
                ModelUnloadedCallback(0);
            }
        });
    });
}

bool FLlamaNative::IsModelLoaded()
{
    return Internal->IsModelLoaded();
}

void FLlamaNative::InsertTemplatedPrompt(const FLlamaChatPrompt& Prompt, TFunction<void(const FString& Response)> OnResponseFinished)
{
    if (!IsModelLoaded())
    {
        UE_LOG(LlamaLog, Warning, TEXT("Model isn't loaded, can't run prompt."));
        return;
    }

    //Copy so we can deal with it on different threads
    FLlamaChatPrompt ThreadSafePrompt = Prompt;

    //run prompt insert on a background thread
    EnqueueBGTask([this, ThreadSafePrompt, OnResponseFinished](int64 TaskId)
    {
        const std::string UserStdString = FLlamaString::ToStd(ThreadSafePrompt.Prompt);
        
        if (ThreadSafePrompt.bGenerateReply)
        {
            FString Response = FLlamaString::ToUE(Internal->InsertTemplatedPrompt(UserStdString, ThreadSafePrompt.Role, ThreadSafePrompt.bAddAssistantBOS, true));

            //NB: OnResponseGenerated will also be called separately from this
            EnqueueGTTask([this, Response, OnResponseFinished]()
            {
                if (OnResponseFinished)
                {
                    OnResponseFinished(Response);
                }
            });
        }
        else
        {
            //We don't want to generate a reply, just append a prompt. (last param = false turns it off)
            Internal->InsertTemplatedPrompt(UserStdString, ThreadSafePrompt.Role, ThreadSafePrompt.bAddAssistantBOS, false);
        }
    });
}

void FLlamaNative::InsertRawPrompt(const FString& Prompt, bool bGenerateReply, TFunction<void(const FString& Response)>OnResponseFinished)
{
    if (!IsModelLoaded())
    {
        UE_LOG(LlamaLog, Warning, TEXT("Model isn't loaded, can't run prompt."));
        return;
    }

    const std::string PromptStdString = FLlamaString::ToStd(Prompt);

    EnqueueBGTask([this, PromptStdString, OnResponseFinished, bGenerateReply](int64 TaskId)
    {
        FString Response = FLlamaString::ToUE(Internal->InsertRawPrompt(PromptStdString, bGenerateReply));
        EnqueueGTTask([this, Response, OnResponseFinished]
        {
            if (OnResponseFinished)
            {
                OnResponseFinished(Response);
            }
        });
    });
}

void FLlamaNative::RemoveLastNMessages(int32 MessageCount)
{
    EnqueueBGTask([this, MessageCount](int64 TaskId)
    {
        Internal->RollbackContextHistoryByMessages(MessageCount);

        //Sync state
        SyncModelStateToInternal();
    });
}

bool FLlamaNative::IsGenerating()
{
    //this is threadsafe
    return Internal->IsGenerating();
}

void FLlamaNative::StopGeneration()
{
    //this is threadsafe
    Internal->StopGeneration();
}

void FLlamaNative::ResumeGeneration()
{
    if (!IsModelLoaded())
    {
        UE_LOG(LlamaLog, Warning, TEXT("Model isn't loaded, can't ResumeGeneration."));
        return;
    }

    EnqueueBGTask([this](int64 TaskId)
    {
        Internal->ResumeGeneration();
    });
}

void FLlamaNative::ClearPendingTasks(bool bClearGameThreadCallbacks)
{
    BackgroundTasks.Empty();

    if (bClearGameThreadCallbacks)
    {
        GameThreadTasks.Empty();
    }
}

void FLlamaNative::OnGameThreadTick(float DeltaTime)
{
    //Handle all the game thread callbacks
    if (!GameThreadTasks.IsEmpty())
    {
        //Run all queued tasks
        while (!GameThreadTasks.IsEmpty())
        {
            FLLMThreadTask Task;
            GameThreadTasks.Dequeue(Task);
            if (Task.TaskFunction)
            {
                //Run Task
                Task.TaskFunction(Task.TaskId);
            }
        }
    }
}

void FLlamaNative::AddTicker()
{
    TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float DeltaTime)
    {
        OnGameThreadTick(DeltaTime);
        return true;
    }));
}

void FLlamaNative::RemoveTicker()
{
    if (IsNativeTickerActive())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
        TickDelegateHandle = nullptr;
    }
}

bool FLlamaNative::IsNativeTickerActive()
{
    return TickDelegateHandle.IsValid();
}

void FLlamaNative::ResetContextHistory(bool bKeepSystemPrompt)
{
    EnqueueBGTask([this, bKeepSystemPrompt](int64 TaskId)
    {
        Internal->ResetContextHistory(bKeepSystemPrompt);

        //Lazy keep version, just re-insert. TODO: implement optimized reset
        /*if (bKeepSystemPrompt)
        {
            Internal->InsertTemplatedPrompt(ModelParams.SystemPrompt, EChatTemplateRole::System, false, false);
        }*/

        SyncModelStateToInternal();
    });
}

void FLlamaNative::RemoveLastUserInput()
{
    //lazily removes last reply and last input
    RemoveLastNMessages(2);
}

void FLlamaNative::RemoveLastReply()
{
    RemoveLastNMessages(1);
}

void FLlamaNative::RegenerateLastReply()
{
    RemoveLastReply();
    //Change seed?
    ResumeGeneration();
}

int32 FLlamaNative::RawContextHistory(FString& OutContextString)
{
    if (IsGenerating())
    {
        //Todo: handle this case gracefully
        UE_LOG(LlamaLog, Warning, TEXT("RawContextString cannot be called yet during generation."));
        return -1;
    }

    if (Internal->ContextHistory.size() == 0)
    {
        return 0;
    }

    // Find the first null terminator (0) in the buffer
    int32 ValidLength = Internal->ContextHistory.size();
    for (int32 i = 0; i < Internal->ContextHistory.size(); i++)
    {
        if (Internal->ContextHistory[i] == '\0')
        {
            ValidLength = i;
            break;
        }
    }

    // Convert only the valid part to an FString
    OutContextString = FString(ValidLength, ANSI_TO_TCHAR(Internal->ContextHistory.data()));

    return ValidLength;
}

void FLlamaNative::GetStructuredChatHistory(FStructuredChatHistory& OutChatHistory)
{
    if (IsGenerating())
    {
        //Todo: handle this case gracefully
        UE_LOG(LlamaLog, Warning, TEXT("GetStructuredChatHistory cannot be called yet during generation."));
        return;
    }

    OutChatHistory.History.Empty();

    for (const llama_chat_message& Msg : Internal->Messages)
    {
        FStructuredChatMessage StructuredMsg;

        // Convert role
        FString RoleStr = FString(ANSI_TO_TCHAR(Msg.role));
        if (RoleStr.Equals(TEXT("system"), ESearchCase::IgnoreCase))
        {
            StructuredMsg.Role = EChatTemplateRole::System;
        }
        else if (RoleStr.Equals(TEXT("user"), ESearchCase::IgnoreCase))
        {
            StructuredMsg.Role = EChatTemplateRole::User;
        }
        else if (RoleStr.Equals(TEXT("assistant"), ESearchCase::IgnoreCase))
        {
            StructuredMsg.Role = EChatTemplateRole::Assistant;
        }
        else
        {
            // Default/fallback role (adjust if needed)
            StructuredMsg.Role = EChatTemplateRole::Assistant;
        }

        // Convert content
        StructuredMsg.Content = FString(ANSI_TO_TCHAR(Msg.content));

        // Add to history
        OutChatHistory.History.Add(StructuredMsg);
    }
}

void FLlamaNative::SyncPassedModelStateToNative(FLLMModelState& StateToSync)
{
    StateToSync = ModelState;
}

int32 FLlamaNative::UsedContextLength()
{
    return Internal->UsedContext();
}

void FLlamaNative::InjectTokensIntoPrompt()
{
}

FString FLlamaNative::WrapPromptForRole(const FString& Text, EChatTemplateRole Role, const FString& OverrideTemplate, bool bAddAssistantBoS)
{
    return FLlamaString::ToUE( Internal->WrapPromptForRole(FLlamaString::ToStd(Text), Role, FLlamaString::ToStd(OverrideTemplate), bAddAssistantBoS) );
}
