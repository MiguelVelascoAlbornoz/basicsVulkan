#include "Settings.h"

#include <fstream>
#include <iostream>
#include <utility>
#include <filesystem>


/**
 * @brief Carga un archivo .json dado con el nombre dado.
 * @details Primero busca y carga el archivo .json en la carpeta settings con el nombre dado.
 * Una vez cargado el .json itera en cada elemento de variable preguntado la primera letra del nombre para saber el tipo. Una vez sabido el tipo de dada variable busca en el .json si la variable esta y carga ese valor
 * en la direccion de memoria dada, caso no este en el .json cargara un valor por defecto (normalmente 0).
 * @param variables Vector que contiene la siguiente informacion: <nombre en el archivo .json de el field cargar>:<pointer de variable en el programa en el que se deve escribir tal field del json>
 * **/
void Settings::loadSettings(std::string name,std::vector<std::pair<void*,std::string>>* variables)
{
	this->variables = *variables;
	this->fileName = std::move(name);
	json data;
	std::ifstream inputFile(SETTINGS_DIRECTORY +"/"+name+".json");
	if (!inputFile.is_open()) {
		std::cout << "Error in loadSettings(): Problem while trying to load settings \"" << name << "\"." << std::endl;
		return;
	}
	inputFile >> data;
	inputFile.close();
	for (const auto& variable : *variables) {
		std::string variableName = variable.second;
		char firstLetter = variableName.at(0);
		json variableData = data[variableName];
		//Comienza por f
		if (firstLetter == 'f') {
			//
			auto* objectPointer = static_cast<float*>(variable.first);
			if (variableData.is_null()) {
				(*objectPointer) = 0;
				continue;
			}
			(*objectPointer) = variableData.get<float>();

		}else
		if (firstLetter == 'i') {
			auto* objectPointer = static_cast<int*>(variable.first);
			if (variableData.is_null()) {
				(*objectPointer) = 0;
				continue;
			}
			(*objectPointer) = variableData.get<int>();

		}else
		if (firstLetter == 'b') {
			auto* objectPointer = static_cast<bool*>(variable.first);
			if (variableData.is_null()) {
				(*objectPointer) = false;
				continue;
			}
			(*objectPointer) = variableData.get<bool>();
		}
		else {
			std::cout << "Error in loadSettings(): .json \"" << name << "\" have invalid field: \"" << variableName << "\"." << std::endl;
		}

	}
}

void Settings::saveSettings()
{
	json data;
	
	for (const auto&[variableP, variableName] : variables) {

		char firstLetter = variableName.at(0);
		if (firstLetter == 'i') {
			data[variableName] = *(static_cast<int*>(variableP));
		}
		else if (firstLetter == 'b') {
			data[variableName] = *(static_cast<bool*>(variableP));
		}
		else {
			data[variableName] = *(static_cast<float*>(variableP));
		}
		
	}

	try {
		std::filesystem::create_directory(SETTINGS_DIRECTORY);
	} catch (const std::filesystem::filesystem_error& e) {
		std::cerr << "Error in saveSettings(): " << e.what() << "\n";
		std::cerr << "Código: " << e.code().message() << "\n";
	}


	std::ofstream outputFile(SETTINGS_DIRECTORY+"/"+fileName+".json");
	outputFile << data.dump(4);  // Guardar con indentación para hacer el archivo legible
	outputFile.close();
}

