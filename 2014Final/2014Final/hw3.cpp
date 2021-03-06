/*

	Note line 139, 149, 197, 218, 229, 434

*/

//#include "stdafx.h"
#include "FlyWin32.h"

/*	constant definition	*/

#define MOVE_SPEED_DEFAULT			3.5f
#define MOVE_SPEED_HALT				0.0f
#define MOVE_BLOCK_RANGE			50

#define CAMERA_DEFAULT_DISTANCE		800
#define CAMERA_DEFAULT_HEIGHT		400
#define CAMERA_CHARACTER_HEIGHT		45
#define CAMERA_PIVOT_SPEED			2*0.00872664625
#define CAMERA_ASCEND_SPEED			2

#define DUMMY_RANGE					25
#define DUMMY_CENTER_SPEED			2

#define ENEMY_NUMBERS				2
#define ENEMY_DEFAULT_HP			200

#define ATTACK1_DAMAGE				10
#define ATTACK1_ANGLE				15
#define ATTACK1_DISTANCE			120
#define ATTACK1_TIMING				5

#define ATTACK2_DAMAGE				10
#define ATTACK2_DISTANCE			150
#define ATTACK2_TIMING				20

#define ATTACK3_DAMAGE				20
#define ATTACK3_DISTANCE			120
#define ATTACK3_TIMING				25

#define ATTACK4_DAMAGE				20
#define ATTACK4_ANGLE				15
#define ATTACK4_DISTANCE			240
#define ATTACK4_TIMING				30

typedef	enum dirFlag{
	DIR_FORWARD,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_BACKWARD,
	DIR_HALT,
	DIR_INIT,
	CHAR_ATTACK1,
	CHAR_ATTACK2,
	CHAR_ATTACK3,
	CHAR_ATTACK4,
	CHAR_DAMAGED,
	CHAR_DEAD
}dirFlag;

typedef	enum attackAction{
	COMBAT_IDLE,
   	NORMAL_ATTACK_1,
   	NORMAL_ATTACK_2,
   	NORMAL_ATTACK_3,
   	NORMAL_ATTACK_4,
   	HEAVY_ATTACK_1,
   	HEAVY_ATTACK_2,
   	HEAVY_ATTACK_3,
   	ULTIMATE_ATTACK,
  	GUARD
}attackAction;

typedef struct Enemy{
	CHARACTERid eid;
	ACTIONid damaged, dead, idle;
	int HitPoint;
	int Flag;
};
struct Enemy enemy[ENEMY_NUMBERS];

/*	global variable declaration	*/

VIEWPORTid vID;                 // the major viewport
SCENEid sID;                    // the 3D scene
OBJECTid cameraID, tID, dummyID;              // the main camera and the terrain for terrain following
CHARACTERid actorID;            // the major character
ACTIONid idleID, runID, curPoseID, normalattack1, normalattack2, normalattack3, normalattack4;
ROOMid terrainRoomID = FAILED_ID;
TEXTid textID = FAILED_ID;

int DirectionFlag = DIR_INIT;
int ForwardFlag = DIR_FORWARD;
int CharacterFlag = DIR_HALT;
int PivotFlag = DIR_HALT;
int ActorFrameCounter = 0;

int frame = 0;

float PivotSin = (float)sin(CAMERA_PIVOT_SPEED);
float PivotCos = (float)cos(CAMERA_PIVOT_SPEED);

float move = 0.0f;

/*	function definition	*/
// hotkey callbacks
void QuitGame(BYTE, BOOL4);
void Movement(BYTE, BOOL4);
void Pivot(BYTE, BOOL4);

// timer callbacks
void GameAI(int);
void RenderIt(int);

//	camera function
int CameraFollow( FnObject dummy, FnCamera camera, float direction[] );
int CameraPivot( FnObject dummy, FnCamera camera, int direction );

//	dummy function
int DummyFollow( FnCharacter actor, FnObject dummy );
int DummyCenter( FnCharacter actor, FnObject dummy );
int DummyRange( const float dpos[3], const float pos[3] );

//	general function
int posdiff( const float pos1[], const float pos2[] );
void NormalizeVector3( float obj[3] );
void Cross( float tar[3], const float obj1[3], const float obj2[3]  );

int InAttackRange( const int attack, FnCharacter enemy );
int ActorRange(const float dpos[3], const float pos[3]);



GAMEFX_SYSTEMid gFXID = FAILED_ID;



