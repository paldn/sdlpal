//
// Copyright (c) 2009, Pal_Bazzi.
//
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <psppower.h>
#include <pspthreadman.h>
#include <stdlib.h>
#include <stdio.h>

#define PSP_HEAP_MEMSIZE 12288

PSP_MODULE_INFO("SDLPAL",0,1,1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(PSP_HEAP_MEMSIZE);

int PSP_exit_callback(int arg1, int arg2, void *common)
{
	exit(0);
	return 0;
}

int PSP_exit_callback_thread(SceSize args, void *argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", PSP_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int PSP_exit_setup_callbacks(void)
{
	int thid = 0;
	thid = sceKernelCreateThread("update_thread", PSP_exit_callback_thread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
		sceKernelStartThread(thid, 0, 0);
	return thid;
}

void sdlpal_psp_init(void)
{
   //
   // need call??
   //
   pspDebugScreenInit();

   //
   // PSP set exit
   //
   PSP_exit_setup_callbacks();

   //
   // Register sceKernelExitGame() to be called when we exit 
   //
   atexit(sceKernelExitGame);

   //
   // set PSP CPU clock
   //
   scePowerSetClockFrequency(333 , 333 , 166);
   
   //sceKernelVolatileMemLock(0, );
}