// ConsoleApplication1.cpp: 定义控制台应用程序的入口点。
//
//
#include "Test.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"Infrastructure.lib")

//#include "Test.h"
#include <iostream>
using namespace std;

static u_char *hky_prefix;

int main()
{
	hky_log_t *log;
	hky_debug_init();
	if (hky_strerror_init() != HKY_OK) {
		return 1;
	}
	hky_time_init();

	hky_pid = hky_getpid();
	//hky_parent = hky_getppid();

	log = hky_log_init(hky_prefix);
	if (log == NULL) {
		return 1;
	}

	hky_log_debug2(HKY_LOG_DEBUG_CORE, log, 0,
		"tree name %uz:\"%s\"", 1, "123");

	int a;
	cout << "123" << endl;
	cin >> a;
    return 0;
}

