# Light, Camera, Action! - 구현 계획서

## 프로젝트 개요
Game Jam #3 결과물에 시네마틱 연출을 추가하여 게임의 몰입감과 타격감을 향상시키는 프로젝트

---

## 필수 요구사항
- [ ] 시네마틱 연출 추가
- [ ] 팀 번호와 구성원 이름 게임 내 표시
- [ ] 각종 카메라 효과 구현 (Camera Shake, Letter Box, Fade In/Out, Spring Arm)
- [ ] 언리얼 Camera System 이해 및 구현
- [ ] Hit Stop, Slomo 구현으로 타격감 개선
- [ ] Release_StandAlone 빌드로 제출

---

## 구현 우선순위 및 단계

### Phase 1: 핵심 Camera System 구조 설계 (우선순위: 최상)
모든 카메라 효과의 기반이 되는 시스템

#### 1.1 APlayerCameraManager 구현
**위치**: `Runtime/Engine/GameFramework/PlayerCameraManager.h/.cpp`

**구현 내용**:
```cpp
class APlayerCameraManager : public AActor
{
    DECLARE_CLASS(APlayerCameraManager, AActor)

private:
    // Fade 관련
    FLinearColor FadeColor;
    float FadeAmount;
    FVector2D FadeAlpha;
    float FadeTime;
    float FadeTimeRemaining;

    // Camera 상태
    FName CameraStyle;
    FViewTarget ViewTarget;

    // Modifier 관리
    TArray<UCameraModifier*> ModifierList;

    // Time Dilation (Slomo, Hit Stop)
    float GlobalTimeDilation;
    float CustomTimeDilation;

public:
    // Camera 전환
    void SetViewTarget(AActor* NewViewTarget, float BlendTime = 0.0f);
    void BlendCamera(AActor* NewTarget, float BlendTime);

    // Fade 효과
    void StartCameraFade(float FromAlpha, float ToAlpha, float Duration,
                         FLinearColor Color = FLinearColor::Black,
                         bool bHoldWhenFinished = false);
    void StopCameraFade();

    // Modifier 관리
    UCameraModifier* AddCameraModifier(TSubclassOf<UCameraModifier> ModifierClass);
    bool RemoveCameraModifier(UCameraModifier* Modifier);
    void ClearAllCameraModifiers();

    // Time Control
    void SetGlobalTimeDilation(float NewTimeDilation);
    float GetGlobalTimeDilation() const { return GlobalTimeDilation; }

    // Update
    virtual void Tick(float DeltaSeconds) override;
    void UpdateCamera(float DeltaTime);
    void ApplyModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV);
};
```

**핵심 기능**:
- ViewTarget 관리 및 카메라 전환
- Fade In/Out 처리
- Camera Modifier 시스템 관리
- Global Time Dilation 관리

---

#### 1.2 UCameraModifier 기본 클래스 구현
**위치**: `Runtime/Engine/GameFramework/CameraModifier.h/.cpp`

**구현 내용**:
```cpp
class UCameraModifier : public UObject
{
    DECLARE_CLASS(UCameraModifier, UObject)

protected:
    APlayerCameraManager* CameraOwner;

    // Alpha Blending
    float AlphaInTime;      // Fade in 시간
    float AlphaOutTime;     // Fade out 시간
    float Alpha;            // 현재 강도 (0.0 ~ 1.0)

    // State
    uint32 bDisabled : 1;
    uint8 Priority;         // 우선순위 (높을수록 나중에 적용)

public:
    // Lifecycle
    virtual void Init(APlayerCameraManager* InCameraOwner);
    virtual void Destroy();

    // Main update function
    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV);

    // Alpha control
    virtual void EnableModifier();
    virtual void DisableModifier(bool bImmediate = false);
    bool IsDisabled() const { return bDisabled; }

    // Alpha blending
    virtual void UpdateAlpha(float DeltaTime);
    float GetAlpha() const { return Alpha; }
};
```

**핵심 개념**:
- 각 효과는 UCameraModifier를 상속받아 구현
- Alpha 값으로 효과의 강도 조절
- Priority로 효과 적용 순서 결정

---

### Phase 2: 카메라 효과 구현 (우선순위: 상)

