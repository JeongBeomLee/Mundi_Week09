#include "pch.h"
#include "EditorEngine.h"
//#include "EmptyActor.h"
#include "SceneComponent.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

// ============================================
// Delegate 유닛 테스트
// ============================================

// 테스트용 클래스
class TestActor
{
public:
    int32 CallCount = 0;
    float LastValue = 0.0f;
    FString LastMessage;

    void OnDamageReceived(float Damage)
    {
        CallCount++;
        LastValue = Damage;
    }

    void OnMessageReceived(const FString& Message)
    {
        CallCount++;
        LastMessage = Message;
    }

    void OnTwoParams(int32 Value1, float Value2)
    {
        CallCount++;
        LastValue = static_cast<float>(Value1) + Value2;
    }
};

void TestDelegate()
{
    std::string result = "=== Delegate System Unit Test ===\n\n";
    bool allTestsPassed = true;

    try
    {
        // ============================================
        // 테스트 1: TDelegate 기본 바인딩 (람다)
        // ============================================
        {
            result += "[Test 1] TDelegate - Lambda Binding\n";

            int32 CallCount = 0;
            TDelegate<int32> OnScoreChanged;

            OnScoreChanged.Bind([&CallCount](int32 NewScore) {
                CallCount = NewScore;
            });

            if (OnScoreChanged.IsBound())
            {
                OnScoreChanged.Execute(100);
                if (CallCount == 100)
                {
                    result += "  [O] PASS: Lambda bound and executed correctly\n";
                }
                else
                {
                    result += "  [X] FAIL: Lambda executed but value incorrect\n";
                    allTestsPassed = false;
                }
            }
            else
            {
                result += "  ✗ FAIL: Lambda not bound\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 2: TDelegate 멤버 함수 바인딩
        // ============================================
        {
            result += "[Test 2] TDelegate - Member Function Binding\n";

            TestActor Actor;
            TDelegate<float> OnDamage;

            OnDamage.BindDynamic(&Actor, &TestActor::OnDamageReceived);

            if (OnDamage.IsBound())
            {
                OnDamage.Execute(50.5f);
                if (Actor.CallCount == 1 && Actor.LastValue == 50.5f)
                {
                    result += "  [O] PASS: Member function bound and executed correctly\n";
                }
                else
                {
                    result += "  [X] FAIL: Member function executed but values incorrect\n";
                    allTestsPassed = false;
                }
            }
            else
            {
                result += "  [X] FAIL: Member function not bound\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 3: TDelegate Unbind
        // ============================================
        {
            result += "[Test 3] TDelegate - Unbind\n";

            int32 CallCount = 0;
            TDelegate<> OnEvent;

            OnEvent.Bind([&CallCount]() { CallCount++; });
            OnEvent.Execute();

            if (CallCount == 1)
            {
                OnEvent.Unbind();
                if (!OnEvent.IsBound())
                {
                    OnEvent.Execute(); // 아무 일도 일어나지 않아야 함
                    if (CallCount == 1) // 여전히 1
                    {
                        result += "  [O] PASS: Unbind works correctly\n";
                    }
                    else
                    {
                        result += "  [X] FAIL: Unbind did not prevent execution\n";
                        allTestsPassed = false;
                    }
                }
                else
                {
                    result += "  [X] FAIL: IsBound still true after Unbind\n";
                    allTestsPassed = false;
                }
            }
            else
            {
                result += "  [X] FAIL: Initial execution failed\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 4: TMulticastDelegate 여러 함수 바인딩
        // ============================================
        {
            result += "[Test 4] TMulticastDelegate - Multiple Bindings\n";

            int32 Count1 = 0, Count2 = 0, Count3 = 0;
            TMulticastDelegate<int32> OnEvent;

            OnEvent.Add([&Count1](int32 Val) { Count1 = Val; });
            OnEvent.Add([&Count2](int32 Val) { Count2 = Val * 2; });
            OnEvent.Add([&Count3](int32 Val) { Count3 = Val * 3; });

            OnEvent.Broadcast(10);

            if (Count1 == 10 && Count2 == 20 && Count3 == 30)
            {
                result += "  [O] PASS: All three functions executed correctly\n";
            }
            else
            {
                result += "  [X] FAIL: Functions not executed correctly\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 5: TMulticastDelegate Handle 제거
        // ============================================
        {
            result += "[Test 5] TMulticastDelegate - Remove by Handle\n";

            int32 Count1 = 0, Count2 = 0, Count3 = 0;
            TMulticastDelegate<> OnEvent;

            auto Handle1 = OnEvent.Add([&Count1]() { Count1++; });
            auto Handle2 = OnEvent.Add([&Count2]() { Count2++; });
            auto Handle3 = OnEvent.Add([&Count3]() { Count3++; });

            OnEvent.Broadcast();
            if (Count1 == 1 && Count2 == 1 && Count3 == 1)
            {
                // Handle2 제거
                OnEvent.RemoveDynamic(Handle2);
                OnEvent.Broadcast();

                if (Count1 == 2 && Count2 == 1 && Count3 == 2)
                {
                    result += "  [O] PASS: Handle removal works correctly\n";
                }
                else
                {
                    result += "  [X] FAIL: Handle removal did not work\n";
                    allTestsPassed = false;
                }
            }
            else
            {
                result += "  [X] FAIL: Initial broadcast failed\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 6: TMulticastDelegate Clear
        // ============================================
        {
            result += "[Test 6] TMulticastDelegate - Clear All\n";

            int32 CallCount = 0;
            TMulticastDelegate<> OnEvent;

            OnEvent.Add([&CallCount]() { CallCount++; });
            OnEvent.Add([&CallCount]() { CallCount++; });
            OnEvent.Add([&CallCount]() { CallCount++; });

            OnEvent.Broadcast();
            if (CallCount == 3)
            {
                OnEvent.RemoveAll();
                if (!OnEvent.IsBound())
                {
                    OnEvent.Broadcast();
                    if (CallCount == 3) // 여전히 3 (증가 안 됨)
                    {
                        result += "  [O] PASS: Clear works correctly\n";
                    }
                    else
                    {
                        result += "  [X] FAIL: Clear did not prevent execution\n";
                        allTestsPassed = false;
                    }
                }
                else
                {
                    result += "  [X] FAIL: IsBound still true after Clear\n";
                    allTestsPassed = false;
                }
            }
            else
            {
                result += "  [X] FAIL: Initial broadcast failed\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 7: 매크로 테스트
        // ============================================
        {
            result += "[Test 7] Delegate Macros\n";

            DECLARE_DELEGATE_OneParam(FOnScoreDelegate, int32);
            DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverlapDelegate, float, float);

            int32 Score = 0;
            FOnScoreDelegate OnScore;
            OnScore.Bind([&Score](int32 NewScore) { Score = NewScore; });
            OnScore.Execute(999);

            float Value1 = 0.0f, Value2 = 0.0f;
            FOnOverlapDelegate OnOverlap;
            OnOverlap.Add([&Value1, &Value2](float V1, float V2) {
                Value1 = V1;
                Value2 = V2;
            });
            OnOverlap.Broadcast(1.5f, 2.5f);

            if (Score == 999 && Value1 == 1.5f && Value2 == 2.5f)
            {
                result += "  [O] PASS: Macros work correctly\n";
            }
            else
            {
                result += "  [X] FAIL: Macros did not work correctly\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 8: 멤버 함수 여러 개 바인딩
        // ============================================
        {
            result += "[Test 8] TMulticastDelegate - Multiple Member Functions\n";

            TestActor Actor1, Actor2, Actor3;
            TMulticastDelegate<float> OnDamage;

            OnDamage.AddDynamic(&Actor1, &TestActor::OnDamageReceived);
            OnDamage.AddDynamic(&Actor2, &TestActor::OnDamageReceived);
            OnDamage.AddDynamic(&Actor3, &TestActor::OnDamageReceived);

            OnDamage.Broadcast(75.5f);

            if (Actor1.CallCount == 1 && Actor1.LastValue == 75.5f &&
                Actor2.CallCount == 1 && Actor2.LastValue == 75.5f &&
                Actor3.CallCount == 1 && Actor3.LastValue == 75.5f)
            {
                result += "  [O] PASS: Multiple member functions executed correctly\n";
            }
            else
            {
                result += "  [X] FAIL: Member functions not executed correctly\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 테스트 9: Operator() 테스트
        // ============================================
        {
            result += "[Test 9] Operator() Overload\n";

            int32 Value = 0;
            TDelegate<int32> SingleDelegate;
            SingleDelegate.Bind([&Value](int32 V) { Value = V; });
            SingleDelegate(42);  // operator() 사용

            bool Test1 = (Value == 42);

            int32 MultiValue = 0;
            TMulticastDelegate<int32> MultiDelegate;
            MultiDelegate.Add([&MultiValue](int32 V) { MultiValue += V; });
            MultiDelegate.Add([&MultiValue](int32 V) { MultiValue += V; });
            MultiDelegate(10);  // operator() 사용

            bool Test2 = (MultiValue == 20);

            if (Test1 && Test2)
            {
                result += "  [O] PASS: Operator() works correctly\n";
            }
            else
            {
                result += "  [X] FAIL: Operator() did not work correctly\n";
                allTestsPassed = false;
            }
            result += "\n";
        }

        // ============================================
        // 최종 결과
        // ============================================
        result += "========================================\n";
        if (allTestsPassed)
        {
            result += "[O] ALL TESTS PASSED!\n";
        }
        else
        {
            result += "[X] SOME TESTS FAILED!\n";
        }
        result += "========================================\n";

        MessageBoxA(NULL, result.c_str(),
                   allTestsPassed ? "Delegate Test - SUCCESS" : "Delegate Test - FAILED",
                   MB_OK | (allTestsPassed ? MB_ICONINFORMATION : MB_ICONWARNING));
    }
    catch (const std::exception& e)
    {
        result += "\n[X] EXCEPTION OCCURRED:\n";
        result += e.what();
        MessageBoxA(NULL, result.c_str(), "Delegate Test - EXCEPTION", MB_OK | MB_ICONERROR);
    }
}

// lua 스크립트 파일 로드 테스트 함수
void TestLua()
{
    try
    {
        // lua 상태 생성
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

        // template.lua 파일 로드
        lua.script_file("Scripts/template.lua");

        // 각 함수가 정의되어 있는지 확인
        std::string message = "Lua Script Load Test:\n\n";
        message += "template.lua loaded successfully!\n\n";

        // BeginPlay 함수 확인
        if (lua["BeginPlay"].valid() && lua["BeginPlay"].get_type() == sol::type::function)
            message += "[OK] BeginPlay function found\n";
        else
            message += "[FAIL] BeginPlay function not found\n";

        // EndPlay 함수 확인
        if (lua["EndPlay"].valid() && lua["EndPlay"].get_type() == sol::type::function)
            message += "[OK] EndPlay function found\n";
        else
            message += "[FAIL] EndPlay function not found\n";

        // OnOverlap 함수 확인
        if (lua["OnOverlap"].valid() && lua["OnOverlap"].get_type() == sol::type::function)
            message += "[OK] OnOverlap function found\n";
        else
            message += "[FAIL] OnOverlap function not found\n";

        // Tick 함수 확인
        if (lua["Tick"].valid() && lua["Tick"].get_type() == sol::type::function)
            message += "[OK] Tick function found\n";
        else
            message += "[FAIL] Tick function not found\n";

        MessageBoxA(NULL, message.c_str(), "Lua Script Test", MB_OK | MB_ICONINFORMATION);
    }
    catch (const sol::error& e)
    {
        std::string errorMsg = "Lua Error:\n";
        errorMsg += e.what();
        errorMsg += "\n\nMake sure Scripts/template.lua exists!";
        MessageBoxA(NULL, errorMsg.c_str(), "Lua Test Failed", MB_OK | MB_ICONERROR);
    }
}

// ============================================
// Lua + Delegate를 활용한 Actor Transform 제어 테스트
// ============================================
void TestLuaWithDelegateTransform()
{
    std::string result = "=== Lua + Delegate Actor Transform Test ===\n\n";
    bool allTestsPassed = true;

    try
    {
        // ============================================
        // 테스트 1: Lua에서 Actor Transform 변경
        // ============================================
        {
            result += "[Test 1] Lua Script - Actor Transform Modification\n";

            // Actor 생성 (ObjectFactory 사용)
            AActor* TestActor = ObjectFactory::NewObject<AActor>();
            TestActor->SetName("TestLuaActor");
            
            // 초기 Transform 설정
            FVector InitialLocation(10.0f, 20.0f, 30.0f);
            FQuat InitialRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, 0.0f));
            FVector InitialScale(1.0f, 1.0f, 1.0f);
            
			TestActor->SetRootComponent(ObjectFactory::NewObject<USceneComponent>());

            TestActor->SetActorLocation(InitialLocation);
            TestActor->SetActorRotation(InitialRotation);
            TestActor->SetActorScale(InitialScale);

            // Lua 상태 생성 및 FVector 타입 바인딩
            sol::state lua;
            lua.open_libraries(sol::lib::base, sol::lib::math);

            // FVector 타입을 Lua에 등록
            lua.new_usertype<FVector>("FVector",
                sol::constructors<FVector(), FVector(float, float, float)>(),
                "X", &FVector::X,
                "Y", &FVector::Y,
                "Z", &FVector::Z,
                "Add", [](const FVector& a, const FVector& b) { return a + b; },
                "Sub", [](const FVector& a, const FVector& b) { return a - b; },
                "Mul", [](const FVector& v, float scalar) { return v * scalar; }
            );

            // FQuat 타입을 Lua에 등록
            lua.new_usertype<FQuat>("FQuat",
                sol::constructors<FQuat()>(),
                "MakeFromEuler", [](float pitch, float yaw, float roll) {
                    return FQuat::MakeFromEulerZYX(FVector(pitch, yaw, roll));
                }
            );

            // Actor 래퍼 클래스 등록
            lua.new_usertype<AActor>("Actor",
                "GetLocation", &AActor::GetActorLocation,
                "SetLocation", &AActor::SetActorLocation,
                "GetRotation", &AActor::GetActorRotation,
                "SetRotation", sol::overload(
                    static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation),
                    static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation)
                ),
                "GetScale", &AActor::GetActorScale,
                "SetScale", &AActor::SetActorScale,
                "AddWorldLocation", &AActor::AddActorWorldLocation,
                "AddWorldRotation", sol::overload(
                    static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorWorldRotation)
                ),
                "GetName", &AActor::GetName
            );

            // Lua 스크립트 실행
            lua["actor"] = TestActor;

            FQuat testRotation = FQuat::MakeFromEulerZYX(FVector(10.0f, 80.0f, 20.0f));
			FVector testRotationEuler = testRotation.ToEulerZYXDeg();

            lua.script(R"(
                -- 위치 변경
                local newLocation = FVector.new(100, 200, 300)
                actor:SetLocation(newLocation)
                
                -- 스케일 변경
                local newScale = FVector.new(2, 2, 2)
                actor:SetScale(newScale)

                -- 회전 변경
                local newRotation = FQuat.MakeFromEuler(10, 80, 20)
                actor:SetRotation(newRotation)
                
                -- 상대 위치 이동
                local deltaLocation = FVector.new(10, 0, 0)
                actor:AddWorldLocation(deltaLocation)
            )");

            // 결과 검증
            FVector finalLocation = TestActor->GetActorLocation();
            FVector finalScale = TestActor->GetActorScale();
			FQuat finalRotation = TestActor->GetActorRotation();

            if (finalLocation.X == 110.0f && finalLocation.Y == 200.0f && finalLocation.Z == 300.0f &&
                finalScale.X == 2.0f && finalScale.Y == 2.0f && finalScale.Z == 2.0f &&
                finalRotation == testRotation)
            {
                result += "  [O] PASS: Lua successfully modified Actor transform\n";
                result += "      Location: (" + std::to_string(finalLocation.X) + ", " + 
                          std::to_string(finalLocation.Y) + ", " + std::to_string(finalLocation.Z) + ")\n";
                result += "      Scale: (" + std::to_string(finalScale.X) + ", " + 
                          std::to_string(finalScale.Y) + ", " + std::to_string(finalScale.Z) + ")\n";
				result += "      Rotation (Euler): (" + std::to_string(finalRotation.X) + ", " +
					std::to_string(finalRotation.Y) + ", " + std::to_string(finalRotation.Z) + ")\n";
            }
            else
            {
                result += "  [X] FAIL: Lua transform modification incorrect\n";
                allTestsPassed = false;
            }
            result += "\n";

            // Actor 정리
            ObjectFactory::DeleteObject(TestActor);
        }

        // ============================================
        // 테스트 2: Delegate로 Transform 변경 알림
        // ============================================
        {
            result += "[Test 2] Delegate - Transform Change Notification\n";

            AActor* TestActor = ObjectFactory::NewObject<AActor>();
            TestActor->SetName("DelegateActor");

            // RootComponent 설정 추가!
            TestActor->SetRootComponent(ObjectFactory::NewObject<USceneComponent>());

            // Transform 변경 알림 델리게이트 선언
            DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnTransformChanged, const FVector&, const FQuat&, const FVector&);
            FOnTransformChanged OnTransformChanged;

            // 변경 이력 저장
            int32 ChangeCount = 0;
            FVector LastLocation;
            FVector LastScale;

            // 델리게이트에 람다 바인딩
            OnTransformChanged.Add([&ChangeCount, &LastLocation, &LastScale](const FVector& Loc, const FQuat& Rot, const FVector& Scale) {
                ChangeCount++;
                LastLocation = Loc;
                LastScale = Scale;
            });

            // Lua 상태 생성
            sol::state lua;
            lua.open_libraries(sol::lib::base, sol::lib::math);

            // FVector 타입을 Lua에 등록
            lua.new_usertype<FVector>("FVector",
                sol::constructors<FVector(), FVector(float, float, float)>(),
                "X", &FVector::X,
                "Y", &FVector::Y,
                "Z", &FVector::Z,
                "Add", [](const FVector& a, const FVector& b) { return a + b; },
                "Sub", [](const FVector& a, const FVector& b) { return a - b; },
                "Mul", [](const FVector& v, float scalar) { return v * scalar; }
            );

            // FQuat 타입을 Lua에 등록
            lua.new_usertype<FQuat>("FQuat",
                sol::constructors<FQuat()>(),
                "MakeFromEuler", [](float pitch, float yaw, float roll) {
                    return FQuat::MakeFromEulerZYX(FVector(pitch, yaw, roll));
                }
            );

            // Actor 래퍼 클래스 등록
            lua.new_usertype<AActor>("Actor",
                "GetLocation", &AActor::GetActorLocation,
                "SetLocation", &AActor::SetActorLocation,
                "GetRotation", &AActor::GetActorRotation,
                "SetRotation", sol::overload(
                    static_cast<void(AActor::*)(const FVector&)>(&AActor::SetActorRotation),
                    static_cast<void(AActor::*)(const FQuat&)>(&AActor::SetActorRotation)
                ),
                "GetScale", &AActor::GetActorScale,
                "SetScale", &AActor::SetActorScale,
                "AddWorldLocation", &AActor::AddActorWorldLocation,
                "AddWorldRotation", sol::overload(
                    static_cast<void(AActor::*)(const FQuat&)>(&AActor::AddActorWorldRotation)
                ),
                "GetName", &AActor::GetName
            );

            // Actor와 델리게이트 함께 사용
            lua["actor"] = TestActor;
            lua["notifyTransformChanged"] = [&](const FVector& loc, const FQuat& rot, const FVector& scale) {
                OnTransformChanged.Broadcast(loc, rot, scale);
            };

            // Lua에서 Transform 변경 및 델리게이트 호출
            lua.script(R"(
                local newLoc = FVector.new(50, 60, 70)
                local newScale = FVector.new(1.5, 1.5, 1.5)
                
                actor:SetLocation(newLoc)
                actor:SetScale(newScale)
                
                -- 변경 사항 알림
                notifyTransformChanged(
                    actor:GetLocation(),
                    actor:GetRotation(),
                    actor:GetScale()
                )
            )");

            if (ChangeCount == 1 && 
                LastLocation.X == 50.0f && LastLocation.Y == 60.0f && LastLocation.Z == 70.0f &&
                LastScale.X == 1.5f && LastScale.Y == 1.5f && LastScale.Z == 1.5f)
            {
                result += "  [O] PASS: Delegate correctly notified of transform changes\n";
                result += "      Notifications: " + std::to_string(ChangeCount) + "\n";
                result += "      Last Location: (" + std::to_string(LastLocation.X) + ", " + 
                          std::to_string(LastLocation.Y) + ", " + std::to_string(LastLocation.Z) + ")\n";
            }
            else
            {
                result += "  [X] FAIL: Delegate notification failed\n";
                allTestsPassed = false;
            }
            result += "\n";

            // Actor 정리
            ObjectFactory::DeleteObject(TestActor);
        }

        // ============================================
        // 테스트 3: Lua 스크립트로 Actor 애니메이션
        // ============================================
        {
            result += "[Test 3] Lua Script - Actor Animation with Tick\n";

            AActor* TestActor = ObjectFactory::NewObject<AActor>();
            TestActor->SetName("AnimatedActor");
            TestActor->SetActorLocation(FVector(0, 0, 0));

            // Tick 델리게이트
            DECLARE_MULTICAST_DELEGATE_OneParam(FOnTick, float);
            FOnTick OnTick;

            sol::state lua;
            lua.open_libraries(sol::lib::base, sol::lib::math);

            // FVector 등록
            lua.new_usertype<FVector>("FVector",
                sol::constructors<FVector(), FVector(float, float, float)>(),
                "X", &FVector::X,
                "Y", &FVector::Y,
                "Z", &FVector::Z
            );

            // Actor 등록
            lua.new_usertype<AActor>("Actor",
                "GetLocation", &AActor::GetActorLocation,
                "SetLocation", &AActor::SetActorLocation,
                "AddWorldLocation", &AActor::AddActorWorldLocation
            );

            lua["actor"] = TestActor;
            lua["time"] = 0.0f;

            // Lua Tick 함수 정의
            lua.script(R"(
                function Tick(deltaTime)
                    time = time + deltaTime
                    
                    -- 원형 운동
                    local radius = 100
                    local x = math.cos(time) * radius
                    local y = math.sin(time) * radius
                    local z = 0
                    
                    local newPos = FVector.new(x, y, z)
                    actor:SetLocation(newPos)
                end
            )");

            // 델리게이트에 Lua Tick 연결
            OnTick.Add([&lua](float dt) {
                if (lua["Tick"].valid()) {
                    lua["Tick"](dt);
                }
            });

            // 시뮬레이션 (몇 프레임)
            float totalTime = 0.0f;
            for (int i = 0; i < 10; i++)
            {
                float dt = 0.016f; // 60 FPS
                OnTick.Broadcast(dt);
                totalTime += dt;
            }

            FVector finalPos = TestActor->GetActorLocation();
            
            // 원형 운동이 올바르게 적용되었는지 확인 (대략적인 범위 체크)
            if (abs(finalPos.X) <= 100.0f && abs(finalPos.Y) <= 100.0f && finalPos.Z == 0.0f)
            {
                result += "  [O] PASS: Lua animation script executed correctly\n";
                result += "      Final Position: (" + std::to_string(finalPos.X) + ", " + 
                          std::to_string(finalPos.Y) + ", " + std::to_string(finalPos.Z) + ")\n";
                result += "      Total Time: " + std::to_string(totalTime) + "s\n";
            }
            else
            {
                result += "  [X] FAIL: Lua animation script failed\n";
                allTestsPassed = false;
            }
            result += "\n";

            // Actor 정리
            ObjectFactory::DeleteObject(TestActor);
        }

        // ============================================
        // 최종 결과
        // ============================================
        result += "========================================\n";
        if (allTestsPassed)
        {
            result += "[O] ALL TESTS PASSED!\n";
        }
        else
        {
            result += "[X] SOME TESTS FAILED!\n";
        }
        result += "========================================\n";

        MessageBoxA(NULL, result.c_str(),
                   allTestsPassed ? "Lua Transform Test - SUCCESS" : "Lua Transform Test - FAILED",
                   MB_OK | (allTestsPassed ? MB_ICONINFORMATION : MB_ICONWARNING));
    }
    catch (const std::exception& e)
    {
        result += "\n[X] EXCEPTION OCCURRED:\n";
        result += e.what();
        MessageBoxA(NULL, result.c_str(), "Lua Transform Test - EXCEPTION", MB_OK | MB_ICONERROR);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetBreakAlloc(0);
#endif

    // Delegate 시스템 테스트 실행
    //TestDelegate();

    // lua 테스트 실행
    //TestLua();

    // Lua + Delegate를 활용한 Actor Transform 제어 테스트
    TestLuaWithDelegateTransform();

    if (!GEngine.Startup(hInstance))
        return -1;

    GEngine.MainLoop();
    GEngine.Shutdown();

    return 0;
}
