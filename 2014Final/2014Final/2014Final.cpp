/*
	Important note!!
	
		How to write ActFunction for an action:
			Argument is skip and character id, return value should be either 0 or 1, any other return value will cause bug on GameAI
			return 0 means that the action has done successfully ,return 1 means that the action changed an aflag and character
			should do the ActFunction of that aflag
			You should SetCurrentAction for character(include yourself and others) if you modify its aflag
*/



#include "FlyWin32.h"
#include "lua.hpp"

#define MAX_ACTION_NUM				32
#define ACTION_NAME_SIZE			128
#define CHARACTER_TYPE_NAME_SIZE	128
#define MAX_CHARACTER_NUM			128
#define MAX_TYPE_NUM				128
#define MAX_EVENT_NUM				128

#define MOVE_BLOCK_RANGE			30
#define ENEMY_ATTACK_RANGE			80
#define ENEMY_FOLLOW_RANGE			300

#define MAX_LEVEL					13

#define ATTACK1_DAMAGE				20
#define ATTACK1_ANGLE				0.86602540378
#define ATTACK1_RANGE				120

#define FRIENDLY					0
#define HOSTILE						1

#define HP_POTION					0
#define MP_POTION					1
#define SP_POTION					2
#define ATK_POTION					3

typedef	enum DirFlag{
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	FRONTLEFT,
	BACKLEFT,
	FRONTRIGHT,
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
	int (*ActFunction)(int,int);
};

typedef struct Character{
	FnCharacter actor;
	struct Actions act[MAX_ACTION_NUM];
	int prototype;
	DirFlag dflag;
	ActFlag aflag;
	int HitPoint;
	int MaxHP;
	int HPRegen;
	int ManaPoint;
	int MaxMP;
	int MPRegen;
	int Experience;
	int Level;
	float MoveSpeed;
	// any other special effect flag
	int AttackFrameCounter;
	int Value;
	int RegenFlag;
	int NPCFlag;
};

typedef struct CharacterType{
	char name[CHARACTER_TYPE_NAME_SIZE];
	FnCharacter actor;
	struct Actions act[MAX_ACTION_NUM];
};

typedef struct Event{
	void (*Action)(int);
	char name[ACTION_NAME_SIZE];
};

struct Character *CharacterList[MAX_CHARACTER_NUM];
struct CharacterType CharacterTypeList[MAX_TYPE_NUM];
struct Event *EventList[MAX_EVENT_NUM];
unsigned int TypeListStackPointer = 0;
unsigned int CharacterListStackPointer = 0;
unsigned int EventListStackPointer = 0;

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
int board_width = 0.6 * width;
int status_width = 0.5 * width;
int MyFlag = 0;
int PauseFlag = 0;
int frame = 0;
int PlayerId;

VIEWPORTid vID;                 // the major viewport
SCENEid sID;                    // the 3D scene
OBJECTid cID, tID;
SCENEid sID0;                   // the 2D scene
OBJECTid spID0, spID1, spID2, spID3, spID4, spID5, spID6;		// the sprites for character status
OBJECTid spID7;					// the sprite for skill & item
OBJECTid spID8;					// the sprite for gold
OBJECTid spID9, spID10, spID11, spID12, spID13, spID14;			// the sprites for character skills
GAMEFX_SYSTEMid MerchantFXId[]={0, 0, 0, 0};
ROOMid terrainRoomID = FAILED_ID;
TEXTid textID = FAILED_ID;
float DIRECTION[9][3];
float DEFAULT_UDIR[3] = {0.0f, 0.0f, 1.0f};
const int EXP_DEF[14] = {0,50,100,150,200,250,300,350,400,450,500,550,600,650};
const int MAXHP_DEF[14] = {50,100,150,200,250,300,350,400,450,500,550,600,650,700};
const int MAXMP_DEF[14] = {0,20,40,60,80,100,120,140,160,180,200,220,240,260};
int Item[4] = {0,0,0,0};
const int ItemPrice[4] = {50,50,150,150};
float SP_POTION_EFFECT = 1;
int SP_POTION_TIMER = 0;
float ATK_POTION_EFFECT = 1;
int ATK_POTION_TIMER = 0;
int PurchaseFlag = -1;
int Credit = 200;
int MerchantId[4];
int SkillLevel[6] = {0,0,0,0,0,2};
const int MaxSkillLevel[6] = {2,2,2,2,2,3};
const int SkillManaCost[6] = {};
int SkillPoint = 1;
//

void RegisterKeyBinding();
void QuitGame(BYTE, BOOL4);
void PurchaseControl(BYTE, BOOL4);
void MainControl(BYTE, BOOL4);
void PauseGame(BYTE, BOOL4);
void UsePortion(int id);

void CreateScene( char Scene[], char Terrain[] );
void InitDirection();

int CheckCharacterType( char name[] );
void RegisterCharacterType( char name[] );
void RegisterCharacterAction( char type[], char name[], ActFlag aflag, int (*ActFunction)(int,int) );
void UpdateCharacterAction( int id, char name[], ActFlag aflag, int (*ActFunction)(int,int) );
void DeleteCharacter( int pointer );
int SpawnCharacter( char type[], float pos[3], DirFlag dflag );

int RegisterEvent(char name[], void (*Action)(int) );
void DeleteEvent(char name[]);

void CameraInit();
void CameraFollow();

int BlockTest( const int id );

void GameAI(int);
void RenderIt(int);

float Angle( float fdir[3], float pos[3], float tar[3] );

int PlayerAttack(int,int);
int PlayerMove(int,int);
int PlayerIdle(int,int);
int PlayerDamaged(int,int);
int PlayerDead(int,int);