void FyMain(int argc, char **argv){
	// create a new world
   BOOL4 beOK = FyStartFlyWin32("NTU@2014 Homework #01 - Use Fly2", 0, 0, 1024, 768, FALSE);

   // setup the data searching paths
   FySetShaderPath("..\\..\\NTU5\\Shaders");
   FySetModelPath("..\\..\\NTU5\\Scenes");
   FySetTexturePath("..\\..\\NTU5\\Scenes\\Textures");
   FySetScenePath("..\\..\\NTU5\\Scenes");
   FySetGameFXPath("..\\..\\NTU5\\FX\\FX0");

   // create a viewport
   vID = FyCreateViewport(0, 0, 1024, 768);
   FnViewport vp;
   vp.ID(vID);

   // create a 3D scene
   sID = FyCreateScene(10);
   FnScene scene;
   scene.ID(sID);

   // load the scene
   scene.Load("gameScene01");
   scene.SetAmbientLights(1.0f, 1.0f, 1.0f, 0.6f, 0.6f, 0.6f);

   // load the terrain
   tID = scene.CreateObject(OBJECT);
   FnObject terrain;
   terrain.ID(tID);
   BOOL beOK1 = terrain.Load("terrain");
   terrain.Show(FALSE);

   // set terrain environment
   terrainRoomID = scene.CreateRoom(SIMPLE_ROOM, 10);
   FnRoom room;
   room.ID(terrainRoomID);
   room.AddObject(tID);

   // load the character
   FySetModelPath("..\\..\\NTU5\\Characters");
   FySetTexturePath("..\\..\\NTU5\\Characters");
   FySetCharacterPath("..\\..\\NTU5\\Characters");
   actorID = scene.LoadCharacter("Lyubu2");

   // put the character on terrain
   float pos[3], fDir[3], uDir[3];
   FnCharacter actor;
   actor.ID(actorID);
   pos[0] = 3569.0f; pos[1] = -3208.0f; pos[2] = 1000.0f;
   fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
   uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
   actor.SetDirection(fDir, uDir);

   actor.SetTerrainRoom(terrainRoomID, 10.0f);
   beOK = actor.PutOnTerrain(pos);

   // Get two character actions pre-defined at Lyubu2
   idleID = actor.GetBodyAction(NULL, "Idle");
   runID = actor.GetBodyAction(NULL, "Run");
   normalattack1 = actor.GetBodyAction(NULL, "NormalAttack1");
   normalattack2 = actor.GetBodyAction(NULL, "NormalAttack2");
   normalattack3 = actor.GetBodyAction(NULL, "NormalAttack3");
   normalattack4 = actor.GetBodyAction(NULL, "NormalAttack4");
   
   /*
		add more character actions here
   */

   // set the character to idle action
   curPoseID = idleID;
   actor.SetCurrentAction(NULL, 0, curPoseID);
   actor.Play(START, 0.0f, FALSE, TRUE);
   actor.TurnRight(90.0f);

   // create dummy
   FnObject dummy;
   dummyID = scene.CreateObject(OBJECT);
   dummy.ID(dummyID);
   actor.GetPosition(pos);
   dummy.SetPosition(pos);
   actor.GetDirection(fDir,uDir);
   dummy.SetDirection(fDir,uDir);

   // translate the camera
   cameraID = scene.CreateObject(CAMERA);
   FnCamera camera;
   camera.ID(cameraID);
   camera.SetNearPlane(5.0f);
   camera.SetFarPlane(100000.0f);

   // set camera initial position and orientation
   dummy.GetDirection(fDir,NULL);
   CameraFollow(dummy, camera, fDir );
   DirectionFlag = DIR_HALT;
   
      /*
		add Donzo2 and Robber2
   */
   
      // load the character
   enemy[0].eid = scene.LoadCharacter("Donzo2");
   enemy[0].HitPoint = ENEMY_DEFAULT_HP;
   enemy[0].Flag = DIR_HALT;

   // put the character on terrain
   actor.ID(enemy[0].eid);
   pos[0] = 3569.0f + 83.0f; pos[1] = -3208.0f + 67.0f; pos[2] = 1000.0f;
   fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
   uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
   actor.SetDirection(fDir, uDir);

   actor.SetTerrainRoom(terrainRoomID, 10.0f);
   beOK = actor.PutOnTerrain(pos);

   // Get two character actions pre-defined at Donzo2
   enemy[0].damaged = actor.GetBodyAction(NULL, "DamageL");
   enemy[0].dead = actor.GetBodyAction(NULL, "Die");
   enemy[0].idle = actor.GetBodyAction(NULL, "Idle");

   // set the character to idle action
   curPoseID = enemy[0].idle;
   actor.SetCurrentAction(NULL, 0, curPoseID);
   actor.Play(START, 0.0f, FALSE, TRUE);
   
   // load the character
   enemy[1].eid = scene.LoadCharacter("Robber02");
   enemy[1].HitPoint = ENEMY_DEFAULT_HP;
   enemy[1].Flag = DIR_HALT;

   // put the character on terrain
   actor.ID(enemy[1].eid);
   pos[0] = 3569.0f + 2*83.0f; pos[1] = -3208.0f + 2*67.0f; pos[2] = 1000.0f;
   fDir[0] = 1.0f; fDir[1] = 1.0f; fDir[2] = 0.0f;
   uDir[0] = 0.0f; uDir[1] = 0.0f; uDir[2] = 1.0f;
   actor.SetDirection(fDir, uDir);

   actor.SetTerrainRoom(terrainRoomID, 10.0f);
   beOK = actor.PutOnTerrain(pos);

   // Get two character actions pre-defined at Robber2
   enemy[1].damaged = actor.GetBodyAction(NULL, "Damage1");
   enemy[1].dead = actor.GetBodyAction(NULL, "Die");
   enemy[1].idle = actor.GetBodyAction(NULL, "CombatIdle");

   // set the character to idle action
   curPoseID = enemy[1].idle;
   actor.SetCurrentAction(NULL, 0, curPoseID);
   actor.Play(START, 0.0f, FALSE, TRUE);

   // setup a point light
   FnLight lgt;
   lgt.ID(scene.CreateObject(LIGHT));
   lgt.Translate(70.0f, -70.0f, 70.0f, REPLACE);
   lgt.SetColor(1.0f, 1.0f, 1.0f);
   lgt.SetIntensity(1.0f);

   // create a text object for displaying messages on screen
   textID = FyCreateText("Trebuchet MS", 18, FALSE, FALSE);

   // set Hotkeys
   FyDefineHotKey(FY_ESCAPE, QuitGame, FALSE);  	// escape for quiting the game
   FyDefineHotKey(FY_UP, Movement, FALSE);      	// Up for moving forward
   FyDefineHotKey(FY_RIGHT, Movement, FALSE);   	// Right for turning right
   FyDefineHotKey(FY_LEFT, Movement, FALSE);    	// Left for turning left
   FyDefineHotKey(FY_DOWN, Movement, FALSE);    	// moving back
   FyDefineHotKey(FY_W, Movement, FALSE);      		// Up for moving forward
   FyDefineHotKey(FY_D, Movement, FALSE);   		// Right for turning right
   FyDefineHotKey(FY_A, Movement, FALSE);    		// Left for turning left
   FyDefineHotKey(FY_S, Movement, FALSE);    		// moving back
   FyDefineHotKey(FY_E, Pivot, FALSE);      		// pivot clockwise
   FyDefineHotKey(FY_Q, Pivot, FALSE);   			// pivot counterclockwise
   
   /*
		define hotkey for actor attack
		attack hotkey function should modify CharacterFlag
   */
   
   FyDefineHotKey(FY_Z, Movement, FALSE);   		// NormalAttack1
   FyDefineHotKey(FY_X, Movement, FALSE);   		// NormalAttack2
   FyDefineHotKey(FY_C, Movement, FALSE);   		// NormalAttack3
   FyDefineHotKey(FY_V, Movement, FALSE);   		// NormalAttack4

   // bind timers, frame rate = 30 fps
   FyBindTimer(0, 30.0f, GameAI, TRUE);
   FyBindTimer(1, 30.0f, RenderIt, TRUE);

   // invoke the system
   FyInvokeFly(TRUE);
}

