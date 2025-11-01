#include "pch.h"

#include "CoroutineScheduler.h"

void UCoroutineScheduler::Start(sol::function F)
{
    sol::thread NewThread = sol::thread::create(F.lua_state());
    sol::coroutine Co(NewThread.state(), F);

    CoroutineEntry E{ NewThread, Co, CurrentTime, nullptr, false, false };
    Entries.Add(std::move(E));

    UE_LOG("[Coroutine] Started new coroutine. Total entries: %zu", Entries.Num());
}

void UCoroutineScheduler::Update(double Dt)
{
    CurrentTime += Dt;

    if (Entries.empty())
    {
		return;
    }

    for (auto It = Entries.begin(); It != Entries.end();)
    {
        if (It->bFinished)
        {
            It = Entries.erase(It);
            continue;
        }
        ++It;
    }

    uint32 index = 0;
    for (auto It = Entries.begin(); It != Entries.end(); ++It, ++index)
    {
        if(It->bFinished)
        {
            continue;
		}

        bool Ready = false;

        if (It->WaitingNextFrame)
        {
            UE_LOG("[Coroutine] Ready: WaitingNextFrame, Entry index %u", index);
            Ready = true;
            It->WaitingNextFrame = false;
        }
        else if (It->WaitUntil)
        {
            if (It->WaitUntil())
            {
                UE_LOG("[Coroutine] Ready: WaitUntil condition met, Entry index %u", index);
                Ready = true;
                It->WaitUntil = nullptr;
            }
        }
        else if (CurrentTime >= It->WakeTime)
        {

            UE_LOG("[Coroutine] Ready: WakeTime reached (CurrentTime: %.3f >= WakeTime: %.3f), Entry index %u",
                   CurrentTime, It->WakeTime, index);
            Ready = true;
        }

        if (!Ready) { continue; }

        UE_LOG("[Coroutine] Executing coroutine..., Entry index %u", index);
        
		// ✅ auto로 받아서 sol2가 자동으로 타입 추론하도록 함
        auto result = It->Co(); // 다음 yield에 도달할 때까지 코루틴 실행 -> yield의 인자 리턴
        
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
            UE_LOG("[Coroutine] Finished (status: ok), Entry index %u", index);
            /*It = Entries.erase(It);*/
			It->bFinished = true;
            continue;
        }

        // ✅ yield 값 해석 - sol::object로 명시적 변환
        sol::object YieldedValue = result;  // auto -> sol::object
        sol::type ValueType = YieldedValue.get_type();

        UE_LOG("[Coroutine] Yielded value type: %d, Entry index %u", static_cast<int>(ValueType), index);

        // ✅ nil 체크
        if (ValueType == sol::type::lua_nil)
        {
            UE_LOG("[Coroutine] Yielded: nil (wait next frame), Entry index %u", index);  
            It->WaitingNextFrame = true;
        }
        // ✅ number 타입
        else if (ValueType == sol::type::number)
        {
            double Sec = YieldedValue.as<double>();
            It->WakeTime = CurrentTime + Sec;
            UE_LOG("[Coroutine] Yielded: %.3f seconds (WakeTime: %.3f), Entry index %u", Sec, It->WakeTime, index);
        }
        // ✅ function 타입
        else if (ValueType == sol::type::function)
        {
            UE_LOG("[Coroutine] Yielded: function (wait until condition), Entry index %u", index);
            sol::function Pred = YieldedValue.as<sol::function>();
            It->WaitUntil = [Pred]() {
                sol::protected_function_result R = Pred();
                return R.valid() && R.get<bool>();
            };
        }
        else
        {
            UE_LOG("[Coroutine] Yielded: unknown type (%d) (wait next frame), Entry index %u", static_cast<int>(ValueType), index);
            It->WaitingNextFrame = true;
        }
    }
}
