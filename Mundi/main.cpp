#include "pch.h"
#include "EditorEngine.h"

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

            OnDamage.BindMember(&Actor, &TestActor::OnDamageReceived);

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
                OnEvent.Remove(Handle2);
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
                OnEvent.Clear();
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

            OnDamage.AddMember(&Actor1, &TestActor::OnDamageReceived);
            OnDamage.AddMember(&Actor2, &TestActor::OnDamageReceived);
            OnDamage.AddMember(&Actor3, &TestActor::OnDamageReceived);

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
    TestDelegate();

    // lua 테스트 실행
    TestLua();

    if (!GEngine.Startup(hInstance))
        return -1;

    GEngine.MainLoop();
    GEngine.Shutdown();

    return 0;
}