#### 2.1 Camera Shake
**위치**: `Runtime/Engine/GameFramework/CameraShakeModifier.h/.cpp`

**구현 방식**:
```cpp
class UCameraShakeModifier : public UCameraModifier
{
    // Shake 패턴
    FVector LocationAmplitude;      // 위치 흔들림 강도
    FRotator RotationAmplitude;     // 회전 흔들림 강도

    float Frequency;                // 주파수
    float Duration;                 // 지속 시간
    float TimeElapsed;              // 경과 시간

    // Shake 곡선
    ECameraShakePattern Pattern;    // Sine, Perlin, Random 등

    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
    FVector CalculateLocationOffset(float Time);
    FRotator CalculateRotationOffset(float Time);
};
```

**활용 시나리오**:
- 총 발사 시 반동
- 폭발 효과
- 착지 충격
- 차량 진동

**구현 세부사항**:
- Perlin Noise를 사용한 자연스러운 흔들림
- Sine Wave를 사용한 리듬감 있는 흔들림
- Duration에 따른 자동 감쇠
- 여러 Shake 동시 적용 가능 (가산)

---

#### 2.2 Letter Box (Cinematic Bars)
**위치**: `Runtime/Engine/GameFramework/LetterBoxModifier.h/.cpp`

**구현 방식**:
```cpp
class ULetterBoxModifier : public UCameraModifier
{
    float TargetBarSize;        // 목표 바 크기 (0.0 ~ 0.5)
    float CurrentBarSize;       // 현재 바 크기
    float TransitionSpeed;      // 전환 속도

    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

    // PostProcess에 바 렌더링 정보 전달
    void UpdateLetterBoxRendering();
};
```

**렌더링 연동**:
- Renderer에 Letter Box 정보 전달
- PostProcess 단계에서 상하단에 검은 바 렌더링
- FOV 조정으로 시네마틱 느낌 강조 가능

**활용 시나리오**:
- 컷신 시작/종료
- 중요한 이벤트 연출
- 보스 등장 연출

---

#### 2.3 Fade In/Out
**위치**: `PlayerCameraManager` 내부 기능

**구현 내용**:
```cpp
// PlayerCameraManager.cpp
void APlayerCameraManager::StartCameraFade(float FromAlpha, float ToAlpha,
                                           float Duration, FLinearColor Color,
                                           bool bHoldWhenFinished)
{
    FadeAlpha.X = FromAlpha;
    FadeAlpha.Y = ToAlpha;
    FadeTime = Duration;
    FadeTimeRemaining = Duration;
    FadeColor = Color;
    bHoldFadeWhenFinished = bHoldWhenFinished;
}

void APlayerCameraManager::UpdateFade(float DeltaTime)
{
    if (FadeTimeRemaining > 0.0f)
    {
        FadeTimeRemaining -= DeltaTime;
        float Progress = 1.0f - (FadeTimeRemaining / FadeTime);
        FadeAmount = FMath::Lerp(FadeAlpha.X, FadeAlpha.Y, Progress);
    }

    // Renderer에 Fade 정보 전달
    UpdateFadeRendering();
}
```

**활용 시나리오**:
- 씬 전환
- 사망 효과
- 게임 시작/종료
- 로딩 화면 전환

---

#### 2.4 Spring Arm (Camera Boom)
**위치**: `Runtime/Engine/Components/SpringArmComponent.h/.cpp`

**구현 내용**:
```cpp
class USpringArmComponent : public USceneComponent
{
    DECLARE_CLASS(USpringArmComponent, USceneComponent)

private:
    float TargetArmLength;          // 목표 거리
    float CurrentArmLength;         // 현재 거리

    FRotator RelativeRotation;      // 상대 회전
    FVector SocketOffset;           // 소켓 오프셋

    // Lag (부드러운 따라가기)
    bool bEnableCameraLag;
    float CameraLagSpeed;
    FVector PreviousDesiredLocation;

    // Collision
    bool bDoCollisionTest;
    float ProbeSize;
    TArray<AActor*> IgnoreActors;

public:
    virtual void Tick(float DeltaSeconds) override;

    void SetTargetArmLength(float NewLength);
    void UpdateDesiredArmLocation(float DeltaTime);
    bool DoCollisionTest(FVector& OutLocation);
};
```

