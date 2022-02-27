#pragma once

#include <windows.h>
#include <stdio.h>

#include <string>
#include <memory>
#include <cassert>
#include <numeric>

#define LOGINFO(format, ...) fprintf(stdout, "[INFO]" format, ##__VA_ARGS__)
#define LOGERROR(format, ...) fprintf(stderr, "[ERROR]" format, ##__VA_ARGS__)
