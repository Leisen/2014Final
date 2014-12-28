#include "FlyWin32.h"
#include "lua.hpp"

#define MAX_ACTION_NUM				32
#define ACTION_NAME_SIZE			128
#define CHARACTER_TYPE_NAME_SIZE	128
#define MAX_CHARACTER_NUM			128
#define MAX_TYPE_NUM				128

typedef	enum DirFlag{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	FRONTLEFT,
	FRONTRIGHT,
	BACKLEFT,
	BACKRIGHT,
	HALT,
	INIT
}DirFlag;

typedef	enum ActFlag{
	IDLE,
	MOVE,
	ATTACK,
	DAMAGED,
	DEAD
}ActFlag;

typedef struct Actions{
	ACTIONid aid;
	char name[ACTION_NAME_SIZE];
	int (*ActFunction)(int);
};

typedef struct Character{
	FnCharacter actor;
	struct Actions act[MAX_ACTION_NUM];
	int prototype;
	DirFlag dflag;
	ActFlag aflag;
	int HitPoint;
	int ManaPoint;
	int Experience;
	float MoveSpeed;
	// any other special effect flag
};

typedef struct CharacterType{
	char name[CHARACTER_TYPE_NAME_SIZE];
	FnCharacter actor;
	struct Actions act[MAX_ACTION_NUM];
};

struct Character *CharacterList[MAX_CHARACTER_NUM];
struct CharacterType CharacterTypeList[MAX_TYPE_NUM];
unsigned int TypeListStackPointer = 0;

void LoadParameters( char lua[] );

// define all global variable here

float PLAYER_STARTING_POSITION[3];
float CAMERA_DEFALT_DIRECION[3];
float CAMERA_DEFALT_DISTANCE;
float PLAYER_MOVE_SPEED;

// 

//
int width = 800;
int height = 600;
int MyFlag = 0;
int frame = 0;
VIEWPORTid vID;                 // the major viewport
SCENEid sID;                    // the 3D scene
OBJECTid cID, tID;
SCENEid sID0;                   // the 2D scene
OBJECTid spID0 = FAILED_ID;		// the sprite for character status
ROOMid terrainRoomID = FAILED_ID;
TEXTid textID = FAILED_ID;
float DIRECTION[9][3];
float DEFAULT_UDIR[3] = {0.0f, 0.0f, 1.0f};

//

void RegisterKeyBinding();
void QuitGame(BYTE, BOOL4);
void MainControl(BYTE, BOOL4);

void SetEnvironment();
void CreateScene( char Scene[], char Terrain[] );
void InitDirection();

int CheckCharacterType( char name[] );
void RegisterCharacterType( char name[] );
void RegisterCharacterAction( char type[], char name[], ActFlag aflag, int (*ActFunction)(int) );
struct Character* SpawnCharacter( char type[], float pos[3], DirFlag dflag );

void CameraInit();
void CameraFollow();

int BlockTest();

void GameAI(int);
void RenderIt(int);

int Lyubu2Attack(int);
int DoNothing(int);

void FyMain(int argc, char **argv){
	BOOL4 beOK = FyStartFlyWin32("NTU@2014 Homework #01 - Use Fly2", 0, 0, width, height, FALSE);
	LoadParameters( "..\\..\\NTU5\\config.cfg" );
	
	//SetEnvironment();
	CreateScene( "gameScene01", "terrain" );
	
	InitDirection();
	RegisterKeyBinding();
	
	RegisterCharacterType( "Lyubu2" );
	RegisterCharacterAction( "Lyubu2", "Idle", IDLE, DoNothing );
	RegisterCharacterAction( "Lyubu2", "Run", MOVE, DoNothing );
	RegisterCharacterAction( "Lyubu2", "NormalAttack1", ATTACK, Lyubu2Attack );
	
	CharacterList[0] = SpawnCharacter( "Lyubu2", PLAYER_STARTING_POSITION, FORWARD );
	CharacterList[0]->MoveSpeed = PLAYER_MOVE_SPEED;
	CameraInit();
	
	FyBindTimer(0, 30.0f, GameAI, TRUE);
	FyBindTimer(1, 30.0f, RenderIt, TRUE);

	FyInvokeFly(TRUE);
}

