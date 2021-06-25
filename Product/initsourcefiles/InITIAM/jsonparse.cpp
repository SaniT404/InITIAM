#include "jsonparse.h"

JSONParser::JSONParser()
	: result()
{

}

JSONParser::JSONParser(std::string json)
{
	this->parse(json);
}

std::map<std::string, std::string> JSONParser::getResult() const
{
	return this->result;
}

std::string JSONParser::str()
{
	std::stringstream results;
	std::map<std::string, std::string>::iterator it;
	for (it = this->result.begin(); it != this->result.end(); it++)
	{
		results << it->first << " => " << it->second << "\n";
	}
	return results.str();
}

// Returns 1 if invalid JSON string, and 0 if completed successfully
// NOTE:  whitespace in between keys and values not allowed;
bool JSONParser::parse(std::string json)
{
	if (!this->result.empty())
		this->result.clear();

	bool on_array = false;
	bool on_json = false; //true

	bool on_key = false;
	std::string key;
	int key_start;
	int key_end;

	bool on_value = false;
	bool is_string;
	std::string value;
	int value_start;
	int value_end;
	
	int current_level = 0;

	for (int i = 0; i < json.size(); i++) // i = 2
	{
		// Check if multidimensional
		if (json[i] == '[' && on_array)
		{
			return false;
		}
		if (json[i] == '{' && on_json)
		{
			return false;
		}
		// Set flags to ensure single dimensional json string
		if (json[i] == '[')
		{
			on_array = true;
		}
		if (json[i] == '{')
		{
			on_json = true;
		}

		// Get key and insert into result
		if (json[i] == '\"' && !on_key && !on_value)
		{
			key_start = i + 1; // key_start = 2
			on_key = true;
		}
		else if (json[i] == '\"' && on_key)
		{
			key_end = i;

			int len = key_end - key_start;

			key = std::string(json, key_start, len);

			on_key = false;
			on_value = true;
		}

		// Use on_key flag to determine value and reset when done
		if (json[i] == ':')
		{	
			if (json[i + 1] == '\"')
			{
				value_start = i + 2;
				is_string = true;
			}
			else
			{
				value_start = i + 1;
				is_string = false;
			}
		}
		if (on_value && (json[i] == ',' || json[i] == '}'))
		{
			if (is_string)
			{
				value_end = i - 1;
				on_value = false;
			}
			else
			{
				on_value = false;
				value_end = i;
			}

			int len = value_end - value_start;
			
			value = std::string(json, value_start, len);
			// Insert key and value to result map
			std::pair<std::string, std::string> newpair(key, value);
			this->result.insert(newpair);
		}
	}

	return true;
}