**핵심 기능**:
- 카메라가 캐릭터를 부드럽게 따라감
- 장애물 감지 시 카메라 자동 당김
- 회전 시 관성 효과

**활용 시나리오**:
- 3인칭 카메라
- 추격 카메라
- 동적 시점 변경

---

#### 2.5 Camera Transition (블렌딩)
**위치**: `PlayerCameraManager` 내부 기능

**구현 방식**:
```cpp
struct FViewTarget
{
    AActor* Target;
    FMinimalViewInfo POV;

    FVector Location;
    FRotator Rotation;
    float FOV;
};

// Blend 타입
enum class ECameraBlendType
{
    Linear,         // 선형 보간
    Cubic,          // 3차 곡선
    EaseInOut,      // EaseInOut 곡선
    Custom          // 커스텀 커브
};

void APlayerCameraManager::UpdateCameraBlend(float DeltaTime)
{
    if (BlendTimeRemaining > 0.0f)
    {
        BlendTimeRemaining -= DeltaTime;
        float Progress = 1.0f - (BlendTimeRemaining / BlendTime);

        // Blend 곡선 적용
        float BlendWeight = ApplyBlendCurve(Progress, BlendType);

        // View 보간
        CurrentView.Location = FMath::Lerp(BlendSource.Location,
                                           BlendTarget.Location,
                                           BlendWeight);
        CurrentView.Rotation = FMath::Lerp(BlendSource.Rotation,
                                           BlendTarget.Rotation,
                                           BlendWeight);
        CurrentView.FOV = FMath::Lerp(BlendSource.FOV,
                                      BlendTarget.FOV,
                                      BlendWeight);
    }
}
```

**활용 시나리오**:
- 컷신 전환
- 카메라 시점 변경
- 보스전 시작 연출

---

### Phase 3: 타격감 시스템 구현 (우선순위: 상)

#### 3.1 Global Time Dilation (Slomo)
**위치**: `Runtime/Engine/GameFramework/World.h/.cpp` 확장

**구현 내용**:
```cpp
class UWorld
{
private:
    float GlobalTimeDilation;       // 전역 시간 배율

public:
    void SetGlobalTimeDilation(float NewTimeDilation);
    float GetGlobalTimeDilation() const { return GlobalTimeDilation; }

    // Tick에서 사용
    virtual void Tick(float DeltaSeconds) override
    {
        float AdjustedDeltaTime = DeltaSeconds * GlobalTimeDilation;
        // ... 기존 틱 로직
    }
};
```

**PlayerCameraManager에서 제어**:
```cpp
// 일시적 슬로우 모션
void APlayerCameraManager::PlaySlowMotion(float TimeDilation, float Duration)
{
    GetWorld()->SetGlobalTimeDilation(TimeDilation);

    // Duration 후 원래대로 복구
    GetWorld()->GetTimerManager().SetTimer(
        SlowMotionTimerHandle,
        [this]() { GetWorld()->SetGlobalTimeDilation(1.0f); },
        Duration,
        false
    );
}
```

**활용 시나리오**:
- 결정타 연출 (0.1 ~ 0.3배속)
- 특수 스킬 발동
- 게임 오버 연출
- 극적인 순간 강조

**구현 세부사항**:
- 사운드 피치도 Time Dilation에 맞춰 조정
- UI는 정상 속도 유지 옵션
- 파티클 효과도 영향 받도록 설정

---

#### 3.2 Hit Stop (Frame Freeze)
**위치**: `Runtime/Engine/GameFramework/HitStopManager.h/.cpp`

**구현 내용**:
```cpp
class UHitStopManager : public UObject
{
private:
    float HitStopDuration;
    float HitStopTimeRemaining;
    bool bIsInHitStop;

    // Hit Stop 중 제외할 액터들
    TArray<AActor*> ExemptActors;

public:
    // Hit Stop 시작
    void StartHitStop(float Duration, AActor* Instigator = nullptr);

    // Tick에서 호출
    void UpdateHitStop(float DeltaTime);

    // 특정 액터를 Hit Stop에서 제외
    void AddExemptActor(AActor* Actor);
    void RemoveExemptActor(AActor* Actor);

    bool IsInHitStop() const { return bIsInHitStop; }
};
```

