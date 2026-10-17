// Exercise the real high_score/options/ImGui persistence across separate processes.
// Reuse the platform file adapter while replacing only the executable entry point.
#define main unused_game_main
#include "../SpaceCadetPinball/SpaceCadetPinball.cpp"
#undef main
#include "../SpaceCadetPinball/high_score.h"
#include "../SpaceCadetPinball/options.h"
#include "../SpaceCadetPinball/pb.h"

static void select(bool fullTilt)
{
	pb::FullTiltMode = fullTilt;
	pb::FullTiltDemoMode = false;
	high_score::read();
}

static void insert(const char* name, int score)
{
	high_score_entry entry{};
	std::strcpy(entry.Entry.Name, name);
	entry.Entry.Score = score;
	entry.Position = high_score::get_score_position(score);
	assert(entry.Position == 0);
	high_score::highscore_table[entry.Position] = entry.Entry;
	high_score::write();
}

int main(int argc, char** argv)
{
	assert(argc == 3);
	ImGui::CreateContext();
	winmain::ImIO = &ImGui::GetIO();
	ImGui::GetIO().IniFilename = argv[1];
	options::InitPrimary();
	const std::string stage = argv[2];
	if (stage == "seed")
	{
		select(false);
		assert(high_score::highscore_table[0].Score == -999);
		insert("Windows player", 1000000);
		select(true);
		assert(high_score::highscore_table[0].Score == -999);
		insert("FT player", 2000000);
		select(false);
		assert(high_score::highscore_table[0].Score == 1000000);
		// Match the settings/high-score/context lifetime during an in-process restart.
		options::uninit();
		high_score::write();
		ImGui::DestroyContext();
		ImGui::CreateContext();
		winmain::ImIO = &ImGui::GetIO();
		ImGui::GetIO().IniFilename = argv[1];
		options::InitPrimary();
		select(true);
		assert(high_score::highscore_table[0].Score == 2000000);
		select(false);
		assert(high_score::highscore_table[0].Score == 1000000);
	}
	else if (stage == "clear-windows")
	{
		select(false);
		assert(high_score::highscore_table[0].Score == 1000000);
		assert(std::strcmp(high_score::highscore_table[0].Name, "Windows player") == 0);
		high_score_storage::clear(high_score::highscore_table);
		high_score::write();
		select(true);
		assert(high_score::highscore_table[0].Score == 2000000);
	}
	else
	{
		assert(stage == "verify");
		select(true);
		assert(high_score::highscore_table[0].Score == 2000000);
		assert(std::strcmp(high_score::highscore_table[0].Name, "FT player") == 0);
		select(false);
		assert(high_score::highscore_table[0].Score == -999);
	}
	assert(options::GetSetting("0.Name", "") == "Legacy player");
	assert(options::GetSetting("0.Score", "") == "3000000");
	assert(options::GetSetting("Verification", "") == "2997286");
	options::uninit();
	high_score::write();
	// The game also relies on context destruction to flush settings at shutdown.
	ImGui::DestroyContext();
	std::cout << "High score disk integration passed: " << stage << '\n';
}
