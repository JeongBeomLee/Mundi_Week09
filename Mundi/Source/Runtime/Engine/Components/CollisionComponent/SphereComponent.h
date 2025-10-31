#pragma once
#include "ShapeComponent.h"

class UShader;

class USphereComponent : public UShapeComponent
{
public:
    DECLARE_CLASS(USphereComponent, UShapeComponent)

    USphereComponent();

protected:
    ~USphereComponent() override;

public:
    void DuplicateSubObjects() override;
    DECLARE_DUPLICATE(USphereComponent)
};