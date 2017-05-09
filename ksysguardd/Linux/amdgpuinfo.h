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

#ifndef KSG_AMDGPUINFO_H
#define KSG_AMDGPUINFO_H

void initAmdGpuInfo(struct SensorModul*);
void exitAmdGpuInfo(void);

int updateAmdGpuInfo(void);

void printGPUClock(const char*);
void printGPUClockInfo(const char*);

void printGPUMemoryClock(const char*);
void printGPUMemoryClockInfo(const char*);

void printDpmState(const char*);
void printDpmStateInfo(const char*);

void printMemoryDpmState(const char*);
void printMemoryDpmStateInfo(const char*);

void printOd(const char*);
void printOdInfo(const char*);

void printMemoryOd(const char*);
void printMemoryOdInfo(const char*);

void printNumGpus(const char*);
void printNumGpusInfo(const char*);

#endif
