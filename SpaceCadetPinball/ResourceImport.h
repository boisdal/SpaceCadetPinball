#pragma once
#include <string>
class ResourceCatalog;
namespace ResourceImport
{
void Initialize(const std::string& preferencesDirectory);
ResourceCatalog& Catalog();
// Selects exactly the requested registered table; never falls back to another mode.
// Returns false for absent/unavailable imports; never silently loads an invalid entry.
bool SelectPreferred(bool preferWindows);
bool HasImports();
bool RunSetup();
void Open();
void Draw();
}
