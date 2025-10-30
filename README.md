# Mundi 엔진 - Week08 Shadow Mapping System

## 프로젝트 정보
- **Week:** 08
- **저자:** 박영빈, 서명교, 이정범
- **주제:** 고급 Shadow Mapping 시스템 구현

---

## 📋 Week08 주요 구현 내용

### 1. Shadow Mapping 시스템 아키텍처
- **ShadowManager** (`ShadowManager.h/cpp`): 전체 섀도우 시스템 관리
- **ShadowMap** (`ShadowMap.h/cpp`): 개별 섀도우맵 리소스 관리
- **ShadowConfiguration** (`ShadowConfiguration.h/cpp`): 섀도우 설정 구조체
- **ShadowStats** (`ShadowStats.h`): 섀도우 메모리 사용량 통계
- **ShadowViewProjection** (`ShadowViewProjection.h`): View-Projection 행렬 계산

### 2. Shadow Mapping 기법 구현

#### 2.1 Cascaded Shadow Mapping (CSM)
- Directional Light용 CSM 구현
- 캐스케이드별 프러스텀 분할 및 최적화
- UI를 통한 CSM 설정 조정 기능

#### 2.2 Light-Space Perspective Shadow Mapping (LiSPSM)
- OpenGL LiSPSM 알고리즘 구현
- Weighted AABB orthogonal 투영
- Perspective aliasing 감소

#### 2.3 Soft Shadow 기법
- **PCF (Percentage Closer Filtering)**: 하드 섀도우 경계 완화
- **VSM (Variance Shadow Mapping)**: 분산 기반 소프트 섀도우 (`ShadowVSM_PS.hlsl`)
- **ESM (Exponential Shadow Mapping)**: 지수 함수 기반 필터링 (`ShadowESM_PS.hlsl`)
- **EVSM (Exponential Variance Shadow Mapping)**: VSM + ESM 결합 (`ShadowEVSM_PS.hlsl`)
- Gaussian 가중치 샘플링 적용

### 3. 라이트별 Shadow 구현

#### Point Light Shadow
- 큐브맵 기반 6면 섀도우 렌더링
- `ShadowDepthCube.hlsl` 셰이더 구현
- 6개의 View-Projection 행렬 상수 버퍼 전달
- Paraboloid 방식 제거 후 큐브맵으로 통일

#### Directional Light Shadow
- `ShadowDepth.hlsl` 셰이더 구현
- CSM/LiSPSM 적용
- Normal과 빛 방향 기반 동적 bias 적용

#### Spot Light Shadow
- 기본 섀도우맵 구현
- 프러스텀 기반 투영 최적화

### 4. 에디터 기능 추가

#### 4.1 Shadow Map Viewer
- SRV Editor를 통한 섀도우맵 시각화
- Viewport에 Shadow Map 표시 (`Viewport_ShadowMap.png` 아이콘)
- 0~1 Depth View 수정

#### 4.2 런타임 설정 조정
- 섀도우맵 해상도 런타임 변경
- 라이트별 Shadow 활성화/비활성화
- Shadow Sharpen 값으로 VSM 파라미터 동적 계산
- DragFloat UI 위젯 추가

#### 4.3 통계 및 디버깅
- `stat shadow` 명령어: 섀도우맵 메모리 사용량 시각화
- StatsOverlayD2D에 섀도우 통계 표시
- 디버그 출력을 콘솔 대신 화면에 표시

### 5. 기타 개선사항

#### 5.1 클립보드 시스템
- `ClipboardManager` 구현
- Actor/Component 복사/붙여넣기 기능
- Gizmo 컴포넌트도 복제 가능

#### 5.2 카메라 시스템
- 카메라 오버라이딩 기능 추가
- 프러스텀 z depth 계산 수정

#### 5.3 버그 수정
- 메모리 누수 해결
- 리소스 언바인딩 문제 해결 (Point Light SRV 슬롯 최적화)
- Decal z-fighting 문제 해결
- VSM/ESM/EVSM depth bias 제거

#### 5.4 리팩토링
- SceneRenderer 대규모 리팩토링
- UWorld 통합
- FLightManager에서 Shadow 책임 분리
- FShadowMap 캡슐화 개선
- 중복 코드 제거

---

## 🎮 사용 방법

### Shadow 설정
1. Light Component 선택 (Directional/Point/Spot)
2. Properties에서 `bIsCastShadows` 활성화
3. Shadow 파라미터 조정:
   - Resolution: 섀도우맵 해상도
   - Bias: 섀도우 아크네 방지
   - Sharpen: VSM 선명도 (VSM 사용 시)

### Shadow Map 확인
- Viewport 상단의 Shadow Map 아이콘 클릭
- SRV Editor에서 각 라이트의 섀도우맵 확인

### 통계 확인
- 콘솔에서 `stat shadow` 입력
- 메모리 사용량 및 섀도우맵 정보 확인

---

## 📘 Mundi 엔진 렌더링 기준

> 🚫 **경고: 이 내용은 Mundi 엔진 렌더링 기준의 근본입니다.**
> 삭제하거나 수정하면 엔진 전반의 좌표계 및 버텍스 연산이 깨집니다.
> **반드시 유지하십시오.**

### 기본 좌표계

* **좌표계:** Z-Up, **왼손 좌표계 (Left-Handed)**
* **버텍스 시계 방향 (CW)** 이 **앞면(Face Front)** 으로 간주됩니다.
  > → **DirectX의 기본 설정**을 그대로 따릅니다.

### OBJ 파일 Import 규칙

* OBJ 포맷은 **오른손 좌표계 + CCW(반시계)** 버텍스 순서를 사용한다고 가정합니다.
  > → 블렌더에서 OBJ 포맷으로 Export 시 기본적으로 이렇게 저장되기 때문입니다.
* 따라서 OBJ를 로드할 때, 엔진 내부 좌표계와 일치하도록 자동 변환을 수행합니다.

```cpp
FObjImporter::LoadObjModel(... , bIsRightHanded = true) // 기본값
```

즉, OBJ를 **Right-Handed → Left-Handed**,
**CCW → CW** 방향으로 변환하여 엔진의 렌더링 방식과 동일하게 맞춥니다.

### 블렌더(Blender) Export 설정

* 블렌더에서 모델을 **Z-Up, X-Forward** 설정으로 Export하여
  Mundi 엔진에 Import 시 **동일한 방향을 바라보게** 됩니다.

> 💡 참고:
> 블렌더에서 축 설정을 변경해도 **좌표계나 버텍스 순서 자체는 변하지 않습니다.**
> 단지 **기본 회전 방향만 바뀌므로**, Mundi 엔진에서는 항상 같은 방식으로 Import하면 됩니다.

### 좌표계 정리

| 구분     | Mundi 엔진 내부 표현      | Mundi 엔진이 해석하는 OBJ   | OBJ Import 결과 |
| ------ | ----------------- | ------------------ | ----------------- |
| 좌표계    | Z-Up, Left-Handed | Z-Up, Right-Handed | Z-Up, Left-Handed |
| 버텍스 순서 | CW (시계 방향)        | CCW (반시계 방향)       | CW |
