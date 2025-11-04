#pragma once
#include "Object.h"

/**
 * UDeltaTimeManager
 *
 * World의 시간 흐름(DeltaTime)을 관리하는 클래스입니다.
 * Time Dilation, Hit Stop, Slomo 등의 시간 왜곡 효과를 제공합니다.
 *
 * 주요 기능:
 * - Global Time Dilation (전역 시간 배율)
 * - Hit Stop (짧은 정지 효과)
 * - Slomo (슬로우 모션)
 * - Real/Scaled DeltaTime 관리
 *
 * World가 소유하며 생명주기를 관리합니다.
 */
class UDeltaTimeManager : public UObject
{
public:
    DECLARE_CLASS(UDeltaTimeManager, UObject)

    UDeltaTimeManager();
    virtual ~UDeltaTimeManager() override = default;

    // ═══════════════════════════════════════════════
    // Time Dilation (시간 왜곡)
    // ═══════════════════════════════════════════════

    /**
     * 전역 시간 배율을 설정합니다.
     * @param NewDilation - 시간 배율 (0.0 = 정지, 1.0 = 정상, 2.0 = 2배속)
     */
    void SetGlobalTimeDilation(float NewDilation);

    /**
     * 현재 전역 시간 배율을 반환합니다.
     */
    float GetGlobalTimeDilation() const { return GlobalTimeDilation; }

    /**
     * Hit Stop 효과 (짧은 시간 정지)
     * @param Duration - 정지 시간 (실제 시간 기준, 초)
     */
    void ApplyHitStop(float Duration);

    /**
     * Slomo 효과 (슬로우 모션)
     * @param Duration - 슬로우 모션 지속 시간 (실제 시간 기준, 초)
     * @param TimeDilation - 시간 배율 (예: 0.3 = 30% 속도)
     */
    void ApplySlomoEffect(float Duration, float TimeDilation = 0.3f);

    /**
     * 시간 왜곡 효과를 즉시 취소하고 정상 속도로 복귀합니다.
     */
    void CancelTimeDilation();

    // ═══════════════════════════════════════════════
    // DeltaTime 계산
    // ═══════════════════════════════════════════════

    /**
     * 조정된 DeltaTime을 반환합니다 (GlobalTimeDilation 적용됨)
     * @param RealDeltaTime - 실제 프레임 시간
     * @return 시간 배율이 적용된 DeltaTime
     */
    float GetScaledDeltaTime(float RealDeltaTime) const
    {
        return RealDeltaTime * GlobalTimeDilation;
    }

    /**
     * Time Dilation 효과 업데이트 (World::Tick에서 호출)
     * @param RealDeltaTime - 실제 프레임 시간
     */
    void Update(float RealDeltaTime);

    // ═══════════════════════════════════════════════
    // 상태 쿼리
    // ═══════════════════════════════════════════════

    /**
     * 현재 시간 왜곡 효과가 활성화되어 있는지 확인합니다.
     */
    bool IsTimeDilationActive() const { return bTimeDilationActive; }

    /**
     * 남은 시간 왜곡 효과 시간을 반환합니다.
     */
    float GetRemainingTimeDilationTime() const { return TimeDilationTimer; }

private:
    // ═══════════════════════════════════════════════
    // 멤버 변수
    // ═══════════════════════════════════════════════

    /** 전역 시간 배율 (1.0 = 정상 속도) */
    float GlobalTimeDilation = 1.0f;

    /** 시간 왜곡 효과가 활성화되어 있는지 여부 */
    bool bTimeDilationActive = false;

    /** 시간 왜곡 효과의 남은 시간 (실제 시간 기준, 초) */
    float TimeDilationTimer = 0.0f;

    /** 시간 왜곡 효과 종료 후 복귀할 시간 배율 */
    float OriginalTimeDilation = 1.0f;
};