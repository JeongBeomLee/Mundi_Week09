#include "pch.h"
#include "UCameraModifier.h"

IMPLEMENT_CLASS(UCameraModifier)

void UCameraModifier::AddedToCamera( APlayerCameraManager* Camera ) 
{
    CameraOwner = Camera;
}

void UCameraModifier::ModifyCamera(
    float DeltaTime,
    FVector ViewLocation,
    FQuat ViewRotation,
    float FOV,
    FVector& NewViewLocation,
    FQuat& NewViewRotation,
    float& NewFOV
)
{
    UpdateAlpha(DeltaTime);

    if (CameraOwner)
    {
        // note: pushing these through the cached PP blend system in the camera to get
        // proper layered blending, rather than letting subsequent mods stomp over each other in the 
        // InOutPOV struct.
        {
            float PPBlendWeight = 0.f;
            //FPostProcessSettings PPSettings;
			
            //  Let native code modify the post process settings.
            ModifyPostProcess(DeltaTime, PPBlendWeight/*, PPSettings*/);
			
            if (PPBlendWeight > 0.f)
            {
                // CameraOwner->AddCachedPPBlend(PPSettings, PPBlendWeight);
            }
        }
    }
}

void UCameraModifier::ModifyPostProcess(
    float DeltaTime,
    float& PostProcessBlendWeight//,
    //FPostProcessSettings& PostProcessSettings
) {}

float UCameraModifier::GetTargetAlpha()
{
    return bIsFadingIn ? 0.f : 1.f;
}

bool UCameraModifier::IsDisabled() const
{
    return bDisabled;
}

void UCameraModifier::EnableModifier()
{
    bDisabled = false;
}

void UCameraModifier::DisableModifier()
{
    bDisabled = true;
}

void UCameraModifier::UpdateAlpha(float DeltaTime)
{
    float const TargetAlpha = GetTargetAlpha();
    float const BlendTime = (TargetAlpha == 0.f) ? AlphaOutTime : AlphaInTime;
    
    // interpolate!
    if (BlendTime <= 0.f)
    {
        // no blendtime means no blending, just go directly to target alpha
        Alpha = TargetAlpha;
    }
    else if (Alpha > TargetAlpha)
    {
        // interpolate downward to target, while protecting against overshooting
        Alpha = std::max<float>(Alpha - DeltaTime / BlendTime, TargetAlpha);
    }
    else
    {
        // interpolate upward to target, while protecting against overshooting
        Alpha = std::min<float>(Alpha + DeltaTime / BlendTime, TargetAlpha);
    }
}

float UCameraModifier::GetAlphaInTime() const
{
    return AlphaInTime;
}
void UCameraModifier::SetAlphaInTime(const float InAlphaInTime)
{
    AlphaInTime = InAlphaInTime;
}

float UCameraModifier::GetAlphaOutTime() const
{
    return AlphaOutTime;
}
void UCameraModifier::SetAlphaOutTime(const float InAlphaOutTime)
{
    AlphaOutTime = InAlphaOutTime;
}

float UCameraModifier::GetAlpha() const
{
    return Alpha;
}

bool UCameraModifier::IsFadingIn() const
{
    return bIsFadingIn;
}
void UCameraModifier::SetIsFadingIn(const bool InIsFadingIn)
{
    bIsFadingIn = InIsFadingIn;
}