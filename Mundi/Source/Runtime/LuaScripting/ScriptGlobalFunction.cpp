#include "pch.h"
#include "Source/Runtime/LuaScripting/ScriptGlobalFunction.h"
#include "Source/Runtime/Engine/GameFramework/RunnerGameMode.h"

void PrintToConsole(const char* ch)
{
    UE_LOG(ch);
}

ARunnerGameMode* GetRunnerGameMode(UWorld* World) {
    if (!World) return nullptr;
    AGameModeBase* GameMode = World->GetGameMode();
    if (!GameMode) return nullptr;
    return Cast<ARunnerGameMode>(GameMode);
}