void LoadParameters( char lua[] ){
	lua_State *ls = lua_open();
	luaopen_base(ls);
	luaL_openlibs(ls);
	
	luaL_dofile( ls, lua );
	/*
		tutorial:
			if you want to use another global variable
				lua_getglobal( ls, GLOBAL_VARIABLE_NAME );
				                   ^^^^^^^^^^^^^^^^^^^^
								   it should be same as what you define at config.cfg
								   
				To get a value from the stack, there are the lua_to* functions:

					int            lua_toboolean (lua_State *L, int index);
					double         lua_tonumber (lua_State *L, int index);
					const char    *lua_tostring (lua_State *L, int index);
					size_t         lua_strlen (lua_State *L, int index);
								   
				YOUR_GLOBAL_VARIABLE = (VARIABLE_TYPE)lua_tonumber( ls, -1 );
				^^^^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^
				define you variable     translate type
				
				lua_pop( ls, 1 );
	*/
	//	here is an example
	lua_getglobal( ls, "PLAYER_STARTING_POSITION_0" );
	PLAYER_STARTING_POSITION[0] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "PLAYER_STARTING_POSITION_1" );
	PLAYER_STARTING_POSITION[1] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "PLAYER_STARTING_POSITION_2" );
	PLAYER_STARTING_POSITION[2] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );

	lua_getglobal( ls, "CAMERA_DEFALT_DIRECION_0" );
	CAMERA_DEFALT_DIRECION[0] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "CAMERA_DEFALT_DIRECION_1" );
	CAMERA_DEFALT_DIRECION[1] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "CAMERA_DEFALT_DIRECION_2" );
	CAMERA_DEFALT_DIRECION[2] = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "CAMERA_DEFALT_DISTANCE" );
	CAMERA_DEFALT_DISTANCE = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_getglobal( ls, "PLAYER_MOVE_SPEED" );
	PLAYER_MOVE_SPEED = (float)lua_tonumber( ls, -1 );
	lua_pop( ls, 1 );
	
	lua_close(ls);
	return ;
}

void RegisterKeyBinding(){
	FyDefineHotKey(FY_ESCAPE, 	QuitGame, 	 	FALSE);
	FyDefineHotKey(FY_UP, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_RIGHT, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_LEFT, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_DOWN, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_Z, 		MainControl, 	FALSE);
}

void QuitGame(BYTE code, BOOL4 value){
	if (code == FY_ESCAPE) {
		if (value) {
			FyQuitFlyWin32();
		}
	}
}

void MainControl(BYTE code, BOOL4 value){
	float fDir[3];
	static bool kflag[4] = { false, false, false, false };
	static ActFlag aflag;
	aflag = CharacterList[0]->aflag;
	if( CharacterList[0]->aflag == ATTACK )return ;
	if( value ){
		switch( code ){
			case FY_Z:
				CharacterList[0]->aflag = ATTACK;
				break;
			case FY_UP:
				kflag[FORWARD] = true;
				break;
			case FY_DOWN:
				kflag[BACKWARD] = true;
				break;
			case FY_LEFT:
				kflag[LEFT] = true;
				break;
			case FY_RIGHT:
				kflag[RIGHT] = true;
				break;
		}
	}
	else{
		switch( code ){
			case FY_Z:
				CharacterList[0]->aflag = IDLE;
				break;
			case FY_UP:
				kflag[FORWARD] = false;
				break;
			case FY_DOWN:
				kflag[BACKWARD] = false;
				break;
			case FY_LEFT:
				kflag[LEFT] = false;
				break;
			case FY_RIGHT:
				kflag[RIGHT] = false;
				break;
		}
	}
	if( CharacterList[0]->aflag == MOVE || CharacterList[0]->aflag == IDLE ){
		if	   ( kflag[FORWARD]&!kflag[BACKWARD]&!(kflag[LEFT]^kflag[RIGHT]) ){	CharacterList[0]->dflag = FORWARD;		CharacterList[0]->aflag = MOVE;}
		else if( !kflag[FORWARD]&kflag[BACKWARD]&!(kflag[LEFT]^kflag[RIGHT]) ){	CharacterList[0]->dflag = BACKWARD;		CharacterList[0]->aflag = MOVE;}
		else if( !(kflag[FORWARD]^kflag[BACKWARD])&kflag[LEFT]&!kflag[RIGHT] ){	CharacterList[0]->dflag = LEFT;			CharacterList[0]->aflag = MOVE;}
		else if( !(kflag[FORWARD]^kflag[BACKWARD])&!kflag[LEFT]&kflag[RIGHT] ){	CharacterList[0]->dflag = RIGHT;		CharacterList[0]->aflag = MOVE;}
		else if( kflag[FORWARD]&!kflag[BACKWARD]&kflag[LEFT]&!kflag[RIGHT] ){	CharacterList[0]->dflag = FRONTLEFT;	CharacterList[0]->aflag = MOVE;}
		else if( kflag[FORWARD]&!kflag[BACKWARD]&!kflag[LEFT]&kflag[RIGHT] ){	CharacterList[0]->dflag = FRONTRIGHT;	CharacterList[0]->aflag = MOVE;}
		else if( !kflag[FORWARD]&kflag[BACKWARD]&kflag[LEFT]&!kflag[RIGHT] ){	CharacterList[0]->dflag = BACKLEFT;		CharacterList[0]->aflag = MOVE;}
		else if( !kflag[FORWARD]&kflag[BACKWARD]&!kflag[LEFT]&kflag[RIGHT] ){	CharacterList[0]->dflag = BACKRIGHT;	CharacterList[0]->aflag = MOVE;}
		else CharacterList[0]->aflag = IDLE;
	}
	
	if( aflag != CharacterList[0]->aflag ){
		CharacterList[0]->actor.SetCurrentAction(NULL, 0, CharacterList[0]->act[CharacterList[0]->aflag].aid );
		/*
		if( CharacterList[0]->actor.SetCurrentAction(NULL, 0, CharacterList[0]->act[CharacterList[0]->aflag].aid ) == 0 ){
			CharacterList[0]->act[CharacterList[0]->aflag].aid = 
			CharacterList[0]->actor.GetBodyAction(NULL, CharacterTypeList[CharacterList[0]->prototype].act[CharacterList[0]->aflag].name);
			CharacterList[0]->act[CharacterList[0]->aflag].ActFunction = 
			CharacterTypeList[CharacterList[0]->prototype].act[CharacterList[0]->aflag].ActFunction;
		}
		*/
	}
}