int DonzoAttack(int,int);
int DonzoMove(int,int);
int DonzoIdle(int,int);
int DonzoDamaged(int,int);
int DonzoDead(int,int);

void Levelup(int);
void Purchase(int);
void PotionEffect(int);
void Regen(int);
void UpdateStatistic(int);

void LevelupFX(int);
void GetFX(GAMEFX_SYSTEMid* FXId, char loc[], char name[], int id);

void FyMain(int argc, char **argv){
	BOOL4 beOK = FyStartFlyWin32("NTU@2014 Homework #01 - Use Fly2", 0, 0, width, height, FALSE);
	
	LoadParameters( "..\\..\\NTU5\\config.cfg" );
	InitDirection();
	RegisterKeyBinding();
	
	CreateScene( "gameScene01", "terrain" );
	
	float pos[3];
	RegisterCharacterType( "Lyubu2" );
	RegisterCharacterAction( "Lyubu2", "Idle", IDLE, PlayerIdle );
	pos[0] = 3900.0f; pos[1] = -3600.0f; pos[2] = PLAYER_STARTING_POSITION[2];
	for(int i=0;i<4;i++){
		pos[1] = -3600.0f+150.0f*i;
		MerchantId[i] = SpawnCharacter( "Lyubu2", pos, BACKLEFT );
		CharacterList[MerchantId[i]]->RegenFlag = 0;
		CharacterList[MerchantId[i]]->NPCFlag = FRIENDLY;
		CharacterList[MerchantId[i]]->actor.SetCurrentAction(NULL,0,CharacterList[MerchantId[i]]->act[CharacterList[MerchantId[i]]->aflag].aid);

		GetFX(&MerchantFXId[0],"..\\..\\NTU5\\FX", "Hp_Merchant", MerchantId[0]);
		GetFX(&MerchantFXId[1],"..\\..\\NTU5\\FX", "Mp_Merchant", MerchantId[1]);
		GetFX(&MerchantFXId[2],"..\\..\\NTU5\\FX", "Sp_Merchant", MerchantId[2]);
		GetFX(&MerchantFXId[3],"..\\..\\NTU5\\FX", "Atk_Merchant", MerchantId[3]);
	}
	
	RegisterCharacterAction( "Lyubu2", "Run", MOVE, PlayerMove );
	//RegisterCharacterAction( "Lyubu2", "NormalAttack1", ATTACK, PlayerAttack );
	RegisterCharacterAction( "Lyubu2", "HeavyDamaged", DAMAGED, PlayerDamaged );
	RegisterCharacterAction( "Lyubu2", "Die", DEAD, PlayerDead );
	
	PlayerId = SpawnCharacter( "Lyubu2", PLAYER_STARTING_POSITION, FRONTLEFT );
	CharacterList[PlayerId]->HitPoint  = 100;
	CharacterList[PlayerId]->MaxHP  = 100;
	CharacterList[PlayerId]->HPRegen  = 2;
	CharacterList[PlayerId]->ManaPoint  = 20;
	CharacterList[PlayerId]->MaxMP  = 20;
	CharacterList[PlayerId]->MPRegen = 1;
	CharacterList[PlayerId]->Experience = 0;
	CharacterList[PlayerId]->Level = 1;
	CharacterList[PlayerId]->MoveSpeed = PLAYER_MOVE_SPEED;
	CharacterList[PlayerId]->RegenFlag = 1;
	CharacterList[PlayerId]->NPCFlag = FRIENDLY;
	CharacterList[PlayerId]->actor.SetCurrentAction(NULL, 0, CharacterList[PlayerId]->act[CharacterList[PlayerId]->aflag].aid);
	
	RegisterCharacterType("Donzo2");
	RegisterCharacterAction("Donzo2", "Idle", IDLE, DonzoIdle);
	RegisterCharacterAction("Donzo2", "Run", MOVE, DonzoMove);
	RegisterCharacterAction("Donzo2", "AttackL1", ATTACK, DonzoAttack);
	RegisterCharacterAction("Donzo2", "DamageL", DAMAGED, DonzoDamaged);
	RegisterCharacterAction("Donzo2", "Die", DEAD, DonzoDead);
	
	int DonzoId;
	pos[0] = PLAYER_STARTING_POSITION[0];
	pos[1] = PLAYER_STARTING_POSITION[1]+800.0f;
	pos[2] = PLAYER_STARTING_POSITION[2];
	DonzoId = SpawnCharacter("Donzo2", pos, BACKWARD);
	CharacterList[DonzoId]->HitPoint = 100;
	CharacterList[DonzoId]->Experience = 100;
	CharacterList[DonzoId]->Value = 100;
	CharacterList[DonzoId]->MoveSpeed = PLAYER_MOVE_SPEED-1.0f;
	CharacterList[DonzoId]->RegenFlag = 0;
	CharacterList[DonzoId]->NPCFlag = HOSTILE;
	CharacterList[DonzoId]->actor.SetCurrentAction(NULL, 0, CharacterList[DonzoId]->act[CharacterList[DonzoId]->aflag].aid);
	
	CameraInit();
	
	RegisterEvent( "Levelup", Levelup );
	RegisterEvent( "Purchase", Purchase);
	RegisterEvent( "PotionEffect", PotionEffect );
	RegisterEvent( "Regen", Regen );
	RegisterEvent( "UpdateStatistic", UpdateStatistic );

	FyBindTimer(0, 30.0f, GameAI, TRUE);
	FyBindTimer(1, 30.0f, RenderIt, TRUE);

	FyInvokeFly(TRUE);
}