**동작 원리**:
```cpp
void UHitStopManager::UpdateHitStop(float DeltaTime)
{
    if (bIsInHitStop)
    {
        HitStopTimeRemaining -= DeltaTime;

        if (HitStopTimeRemaining <= 0.0f)
        {
            bIsInHitStop = false;
            // 모든 액터의 틱 재개
            ResumeAllActors();
        }
        else
        {
            // Hit Stop 중에는 대부분의 액터 틱 정지
            // (ExemptActor 제외)
            PauseActorsTick();
        }
    }
}

// 효과적인 구현: Time Dilation을 극단적으로 낮춤
void StartHitStop(float Duration)
{
    SavedTimeDilation = GetWorld()->GetGlobalTimeDilation();
    GetWorld()->SetGlobalTimeDilation(0.0f);  // 완전 정지

    GetWorld()->GetTimerManager().SetTimer(
        HitStopHandle,
        [this]() {
            GetWorld()->SetGlobalTimeDilation(SavedTimeDilation);
        },
        Duration,
        false,
        -1.0f  // Real time 사용
    );
}
```

**활용 시나리오**:
- 강력한 공격 적중 시 (0.1 ~ 0.2초)
- 크리티컬 히트
- 방어 성공/패링
- 필살기 히트

**Camera Shake와 조합**:
```cpp
void OnPowerfulHit()
{
    // Hit Stop
    HitStopManager->StartHitStop(0.15f);

    // Camera Shake (Hit Stop 후 실행)
    GetWorld()->GetTimerManager().SetTimer(
        ShakeHandle,
        [this]() {
            CameraManager->PlayCameraShake(HeavyHitShake);
        },
        0.15f,
        false,
        -1.0f
    );
}
```

---

### Phase 4: 시네마틱 시스템 통합 (우선순위: 중)

#### 4.1 Cinematic Sequence Manager
**위치**: `Runtime/Engine/Cinematic/CinematicSequenceManager.h/.cpp`

**구현 내용**:
```cpp
class UCinematicSequenceManager : public UObject
{
private:
    struct FCinematicEvent
    {
        float TriggerTime;
        TFunction<void()> EventFunction;
        bool bTriggered;
    };

    TArray<FCinematicEvent> Events;
    float SequenceTime;
    bool bIsPlaying;

public:
    // 시퀀스 시작
    void PlayCinematic();

    // 이벤트 등록
    void AddEvent(float Time, TFunction<void()> Event);

    // Update
    void UpdateSequence(float DeltaTime);

    // 시네마틱 종료
    void StopCinematic();
};
```

**사용 예시**:
```cpp
void SetupOpeningCinematic()
{
    auto Sequence = NewObject<UCinematicSequenceManager>();

    // 0초: Fade In
    Sequence->AddEvent(0.0f, [this]() {
        CameraManager->StartCameraFade(1.0f, 0.0f, 2.0f);
    });

    // 1초: Letter Box 시작
    Sequence->AddEvent(1.0f, [this]() {
        CameraManager->EnableLetterBox(0.1f, 1.5f);
    });

    // 2초: 카메라 전환
    Sequence->AddEvent(2.0f, [this]() {
        CameraManager->SetViewTarget(CinematicCamera1, 2.0f);
    });

    // 5초: 크레딧 표시
    Sequence->AddEvent(5.0f, [this]() {
        ShowTeamCredits();
    });

    // 10초: Letter Box 종료 및 게임 시작
    Sequence->AddEvent(10.0f, [this]() {
        CameraManager->DisableLetterBox(1.0f);
        StartGameplay();
    });

    Sequence->PlayCinematic();
}
```

---

#### 4.2 팀 정보 표시 시스템
**위치**: UI 시스템 활용

**구현 내용**:
```cpp
// 게임 시작 시 팀 정보 표시
void ShowTeamCredits()
{
    // UI에 팀 번호와 구성원 표시
    ShowCreditText("Team #X");
    ShowCreditText("Members:");
    ShowCreditText("- Member 1");
    ShowCreditText("- Member 2");
    ShowCreditText("- Member 3");

    // Fade In/Out 효과
    FadeInCredits(1.0f);

    // 5초 후 Fade Out
    GetWorld()->GetTimerManager().SetTimer(
        CreditTimerHandle,
        [this]() { FadeOutCredits(1.0f); },
        5.0f,
        false
    );
}
```