void GameAI(int skip)
{
	//	variable initialization
	float pos[3], cpos[3], dpos[3], ppos[3], tpos[3], fDir[3], cfDir[3], tDir[3], uDir[3], unit_fDir[3], epos[ENEMY_NUMBERS][3];
	FnCharacter actor;	FnCamera camera;	FnObject dummy;
	/*
	for each enemy
		switch( enemy status flag )
			case damaged:
				play damaged action
			case dead:
				play dead action
			case default:
				enemy will not do further action in HW3
	*/
	for(int i=0;i<ENEMY_NUMBERS;i++ ){
		actor.ID(enemy[i].eid);
		actor.GetPosition(epos[i]);
		switch( enemy[i].Flag ){
			case CHAR_DAMAGED:
				if( actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
					enemy[i].Flag = DIR_HALT;
					curPoseID = enemy[i].idle;
					actor.SetCurrentAction(NULL, 0, curPoseID);
				}
				break;
			case CHAR_DEAD:
				actor.Play(ONCE, (float)skip, FALSE, TRUE);
				break;
			case DIR_HALT:
				actor.Play(LOOP, (float)skip, FALSE, TRUE);
				break;
			default:
				break;
		}
	}
	
	actor.ID(actorID);
	//特效
	FnScene scene(sID);
	FnGameFXSystem gxS(gFXID);
	switch( CharacterFlag ){
		case CHAR_ATTACK1:
			/*
			play attacking action
			we may need a frame counter to record how long attacking action will last, also when should enemy play damaged action and receive damage
			*/
			ActorFrameCounter++;
			if( actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
				curPoseID = idleID;
				actor.SetCurrentAction(NULL, 0, curPoseID);
				CharacterFlag = DIR_HALT;
				ActorFrameCounter = 0;
			}else{
				//特效
				gxS.Play((float) skip, ONCE);
			}

			if( ActorFrameCounter != ATTACK1_TIMING )return ;
			/*
			for each enemy
				if( enemy is in attacking range )
					set damaged flag for the enemy and deal damage to the enemy
					
			if the frame counter has show that attack has been done, set CharacterFlag to CharacterIdle and break switch
			*/
			for(int i=0;i<ENEMY_NUMBERS;i++){
				actor.ID(enemy[i].eid);
				if( enemy[i].Flag == CHAR_DEAD )continue;
				if( InAttackRange( NORMAL_ATTACK_1, actor ) ){
					enemy[i].HitPoint -= ATTACK1_DAMAGE;
					if( enemy[i].HitPoint <= 0 ){
						enemy[i].HitPoint = 0;
						enemy[i].Flag = CHAR_DEAD;
						curPoseID = enemy[i].dead;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						}
					else {
						enemy[i].Flag = CHAR_DAMAGED;
						curPoseID = enemy[i].damaged;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						actor.Play(START, 0.0f, FALSE, TRUE);
						}
				}
			}
			
			return ;		// remember character should not move when attacking
		case CHAR_ATTACK2:
			ActorFrameCounter++;
			if( actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
				curPoseID = idleID;
				actor.SetCurrentAction(NULL, 0, curPoseID);
				CharacterFlag = DIR_HALT;
				ActorFrameCounter = 0;
		
				//特效
				scene.DeleteGameFXSystem(gFXID);
				gFXID = FAILED_ID;
			}else{
				//特效
				gxS.Play((float) skip, ONCE);
			}

			if( ActorFrameCounter != ATTACK2_TIMING )return ;

			for(int i=0;i<ENEMY_NUMBERS;i++){
				actor.ID(enemy[i].eid);
				if( enemy[i].Flag == CHAR_DEAD )continue;
				if( InAttackRange( NORMAL_ATTACK_2, actor ) ){
					enemy[i].HitPoint -= ATTACK2_DAMAGE;
					if( enemy[i].HitPoint <= 0 ){
						enemy[i].HitPoint = 0;
						enemy[i].Flag = CHAR_DEAD;
						curPoseID = enemy[i].dead;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						}
					else {
						enemy[i].Flag = CHAR_DAMAGED;
						curPoseID = enemy[i].damaged;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						actor.Play(START, 0.0f, FALSE, TRUE);
						}
				}
			}
			
			return ;
		case CHAR_ATTACK3:
			ActorFrameCounter++;
			if( actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
				curPoseID = idleID;
				actor.SetCurrentAction(NULL, 0, curPoseID);
				CharacterFlag = DIR_HALT;
				ActorFrameCounter = 0;
			}else{
				//特效
				gxS.Play((float) skip, ONCE);
			}

			if( ActorFrameCounter != ATTACK3_TIMING )return ;

			for(int i=0;i<ENEMY_NUMBERS;i++){
				actor.ID(enemy[i].eid);
				if( enemy[i].Flag == CHAR_DEAD )continue;
				if( InAttackRange( NORMAL_ATTACK_3, actor ) ){
					enemy[i].HitPoint -= ATTACK3_DAMAGE;
					if( enemy[i].HitPoint <= 0 ){
						enemy[i].HitPoint = 0;
						enemy[i].Flag = CHAR_DEAD;
						curPoseID = enemy[i].dead;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						}
					else {
						enemy[i].Flag = CHAR_DAMAGED;
						curPoseID = enemy[i].damaged;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						actor.Play(START, 0.0f, FALSE, TRUE);
						}
				}
			}
			
			return ;
		case CHAR_ATTACK4:
			ActorFrameCounter++;
			if( actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
				curPoseID = idleID;
				actor.SetCurrentAction(NULL, 0, curPoseID);
				CharacterFlag = DIR_HALT;
				ActorFrameCounter = 0;
			}else{
				//特效
				gxS.Play((float) skip, ONCE);
			}
			if( ActorFrameCounter != ATTACK4_TIMING )return ;

			for(int i=0;i<ENEMY_NUMBERS;i++){
				actor.ID(enemy[i].eid);
				if( enemy[i].Flag == CHAR_DEAD )continue;
				if( InAttackRange( NORMAL_ATTACK_4, actor ) ){
					enemy[i].HitPoint -= ATTACK4_DAMAGE;
					if( enemy[i].HitPoint <= 0 ){
						enemy[i].HitPoint = 0;
						enemy[i].Flag = CHAR_DEAD;
						curPoseID = enemy[i].dead;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						}
					else {
						enemy[i].Flag = CHAR_DAMAGED;
						curPoseID = enemy[i].damaged;
						actor.SetCurrentAction(NULL, 0, curPoseID);
						actor.Play(START, 0.0f, FALSE, TRUE);
						}
				}
			}
			
			return ;
		case CHAR_DEAD:
		case CHAR_DAMAGED:
			//They will not be implemented in HW3
		default:
			break;	// Only if Character is not attacking, character and camera will be able to move.
	}
	
	actor.ID(actorID);	dummy.ID(dummyID);	camera.ID(cameraID);
	actor.GetPosition(ppos);	//	ppos will be used to determine if actor move
	dummy.GetPosition(dpos);
	
	switch( PivotFlag ){
		case DIR_LEFT:
			CameraPivot(dummy, camera, DIR_LEFT);
			break;
		case DIR_RIGHT:
			CameraPivot(dummy, camera, DIR_RIGHT);
			break;
		case DIR_HALT:
			break;
	}

	bool no_block = true;
	switch( DirectionFlag ){
		case DIR_FORWARD:
			//	forward vector will be calculated by two Cross
			CharacterFlag = DIR_FORWARD;
			camera.GetDirection(cfDir,NULL);
			actor.GetDirection(NULL,uDir);
			Cross(tDir,uDir,cfDir);
			Cross(fDir,tDir,uDir);
			dummy.SetDirection(fDir,NULL);
			//	forward left/right is implemented by vector Synthesis
			if( ForwardFlag != DIR_FORWARD ){
				if( ForwardFlag == DIR_LEFT )Cross(tDir,uDir,fDir);
				else if( ForwardFlag == DIR_RIGHT )Cross(tDir,fDir,uDir);
				NormalizeVector3(fDir);	NormalizeVector3(tDir);
				fDir[0]+=tDir[0];	fDir[1]+=tDir[1];	fDir[2]+=tDir[2];
			}
			//	actor move
			actor.SetDirection(fDir , uDir );
			unit_fDir[0] = fDir[0]; unit_fDir[1] = fDir[1]; unit_fDir[2] = fDir[2];
			NormalizeVector3(unit_fDir);
			tpos[0] = ppos[0] + move*unit_fDir[0]; tpos[1] = ppos[1] + move*unit_fDir[1]; tpos[2] = ppos[2] + move*unit_fDir[2];
			for (int i = 0; i < ENEMY_NUMBERS; i++){
				if ((enemy[i].Flag != CHAR_DEAD)){
					if (ActorRange(epos[i], tpos)){
						no_block = false;
						break;
					}
				}
			}
			if (no_block == true){
				actor.MoveForward(move, true, true, 0.0f, true);
			}
			actor.Play(LOOP, (float)skip, FALSE, TRUE);
			
			actor.GetPosition(pos);
			if( posdiff(pos,ppos) ){
			//	If actor didn't move, DummyCenter will center the actor.
				CharacterFlag = DIR_HALT;
				DummyCenter(actor, dummy);
				dummy.GetDirection(fDir,NULL);
				CameraFollow( dummy, camera, fDir );
			}
			else if( DummyRange( dpos, pos ) ){
			//	If actor is out of DUMMY_RANGE, dummy will follow the actor and camera will follow dummy
				DummyFollow( actor, dummy );
				dummy.GetDirection(fDir,NULL);
				CameraFollow( dummy, camera, fDir );
			}
			//	Else dummy and camera will not move
			break;
		case DIR_LEFT:
			//	left vector will be calculate by a Cross
			CharacterFlag = DIR_LEFT;
			actor.GetPosition(pos);
			actor.GetDirection(NULL,uDir);
			camera.GetPosition(cpos);
			cfDir[0] = pos[0]-cpos[0];	cfDir[1] = pos[1]-cpos[1];	cfDir[2] = pos[2]-cpos[2];
			Cross(tDir,uDir,cfDir);

			actor.SetDirection(tDir , uDir );
			unit_fDir[0] = tDir[0]; unit_fDir[1] = tDir[1]; unit_fDir[2] = tDir[2];
			NormalizeVector3(unit_fDir);
			tpos[0] = ppos[0] + move*unit_fDir[0]; tpos[1] = ppos[1] + move*unit_fDir[1]; tpos[2] = ppos[2] + move*unit_fDir[2];
			for (int i = 0; i < ENEMY_NUMBERS; i++){
				if ((enemy[i].Flag != CHAR_DEAD)){
					if (ActorRange(epos[i], tpos)){
						no_block = false;
						break;
					}
				}
			}
			if (no_block == true){
				actor.MoveForward(move, true, true, 0.0f, true);
			}
			actor.Play(LOOP, (float)skip, FALSE, TRUE);
			
			actor.GetPosition(pos);
			if( posdiff(pos,ppos) ){
			//	If actor didn't move, here will center the actor and pivot the camera
				CharacterFlag = DIR_HALT;
				CameraPivot(dummy, camera, DIR_LEFT);
				DummyCenter(actor, dummy);
			}
			else if( DummyRange( dpos, pos ) ){
				DummyFollow( actor, dummy );
				CameraFollow( dummy, camera, NULL );
			}
			break;
		case DIR_RIGHT:
			//	DIR_RIGHT is similar to DIR_LEFT
			CharacterFlag = DIR_RIGHT;
			actor.GetPosition(pos);
			actor.GetDirection(NULL,uDir);
			camera.GetPosition(cpos);
			cfDir[0] = pos[0]-cpos[0];	cfDir[1] = pos[1]-cpos[1];	cfDir[2] = pos[2]-cpos[2];
			Cross(tDir,cfDir,uDir);
			
			actor.SetDirection(tDir , uDir );
			unit_fDir[0] = tDir[0]; unit_fDir[1] = tDir[1]; unit_fDir[2] = tDir[2];
			NormalizeVector3(unit_fDir);
			tpos[0] = ppos[0] + move*unit_fDir[0]; tpos[1] = ppos[1] + move*unit_fDir[1]; tpos[2] = ppos[2] + move*unit_fDir[2];
			for (int i = 0; i < ENEMY_NUMBERS; i++){
				if ((enemy[i].Flag != CHAR_DEAD)){
					if (ActorRange(epos[i], tpos)){
						no_block = false;
						break;
					}
				}
			}
			if (no_block == true){
				actor.MoveForward(move, true, true, 0.0f, true);
			}
			actor.Play(LOOP, (float)skip, FALSE, TRUE);
			
			actor.GetPosition(pos);
			if( posdiff(pos,ppos) ){
				CharacterFlag = DIR_HALT;
				CameraPivot(dummy, camera, DIR_RIGHT);
				DummyCenter(actor, dummy);
			}
			else if( DummyRange( dpos, pos ) ){
				DummyFollow( actor, dummy );
				CameraFollow( dummy, camera, NULL );
			}
			break;
		case DIR_BACKWARD:
			//	DIR_BACKWARD is similar with DIR_FORWARD
			CharacterFlag = DIR_BACKWARD;
			camera.GetDirection(cfDir,NULL);
			actor.GetDirection(NULL,uDir);
			Cross(tDir,cfDir,uDir);
			Cross(fDir,tDir,uDir);
			
			actor.SetDirection(fDir , uDir );
			unit_fDir[0] = fDir[0]; unit_fDir[1] = fDir[1]; unit_fDir[2] = fDir[2];
			NormalizeVector3(unit_fDir);
			tpos[0] = ppos[0] + move*unit_fDir[0]; tpos[1] = ppos[1] + move*unit_fDir[1]; tpos[2] = ppos[2] + move*unit_fDir[2];
			for (int i = 0; i < ENEMY_NUMBERS; i++){
				if ((enemy[i].Flag != CHAR_DEAD)){
					if (ActorRange(epos[i], tpos)){
						no_block = false;
						break;
					}
				}
			}
			if (no_block == true){
				actor.MoveForward(move, true, true, 0.0f, true);
			}
			actor.Play(LOOP, (float)skip, FALSE, TRUE);
			
			actor.GetPosition(pos);
			if( posdiff(pos,ppos) ){
				CharacterFlag = DIR_HALT;
				DummyCenter(actor, dummy);
				actor.GetDirection(fDir,NULL);
				fDir[0]*=-1;	fDir[1]*=-1;
				CameraFollow( dummy, camera, fDir );
			}
			else if( DummyRange( dpos, pos ) ){
				DummyFollow( actor, dummy );
				actor.GetDirection(fDir,NULL);
				fDir[0]*=-1;	fDir[1]*=-1;
				CameraFollow( dummy, camera, fDir );
			}
			break;
		case DIR_HALT:
			//	When actor is not moving, center the actor slowly
			dummy.GetPosition(tpos);
			DummyCenter(actor, dummy);
			camera.GetDirection(cfDir,NULL);
			actor.GetDirection(NULL,uDir);
			Cross(tDir,uDir,cfDir);
			Cross(fDir,tDir,uDir);
			CameraFollow( dummy, camera, fDir );
			actor.Play(LOOP, (float)skip, FALSE, TRUE);
			dummy.GetPosition(dpos);
			if( posdiff(dpos,tpos) )CharacterFlag = DIR_HALT;
			break;
		default:
			break;
	}
}