void GameAI(int skip){
	for(int i=0;i<EventListStackPointer;i++){
		(*(EventList[i]->Action))(skip);
	}
	if( PauseFlag )return ;
	for (int i = 0; i < CharacterListStackPointer; i++){
		if( CharacterList[i] == NULL )continue ;
		if( CharacterList[i]->act[CharacterList[i]->aflag].aid )i-=(*(CharacterList[i]->act[CharacterList[i]->aflag].ActFunction))(skip,i);
	}
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
	//CharacterList[PlayerId]->actor.GetPosition(pos);
	CharacterList[PlayerId]->actor.GetPosition(pos);
	CharacterList[PlayerId]->actor.GetDirection(fDir, uDir);

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
	sprintf(buff, "SkillPoint=%d, SkillLevel[0]=%d, MyFlag=%d", SkillPoint, SkillLevel[0], MyFlag );

	text.Write(posS, 20, 235, 255, 255, 0);
	text.Write(fDirS, 20, 250, 255, 255, 0);
	text.Write(uDirS, 20, 265, 255, 255, 0);
	text.Write(buff, 20, 280, 255, 255, 0);

	//顯示等級
	char level[256];
	sprintf(level, "LEVEL %d", CharacterList[PlayerId]->Level );
	text.Write(level, 10+status_width*0.21, 10+status_width*0.143+status_width*0.025, 202, 202, 202);

	//顯示生命值
	char hp[256];
	sprintf(hp, "%d / %d", CharacterList[PlayerId]->HitPoint, CharacterList[PlayerId]->MaxHP );
	text.Write(hp, 10+status_width*0.5, 10+status_width*0.057+0.5*(status_width*0.040-16), 255, 255, 255);

	//顯示法力值
	char mp[256];
	sprintf(mp, "%d / %d", CharacterList[PlayerId]->ManaPoint, CharacterList[PlayerId]->MaxMP );
	text.Write(mp, 10+status_width*0.5, 10+status_width*0.108+0.5*(status_width*0.028-16), 255, 255, 255);

	//顯示金錢
	char gold[256];
	sprintf(gold, "%d", Credit );
	text.Write(gold, width-0.1*width-10+0.1*status_width, status_width*0.107+10-0.1*width*0.255+(0.1*width*0.205-16)/2, 255, 255, 255);

	//顯示藥品數
	char potion1[256];
	sprintf(potion1, "%d", Item[HP_POTION] );
	text.Write(potion1, 0.5*width-0.5*board_width+0.162*board_width, height-0.060*board_width, 255, 255, 255);

	/*
	char potion2[256];
	sprintf(potion2, "%d", Item[HP_POTION] );
	text.Write(potion2, 0.5*width-0.5*board_width+0.237*board_width, height-0.060*board_width, 255, 255, 255);

	char potion3[256];
	sprintf(potion3, "%d", Item[HP_POTION] );
	text.Write(potion3, 0.5*width-0.5*board_width+0.311*board_width, height-0.060*board_width, 255, 255, 255);

	char potion4[256];
	sprintf(potion4, "%d", Item[HP_POTION] );
	text.Write(potion3, 0.5*width-0.5*board_width+0.386*board_width, height-0.060*board_width, 255, 255, 255);
	*/

	char potion5[256];
	sprintf(potion5, "%d", Item[MP_POTION] );
	text.Write(potion5, 0.5*width-0.5*board_width+0.461*board_width, height-0.060*board_width, 255, 255, 255);

	/*
	char potion6[256];
	sprintf(potion6, "%d", Item[MP_POTION] );
	text.Write(potion6, 0.5*width-0.5*board_width+0.535*board_width, height-0.060*board_width, 255, 255, 255);

	char potion7[256];
	sprintf(potion7, "%d", Item[MP_POTION] );
	text.Write(potion7, 0.5*width-0.5*board_width+0.610*board_width, height-0.060*board_width, 255, 255, 255);

	char potion8[256];
	sprintf(potion8, "%d", Item[MP_POTION] );
	text.Write(potion8, 0.5*width-0.5*board_width+0.685*board_width, height-0.060*board_width, 255, 255, 255);
	*/

	char potion9[256];
	sprintf(potion9, "%d", Item[SP_POTION] );
	text.Write(potion9, 0.5*width-0.5*board_width+0.760*board_width, height-0.060*board_width, 255, 255, 255);

	char potion10[256];
	sprintf(potion10, "%d", Item[ATK_POTION] );
	text.Write(potion10, 0.5*width-0.5*board_width+0.835*board_width, height-0.060*board_width, 255, 255, 255);

	text.End();

	// swap buffer
	FySwapBuffers();
}

void RegisterKeyBinding(){
	FyDefineHotKey(FY_ESCAPE, 	QuitGame, 	 	FALSE);
	FyDefineHotKey(FY_UP, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_RIGHT, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_LEFT, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_DOWN, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_Z, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_X, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_PAUSE, 	PauseGame,	 	FALSE);
	FyDefineHotKey(FY_B, 		PurchaseControl,	FALSE);
	FyDefineHotKey(FY_SHIFT, 	MainControl, 	FALSE);
	FyDefineHotKey(FY_Q, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_W, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_E, 		MainControl, 	FALSE);
	FyDefineHotKey(FY_R, 		MainControl, 	FALSE);
}

void PurchaseControl(BYTE code, BOOL4 value){
	if( value ){
		if(PurchaseFlag>=0){
			if( ( Credit-=ItemPrice[PurchaseFlag] ) < 0 ){
				Credit+=ItemPrice[PurchaseFlag];
				/*
					play error sound effect
				*/
				return ;
			}
			Item[PurchaseFlag]++;
			/*
				Purchase FX
			*/
			}
	}
	else return ;
}

