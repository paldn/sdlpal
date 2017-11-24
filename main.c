//
// Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
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
#include "main.h"
#include "getopt.h"
#ifdef NDS
#include "fat.h"
#endif

#ifdef PSP
#include "main_PSP.h"
#endif

#define BITMAPNUM_SPLASH_UP         0x26
#define BITMAPNUM_SPLASH_DOWN       0x27
#define SPRITENUM_SPLASH_TITLE      0x47
#define SPRITENUM_SPLASH_CRANE      0x49
#define NUM_RIX_TITLE               0x5


static VOID
PAL_Init(
   WORD             wScreenWidth,
   WORD             wScreenHeight,
   BOOL             fFullScreen
)
/*++
  Purpose:

    Initialize everything needed by the game.初始化游戏需要的所有东东

  Parameters:

    [IN]  wScreenWidth - width of the screen.

    [IN]  wScreenHeight - height of the screen.

    [IN]  fFullScreen - TRUE to use full screen mode, FALSE to use windowed mode.

  Return value:

    None.

--*/
{
   int           e;

#ifdef NDS
   fatInitDefault();
#endif

   //
   // Initialize defaults, video and audio
   //
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK) == -1)
   {
#if defined (_WIN32) && SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
      //
      // Try the WINDIB driver if DirectX failed.
      //
      putenv("SDL_VIDEODRIVER=windib");
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK) == -1)
      {
         TerminateOnError("Could not initialize SDL: %s.\n", SDL_GetError());
      }
#else
      TerminateOnError("Could not initialize SDL: %s.\n", SDL_GetError());
#endif
   }

   //
   // Initialize subsystems.
   //
   e = VIDEO_Init(wScreenWidth, wScreenHeight, fFullScreen);
   if (e != 0)
   {
      TerminateOnError("Could not initialize Video: %d.\n", e);
   }

   SDL_WM_SetCaption("Loading...", NULL);

   // 初始化全局数据,gpGlobals的各项数据文件指针被赋值
   e = PAL_InitGlobals();
   if (e != 0)
   {
      TerminateOnError("Could not initialize global data: %d.\n", e);
   }

   // 字体
   e = PAL_InitFont();
   if (e != 0)
   {
      TerminateOnError("Could not load fonts: %d.\n", e);
   }

   // ui
   e = PAL_InitUI();
   if (e != 0)
   {
      TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
   }

   // 文本和对话内容，由g_TextLib管理
   e = PAL_InitText();
   if (e != 0)
   {
      TerminateOnError("Could not initialize text subsystem: %d.\n", e);
   }

   PAL_InitInput();
   // gpResources人物，地图，eventObject精灵
   PAL_InitResources();
   SOUND_OpenAudio();

#ifdef _DEBUG
   SDL_WM_SetCaption("Pal (Debug Build)", NULL);
#else
   SDL_WM_SetCaption("Pal", NULL);
#endif
}

