// Scheduler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WorkManager.h"
#include <conio.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sstream>


int main(int argc, char* argv[])
{
	if (argc == 3)
	{
		std::string sPort(argv[1]);
		std::string sFile(argv[2]);
		std::cout << sPort << ' ' << sFile << std::endl;
		
		std::ifstream in(sFile);
		std::streambuf *cinbuf = std::cin.rdbuf();
		std::cin.rdbuf(in.rdbuf());
		std::string line;

		WorkManager* mgr = new WorkManager(stoi(sPort));
		double a, b, e;
		std::stringstream ss;
		while (std::getline(std::cin, line))
		{
			ss << line;
			ss >> a; 
			ss >> b; 
			ss >> e;
			mgr->AddTask(a, b, e);
			ss.clear();
		}

		while (true)
		{
			char c;
			c = _getch();
			if (c == 13)
			{
				delete mgr;
				return 0;
			}
		}
	}
	else
		std::cerr << "Bad startup parameters!" << std::endl;
	system("pause");
	
}
