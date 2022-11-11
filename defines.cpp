#include "device.hpp"

std::string uintToString(uint8_t v)
{
	std::string result;
	std::stringstream ss;

	ss << std::hex << (int)v;
	result = ss.str();

	while (2 - result.size() > 0)
	{
		result = "0" + result;
	}

	for (auto& c : result) c = toupper(c);

	return result;
}

std::string uintToString(uint32_t v)
{
	std::string result;
	std::stringstream ss;

	ss << std::hex << v;
	result = ss.str();

	while (6 - result.size() > 0) // Because the biggest number the CPU can handle are 24 bits long, so 6 bytes, even though uint32_t are 8 bytes long
	{
		result = "0" + result;
	}

	for (auto& c : result) c = toupper(c);

	return result;
}