VOID
PAL_Shutdown(
   VOID
)
/*++
  Purpose:

    Free everything needed by the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
//   SOUND_CloseAudio();
   PAL_FreeFont();
   PAL_FreeResources();
   PAL_FreeGlobals();
   PAL_FreeUI();
   PAL_FreeText();
   PAL_ShutdownInput();
   VIDEO_Shutdown();

   UTIL_CloseLog();

   SDL_Quit();
}

VOID
PAL_TrademarkScreen(
   VOID
)
/*++
  Purpose:

    Show the trademark screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_SetPalette(3, FALSE);
   PAL_RNGPlay(6, 0, 1000, 25);
   UTIL_Delay(1000);
   PAL_FadeOut(1);
}

VOID
PAL_SplashScreen(
   VOID
)
/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_Color     *palette = PAL_GetPalette(1, FALSE);
   SDL_Color      rgCurrentPalette[256];
   SDL_Surface   *lpBitmapDown, *lpBitmapUp;
   SDL_Rect       srcrect, dstrect;
   LPSPRITE       lpSpriteCrane;
   LPBITMAPRLE    lpBitmapTitle;
   LPBYTE         buf, buf2;
   int            cranepos[9][3], i, iImgPos = 200, iCraneFrame = 0, iTitleHeight;
   DWORD          dwTime, dwBeginTime;
   BOOL           fUseCD = TRUE;
   //FILE * f;本人2016-11-27注释
   //INT  uiChunkCount;本人2016-11-27注释

   if (palette == NULL)
   {
      fprintf(stderr, "ERROR: PAL_SplashScreen(): palette == NULL\n");
      return;
   }

   //
   // Allocate all the needed memory at once for simplification
   //
   buf = (LPBYTE)UTIL_calloc(1, 320 * 200 * 2);
   buf2 = (LPBYTE)(buf + 320 * 200);
   lpSpriteCrane = (LPSPRITE)buf2 + 32000;

   //
   // Create the surfaces
   //
   lpBitmapDown = SDL_CreateRGBSurface(gpScreen->flags, 320, 200, 8,
      gpScreen->format->Rmask, gpScreen->format->Gmask, gpScreen->format->Bmask,
      gpScreen->format->Amask);
   lpBitmapUp = SDL_CreateRGBSurface(gpScreen->flags, 320, 200, 8,
      gpScreen->format->Rmask, gpScreen->format->Gmask, gpScreen->format->Bmask,
      gpScreen->format->Amask);

   //
   // Read the bitmaps
   //
   //f = UTIL_OpenRequiredFile("init.mkf");本人2016-11-27注释
   //uiChunkCount = PAL_MKFGetChunkCount(f);本人2016-11-27注释

   gpGlobals->f.fpMGO = UTIL_OpenRequiredFile("mgo.mkf");
   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_UP, gpGlobals->f.fpFBP);
   DecodeYJ1(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapUp);
   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_DOWN, gpGlobals->f.fpFBP);
   DecodeYJ1(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapDown);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_TITLE, gpGlobals->f.fpMGO);
   DecodeYJ1(buf, buf2, 32000);
   lpBitmapTitle = (LPBITMAPRLE)PAL_SpriteGetFrame(buf2, 0);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_CRANE, gpGlobals->f.fpMGO);
   DecodeYJ1(buf, lpSpriteCrane, 32000);

   iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
   lpBitmapTitle[2] = 0;
   lpBitmapTitle[3] = 0; // HACKHACK

   UTIL_CloseFile(gpGlobals->f.fpMGO);
   //
   // Generate the positions of the cranes
   //
   // 渲染9只鹤
   for (i = 0; i < 9; i++)
   {
	  // x坐标取值根据surface->format.w = 320, 然后递减该值，造成鹤从右往左飞行的效果
      cranepos[i][0] = RandomLong(300, 600);
	  // surface->foramt.h = 200, iImgPos从200开始递减，值为奇数时鹤的y值加1,总共加100,y的最大值为180，正好没超过高度
      cranepos[i][1] = RandomLong(0, 80);
	  // 仙鹤的动画帧序列号
      cranepos[i][2] = RandomLong(0, 8);
   }

   //
   // Play the title music
   //
   if (!SOUND_PlayCDA(7))
   {
      fUseCD = FALSE;
      PAL_PlayMUS(NUM_RIX_TITLE, TRUE, 2);
   }

   //
   // Clear all of the events and key states
   //
   PAL_ProcessEvent();
   PAL_ClearKeyState();

   dwBeginTime = SDL_GetTicks();

   srcrect.x = 0;
   srcrect.w = 320;
   dstrect.x = 0;
   dstrect.w = 320;

   while (TRUE)
   {
      PAL_ProcessEvent();
      dwTime = SDL_GetTicks() - dwBeginTime;

      //
      // Set the palette
      //
      if (dwTime < 15000)
      {
         for (i = 0; i < 256; i++)
         {
            rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
            rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
            rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
         }
      }

      VIDEO_SetPalette(rgCurrentPalette);

      //
      // Draw the screen
      //
      if (iImgPos > 1)
      {
         iImgPos--;
      }

      //
      // The upper part...
      //
      srcrect.y = iImgPos;
      srcrect.h = 200 - iImgPos;

      dstrect.y = 0;
      dstrect.h = srcrect.h;

      SDL_BlitSurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

      //
      // The lower part...
      //
      srcrect.y = 0;
      srcrect.h = iImgPos;

      dstrect.y = 200 - iImgPos;
      dstrect.h = srcrect.h;

      SDL_BlitSurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);

      //
      // Draw the cranes...
      //
      for (i = 0; i < 9; i++)
      {
         LPCBITMAPRLE lpFrame = PAL_SpriteGetFrame(lpSpriteCrane,
           cranepos[i][2] = (cranepos[i][2] + (iCraneFrame & 1)) % 8); // 更新动画：奇数帧的时候动画序列号+1
         cranepos[i][1] += ((iImgPos > 1) && (iImgPos & 1)) ? 1 : 0;   // 更新y坐标：iImgPos为奇数的时候y值+1
         PAL_RLEBlitToSurface(lpFrame, gpScreen,
            PAL_XY(cranepos[i][0], cranepos[i][1]));
         cranepos[i][0]--; // 更新x坐标
      }
      iCraneFrame++;

      //
      // Draw the title...
      //
      if (PAL_RLEGetHeight(lpBitmapTitle) < iTitleHeight)
      {
         //
         // HACKHACK
         //
         WORD w = lpBitmapTitle[2] | (lpBitmapTitle[3] << 8);
         w++;
         lpBitmapTitle[2] = (w & 0xFF);
         lpBitmapTitle[3] = (w >> 8);
      }

      PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

	  VIDEO_UpdateScreen(NULL);

      //
      // Check for keypress...
      //
      if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
      {
         //
         // User has pressed a key...
         //
         lpBitmapTitle[2] = iTitleHeight & 0xFF;
         lpBitmapTitle[3] = iTitleHeight >> 8; // HACKHACK

         PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

         VIDEO_UpdateScreen(NULL);

         if (dwTime < 15000)
         {
            //
            // If the picture has not completed fading in, complete the rest
            //
            while (dwTime < 15000)
            {
               for (i = 0; i < 256; i++)
               {
                  rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
                  rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
                  rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
               }
               VIDEO_SetPalette(rgCurrentPalette);
               UTIL_Delay(8);
               dwTime += 250;
            }
            UTIL_Delay(500);
         }

         //
         // Quit the splash screen
         //
         break;
      }

      //
      // Delay a while...
      //
      PAL_ProcessEvent();
      while (SDL_GetTicks() - dwBeginTime < dwTime + 85)
      {
         SDL_Delay(1);
         PAL_ProcessEvent();
      }
   }

   SDL_FreeSurface(lpBitmapDown);
   SDL_FreeSurface(lpBitmapUp);
   free(buf);

   if (!fUseCD)
   {
      PAL_PlayMUS(0, FALSE, 1);
   }

   PAL_FadeOut(1);
}

int
main(
   int      argc,
   char    *argv[]
)
/*++
  Purpose:

    Program entry.

  Parameters:

    argc - Number of arguments.

    argv - Array of arguments.

  Return value:

    Integer value.

--*/
{
   WORD          wScreenWidth = 0, wScreenHeight = 0;
   int           c;
   BOOL          fFullScreen = FALSE;

   UTIL_OpenLog();

#ifdef _WIN32
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
   putenv("SDL_VIDEODRIVER=directx");
#else
   putenv("SDL_VIDEODRIVER=win32");
#endif
#endif

#ifndef __SYMBIAN32__
   //
   // Parse parameters.
   //
   while ((c = getopt(argc, argv, "w:h:fjm")) != -1)
   {
      switch (c)
      {
      case 'w':
         //
         // Set the width of the screen
         //
         wScreenWidth = atoi(optarg);
         if (wScreenHeight == 0)
         {
            wScreenHeight = wScreenWidth * 200 / 320;
         }
         break;

      case 'h':
         //
         // Set the height of the screen
         //
         wScreenHeight = atoi(optarg);
         if (wScreenWidth == 0)
         {
            wScreenWidth = wScreenHeight * 320 / 200;
         }
         break;

      case 'f':
         //
         // Fullscreen Mode
         //
         fFullScreen = TRUE;
         break;

      case 'j':
         //
         // Disable joystick
         //
         g_fUseJoystick = FALSE;
         break;

#ifdef PAL_HAS_NATIVEMIDI
      case 'm':
         //
         // Use MIDI music
         //
         g_fUseMidi = TRUE;
         break;
#endif
      }
   }
#endif

   //
   // Default resolution is 640x400 (windowed) or 640x480 (fullscreen).
   //
   if (wScreenWidth == 0)
   {
#ifdef __SYMBIAN32__
#ifdef __S60_5X__
      wScreenWidth = 640;
      wScreenHeight = 360;
#else
      wScreenWidth = 320;
      wScreenHeight = 240;
#endif
#else
      wScreenWidth = 640;
      wScreenHeight = fFullScreen ? 480 : 400;
#endif
   }

   //
   // Initialize everything
   //
#ifdef PSP
   sdlpal_psp_init();
#endif
   // 初始化游戏需要的所有数据
   PAL_Init(wScreenWidth, wScreenHeight, fFullScreen);

   //
   // Show the trademark screen and splash screen
   // 显示图标和开场的渐变屏幕
   //
   PAL_TrademarkScreen();
   PAL_SplashScreen();

   //
   // Run the main game routine
   //
   PAL_GameMain();

   //
   // Should not really reach here...
   //
   assert(FALSE);
   return 255;
}