/*----------------------
  perform the rendering
  C.Wang 0720, 2006
 -----------------------*/
void RenderIt(int skip)
{
   FnViewport vp;

   // render the whole scene
   vp.ID(vID);
   vp.Render3D(cameraID, TRUE, TRUE);

   // get camera's data
   FnCamera camera;
   camera.ID(cameraID);

   float pos[3], fDir[3], uDir[3];
   camera.GetPosition(pos);
   camera.GetDirection(fDir, uDir);

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
   
   /*
		Show enemy hit point here
   */

   FnText text;
   text.ID(textID);

   text.Begin(vID);
   text.Write(string, 20, 20, 255, 0, 0);

   char posS[256], fDirS[256], uDirS[256], chf[256], blood[2][256];
   sprintf(posS, "pos: %8.3f %8.3f %8.3f", pos[0], pos[1], pos[2]);
   sprintf(fDirS, "facing: %8.3f %8.3f %8.3f", fDir[0], fDir[1], fDir[2]);
   sprintf(uDirS, "up: %8.3f %8.3f %8.3f", uDir[0], uDir[1], uDir[2]);
   sprintf(chf, "CharacterFlag= %d, ActorFrameCounter= %d", CharacterFlag, ActorFrameCounter );
   sprintf(blood[0], "Dozon2: %d", enemy[0].HitPoint); sprintf(blood[1], "Robber02: %d", enemy[1].HitPoint);

   text.Write(posS, 20, 35, 255, 255, 0);
   text.Write(fDirS, 20, 50, 255, 255, 0);
   text.Write(uDirS, 20, 65, 255, 255, 0);
   text.Write(chf, 20, 80, 255, 255, 0);
   text.Write(blood[0], 20, 100, 255, 255, 0); text.Write(blood[1], 20, 115, 255, 255, 0);


   text.End();

   // swap buffer
   FySwapBuffers();
}

