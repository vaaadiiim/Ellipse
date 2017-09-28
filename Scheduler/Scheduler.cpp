// Scheduler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "WorkManager.h"
#include <conio.h>
#include "FileParser.h"
#include <iostream>


int main(int argc, char* argv[])
{
	if (argc == 3)
	{
		std::string sPort(argv[1]);
		std::string sFilePath(argv[2]);
		cout << sPort << ' ' << sFilePath << endl;
		WorkManager* mgr = new WorkManager(stoi(sPort));
		try
		{
			FileParser fpParser(sFilePath);
			bool bRead = true;
			while (bRead)
			{
				double a = 0;
				double b = 0;
				double eps = 0;
				bRead = fpParser.GetEllipseInfo(&a, &b, &eps);
				if (bRead)
					mgr->AddTask(a, b, eps);
			}
		}
		catch (int ex)
		{
			if (ex == 1)
			{
				std::cout << "Bad file path!" << std::endl;
			}
		}
		char c;
		c = _getch();
		if (c == 13)
		{
			delete mgr;
			return 0;
		}
	}
	else
		std::cout << "Bad startup parameters!" << std::endl;
	system("pause");
	
}
