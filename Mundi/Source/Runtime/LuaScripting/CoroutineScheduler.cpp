#include "pch.h"

#include "CoroutineScheduler.h"

void UCoroutineScheduler::Start(sol::function F)
{
    sol::coroutine Co(F);
    CoroutineEntry E{ Co, CurrentTime, nullptr, true };  // 첫 실행을 위해 true로 설정
    Entries.push_back(std::move(E));
    
    UE_LOG("[Coroutine] Started new coroutine. Total entries: %zu", Entries.size());
}

void UCoroutineScheduler::Update(double Dt)
{
    CurrentTime += Dt;

    /*UE_LOG("[Coroutine] Update called. CurrentTime: %.3f, Active coroutines: %zu", 
           CurrentTime, Entries.size());*/

    for (auto It = Entries.begin(); It != Entries.end();)
    {
        bool Ready = false;

        if (It->WaitingNextFrame)
        {
            UE_LOG("[Coroutine] Ready: WaitingNextFrame");
            Ready = true;
            It->WaitingNextFrame = false;
        }
        else if (It->WaitUntil)
        {
            if (It->WaitUntil())
            {
                UE_LOG("[Coroutine] Ready: WaitUntil condition met");
                Ready = true;
                It->WaitUntil = nullptr;
            }
        }
        else if (CurrentTime >= It->WakeTime)
        {
            UE_LOG("[Coroutine] Ready: WakeTime reached (CurrentTime: %.3f >= WakeTime: %.3f)", 
                   CurrentTime, It->WakeTime);
            Ready = true;
        }
        else
        {
            UE_LOG("[Coroutine] Waiting: CurrentTime: %.3f < WakeTime: %.3f", 
                   CurrentTime, It->WakeTime);
        }

        if (!Ready) { ++It; continue; }

        UE_LOG("[Coroutine] Executing coroutine...");
        //double val = It->Co();
        sol::protected_function_result Res = It->Co();
        
        if (!Res.valid())
        {
            sol::error Err = Res;
            UE_LOG("[Coroutine Error] %s", Err.what());
            It = Entries.erase(It);
            continue;
        }

        // 정상 종료
        if (It->Co().status() == sol::call_status::ok)
        {
            UE_LOG("[Coroutine] Finished (status: ok)");
            It = Entries.erase(It);
            continue;
        }

        // yield 값 해석
        UE_LOG("[Coroutine] Yield return_count: %d", Res.return_count());

        /*if (Res.return_count() == 0 || Res.get<sol::object>(0).is<sol::nil_t>())
        {
            UE_LOG("[Coroutine] Yielded: nil (wait next frame)");
            It->WaitingNextFrame = true;
        }
        else if (Res.get<sol::object>(0).is<double>())
        {
            double Sec = Res.get<double>(0);
            It->WakeTime = CurrentTime + Sec;
            UE_LOG("[Coroutine] Yielded: %.3f seconds (WakeTime: %.3f)", Sec, It->WakeTime);
        }
        else if (Res.get<sol::object>(0).is<sol::function>())
        {
            UE_LOG("[Coroutine] Yielded: function (wait until condition)");
            sol::function Pred = Res.get<sol::function>(0);
            It->WaitUntil = [Pred]() {
                sol::protected_function_result R = Pred();
                return R.valid() && R.get<bool>();
            };
        }
        else
        {
            UE_LOG("[Coroutine] Yielded: unknown type (wait next frame)");
            It->WaitingNextFrame = true;
        }*/

        // yield 값 해석
        if (Res.return_count() == 0)
        {
            UE_LOG("[Coroutine] Yielded: nil (wait next frame)");
            It->WaitingNextFrame = true;
        }
        else
        {
            // ✅ 첫 번째 반환값 확인
            sol::object FirstArg = Res[0];  // get<sol::object>(0) 대신 [0] 사용
            sol::type ArgType = FirstArg.get_type();

            UE_LOG("[Coroutine] FirstArg type: %d", static_cast<int>(ArgType));

            if (ArgType == sol::type::number)
            {
                double Sec = FirstArg.as<double>();
                It->WakeTime = CurrentTime + Sec;
                UE_LOG("[Coroutine] Yielded: %.3f seconds (WakeTime: %.3f)", Sec, It->WakeTime);
            }
            else if (ArgType == sol::type::function)
            {
                UE_LOG("[Coroutine] Yielded: function (wait until condition)");
                sol::function Pred = FirstArg.as<sol::function>();
                It->WaitUntil = [Pred]() {
                    sol::protected_function_result R = Pred();
                    return R.valid() && R.get<bool>();
                    };
            }
            else if (ArgType == sol::type::boolean)
            {
                // ✅ resume의 성공 여부 (true)인 경우 무시
                UE_LOG("[Coroutine] Yielded: boolean (resume success flag, ignored)");
                It->WaitingNextFrame = true;
            }
            else
            {
                UE_LOG("[Coroutine] Yielded: unknown type (%d) (wait next frame)", static_cast<int>(ArgType));
                It->WaitingNextFrame = true;
            }
        }

        ++It;
    }
}