/*------------------
  movement control
  C.Wang 1103, 2006
 -------------------*/
void Movement(BYTE code, BOOL4 value)
{
	FnCharacter actor;
	actor.ID(actorID);
	
	if( CharacterFlag == CHAR_ATTACK1 || CharacterFlag == CHAR_ATTACK2 || CharacterFlag == CHAR_ATTACK3 || CharacterFlag == CHAR_ATTACK4 )return ;
	if (value){

		//特效
		FnScene scene(sID);
		gFXID = scene.CreateGameFXSystem();
		OBJECTid baseID = actor.GetBaseObject();
		FnGameFXSystem gxS(gFXID);

		if (code == FY_Z){
			DirectionFlag = DIR_HALT;
			curPoseID = normalattack1;
			actor.SetCurrentAction(NULL, 0, curPoseID);
			CharacterFlag = CHAR_ATTACK1;

			//特效
			BOOL4 beOK = gxS.Load("Lyubu_skill01", TRUE);
			gxS.SetParentObjectForAll(baseID);

			return ;
		}
		if (code == FY_X){
			DirectionFlag = DIR_HALT;
			curPoseID = normalattack2;
			actor.SetCurrentAction(NULL, 0, curPoseID);
			CharacterFlag = CHAR_ATTACK2;

			//特效
			BOOL4 beOK = gxS.Load("Lyubu_skill03", TRUE);
			gxS.SetParentObjectForAll(baseID);
		
			return ;
		}
		if (code == FY_C){
			DirectionFlag = DIR_HALT;
			curPoseID = normalattack3;
			actor.SetCurrentAction(NULL, 0, curPoseID);
			CharacterFlag = CHAR_ATTACK3;

			//特效
			BOOL4 beOK = gxS.Load("Lyubu_atk01", TRUE);
			gxS.SetParentObjectForAll(baseID);

			return ;
		}
		if (code == FY_V){
			DirectionFlag = DIR_HALT;
			curPoseID = normalattack4;
			actor.SetCurrentAction(NULL, 0, curPoseID);
			CharacterFlag = CHAR_ATTACK4;

			//特效
			BOOL4 beOK = gxS.Load("Lyubu_skill04", TRUE);
			gxS.SetParentObjectForAll(baseID);

			return ;
		}

		curPoseID = runID;
		actor.SetCurrentAction(NULL, 0, curPoseID);
		move = MOVE_SPEED_DEFAULT;
		if (code == FY_UP || code == FY_W ){
			if( DirectionFlag != DIR_LEFT && DirectionFlag != DIR_RIGHT )DirectionFlag = DIR_FORWARD;
		}
		else if (code == FY_RIGHT || code == FY_D ){
			if( DirectionFlag == DIR_FORWARD )ForwardFlag = DIR_RIGHT;
			else DirectionFlag = DIR_RIGHT;
		}
		else if (code == FY_LEFT || code == FY_A ){
			if( DirectionFlag == DIR_FORWARD )ForwardFlag = DIR_LEFT;
			else DirectionFlag = DIR_LEFT;
		}
		else if (code == FY_DOWN || code == FY_S ){
			DirectionFlag = DIR_BACKWARD;
		}
	}
	else{
		if( code == FY_Z || code == FY_X || code == FY_C || code == FY_V){
			DirectionFlag = DIR_HALT;
			return ;
		}
		else if (code == FY_UP|| code == FY_W){
			DirectionFlag = ForwardFlag;
			if( DirectionFlag != DIR_FORWARD )return ;
		}
		else if (code == FY_RIGHT || code == FY_LEFT || code == FY_D || code == FY_A){
			ForwardFlag = DIR_FORWARD;
			if( DirectionFlag == DIR_FORWARD )return ;
		}
		DirectionFlag = DIR_HALT;
		move = MOVE_SPEED_HALT;
		curPoseID = idleID;
		actor.SetCurrentAction(NULL, 0, curPoseID);
	}
}

