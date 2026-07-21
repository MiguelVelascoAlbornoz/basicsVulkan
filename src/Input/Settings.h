#pragma once
#include <nlhomann/json.hpp>
using json = nlohmann::json;
#define SETTINGS_DIRECTORY std::string("settings")
class Settings
{
public:

	Settings(std::string name, std::vector<std::pair<void *, std::string>> variables);
	const std::vector<std::pair<void *, std::string>> variablesName;
	const std::string fileName;
	void saveSettings();


};

