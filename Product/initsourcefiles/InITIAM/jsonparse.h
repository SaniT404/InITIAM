// CREATED BY: Collin Walker
//
// DATE: 04/05/2017
//
// SUMMARY:  A C++ class designed to transform a json string into an interactable, single or multi-dimensional vector

#pragma once
#include <iostream>
#include <sstream>
#include <map>
#include <string>

class JSONParser
{
public:
	JSONParser();
	JSONParser(std::string json);

	std::map<std::string,std::string> getResult() const;
	std::string str();

	bool parse(std::string json);
private:
	std::map<std::string,std::string> result;
};