/*------------------
  quit the demo
  C.Wang 0327, 2005
 -------------------*/
void QuitGame(BYTE code, BOOL4 value)
{
   if (code == FY_ESCAPE) {
      if (value) {
         FyQuitFlyWin32();
      }
   }
}

void Pivot(BYTE code, BOOL4 value)
{
	if( value ){
		if( code == FY_E ){
			if( PivotFlag == DIR_LEFT )return ;
			else PivotFlag = DIR_RIGHT;
		}
		else if( code == FY_Q ){
			if( PivotFlag == DIR_RIGHT )return ;
			else PivotFlag = DIR_LEFT;
		}
	}
	else{
		PivotFlag = DIR_HALT;
	}
}

int CameraFollow( FnObject dummy, FnCamera camera, float direction[3] ){
	//	variable initialization
	float pos[3], cpos[3], tpos[3] = {0,0,-1}, t2pos[3], cfDir[3], cuDir[3], tDir[3], t2Dir[3];
	FnObject terrain;
	terrain.ID(tID);

	dummy.GetPosition(pos);
	camera.GetPosition(t2pos);
	
	if( direction == NULL ){
	//	If we don't want to move camera position, direction will be NULL.
		camera.GetPosition(cpos);
	}
	else {
		//	standard camera position.
		NormalizeVector3(direction);
		cpos[0] = pos[0]-direction[0]*CAMERA_DEFAULT_DISTANCE;
		cpos[1] = pos[1]-direction[1]*CAMERA_DEFAULT_DISTANCE;
		cpos[2] = pos[2]+CAMERA_DEFAULT_HEIGHT;
		
		if( t2pos[2]-pos[2] > CAMERA_DEFAULT_HEIGHT && ( CharacterFlag == DIR_FORWARD || CharacterFlag == DIR_HALT ) ){
		//	return to normal height
			camera.GetPosition(cpos);
			if( CharacterFlag != DIR_HALT )cpos[2] -= CAMERA_ASCEND_SPEED;
		}
		if( terrain.HitTest(cpos,tpos) <= 0 && ( CharacterFlag == DIR_BACKWARD || CharacterFlag == DIR_HALT ) ){
		//	if standard camera position is blocked, it will raise the camera position.
			camera.GetPosition(cpos);
			if( CharacterFlag != DIR_HALT )cpos[2] += CAMERA_ASCEND_SPEED;
		}
	}
	
	//	camera uDir will be calculated by two Cross.
	cfDir[0] = pos[0]-cpos[0];	cfDir[1] = pos[1]-cpos[1];	cfDir[2] = pos[2]-cpos[2]+CAMERA_CHARACTER_HEIGHT;
    tDir[0] = cfDir[0];			tDir[1] = cfDir[1];			tDir[2] = 0.0f;
	
	Cross(t2Dir,cfDir,tDir);
    Cross(cuDir,t2Dir,cfDir);
	//	set camera position and direction
	camera.SetPosition(cpos);
	camera.SetDirection(cfDir,cuDir);
	
	return 0;
}