**표시 시점**:
- 게임 시작 시 (오프닝 시네마틱)
- 게임 오버 시 (엔딩 크레딧)
- 메뉴 화면

---

### Phase 5: 선택 사항 (우선순위: 하)

#### 5.1 Gamma Correction
**위치**: `Runtime/Renderer/PostProcess/`

**구현 개념**:
- 렌더링 파이프라인에서 색상 보정
- Show Flag로 On/Off 가능
- 시네마틱 느낌 강화

---

#### 5.2 Vignetting (비네팅)
**위치**: PostProcess Modifier

**구현 방식**:
- 화면 가장자리 어둡게 처리
- 중앙 집중 효과
- 긴장감 있는 장면에 효과적

```cpp
class UVignettingModifier : public UCameraModifier
{
    float Intensity;        // 강도 (0.0 ~ 1.0)
    float Radius;           // 중심 반경

    virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
};
```

---

## 구현 순서 (추천)

### Week 1: 핵심 시스템 구축
1. **Day 1-2**: PlayerCameraManager 기본 구조 구현
   - 클래스 생성 및 기본 틱 설정
   - ViewTarget 관리 시스템

2. **Day 3-4**: CameraModifier 시스템 구현
   - 베이스 클래스 완성
   - Modifier 추가/제거/우선순위 시스템

3. **Day 5**: Time Dilation 시스템 구현
   - World에 GlobalTimeDilation 추가
   - 기본 Slomo 기능 구현

### Week 2: 카메라 효과 구현
1. **Day 1**: Camera Shake 구현
   - 기본 Shake Modifier
   - Perlin/Sine 패턴

2. **Day 2**: Fade In/Out 구현
   - PlayerCameraManager에 Fade 시스템 추가
   - 렌더러 연동

3. **Day 3**: Letter Box 구현
   - LetterBox Modifier
   - 렌더러 연동

4. **Day 4**: Spring Arm 구현
   - 기본 컴포넌트 구현
   - Collision 테스트

5. **Day 5**: Camera Transition 구현
   - 블렌드 시스템
   - 다양한 블렌드 커브

### Week 3: 타격감 및 통합
1. **Day 1-2**: Hit Stop 구현
   - HitStopManager 구현
   - Actor Tick 제어

2. **Day 3**: Slomo 고도화
   - 사운드 연동
   - 효과 조합 테스트

3. **Day 4**: 시네마틱 시퀀스 구현
   - CinematicSequenceManager
   - 이벤트 시스템

4. **Day 5**: 팀 정보 표시 및 최종 통합
   - 크레딧 시스템
   - 전체 효과 조합 테스트

### Week 4: 폴리싱 및 빌드
1. **Day 1-3**: 게임에 적용 및 밸런싱
   - 각 효과 파라미터 튜닝
   - 시네마틱 연출 제작

2. **Day 4**: 최종 테스트
   - 버그 수정
   - 성능 최적화

3. **Day 5**: 빌드 및 제출
   - Release_StandAlone 빌드
   - 최종 검수

---

## 기술적 고려사항

### 1. 성능 최적화
- Camera Shake는 프레임당 계산이므로 최적화 필수
- Modifier는 Priority 순으로 정렬하여 캐싱
- Hit Stop은 필요한 액터만 제어

### 2. 멀티플레이 고려 (해당시)
- Time Dilation은 서버에서 제어
- 카메라 효과는 클라이언트 로컬
- Hit Stop은 각 클라이언트 독립 실행

### 3. 디버그 도구
- 각 효과별 On/Off 토글
- 파라미터 실시간 조정 UI
- Show Flag를 통한 시각적 디버깅

### 4. 사운드 연동
```cpp
// Time Dilation 시 사운드 피치 조정
void UpdateSoundPitch()
{
    float TimeDilation = GetWorld()->GetGlobalTimeDilation();
    AudioManager->SetGlobalPitchMultiplier(TimeDilation);
}
```

---

## 테스트 시나리오

