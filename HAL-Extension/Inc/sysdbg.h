/*
 * sysdbg.h
 *
 *  Created on: Mar 21, 2019
 *      Author: Andrew
 */

#ifndef SYSDBG_H_
#define SYSDBG_H_

#ifdef __cplusplus

#include <cstdio>
#include <cstddef>
extern "C" {

#else

#include <stdio.h>
#include <stddef.h>

#endif


typedef struct {
	size_t freeHeapSizeNow;
	size_t freeHeapSizeMin;
}_SysMemState;

void _SysMemCheckpoint(_SysMemState *);
int _SysMemDifference(_SysMemState *, _SysMemState *, _SysMemState *);
void _SysMemDumpStatistics(_SysMemState *);

#ifdef __cplusplus
}
#endif /* extern "C" */


#ifdef __cplusplus
#define _SysCheckMemory() SysCheckMemory(__FILE__, __LINE__, __func__)
struct SysCheckMemory
{
	_SysMemState state1;
	_SysMemState state2;
	_SysMemState state3;
	SysCheckMemory()
	{
		_SysMemCheckpoint(&state1);
	}

	SysCheckMemory(const char* file, size_t line, const char *func)
	{
		filename = file;
		lineNumber = line;
		funcName = func;

		_SysMemCheckpoint(&state1);
	}

	~SysCheckMemory()
	{
		_SysMemCheckpoint(&state2);

		if( _SysMemDifference( &state3, &state1, &state2) ) {
			std::printf("*** Error - Heap Memory Leak Detected ***\n");
			std::printf("File: %s\nFunc: %s\nLine: %u\n", filename, funcName, lineNumber);
			_SysMemDumpStatistics( &state3 );
		}
	}

	const char *filename;
	size_t lineNumber;
	const char *funcName;
};
#endif /* SysCheckMemory */


#endif /* SYSDBG_H_ */