int CameraPivot( FnObject dummy, FnCamera camera, int direction ){
	// variable initialization
	float pos[3], cpos[3], tpos[3], cfDir[3], cuDir[3], tDir[3], t2Dir[3];
	dummy.GetPosition(pos);
	camera.GetPosition(cpos);
	
	tpos[0]=pos[0];	tpos[1]=pos[1];	tpos[2]=cpos[2];
	tDir[0] = cpos[0]-tpos[0];	tDir[1] = cpos[1]-tpos[1];
	//	a two dimension rotation matrix multiplication
	if( direction == DIR_LEFT ){
		tDir[0] = tDir[0]*PivotCos-tDir[1]*PivotSin;
		tDir[1] = tDir[0]*PivotSin+tDir[1]*PivotCos;
	}
	else if( direction == DIR_RIGHT ){
		tDir[0] = tDir[0]*PivotCos+tDir[1]*PivotSin;
		tDir[1] = -1*tDir[0]*PivotSin+tDir[1]*PivotCos;
	}
	else return -1;

	cpos[0] = tpos[0]+tDir[0];	cpos[1] = tpos[1]+tDir[1];
	cfDir[0] = pos[0]-cpos[0];	cfDir[1] = pos[1]-cpos[1];	cfDir[2] = pos[2]-cpos[2]+CAMERA_CHARACTER_HEIGHT;
    tDir[0] = cfDir[0];			tDir[1] = cfDir[1];			tDir[2] = 0.0f;
	
	Cross(t2Dir,cfDir,tDir);
    Cross(cuDir,t2Dir,cfDir);
	
	camera.SetPosition(cpos);
	camera.SetDirection(cfDir,cuDir);
	
	return 0;
}

