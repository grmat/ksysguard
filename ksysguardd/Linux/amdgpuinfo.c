/*
    KSysGuard, the KDE System Guard

    Copyright (c) 2017 grmat <grmat@sub.red>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "Command.h"
#include "ksysguardd.h"

#include "amdgpuinfo.h"

#define PREFIX_DRM  "/sys/class/drm"       /* sysfs drm path */
#define F_SCLK      "device/pp_dpm_sclk"   /* shader (gpu core) clock */
#define F_MCLK      "device/pp_dpm_mclk"   /* memory clock */
#define F_SOD       "device/pp_sclk_od"    /* shader overdrive (overclock) */
#define F_MOD       "device/pp_mclk_od"    /* memory overdrive */

/* holds values for one GPU */
struct amdgpu_device
{
    int sdpm;   /* GPU core dpm state */
    int sclk;   /* GPU core clock */
    int mdpm;   /* graphics memory dpm state */
    int mclk;   /* graphics memory clock */
    int sod;    /* GPU overdrive value */
    int mod;    /* graphics memory overdrive value */
};

static int amdGPUInfoOK = 0;
static struct amdgpu_device device;

static int Dirty = 0;
static struct SensorModul *AmdGpuInfoSM;

int readDpmInfo(const char* filepath, int* dpm, int* clk)
{
    FILE * fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filepath, "r");
    if (fp == NULL) return -1;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        *dpm = 0;
        *clk = 0;
        char active = ' ';
        sscanf(line, "%d: %dMhz %c", dpm, clk, &active);
        if (active == '*')
        {
            fclose(fp);
            free(line);
            return 0;
        }
    }

    fclose(fp);
    if (line) free(line);
    return -1;
}

int readOdInfo(const char* filepath, int* od)
{
    FILE * fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filepath, "r");
    if (fp == NULL) return -1;

    read = getline(&line, &len, fp);
    *od = 0;
    sscanf(line, "%d", od);

    fclose(fp);
    if (line) free(line);
    return 0;
}

int processGpuInfo(int id)
{
    int res = 0;
    char* filepath;

    struct amdgpu_device dev = {0, 0, 0, 0, 0, 0};

    /* get sdpm, sclk */
    asprintf(&filepath, "%s/card%d/%s", PREFIX_DRM, id, F_SCLK);
    res = readDpmInfo(filepath, &device.sdpm, &device.sclk);
    free(filepath);
    if (res < 0) return res;
    registerMonitor("amdgpu/card0/sClk", "integer", printGPUClock, printGPUClockInfo, AmdGpuInfoSM);
    registerMonitor("amdgpu/card0/sDpm", "integer", printDpmState, printDpmStateInfo, AmdGpuInfoSM);

    /* get mdpm, mclk */
    asprintf(&filepath, "%s/card%d/%s", PREFIX_DRM, id, F_MCLK);
    res = readDpmInfo(filepath, &device.mdpm, &device.mclk);
    free(filepath);
    if (res < 0) return res;
    registerMonitor("amdgpu/card0/mClk", "integer", printGPUMemoryClock, printGPUMemoryClockInfo, AmdGpuInfoSM);
    registerMonitor("amdgpu/card0/mDpm", "integer", printMemoryDpmState, printMemoryDpmStateInfo, AmdGpuInfoSM);

    /* get sod */
    asprintf(&filepath, "%s/card%d/%s", PREFIX_DRM, id, F_SOD);
    res = readOdInfo(filepath, &device.sod);
    free(filepath);
    if (res < 0) return res;
    registerMonitor("amdgpu/card0/sOd", "integer", printOd, printOdInfo, AmdGpuInfoSM);

    /* get mod */
    asprintf(&filepath, "%s/card%d/%s", PREFIX_DRM, id, F_MOD);
    res = readOdInfo(filepath, &device.mod);
    free(filepath);
    if (res < 0) return res;
    registerMonitor("amdgpu/card0/mOd", "integer", printMemoryOd, printMemoryOdInfo, AmdGpuInfoSM);

    Dirty = 0;

    /* TODO: adjust clocks with overdrive values */
    return res;
}

/*
================================ public part =================================
*/

void initAmdGpuInfo(struct SensorModul* sm)
{
    AmdGpuInfoSM = sm;

    if (updateAmdGpuInfo() < 0)
        return;

    /* TODO: init amdgpu devices (count etc.) */
    struct amdgpu_device dev = {0, 0, 0, 0, 0, 0};
    device = dev;
    registerMonitor("amdgpu/gpus", "integer", printNumGpus, printNumGpusInfo, AmdGpuInfoSM);
}

void exitAmdGpuInfo(void)
{
    amdGPUInfoOK = -1;
}

int updateAmdGpuInfo(void)
{
    int res = 0;
    res = processGpuInfo(0);
    return res;
}

void printGPUClock(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.sclk);
}

void printGPUClockInfo(const char* cmd)
{
    cmd = cmd; /*Silence warning*/
    output("GPU Clock Frequency\t0\t0\tMHz\n");
}

void printGPUMemoryClock(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.mclk);
}

void printGPUMemoryClockInfo(const char* cmd)
{
    cmd = cmd;
    output("GPU Memory Frequency\t0\t0\tMHz\n");
}

void printDpmState(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.sdpm);
}
void printDpmStateInfo(const char* cmd)
{
    cmd = cmd;
    output("GPU DPM state\t0\t0\t\n");
}

void printMemoryDpmState(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.mdpm);
}
void printMemoryDpmStateInfo(const char* cmd)
{
    cmd = cmd;
    output("Memory DPM state\t0\t0\t\n");
}

void printOd(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.sod);
}
void printOdInfo(const char* cmd)
{
    cmd = cmd;
    output("GPU overdrive\t0\t0\t%\n");
}

void printMemoryOd(const char* cmd)
{
    if (Dirty)
        processGpuInfo(0);

    output("%d\n", device.mod);
}
void printMemoryOdInfo(const char* cmd)
{
    cmd = cmd;
    output("Memory overdrive\t0\t0\t%\n");
}

void printNumGpus(const char* cmd)
{
    (void) cmd;

    output("%d\n", 1); /* TODO */
}

void printNumGpusInfo(const char* cmd)
{
    (void) cmd;

    output("Number of physical GPUs\t0\t0\t\n");
}