void SetEnvironment(){

    // setup the data searching paths	
	FySetGameFXPath("..\\..\\NTU5\\GameFXs");
	FySetShaderPath("..\\..\\NTU5\\Shaders");
	FySetModelPath("..\\..\\NTU5\\Scenes");
	FySetTexturePath("..\\..\\NTU5\\Scenes\\Textures");
	FySetScenePath("..\\..\\NTU5\\Scenes");
}

void CreateScene( char Scene[], char Terrain[] ){
	FySetShaderPath("..\\..\\NTU5\\Shaders");
	FySetModelPath("..\\..\\NTU5\\Scenes");
	FySetTexturePath("..\\..\\NTU5\\Scenes\\Textures");
	FySetScenePath("..\\..\\NTU5\\Scenes");
	// create a viewport
	vID = FyCreateViewport(0, 0, width, height);
	FnViewport vp;
	vp.ID(vID);
	
	// create a 3D scene
	sID = FyCreateScene(10);
	FnScene scene;
	scene.ID(sID);

	// load the scene
	scene.Load( Scene );
	scene.SetAmbientLights(1.0f, 1.0f, 1.0f, 0.6f, 0.6f, 0.6f);
	
	// load the terrain
	tID = scene.CreateObject(OBJECT);
	FnObject terrain;
	terrain.ID(tID);
	terrain.Load( Terrain );
	terrain.Show(FALSE);

   // set terrain environment
	terrainRoomID = scene.CreateRoom(SIMPLE_ROOM, 10);
	FnRoom room;
	room.ID(terrainRoomID);
	room.AddObject(tID);
	
	cID = scene.CreateObject(CAMERA);
    FnCamera camera;
    camera.ID(cID);
    camera.SetNearPlane(5.0f);
    camera.SetFarPlane(100000.0f);
	
	// setup a point light
    FnLight lgt;
    lgt.ID(scene.CreateObject(LIGHT));
    lgt.Translate(70.0f, -70.0f, 70.0f, REPLACE);
    lgt.SetColor(1.0f, 1.0f, 1.0f);
    lgt.SetIntensity(1.0f);

    // create a text object for displaying messages on screen
    textID = FyCreateText("Trebuchet MS", 18, FALSE, FALSE);

	// create a 2D scene for sprites
	sID0 = FyCreateScene(1);
	FnScene scene0;
	scene0.ID(sID0);
	scene0.SetSpriteWorldSize(width, height);

	// sprite for character status
	FySetTexturePath("..\\..\\NTU5\\Textures");
	FnSprite sp;
	spID0 = scene0.CreateObject(SPRITE);
	sp.ID(spID0);
	sp.SetSize(0.3*width, 0.3*width*0.375);
	sp.SetImage("status", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10, height-0.3*width*0.375-10, 0);
}

