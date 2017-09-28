// Worker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Work.h"
#include <conio.h>
#include <iostream>


int main(int argc, char* argv[])
{
	if (argc == 3)
	{
		std::string sIP(argv[1]);
		std::string sPort(argv[2]);
		std::cout << sIP << ' ' << sPort << std::endl;
		Work* w = new Work(sIP, stoi(sPort));
		char c;
		c = _getch();
		if (c == 13)
		{
			delete w;
			return 0;
		}
	}
	else
		std::cerr << "Bad startup parameters!" << std::endl;
	system("pause");
}