void MainControl(BYTE code, BOOL4 value){
	if( CharacterList[PlayerId]->aflag == DEAD )return ;
	float fDir[3];
	static bool kflag[4] = { false, false, false, false };
	ActFlag aflag;
	int ControlFlag = 0;
	static int ShiftLFlag = 0;
	aflag = CharacterList[PlayerId]->aflag;
	if (CharacterList[PlayerId]->aflag == ATTACK || CharacterList[PlayerId]->aflag == DAMAGED || CharacterList[PlayerId]->aflag == DEAD )ControlFlag = 1;
	if( code == FY_SHIFT ){
		if( value )ShiftLFlag = 1;
		else ShiftLFlag = 0;
	}
	if( value ){
		switch( code ){
			case FY_Z:
				/*
					I assume that Z attack is normal attack and not able to levelup
				*/
				if( ControlFlag )break;
				UpdateCharacterAction(PlayerId, "NormalAttack1", ATTACK, PlayerAttack);
				CharacterList[PlayerId]->aflag = ATTACK;
				CharacterList[PlayerId]->AttackFrameCounter = 0;
		 	    break;
			case FY_X:
				if( ShiftLFlag ){
					if( SkillPoint > 0 && SkillLevel[0]+1<=MaxSkillLevel[0] ){
						SkillLevel[0]++;
						SkillPoint--;
					}
					else{
						/*
							Play error sound effect
						*/
					}
					break;
				}
				if( !SkillLevel[0] )break;
				if( ControlFlag )break;
				UpdateCharacterAction(PlayerId, "HeavyAttack1", ATTACK, PlayerAttack);
				CharacterList[PlayerId]->aflag = ATTACK;
				CharacterList[PlayerId]->AttackFrameCounter = 0;
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
			case FY_Q:
				UsePortion(HP_POTION);
				break;
			case FY_W:
				UsePortion(MP_POTION);
				break;
			case FY_E:
				UsePortion(SP_POTION);
				break;
			case FY_R:
				UsePortion(ATK_POTION);
				break;
		}
	}
	else{
		switch( code ){
			case FY_Z:
			case FY_X:
				CharacterList[PlayerId]->aflag = IDLE;
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
			case FY_Q:
				break;
		}
	}
	if( !( CharacterList[PlayerId]->aflag == IDLE || CharacterList[PlayerId]->aflag == MOVE ) ){}
	else if( (kflag[FORWARD]^kflag[BACKWARD])|(kflag[LEFT]^kflag[RIGHT]) ){
		CharacterList[PlayerId]->aflag = MOVE;
		CharacterList[PlayerId]->dflag = DirFlag(((kflag[FORWARD]^kflag[BACKWARD])&(kflag[LEFT]^kflag[RIGHT]))<<2);
		if( CharacterList[PlayerId]->dflag )
			CharacterList[PlayerId]->dflag = DirFlag(CharacterList[PlayerId]->dflag|((!kflag[FORWARD])&kflag[BACKWARD])|(((!kflag[LEFT])&kflag[RIGHT])<<1));
		else{
			if( kflag[FORWARD]^kflag[BACKWARD] )CharacterList[PlayerId]->dflag = DirFlag((!kflag[FORWARD])&kflag[BACKWARD]);
			else if( kflag[LEFT]^kflag[RIGHT] ) CharacterList[PlayerId]->dflag = DirFlag(2 | ((!kflag[LEFT])&kflag[RIGHT]));
		}
	}
	else CharacterList[PlayerId]->aflag = IDLE;
	
	if( ControlFlag || PauseFlag ){
		CharacterList[PlayerId]->aflag = aflag;
		return ;
		}
	
	if (aflag != CharacterList[PlayerId]->aflag ){
		CharacterList[PlayerId]->actor.SetCurrentAction(NULL, 0, CharacterList[PlayerId]->act[CharacterList[PlayerId]->aflag].aid);
	}
}

void QuitGame(BYTE code, BOOL4 value){
	if (code == FY_ESCAPE) {
		if (value) {
			FyQuitFlyWin32();
		}
	}
}

void PauseGame(BYTE code, BOOL4 value){
	if (code == FY_PAUSE && value) {
		PauseFlag = !PauseFlag;
	}
}