int DummyFollow( FnCharacter actor, FnObject dummy ){
	float dpos[3], fDir[3], uDir[3];
	dummy.GetPosition(dpos);
	actor.GetDirection(fDir,uDir);
	NormalizeVector3(fDir);
	dpos[0] += move*fDir[0];	dpos[1] += move*fDir[1];
	dummy.SetPosition(dpos);
	//dummy.SetDirection(fDir, uDir);
	return 0;
}

int DummyCenter( FnCharacter actor, FnObject dummy ){
	float pos[3], dpos[3], tDir[3];
	actor.GetPosition(pos);
	dummy.GetPosition(dpos);
	
	tDir[0] = pos[0] - dpos[0];	tDir[1] = pos[1] - dpos[1];	tDir[2] = 0;
	NormalizeVector3(tDir);
	if( fabs(pos[0]-dpos[0])<=fabs(tDir[0]*DUMMY_CENTER_SPEED))dpos[0] = pos[0];
	else dpos[0] = dpos[0]+tDir[0]*DUMMY_CENTER_SPEED;
	if( fabs(pos[1]-dpos[1])<=fabs(tDir[1]*DUMMY_CENTER_SPEED))dpos[1] = pos[1];
	else dpos[1] = dpos[1]+tDir[1]*DUMMY_CENTER_SPEED;
	
	dummy.SetPosition(dpos);
	
	return 0;
}

int DummyRange( const float dpos[3], const float pos[3] ){
	if( sqrt( ( dpos[0]-pos[0] )*( dpos[0]-pos[0] ) + ( dpos[1]-pos[1] )*( dpos[1]-pos[1] )  ) >= DUMMY_RANGE )return 1;
	else return 0;
}

int posdiff( const float pos1[], const float pos2[] ){
	if( pos1[0] ==  pos2[0] && pos1[1] ==  pos2[1] && pos1[2] ==  pos2[2] )return 1;
	else return 0;
}

void NormalizeVector3( float obj[3] ){
	float c = (float)sqrt(obj[0]*obj[0]+obj[1]*obj[1]+obj[2]*obj[2]);
	if( c == 0 )return ;
	obj[0]/=c;	obj[1]/=c;	obj[2]/=c;
	return ;
}

void Cross( float tar[3], const float obj1[3], const float obj2[3]  ){
	tar[0]=obj1[1]*obj2[2]-obj1[2]*obj2[1];
	tar[1]=obj1[2]*obj2[0]-obj1[0]*obj2[2];
	tar[2]=obj1[0]*obj2[1]-obj1[1]*obj2[0];
	return;
}

int ActorRange(const float dpos[3], const float pos[3]){
	if (sqrt((dpos[0] - pos[0])*(dpos[0] - pos[0]) + (dpos[1] - pos[1])*(dpos[1] - pos[1])) <= MOVE_BLOCK_RANGE)return 1;
	else return 0;
}

int InAttackRange( const int attack, FnCharacter enemy ){
	float epos[3], pos[3], fDir[3], tfDir[3], r, cross, cosθ, angle;
	FnCharacter actor;
	actor.ID(actorID);
	enemy.GetPosition(epos);
	actor.GetPosition(pos);
	actor.GetDirection(fDir,NULL);

	tfDir[0] = epos[0] - pos[0];	tfDir[1] = epos[1] - pos[1];	tfDir[2] = epos[2] - pos[2];
	r = sqrt((epos[0] - pos[0])*(epos[0] - pos[0]) + (epos[1] - pos[1])*(epos[1] - pos[1]));
	cosθ = (fDir[0] * tfDir[0] + fDir[1] * tfDir[1]) / (sqrt(fDir[0] * fDir[0] + fDir[1] * fDir[1])*sqrt(tfDir[0] * tfDir[0] + tfDir[1] * tfDir[1]));
	angle = acos(cosθ) * 180.0 / 3.14159265;
	//if cross > 0, 0 = the angle fDir rotate clockwise
	cross = fDir[0] * tfDir[1] - fDir[1] * tfDir[0];

	switch( attack ){
		case NORMAL_ATTACK_1:
			if ( r <= ATTACK1_DISTANCE && angle <= ATTACK1_ANGLE )
				return 1;
			else return 0;
		case NORMAL_ATTACK_2:
			if ( r <= ATTACK2_DISTANCE )
				return 1;
			else return 0;
		case NORMAL_ATTACK_3:
			if ( r <= ATTACK3_DISTANCE && !( 90 < angle && angle < 180 && cross < 0 ) )
				return 1;
			else return 0;
		case NORMAL_ATTACK_4:
			if ( r <= ATTACK4_DISTANCE && angle <= ATTACK4_ANGLE )
				return 1;
			else return 0;
	}
	

	
}