void InitDirection(){
	DIRECTION[HALT][0] = 0.0f;			DIRECTION[HALT][1] = 0.0f;			DIRECTION[HALT][2] = 0.0f;
	DIRECTION[FORWARD][0] = 1.0f;		DIRECTION[FORWARD][1] = 0.0f;		DIRECTION[FORWARD][2] = 0.0f;
	DIRECTION[BACKWARD][0] = -1.0f;		DIRECTION[BACKWARD][1] = 0.0f;		DIRECTION[BACKWARD][2] = 0.0f;
	DIRECTION[LEFT][0] = 0.0f;			DIRECTION[LEFT][1] = 1.0f;			DIRECTION[LEFT][2] = 0.0f;
	DIRECTION[RIGHT][0] = 0.0f;			DIRECTION[RIGHT][1] = -1.0f;		DIRECTION[RIGHT][2] = 0.0f;
	DIRECTION[FRONTLEFT][0] = 1.0f;		DIRECTION[FRONTLEFT][1] = 1.0f;		DIRECTION[FRONTLEFT][2] = 0.0f;
	DIRECTION[BACKLEFT][0] = -1.0f;		DIRECTION[BACKLEFT][1] = 1.0f;		DIRECTION[BACKLEFT][2] = 0.0f;
	DIRECTION[FRONTRIGHT][0] = 1.0f;	DIRECTION[FRONTRIGHT][1] = -1.0f;	DIRECTION[FRONTRIGHT][2] = 0.0f;
	DIRECTION[BACKRIGHT][0] = -1.0f;	DIRECTION[BACKRIGHT][1] = -1.0f;	DIRECTION[BACKRIGHT][2] = 0.0f;
}

int CheckCharacterType( char name[] ){
	for(int tmp=0;tmp < TypeListStackPointer;tmp++ ){
		if( !strcmp( name, CharacterTypeList[tmp].name ) )return tmp;
	}
	return -1;
}

void RegisterCharacterType( char name[] ){
	FySetModelPath("..\\..\\NTU5\\Characters");
    FySetTexturePath("..\\..\\NTU5\\Characters");
    FySetCharacterPath("..\\..\\NTU5\\Characters");
	if( CheckCharacterType( name ) != -1 )return ;
	if( TypeListStackPointer >= MAX_TYPE_NUM )return ;
	
	FnScene scene;
	scene.ID(sID);
	
	CHARACTERid cid = scene.LoadCharacter( name );
	CharacterTypeList[TypeListStackPointer].actor.ID(cid);
	strcpy( CharacterTypeList[TypeListStackPointer].name, name );
	for(int i=0;i<MAX_ACTION_NUM;i++)CharacterTypeList[TypeListStackPointer].act[i].aid = 0;
	
	TypeListStackPointer++;
}

void RegisterCharacterAction( char type[], char name[], ActFlag aflag, int (*ActFunction)(int) ){
	FySetModelPath("..\\..\\NTU5\\Characters");
    FySetTexturePath("..\\..\\NTU5\\Characters");
    FySetCharacterPath("..\\..\\NTU5\\Characters");
	int TypeID;
	if( ( TypeID = CheckCharacterType( type ) ) == -1 )return ;
	CharacterTypeList[TypeID].act[aflag].aid = 1;
	strcpy( CharacterTypeList[TypeID].act[aflag].name, name );
	CharacterTypeList[TypeID].act[aflag].ActFunction = ActFunction;
	return ;
}

struct Character* SpawnCharacter( char type[], float pos[3], DirFlag dflag ){
	int TypeID;
	if( ( TypeID = CheckCharacterType( type ) ) == -1 )return NULL;
	struct Character* tmp = ( struct Character* )malloc( sizeof(Character) );
	CHARACTERid cid = CharacterTypeList[TypeID].actor.Clone( false, false, false );
	tmp->actor.ID(cid);
	tmp->actor.SetDirection(DIRECTION[dflag], DEFAULT_UDIR);
	tmp->actor.SetTerrainRoom(terrainRoomID, 10.0f);
	tmp->actor.PutOnTerrain(pos);
	tmp->prototype = TypeID;
	tmp->aflag = IDLE;
	tmp->dflag = dflag;
	
	for(int i=0;i<MAX_ACTION_NUM;i++){
		if( CharacterTypeList[TypeID].act[i].aid == 0 )tmp->act[i].aid = 0;
		else{
			tmp->act[i].aid = tmp->actor.GetBodyAction(NULL, CharacterTypeList[TypeID].act[i].name );
			tmp->act[i].ActFunction = CharacterTypeList[TypeID].act[i].ActFunction;
		}
	}
	