void UsePortion(int id){
	if( PauseFlag )return ;
	if( Item[id]<1 ){
		/*
			play error sound effect
		*/
	}
	Item[id]--;
	switch(id){
		case HP_POTION:
			if( ( CharacterList[PlayerId]->HitPoint+=100 ) > CharacterList[PlayerId]->MaxHP )
				CharacterList[PlayerId]->HitPoint = CharacterList[PlayerId]->MaxHP;
			/*
				HP potion FX
			*/
			break ;
		case MP_POTION:
			if( ( CharacterList[PlayerId]->ManaPoint+=100 ) > CharacterList[PlayerId]->MaxMP )
				CharacterList[PlayerId]->ManaPoint = CharacterList[PlayerId]->MaxMP;
			/*
				MP potion FX
			*/
			break ;
		case SP_POTION:
			SP_POTION_TIMER = 300;
			break ;
		case ATK_POTION:
			ATK_POTION_TIMER = 300;
			break ;
	}
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

	// sprites for character status
	FySetTexturePath("..\\..\\NTU5\\Textures");
	FnSprite sp;

	//狀態欄
	spID0 = scene0.CreateObject(SPRITE);
	sp.ID(spID0);
	sp.SetSize(status_width, status_width*0.217);
	sp.SetImage("status", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10, height-status_width*0.217-10, 2);

	//血條
	spID1 = scene0.CreateObject(SPRITE);
	sp.ID(spID1);
	sp.SetSize(status_width*0.770, status_width*0.045);
	sp.SetImage("hp", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.203, height-status_width*0.057-status_width*0.045-10, 1);

	//法力條
	spID2 = scene0.CreateObject(SPRITE);
	sp.ID(spID2);
	sp.SetSize(status_width*0.696, status_width*0.028);
	sp.SetImage("mp", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.213, height-status_width*0.107-status_width*0.028-10, 1);

	//經驗值
	spID3 = scene0.CreateObject(SPRITE);
	sp.ID(spID3);
	sp.SetSize(status_width*0.381, status_width*0.024);
	sp.SetImage("exp", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.200, height-status_width*0.143-status_width*0.024-10, 1);

	//空血條
	spID4 = scene0.CreateObject(SPRITE);
	sp.ID(spID4);
	sp.SetSize(status_width*0.770, status_width*0.045);
	sp.SetImage("hp_empty", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.203, height-status_width*0.057-status_width*0.045-10, 0);

	//空法力條
	spID5 = scene0.CreateObject(SPRITE);
	sp.ID(spID5);
	sp.SetSize(status_width*0.696, status_width*0.028);
	sp.SetImage("mp_empty", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.213, height-status_width*0.107-status_width*0.028-10, 0);

	//空經驗值
	spID6 = scene0.CreateObject(SPRITE);
	sp.ID(spID6);
	sp.SetSize(status_width*0.381, status_width*0.024);
	sp.SetImage("exp_empty", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(10+status_width*0.200, height-status_width*0.143-status_width*0.024-10, 0);

	//技能&道具欄
	spID7 = scene0.CreateObject(SPRITE);
	sp.ID(spID7);
	sp.SetSize(board_width, 0.26*board_width);
	sp.SetImage("skill&item", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width, 0, 0);

	//金錢數
	spID8 = scene0.CreateObject(SPRITE);
	sp.ID(spID8);
	sp.SetSize(0.1*width, 0.03*width);
	sp.SetImage("gold", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(width-0.1*width-10, height-status_width*0.107-10, 0);

	//技能等級
	spID9 = scene0.CreateObject(SPRITE);
	sp.ID(spID9);
	sp.SetSize(0.025*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.263*board_width, 0.115*board_width, 1);

	spID10 = scene0.CreateObject(SPRITE);
	sp.ID(spID10);
	sp.SetSize(0.025*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.347*board_width, 0.115*board_width, 1);

	spID11 = scene0.CreateObject(SPRITE);
	sp.ID(spID11);
	sp.SetSize(0.025*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.431*board_width, 0.115*board_width, 1);

	spID12 = scene0.CreateObject(SPRITE);
	sp.ID(spID12);
	sp.SetSize(0.025*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.512*board_width, 0.115*board_width, 1);

	spID13 = scene0.CreateObject(SPRITE);
	sp.ID(spID13);
	sp.SetSize(0.025*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.620*board_width, 0.115*board_width, 1);
	
	spID14 = scene0.CreateObject(SPRITE);
	sp.ID(spID14);
	sp.SetSize(0.035*board_width, 0.014*board_width);
	sp.SetImage("black", 0, NULL, FALSE, NULL, 2, TRUE, FILTER_LINEAR);
	sp.SetPosition(0.5*width-0.5*board_width+0.704*board_width, 0.115*board_width, 1);
}

void InitDirection(){
	DIRECTION[HALT][0] = 0.0f;			DIRECTION[HALT][1] = 0.0f;			DIRECTION[HALT][2] = 0.0f;
	DIRECTION[FORWARD][0] = 1.0f;		DIRECTION[FORWARD][1] = 1.0f;		DIRECTION[FORWARD][2] = 0.0f;
	DIRECTION[BACKWARD][0] = -1.0f;		DIRECTION[BACKWARD][1] = -1.0f;		DIRECTION[BACKWARD][2] = 0.0f;
	DIRECTION[LEFT][0] = -1.0f;			DIRECTION[LEFT][1] = 1.0f;			DIRECTION[LEFT][2] = 0.0f;
	DIRECTION[RIGHT][0] = 1.0f;			DIRECTION[RIGHT][1] = -1.0f;		DIRECTION[RIGHT][2] = 0.0f;
	DIRECTION[FRONTLEFT][0] = 0.0f;		DIRECTION[FRONTLEFT][1] = 1.0f;		DIRECTION[FRONTLEFT][2] = 0.0f;
	DIRECTION[BACKLEFT][0] = -1.0f;		DIRECTION[BACKLEFT][1] = 0.0f;		DIRECTION[BACKLEFT][2] = 0.0f;
	DIRECTION[FRONTRIGHT][0] = 1.0f;	DIRECTION[FRONTRIGHT][1] = 0.0f;	DIRECTION[FRONTRIGHT][2] = 0.0f;
	DIRECTION[BACKRIGHT][0] = 0.0f;	DIRECTION[BACKRIGHT][1] = -1.0f;	DIRECTION[BACKRIGHT][2] = 0.0f;
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

void RegisterCharacterAction( char type[], char name[], ActFlag aflag, int (*ActFunction)(int, int) ){
	int TypeID;
	if( ( TypeID = CheckCharacterType( type ) ) == -1 )return ;
	CharacterTypeList[TypeID].act[aflag].aid = 1;
	strcpy( CharacterTypeList[TypeID].act[aflag].name, name );
	CharacterTypeList[TypeID].act[aflag].ActFunction = ActFunction;
	return ;
}

void UpdateCharacterAction( int id, char name[], ActFlag aflag, int (*ActFunction)(int,int) ){
	if( id > CharacterListStackPointer )return ;
	CharacterList[id]->act[aflag].aid = CharacterList[id]->actor.GetBodyAction(NULL, name);
	strcpy( CharacterList[id]->act[aflag].name, name );
	CharacterList[id]->act[aflag].ActFunction = ActFunction;
	return ;
}

void DeleteCharacter(int ptr){
	if( ptr > CharacterListStackPointer )return ;
	FnScene scene; 
	scene.ID(sID); 
	scene.DeleteCharacter(CharacterList[ptr]->actor.ID());
	/*
	if( ptr < CharacterListStackPointer )CharacterList[ptr] = CharacterList[CharacterListStackPointer];
	CharacterListStackPointer--;
	*/
	return;
}

int SpawnCharacter( char type[], float pos[3], DirFlag dflag ){
	int TypeID;
	if( CharacterListStackPointer >= MAX_CHARACTER_NUM )return -1;
	if( ( TypeID = CheckCharacterType( type ) ) == -1 )return -1;
	
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
	CharacterList[CharacterListStackPointer] = tmp;
	CharacterListStackPointer++;
	return CharacterListStackPointer-1;
}

int RegisterEvent(char name[],void (*Action)(int) ){
	int tptr = EventListStackPointer;
	for(int i=0;i<EventListStackPointer;i++)if( !strcmp(name, EventList[i]->name) ){tptr = i;	break;}
	if( tptr >= MAX_EVENT_NUM )return -1;
	struct Event* tmp = ( struct Event* )malloc(sizeof(struct Event));
	strcpy(tmp->name, name);
	tmp->Action = Action;
	if( tptr!=EventListStackPointer ){
		free(EventList[tptr]);
		EventList[tptr] = tmp;
	}
	else{
		EventList[tptr] = tmp;
		EventListStackPointer++;
	}
	return tptr;
}
void DeleteEvent(char name[]){
	for(int i=0;i<EventListStackPointer;i++)
		if( !strcmp(name, EventList[i]->name) ){
			free(EventList[i]);
			if(i!=EventListStackPointer){
				EventList[i] = EventList[EventListStackPointer];
			}
			EventListStackPointer--;
			break;
		}
}

void GetFX(GAMEFX_SYSTEMid* FXId, char loc[], char name[], int id){
	FySetGameFXPath(loc);
	if( *FXId == 0 ){
		FnScene scene(sID);
		*FXId = scene.CreateGameFXSystem();
		}
	OBJECTid baseID = CharacterList[id]->actor.GetBaseObject();
	FnGameFXSystem gxS(*FXId);
    gxS.Load(name, TRUE);
    gxS.SetParentObjectForAll(baseID);
}

void CameraInit(){
	CameraFollow();
	FnCamera camera;
	camera.ID(cID);
	float fDir[3] = { CAMERA_DEFALT_DIRECION[0]*CAMERA_DEFALT_DISTANCE, CAMERA_DEFALT_DIRECION[1]*CAMERA_DEFALT_DISTANCE, CAMERA_DEFALT_DIRECION[2]*CAMERA_DEFALT_DISTANCE };
	float uDir[3], tDir[3];
	CharacterList[PlayerId]->actor.GetDirection(NULL, uDir);
	FyCross( tDir, fDir, uDir );
	FyCross( uDir, tDir, fDir );
	camera.SetDirection(fDir, uDir);
}

void CameraFollow(){
	static float dis[3] = { -1*CAMERA_DEFALT_DIRECION[0]*CAMERA_DEFALT_DISTANCE, -1*CAMERA_DEFALT_DIRECION[1]*CAMERA_DEFALT_DISTANCE, -1*CAMERA_DEFALT_DIRECION[2]*CAMERA_DEFALT_DISTANCE };
	float pos[3];
	FnCamera camera;
	camera.ID(cID);
	CharacterList[PlayerId]->actor.GetPosition(pos);
	pos[0]+=dis[0];	pos[1]+=dis[1];	pos[2]+=dis[2];
	camera.SetPosition(pos);
}

int BlockTest(const int id){
	float pos[3] , fdir[3], tpos[3];
	CharacterList[id]->actor.GetPosition(pos);
	CharacterList[id]->actor.GetDirection(fdir,NULL);
	FyNormalizeVector3(fdir);
	pos[0]+=fdir[0]*CharacterList[id]->MoveSpeed;	pos[1]+=fdir[1]*CharacterList[id]->MoveSpeed;	pos[2]+=fdir[2]*CharacterList[id]->MoveSpeed;
	for(int i=0;i<CharacterListStackPointer;i++){
		if( i==id )continue ;
		if( CharacterList[id]->aflag == DEAD )continue ;
		CharacterList[i]->actor.GetPosition(tpos); 
		if( FyDistance(pos,tpos) <= MOVE_BLOCK_RANGE )return 1;
	}
	return 0;
}

float Angle( float fdir[3], float pos[3], float tar[3] ){
	float tfdir[3];
	tfdir[0] = tar[0]-pos[0];	tfdir[1] = tar[1]-pos[1];	tfdir[2] = tar[2]-pos[2];
	return (float)FyDot( fdir, tfdir )/((sqrt(fdir[0]*fdir[0]+fdir[1]*fdir[1]+fdir[2]*fdir[2]))*(sqrt(tfdir[0]*tfdir[0]+tfdir[1]*tfdir[1]+tfdir[2]*tfdir[2])));
}

int PlayerAttack(int skip, int id){
	float ipos[3], epos[3];
	if( CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
		CharacterList[id]->aflag = IDLE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[IDLE].aid);
		return 1;
	}
	/*
		Attack FX
	*/
	CharacterList[id]->AttackFrameCounter++;
	if( CharacterList[id]->AttackFrameCounter != 1 )return 0;
	CharacterList[id]->actor.GetPosition(ipos);
	for(int i=0;i<CharacterListStackPointer;i++){
		if( CharacterList[i]->NPCFlag == FRIENDLY )continue;
		CharacterList[i]->actor.GetPosition(epos);
		if( FyDistance(ipos, epos)<=ATTACK1_RANGE && CharacterList[i]->aflag != DEAD ){
			if( ( CharacterList[i]->HitPoint -= 50 ) > 0 ){
				CharacterList[i]->aflag = DAMAGED;
				CharacterList[i]->actor.SetCurrentAction(NULL, 0, CharacterList[i]->act[DAMAGED].aid);
				}
			else{
				CharacterList[i]->aflag = DEAD;
				CharacterList[PlayerId]->Experience += CharacterList[i]->Experience;
				Credit += CharacterList[i]->Value;
				CharacterList[i]->actor.SetCurrentAction(NULL, 0, CharacterList[i]->act[DEAD].aid);
			}
		}
	}
	return 0;
}

int PlayerMove(int skip, int id){
	CharacterList[id]->actor.SetDirection( DIRECTION[CharacterList[id]->dflag], NULL );
	if( !BlockTest( id ) )CharacterList[id]->actor.MoveForward(CharacterList[id]->MoveSpeed*SP_POTION_EFFECT, true, true, 0.0f, true);
	CharacterList[id]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
	CameraFollow();
	return 0;
}

int PlayerIdle(int skip, int id){
	CharacterList[id]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
	return 0;
}

int PlayerDamaged(int skip,int id){
	if( CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
		CharacterList[id]->aflag = IDLE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[CharacterList[id]->aflag].aid);
		return 1;
	}
	return 0;
}
int PlayerDead(int skip,int id){
	CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE);
	return 0;
}

