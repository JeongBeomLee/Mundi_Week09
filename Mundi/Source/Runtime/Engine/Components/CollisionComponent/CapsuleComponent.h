#pragma once
#include "ShapeComponent.h"

class UShader;

class UBoxComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(UBoxComponent, UShapeComponent)

    UBoxComponent();

protected:
    ~UBoxComponent() override;

public:
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(UBoxComponent)
};