#include <fstream>
#include <sstream>
#include <iostream>

std::string ReadFile(const std::string &filename)
{
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Error: Could not open file '" << filename << "'\n";
		return "";
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

int main()
{
	std::string source = ReadFile("test.rd");
	if (source.empty())
	{
		std::cerr << "Error: file is empty or could not be read.\n";
		return 1; 
	}

	std::cout << source;
	return 0;
}
