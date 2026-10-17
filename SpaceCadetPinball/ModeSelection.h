#pragma once
#include <vector>
namespace ModeSelection
{
bool Prepare(const std::vector<const char*>& legacySearchPaths);
void DrawMenu();
void DrawConfirmation();
const char* WindowTitle();
}
