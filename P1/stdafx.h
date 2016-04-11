// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include "targetver.h"
#include <tchar.h>
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <iostream>
#include <sstream>
#include "SenderSocket.h"
#include <iomanip>
#include "Checksum.h"


using namespace std;




// TODO: reference additional headers your program requires here
