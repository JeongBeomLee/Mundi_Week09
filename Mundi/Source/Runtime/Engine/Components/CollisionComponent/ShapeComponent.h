#pragma once
#include "PrimitiveComponent.h"

class UShader;

class UShapeComponent : public UPrimitiveComponent
{
public:
    DECLARE_CLASS(UShapeComponent, UPrimitiveComponent)

    UShapeComponent();

protected:
    ~UShapeComponent() override;

public:
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UShapeComponent)

public:
    // 공통 속성
    FColor ShapeColor = FColor::Green;
    bool bDrawOnlyIfSelected = false;
    bool bGenerateOverlapEvents = true;
    bool bBlockComponent = false;

    // Overlap 정보
    TArray<FOverlapInfo> OverlapInfos;

    // 디버그 렌더링
    virtual void DrawDebugShape() const = 0;

    // Bound 갱신
    virtual void UpdateBounds() override = 0;

    // Overlap 체크
    virtual bool IsOverlappingComponent(const UShapeComponent* Other) const;

    // Bounds 반환
    virtual FBoxSphereBounds GetScaledBounds() const = 0;


};