int DonzoAttack(int skip,int id){
	float ipos[3], epos[3];
	if( CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
		CharacterList[id]->aflag = IDLE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[IDLE].aid);
		return 1;
	}
	/*
		Attack FX
	*/
	CharacterList[id]->AttackFrameCounter++;
	if( CharacterList[id]->AttackFrameCounter != 1	/*	set attack timing	*/ )return 0;
	CharacterList[PlayerId]->actor.GetPosition(ipos);
	CharacterList[id]->actor.GetPosition(epos);
	if( FyDistance(ipos, epos)<=ENEMY_ATTACK_RANGE && CharacterList[PlayerId]->aflag != DEAD ){
		if( ( CharacterList[PlayerId]->HitPoint -= 30 ) > 0 ){
			CharacterList[PlayerId]->aflag = DAMAGED;
			CharacterList[PlayerId]->actor.SetCurrentAction(NULL, 0, CharacterList[PlayerId]->act[DAMAGED].aid);
			}
		else{
			CharacterList[PlayerId]->aflag = DEAD;
			CharacterList[PlayerId]->actor.SetCurrentAction(NULL, 0, CharacterList[PlayerId]->act[DEAD].aid);
		}
	}
	return 0;
}

int DonzoMove(int skip,int id){
	float pos[3], tpos[3], fdir[3];
	CharacterList[PlayerId]->actor.GetPosition(pos);
	CharacterList[id]->actor.GetPosition(tpos);
	if( FyDistance( pos, tpos ) > 500.0f || CharacterList[PlayerId]->aflag == DEAD ){
		CharacterList[id]->aflag = IDLE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[IDLE].aid);
		return 1;
	}
	if( FyDistance( pos, tpos ) < 100.0f ){
		CharacterList[id]->aflag = ATTACK;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[ATTACK].aid );
		CharacterList[id]->AttackFrameCounter = 0;
		return 1;
	}
	fdir[0] = pos[0]-tpos[0];	fdir[1] = pos[1]-tpos[1];	fdir[2] = pos[2]-tpos[2];
	CharacterList[id]->actor.SetDirection(fdir, NULL);
	if( !BlockTest( id ) )CharacterList[id]->actor.MoveForward(CharacterList[id]->MoveSpeed, true, true, 0.0f, true);
	CharacterList[id]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
	return 0;
}

