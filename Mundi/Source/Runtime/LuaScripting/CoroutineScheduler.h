#pragma once
#include <sol/sol.hpp>

#include "Object.h"

struct CoroutineEntry 
{
    sol::coroutine Co;
    double WakeTime = 0.0;
    std::function<bool()> WaitUntil;
    bool WaitingNextFrame = false;
};

class UCoroutineScheduler : public UObject 
{
public:
    UCoroutineScheduler() = default;
	~UCoroutineScheduler() override = default;

    void Start(sol::function F);
    void Update(double Dt);

    void RegisterCoroutineTo(sol::state& Lua)
    {
        Lua.set_function("StartCoroutine", [&](sol::function f) {
            Start(f);
        });
	}
private:
    double CurrentTime = 0.0;
    std::vector<CoroutineEntry> Entries;
};

// 코루틴 예시
//lua.script(R"(
//        function MyRoutine()
//            print("[Lua] 시작")
//            coroutine.yield()               -- 다음 프레임
//            print("[Lua] 0.5초 대기")
//            coroutine.yield(0.5)            -- 0.5초 대기
//            print("[Lua] 조건 대기")
//            local count = 0
//            coroutine.yield(function()
//                count = count + 1
//                return count >= 3
//            end)
//            print("[Lua] 끝")
//        end
//
//        StartCoroutine(MyRoutine)
//    )");