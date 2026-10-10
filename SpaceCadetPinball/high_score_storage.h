#pragma once

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <stdexcept>

struct high_score_struct
{
	char Name[32];
	int Score;
};

// Settings namespaces are stable persistence identifiers, not display names.
// Reserve "custom.<preset-id>" for future presets; never alias historical modes.
namespace high_score_storage
{
	inline const char* profile(bool fullTilt, bool demo)
	{
		return demo ? "full-tilt-demo" : fullTilt ? "full-tilt" : "windows";
	}

	inline std::string prefix(const std::string& profileId)
	{
		return profileId.empty() ? "" : "HighScores." + profileId + ".";
	}

	inline void clear(high_score_struct (&table)[5])
	{
		for (auto& entry : table)
		{
			entry = {};
			entry.Score = -999;
		}
	}

	inline uint32_t checksum(const high_score_struct (&table)[5])
	{
		uint32_t sum = 0;
		for (const auto& entry : table)
		{
			for (const char* ch = entry.Name; *ch; ++ch)
				sum += static_cast<int8_t>(*ch);
			sum += static_cast<uint32_t>(entry.Score);
		}
		return sum;
	}

	inline int64_t parse(const std::string& value)
	{
		size_t end;
		auto number = std::stoll(value, &end);
		if (end != value.size())
			throw std::invalid_argument("Invalid score setting");
		return number;
	}

	template <typename Get>
	bool read(high_score_struct (&table)[5], const std::string& profileId, Get get)
	{
		clear(table);
		const auto base = prefix(profileId);
		try
		{
			for (int index = 0; index < 5; ++index)
			{
				auto key = base + std::to_string(index);
				auto name = get(key + ".Name", "");
				if (name.size() >= sizeof table[index].Name || name.find('\0') != std::string::npos)
					throw std::invalid_argument("Invalid score name");
				std::memcpy(table[index].Name, name.c_str(), name.size() + 1);
				auto score = parse(get(key + ".Score", "-999"));
				if (score < std::numeric_limits<int>::min() || score > std::numeric_limits<int>::max())
					throw std::out_of_range("Score out of range");
				table[index].Score = static_cast<int>(score);
			}
			auto verification = parse(get(base + "Verification", "7"));
			if (verification < std::numeric_limits<int32_t>::min() || verification > std::numeric_limits<uint32_t>::max()
				|| static_cast<uint32_t>(verification) != checksum(table))
				throw std::invalid_argument("Score checksum mismatch");
			return true;
		}
		catch (const std::invalid_argument&) {}
		catch (const std::out_of_range&) {}
		clear(table);
		return false;
	}

	template <typename Set>
	void write(const high_score_struct (&table)[5], const std::string& profileId, Set set)
	{
		// The empty namespace is the original, unassigned archive: read-only.
		if (profileId.empty())
			return;
		const auto base = prefix(profileId);
		for (int index = 0; index < 5; ++index)
		{
			auto key = base + std::to_string(index);
			set(key + ".Name", std::string(table[index].Name));
			set(key + ".Score", std::to_string(table[index].Score));
		}
		set(base + "Verification", std::to_string(checksum(table)));
	}
}
