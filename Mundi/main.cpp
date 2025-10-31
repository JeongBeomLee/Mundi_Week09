#include "pch.h"
#include "EditorEngine.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#   define _CRTDBG_MAP_ALLOC
#   include <cstdlib>
#   include <crtdbg.h>
#endif

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

    // lua 테스트 실행
    TestLua();

    if (!GEngine.Startup(hInstance))
        return -1;

    GEngine.MainLoop();
    GEngine.Shutdown();

    return 0;
}
