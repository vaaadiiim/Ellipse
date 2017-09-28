#include <string>
#include <fstream>

#pragma once

using namespace std;

class FileParser
{
private:
	string sFileName;
	ifstream finput;
public:
	FileParser(string sFilePath);
	~FileParser();
	bool GetEllipseInfo(double* pdA, double* pdB, double* pdEps);//return false mean end of file
};

