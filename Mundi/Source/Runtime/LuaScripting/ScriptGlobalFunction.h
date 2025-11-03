#pragma once

void PrintToConsole(const char* ch);
bool IsKeyPressed(int KeyCode);

class ARunnerGameMode;

ARunnerGameMode* GetRunnerGameMode(UWorld* World);