### Camera Shake 테스트
- [ ] 총 발사 시 미세한 흔들림
- [ ] 폭발 시 강한 흔들림
- [ ] 여러 Shake 동시 적용 시 올바른 합성
- [ ] Duration 후 자동 종료

### Letter Box 테스트
- [ ] 부드러운 전환 (Fade In/Out)
- [ ] 게임플레이 중 정상 렌더링
- [ ] 컷신 모드로 전환 확인

### Fade In/Out 테스트
- [ ] 씬 전환 시 자연스러운 페이드
- [ ] 색상 변경 (검은색, 흰색 등)
- [ ] 투명도 정확도

### Hit Stop 테스트
- [ ] 타격 시 프레임 정지
- [ ] 정지 후 정상 복구
- [ ] 사운드 효과 정상 재생
- [ ] Camera Shake와 조합

### Slomo 테스트
- [ ] 시간 느려짐 효과
- [ ] 사운드 피치 변경
- [ ] UI 정상 동작
- [ ] 복구 후 정상 속도

### Spring Arm 테스트
- [ ] 카메라 부드럽게 따라가기
- [ ] 장애물 감지 시 자동 당김
- [ ] 장애물 제거 시 원위치 복귀

---

## 최종 체크리스트

### 필수 구현
- [ ] APlayerCameraManager 구현
- [ ] UCameraModifier 구현
- [ ] Camera Shake
- [ ] Letter Box
- [ ] Fade In/Out
- [ ] Spring Arm
- [ ] Camera Transition
- [ ] Hit Stop
- [ ] Slomo
- [ ] 팀 정보 표시
- [ ] 시네마틱 연출 1개 이상

### 빌드 및 제출
- [ ] Release_StandAlone 빌드 성공
- [ ] 모든 효과 정상 동작 확인
- [ ] 크래시 없음
- [ ] 팀 정보 게임 내 표시 확인

### 선택 구현 (가산점)
- [ ] Gamma Correction
- [ ] Vignetting
- [ ] 다양한 Blend Curve
- [ ] 커스텀 Camera Modifier

---

## 참고 자료

### 핵심 키워드 학습 자료
- **Transition & Interpolation**: 선형보간, Ease In/Out, Bezier Curve
- **Camera System**: PlayerCameraManager, CameraModifier, ViewTarget
- **Cinematic Effects**: Fade, Letter Box, DOF, Vignetting
- **Game Feel**: Hit Stop, Screen Shake, Slomo
- **Rendering**: Post Process, Alpha Blending, Gamma Correction

### 예제 게임 연구
- 타격감이 뛰어난 게임의 Camera Shake 분석
- 영화적 연출이 돋보이는 게임의 시네마틱 분석

---

## 리스크 관리

### 높은 리스크
1. **PlayerCameraManager 구조 복잡도**
   - 대응: 단계별 구현, 각 단계마다 테스트

2. **Time Dilation 시스템 영향 범위**
   - 대응: 영향받지 않아야 할 시스템 명확히 정의

3. **렌더링 연동 이슈**
   - 대응: 기존 렌더러 구조 파악 먼저 진행

### 중간 리스크
1. **성능 이슈**
   - 대응: 프로파일링 도구 활용, 최적화 우선순위 설정

2. **효과 조합 시 충돌**
   - 대응: Modifier Priority 시스템으로 해결

### 낮은 리스크
1. **파라미터 튜닝 시간 부족**
   - 대응: 기본값 먼저 설정, 점진적 개선

---

## 결론

이 프로젝트는 **카메라 시스템의 아키텍처 설계**가 가장 중요합니다.

핵심 시스템(PlayerCameraManager, CameraModifier)을 견고하게 구축하면,
나머지 효과들은 Modifier를 상속받아 비교적 쉽게 구현할 수 있습니다.

**우선순위**:
1. PlayerCameraManager + CameraModifier 시스템 (기반)
2. Hit Stop + Slomo (타격감)
3. Camera Shake + Fade (기본 효과)
4. Letter Box + Spring Arm (추가 효과)
5. 시네마틱 시퀀스 (통합)
6. 선택 사항 (여유 시)

이 순서대로 구현하면 **점진적으로 기능을 추가하면서 안정적으로 개발**할 수 있습니다.
