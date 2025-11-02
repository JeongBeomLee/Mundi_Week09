// ────────────────────────────────────────────────────────────────────────────
// InputComponent.cpp
// 입력 바인딩 관리 컴포넌트 구현
// ────────────────────────────────────────────────────────────────────────────
#include "pch.h"
#include "InputComponent.h"
#include "InputManager.h"

IMPLEMENT_CLASS(UInputComponent)

// ────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ────────────────────────────────────────────────────────────────────────────

UInputComponent::UInputComponent()
{
	bCanEverTick = false; // InputComponent는 Tick을 사용하지 않음
}

UInputComponent::~UInputComponent()
{
	ClearBindings();
}

// ────────────────────────────────────────────────────────────────────────────
// 액션 바인딩
// ────────────────────────────────────────────────────────────────────────────

void UInputComponent::BindAction(const FString& ActionName, int32 KeyCode,
	std::function<void()> PressedCallback,
	std::function<void()> ReleasedCallback)
{
	FInputActionBinding Binding(ActionName, KeyCode);
	Binding.PressedCallback = PressedCallback;
	Binding.ReleasedCallback = ReleasedCallback;

	ActionBindings.Add(Binding);
}

// ────────────────────────────────────────────────────────────────────────────
// 축 바인딩
// ────────────────────────────────────────────────────────────────────────────

void UInputComponent::BindAxis(const FString& AxisName, int32 KeyCode, float Scale, std::function<void(float)> Callback)
{
	FInputAxisBinding Binding(AxisName, KeyCode, Scale);
	Binding.Callback = Callback;

	AxisBindings.Add(Binding);
}

// ────────────────────────────────────────────────────────────────────────────
// 입력 처리
// ────────────────────────────────────────────────────────────────────────────

void UInputComponent::ProcessInput()
{
	UInputManager& InputManager = UInputManager::GetInstance();

	// 액션 바인딩 처리
	for (FInputActionBinding& Binding : ActionBindings)
	{
		// 키가 눌렸을 때
		if (InputManager.IsKeyPressed(Binding.KeyCode))
		{
			if (Binding.PressedCallback)
			{
				Binding.PressedCallback();
			}
		}

		// 키가 떼어졌을 때
		if (InputManager.IsKeyReleased(Binding.KeyCode))
		{
			if (Binding.ReleasedCallback)
			{
				Binding.ReleasedCallback();
			}
		}
	}

	// 축 바인딩 처리
	for (FInputAxisBinding& Binding : AxisBindings)
	{
		if (Binding.Callback)
		{
			// 키가 눌려있으면 스케일 값 전달, 아니면 0.0 전달
			if (InputManager.IsKeyDown(Binding.KeyCode))
			{
				Binding.Callback(Binding.Scale);
			}
			else
			{
				// 키를 뗐을 때 0.0 전달 (매우 중요!)
				Binding.Callback(0.0f);
			}
		}
	}
}

void UInputComponent::ClearBindings()
{
	ActionBindings.Empty();
	AxisBindings.Empty();
}
