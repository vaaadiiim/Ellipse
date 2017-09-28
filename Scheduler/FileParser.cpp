#include "stdafx.h"
#include "FileParser.h"


FileParser::FileParser(string sFilePath)
{
	sFileName = sFilePath;
	finput.open(sFileName);
	if (!finput.is_open())
		throw 1;
}


FileParser::~FileParser()
{
	finput.close();
}

bool FileParser::GetEllipseInfo(double* pdA, double* pdB, double* pdEps)
{
	std::wstring sLine = L"";
	char c;
	int iNumItems = 0;
	while (finput.get(c))
	{
		if (c == '\r' || c == '\n')
		{
			*pdEps = stod(sLine.c_str());
			return true;
		}
		else if (c == ' ')
		{
			switch (iNumItems)
			{
				case 0:
					*pdA = stod(sLine.c_str());
					iNumItems++;
					break;
				case 1:
					*pdB = stod(sLine.c_str());
					iNumItems++;
					break;
			}
			sLine = L"";
		}
		else
			sLine += c;
	}
	if (iNumItems > 0)
	{
		if (sLine != L"")
			*pdEps = stod(sLine);
		return true;
	}	
	return false;

}