int DonzoIdle(int skip,int id){
	float pos[3], tpos[3];
	CharacterList[PlayerId]->actor.GetPosition(pos);
	CharacterList[id]->actor.GetPosition(tpos);
	if( FyDistance( pos, tpos ) < 500.0f && CharacterList[PlayerId]->aflag != DEAD ){
		CharacterList[id]->aflag = MOVE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[MOVE].aid );
		return 1;
	}
	CharacterList[id]->actor.Play(LOOP, (float)skip, FALSE, TRUE);
	return 0;
}

int DonzoDamaged(int skip,int id){
	if( CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE) == false ){
		CharacterList[id]->aflag = IDLE;
		CharacterList[id]->actor.SetCurrentAction(NULL, 0, CharacterList[id]->act[CharacterList[id]->aflag].aid);
	}
	return 0;
}

int DonzoDead(int skip,int id){
	CharacterList[id]->actor.Play(ONCE, (float)skip, FALSE, TRUE);
	return 0;
}

void Levelup(int skip){
	int tmp = CharacterList[PlayerId]->Level;
	for(;CharacterList[PlayerId]->Experience>=EXP_DEF[CharacterList[PlayerId]->Level]&&CharacterList[PlayerId]->Level<=MAX_LEVEL;
	CharacterList[PlayerId]->Level++){
		CharacterList[PlayerId]->Experience-=EXP_DEF[CharacterList[PlayerId]->Level];
	}
	if( tmp == CharacterList[PlayerId]->Level )return ;
	CharacterList[PlayerId]->MaxHP = MAXHP_DEF[CharacterList[PlayerId]->Level];
	CharacterList[PlayerId]->HitPoint = CharacterList[PlayerId]->MaxHP;
	CharacterList[PlayerId]->MaxMP = MAXMP_DEF[CharacterList[PlayerId]->Level];
	CharacterList[PlayerId]->ManaPoint = CharacterList[PlayerId]->MaxMP;
	CharacterList[PlayerId]->Experience = 0;
	SkillPoint+= CharacterList[PlayerId]->Level-tmp;
	/*
		levelup FX
	*/
	RegisterEvent("LevelupFX", LevelupFX);
}

