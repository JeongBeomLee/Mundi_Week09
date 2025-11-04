#include "pch.h"
#include "SoundManager.h"
#include "fmod.hpp"
#include "fmod_errors.h"

IMPLEMENT_CLASS(USoundManager)

USoundManager::USoundManager()
	: FMODSystem(nullptr)
	, MasterChannelGroup(nullptr)
	, MusicChannelGroup(nullptr)
	, SFXChannelGroup(nullptr)
	, MasterVolume(1.0f)
	, MusicVolume(0.7f)
	, SFXVolume(1.0f)
	, bIsInitialized(false)
{
}

USoundManager::~USoundManager()
{
	Shutdown();
}

USoundManager& USoundManager::GetInstance()
{
	static USoundManager* Instance = nullptr;
	if (!Instance)
	{
		Instance = NewObject<USoundManager>();
	}
	return *Instance;
}

//================================================================================================
// 생명주기
//================================================================================================

void USoundManager::Initialize()
{
	if (bIsInitialized)
	{
		std::cout << "[USoundManager] 이미 초기화되었습니다." << std::endl;
		return;
	}

	// FMOD System 생성
	FMOD_RESULT Result = FMOD::System_Create(&FMODSystem);
	if (!CheckFMODResult(Result, "FMOD::System_Create"))
	{
		return;
	}

	// 시스템 버전 확인
	unsigned int Version = 0;
	Result = FMODSystem->getVersion(&Version);
	if (!CheckFMODResult(Result, "FMODSystem->getVersion"))
	{
		return;
	}

	std::cout << "[USoundManager] FMOD 버전: " << std::hex << Version << std::dec << std::endl;

	// FMOD System 초기화
	Result = FMODSystem->init(MaxChannels, FMOD_INIT_NORMAL, nullptr);
	if (!CheckFMODResult(Result, "FMODSystem->init"))
	{
		return;
	}

	// 3D 설정 (Doppler scale, Distance factor, Rolloff scale)
	Result = FMODSystem->set3DSettings(1.0f, 1.0f, 1.0f);
	CheckFMODResult(Result, "FMODSystem->set3DSettings");

	// 마스터 채널 그룹 가져오기
	Result = FMODSystem->getMasterChannelGroup(&MasterChannelGroup);
	CheckFMODResult(Result, "FMODSystem->getMasterChannelGroup");

	// Phase 5에서 Music/SFX 채널 그룹 생성 예정
	// Result = FMODSystem->createChannelGroup("Music", &MusicChannelGroup);
	// Result = FMODSystem->createChannelGroup("SFX", &SFXChannelGroup);

	bIsInitialized = true;
	std::cout << "[USoundManager] FMOD 사운드 시스템 초기화 완료 (최대 채널 수: " << MaxChannels << ")" << std::endl;
}

void USoundManager::Update(float DeltaTime)
{
	if (!bIsInitialized || !FMODSystem)
	{
		return;
	}

	// FMOD 시스템 업데이트 (매우 중요! 매 프레임 호출 필수)
	FMOD_RESULT Result = FMODSystem->update();
	if (Result != FMOD_OK)
	{
		// 에러가 발생해도 계속 진행 (다음 프레임에서 복구 시도)
		CheckFMODResult(Result, "FMODSystem->update");
	}
}

void USoundManager::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	std::cout << "[USoundManager] FMOD 사운드 시스템 종료 중..." << std::endl;

	// 모든 캐시된 사운드 해제
	for (auto& Pair : LoadedSounds)
	{
		if (Pair.second)
		{
			Pair.second->release();
		}
	}
	LoadedSounds.clear();

	// FMOD System 종료 및 해제
	if (FMODSystem)
	{
		FMODSystem->close();
		FMODSystem->release();
		FMODSystem = nullptr;
	}

	MasterChannelGroup = nullptr;
	MusicChannelGroup = nullptr;
	SFXChannelGroup = nullptr;

	bIsInitialized = false;
	std::cout << "[USoundManager] FMOD 사운드 시스템 종료 완료" << std::endl;
}

FMOD::Channel* USoundManager::PlaySound2D(const FString& SoundPath, bool bLoop)
{
	if (!bIsInitialized || !FMODSystem)
	{
		std::cout << "[USoundManager] 사운드 시스템이 초기화되지 않았습니다." << std::endl;
		return nullptr;
	}

	// 사운드 로드 (또는 캐시에서 가져오기)
	FMOD::Sound* Sound = GetOrLoadSound(SoundPath, false, bLoop);
	if (!Sound)
	{
		return nullptr;
	}

	// 사운드 재생
	FMOD::Channel* Channel = nullptr;
	FMOD_RESULT Result = FMODSystem->playSound(Sound, nullptr, false, &Channel);
	if (!CheckFMODResult(Result, "FMODSystem->playSound"))
	{
		return nullptr;
	}

	// 2D 사운드는 3D 처리 비활성화
	if (Channel)
	{
		Channel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	}

	return Channel;
}