	return tmp;
}

void CameraInit(){
	CameraFollow();
	FnCamera camera;
	camera.ID(cID);
	float fDir[3] = { CAMERA_DEFALT_DIRECION[0]*CAMERA_DEFALT_DISTANCE, CAMERA_DEFALT_DIRECION[1]*CAMERA_DEFALT_DISTANCE, CAMERA_DEFALT_DIRECION[2]*CAMERA_DEFALT_DISTANCE };
	float uDir[3], tDir[3];
	CharacterList[0]->actor.GetDirection(NULL, uDir);
	FyCross( tDir, fDir, uDir );
	FyCross( uDir, tDir, fDir );
	camera.SetDirection(fDir, uDir);
}

void CameraFollow(){
	static float dis[3] = { -1*CAMERA_DEFALT_DIRECION[0]*CAMERA_DEFALT_DISTANCE, -1*CAMERA_DEFALT_DIRECION[1]*CAMERA_DEFALT_DISTANCE, -1*CAMERA_DEFALT_DIRECION[2]*CAMERA_DEFALT_DISTANCE };
	float pos[3];
	FnCamera camera;
	camera.ID(cID);
	CharacterList[0]->actor.GetPosition(pos);
	pos[0]+=dis[0];	pos[1]+=dis[1];	pos[2]+=dis[2];
	camera.SetPosition(pos);
}

int BlockTest(){
	return 1;
}

void GameAI(int skip){
	switch( CharacterList[0]->aflag ){
		case IDLE:
			CharacterList[0]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
			break;
		case MOVE:
			CharacterList[0]->actor.SetDirection( DIRECTION[CharacterList[0]->dflag], NULL );
			if( BlockTest() )CharacterList[0]->actor.MoveForward(CharacterList[0]->MoveSpeed, true, true, 0.0f, true);
			CharacterList[0]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
			break;
		case ATTACK:
			(*(CharacterList[0]->act[CharacterList[0]->aflag].ActFunction))(skip);
			/*
			if( CharacterList[0]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
				CharacterList[0]->aflag = IDLE;
				CharacterList[0]->actor.SetCurrentAction(NULL, 0, CharacterList[0]->act[CharacterList[0]->aflag].aid );
			}
			*/
			break;
		case DAMAGED:
			CharacterList[0]->actor.Play(ONCE, (float)skip, FALSE, TRUE);
			break;
		case DEAD:
			CharacterList[0]->actor.Play(ONCE, (float)skip, FALSE, TRUE);
			break;
	}
	
	CameraFollow();
}

void RenderIt(int skip)
{
   FnViewport vp;

   // render the whole scene
   vp.ID(vID);
   vp.Render3D(cID, TRUE, TRUE);
   vp.RenderSprites(sID0, FALSE, FALSE);
   

   // get camera's data
   FnCamera camera;
   camera.ID(cID);

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

   FnText text;
   text.ID(textID);

   text.Begin(vID);
   text.Write(string, 20, 220, 255, 0, 0);

   char posS[256], fDirS[256], uDirS[256], buff[256];
   sprintf(posS, "pos: %8.3f %8.3f %8.3f", pos[0], pos[1], pos[2]);
   sprintf(fDirS, "facing: %8.3f %8.3f %8.3f", fDir[0], fDir[1], fDir[2]);
   sprintf(uDirS, "up: %8.3f %8.3f %8.3f", uDir[0], uDir[1], uDir[2]);
   //sprintf(buff, "psp:(%2f, %2f, %2f)", PLAYER_STARTING_POSITION[0], PLAYER_STARTING_POSITION[1], PLAYER_STARTING_POSITION[2]);
   sprintf(buff, "MyFlag=%d", MyFlag );

   text.Write(posS, 20, 235, 255, 255, 0);
   text.Write(fDirS, 20, 250, 255, 255, 0);
   text.Write(uDirS, 20, 265, 255, 255, 0);
   text.Write(buff, 20, 280, 255, 255, 0);

   text.End();

   // swap buffer
   FySwapBuffers();
}

int Lyubu2Attack(int skip){
	if( CharacterList[0]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
		CharacterList[0]->aflag = IDLE;
		CharacterList[0]->actor.SetCurrentAction(NULL, 0, CharacterList[0]->act[CharacterList[0]->aflag].aid );
		return 0;
	}
}

int DoNothing(int skip){
	return 0;
}