void LevelupFX(int skip){
	static GAMEFX_SYSTEMid LevelupFXId = 0;
	GetFX(&LevelupFXId,"..\\..\\NTU5\\FX\\FX2\\LvUp", "LvUp1", PlayerId);
	FnGameFXSystem gxS(LevelupFXId);
	if( gxS.Play((float) skip, ONCE) == false ){
		DeleteEvent("LevelupFX");
	}
}

void Purchase(int skip){
	float pos[3], mpos[3];

	CharacterList[PlayerId]->actor.GetPosition(pos);
	for(int i=0;i<4;i++){
		CharacterList[MerchantId[i]]->actor.GetPosition(mpos);
		if( FyDistance(pos,mpos) < 70 ){
			PurchaseFlag = i;

			FnGameFXSystem gxS(MerchantFXId[i]);
			gxS.Play((float)skip, LOOP);
			
			return ;
		}
		else{
			FnGameFXSystem gxS(MerchantFXId[i]);
			gxS.Play((float)skip, ONCE);
		}
	}

	PurchaseFlag = -1;
	/*
	Hide Purchase UI
	*/
	return ;
}

void PotionEffect(int skip){
	if( SP_POTION_TIMER == 0 ){ SP_POTION_EFFECT = 1; }
	else{
		SP_POTION_TIMER--;
		SP_POTION_EFFECT = 1.5;
		/*
			Speed potion FX
		*/
	}
	if( ATK_POTION_TIMER == 0 ){ ATK_POTION_EFFECT = 1; }
	else{
		ATK_POTION_TIMER--;
		ATK_POTION_EFFECT = 1.5;
		/*
			Attack potion FX
		*/
	}
}

void Regen(int skip){
	static int FrameCounter = 0;
	if( (FrameCounter++)<30 )return ;
	FrameCounter = 0;
	for(int i=0;i<CharacterListStackPointer;i++){
		if(CharacterList[i] == NULL)continue ;
		if(CharacterList[i]->aflag == DEAD)continue ;
		if(CharacterList[i]->RegenFlag){
			if( ( CharacterList[i]->HitPoint += CharacterList[i]->HPRegen ) > CharacterList[i]->MaxHP )
				CharacterList[i]->HitPoint = CharacterList[i]->MaxHP;
			if( ( CharacterList[i]->ManaPoint += CharacterList[i]->MPRegen ) > CharacterList[i]->MaxMP )
				CharacterList[i]->ManaPoint = CharacterList[i]->MaxMP;
		}
	}
}

void UpdateStatistic(int skip){
	FnSprite sp;
	float ratio;

	//血條
	ratio = (float)CharacterList[PlayerId]->HitPoint / CharacterList[PlayerId]->MaxHP;
	if( ratio < 0 ) ratio = 0;
	sp.ID(spID1);
	sp.SetSize(status_width*0.770*ratio, status_width*0.045);
	
	//法力條
	ratio = (float)CharacterList[PlayerId]->ManaPoint / CharacterList[PlayerId]->MaxMP;
	if( ratio < 0 ) ratio = 0;
	sp.ID(spID2);
	sp.SetSize(status_width*0.696*ratio, status_width*0.028);

	//經驗值
	ratio = (float)CharacterList[PlayerId]->Experience / EXP_DEF[CharacterList[PlayerId]->Level];
	if( ratio > 1 ) ratio = 1;
	sp.ID(spID3);
	sp.SetSize(status_width*0.381*ratio, status_width*0.024);

	//技能等級
	sp.ID(spID9);
	if( SkillLevel[0] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.275*board_width, 0.115*board_width, 1);
	if( SkillLevel[0] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.288*board_width, 0.115*board_width, 1);

	sp.ID(spID10);
	if( SkillLevel[1] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.359*board_width, 0.115*board_width, 1);
	if( SkillLevel[1] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.372*board_width, 0.115*board_width, 1);

	sp.ID(spID11);
	if( SkillLevel[2] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.441*board_width, 0.115*board_width, 1);
	if( SkillLevel[2] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.453*board_width, 0.115*board_width, 1);

	sp.ID(spID12);
	if( SkillLevel[3] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.524*board_width, 0.115*board_width, 1);
	if( SkillLevel[3] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.536*board_width, 0.115*board_width, 1);

	sp.ID(spID13);
	if( SkillLevel[4] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.632*board_width, 0.115*board_width, 1);
	if( SkillLevel[4] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.644*board_width, 0.115*board_width, 1);
	
	sp.ID(spID14);
	if( SkillLevel[5] == 1 )
		sp.SetPosition(0.5*width-0.5*board_width+0.716*board_width, 0.115*board_width, 1);
	if( SkillLevel[5] == 2 )
		sp.SetPosition(0.5*width-0.5*board_width+0.726*board_width, 0.115*board_width, 1);
	if( SkillLevel[5] == 3 ){
		sp.SetSize(0.020*board_width, 0.014*board_width);
		sp.SetPosition(0.5*width-0.5*board_width+0.736*board_width, 0.115*board_width, 1);
	}
}