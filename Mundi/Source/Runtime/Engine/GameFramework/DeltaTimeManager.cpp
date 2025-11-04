#include "pch.h"
#include "DeltaTimeManager.h"

IMPLEMENT_CLASS(UDeltaTimeManager)

UDeltaTimeManager::UDeltaTimeManager()
    : GlobalTimeDilation(1.0f)
    , bTimeDilationActive(false)
    , TimeDilationTimer(0.0f)
    , OriginalTimeDilation(1.0f)
{
}

void UDeltaTimeManager::SetGlobalTimeDilation(float NewDilation)
{
    // 음수 방지 및 범위 제한
    NewDilation = FMath::Clamp(NewDilation, 0.0f, 10.0f);

    if (GlobalTimeDilation != NewDilation)
    {
        float OldDilation = GlobalTimeDilation;
        GlobalTimeDilation = NewDilation;

        UE_LOG("[DeltaTimeManager] Global Time Dilation changed: %.2f -> %.2f",
            OldDilation, NewDilation);
    }
}

void UDeltaTimeManager::ApplyHitStop(float Duration)
{
	ApplySlomoEffect(Duration, 0.0f);
}

void UDeltaTimeManager::ApplySlomoEffect(float Duration, float TimeDilation)
{
    if (Duration < 0.0f)
    {
        UE_LOG("[DeltaTimeManager] Slomo duration must be positive or zero!");
        return;
    }

    // Editor 모드에서는 TimeDilation 비활성화
    if (!GWorld->bPie)
    {
        UE_LOG("[DeltaTimeManager] TimeDilation is disabled in Editor mode");
        return;
    }

    // 현재 시간 배율 저장
    if (!bTimeDilationActive)
    {
        OriginalTimeDilation = GlobalTimeDilation;
    }

    // Slomo 적용
    GlobalTimeDilation = TimeDilation;
    TimeDilationTimer = Duration;
    bTimeDilationActive = true;

    UE_LOG("[DeltaTimeManager] Slomo effect applied: Duration=%.2fs, TimeDilation=%.2f",
        Duration, TimeDilation);
}

void UDeltaTimeManager::CancelTimeDilation()
{
    if (bTimeDilationActive)
    {
        GlobalTimeDilation = OriginalTimeDilation;
        bTimeDilationActive = false;
        TimeDilationTimer = 0.0f;

        UE_LOG("[DeltaTimeManager] Time Dilation effect cancelled. Returning to %.2f",
            GlobalTimeDilation);
    }
}

void UDeltaTimeManager::Update(float RealDeltaTime)
{
    if (!bTimeDilationActive)
    {
        return;
    }

    TimeDilationTimer -= RealDeltaTime;

    if (TimeDilationTimer <= 0.0f)
    {
        // 효과 종료 - 원래 시간 배율로 복귀
        GlobalTimeDilation = OriginalTimeDilation;
        bTimeDilationActive = false;
        UE_LOG("[DeltaTimeManager] Time Dilation effect ended. Returning to normal speed (%.2f)",
            GlobalTimeDilation);
    }
}