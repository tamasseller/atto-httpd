/*
 * TestMain.cpp
 *
 *  Created on: 2017.02.06.
 *      Author: tooma
 */

#include "CppUTest/CommandLineTestRunner.h"

int main(int argc, char *argv[])
{
    MemoryLeakWarningPlugin::destroyGlobalDetector();
	return CommandLineTestRunner::RunAllTests(argc, argv);
}


