#pragma once 

#include <string>
#include <iostream>

namespace Debug {
	
	struct DebugOutput {

		DebugOutput() = default;
		
		static void outputDebug(const std::string& text)
		{
			{
				using namespace std;
				cout << text << '\n'; 
			}
		}

		static void outputDebug(const char* text)
		{
			{
				using namespace std;
				cout << text << '\n';
			}
		}

	};

}