FMOD::Channel* USoundManager::PlaySound3D(const FString& SoundPath, const FVector& Location, bool bLoop, float MinDistance, float MaxDistance)
{
	if (!bIsInitialized || !FMODSystem)
	{
		std::cout << "[USoundManager] 사운드 시스템이 초기화되지 않았습니다." << std::endl;
		return nullptr;
	}

	// 사운드 로드 (3D 모드)
	FMOD::Sound* Sound = GetOrLoadSound(SoundPath, true, bLoop);
	if (!Sound)
	{
		return nullptr;
	}

	// 사운드 재생
	FMOD::Channel* Channel = nullptr;
	FMOD_RESULT Result = FMODSystem->playSound(Sound, nullptr, false, &Channel);
	if (!CheckFMODResult(Result, "FMODSystem->playSound"))
	{
		return nullptr;
	}

	if (Channel)
	{
		// 3D 위치 설정
		FMOD_VECTOR FMODPos;
		VectorToFMOD(Location, FMODPos);
		FMOD_VECTOR FMODVel = { 0.0f, 0.0f, 0.0f }; // 속도 (도플러 효과용)

		Channel->set3DAttributes(&FMODPos, &FMODVel);
		Channel->set3DMinMaxDistance(MinDistance, MaxDistance);
		Channel->setMode(bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	}

	return Channel;
}

FMOD::Sound* USoundManager::LoadSound(const FString& SoundPath, bool bIs3D, bool bLoop)
{
	if (!bIsInitialized || !FMODSystem)
	{
		std::cout << "[USoundManager] 사운드 시스템이 초기화되지 않았습니다." << std::endl;
		return nullptr;
	}

	// 사운드 모드 설정
	FMOD_MODE Mode = FMOD_DEFAULT;
	if (bIs3D)
	{
		Mode |= FMOD_3D;
	}
	else
	{
		Mode |= FMOD_2D;
	}

	if (bLoop)
	{
		Mode |= FMOD_LOOP_NORMAL;
	}
	else
	{
		Mode |= FMOD_LOOP_OFF;
	}

	// 사운드 생성
	FMOD::Sound* Sound = nullptr;
	FMOD_RESULT Result = FMODSystem->createSound(SoundPath.c_str(), Mode, nullptr, &Sound);
	if (!CheckFMODResult(Result, "FMODSystem->createSound"))
	{
		std::cout << "[USoundManager] 사운드 로드 실패: " << SoundPath << std::endl;
		return nullptr;
	}

	std::cout << "[USoundManager] 사운드 로드 성공: " << SoundPath << std::endl;
	return Sound;
}

FMOD::Sound* USoundManager::GetOrLoadSound(const FString& SoundPath, bool bIs3D, bool bLoop)
{
	// 캐시에서 찾기
	auto Iter = LoadedSounds.find(SoundPath);
	if (Iter != LoadedSounds.end())
	{
		return Iter->second;
	}

	// 캐시에 없으면 로드
	FMOD::Sound* Sound = LoadSound(SoundPath, bIs3D, bLoop);
	if (Sound)
	{
		LoadedSounds[SoundPath] = Sound;
	}

	return Sound;
}

void USoundManager::SetListenerPosition(const FVector& Position, const FVector& Forward, const FVector& Up)
{
	if (!bIsInitialized || !FMODSystem)
	{
		return;
	}

	// FVector를 FMOD_VECTOR로 변환
	FMOD_VECTOR FMODPos, FMODForward, FMODUp;
	VectorToFMOD(Position, FMODPos);
	VectorToFMOD(Forward, FMODForward);
	VectorToFMOD(Up, FMODUp);

	FMOD_VECTOR FMODVel = { 0.0f, 0.0f, 0.0f }; // 리스너 속도 (도플러 효과용)

	// 리스너 속성 설정 (리스너 인덱스 0)
	FMOD_RESULT Result = FMODSystem->set3DListenerAttributes(0, &FMODPos, &FMODVel, &FMODForward, &FMODUp);
	CheckFMODResult(Result, "FMODSystem->set3DListenerAttributes");
}

void USoundManager::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);

	if (MasterChannelGroup)
	{
		MasterChannelGroup->setVolume(MasterVolume);
	}
}

bool USoundManager::CheckFMODResult(int Result, const char* FunctionName)
{
	if (Result != FMOD_OK)
	{
		std::cout << "[USoundManager] FMOD 에러 (" << FunctionName << "): "
			<< FMOD_ErrorString(static_cast<FMOD_RESULT>(Result)) << std::endl;
		return false;
	}
	return true;
}

void USoundManager::VectorToFMOD(const FVector& InVector, FMOD_VECTOR& OutFMODVector)
{
	// DirectX11 Z-Up Left-Handed 좌표계를 FMOD 좌표계로 변환
	// FMOD는 기본적으로 Left-Handed Y-Up 좌표계를 사용
	// Z-Up → Y-Up 변환: (X, Y, Z) → (X, Z, Y)
	OutFMODVector.x = InVector.X;
	OutFMODVector.y = InVector.Z; // Z를 Y로
	OutFMODVector.z = InVector.Y; // Y를 Z로
}
