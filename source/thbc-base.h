#if !defined(THBC_BASE)
#define THBC_BASE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <errno.h>

#include <raylib.h>
#include <cframe.h>
#include "qrcodegen.h"

#include <ifaddrs.h>
#include <net/if.h>

#define THBC_LONGEST_RESOURCE_NAME 1024

#define internal        static
#define local_persist   static
#define global          static

#endif // THBC_BASE