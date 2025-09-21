#include <iostream>
#include <fstream>
#include <string>

int main()
{
	std::ifstream file("Test.rd");
	if (!file)
	{
		std::cerr << "Could not open test.rd\n";
		return 1;
	}
	std::string line;
	while (std::getline(file, line))
	{
		std::cout << line << "\n";
	}
	return 0;
}
