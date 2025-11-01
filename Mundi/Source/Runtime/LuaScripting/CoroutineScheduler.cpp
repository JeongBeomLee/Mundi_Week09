#include "pch.h"

#include "CoroutineScheduler.h"

void UCoroutineScheduler::Start(sol::function F)
{
    sol::coroutine Co(F);
    CoroutineEntry E{ Co, CurrentTime, nullptr, false };
    Entries.push_back(std::move(E));
    
    UE_LOG("[Coroutine] Started new coroutine. Total entries: %zu", Entries.size());
}

void UCoroutineScheduler::Update(double Dt)
{
    CurrentTime += Dt;

    uint32 index = 0;
    for (auto It = Entries.begin(); It != Entries.end();)
    {
        bool Ready = false;

        if (It->WaitingNextFrame)
        {
            UE_LOG("Entry index %u: CurrentTime = %.3f, WakeTime = %.3f, WaitingNextFrame = %s, HasWaitUntil = %s",
                index++,
                CurrentTime,
                It->WakeTime,
                It->WaitingNextFrame ? "true" : "false",
                It->WaitUntil ? "true" : "false");
            UE_LOG("[Coroutine] Ready: WaitingNextFrame");
            Ready = true;
            It->WaitingNextFrame = false;
        }
        else if (It->WaitUntil)
        {
            UE_LOG("Entry index %u: CurrentTime = %.3f, WakeTime = %.3f, WaitingNextFrame = %s, HasWaitUntil = %s",
                index++,
                CurrentTime,
                It->WakeTime,
                It->WaitingNextFrame ? "true" : "false",
                It->WaitUntil ? "true" : "false");
            if (It->WaitUntil())
            {
                UE_LOG("[Coroutine] Ready: WaitUntil condition met");
                Ready = true;
                It->WaitUntil = nullptr;
            }
        }
        else if (CurrentTime >= It->WakeTime)
        {
            UE_LOG("Entry index %u: CurrentTime = %.3f, WakeTime = %.3f, WaitingNextFrame = %s, HasWaitUntil = %s",
                index++,
                CurrentTime,
                It->WakeTime,
                It->WaitingNextFrame ? "true" : "false",
                It->WaitUntil ? "true" : "false");
            UE_LOG("[Coroutine] Ready: WakeTime reached (CurrentTime: %.3f >= WakeTime: %.3f)", 
                   CurrentTime, It->WakeTime);
            Ready = true;
        }
        else
        {
            /*UE_LOG("[Coroutine] Waiting: CurrentTime: %.3f < WakeTime: %.3f", 
                   CurrentTime, It->WakeTime);*/
        }

        if (!Ready) { ++It; continue; }

        UE_LOG("[Coroutine] Executing coroutine...");
        
        // ✅ auto로 받아서 sol2가 자동으로 타입 추론하도록 함
        auto result = It->Co();
        
        // ✅ 에러 체크
        if (!result.valid())
        {
            sol::error err = result;
            UE_LOG("[Coroutine Error] %s", err.what());
            It = Entries.erase(It);
            continue;
        }

        // ✅ 코루틴 상태 확인
        if (It->Co.status() == sol::call_status::ok)
        {
            UE_LOG("[Coroutine] Finished (status: ok)");
            It = Entries.erase(It);
            continue;
        }

        // ✅ yield 값 해석 - sol::object로 명시적 변환
        sol::object YieldedValue = result;  // auto -> sol::object
        sol::type ValueType = YieldedValue.get_type();
        
        UE_LOG("[Coroutine] Yielded value type: %d", static_cast<int>(ValueType));

        // ✅ nil 체크
        if (ValueType == sol::type::lua_nil)
        {
            UE_LOG("[Coroutine] Yielded: nil (wait next frame)");
            It->WaitingNextFrame = true;
        }
        // ✅ number 타입
        else if (ValueType == sol::type::number)
        {
            double Sec = YieldedValue.as<double>();
            It->WakeTime = CurrentTime + Sec;
            UE_LOG("[Coroutine] Yielded: %.3f seconds (WakeTime: %.3f)", Sec, It->WakeTime);
        }
        // ✅ function 타입
        else if (ValueType == sol::type::function)
        {
            UE_LOG("[Coroutine] Yielded: function (wait until condition)");
            sol::function Pred = YieldedValue.as<sol::function>();
            It->WaitUntil = [Pred]() {
                sol::protected_function_result R = Pred();
                return R.valid() && R.get<bool>();
            };
        }
        else
        {
            UE_LOG("[Coroutine] Yielded: unknown type (%d) (wait next frame)", static_cast<int>(ValueType));
            It->WaitingNextFrame = true;
        }

        ++It;
    }
}
