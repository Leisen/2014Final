/*==============================================================
  Test Basic Magician Sprite System on Fly2

  (C)2013-2014 Chuan-Chang Wang, All Rights Reserved
  Created : 1212, 2013

  Last Updated : 1212, 2013, Kevin C. Wang
 ===============================================================*/
#include "FlyWin32.h"

int frame = 0;

int width = 800;            // window width
int height = 600;            // window height
BOOL4 beFullScreen = FALSE;  // be full screen or not

VIEWPORTid vID;              // viewport
TEXTid textID;               // text ID

SCENEid sID2;                // the 2D scene
OBJECTid spID0 = FAILED_ID;  // the sprite for background, Graphic01.dds
OBJECTid spID1 = FAILED_ID;  // the sprite for background, Graphic02.dds
OBJECTid spID2 = FAILED_ID;  // the sprite for FSN_Become-13
OBJECTid spID3 = FAILED_ID;  // the sprite for testing

float uv[4];                 // the texture uv to display image on sprite spID2

void StartPlayAnim(BYTE, BOOL4);
void QuitGame(BYTE, BOOL4);
void RenderIt(int);
void PlayAnimation(int);

void HitTest(int, int);

/*------------------
  the main program
  C.Wang 1212, 2013
 -------------------*/
void FyMain(int argc, char **argv)
{
   // create a new world
   FyStartFlyWin32("Magician Sprite Test on Fly2 DX9", 0, 0, width, height, beFullScreen);

   FySetTexturePath("..\\..\\NTU5\\Textures");

   // create a viewport
   vID = FyCreateViewport(0, 0, width, height);
   FnViewport vp;
   vp.Object(vID);
   vp.SetBackgroundColor(0.5f, 0.2f, 0.2f);

   // create a 2D scene for sprite rendering which will be rendered on the top of 3D
   FnScene scene;
   sID2 = FyCreateScene(1);
   scene.ID(sID2);
   scene.SetSpriteWorldSize(width, height);         // 2D scene size in pixels

   FnSprite sp;
   spID0 = scene.CreateObject(SPRITE);
   sp.ID(spID0);
   sp.SetSize(400, 150);
   sp.SetImage("status", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
   sp.SetPosition(0, 256, 0);

   spID1 = scene.CreateObject(SPRITE);
   sp.ID(spID1);
   sp.SetSize(192, 170);
   sp.SetImage("test02", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
   sp.SetPosition(0, 0, 0);


   textID = FyCreateText("Trebuchet MS", 18, FALSE, FALSE);

   // set Hotkeys
   FyDefineHotKey(FY_F1, StartPlayAnim, FALSE);      // press F1 to play animation
   FyDefineHotKey(FY_F2, StartPlayAnim, FALSE);      // press F1 to play animation
   FyDefineHotKey(FY_ESCAPE, QuitGame, FALSE);      // escape for quiting the game

   // set hittest mosue callback
   FyBindMouseFunction(LEFT_MOUSE, HitTest, NULL, NULL, NULL);

   // bind timers, frame rate = 30 fps
   FyBindTimer(0, 30.0f, RenderIt, FALSE);
   FyBindTimer(1, 30.0f, PlayAnimation, TRUE);

   // invoke the system
   FyInvokeFly(TRUE);
}


/*-------------------
  play the animation
  C.Wang 1212, 2013
 --------------------*/
void PlayAnimation(int skip)
{
}


/*------------------
  renderer
  C.Wang 1212, 2013
 -------------------*/
void RenderIt(int skip)
{
   // render the 2D scene for sprites on the top of the 3D
   FnViewport vp;
   vp.Object(vID);
   vp.RenderSprites(sID2, TRUE, TRUE);  // no clear the background but clear the z buffer

   // show frame rate
   static char string[128];

   if (frame == 0) {
      FyTimerReset(0);
   }

   if (frame/10*10 == frame) {
      float curTime;

      curTime = FyTimerCheckTime(0);
      sprintf(string, "Fps: %6.2f", frame/curTime);
   }

   frame += skip;

   if (frame >= 1000) {
      frame = 0;
   }

   char text[128];
   FnText shufa(textID);
   shufa.Begin(vID);
   shufa.Write(string, 20, 40, 255, 0, 0);
   sprintf(text, "Free Video Memory %d MB : %d Textures", FyQuerySystem(VIDEO_MEMORY)/1024/1024, FyQuerySystem(TEXTURE_NUMBER_IN_VIDEO_MEMORY));
   shufa.Write(text, 20, 60, 255, 0, 0);
   shufa.End();

   // swap buffer
   FySwapBuffers();
}


/*------------------
  quit the demo
  C.Wang 1212, 2013
 -------------------*/
void QuitGame(BYTE code, BOOL4 value)
{
   if (code == FY_ESCAPE) {
      if (value) {
         FyQuitFlyWin32();
      }
   }
}


/*-------------------
  play the animation
  C.Wang 1212, 2013
 --------------------*/
void StartPlayAnim(BYTE code, BOOL4 value)
{
   if (code == FY_F1) {
      if (!value) {
         FnScene scene;
         scene.ID(sID2);

         // create the sprite to displying the animation
         FnSprite sp;
         spID2 = scene.CreateObject(SPRITE);
         sp.ID(spID2);

         // use the alpha channel
         sp.SetImage("..\\..\\NTU5\\Textures\\fruit_png", 0, NULL, FALSE, NULL, MANAGED_MEMORY, TRUE, FILTER_LINEAR);
         sp.SetSize(256, 256);

         // initialze the sprite position on viewport
         sp.SetPosition(256, 128, 1);   // z = 1 to make sure the animation is on the background
      }
   }
}


/*-------------------------------
  mouse callback for hit-testing
  C.Wang 0430, 2014
 --------------------------------*/
void HitTest(int x, int y)
{
   FnViewport vp;
   vp.ID(vID);
   OBJECTid sssID = vp.HitSprite(&spID2, 1, x, y);
   if (sssID == spID2) {
      int aaa;
      aaa = 1;
   }
}