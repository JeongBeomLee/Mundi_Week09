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

        //Lua.set_function("StartCoroutine", [this](sol::object obj) {  // ✅ sol::object로 받기
        //    if (obj.is<sol::function>()) {
        //        // 일반 함수면 코루틴으로 변환
        //        sol::function f = obj.as<sol::function>();
        //        sol::coroutine co = sol::coroutine(f);  // ✅ 명시적 변환
        //        Start(co);
        //    }
        //    else if (obj.is<sol::coroutine>()) {
        //        // 이미 코루틴이면 그대로 사용
        //        Start(obj.as<sol::coroutine>());
        //    }
        //});

 //       Lua.set_function("StartCoroutine", [this](sol::function f) {
 //           // ✅ sol::function을 sol::coroutine으로 명시적 변환
 //           sol::coroutine co(f);

 //           // 첫 실행 (yield 전까지 실행)
 //           sol::protected_function_result result = co();
 //           if (!result.valid()) {
 //               sol::error err = result;
 //               UE_LOG("[Coroutine] Failed to start: %s", err.what());
 //               return;
 //           }

 //           // ✅ 여기서 yield 값을 확인!
 //           CoroutineEntry E{ co, CurrentTime, nullptr, false };

 //           // yield 값 해석
 //           if (result.return_count() > 0 && !result.get<sol::object>(0).is<sol::nil_t>()) {
 //               sol::object firstArg = result.get<sol::object>(0);
 //               if (firstArg.get_type() == sol::type::number) {
 //                   double sec = firstArg.as<double>();
 //                   E.WakeTime = CurrentTime + sec;
 //                   UE_LOG("[Coroutine] Started with %.3f seconds delay", sec);
 //               }
 //           }
 //           else {
 //               E.WaitingNextFrame = true;
 //           }

 //           Entries.push_back(std::move(E));
 //           UE_LOG("[Coroutine] Started new coroutine. Total entries: %zu", Entries.size());
 //       });
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