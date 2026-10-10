#include "../SpaceCadetPinball/high_score_storage.h"
#include <cassert>
#include <map>
#include <iostream>

int main()
{
	std::map<std::string, std::string> settings;
	auto get = [&](const std::string& key, const std::string& fallback) {
		auto it = settings.find(key);
		return it == settings.end() ? fallback : it->second;
	};
	auto set = [&](const std::string& key, const std::string& value) { settings[key] = value; };
	high_score_struct table[5], loaded[5];
	high_score_storage::clear(table);
	assert(!high_score_storage::read(loaded, "windows", get));
	assert(loaded[0].Score == -999);
	std::strcpy(table[0].Name, "Windows player");
	table[0].Score = 1000000;
	high_score_storage::write(table, "windows", set);
	std::strcpy(table[0].Name, "FT player");
	table[0].Score = 2000000;
	high_score_storage::write(table, "full-tilt", set);
	// Fresh table reads simulate a restart; alternate profiles repeatedly.
	for (int repeat = 0; repeat < 3; ++repeat)
	{
		assert(high_score_storage::read(loaded, "windows", get));
		assert(loaded[0].Score == 1000000);
		assert(std::string(loaded[0].Name) == "Windows player");
		assert(high_score_storage::read(loaded, "full-tilt", get));
		assert(loaded[0].Score == 2000000);
	}
	// Seed original upstream keys; importing them never assigns a historical mode.
	for (int index = 0; index < 5; ++index)
	{
		settings[std::to_string(index) + ".Name"] = table[index].Name;
		settings[std::to_string(index) + ".Score"] = std::to_string(table[index].Score);
	}
	settings["Verification"] = std::to_string(high_score_storage::checksum(table));
	auto original = settings;
	assert(high_score_storage::read(loaded, "", get));
	assert(loaded[0].Score == 2000000);
	high_score_storage::clear(loaded);
	high_score_storage::write(loaded, "", set);
	assert(settings == original);
	high_score_storage::write(loaded, "windows", set);
	assert(high_score_storage::read(loaded, "full-tilt", get));
	assert(loaded[0].Score == 2000000);
	assert(!high_score_storage::read(loaded, "custom.first", get));
	assert(!high_score_storage::read(loaded, "full-tilt-demo", get));
	assert(std::string(high_score_storage::profile(true, true)) == "full-tilt-demo");
	// Malformed settings and checksum corruption cannot leak partially read scores.
	for (auto invalid : {"oops", "2147483648", "123junk"})
	{
		settings["HighScores.full-tilt.0.Score"] = invalid;
		assert(!high_score_storage::read(loaded, "full-tilt", get));
		assert(loaded[0].Name[0] == 0 && loaded[0].Score == -999);
	}
	settings = original;
	settings["HighScores.full-tilt.0.Name"] = std::string(32, 'x');
	assert(!high_score_storage::read(loaded, "full-tilt", get));
	// Defined checksum wrap even when five scores approach INT_MAX.
	for (auto& entry : table) entry.Score = 2000000000;
	high_score_storage::write(table, "windows", set);
	assert(high_score_storage::read(loaded, "windows", get));
	assert(loaded[4].Score == 2000000000);
	std::cout << "High score persistence tests passed\n";
}
