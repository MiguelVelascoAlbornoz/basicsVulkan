#pragma once
#include <nlhomann/json.hpp>
using json = nlohmann::json;
#define SETTINGS_DIRECTORY std::string("settings")
class Settings
{
public:
	std::unordered_map<std::string, void*> settingsMap;
	void loadSettings(std::string name, std::vector<std::pair<void *, std::string>> *variables);
	std::vector<std::pair<void*,std::string>> variables;
	std::vector<std::string> variablesName;
	std::string fileName;
	void saveSettings();


};

