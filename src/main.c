/*
A game programmed for Ludum Dare 49.
Done using the base code for The Ruins of Cuglatr, my Ludum Dare 36 entry.
Theme: Unstable
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <conio.h>
#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <varargs.h>

#include "stretchy_buffer.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 50

#define BASE_SKILL_POINTS 48
#define MIN_SKILL_POINTS 36

const WORD BG_WHITE = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY;
const WORD BG_BLACK = 0;
const WORD BG_GREY = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
const WORD BG_BROWN = BACKGROUND_RED | BACKGROUND_GREEN;
const WORD BG_YELLOW = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
const WORD BG_MAGENTA = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY;
const WORD BG_PURPLE = BACKGROUND_RED | BACKGROUND_BLUE;
const WORD BG_DARK_CYAN = BACKGROUND_BLUE | BACKGROUND_GREEN;
const WORD BG_CYAN = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;

const WORD FG_WHITE = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD FG_BLACK = 0;
const WORD FG_GREY = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
const WORD FG_BROWN = FOREGROUND_RED | FOREGROUND_GREEN;
const WORD FG_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_MAGENTA = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
const WORD FG_PURPLE = FOREGROUND_RED | FOREGROUND_BLUE;
const WORD FG_DARK_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN;
const WORD FG_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
const WORD FG_MAROON = FOREGROUND_RED;
const WORD FG_GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
const WORD FG_DARK_GREEN = FOREGROUND_GREEN;

// ═  205
// ║  186
// ╗  187
// ╚  200
// ╝  188
// ╔  201
// ╦  203
// ╩  202
// ╠  204
// ╣  185
// ╬  206
// /n 10
// _  32

typedef enum {
	CC_SOLDIER,
	CC_SCOUT,
	CC_SCHOLAR,
	CC_DEV_TEST,
	NUM_CLASSES
} CharacterClass;

char* classNames[] = {
	"Soldier",
	"Scout",
	"Scholar",
	"DEVELOPER"
};

typedef enum {
	G_HANDAXE,
	G_PISTOL,
	G_LANTERN,
	G_RATION,
	G_ROPE,
	G_TOWEL,
	G_LIGHT_ARMOR,
	G_RIFLE,
	G_FLASHLIGHT,
	G_SWORD,
	G_MONEY,
	G_MAP,
	NUM_GEAR
} Gear;

typedef enum {
	CMP_SCHOLAR,
	CMP_ELF,
	CMP_GOBLIN,
	NUM_COMPANIONS
} Companions;

typedef enum {
	SKL_FIGHT,
	SKL_SHOOT,
	SKL_ACROBATICS,
	SKL_STEALTH,
	SKL_INVESTIGATE,
	SKL_KNOWLEDGE,
	SKL_CRAFT,
	SKL_SURVIVAL,
	SKL_EMPATHY,
	SKL_INTIMIDATE,
	SKL_BARTER,
	SKL_LIE,
	NUM_SKILLS
} Skills;

typedef struct {
	CharacterClass class;

	// stats
	uint8_t stat_physicalDie;
	uint8_t stat_mentalDie;
	uint8_t stat_socialDie;

	uint8_t wounds_physical;
	uint8_t wounds_mental;
	uint8_t wounds_social;

	// skills
	//  physical
	uint8_t ps_fight;
	uint8_t ps_shoot;
	uint8_t ps_acrobatics;
	uint8_t ps_stealth;
	//  mental
	uint8_t ms_investigate;
	uint8_t ms_knowledge;
	uint8_t ms_craft;
	uint8_t ms_survival;
	//  social
	uint8_t ss_empathy;
	uint8_t ss_intimidate;
	uint8_t ss_barter;
	uint8_t ss_lie;

	uint32_t flags;

	uint8_t skillPointsLeft;

	uint8_t gear[NUM_GEAR];

	boolean hasCompanion[NUM_COMPANIONS];
} Character;

Character character;

void turnOnFlag( int id )
{
	if( id >= 32 ) return;

	character.flags |= ( 1 << id );
}

void turnOffFlag( int id )
{
	if( id >= 32 ) return;

	character.flags &= ~( 1 << id );
}

boolean isFlagOn( int id )
{
	if( id >= 32 ) return false;

	return ( ( character.flags & ( 1 << id ) ) > 0 );
}

typedef struct {
	HANDLE write;
	HANDLE read;
	CHAR_INFO buffer[SCREEN_WIDTH*SCREEN_HEIGHT];
} Screen;
Screen screen;

typedef enum {
	IN_NONE,
	IN_UP,
	IN_DOWN,
	IN_LEFT,
	IN_RIGHT,
	IN_ENTER,
	IN_SPACE,
	IN_PLUS,
	IN_MINUS,
	IN_OPT_1,
	IN_OPT_2,
	IN_OPT_3,
	IN_OPT_4,
	IN_OPT_5,
	IN_OPT_6,
	IN_OPT_7,
	IN_OPT_8,
	IN_OPT_9,
	IN_C,
	IN_H,
	IN_OTHER
} Input;

SMALL_RECT windowSize = { 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 };
SMALL_RECT renderArea = { 1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };
SMALL_RECT safeWriteArea = { 2, 2, SCREEN_WIDTH - 3, SCREEN_HEIGHT - 3 };
COORD topLeft = { 0, 0 };
COORD bufferSize = { SCREEN_WIDTH, SCREEN_HEIGHT };

char* topTitle = NULL;

#define SCREEN_POS( x, y ) ( ( x ) + ( SCREEN_WIDTH * ( y ) ) )

char tempStrBuffer[2048];
char otherTempStrBuffer[2048];

typedef void (*Scene)( void );
Scene currentScene;
Scene storedScene;
Scene nextScene;

int stolenItems[2];
Companions chosenCmp;

typedef enum {
	R_FAILURE,
	R_COSTLY_SUCCESS,
	R_SUCCESS
} Result;

boolean isCharacterClass( CharacterClass c )
{
	return ( character.class == c ) || ( character.class == CC_DEV_TEST );
}

boolean contains( COORD c, SMALL_RECT area )
{
	return ( ( c.X <= area.Right ) && ( c.X >= area.Left ) && ( c.Y <= area.Bottom ) && ( c.Y >= area.Top ) );
}

void simplePutChar( CHAR c, WORD attr, SHORT x, SHORT y )
{
	int idx = SCREEN_POS( x, y );
	screen.buffer[idx].Attributes = attr;
	screen.buffer[idx].Char.AsciiChar = c;
}

void putChar( CHAR c, WORD attr, COORD pos, SMALL_RECT border )
{
	if( contains( pos, border ) ) {
		int idx = SCREEN_POS( pos.X, pos.Y );
		screen.buffer[idx].Attributes = attr;
		screen.buffer[idx].Char.AsciiChar = c;
	}
}

COORD vdrawStringIgnoreSize( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, va_list args )
{
	vsnprintf( otherTempStrBuffer, ARRAYSIZE( otherTempStrBuffer ), str, args );
	SHORT startX = x;
	size_t len = strlen( otherTempStrBuffer );
	COORD endPoint;
	endPoint.X = x;
	endPoint.Y = y;

	for( size_t i = 0; i < len; ++i ) {
		if( otherTempStrBuffer[i] != '\n' ) {
			putChar( otherTempStrBuffer[i], attributes, endPoint, border );
			++endPoint.X;
		}

		if( ( otherTempStrBuffer[i] == '\n' ) || ( endPoint.X > border.Right ) ) {
			endPoint.X = startX;
			++endPoint.Y;
		}

		if( endPoint.Y > border.Bottom ) {
			return endPoint;
		}
	}

	return endPoint;
}

COORD drawStringIgnoreSize( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, ... )
{
	COORD c;
	va_list args;
	va_start( args, attributes );
	c = vdrawStringIgnoreSize( x, y, border, str, attributes, args );
	va_end( args );
	return c;
}

COORD vdrawString( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, va_list args )
{
	vsnprintf( otherTempStrBuffer, ARRAYSIZE( otherTempStrBuffer ), str, args );
#define NEWLINE { endPoint.X = startX; ++endPoint.Y; }
	COORD endPoint;
	SHORT startX = x;
	size_t len = strlen( otherTempStrBuffer );
	size_t strPos = 0;
	endPoint.X = x;
	endPoint.Y = y;

	char* copy = strncpy( &( tempStrBuffer[0] ), otherTempStrBuffer, ARRAYSIZE( tempStrBuffer ) );

	if( otherTempStrBuffer[0] == ' ' ) {
		putChar( ' ', attributes, endPoint, border );
		++( endPoint.X );
		++strPos;
		if( endPoint.X > border.Right ) {
			NEWLINE
		}
	}

	char* tok = strtok( tempStrBuffer, " " );
	
	while( tok != NULL ) {
		size_t tokLen = strlen( tok );

		// first see if there's enough room to write the word
		if( (size_t)( border.Right - endPoint.X ) < tokLen ) {
			endPoint.X = startX;
			++endPoint.Y;
		} else {
			for( size_t i = 0; i < tokLen; ++i ) {
				if( tok[i] == '\n' ) {
					NEWLINE
				} else {
					putChar( tok[i], attributes, endPoint, border );
					++endPoint.X;
					++strPos;
				}
			}

			// if we're not at the end of the string, that means we ran into a space, write it out
			if( otherTempStrBuffer[strPos] != 0 ) {
				putChar( ' ', attributes, endPoint, border );
				++endPoint.X;
				++strPos;
			}
			tok = strtok( NULL, " " );
		}

		if( endPoint.X > border.Right ) {
			NEWLINE
		}

		if( endPoint.Y > border.Bottom ) {
			return endPoint;
		}
	}

	return endPoint;

#undef NEWLINE
}

COORD drawString( SHORT x, SHORT y, SMALL_RECT border, const char* str, WORD attributes, ... )
{
	COORD c;
	va_list args;
	va_start( args, attributes );
	c = vdrawString( x, y, border, str, attributes, args );
	va_end( args );
	return c;
}

void centerStringHoriz( SMALL_RECT area, SHORT y, const char* str, WORD attributes, ... )
{
	va_list args;
	va_start( args, attributes );
	// want to measure the string by it's longest line
	char* copy = strncpy( &( tempStrBuffer[0] ), str, ARRAYSIZE( tempStrBuffer ) );
	SHORT x = 0;
	size_t maxLen = 0;
	char* tok = strtok( tempStrBuffer, "\n" );
	while( tok != NULL ) {
		size_t len = strlen( tok );
		if( len > maxLen ) {
			maxLen = len;
			size_t strMid = len / 2;
			SHORT renderMid = ( ( area.Right + area.Left ) / 2 );
			x = renderMid - (SHORT)strMid;
		}

		tok = strtok( NULL, "\n" );
	}

	vdrawStringIgnoreSize( x, y, area, str, attributes, args );
	va_end( args );
}

void drawBorder( )
{
	WORD attr = FG_GREY | BG_BLACK;
	// draw corners
	simplePutChar( 201, attr, 0, 0 );
	simplePutChar( 200, attr, 0, SCREEN_HEIGHT - 1 );
	simplePutChar( 187, attr, SCREEN_WIDTH - 1, 0 );
	simplePutChar( 188, attr, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1 );

	// draw top and bottom
	for( int x = 1; x < SCREEN_WIDTH-1; ++x ) {
		simplePutChar( 205, attr, x, SCREEN_HEIGHT - 1 );
		simplePutChar( 205, attr, x, 0 );
	}

	// draw left and right
	for( int y = 1; y < SCREEN_HEIGHT-1; ++y ) {
		simplePutChar( 186, attr, 0, y );
		simplePutChar( 186, attr, SCREEN_WIDTH - 1, y );
	}

	// draw top label
	if( topTitle != NULL ) {
		centerStringHoriz( windowSize, 0, topTitle, FG_WHITE | BG_BLACK );
	}
}

void startDraw( void )
{
	memset( screen.buffer, 0, sizeof( screen.buffer[0] ) * ARRAYSIZE( screen.buffer ) );
	drawBorder( );
}

void endDraw( void )
{
	WriteConsoleOutput( screen.write, screen.buffer, bufferSize, topLeft, &windowSize );
}

Input getNextInput( void )
{
	DWORD evtCnt = 0;
	DWORD evtRead = 0;

	GetNumberOfConsoleInputEvents( screen.read, &evtCnt );
	if( evtCnt <= 0 ) {
		return IN_NONE;
	}

	INPUT_RECORD inputRec;
	ReadConsoleInput( screen.read, &inputRec, 1, &evtRead );
	if( evtRead <= 0 ) {
		return IN_NONE;
	}

	if( ( inputRec.EventType == KEY_EVENT ) && inputRec.Event.KeyEvent.bKeyDown ) {
		if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_UP ) {
			return IN_UP;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN ) {
			return IN_DOWN;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_LEFT ) {
			return IN_LEFT;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT ) {
			return IN_RIGHT;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_RETURN ) {
			return IN_ENTER;
		} else if( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_SPACE ) {
			return IN_SPACE;
		} else if( ( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_ADD ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '=' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '+' ) ) {
			return IN_PLUS;
		} else if( ( inputRec.Event.KeyEvent.wVirtualKeyCode == VK_SUBTRACT ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '-' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == '_' ) ) {
			return IN_MINUS;
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar >= '1' ) &&
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar <= '9' ) ) {
			return ( IN_OPT_1 + ( inputRec.Event.KeyEvent.uChar.AsciiChar - '1' ) );
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'c' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'C' ) ) {
			return IN_C;
		} else if( ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'h' ) ||
				   ( inputRec.Event.KeyEvent.uChar.AsciiChar == 'H' ) ) {
			return IN_H;
		}
		return IN_OTHER;
	}

	return IN_NONE;
}

void waitForAnyInput( void )
{
	Input input;
	do {
		input = getNextInput( );
	} while( input == IN_NONE );
}

void eatAllInputs( void )
{
	Input input;
	do {
		input = getNextInput( );
	} while( input != IN_NONE );
}

void testDevCharacter( )
{
	character.class = CC_DEV_TEST;

	character.stat_physicalDie = 8;
	character.stat_mentalDie = 8;
	character.stat_socialDie = 8;

	character.ps_fight = 4;
	character.ps_shoot = 4;
	character.ps_acrobatics = 40; // want to carry all the things
	character.ps_stealth = 4;
	character.ms_investigate = 4;
	character.ms_knowledge = 4;
	character.ms_craft = 4;
	character.ms_survival = 4;
	character.ss_empathy = 4;
	character.ss_intimidate = 4;
	character.ss_barter = 4;
	character.ss_lie = 4;

	character.flags = 0;

	character.skillPointsLeft = 0;

	character.wounds_physical = 0;
	character.wounds_mental = 0;
	character.wounds_social = 0;

	for( int i = 0; i < NUM_GEAR; ++i ) {
		character.gear[i] = 2;
	}

	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		character.hasCompanion[i] = true;
	}
}

void initCharacter( )
{
	character.class = CC_SCOUT;

	character.stat_physicalDie = 8;
	character.stat_mentalDie = 8;
	character.stat_socialDie = 8;

	character.ps_fight = 0;
	character.ps_shoot = 0;
	character.ps_acrobatics = 0;
	character.ps_stealth = 0;
	character.ms_investigate = 0;
	character.ms_knowledge = 0;
	character.ms_craft = 0;
	character.ms_survival = 0;
	character.ss_empathy = 0;
	character.ss_intimidate = 0;
	character.ss_barter = 0;
	character.ss_lie = 0;

	character.flags = 0;

	character.skillPointsLeft = BASE_SKILL_POINTS;

	character.wounds_physical = 0;
	character.wounds_mental = 0;
	character.wounds_social = 0;

	for( int i = 0; i < NUM_GEAR; ++i ) {
		character.gear[i] = 0;
	}

	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		character.hasCompanion[i] = false;
	}
}

void increaseStat( uint8_t* stat )
{
	if( (*stat) >= 12 ) {
		character.skillPointsLeft += 4;
	} else {
		(*stat) += 2;
	}
}

void decreaseStat( uint8_t* stat )
{
	if( (*stat) <= 4 )  {
		if( ( character.skillPointsLeft - 4 ) >= MIN_SKILL_POINTS ) {
			character.skillPointsLeft -= 4;
		}
	} else {
		(*stat) -= 2;
	}
}

#define SHUFFLE( a, type ) { \
		for( int i = 0; i < ( ARRAYSIZE( a ) - 1 ); ++i ) { \
			int swap = i + ( rand( ) % ( ARRAYSIZE( a ) - ( i + 1 ) ) ); \
			type temp = a[i]; a[i] = a[swap]; a[swap] = temp; \
		} \
	}

#define SHUFFLE_SB( a, type ) { \
		for( int i = 0; i < ( sb_count( a ) - 1 ); ++i ) { \
			\
				int swap = i + ( rand( ) % ( sb_count( a ) - ( i + 1 ) ) ); \
				type temp = a[i]; a[i] = a[swap]; a[swap] = temp; \
		} \
	}

#define NUM_SKILLS 12

typedef struct {
	char* name;
	uint8_t physical;
	uint8_t mental;
	uint8_t social;
} CompanionData;

CompanionData companionsData[NUM_COMPANIONS];

void setupCompanionData( void )
{
	companionsData[CMP_SCHOLAR].name = "Freda";
	companionsData[CMP_SCHOLAR].physical = 1;
	companionsData[CMP_SCHOLAR].mental = 2;
	companionsData[CMP_SCHOLAR].social = 1;

	companionsData[CMP_GOBLIN].name = "Zobd";
	companionsData[CMP_GOBLIN].physical = 3;
	companionsData[CMP_GOBLIN].mental = 0;
	companionsData[CMP_GOBLIN].social = 0;

	companionsData[CMP_ELF].name = "Noniac";
	companionsData[CMP_ELF].physical = 1;
	companionsData[CMP_ELF].mental = 0;
	companionsData[CMP_ELF].social = 2;
}

typedef struct {
	char* name;
	char* description;
	boolean availableAtStore;
	uint8_t bonus;
} GearData;

GearData gearData[NUM_GEAR];

void setupGearData( void )
{
	gearData[G_HANDAXE].name = "Handaxe";
	gearData[G_HANDAXE].description = "A small axe, not a very good weapon but can be used as one in a pinch.";
	gearData[G_HANDAXE].availableAtStore = true;
	gearData[G_HANDAXE].bonus = 1;

	gearData[G_PISTOL].name = "Pistol";
	gearData[G_PISTOL].description = "A small firearm, noisy but able to drop most things in a single shot.";
	gearData[G_PISTOL].availableAtStore = true;
	gearData[G_PISTOL].bonus = 2;

	gearData[G_LANTERN].name = "Lantern";
	gearData[G_LANTERN].description = "A glass lantern, useful for when you have to go somewhere with no light.";
	gearData[G_LANTERN].availableAtStore = true;

	gearData[G_RATION].name = "Ration";
	gearData[G_RATION].description = "Perserved food. A necessity if you're out on the road.";
	gearData[G_RATION].availableAtStore = true;

	gearData[G_ROPE].name = "Rope";
	gearData[G_ROPE].description = "A long length of rope with multiple uses.";
	gearData[G_ROPE].availableAtStore = true;

	gearData[G_TOWEL].name = "Towel";
	gearData[G_TOWEL].description = "An ancient holy book has a commandment saying you should never go anywhere without one.";
	gearData[G_TOWEL].availableAtStore = true;

	gearData[G_LIGHT_ARMOR].name = "Light Armor";
	gearData[G_LIGHT_ARMOR].description = "Scraps of leather fitted together into crude armor. Better than nothing.";
	gearData[G_LIGHT_ARMOR].availableAtStore = true;

	gearData[G_RIFLE].name = "Rifle";
	gearData[G_RIFLE].description = "A sturdy long range fire arm.";
	gearData[G_RIFLE].availableAtStore = false;
	gearData[G_RIFLE].bonus = 3;

	gearData[G_FLASHLIGHT].name = "Flashlight";
	gearData[G_FLASHLIGHT].description = "A torch that runs on electricity.";
	gearData[G_FLASHLIGHT].availableAtStore = false;

	gearData[G_SWORD].name = "Sword";
	gearData[G_SWORD].description = "A long bladed weapon.";
	gearData[G_SWORD].availableAtStore = false;
	gearData[G_SWORD].bonus = 3;

	gearData[G_MONEY].name = "Money";
	gearData[G_MONEY].description = "A handful of currency.";
	gearData[G_MONEY].availableAtStore = true;

	gearData[G_MAP].name = "Map";
	gearData[G_MAP].description = "A map of the region surrounding where you grew up.";
	gearData[G_MAP].availableAtStore = false;
}

typedef struct {
	uint8_t* value;
	uint8_t* attrValue;
	char* name;
	char* description;
	char* abbreviation;
} SkillData;

SkillData skillsData[NUM_SKILLS];

void setupSkillsData( void )
{
	skillsData[SKL_FIGHT].value = &( character.ps_fight );
	skillsData[SKL_FIGHT].name = "Fight";
	skillsData[SKL_FIGHT].description = "Your ability in melee combat, whether with hands or weapons.";
	skillsData[SKL_FIGHT].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_FIGHT].abbreviation = "FGH";

	skillsData[SKL_SHOOT].value = &( character.ps_shoot );
	skillsData[SKL_SHOOT].name = "Shoot";
	skillsData[SKL_SHOOT].description = "How good you are at using any sort of ranged weapons.";
	skillsData[SKL_SHOOT].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_SHOOT].abbreviation = "SHT";

	skillsData[SKL_ACROBATICS].value = &( character.ps_acrobatics );
	skillsData[SKL_ACROBATICS].name = "Acrobatics";
	skillsData[SKL_ACROBATICS].description = "The measure of your general physical training and how good you are at various physical tasks.";
	skillsData[SKL_ACROBATICS].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_ACROBATICS].abbreviation = "ACR";

	skillsData[SKL_STEALTH].value = &( character.ps_stealth );
	skillsData[SKL_STEALTH].name = "Stealth";
	skillsData[SKL_STEALTH].description = "How good you are at escaping notice when you don't want to be seen.";
	skillsData[SKL_STEALTH].attrValue = &( character.stat_physicalDie );
	skillsData[SKL_STEALTH].abbreviation = "STL";

	skillsData[SKL_INVESTIGATE].value = &( character.ms_investigate );
	skillsData[SKL_INVESTIGATE].name = "Investigate";
	skillsData[SKL_INVESTIGATE].description = "How good you are at noticing things around you.";
	skillsData[SKL_INVESTIGATE].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_INVESTIGATE].abbreviation = "INV";

	skillsData[SKL_KNOWLEDGE].value = &( character.ms_knowledge );
	skillsData[SKL_KNOWLEDGE].name = "Knowledge";
	skillsData[SKL_KNOWLEDGE].description = "The measure of your general knowledge.";
	skillsData[SKL_KNOWLEDGE].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_KNOWLEDGE].abbreviation = "KNW";

	skillsData[SKL_CRAFT].value = &( character.ms_craft );
	skillsData[SKL_CRAFT].name = "Craft";
	skillsData[SKL_CRAFT].description = "How handy you are at making and repairing items. Also useful when breaking things as well.";
	skillsData[SKL_CRAFT].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_CRAFT].abbreviation = "CFT";

	skillsData[SKL_SURVIVAL].value = &( character.ms_survival );
	skillsData[SKL_SURVIVAL].name = "Survival";
	skillsData[SKL_SURVIVAL].description = "The amount of knowledge you have about surviving in the wilderness. Very important if you're going out into the wild.";
	skillsData[SKL_SURVIVAL].attrValue = &( character.stat_mentalDie );
	skillsData[SKL_SURVIVAL].abbreviation = "SRV";

	skillsData[SKL_EMPATHY].value = &( character.ss_empathy );
	skillsData[SKL_EMPATHY].name = "Empathy";
	skillsData[SKL_EMPATHY].description = "Your ability to see other people's points of view and understand them better.";
	skillsData[SKL_EMPATHY].attrValue = &( character.stat_socialDie );
	skillsData[SKL_EMPATHY].abbreviation = "EMP";

	skillsData[SKL_INTIMIDATE].value = &( character.ss_intimidate );
	skillsData[SKL_INTIMIDATE].name = "Intimidate";
	skillsData[SKL_INTIMIDATE].description = "The measure of how well you are able to make people fear you.";
	skillsData[SKL_INTIMIDATE].attrValue = &( character.stat_socialDie );
	skillsData[SKL_INTIMIDATE].abbreviation = "INT";

	skillsData[SKL_BARTER].value = &( character.ss_barter );
	skillsData[SKL_BARTER].name = "Barter";
	skillsData[SKL_BARTER].description = "How good you are at making business deals.";
	skillsData[SKL_BARTER].attrValue = &( character.stat_socialDie );
	skillsData[SKL_BARTER].abbreviation = "BTR";

	skillsData[SKL_LIE].value = &( character.ss_lie );
	skillsData[SKL_LIE].name = "Lie";
	skillsData[SKL_LIE].description = "How good you are at bluffing and lying to others.";
	skillsData[SKL_LIE].attrValue = &( character.stat_socialDie );
	skillsData[SKL_LIE].abbreviation = "LIE";
}

void randomlyDistributeSkills( void )
{
	// randomly weight based on the size of the attribute and how many points have already
	//  been spent, don't go over the size of the die
	int skillWeights[NUM_SKILLS];
	while( character.skillPointsLeft > 0 ) {
		memset( skillWeights, 0, sizeof( skillWeights[0] ) * ARRAYSIZE( skillWeights ) );
		int totalWeights = 0;
		for( int i = 0; i < NUM_SKILLS; ++i ) {
			skillWeights[i] = (*skillsData[i].attrValue) - (*skillsData[i].value);
			totalWeights += skillWeights[i];
		}
		int choice = rand( ) % totalWeights;

		int idx = 0;
		while( skillWeights[idx] < choice ) {
			choice -= skillWeights[idx];
			++idx;
		}

		++(*skillsData[idx].value);
		--character.skillPointsLeft;
	}
}

int simpleRoll( int stat )
{
	return 1 + ( rand( ) % stat );
}

Result skillCheck( int skillIdx, uint8_t modifiers, uint8_t difficulty )
{
	// gather all the data we need to make the roll
	uint8_t companionPhysical = 0;
	uint8_t companionMental = 0;
	uint8_t companionSocial = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) {
			companionPhysical += companionsData[i].physical;
			companionMental += companionsData[i].mental;
			companionSocial += companionsData[i].social;
		}
	}

	uint8_t stat;
	uint8_t skill;
	if( ( skillIdx >= 0 ) && ( skillIdx < 4 ) ) {
		stat = character.stat_physicalDie;
		skill = companionPhysical;
	} else if( ( skillIdx >= 4 ) && ( skillIdx < 8 ) ) {
		stat = character.stat_mentalDie;
		skill = companionMental;
	} else {
		stat = character.stat_socialDie;
		skill = companionSocial;
	}
	skill += (*skillsData[skillIdx].value);

	// make the roll
	int roll = 1 + ( rand( ) % (int)stat );
	int check = ( roll + (int)skill ) + (int)modifiers;

	Result result;
	if( check >= difficulty ) {
		// player gets what they want
		result = R_SUCCESS;
	} else if( check >= ( difficulty / 2 ) ) {
		// player gets what they want but at a cost
		result = R_COSTLY_SUCCESS;
	} else {
		// player doesn't get what they want and pays a cost
		result = R_FAILURE;
	}

	// rolling a 1 reduces the result by one step
	if( ( roll == 1 ) && ( result > R_FAILURE ) ) {
		--result;
	}

	return result;
}

int carryWeight( void )
{
	return ( character.stat_physicalDie + character.ps_acrobatics );
}

int totalGearCount( void )
{
	int total = 0;
	for( int i = 0; i < NUM_GEAR; ++i ) {
		total += character.gear[i];
	}
	return total;
}

int gearSpaceLeft( void )
{
	return ( carryWeight( ) - totalGearCount( ) );
}

void addGear( Gear g )
{
	++character.gear[g];
}

void removeGear( Gear g )
{
	if( character.gear[g] > 0 ) {
		--character.gear[g];
	}
}

#define DESC_BORDER_BOTTOM 41
#define DESC_BORDER_RIGHT 55
SMALL_RECT descriptionSafeArea = { 2, 2, DESC_BORDER_RIGHT - 2, DESC_BORDER_BOTTOM - 2 };
SMALL_RECT choicesSafeArea = { 3, DESC_BORDER_BOTTOM + 1, DESC_BORDER_RIGHT - 2, SCREEN_HEIGHT - 2 };
SMALL_RECT characterSafeArea = { DESC_BORDER_RIGHT + 2, 2, SCREEN_WIDTH - 3, SCREEN_HEIGHT - 2 };

void drawPlayScreen( void )
{
	// we already have the main border, so we just need to draw the
	//  border for the other areas (text description, choices, and character)
	WORD attr = FG_GREY | BG_BLACK;
	
	simplePutChar( 204, attr, 0, DESC_BORDER_BOTTOM );
	simplePutChar( 202, attr, DESC_BORDER_RIGHT, SCREEN_HEIGHT - 1 );
	
	CHAR c = screen.buffer[SCREEN_POS( DESC_BORDER_RIGHT, 0 )].Char.AsciiChar;
	if( ( c < 0 ) || ( c > 127 ) ) {
		simplePutChar( 203, attr, DESC_BORDER_RIGHT, 0 );
	}

	for( int i = 1; i < ( DESC_BORDER_RIGHT ); ++i ) {
		simplePutChar( 205, attr, i, DESC_BORDER_BOTTOM );
	}

	for( int i = 1; i < ( SCREEN_HEIGHT - 1 ); ++i ) {
		simplePutChar( 186, attr, DESC_BORDER_RIGHT, i );
	}

	simplePutChar( 185, attr, DESC_BORDER_RIGHT, DESC_BORDER_BOTTOM );

	// draw the character info
	//  stats and skills
	COORD pos;
	pos.X = characterSafeArea.Left + 2;
	pos.Y = characterSafeArea.Top;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "%s", FG_DARK_CYAN, classNames[character.class] );
	pos.Y += 2;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Stats and Skills", FG_BROWN );
	pos.Y += 2;
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: d%i", FG_DARK_GREEN, character.stat_physicalDie );
	++pos.Y;
	int i = 0;
	for( ; i < 4; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: d%i", FG_DARK_GREEN, character.stat_mentalDie );
	++pos.Y;
	for( ; i < 8; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: d%i", FG_DARK_GREEN, character.stat_socialDie );
	++pos.Y;
	for( ; i < 12; ++i ) {
		drawString( pos.X, pos.Y, characterSafeArea, " %s: %i", FG_DARK_CYAN, skillsData[i].name, (*skillsData[i].value ) );
		++pos.Y;
	}

	// draw cumulative wounds
	++pos.Y;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Total Wounds", FG_BROWN );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: %i", FG_MAROON, character.wounds_physical );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: %i", FG_MAROON, character.wounds_mental );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: %i", FG_MAROON, character.wounds_social );

	pos.Y += 2;
	drawString( pos.X - 1, pos.Y, characterSafeArea, "Companion Bonuses", FG_BROWN );
	++pos.Y;
	int physicalBonus = 0;
	int mentalBonus = 0;
	int socialBonus = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) {
			physicalBonus += companionsData[i].physical;
			mentalBonus += companionsData[i].mental;
			socialBonus += companionsData[i].social;
		}
	}
	drawString( pos.X, pos.Y, characterSafeArea, "Physical: +%i", FG_DARK_GREEN, physicalBonus );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Mental: +%i", FG_DARK_GREEN, mentalBonus );
	++pos.Y;
	drawString( pos.X, pos.Y, characterSafeArea, "Social: +%i", FG_DARK_GREEN, socialBonus );


	drawString( characterSafeArea.Left, characterSafeArea.Bottom - 1, characterSafeArea, "C -> Character Sheet", FG_YELLOW );
	drawString( characterSafeArea.Left, characterSafeArea.Bottom, characterSafeArea, "H -> Help", FG_YELLOW );
}

void startPlayDraw( void )
{
	startDraw( );
	drawPlayScreen( );
}

void helpScene( void );
void characterDetailsScene( void );
boolean fromStatusScreen;

boolean testSharedInput( Input input )
{
	boolean handled = false;
	switch( input ) {
	case IN_C:
		handled = true;
		storedScene = currentScene;
		nextScene = characterDetailsScene;
		break;
	case IN_H:
		handled = true;
		storedScene = currentScene;
		nextScene = helpScene;
		break;
	}

	return handled;
}

typedef enum {
	CT_SKILL_BASED,
	CT_SIMPLE,
	NUM_CHOICE_TYPES
} ChoiceType;

typedef struct {
	ChoiceType type;
	char* description;

	Scene failureScene;
	Scene successScene;
	Scene costlySuccessScene;

	Skills skill;
	uint8_t difficulty;
} SkillBasedChoice;

typedef struct {
	ChoiceType type;
	char* description;

	Scene nextScene;
} SimpleChoice;

typedef struct {
	ChoiceType type;
	char* description;
} BaseChoice;

typedef union {
	ChoiceType type;

	BaseChoice base;
	SkillBasedChoice skillBased;
	SimpleChoice simple;
} Choice;

char* deathReason;

void handleSimpleChoice( SimpleChoice choice )
{
	nextScene = choice.nextScene;
}

void handleSkillBasedChoice( SkillBasedChoice choice )
{
	Result r = skillCheck( choice.skill, 0, choice.difficulty );
	switch( r ) {
	case R_SUCCESS:
		nextScene = choice.successScene;
		break;
	case R_COSTLY_SUCCESS:
		nextScene = choice.costlySuccessScene;
		break;
	case R_FAILURE:
		nextScene = choice.failureScene;
		break;
	}
}

void handleChoice( Choice choice )
{
	switch( choice.type ) {
	case CT_SIMPLE:
		handleSimpleChoice( choice.simple );
		break;
	case CT_SKILL_BASED:
		handleSkillBasedChoice( choice.skillBased );
		break;
	}
}

int choiceTop;
int standardCurrentChoice;
void standardSceneChoiceDraw( Choice* choices, size_t numChoices )
{
	WORD selected = FG_CYAN | BG_BROWN;
	WORD normal = FG_DARK_CYAN | BG_BLACK;
	WORD attr;

	COORD outPos;
	outPos.Y = choicesSafeArea.Top - 1;
	for( int i = choiceTop; i < numChoices; ++i ) {
		attr = ( standardCurrentChoice == i ) ? selected : normal;

		outPos = drawString( choicesSafeArea.Left, outPos.Y + 1, choicesSafeArea, "%s", attr, choices[i].base.description );
	}

	// if there are more below then show down arrow
	if( ( choiceTop + ( choicesSafeArea.Bottom - choicesSafeArea.Top ) ) < ( numChoices - 1 ) ) {
		drawString( choicesSafeArea.Left - 1, choicesSafeArea.Bottom, renderArea, "v", FG_YELLOW );
	}
	// if there are more above then show up arrow
	if( choiceTop > 0 ) {
		drawString( choicesSafeArea.Left - 1, choicesSafeArea.Top, renderArea, "^", FG_YELLOW );
	}
}

void fitCurrentSelection( void )
{
	// see if the current choice is visible
	int range = choicesSafeArea.Bottom - choicesSafeArea.Top;
	while( ( standardCurrentChoice - choiceTop ) > range ) {
		++choiceTop;
	}

	while( ( standardCurrentChoice - choiceTop ) < 0 ) {
		--choiceTop;
	}
}

void standardSceneChoice( Choice* choices, size_t numChoices, boolean allowDefaults )
{
	Input input;
	boolean handled = false;

	do {
		input = getNextInput( );
		if( allowDefaults ) {
			handled = testSharedInput( input );
		}
		if( !handled ) {
			switch( input ) {
			case IN_UP:
				if( standardCurrentChoice > 0 ) {
					--standardCurrentChoice;
				}
				fitCurrentSelection( );
				handled = true;
				break;
			case IN_DOWN:
				if( standardCurrentChoice < ( numChoices - 1 ) ) {
					++standardCurrentChoice;
				}
				//standardCurrentChoice = ( standardCurrentChoice + 1 ) % numChoices;
				fitCurrentSelection( );
				handled = true;
				break;
			case IN_ENTER:
				handleChoice( choices[standardCurrentChoice] );
				handled = true;
				break;
			}
		}
	} while( !handled );

	// eat the rest of the inputs
	eatAllInputs( );
}

boolean armoredWound = false;
void deathScene( void );
void startScene( )
{
	armoredWound = false;
	if( nextScene != deathScene ) {
		nextScene = NULL;
	}
	standardCurrentChoice = 0;
	choiceTop = 0;
	topTitle = NULL;
	fromStatusScreen = false;;
}

void pushSimpleChoice( Choice** sbChoices, char* description, Scene nextScene )
{
	Choice c;
	c.type = CT_SIMPLE;
	c.simple.description = description;
	c.simple.nextScene = nextScene;

	sb_push( (*sbChoices), c );
}

void pushSkillBasedChoice( Choice** sbChoices, char* description, Skills skill, uint8_t difficulty,
	Scene successScene, Scene costlySuccessScene, Scene failureScene )
{
	// filter skill choices by the amount of wounds they have to that statistic
	switch( skill % 4 ) {
	case 0:
		if( character.wounds_physical >= character.stat_physicalDie ) {
			return;
		}
		break;
	case 1:
		if( character.wounds_mental >= character.stat_mentalDie ) {
			return;
		}
		break;
	case 2:
		if( character.wounds_social >= character.stat_socialDie ) {
			return;
		}
		break;
	}

	Choice c;
	c.type = CT_SKILL_BASED;
	c.skillBased.description = description;
	c.skillBased.skill = skill;
	c.skillBased.difficulty = difficulty;
	c.skillBased.successScene = successScene;
	c.skillBased.costlySuccessScene = costlySuccessScene;
	c.skillBased.failureScene = failureScene;

	sb_push( (*sbChoices), c );
}

void dropGearScene( void );

boolean isDead( )
{
	return character.wounds_physical >= character.stat_physicalDie;
}

boolean checkForDeath( const char* reason )
{
	if( character.wounds_physical >= character.stat_physicalDie ) {
		deathReason = reason;
		nextScene = deathScene;
		return true;
	}
	return false;
}

void gainPhysicalWound_base( const char* deathReason, bool ignoreArmor )
{
	armoredWound = false;
	// armor absorbs wounds
	if( !ignoreArmor && ( character.gear[G_LIGHT_ARMOR] > 0 ) ) {
		armoredWound = true;
		--character.gear[G_LIGHT_ARMOR];
		return;
	}
	++character.wounds_physical;
	checkForDeath( deathReason );
}

void gainPhysicalWound( const char* deathReason )
{
	gainPhysicalWound_base( deathReason, false );
}

void gainArmorIgnoringWound( const char* deathReason )
{
	gainPhysicalWound_base( deathReason, true );
}

void gainNonArmorPhysicalWound( const char* deathReason )
{
	armoredWound = false;
	++character.wounds_physical;
	checkForDeath( deathReason );
}

void gainMentalWound( void )
{
	++character.wounds_mental;
}

void gainSocialWound( void )
{
	++character.wounds_social;
}

void checkTooMuchGear( Scene next )
{
	if( gearSpaceLeft( ) < 0 ) {
		currentScene = dropGearScene;
		nextScene = next;
	} else {
		currentScene = nextScene;
	}
}

COORD standardGearGainText( SHORT x, SHORT y, SMALL_RECT area, Gear newGear )
{
	return drawString( x, y, area, "You've gained a %s!", FG_GREEN, gearData[newGear].name );
}

COORD standardGearLossText( SHORT x, SHORT y, SMALL_RECT area, Gear oldGear )
{
	return drawString( x, y, area, "You've lost a %s.", FG_RED, gearData[oldGear].name );
}

COORD standardWoundText( SHORT x, SHORT y, SMALL_RECT area, const char* type, const char* reason )
{
	if( ( strcmp( type, "Physical" ) == 0 ) && armoredWound ) {
		return drawString( x, y, area, "Your armor absorbed the Wound!", FG_GREEN, reason, type );
	}  else {
		return drawString( x, y, area, "You %s and gained a %s Wound...", FG_RED, reason, type );
	}
}

Gear bestFightWeapon( void )
{
	if( character.gear[G_SWORD] > 0 ) {
		return G_SWORD;
	} else if( character.gear[G_HANDAXE] > 0 ) {
		return G_HANDAXE;
	}

	return -1;
}

int bestFightWeaponBonus( void )
{
	int g = bestFightWeapon( );
	int bonus = 0;
	if( g >= 0 ) {
		bonus = gearData[g].bonus;
	}

	return bonus;
}

Gear bestShootWeapon( void )
{
	if( character.gear[G_RIFLE] > 0 ) {
		return G_RIFLE;
	} else if( character.gear[G_PISTOL] > 0 ) {
		return G_PISTOL;
	}

	return -1;
}

int bestShootWeaponBonus( void )
{
	int g = bestShootWeapon( );
	int bonus = 0;
	if( g >= 0 ) {
		bonus = gearData[g].bonus;
	}

	return bonus;
}

Gear randomGearToLose( void )
{
	int total = 0;
	for( int i = 0; i < NUM_GEAR; ++i ) {
		total += character.gear[i];
	}

	int choice = rand( ) % total;

	int idx = 0;
	while( character.gear[idx] < choice ) {
		choice -= character.gear[idx];
		++idx;
	}

	return idx;
}

int numCompanions( void )
{
	int total = 0;
	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) ++total;
	}

	return total;
}

Companions getFirstCompanion( void )
{
	int i = 0;
	while( ( i < NUM_COMPANIONS ) && !character.hasCompanion[i] ) {
		++i;
	}
	return i;
}

Companions getNextCompanionAfter( Companions cmp )
{
	int i = cmp + 1;
	while( ( i < NUM_COMPANIONS ) && !character.hasCompanion[i] ) {
		++i;
	}
	return i;
}

char everyoneString[64];
const char* getEveryoneString( bool capitalizeYou )
{
	// order always goes: SCHOLAR, ELF, GOBLIN
	int numCmp = numCompanions( );
	char capitalY = capitalizeYou ? 'Y' : 'y';

	if( numCmp == 0 ) {
		snprintf( everyoneString, 63, "%cou", capitalY );
	} else if( numCmp == 1 ) {
		char* other = companionsData[getFirstCompanion( )].name;
		snprintf( everyoneString, 63, "%cou and %s", capitalY, other );
	} else if( numCmp == 2 ) {
		Companions c = getFirstCompanion( );
		char* otherOne = companionsData[c].name;
		c = getNextCompanionAfter( c );
		char* otherTwo = companionsData[c].name;
		snprintf( everyoneString, 63, "%cou, %s, and %s", capitalY, otherOne, otherTwo );
	} else {
		snprintf( everyoneString, 63, "%cou, %s, %s, and %s", capitalY, companionsData[0].name, companionsData[1].name, companionsData[2].name );
	}

	return &( everyoneString[0] );
}

Companions getRandomCompanion( void )
{
	Companions c = -1;
	int num = 0;

	for( int i = 0; i < NUM_COMPANIONS; ++i ) {
		if( character.hasCompanion[i] ) {
			++num;
			if( ( rand( ) % num ) == 0 ) {
				c = i;
			}
		}
	}

	return c;
}

/******* SCENES *********/

void titleScene( void );

#if 0
void victoryScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You've won!", titleScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly grab whatever you can get your hands on, filling your pack with "
				"scientific equipment and notes.\n\n"
				"When you get back to town you're welcomed with open arms. The researchers and "
				"engineers are excited to go through your findings. You also tell them where "
				"you found it and that the place isn't empty yet. They start putting together "
				"a larger group to go there, giving you a place among the scouts.\n\n"
				"It's not the cushiest job, but you get plenty of exercise and lots of free "
				"time to relax.",
				FG_GREY );
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void greatVictoryScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You got the best ending!", titleScene );

	COORD pos;
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Instead of just grabbing everything you can you carefully search the area, "
				"using what you remember from your lessons. You find what appears to be safe "
				"that has been rusted partially through. Reaching in you find a stack of paper "
				"full of strange symbols and numbers, written in a language you can't understand.\n\n"
				"When you get back to town you're welcomed with open arms. When you bring the stack "
				"of papers to the lead researcher his eyes widen and his jaw drops. He quickly shuffles "
				"through the papers and then quietly says it's all here to himself. He yells for one "
				"of the other researchers who quickly comes over. He hands her the stack of papers "
				"and she has a similar reaction.\n\n"
				"Looks like you've assured yourself the job of your choice.",
				FG_GREY );
			
			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}
#endif

void printNPCPostGameStuff( COORD pos )
{
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* elf = companionsData[CMP_ELF].name;

	if( character.hasCompanion[CMP_GOBLIN] ) {
		pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
			"%s stays in the settlement for a while helping people with odd jobs and helping fight "
			"the remains of the robots. He gets bored though and wanders off half a year later.",
			FG_GREY, goblin );
	}

	if( character.hasCompanion[CMP_ELF] ) {
		pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
			"%s doesn't stay in the settlement long. Being around large crowds seems to make him "
			"uncomfortable. A month after getting back he comes to visit you and says he's leaving, "
			"but there's something in the future he may want your help with.",
			FG_GREY, elf );
	}
}

void poweredDownVictoryScene( void )
{
	// power is no longer available, best end
	char* scholar = companionsData[CMP_SCHOLAR].name;
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You got the Power Down ending!", titleScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s is able to find the parts feeding the factory power. Throwing a switch you hear "
				"the factory stopping. She then takes out a small screwdriver and dissassembles part "
				"of the mechanism so it can't be turned back on. She takes a few things back to her "
				"lab for study.\n\n"
				"Getting back to town you give your report of what happend. You congratulated and given "
				"a bit of a bump in pay. You also don't have to buy your own drinks for month.\n\n"
				"The factory is never turned back after that. Regular shipments of relics for study are "
				"sent back by some of the explorers that happen upon it. If not for everything else that "
				"was going to happen it would be a relatively peaceful time.",
				FG_GREY, scholar );

			printNPCPostGameStuff( pos );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void smashedVictoryScene( void )
{
	// everything is smashed up, nothing left to study
	char* scholar = companionsData[CMP_SCHOLAR].name;
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You got the Smashed ending!", titleScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Looking at the ruined machines fills you with a sense of pride. %s looks a little sad "
				"but picks up some things to bring back with her for study.\n\n"
				"Getting back to town you give your report of what happend. You congratulated and given "
				"a bit of a bump in pay. You also don't have to buy your own drinks for month.\n\n"
				"The scholars are disappointed that the machines were smashed as there's nothing left "
				"to study. They don't hold this against you though, and know that it was better destroyed "
				"than left running.",
				FG_GREY, scholar );

			printNPCPostGameStuff( pos );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void turnedOffVictoryScene( void )
{
	// everything is off, but could be turned on again
	char* scholar = companionsData[CMP_SCHOLAR].name;
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Congratulations! You got the Turned Off ending!", titleScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The factory is quiet as you leave it. %s is happy and brings some of the smaller robots "
				"back for study.\n\n"
				"Getting back to town you give your report of what happend. You congratulated and given "
				"a bit of a bump in pay. You also don't have to buy your own drinks for month.\n\n"
				"Over the years the factory is turned back on occasionally. Noone knows how or why but "
				"since your expedition it hasn't been much of an issue to turn it off again. The scouts "
				"always keep an eye on the place as well.",
				FG_GREY, scholar );

			printNPCPostGameStuff( pos );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntrancePacifiedScene( void );

void factoryControlRoomScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Have Freda turn off the factory and leave.", turnedOffVictoryScene );
	pushSimpleChoice( &sbChoices, "Continue exploring.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The control room is lit up underneath a heavy layer of dust so thick it's almost dirt. "
				"Smashing the controls would have little effect. You'd need someone who knows about "
				"this technology to be able to switch it off. Freda smiles at you.\n\n"
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryFloorGoblinScene( void )
{
	char* goblin = companionsData[CMP_GOBLIN].name;

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Give the goblin a pat on the back and leave.", smashedVictoryScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You smile at %s. %s smiles at you and hefts his blade. It's big, thick, heavy, and rough. "
				"More a hunk iron than a proper sword. Perfect for smashing all this delicate machinery.\n\n"
				"After the whirlwind of destruction abates %s's chest is heaving and the factory is silent.",
				FG_GREY, goblin, goblin, goblin );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryFloorScene( void );

void factoryFloorBreakSuccessScene( void )
{
	// factory stopped
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Breath a sigh of relief and leave.", smashedVictoryScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s set to work. You give instructions on how to safely break everything. After a few "
				"hours of work the factory sits there fidgeting, unable to to do anything but also "
				"unable to stop. Whatever is powering should run out in a few years and then it "
				"should be safe to come back. With the machines disabled it seems you've fulfilled "
				"your goal.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryFloorBreakCostlyScene( void )
{
	// get hurt, factory stopped
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Crushed by machinery." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Breath a sigh of relief and leave.", smashedVictoryScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s set to work. You give instructions on how to safely break everything. "
				"It would have been better if you had been paying attention to your own instructions "
				"as you get your hand caught in some machinery while reaching into it. One of your "
				"fingers may be broken. After setting it you continue working.\n\n"
				"After a few hours of work the factory sits there fidgeting, unable to to do anything but also "
				"unable to stop. Whatever is powering should run out in a few years and then it "
				"should be safe to come back. With the machines disabled it seems you've fulfilled "
				"your goal.",
				FG_GREY, getEveryoneString( true ) );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "got your hand caught in some machinery" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryFloorBreakFailureScene( void )
{
	// get hurt, factory is still going
	static Companions cmp;
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Crushed by machinery." );
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Think about what to do.", factoryFloorScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Smiling you start reaching into the machinery around you. You've disabled one of the arms "
				"and are going to disable the a nearby conveyor belt when you stick your hand somewhere "
				"you shouldn't. Luckily %s pulls you back before you get pulled into somewhere fatal.",
				FG_GREY, getEveryoneString( true ) );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "got your hand caught in some machinery" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryFloorScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	if( character.hasCompanion[CMP_GOBLIN] ) {
		pushSimpleChoice( &sbChoices, "Have Zobd smash everything.", factoryFloorGoblinScene );
	}
	pushSkillBasedChoice( &sbChoices, "Break everything. (Craft)", SKL_CRAFT, 16,
		factoryFloorBreakSuccessScene, factoryFloorBreakCostlyScene, factoryFloorBreakFailureScene );
	pushSimpleChoice( &sbChoices, "Continue exploring.", factoryEntrancePacifiedScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test break success.", factoryFloorBreakSuccessScene );
		pushSimpleChoice( &sbChoices, "Test break costly.", factoryFloorBreakCostlyScene );
		pushSimpleChoice( &sbChoices, "Test break failure.", factoryFloorBreakFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The factory floor is a hive of activity. Belts and mechanical arms moving things around "
				"in a complicated dance. Looking at the machinery it would be possible to just smash it all.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

#define VINES_DONE 0
#define GAS_OFF 1

void factoryBasementGasPacifiedScene( void );
void factoryBasementGasActiveScene( void );

void factoryGasGoblinScene( void )
{
	turnOnFlag( GAS_OFF );

	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Enter and look around.", factoryBasementGasPacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s opens the door and walks in. %s watches him in fascination, asking if he feels "
				"light-headed or dizzy at all. He shrugs and says he can't be poisoned before heading "
				"over to the valve and turning it off.\n\n"
				"After a short wait the smell has left the room, it should be safe to enter.",
				FG_GREY, goblin, scholar );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryGasAcrobaticsSuccess( void )
{
	// turn off the gas
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Enter and look around.", factoryBasementGasPacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You dip your towel in some water and hold it over your mouth and nose. You take a deep "
				"breath and as quickly as possible run to the valve. Still holding the towel you are able "
				"to turn off the valve. You run back out of the room and collapse on the ground gasping "
				"for breath.\n\n"
				"After a short wait the smell has left the room, it should be safe to enter.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryGasAcrobaticsCostly( void )
{
	// gain a wound and turn off the gas
	if( !fromStatusScreen ) {
		gainNonArmorPhysicalWound( "Suffocated on poisonous gas." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Enter and look around.", factoryBasementGasPacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You dip your towel in some water and hold it over your mouth and nose. You take a deep "
				"breath and as quickly as possible run to the valve. Still holding the towel you struggle "
				"to turn the valve. Your lungs start to tingle and you're starting to feel a bit dizzy by "
				"the time you run back out of the room.\n\n"
				"After a short wait the smell has left the room, it should be safe to enter.",
				FG_GREY, getEveryoneString( true ) );


			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "breathed in toxic gas" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryGasAcrobaticsFailure( void )
{
	// gain a wound
	if( !fromStatusScreen ) {
		gainNonArmorPhysicalWound( "Suffocated on poisonous gas." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Think about what to do.", factoryBasementGasActiveScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You dip your towel in some water and hold it over your mouth and nose. You take a deep "
				"breath and as quickly as possible run to the valve. Part way there you slip and fall, "
				"involuntarily taking a deep breath. You scramble back out of the room. For the next "
				"few minutes you do nothing but cough. There's a tingling in you lungs that lingers for a bit longer than that.",
				FG_GREY, getEveryoneString( true ) );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "breathed in toxic gas" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryBasementGasPacifiedScene( void )
{
	turnOnFlag( GAS_OFF );

	char* goblin = companionsData[CMP_GOBLIN].name;

	// if you have a towel you can use acrobatics to turn off the valve
	// if you have zobd with you he's immune to poison
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Get Freda to turn off the power and leave.", poweredDownVictoryScene );
	pushSimpleChoice( &sbChoices, "Go back to the entrance.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The only sounds in the room is a hum from the electrical equipment in here and "
				"the distant cacophany of the factory above.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryBasementGasActiveScene( void )
{
	char* scholar = companionsData[CMP_SCHOLAR].name;
	char* goblin = companionsData[CMP_GOBLIN].name;

	static boolean alreadyBeenHere = false;
	// if you have a towel you can use acrobatics to turn off the valve
	// if you have zobd with you he's immune to poison
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	if( character.gear[G_TOWEL] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Make a crude gasmask out of your towel and go in. (Acrobatics)", SKL_ACROBATICS, 16,
			factoryGasAcrobaticsSuccess, factoryGasAcrobaticsCostly, factoryGasAcrobaticsFailure );
	}
	if( character.hasCompanion[CMP_GOBLIN] ) {
		pushSimpleChoice( &sbChoices, "Send Zobd in.", factoryGasGoblinScene );
	}
	pushSimpleChoice( &sbChoices, "Go back to the entrance.", factoryEntrancePacifiedScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test acrobatics success.", factoryGasAcrobaticsSuccess );
		pushSimpleChoice( &sbChoices, "Test acrobatics costly.", factoryGasAcrobaticsCostly );
		pushSimpleChoice( &sbChoices, "Test acrobatics failure.", factoryGasAcrobaticsFailure );
	}

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( alreadyBeenHere ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The noxious, poisonous gas is still leaking out of the room.",
					FG_GREY, scholar );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Opening the door causes a foul odor to pour out of the room. %s holds you back "
					"saying the ancients used the smell as a warning for dangerous miasmas. Peeking "
					"inside she points out to a valve that the hissing sound is coming from. If you "
					"could turn that off the main power switch should be somewhere in this room.",
					FG_GREY, scholar );
			}

			if( character.hasCompanion[CMP_GOBLIN] ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"%s takes a big whiff and says it smells good to him.",
					FG_GREY, goblin );
			}
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryBasementGasScene( void )
{
	if( isFlagOn( GAS_OFF ) ) {
		currentScene = factoryBasementGasPacifiedScene;
	} else {
		currentScene = factoryBasementGasActiveScene;
	}
}

void factoryBasementVinesPacifiedScene( void )
{
	turnOnFlag( VINES_DONE );

	// can just move through
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Continue in deeper.", factoryBasementGasScene );
	pushSimpleChoice( &sbChoices, "Go back to the entrance.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The hallway stands empty. Near the end you see a door. From beyond it you hear "
				"the hum of machinery and a hissing sound.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryVinesElfScene( void )
{
	char* elf = companionsData[CMP_ELF].name;

	static boolean beenHereAlready = false;

	// entrance is dark, need a light to continue
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Look around.", factoryBasementVinesPacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You motion for %s to go forward and deal with this. He clears his throat again "
				"and starts singning. It's not the same song as before. But after a few moments "
				"the vines start receding. Within a minute the hallway is empty. %s says that "
				"song generally isn't very useful as most plants aren't very good at acting "
				"on command.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;

	beenHereAlready = true;
}

void factoryVinesFightSuccessScene( void )
{
	// get through
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Look around.", factoryBasementVinesPacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s set to work. Carefully you hack away at the vines. Luring a few out at a time and "
				"chopping until the vines are too weak to grasp anything. After you're about a quarter "
				"of the way through them the rest retract into the walls.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryVinesFightCostlyScene( void )
{
	static char* cmp;

	// get through, gain wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Crushed by a mutant plant." );
		cmp = companionsData[getRandomCompanion( )].name;
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Look around.", factoryBasementVinesPacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s set to work. Carefully you hack away at the vines. Luring a few out at a time and "
				"chopping until the vines are too weak to grasp anything. After you're about a quarter "
				"of the way through one of them gets around your neck and starts squeezing. %s gets the "
				"plant off you as fast as they can.\n\n"
				"After spending a few minutes recovering you approach you task with a sense of vengence "
				"and make short work of the rest of the plants.",
				FG_GREY, getEveryoneString( true ), cmp );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were strangled by a plant" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryBasementVinesActiveScene( void );

void factoryVinesFightFailureScene( void )
{
	// can't get through, gain wound
	static char* cmp;

	// get through, gain wound
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Crushed by a mutant plant." );
		cmp = companionsData[getRandomCompanion( )].name;
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Think about what to do.", factoryBasementVinesActiveScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You jump into the nearest clump of plants and start hacking away. Before you're "
				"able to take out even one they overwhelm you. %s is able to grab you and pull out.\n\n"
				"It takes a few minutes for you to recover from your ordeal.",
				FG_GREY, cmp );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were strangled by a plant" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryBasementVinesActiveScene( void )
{
	// moving vines are covering this area
	//  can hack through them or get the elf to take care of them
	// can just move through
	static boolean beenHereBefore = false;

	char* goblin = companionsData[CMP_GOBLIN].name;
	char* elf = companionsData[CMP_ELF].name;

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	if( bestFightWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use a weapon to hack through the plants. (Fight)", SKL_FIGHT, 16 - bestFightWeaponBonus( ),
			factoryVinesFightSuccessScene, factoryVinesFightCostlyScene, factoryVinesFightFailureScene );
	}
	if( character.hasCompanion[CMP_ELF] ) {
		pushSimpleChoice( &sbChoices, "Let Noniac do his thing.", factoryVinesElfScene );
	}
	
	pushSimpleChoice( &sbChoices, "Go back to the entrance.", factoryEntrancePacifiedScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", factoryVinesFightSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.", factoryVinesFightCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", factoryVinesFightFailureScene );
	}

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( !beenHereBefore ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The hallway is choked with pale vines. Do vines grow underground? Maybe they're roots. "
					"As you approach they writhe and move, reaching out to grab you. You quickly back off.",
					FG_GREY );

				if( character.hasCompanion[CMP_GOBLIN] ) {
					pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"You see %s try approaching and swinging his blade at the plants. But they dodge out of "
						"the way and grab onto his blade. He's barely able to free it.",
						FG_GREY, goblin );
				}
				if( character.hasCompanion[CMP_ELF] ) {
					pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"%s clears his throat and says he can deal with this if you want.",
						FG_GREY, elf );
				}
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"The hallway is still blocked with plants.",
					FG_GREY );
				if( character.hasCompanion[CMP_ELF] ) {
					pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"%s clears his throat and says he can deal with this if you want.",
						FG_GREY, goblin );
				}
			}

			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;

	beenHereBefore = true;
}

void factoryBasementVinesScene( void )
{
	if( isFlagOn( VINES_DONE ) ) {
		currentScene = factoryBasementVinesPacifiedScene;
	} else {
		currentScene = factoryBasementVinesActiveScene;
	}
}

void factoryBasementScene( void )
{
	static boolean beenHereAlready = false;

	// entrance is dark, need a light to continue
	startScene( );

	Choice* sbChoices = NULL;

	if( ( character.gear[G_FLASHLIGHT] > 0 ) || ( character.gear[G_LANTERN] > 0 ) ) {
		pushSimpleChoice( &sbChoices, "Use a light to explore.", factoryBasementVinesScene );
	}
	pushSimpleChoice( &sbChoices, "Go back and explore the rest.", factoryEntrancePacifiedScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( beenHereAlready ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Still really dark down there.",
					FG_GREY );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Opening the door down into the basement you see nothing but inky blackness. "
					"If you had a light source you could continue downwards.",
					FG_GREY );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;

	beenHereAlready = true;
}

void factoryEntrancePacifiedScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Go to the control room.", factoryControlRoomScene );
	pushSimpleChoice( &sbChoices, "Go to the floor.", factoryFloorScene );
	pushSimpleChoice( &sbChoices, "Go to the basement.", factoryBasementScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s are standing in the entrace. You see three ways you could go: out to the factory floor, "
				"up to the control room, or down into the basement.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceScene( void );

void factoryEntranceShootSuccess( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Look around.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Looking around you notice that there's only one camera in the entire room. Taking "
				"careful aim you shoot it. The camera shatters and shortly afterwards the turrets "
				"retract.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceShootCostly( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Duck back into cover.", factoryEntranceScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Keeping in cover you shoot repeatedly at the turrets. Your bullets have "
				"little effect on them. Their bullets have a much greater effects on you.",
				FG_GREY );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceShootFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Pull yourself into cover.", factoryEntranceScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You stand up and shoot repeatedly at the turrets. Your bullets have "
				"little effect on them. Their bullets have a much greater effects on you.",
				FG_GREY );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceLieSuccess( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Look around.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a few deep breaths and stand up. Your hands held up above your head. The "
				"turrets all turn to face you and the voice starts spewing gibberish. Once it's "
				"finished you reply to it, trying to copy the style of nonsense you just heard.\n\n"
				"After a few brief moments the turrets rectract and the the voice tells you "
				"impression to discretion understood to we interested he excellence. You take that "
				"as acceptance of whatever you just said.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceLieCostly( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Dive into cover.", factoryEntranceScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a few deep breaths and stand up. Your hands held up above your head. The "
				"turrets all turn to face you and the voice starts spewing gibberish. Once it's "
				"finished you reply to it, trying to copy the style of nonsense you just heard.\n\n"
				"The turrets immediately start firing at you.",
				FG_GREY );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceLieFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Dive into cover.", factoryEntranceScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a few deep breaths and stand up. Your hands held up above your head. The "
				"turrets all turn to face you and the voice starts spewing gibberish. Once it's "
				"finished you politely ask it to stop, telling it that you're not it's enemy.\n\n"
				"The turrets immediately start firing at you.",
				FG_GREY );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceInvestigateSuccess( void )
{
	static char* cmpName;

	if( !fromStatusScreen ) {
		cmpName = companionsData[getRandomCompanion( )].name;
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Look around.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"This is a defense system, so it was most likely used by whoever sat at that desk over "
				"there. While the turrets are distracted by %s changing cover you make a dash for the desk. "
				"Underneath you find a number of switches, a red one catches your eye. You switch from \"non-lethal\" to \"off\". The "
				"turrets retract and the voice tells two among sir sorry men court.",
				FG_GREY, cmpName );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceInvestigateCostly( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Look around.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"This is a defense system, so it was most likely used by whoever sat at that desk over "
				"there. The turrets are able to get some shots off at you before you're back in cover. "
				"Underneath you find a number of switches, a red one catches your eye and you flip it. The "
				"turrets retract and the voice tells you are will took form the nor true.",
				FG_GREY );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceInvestigateFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Shot by a turret." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Look around.", factoryEntrancePacifiedScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"This is a defense system, so it was most likely used by whoever sat at that desk over "
				"there. The turrets are able to get some shots off at you before you're back in cover. "
				"This is a defense system, so it was most likely used by whoever sat at that desk over "
				"there. While the turrets are distracted by %s changing cover you make a dash for the desk. "
				"Underneath you find a number of switches, a red one catches your eye. You switch from \"non-lethal\" to \"off\". The "
				"turrets retract and the voice tells you are will took form the nor true.",
				FG_GREY );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were shot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryEntranceScene( void )
{
	// can shoot at them (only need one almost impossible success)
	// or try to convince them to let you pass (Lie)
	// or try to find an off switch (Investigation)
	static boolean alreadyBeenHere = false;
	startScene( );	

	Choice* sbChoices = NULL;

	if( bestShootWeapon( ) > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Shoot at the turrets. (Shoot)", SKL_SHOOT, 20 - bestShootWeapon( ),
			factoryEntranceShootSuccess, factoryEntranceShootCostly, factoryEntranceShootFailure );
	}
	pushSkillBasedChoice( &sbChoices, "Convince it you're a friend. (Lie)", SKL_LIE, 18,
		factoryEntranceLieSuccess, factoryEntranceLieCostly, factoryEntranceLieFailure );
	pushSkillBasedChoice( &sbChoices, "Try and find an off switch. (Investigate)", SKL_INVESTIGATE, 16,
		factoryEntranceInvestigateSuccess, factoryEntranceInvestigateCostly, factoryEntranceInvestigateFailure );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test shoot success", factoryEntranceShootSuccess );
		pushSimpleChoice( &sbChoices, "Test shoot costly",  factoryEntranceShootCostly );
		pushSimpleChoice( &sbChoices, "Test shoot failure", factoryEntranceShootFailure );

		pushSimpleChoice( &sbChoices, "Test investigate success", factoryEntranceInvestigateSuccess );
		pushSimpleChoice( &sbChoices, "Test investigate costly",  factoryEntranceInvestigateCostly );
		pushSimpleChoice( &sbChoices, "Test investigate failure", factoryEntranceInvestigateFailure );

		pushSimpleChoice( &sbChoices, "Test lie success", factoryEntranceLieSuccess );
		pushSimpleChoice( &sbChoices, "Test lie costly",  factoryEntranceLieCostly );
		pushSimpleChoice( &sbChoices, "Test lie failure", factoryEntranceLieFailure );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( alreadyBeenHere ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You're hiding behind a couch, the turrets scanning the room looking for any targets.\n\n"
					"What do you do?",
					FG_GREY, getEveryoneString( true ) );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"Once you enter the factory you hear motors whirr to life. Multiple turrets pop out "
					"of the ceiling and aim at you. As you dive under some nearby furniture a voice starts spouting nonsense at you.\n\n"
					"What do you do?",
					FG_GREY, getEveryoneString( true ) );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	alreadyBeenHere = true;

	sb_free( sbChoices );
	currentScene = nextScene;
}

void factoryIntroScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	COORD pos;

	pushSimpleChoice( &sbChoices, "Head inside.", factoryEntranceScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s stand outside the factory. Looking around you see nothing guarding the outside.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	currentScene = nextScene;
}

// Wilderness scenes, you will have a number of days you have to travel, with random encounters along the way
int timeLeft;
void wanderScene( void );

void elfScene( void );
void goblinScene( void );
void scoutsScene( void );
void hiddenCacheScene( void );
void bearScene( void );
void wanderingRobotScene( void );
void brokenRobotScene( void );
void madmanInATreeScene( void );
void traderScene( void );
void chasmScene( void );
void abandonedHouseScene( void );
void caveScene( void );

void elfGoblinConflictScene( void );
void elfScholarConflictScene( void );
void goblinScholarConflictScene( void );

Scene* sbWildernessScenes = NULL;
#define WANDER_COUNT 11

void setupWilderness( void )
{
	sb_free( sbWildernessScenes );
	sbWildernessScenes = NULL;

	sb_push( sbWildernessScenes, scoutsScene );
	sb_push( sbWildernessScenes, hiddenCacheScene );
	sb_push( sbWildernessScenes, bearScene );
	sb_push( sbWildernessScenes, wanderingRobotScene );
	sb_push( sbWildernessScenes, brokenRobotScene );
	sb_push( sbWildernessScenes, madmanInATreeScene );
	sb_push( sbWildernessScenes, traderScene );
	//sb_push( sbWildernessScenes, chasmScene );
	//sb_push( sbWildernessScenes, abandonedHouseScene );
	//sb_push( sbWildernessScenes, caveScene );

	for( int i = 0; i < WANDER_COUNT; ++i ) {
		sb_push( sbWildernessScenes, wanderScene );
	}

	SHUFFLE_SB( sbWildernessScenes, Scene );

	// add the other NPC scenes near the start, place between events 2 and 5
	int gobSpot = 2 + ( rand( ) % 4 );
	int elfSpot = 2 + ( rand( ) % 4 );
	sb_insert( sbWildernessScenes, gobSpot, goblinScene );
	sb_insert( sbWildernessScenes, elfSpot, elfScene );

	// add the conflict scenes near the middle, place between event 6 and 10
	int spot = 6 + ( rand( ) % 5 );
	sb_insert( sbWildernessScenes, spot, elfGoblinConflictScene );
	spot = 6 + ( rand( ) % 5 );
	sb_insert( sbWildernessScenes, spot, elfScholarConflictScene );
	spot = 6 + ( rand( ) % 5 );
	sb_insert( sbWildernessScenes, spot, goblinScholarConflictScene );

	timeLeft = 26;
}

void gotoNextWildernessScene( void )
{
	timeLeft -= 2;
	if( timeLeft <= 0 ) {
		currentScene = factoryIntroScene;
	} else {
		int idx = 0;
		while( ( idx < sb_count( sbWildernessScenes ) ) && ( sbWildernessScenes[idx] == NULL ) ) {
			++idx;
		}

		if( idx < sb_count( sbWildernessScenes ) ) {
			currentScene = sbWildernessScenes[idx];
			sbWildernessScenes[idx] = NULL;
		} else {
			currentScene = wanderScene;
		}
	}
}

void leaveElfScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You bow slightly to the elf and thank it for the food. Backing away from the blood covered elf, "
				"making sure not to make any sudden movements, you make your way back to the trail you had been "
				"on originally.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void avoidElfScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s all agree that whatever is making that sound could be dangerous, or is at least "
				"dangerous enough to not worry about being found. You get back to the trail you had "
				"been following and continue on your way.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void rangedElfSuccess( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly take out your %s and before the elf can react you shoot him, but not fatally. "
				"He backs away and runs off into the woods. You give a short chase by he's nowhere to "
				"be found. Heading back to where you found him you see his pack. Going through it you grab "
				"a couple things and hurry off before he returns.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestShootWeapon( )].name );

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void rangedElfCostly( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
		gainPhysicalWound( "Stabbed by an elf." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take out your %s and aim at the elf. You're quick but not quick enough and he's able "
				"to stab you before you get your shot off. Luckily his reflexes were to back away as soon "
				"as he hit you.\n\n"
				"He backs away and runs off into the woods. You give a short chase by he's nowhere to "
				"be found. Heading back to where you found him you see his pack. Going through it you grab "
				"a couple things and hurry off before he returns.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestShootWeapon( )].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed" );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void rangedElfFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Stabbed by an elf." );
		gainPhysicalWound( "Stabbed by an elf." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"As you try to take out your %s the elf notices and stabs you multiple times. Still fumbling with your "
				"weapon you decide to retreat instead of push your attack.\n\n"
				"Luckily you're able to out run the elf and after a few minutes there's no trace of "
				"anything chasing you.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void meleeElfSuccess( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly take out your %s and before the elf can react you land a solid blow on him. "
				"He backs away and runs off into the woods. You give a short chase by he's nowhere to "
				"be found. Heading back to where you found him you see his pack. Going through it you grab "
				"a couple things and hurry off before he returns.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void meleeElfCostly( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
		gainPhysicalWound( "Stabbed by an elf." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take out your %s and you land a solid blow on him. But you weren't fast enough and "
				"he's able to get you with his knife before retreating.\n\n"
				"He backs away and runs off into the woods. You give a short chase by he's nowhere to "
				"be found. Heading back to where you found him you see his pack. Going through it you grab "
				"a couple things and hurry off before he returns.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed" );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void meleeElfFailure( void )
{
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Stabbed by an elf." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"As you try to take out your %s the elf notices and stabs you. Still fumbling with your "
				"weapon you decide to retreat instead of push your attack.\n\n"
				"Luckily you're able to out run the elf and after a few minutes there's no trace of "
				"anything chasing you.\n\n"
				"After a short walk you're back to the trail you had left.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void convinceElfSuccess( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ELF] = true;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s cleans himself off as you make your case. You tell him of the robots that are "
				"swarming the land and the dangers they will bring to everyone in the region. Appealing "
				"to his sense of safety, adventure, and moral rightness you're able to convince him to "
				"join you.\n\n"
				"After he packs his things both of you head back to the trail you had been following. Then "
				"%s set off.",
				FG_GREY, companionsData[CMP_ELF].name, getEveryoneString( false ) );
			
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void convinceElfCostly( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ELF] = true;
		gainPhysicalWound( "Took a blood oath without enough blood." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s cleans himself off as you make your case. You tell him of the robots that are "
				"swarming the land. You appeal to his sense of self-preservation. He agrees to this on one "
				"condition. That you will help him with something when this is all done. You agree.\n\n"
				"Quickly he takes out his knife again, slashing both his and your hand. He then presses his "
				"wound to yours and says that your both now bound to each other by a blood oath.\n\n"
				"After he packs his things both of you head back to the trail you had been following. Then "
				"%s set off.",
				FG_GREY, companionsData[CMP_ELF].name, getEveryoneString( false ) );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "took a blood oath" );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void convinceElfFailure( void )
{
	if( !fromStatusScreen ) {
		gainMentalWound( );
		gainSocialWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The elf cleans himself off as you make your case. You tell him how you've never "
				"met an elf before, and going on an adventure with one would make for a ripping "
				"good story when you get back to civilization.\n\n"
				"%s smiles and declines, saying he's been out here long enough on his own that "
				"he doesn't really need anyone else.",
				FG_GREY, companionsData[CMP_ELF].name );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "had a stupid idea" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "had were turned down" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void meetElfScene( void )
{
	if( !fromStatusScreen ) {
		
		++character.gear[G_RATION];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Thank the elf and leave.", leaveElfScene );
	pushSkillBasedChoice( &sbChoices, "Convince the elf to join you. (Empathy)", SKL_EMPATHY, 10, convinceElfSuccess, convinceElfCostly, convinceElfFailure );
	pushSkillBasedChoice( &sbChoices, "Attack the elf. (Fight)", SKL_SHOOT, 16 - bestFightWeaponBonus( ), meleeElfSuccess, meleeElfCostly, meleeElfFailure );
	if( bestShootWeapon( ) > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Attack the elf. (Shoot)", SKL_SHOOT, 14 - bestShootWeaponBonus( ), rangedElfSuccess, rangedElfCostly, rangedElfFailure );
	}

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test empathy success.", convinceElfSuccess );
		pushSimpleChoice( &sbChoices, "Test empathy costly.",  convinceElfCostly );
		pushSimpleChoice( &sbChoices, "Test empathy failure.", convinceElfFailure );

		pushSimpleChoice( &sbChoices, "Test fight success.", meleeElfSuccess );
		pushSimpleChoice( &sbChoices, "Test fight costly.",  meleeElfCostly );
		pushSimpleChoice( &sbChoices, "Test fight failure.", meleeElfFailure );

		pushSimpleChoice( &sbChoices, "Test shoot success.", rangedElfSuccess );
		pushSimpleChoice( &sbChoices, "Test shoot costly.",  rangedElfCostly );
		pushSimpleChoice( &sbChoices, "Test shoot failure.", rangedElfFailure );
	}

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You approach the singer, they put their finger up to their lips for brief second and "
				"continues singing, stroking the deers head. This close you're able to see that the singer "
				"isn't human. They're an elf, short with dark hair and pointed ears.\n\n"
				"Quicker than you can react the elf draws a knife and slices the deer's throat in a "
				"single clean slash. The beast doesn't panic but keeps still and slowly bleeds out "
				"as the elf serenades it.\n\n"
				"Once the deer is dead the elf stops singing and smiles at you. He introduces himself as %s. "
				"As he is doing this starts skinning and butchering the deer. He offers you some of "
				"the venison as well, as it would be too much for him to carry.\n\n"
				"What do you do?",
				FG_GREY, companionsData[CMP_ELF].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void stealFromElfSuccessScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;

		++character.gear[stolenItems[0]];
		++character.gear[stolenItems[1]];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly and quietly go through the pack and are able to get a couple things. Then "
				"just as you came you left, the singer none the wiser of what happened.\n\n"
				"After a short while you're able to get back to the path you had started out on.",
				FG_GREY, getEveryoneString( true ) );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void stealFromElfCostlyScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		gainPhysicalWound( "Tripped on a root." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You quickly go through the pack. You are able to grab one item before the singing stops. "
				"Immediately you bolt back into the forest. In your haste you trip on a root and fall. "
				"You get up quickly, and there's no other sounds in the forest.\n\n"
				"After a short while you're able to get back to the path you had started out on.",
				FG_GREY, getEveryoneString( true ) );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "tripped on a root" );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void stealFromElfFailureScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You try to go through the pack. But before you are even able to start the singing stops. "
				"Immediately you bolt back into the forest. In your haste you trip on a root and fall. "
				"You get up quickly, and there's no other sounds in the forest.\n\n"
				"After a short while you're able to get back to the path you had started out on.",
				FG_GREY, getEveryoneString( true ) );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "tripped on a root" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void gotoElfStealthSuccessScene( void )
{
	// can auto success steal
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Steal from the pack.", stealFromElfSuccessScene );
	pushSimpleChoice( &sbChoices, "Approach them.", meetElfScene );
	pushSimpleChoice( &sbChoices, "Leave right now.", avoidElfScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Not even the birds in the trees notice you sneaking up to the clearing where the "
				"the singer sits. The song has not stopped nor has the singer seemed to notice you.\n\n"
				"It looks like their pack is nearby, it would be very easy to steal some things from it.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void gotoElfStealthCostlyScene( void )
{
	// hard to steal
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Steal from the pack. (Stealth)", SKL_STEALTH, 14, stealFromElfSuccessScene, stealFromElfCostlyScene, stealFromElfFailureScene );
	pushSimpleChoice( &sbChoices, "Approach them.", meetElfScene );
	pushSimpleChoice( &sbChoices, "Leave right now.", avoidElfScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test stealth success.", stealFromElfSuccessScene );
		pushSimpleChoice( &sbChoices, "Test stealth costly.", stealFromElfCostlyScene );
		pushSimpleChoice( &sbChoices, "Test stealth failure.", stealFromElfFailureScene );
	}
	
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The singer's voice stops and you hold your breath. For a few heartbeats all is "
				"silent. Then the singing resumes.\n\n"
				"It looks like their pack is nearby, you could try to steal some things from it.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void gotoElfStealthFailureScene( void )
{
	// knows you're there
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Approach them.", meetElfScene );
	pushSimpleChoice( &sbChoices, "Leave right now.", avoidElfScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The singer pauses for a moment and calls out to you. Saying there's no need to "
				"be afraid. They then turn away and continue singing.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void gotoElfNormalScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Approach the mysterious siren.", meetElfScene );
	pushSimpleChoice( &sbChoices, "Leave with all due haste.", avoidElfScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"All of you approach the sound. Whoever is singing is sure to have heard you "
				"but the singing has not changed. You enter out into a glade and see a short "
				"human-like figure sitting with deer resting it's head in their lap. They continue singing "
				"but motion for you to come closer.",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScene( void )
{
	// hear a song coming from the woods, can choose to go, sneak, or leave
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Ignore it.", avoidElfScene );
	pushSimpleChoice( &sbChoices, "Approach the sound.", gotoElfNormalScene );
	pushSkillBasedChoice( &sbChoices, "Carefully approach the sound. (Stealth)", SKL_STEALTH, 16,
		gotoElfStealthSuccessScene, gotoElfStealthCostlyScene, gotoElfStealthFailureScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test stealth success.", gotoElfStealthSuccessScene );
		pushSimpleChoice( &sbChoices, "Test stealth costly.", gotoElfStealthCostlyScene );
		pushSimpleChoice( &sbChoices, "Test stealth failure.", gotoElfStealthFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"While traveling %s hears some sound off in the distance. Getting closer you recognize "
				"it as singing. Whoever or whatever is singing is close by.\n\n"
				"What do you do?",
				FG_GREY, companionsData[chosenCmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void leaveGoblinScene( void )
{
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Slowly you make your way back up the slope. The goblin keeps a steady distance away from you, "
				"getting neither too close nor too far from you. As you think it's starting to get closer you "
				"hear %s call out for you. The creature panics and runs off.\n\n"
				"You're able to get up the slope without too much trouble after it's gone.",
				FG_GREY, companionsData[chosenCmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void lieGoblinSuccessScene( void )
{
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_GOBLIN] = true;
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,

				"You start talking to the goblin. Telling it of the many battles you've fought, the "
				"horrible creatures you vanquished, and your current quest to face down a horde of "
				"robots. He hangs on your every word. You feel like one of the master storytellers "
				"you've listened to at the tavern.\n\n"
				"After a while the goblin relaxes and introduces himself as %s. He starts walking "
				"towards you with a toothy grin. You heart skips a beat but he walks past you and "
				"then starts to help you up the slope. Confused you ask him what's going on. "
				"He replies that he's going to travel with you. If you're able to slay such powerful "
				"beats you must be able to help him get a trophy that will make him famous in his "
				"tribe. No matter how you try to approach the subject it seems he's made up his mind.\n\n"
				"Your new companion helps you up the slope and you make your way back to the path.",
				FG_GREY, companionsData[CMP_GOBLIN].name );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_GOBLIN].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void lieGoblinCostlyScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = randomGearToLose( );
		--character.gear[stolenItems[0]];
		character.hasCompanion[CMP_GOBLIN] = true;
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You start talking to the goblin. Telling him about how rich and powerful you are. "
				"The goblin laughs and replies in broken speech that you aren't, you have no proof. "
				"You take out a %s and show it to the goblin, proof that you have wealth beyond what "
				"it's every seen. The creature looks at it, then grabs it and looks at it closer.\n\n"
				"After a few moments he nods and says he'll work for you. You look back at it confused.\n\n"
				"He introduces himself as %s and says as long as you pay him with stuff like this he'll "
				"protect you.\n\n"
				"Your new companion helps you up the slope and you make your way back to the path.",
				FG_GREY, gearData[stolenItems[0]].name, companionsData[CMP_GOBLIN].name );

			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_GOBLIN].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void lieGoblinFailureScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = randomGearToLose( );
		--character.gear[stolenItems[0]];
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You start talking to the goblin. Telling it about how rich and powerful you are. "
				"The goblin laughs and replies in broken speech that you aren't, you have no proof. "
				"You take out a %s and show it to the goblin, proof that you have wealth beyond what "
				"it's every seen. The creature looks at it, then grabs it and runs away.\n\n"
				"You're able to get up the slope without too much trouble after it's gone.",
				FG_GREY, gearData[stolenItems[0]].name );

			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void fightGoblinSuccessScene( void )
{
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You lunge at the goblin with your %s. The creature doesn't have enough time "
				"to prepare a proper defense and is forced to retreat, less the eye you took "
				"from it.\n\n"
				"You're able to get up the slope without too much trouble after it's gone.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void fightGoblinCostlyScene( void )
{
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );
		gainPhysicalWound( "Punched by a goblin." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You lunge at the goblin with your %s. The goblin is barely able to get his blade up "
				"in time to defend himself. While is blade is immobilized you take the opportunity to get "
				"blow in. The goblin starts smiling and then punches you in the stomach, causing you to "
				"double over.\n\n"
				"Before he's able to finish you off you hear %s call out for you. The creature looks at "
				"you then back up the slope. After a short delibiration it retreats into the woods.\n\n"
				"You're able to get up the slope without too much trouble after it's gone.",
				FG_GREY, gearData[bestFightWeapon( )].name, companionsData[chosenCmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were punched" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void fightGoblinFailureScene( void )
{
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );
		gainPhysicalWound( "Sliced up by a goblin." );
		gainPhysicalWound( "Sliced up by a goblin." );
		gainPhysicalWound( "Sliced up by a goblin." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You lunge at the goblin with your %s. The attack is clumsy though and you overshoot. "
				"This gives him enough time to bring is massive blade down on you.\n\n"
				"Before he's able to finish you off you hear %s call out for you. The creature looks at "
				"you then back up the slope. After a short delibiration it retreats into the woods.\n\n"
				"You're able to get up the slope without too much trouble after it's gone.",
				FG_GREY, gearData[bestFightWeapon( )].name, companionsData[chosenCmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were cut" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were cut" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were cut" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Back away slowly.", leaveGoblinScene );
	pushSkillBasedChoice( &sbChoices, "Attack the goblin. (Fight)", SKL_FIGHT, 16 - bestFightWeaponBonus( ), fightGoblinSuccessScene, fightGoblinCostlyScene, fightGoblinFailureScene );
	pushSkillBasedChoice( &sbChoices, "Try to convince the goblin it shouldn't eat you. (Lie)", SKL_LIE, 10, lieGoblinSuccessScene, lieGoblinCostlyScene, lieGoblinFailureScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", fightGoblinSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.",  fightGoblinCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", fightGoblinFailureScene );

		pushSimpleChoice( &sbChoices, "Test lie success.", lieGoblinSuccessScene );
		pushSimpleChoice( &sbChoices, "Test lie costly.",  lieGoblinCostlyScene );
		pushSimpleChoice( &sbChoices, "Test lie failure.", lieGoblinFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s are walking long when you slip and fall down a slope. You don't appear to be "
				"hurt. But someone stands over you. Wearing scraggly clothes and carrying large blade you. The sun behind them and you "
				"can't see them very well.\n\n"
				"They make no move to help you, but neither are they attacking you. Slowly you get up, never "
				"taking your eyes off them. Once the sun is out of your eyes you can see them better. About "
				"as tall as you are with patchwork colored skin and tusks emerging from it's bottom lips. It's wiry with "
				"an oddly large gut.\n\n"
				"A goblin. They can be dangerous, not because they're prone to violence (although they are), "
				"but because of how erratic they are.\n\n"
				"You stand there, in a staring contest with the goblin. You're too close to make use of ranged weapons.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

bool lostWound;
void scoutsScoutScene( void )
{
	// gain ration, random gear, and healing
	if( !fromStatusScreen ) {
		stolenItems[0] = G_RATION;
		stolenItems[1] = rand( ) % NUM_GEAR;

		++character.gear[stolenItems[0]];
		++character.gear[stolenItems[1]];
		
		lostWound = false;
		if( character.wounds_physical > 0 ) {
			--character.wounds_physical;
			lostWound = true;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Say goodbye and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You ask if there are any caches of equipment nearby. The one you know is named Erlea "
				"says they have some extra stuff that's just weighing them down if you would need it. "
				"She also offers to look at any wounds you may have.",
				FG_GREY );

			if( lostWound ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
			}

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void scoutsConvinceSuccessScene( void )
{
	// gain ration, random gear, and healing
	if( !fromStatusScreen ) {
		stolenItems[0] = G_RATION;
		stolenItems[1] = rand( ) % NUM_GEAR;

		++character.gear[stolenItems[0]];
		++character.gear[stolenItems[1]];

		lostWound = false;
		if( character.wounds_physical > 0 ) {
			--character.wounds_physical;
			lostWound = true;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Say goodbye and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You ask if they might have anything to help you on your journey. The lead scouts answers "
				"that she has some extra suff that's just weighing them down if you need it. "
				"She also offers to look at any wounds you may have.",
				FG_GREY );

			if( lostWound ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
			}

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void scoutsConvinceCostlyScene( void )
{
	// one random item and some healing
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		++character.gear[stolenItems[0]];

		lostWound = false;
		if( character.wounds_physical > 0 ) {
			--character.wounds_physical;
			lostWound = true;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Say goodbye and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You ask if they might have anything to help you on your journey. The lead scouts answers "
				"that she has a little extra suff that's just weighing them down if you need it. "
				"She also offers to look at any wounds you may have.",
				FG_GREY );

			if( lostWound ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
			}

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void scoutsConvinceFailureScene( void )
{
	// some healing
	if( !fromStatusScreen ) {
		lostWound = false;
		if( character.wounds_physical > 0 ) {
			--character.wounds_physical;
			lostWound = true;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Say goodbye and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You ask if they might have anything to help you on your journey. The lead scouts answers "
				"that they aren't carrying any extra supplies right now, but she does offer to look at "
				"any wounds you may have.",
				FG_GREY );

			if( lostWound ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void scoutsScene( void )
{
	// meet with some scouts, if you're a scout get rations and some random gear, otherwise you can convince them to give you some rations
	//  they'll also heal one physical wound for you
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	if( isCharacterClass( CC_SCOUT ) ) {
		pushSimpleChoice( &sbChoices, "You know these guys. (Scout)", scoutsScoutScene );
	} else {
		pushSkillBasedChoice( &sbChoices, "Convince them to give you some aid. (Empathy)", SKL_EMPATHY, 12,
			scoutsConvinceSuccessScene, scoutsConvinceCostlyScene, scoutsConvinceFailureScene );
	}

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test empathy success.", scoutsConvinceSuccessScene );
		pushSimpleChoice( &sbChoices, "Test empathy costly.", scoutsConvinceCostlyScene );
		pushSimpleChoice( &sbChoices, "Test empathy failure.", scoutsConvinceFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Say goodbye and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"While walking along you're suprised by two shapes dropping from the trees. You immediately drop into "
				"a fighting stance but relax when you recognize them as scouts from the settlement. They recognize you "
				"and %s as well and apologize for scaring you.\n\n"
				"They tell you that you're headed in the right direction and that they wish you success.\n\n"
				"What do you do?",
				FG_GREY, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void hiddenCacheGoblinScene( void )
{
	// gain one random gear, Zobd also takes one
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
		++character.gear[stolenItems[0]];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Grabbing %s you point at the door and tell him to open. He grins and runs over. It takes "
				"him a bit of time and no little strain, but he opens it. Immediately he hops down in and comes "
				"back with a %s, saying it's his payment as walks past. Luckily it looks like he didn't "
				"take everything.",
				FG_GREY, companionsData[CMP_GOBLIN].name, gearData[stolenItems[1]].name );

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void hiddenCacheSuccessScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		stolenItems[1] = rand( ) % NUM_GEAR;
		++character.gear[stolenItems[0]];
		++character.gear[stolenItems[1]];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The lock is fairly simple. It only takes a couple of minutes for it to pop open. "
				"Luckly for you this is a relatively new cache so everything should still be fresh. ",
				FG_GREY );

			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void hiddenCacheCostlyScene( void )
{
	if( !fromStatusScreen ) {
		stolenItems[0] = rand( ) % NUM_GEAR;
		++character.gear[stolenItems[0]];

		chosenCmp = getRandomCompanion( );

		gainSocialWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The lock is fairly simple. So why is it taking so long to get into? After half an hour "
				"you're able to pop the lock. Frustated you jump down into the cache quickly and hear "
				"something break under your feet as you land.\n\n"
				"%s watched the entire thing and can't hold back their laughter at your screams of defeat. "
				"There's still something down here though, so you're not empty handed.",
				FG_GREY, companionsData[chosenCmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "embarrassed yourself" );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, stolenItems[0] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void hiddenCacheFailureScene( void )
{
	if( !fromStatusScreen ) {
		chosenCmp = getRandomCompanion( );

		gainSocialWound( );
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The lock looks fairly simple. So why can't you open it? After an hour of trying all "
				"the different methods you know to try and open this lock you give up. You try to kick "
				"the lock in frustration but miss and fall.\n\n"
				"%s watched the entire thing and can't hold back their laughter at your screams of defeat.",
				FG_GREY, companionsData[chosenCmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "were outsmarted by an inanimate object" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "embarrassed yourself" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void hiddenCacheScene( void )
{
	// just a chance at some items
	//  craft to open it
	//  goblin auto-breaks
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	if( character.hasCompanion[CMP_GOBLIN] ) {
		pushSimpleChoice( &sbChoices, "Zobd should be strong enough to open it.", hiddenCacheGoblinScene );
	}
	pushSkillBasedChoice( & sbChoices, "Attempt to open the door. (Craft)", SKL_CRAFT, 10, hiddenCacheSuccessScene, hiddenCacheCostlyScene, hiddenCacheFailureScene );
	pushSimpleChoice( &sbChoices, "Ignore it and continue on your journey.", gotoNextWildernessScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test craft success.", hiddenCacheSuccessScene );
		pushSimpleChoice( &sbChoices, "Test craft costly.", hiddenCacheCostlyScene );
		pushSimpleChoice( &sbChoices, "Test craft failure.", hiddenCacheFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"An odd sound comes from the ground as you're walking along. Inspecting the area you "
				"find an old door buried under some leaves. It looks like it's locked.\n\n"
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearElfScene( void )
{
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Thank the elf and continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Not being sure what to do you stand still and call out for help. A few moments later %s "
				"comes out of the woods. Immediately he starts singing the same song as when you "
				"first met. The bear slowly calms down, eventually returning to it's food. While it's "
				"distracted you both escape.\n\n",
				FG_GREY, companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearSmartSuccessScene( void )
{
	// get out safe
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ok, don't run, don't yell, don't panic. You back away slowly from the bear, never "
				"turning away from it, and speaking to it in a calm voice. The bear makes no attempts "
				"to advance on you. Once out of sight you finally breath and are quite thankful you had "
				"already emptied your bladder earlier.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearSmartCostlyScene( void )
{
	// throw a ration at it as backing away, drop some equipment as well
	static Gear lostItem;
	if( !fromStatusScreen ) {
		do {
			lostItem = randomGearToLose( );
		} while( lostItem == G_RATION );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ok, don't run, don't yell, don't panic. You back away slowly from the bear, never "
				"turning away from it, and speaking to it in a calm voice. The bear slowly advances on "
				"you. In a panic you grab something from your pack and throw it onto the ground between you "
				"two. This distracts the bear long enough for you to get away.",
				FG_GREY );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, lostItem );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearSmartFailureScene( void )
{
	// play dead, bear mauls you a bit and then wanders off, three wounds
	static Gear lostItem;
	static Companions cmp;
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
		cmp = getRandomCompanion( );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ok, don't run, don't yell, don't panic. You decide the best way to deal with this is "
				"to play dead. Curling into a ball and spreading out your arms and legs so the bear "
				"can't flip you. You wait. The bear approaches you. You're very glad you emptied your "
				"bladder earlier.\n\n"
				"The bear plays with you for a bit, swatting at you and trying to flip you over. As "
				"the panic is becoming unbearable you hear someone making a lot of noise. The "
				"bear looks up and runs into the woods. %s comes out shortly afterwards and does their "
				"best to attend to your wounds.",
				FG_GREY, companionsData[cmp].name );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

int bearHealth;
void bearMeleeSuccessScene( void );
void bearMeleeCostlyScene( void );
void bearMeleeFailureScene( void );

void bearMeleeSuccessScene( void )
{
	// one damage to the bear
	if( !fromStatusScreen ) {
		--bearHealth;
	}
	startScene( );

	Choice* sbChoices = NULL;

	if( bearHealth > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
		if( isCharacterClass( CC_DEV_TEST ) ) {
			pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
			pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
			pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
		}
	} else {
		pushSimpleChoice( &sbChoices, "Continue on your journey", gotoNextWildernessScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a swipe at the bear and are able to land a solid blow. It's claws miss you.",
				FG_GREY );
			if( bearHealth <= 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"After your last blow the bear backs off and retreats.",
					FG_GREY );
			}
			//pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearMeleeCostlyScene( void )
{
	// one damage a piece
	if( !fromStatusScreen ) {
		--bearHealth;
		gainPhysicalWound( "Eaten by a bear." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	if( bearHealth > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
		if( isCharacterClass( CC_DEV_TEST ) ) {
			pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
			pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
			pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
		}
	} else {
		pushSimpleChoice( &sbChoices, "Continue on your journey", gotoNextWildernessScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a swipe at the bear and are able to land a solid blow. It's claws miss you.",
				FG_GREY );
			if( bearHealth <= 0 ) {
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"After your last blow the bear backs off and retreats.",
					FG_GREY );
			}
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearMeleeFailureScene( void )
{
	// three damage to the player
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
	}
	startScene( );

	if( isDead( ) ) {
		int x = 0;
	}

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You take a swipe at the bear but miss. The bears claws don't miss you though.",
				FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearShootSuccessScene( void )
{
	// one damage to the bear
	if( !fromStatusScreen ) {
		--bearHealth;
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ok, don't run, don't yell, don't panic. You quickly pull out your %s and shoot the bear. "
				"It seems to have hit but the animal just charges.",
				FG_GREY, gearData[bestShootWeapon( )] );
			//pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearShootCostlyScene( void )
{
	// one damage a piece
	if( !fromStatusScreen ) {
		--bearHealth;
		gainPhysicalWound( "Eaten by a bear." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ok, don't run, don't yell, don't panic. The bear starts charging and you're able get a shot "
				"off at it. This doesn't slow it down at all and it answers with a swipe of it's claws.", 
				FG_GREY, gearData[bestShootWeapon( )] );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearShootFailureScene( void )
{
	// three damage to the player
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
		gainPhysicalWound( "Eaten by a bear." );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Fight the bear. (Fight)", SKL_FIGHT, 20 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", bearMeleeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test fight costly.", bearMeleeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test fight failure.", bearMeleeFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The bear starts charging and before you're able get a shot "
				"off at it. It's shreds you with it's claws.",
				FG_GREY, gearData[bestShootWeapon( )] );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were mauled by a bear" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void bearScene( void )
{
	// stumble on a bear eating a dead deer
	//  shoot - will be a fight to the death, good chance to get fucked up, repeat until bear has taken three wounds, once in melee the bear can't be shot
	//  fight - same as shoot
	//  knowledge/survival - back away slowly and don't get hurt, survival is easier, chance of damage from stumbling
	//  elf - calm animal
	if( !fromStatusScreen ) {
		timeLeft += 2;
		bearHealth = 3;
	}

	startScene( );

	Choice* sbChoices = NULL;

	if( bestShootWeapon( ) > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Shoot the bear. (Shoot)", SKL_SHOOT, 14 - bestShootWeaponBonus( ), bearShootSuccessScene, bearShootCostlyScene, bearShootFailureScene );
	}
	pushSkillBasedChoice( &sbChoices, "Attack the bear. (Fight)", SKL_FIGHT, 18 - bestFightWeaponBonus( ), bearMeleeSuccessScene, bearMeleeCostlyScene, bearMeleeFailureScene );
	if( character.ms_knowledge > ( character.ms_survival + 2 ) ) {
		pushSkillBasedChoice( &sbChoices, "Try to outsmart it. (KNOWLEDGE)", SKL_KNOWLEDGE, 14, bearSmartSuccessScene, bearSmartCostlyScene, bearSmartFailureScene );
	} else {
		pushSkillBasedChoice( &sbChoices, "Try to outsmart it. (SURVIVAL)", SKL_SURVIVAL, 12, bearSmartSuccessScene, bearSmartCostlyScene, bearSmartFailureScene );
	}
	if( character.hasCompanion[CMP_ELF] ) {
		pushSimpleChoice( &sbChoices, "Call for help.", bearElfScene );
	}

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test shoot success.", bearShootSuccessScene );
		pushSimpleChoice( &sbChoices, "Test shoot costly.",  bearShootCostlyScene );
		pushSimpleChoice( &sbChoices, "Test shoot failure.", bearShootFailureScene );
		pushSimpleChoice( &sbChoices, "Test melee success.", bearMeleeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test melee costly.",  bearMeleeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test melee failure.", bearMeleeFailureScene );
		pushSimpleChoice( &sbChoices, "Test smart success.", bearSmartSuccessScene );
		pushSimpleChoice( &sbChoices, "Test smart costly.", bearSmartCostlyScene );
		pushSimpleChoice( &sbChoices, "Test smart failure.", bearSmartFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"After taking a short privacy back and refastening your pants you hear a roar behind you. "
				"Turning around you see a large bear standing over the carcass of a dead dear.\n\n"
				"What do you do?",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotStealthSuccessScene( void )
{
	// you get around it successfully
	static Companions cmp;
	if( !fromStatusScreen ) {
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s use the trees as cover to successfully sneak around the robot.",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotStealthCostlyScene( void )
{
	// you lose some gear
	static Companions cmp;
	static Gear lostGear;
	if( !fromStatusScreen ) {
		cmp = getRandomCompanion( );
		lostGear = randomGearToLose( );
		--character.gear[lostGear];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s use the trees as cover to try and skirt around the robot. When you're almost past it "
				"you trip and a root and something comes flying out of your pack. The robot notices this "
				"and comes over to investigate. You mutter off some curses to The Red Marquis under your "
				"breath before continuing on your way.\n\n",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, lostGear );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotStealthFailureScene( void )
{
	// you get seen and have to run, getting lost and losing some gear
	static Companions cmp;
	static Gear lostGear;
	if( !fromStatusScreen ) {
		cmp = getRandomCompanion( );
		lostGear = randomGearToLose( );
		timeLeft += 2;
		--character.gear[lostGear];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s use the trees as cover to try and skirt around the robot. Unfortunately you trip "
				"and stumble on some debris. Something you were carrying flies out of your pack. As you "
				"reach over to grab it the robots eyes turn towards you. It starts heading "
				"in your direction while clacking it's claws ominously.\n\n"
				"You turn and run as fast as you can. You lose it eventually, but not before you become "
				"very lost.\n\n"
				"You spend a good amount of time trying to get back on the trail you had been following. "
				"Eventually you hear %s calling for you and are able to find them and get under way.",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, lostGear );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotAcrobaticsSuccessScene( void )
{
	// you get through
	static Companions cmp;
	if( !fromStatusScreen ) {
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s take off at in a dash, trying to get past the robot while avoiding it's swings.\n\n"
				"You're able to run past and avoid all it's swings. %s almost takes a nasty hit but "
				"you're able to pull them out of the way before the tree connects. You both hurry back "
				"into the woods past the wild automaton.\n\n",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotAcrobaticsCostlyScene( void )
{
	// you get hit
	static Companions cmp;
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Clubbed to death by an insane robot." );
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s take off at in a dash, trying to get past the robot while avoiding it's swings.\n\n"
				"As you're running past the tree it's swinging around clips you, sending you flying. "
				"Disorientated and stunned you sit there for a while. Luckily the robot didn't seem "
				"to notice you and is continuing on in it's game.\n\n"
				"Luckily %s finds you quickly and gets you to a safe spot. You're able to walk soon "
				"afterwards and you're able to continue with no big time losses.",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were hit with a small tree" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotAcrobaticsFailureScene( void )
{
	// you get hit, and get lost
	static Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;
		gainPhysicalWound( "Clubbed to death by an insane robot." );
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s take off at in a dash, trying to get past the robot while avoiding it's swings.\n\n"
				"As you're running past the tree it's swinging around clips you, sending you flying. "
				"Disorientated and stunned you sit there for a while. Luckily the robot didn't seem "
				"to notice you and is continuing on in it's game.\n\n"
				"You spend a good amount of time trying to get back on the trail you had been following. "
				"Eventually you hear %s calling for you and are able to find them and get under way.",
				FG_GREY, getEveryoneString( true ), companionsData[cmp].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were hit with a small tree" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotMapSuccessScene( void )
{
	// you find a way around
	if( !fromStatusScreen ) {
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Following the directions on the map you're able to quickly find a path that "
				"should lead you to where you need to go. It's also slightly quicker so you've "
				"lost no time on your progress.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotMapCostlyScene( void )
{
	// slight delay
	Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 1;
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The map is complex, using a notation you aren't familiar with. After a while you're "
				"able to find a nearby trail that should take you in the correct diration.\n\n"
				"After getting to it %s thinks you've lost about half a day worth of travel.",
				FG_GREY, companionsData[cmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotMapFailureScene( void )
{
	// get lost
	Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"The map doesn't seem to be of this area. Either that or you don't know how to "
				"read it correctly.\n\n"
				"Not knowing where to go you spend a long time trying to find a path that'll take "
				"you where you need to go. After you find one %s thinks you've lost about a days "
				"worth of travel.",
				FG_GREY, companionsData[cmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotAroundScene( void )
{
	// get lost
	Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Not knowing where to go you spend a long time trying to find a path that'll take "
				"you where you need to go. After you find one %s thinks you've lost about a days "
				"worth of travel.",
				FG_GREY, companionsData[cmp].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderingRobotScene( void )
{
	// you can try to sneak, run past it, or go around it
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Attempt to sneak around it. (Stealth)", SKL_STEALTH, 12, wanderingRobotStealthSuccessScene, wanderingRobotStealthCostlyScene, wanderingRobotStealthFailureScene );
	pushSkillBasedChoice( &sbChoices, "Attempt to run past it. (Acrobatics)", SKL_ACROBATICS, 14, wanderingRobotAcrobaticsSuccessScene, wanderingRobotAcrobaticsCostlyScene, wanderingRobotAcrobaticsFailureScene );
	if( character.gear[G_MAP] > 0 ) {
		pushSkillBasedChoice( &sbChoices, "Use your map to find a way around it. (Survival)", SKL_SURVIVAL, 10, wanderingRobotMapSuccessScene, wanderingRobotMapCostlyScene, wanderingRobotMapFailureScene );
	}
	pushSimpleChoice( &sbChoices, "Find a different way.", wanderingRobotAroundScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test stealth success.", wanderingRobotStealthSuccessScene );
		pushSimpleChoice( &sbChoices, "Test stealth costly.",  wanderingRobotStealthCostlyScene );
		pushSimpleChoice( &sbChoices, "Test stealth failure.", wanderingRobotStealthFailureScene );

		pushSimpleChoice( &sbChoices, "Test acrobatics success.", wanderingRobotAcrobaticsSuccessScene );
		pushSimpleChoice( &sbChoices, "Test acrobatics costly.",  wanderingRobotAcrobaticsCostlyScene );
		pushSimpleChoice( &sbChoices, "Test acrobatics failure.", wanderingRobotAcrobaticsFailureScene );

		pushSimpleChoice( &sbChoices, "Test survival success.", wanderingRobotMapSuccessScene );
		pushSimpleChoice( &sbChoices, "Test survival costly.",  wanderingRobotMapCostlyScene );
		pushSimpleChoice( &sbChoices, "Test survival failure.", wanderingRobotMapFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Ahead of you comes a large crashing sound. Slowly going forward you see a "
				"large robot uprooting trees and using them as clubs to smash other trees.\n\n"
				"You and %s are puzzled by the behavior but the more immediate issue is that "
				"the robot is in your path. You could try to get past it, or you could "
				"go back and try to find a way around it.\n\n"
				"What do you do?",
				FG_GREY, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void brokenRobotBreakSuccess( void )
{
	// you break it, gain back a wound
	static int woundType;
	static Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;

		woundType = -1;
		if( character.wounds_mental > 0 ) {
			woundType = 0;
			--character.wounds_mental;
		} else if( character.wounds_social > 0 ) {
			woundType = 1;
			--character.wounds_social;
		} else if( character.wounds_physical > 0 ) {
			woundType = 2;
			--character.wounds_physical;
		}

		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	// can also ignore it
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You approach the robot carefully, avoiding some of it's legs that still look active. "
				"Since it's disabled you spend your time looking over it. Noticing a piece of shattered "
				"shell one of it's eyes you come up with a plan. You head over to the eye and are able to "
				"use your %s to pry it out. Once out you can reach inside and start disconnecting things. "
				"The robot soon stops moving.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			switch( woundType ) {
			case 0:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Knowing that these things can be destroyed and learning some more about how they were made "
					"relaxes you a bit.",
					FG_GREY );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Mental Wound!", FG_GREEN );
				break;
			case 1:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"%s cheers you on as you step triumpantly off the robots shell. You're "
					"feeling a bit more confident in yourself.",
					FG_GREY, companionsData[cmp].name );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Social Wound!", FG_GREEN );
				break;
			case 2:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You let out a mighty roar as you grab a handful of wiring and rip "
					"it out of the robots eye socket. You feel invigorated and that you could "
					"fight off any number of these things",
					FG_GREY, companionsData[cmp].name );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
				break;
			default:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You feel satisfied as the robot's motors slow down and stop. Hopping of the "
					"husk you get back to the trail you had been following.",
					FG_GREY, companionsData[cmp].name );
				break;
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void brokenRobotBreakCostly( void )
{
	// you break it, but also break your weapon, gain back a social or a mental wound
	static Gear lostGear;
	static int woundType;
	static Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;

		lostGear = bestFightWeapon( );
		--character.gear[lostGear];

		woundType = -1;
		if( character.wounds_mental > 0 ) {
			woundType = 0;
			--character.wounds_mental;
		} else if( character.wounds_social > 0 ) {
			woundType = 1;
			--character.wounds_social;
		} else if( character.wounds_physical > 0 ) {
			woundType = 2;
			--character.wounds_physical;
		}

		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	// can also ignore it
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You approach the robot carefully, avoiding some of it's legs that still look active. "
				"Once in a good spot you start attacking it with your %s, aiming for joints or other "
				"parts that look like they're already weak. It takes about half an hour, but you're able "
				"to finally completely disable it.",
				FG_GREY, gearData[lostGear].name );

			switch( woundType ) {
			case 0:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"Knowing that these things can be destroyed and learning some more about how they were made "
					"relaxes you a bit. Unfortunately your weapon did not survive the process.",
					FG_GREY );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Mental Wound!", FG_GREEN );
				break;
			case 1:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"%s cheers you on as you step triumpantly off the robots shell. You're "
					"feeling a bit more confident in yourself. Unfortunately your weapon did not survive the process.",
					FG_GREY, companionsData[cmp].name );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Social Wound!", FG_GREEN );
				break;
			case 2:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You let out a mighty roar as you land the final blow with your %s. "
					"The robot finally dies down. You feel invigorated and that you could "
					"fight off any number of these things. Unfortunately your weapon did not fair as well as you.",
					FG_GREY, companionsData[cmp].name );
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You lose a Physical Wound!", FG_GREEN );
				break;
			default:
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"You feel satisfied as the robot's motors slow down and stop. Hopping of the "
					"husk you realize your weapon didn't surive the task. Tossing it aside you get back to "
					"the trail you had been following.",
					FG_GREY, gearData[lostGear].name );
				break;
			}

			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, lostGear );
			
			//pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed with by a robot" );
			//standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed with by a robot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void brokenRobotBreakFailure( void )
{
	// you injure yourself in the process of breaking it
	if( !fromStatusScreen ) {
		timeLeft += 2;

		gainPhysicalWound( "Impaled by a robot." );
		gainPhysicalWound( "Impaled by a robot." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	// can also ignore it
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You charge at the robot with your %s, swinging wildly at it. The robot responds in "
				"more gibberish before stabbing you with one of it's broken legs before you get in "
				"range. The fresh wound makes you rethink your strategy. It can't stop you, and your "
				"goal is more important than a single robot. You decide to leave it to its struggle.",
				FG_GREY, gearData[bestFightWeapon( )].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed with by a robot" );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were stabbed with by a robot" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void brokenRobotScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	// breaking it will heal some wounds
	if( bestFightWeapon( ) >= 0 ) {
		pushSkillBasedChoice( &sbChoices, "Try to destroy the robot. (Fight)", SKL_CRAFT, 16 - bestFightWeaponBonus( ),
			brokenRobotBreakSuccess, brokenRobotBreakCostly, brokenRobotBreakFailure );
	}
	// can also ignore it
	pushSimpleChoice( &sbChoices, "Ignore it and continue on your journey.", gotoNextWildernessScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test fight success.", brokenRobotBreakSuccess );
		pushSimpleChoice( &sbChoices, "Test fight costly.", brokenRobotBreakCostly );
		pushSimpleChoice( &sbChoices, "Test fight failure.", brokenRobotBreakFailure );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s all hear a strange voice yelling nonsense. You're still trying to puzzle out what "
				"\"Up branch to easily missed by do.\" means "
				"when you see the source. A large robot has partially broken down and is yelling this nonsense "
				"as it tries to climb out of a hole with broken legs.\n\n"
				"What do you do?",
				FG_GREY, getEveryoneString( true ) );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void madmanInATreeScene( void )
{
	// madman can either hurt you mentally, socially, or physically
	//  mentally by asking you a question you can't answer
	//  socially by pissing on you from the tree
	//  physically by throwing something at you
	static int madmanChoice;
	static Gear item;
	static Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;

		madmanChoice = rand( ) % 3;

		switch( madmanChoice ) {
		case 0: // mental anguish
			gainMentalWound( );
			break;
		case 1: // social anguish
			// if you have towel to clean youself off you're fine
			if( character.gear[G_TOWEL] == 0 ) {
				gainSocialWound( );
				if( character.hasCompanion[CMP_ELF] ) {
					cmp = CMP_ELF;
				} else {
					cmp = CMP_SCHOLAR;
				}
			}
			break;
		case 2: // physical anguish
			gainPhysicalWound( "Beaned by a madman in a tree." );
			item = rand( ) % NUM_GEAR;
			break;
		}
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s are walking along when you hear something above you in trees. Looking up you see an old "
				"unshaven man making strange faces at you. You ask him who he is and if he needs any help.",
				FG_GREY, getEveryoneString( true ) );

			switch( madmanChoice ) {
			case 0: // mental, asks about the tides and knows a whole lot more than you
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"The old man just says he wants to know why the tides work. You explain to him how the "
					"moon pulls on the water creating the rising and sinking tides. He laughs and says he "
					"knows how it works and he had asked for why it works. While sitting on a branch the old man "
					"goes into a lecture about tides, explaining in far greater detail things you know "
					"nothing about. %s is listening intently.\n\n"
					"The lecture only stops after you walk away, and not because the old man has stopped "
					"talking. After he's out of earshot %s says that if even half of what the old man "
					"said was true it could fill in lots of missing gaps in The Coalition's knowledge.",
					FG_GREY );

				pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "were lectured to by a madman" );
				break;
			case 1: // social, pisses on you
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"The old mans says he could use some help and that all you need to do is stand real still. "
					"He then stands on the branch, almost falling off, before loosing a stream of fetid yellow "
					"liquid onto your head. You stand in stunned silence for far too long before getting out "
					"of the way.",
					FG_GREY );
				if( character.gear[G_TOWEL] > 0 ) {
					pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"Luckily you know where your towel is and you use it to clean yourself up.",
						FG_GREY );
				} else {
					pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
						"It takes a few hours to reach a stream where you can wash yourself off. %s spends that "
						"time trying to not stand downwind of you.",
						FG_GREY, companionsData[cmp].name );
					pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "had an impromptu shower" );
				}
				break;
			case 2: // physical throws something at you, but you gain an item
				pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
					"With reflexes belying his age he throws something directly at you and yells catch. "
					"You aren't fast enough and get hit in the head with the package. The old man asks for "
					"it back but you walk off with your new loot.",
					FG_GREY );
				pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were beaned by a madman" );
				pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, item );
				break;
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void traderSuccessScene( void )
{
	static Gear items[2];
	static Gear payment;
	if( !fromStatusScreen ) {
		if( character.gear[G_MONEY] > 0 ) {
			payment = G_MONEY;
		} else {
			payment = randomGearToLose( );
		}

		do {
			items[0] = rand( ) % NUM_GEAR;
		} while( items[0] == payment );

		do {
			items[1] = rand( ) % NUM_GEAR;
		} while( items[0] == payment );

		++character.gear[items[0]];
		++character.gear[items[1]];
		--character.gear[payment];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Wish him luck and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You spend the better part of an hour going over the merchants wares and haggling. "
				"Finally you're able to reach an agreement. Goods exchange hands and the merchant "
				"seems pleased with himself.",
				FG_GREY );

			
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, payment );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, items[0] );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, items[1] );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void traderCostlyScene( void )
{
	static Gear item;
	static Gear payment;
	if( !fromStatusScreen ) {
		if( character.gear[G_MONEY] > 0 ) {
			payment = G_MONEY;
		} else {
			payment = randomGearToLose( );
		}

		do {
			item = rand( ) % NUM_GEAR;
		} while( item == payment );

		++character.gear[item];
		--character.gear[payment];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Wish him luck and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You spend the better part of an hour going over the merchants wares and haggling. "
				"Finally you're able to reach an agreement. It isn't as much as you wanted, but you "
				"still feel like you got a good deal. Goods exchange hands and the merchant "
				"seems pleased with himself.",
				FG_GREY );


			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, payment );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, item );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void traderFailureScene( void )
{
	static Gear payment;
	if( !fromStatusScreen ) {
		if( character.gear[G_MONEY] > 0 ) {
			payment = G_MONEY;
		} else {
			payment = randomGearToLose( );
		}
		--character.gear[payment];
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Curse him under your breath and continue your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You spend the better part of an hour going over the merchants wares and haggling. "
				"Finally you're able to reach an agreement. The merchant takes the payment and before "
				"handing over the agreed upon goods calls over one of the guards. They start having a "
				"conversation on the taxes The Coalition imposes on him. He makes it all sound very sad "
				"and unfair.\n\n"
				"While this is happening another group of guards are slowly herding you away from the "
				"cart. There are about 10 guards in total, and all of them are heavily armed.",
				FG_GREY );

			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, payment );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void traderScene( void )
{
	// can attempt to trade for some random equipment (takes one random thing from you and gives you a different random thing)
	// he'll prefer money if you have it, otherwise he'll give anyhing
	// check will be easier with money
	static Companions cmp;
	if( !fromStatusScreen ) {
		timeLeft += 2;
		cmp = getRandomCompanion( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	uint8_t difficulty = 14 - ( character.gear[G_MONEY] > 0 ? 2 : 0 );
	pushSkillBasedChoice( &sbChoices, "Trade for some goods. (Barter)", SKL_BARTER, difficulty, traderSuccessScene, traderCostlyScene, traderFailureScene );
	pushSimpleChoice( &sbChoices, "Wish him luck and continue on your journey.", gotoNextWildernessScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test barter success.", traderSuccessScene );
		pushSimpleChoice( &sbChoices, "Test barter costly.", traderCostlyScene );
		pushSimpleChoice( &sbChoices, "Test barter failure.", traderFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"%s suddenly calls out. Looking you see them pointing at a wagon some ways in the distance. You "
				"pick up your pace and are able to get there within a few minutes. It seems to be a merchant cart "
				"with a broken wheel. Some of the guards are hoisting the thing up and gathering materials for "
				"a new wheel. The rest of the guards are watching you closely as you approach.\n\n"
				"You introduce yourself and on seeing you and %s are official Coalition members everybody relaxes. "
				"The merchant in charge comes over to meet you. He's a rotund chap in fancy clothes.\n\n"
				"What do you do?",
				FG_GREY, companionsData[cmp].name, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void chasmScene( void )
{
	// choose to try to cross it or go around it
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"THIS HAS NOT BEEN IMPLEMENTED YET.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void abandonedHouseScene( void )
{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"THIS HAS NOT BEEN IMPLEMENTED YET.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void caveScene( void )

{
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"THIS HAS NOT BEEN IMPLEMENTED YET.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfGoblinElfFriendScene( void )
{
	// fuck that goblin
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_GOBLIN] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn towards %s and immediately start going off on him. Telling him he "
				"should know his place. The goblin walks up to you snarling. Standing up straight, "
				"he gets as close as his bulbous gut allows. He stares you straight in the eyes for "
				"a minute before backing down. He yells some things in a language you don't understand "
				"as he packs his gear.\n\n"
				"Once he's finished he makes a rude gesture at all of you and leaves.",
				FG_GREY, companionsData[CMP_GOBLIN].name );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, companionsData[CMP_GOBLIN].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfGoblinGoblinFriendScene( void )
{
	// fuck that elf
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ELF] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn towards %s and eyeing him warely ask why the phrase murderer caused "
				"him to attack.\n\n"
				"The elf looks at the ground as he tells you why he was living in the woods alone "
				"for so long. He had committed the only crime among his kind that would warrant "
				"such a punishment, the murder of another elf. Pulling aside his hair you see "
				"a small tattoo under his left ear. He says this is to mark him so all others "
				"will know what he's done and that the goblin had seen it. After saying all this "
				"he walks away into the woods.\n\n"
				"You never see him again.",
				FG_GREY, companionsData[CMP_ELF].name );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfGoblinSuccessScene( void )
{
	// gotta get along
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Standing between them you back up a bit and ask what's going on.\n\n"
				"%s says that he saw the tattoo on %s's neck. The unforgivable tattoo. "
				"He seems very proud of knowing what it is.\n\n"
				"You turn to the elf and he sighs before pulling aside his hair and revealing "
				"a small tattoo under his left ear. He says that it's used to mark elvish "
				"outlaws. %s interjects and says the only crime that she knows that could "
				"warrant that is murder.\n\n"
				"Closing your eyes for a bit you spend a moment thinking. Looking at everybody "
				"you say that while he may be a criminal he's never hurt any of us, and you'd "
				"rather have his help than not. Looking at %s you ask if this is acceptable. "
				"He shrugs.\n\n"
				"Later %s thanks you and tells you that your trust is not misplaced.",
				FG_GREY, companionsData[CMP_GOBLIN].name, companionsData[CMP_ELF].name,
				companionsData[CMP_SCHOLAR].name, companionsData[CMP_GOBLIN].name,
				companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfGoblinCostlyScene( void )
{
	// both stay, but you gain a wound
	if( !fromStatusScreen ) {
		gainMentalWound( );
	}
	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Standing between them you back up a bit and ask what's going on.\n\n"
				"%s says that %s just started insulting him and he has no idea why. "
				"Looking at %s they just smile and say they did, and they did it "
				"because it's fun. Eventually you're able to get them both to calm "
				"down and agree to not fight.\n\n"
				"Later %s approaches you, a look of concern on her face. She says that "
				"%s only became violent when he was called a murderer. Your mind starts "
				"racing but you calm yourself. You tell her that if he had wanted to "
				"hurt any of you he could have by now. The next few nights sleeps are "
				"not as restful as you'd like.",
				FG_GREY, companionsData[CMP_ELF].name, companionsData[CMP_GOBLIN].name,
				companionsData[CMP_GOBLIN].name, companionsData[CMP_SCHOLAR].name,
				companionsData[CMP_ELF].name );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "didn't address something important" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfGoblinFailureScene( void )
{
	// only one will stay, choose one of the lose scenes randomly
	currentScene = ( ( rand( ) % 2 ) == 0 ) ? elfGoblinGoblinFriendScene : elfGoblinElfFriendScene;
}

void elfGoblinConflictScene( void )
{
	if( !character.hasCompanion[CMP_GOBLIN] || !character.hasCompanion[CMP_ELF] ) {
		currentScene = wanderScene;
		return;
	}

	// elf and goblin are calling each other names, finally the goblin calls the elf a murderer which enrages him
	//  have to use empathy to get them to calm down
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Confront the elf.", elfGoblinGoblinFriendScene );
	pushSimpleChoice( &sbChoices, "Confront the goblin.", elfGoblinElfFriendScene );
	pushSkillBasedChoice( &sbChoices, "Try to calm them both. (Empathy)", SKL_EMPATHY, 16, elfGoblinSuccessScene, elfGoblinCostlyScene, elfGoblinFailureScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test success.", elfGoblinSuccessScene );
		pushSimpleChoice( &sbChoices, "Test costly.",  elfGoblinCostlyScene );
		pushSimpleChoice( &sbChoices, "Test failure.", elfGoblinFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"As your taking a break and drinking from a stream with %s you hear a commotion nearby. "
				"It sounds like two people are yelling at each other.\n\n"
				"Heading over you see %s and %s both yelling insults at each other at the top of their "
				"lungs. Phrases like \"knife-ears\", \"patchskin\", \"tree-humper\", and \"glutton\" are being thrown out. Finally %s "
				"looks at the elf and calls him a murderer. This sets %s off and he draws a knife and charges. "
				"Before any harm can be done you step between them.\n\n"
				"What do you do?",
				FG_GREY, companionsData[CMP_SCHOLAR].name, companionsData[CMP_ELF].name, companionsData[CMP_GOBLIN].name,
				companionsData[CMP_GOBLIN].name, companionsData[CMP_ELF].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarEmpathySuccessScene( void )
{
	// scholar is convinced
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn to %s and stare at her for a minute. She doesn't make eye contact. "
				"Walking over and kneeling down beside her causes her to tense up a bit. You "
				"smile and put your hand on her shoulder. After talking for a bit, you're able "
				"to convince her stop. You also get her to apologize to %s and promise that "
				"she won't do it again. He reluctantly agrees but moves his bedroll next to "
				"yours.",
				FG_GREY, scholar, elf );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarEmpathyCostlyScene( void )
{
	// scholar is convinced, but you have to keep an eye on her, gaining a mental wound
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	if( !fromStatusScreen ) {
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn to %s and stare at her for a minute. She doesn't make eye contact. "
				"Walking over and kneeling down beside her causes her to tense up a bit. You "
				"smile and put your hand on her shoulder. After talking for a bit, she says "
				"she'll stop. Later you catch her watching %s from far away. You'll have to keep "
				"an eye on her.",
				FG_GREY, scholar, elf );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "are having to keep an eye on Freda" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarEmpathyFailureScene( void )
{
	// scholar is unconvinced, elf leaves
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ELF] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk over to %s. She shrinks into herself as you get close "
				"and isn't able to make eye contact. You lead her back towards "
				"camp where you both eat and talk a bit. Eventually she's out of "
				"shell and laughing along with you.\n\n"
				"The next day you find a note from %s. It says that he's leaving "
				"and %s can keep his underwear for all he cares.",
				FG_GREY, scholar, elf, scholar );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, elf );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarLieSuccessScene( void )
{
	// elf believes you
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn to %s and smile at him. Getting his attention off of %s you "
				"start telling him of strange human customs, some of which exist and "
				"some of which don't. You conclude by telling him that this sort of "
				"behavior is normal for humans. %s asks why she doesn't watch you bathe "
				"then and you reply that she does.\n\n"
				"You're able to convince the elf that this isn't creepy. You're even "
				"able to convince yourself enough that you don't feel much guilt about "
				"it.",
				FG_GREY, elf, scholar, elf );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarLieCostlyScene( void )
{
	// elf believs you but you gain a social wound from guilt
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	if( !fromStatusScreen ) {
		gainSocialWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn to %s and smile at him. Getting his attention off of %s you "
				"start telling him of strange human customs, some of which exist and "
				"some of which don't. You conclude by telling him that this sort of "
				"behavior is normal for humans. %s asks why she doesn't watch you bathe "
				"then and you reply that she does.\n\n"
				"You're able to convince the elf that this isn't creepy. You tell yourself "
				"this is for the good of the mission.",
				FG_GREY, elf, scholar, elf );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "feel guilty about lying" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarLieFailureScene( void )
{
	// elf doesn't believe you and leaves
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_ELF] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You turn to %s and smile at him. Getting his attention off of %s you "
				"start telling him of strange human customs, some of which exist and "
				"some of which don't. You conclude by telling him that this sort of "
				"behavior is normal for humans. %s asks why she doesn't watch you bathe. "
				"You stutter for a bit trying to think of a good reason why. At your reluctance "
				"to offer a response %s turns and walk away. You never see him again.",
				FG_GREY, elf, scholar, elf, elf );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, elf );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void elfScholarConflictScene( void )
{
	if( !character.hasCompanion[CMP_SCHOLAR] || !character.hasCompanion[CMP_ELF] ) {
		currentScene = wanderScene;
		return;
	}

	// scholar is horny for elf, elf doesn't like it
	//  can use empathy to calm the scholar down, or lie to make the elf believe this is normal
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}
	char* elf = companionsData[CMP_ELF].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;

	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Convince Freda to stop. (Empathy)", SKL_EMPATHY, 14, elfScholarEmpathySuccessScene, elfScholarEmpathyCostlyScene, elfScholarEmpathyFailureScene );
	pushSkillBasedChoice( &sbChoices, "Convince Noniac it's normal. (Lie)", SKL_LIE, 16, elfScholarLieSuccessScene, elfScholarLieCostlyScene, elfScholarLieFailureScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test empathy success.", elfScholarEmpathySuccessScene );
		pushSimpleChoice( &sbChoices, "Test empathy costly.",  elfScholarEmpathyCostlyScene );
		pushSimpleChoice( &sbChoices, "Test empathy failure.", elfScholarEmpathyFailureScene );

		pushSimpleChoice( &sbChoices, "Test lie success.", elfScholarLieSuccessScene );
		pushSimpleChoice( &sbChoices, "Test lie costly.",  elfScholarLieCostlyScene );
		pushSimpleChoice( &sbChoices, "Test lie failure.", elfScholarLieFailureScene );
	}
	
	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Taking a short break you hear some yelling from the river nearby. You head over to "
				"see if anyone's hurt.\n\n"
				"Arriving there you see %s wet and getting dressed. %s is sitting nearby looking "
				"sheepish. %s marches up to and says if you don't stop her that "
				"he's going to leave. Asking %s what happened he says she's been stalking his every "
				"move and had just now caught her watching him bathe.\n\n"
				"What do you do?",
				FG_GREY, elf, scholar, elf, elf );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarKnowledgeSuccessScene( void )
{
	// make peace
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You've studied goblin cultures before, and there's a few consistencies between "
				"all of them. You turn to %s and say he means that as a sign of respect. She doesn't "
				"seem to believe you. Going on you talk about how goblins practice funerary cannablism, "
				"believing it passes on the strength of those eaten. She gives %s a horrified look. He "
				"just nods solemnly and says her brain is strong and should be saved. %s sits staring "
				"into the fire the rest of the night.",
				FG_GREY, scholar, goblin, scholar );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarKnowledgeCostlyScene( void )
{
	// gain a mental wound from the goblin's description
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
		gainMentalWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You remember reading some things about goblin culture before. One thing you remember "
				"is that they eat their dead and you relay this information to them both. %s responds "
				"with a solemn nod and says it passes the strength of the previous generation to the "
				"next, and only weaklings or cowards are buried. He then spends the next hour talking "
				"about his Rite of Ascension, including the feast that followed. Both you and %s listen "
				"on in shock as he's describing how his grandfather tasted.",
				FG_GREY, goblin, scholar );

			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Mental", "learned too much about goblin customs" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarKnowledgeFailureScene( void )
{
	// you have no clue and tell the goblin to leave
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_GOBLIN] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You wrack your brain but can't think of any reason he'd want to do "
				"that. Everyone knows that cannabilism is wrong. You've seen the reports "
				"and know the mental unstability that can follow. While standing next to %s you tell %s "
				"that he has to leave. He looks at you, a confused look on his face. "
				"You repeat yourself with a firm voice while looking him in the eyes.\n\n"
				"After he's packed up he comes to say goodbye but you and %s ignore him.",
				FG_GREY, scholar, goblin, scholar );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, goblin );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarIntimidateSuccessScene( void )
{
	// goblin will let you eat her instead
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {

	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You stare down %s and tell him that he can't eat %s. Standing up he gets close "
				"stares right back at you. This continues for a few minutes with %s trying to "
				"get you to flinch. You remain stoic the entire time.\n\n"
				"Finally %s smiles and laughs, saying he won't eat %s. That he'll let you eat "
				"her instead. He then gets up and walks away. You and %s exchange confused "
				"looks.",
				FG_GREY, goblin, scholar, goblin, goblin, scholar, scholar );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarIntimidateCostlyScene( void )
{
	// gain a physical wound but the goblin will let you eat her instead
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
		gainPhysicalWound( "Punched by a goblin." );
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You stare down %s and tell him that he can't eat %s. Standing up he gets close "
				"stares right back at you. He seems to make a move to attack you so dodge to the "
				"side and punch him in the gut. %s flinches but relaliates with a fist in your "
				"face. After the punch he makes no move to attack again, but continues staring. "
				"Wiping the blood off your face you stare right back and don't fall for any more "
				"of his threats.\n\n"
				"Finally %s smiles and laughs, saying he won't eat %s. That he'll let you eat "
				"her instead. He then gets up and walks away. You and %s exchange confused "
				"looks.",
				FG_GREY, goblin, scholar, goblin, goblin, scholar, scholar );

			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were punched in the face" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarIntimidateFailureScene( void )
{
	// you chicken out, the goblin leaves
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_GOBLIN] = false;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You immediately go up to %s and start yelling at him. Calling "
				"him a monster and saying if he touches %s that you will personally "
				"kill him. You yell and snarl and are quite ferocious.\n\n"
				"%s gets up and leaves, you never see him again.",
				FG_GREY, goblin, scholar, goblin );

			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost %s as a companion...", FG_RED, goblin );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void goblinScholarConflictScene( void )
{
	if( !character.hasCompanion[CMP_GOBLIN] || !character.hasCompanion[CMP_SCHOLAR] ) {
		currentScene = wanderScene;
		return;
	}

	// goblin admires scholar, but expresses this in a very strange way
	//  knowledge to know about goblins
	//  intimidate to convince him to stop
	char* goblin = companionsData[CMP_GOBLIN].name;
	char* scholar = companionsData[CMP_SCHOLAR].name;
	if( !fromStatusScreen ) {
		timeLeft += 2;
	}

	startScene( );

	Choice* sbChoices = NULL;

	pushSkillBasedChoice( &sbChoices, "Try to remember about goblin culture. (Knowledge)", SKL_KNOWLEDGE, 14,
		goblinScholarKnowledgeSuccessScene, goblinScholarKnowledgeCostlyScene, goblinScholarKnowledgeFailureScene );
	pushSkillBasedChoice( &sbChoices, "Let him know you're the one in charge. (Intimidate)", SKL_INTIMIDATE, 12,
		goblinScholarKnowledgeSuccessScene, goblinScholarKnowledgeCostlyScene, goblinScholarKnowledgeFailureScene );

	if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test knowledge success.", goblinScholarKnowledgeSuccessScene );
		pushSimpleChoice( &sbChoices, "Test knowledge costly.", goblinScholarKnowledgeCostlyScene );
		pushSimpleChoice( &sbChoices, "Test knowledge failure.", goblinScholarKnowledgeFailureScene );

		pushSimpleChoice( &sbChoices, "Test intimidate success.", goblinScholarIntimidateSuccessScene );
		pushSimpleChoice( &sbChoices, "Test intimidate costly." , goblinScholarIntimidateCostlyScene );
		pushSimpleChoice( &sbChoices, "Test intimidate failure.", goblinScholarIntimidateFailureScene );
	}

	COORD pos;

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"While washing the dishes you hear %s cry out. Hurrying over to the fire you see her "
				"and %s sitting there. She has a look of diguist on her face while he seems confused.\n\n"
				"You ask what's going on. While slowly backing away %s says that %s said he was going to eat her. "
				"%s nods and responds that yes, he is going to. %s responds that he needs to leave, that "
				"we can't trust him.\n\n"
				"What do you do?",
				FG_GREY, scholar, goblin, scholar, goblin, goblin, scholar );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void huntWithElfScene( void )
{
	static bool hunt;
	static bool which;
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;

	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You and %s head out. While you wait patiently he starts singing. Soon a small %s "
					"comes out of the woods and you'r able to catch it. You skin and cook it up before "
					"returning to the path you were following.",
					FG_GREY,
					companionsData[CMP_ELF].name,
					which ? "squirrel" : "rabbit" );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"You and %s head out. You stumble on a strange plant that you don't recognize "
					"but %s talks to it for a bit before informing you it's edible. You stick some in your "
					"pack before returning to the path you were following.",
					FG_GREY,
					companionsData[CMP_ELF].name,
					companionsData[CMP_ELF].name );
			}
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void huntSuccessScene( void )
{
	static bool hunt;
	static bool which;
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to trap a %s. You skin and cook it up before "
						"returning to the path you were following.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to find some edible %s. You stick some in your "
						"pack before returning to the path you were following.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void huntCostlySuccessScene( void )
{
	static bool hunt;
	static bool which;
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
		++timeLeft;
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to trap a %s. You skin and cook it up before "
						"attempting to find the path you were following. It takes you a while and you figure "
						"you've lost about half a day.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"After a short search you're able to find some edible %s. You stick some in your "
						"pack before attempting to find the path you were following. It takes you a while "
						"and you figure you've lost about half a day.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost some time.", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void huntFailureScene( void )
{
	static bool hunt;
	static bool which;
	if( !fromStatusScreen ) {
		hunt = ( ( rand( ) % 2 ) == 0 );
		which = ( ( rand( ) % 2 ) == 0 );
		++timeLeft;
	}

	startScene( );
	
	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );

	COORD pos;
	
	do {
		startPlayDraw( ); {
			if( hunt ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You spend about half the day hunting down a %s you saw. It evades you every time "
						"you get close to it. You eventually give up and find the path you had been following "
						"before.",
						FG_GREY,
						which ? "squirrel" : "rabbit");
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
						"You find some %s that look delicious. After trying a little bit of them to make sure "
						"they're safe you spend about half the day vomiting. Once you recover you go find "
						"the path you had been following before.",
						FG_GREY,
						which ? "berries" : "roots");
			}
			pos = drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've lost some time.", FG_RED );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void wanderScene( void )
{
	startScene( );

	static boolean ateWanderFood;
	if( !fromStatusScreen ) {
		if( character.gear[G_RATION] > 0 ) {
			--character.gear[G_RATION];
			ateWanderFood = true;
		} else {
			gainArmorIgnoringWound( "Starved to Death" );
			ateWanderFood = false;
		}
	}

	COORD pos;

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Continue on your journey.", gotoNextWildernessScene );
	pushSkillBasedChoice( &sbChoices, "Forage and hunt for food while traveling. (Survival)", SKL_SURVIVAL, 10,
		huntSuccessScene, huntCostlySuccessScene, huntFailureScene );
	if( character.hasCompanion[CMP_ELF] ) {
		pushSimpleChoice( &sbChoices, "Go out with Noniac to hunt for food while traveling.", huntWithElfScene );
	}

	while( nextScene == NULL ) {
		startPlayDraw( ); {
			if( ateWanderFood ) {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"%s spend the day traveling through the wilderness. You snack on a ration as you go.\n\n"
					"What do you want to do?",
					FG_GREY, getEveryoneString( true ) );
				standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			} else {
				pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
					"%s spend the day traveling through the wilderness. Your stomach complains all the way.\n\n"
					"What do you want to do?",
					FG_GREY, getEveryoneString( true ) );
				standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "go hungry" );
			}

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	}

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

// ******************************************************************************************************************************
// Town scenes
void leaveTownLateScene( void )
{
	if( storedScene != leaveTownLateScene ) {
		setupWilderness( );
	}

	// they make no progress towards the goal, but hopefully they got something from their encounter
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Go into the wilderness.", gotoNextWildernessScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You and %s make your way to edge of the settlement. The sun is almost setting so you set "
				"up a small camp site near the edge of the woods. The following day "
				"you both head off into wilderness.",
				FG_GREY, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void leaveTownScene( void )
{
	if( storedScene != leaveTownScene ) {
		setupWilderness( );
		timeLeft -= 1;
	}

	// they make half a days worth of progress towards the goal
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Go into the wilderness.", gotoNextWildernessScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You and %s make your way to edge of the settlement. There's plenty of daylight left "
				"so you both head off into wilderness.",
				FG_GREY, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernScout( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		++character.gear[G_RATION];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave town.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Volos greets you warmly, setting down the washing he's currently doing. After some small "
				"talk you get to business.\n\n"
				"You tell him that you need some extra supplies, that you're going on a journey and are "
				"unsure how long it will take. He seems hesitant until you remind him that his cattle herd "
				"would be half what is now if you hadn't helped take care of the wolves last summer.\n\n"
				"He concedes with a smile and gets one of the helper boys to grab you some extra rations while"
				"you spend some time chatting. You wave goodbye as you head out the door.",
				FG_GREY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernMoney( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		++character.gear[G_RATION];
		--character.gear[G_MONEY];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave town.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Volos greets you warmly, setting down the washing he's currently doing. After some small "
				"talk you get to business.\n\n"
				"You tell him that you need some extra supplies, that you're going on a journey and are "
				"unsure how long it will take. He seems hesitant until you take out some cash and hand it to "
				"him.\n\n"
				"He gets one of the helper boys to grab you some extra rations while"
				"you spend some time chatting. You wave goodbye as you head out the door.",
				FG_GREY );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernEmpathySuccess( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		++character.gear[G_RATION];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave town.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Volos greets you warmly, setting down the washing he's currently doing. After some small "
				"talk you get to business.\n\n"
				"You tell him that you need some extra supplies, that you're going on a journey and are "
				"unsure how long it will take. He seems hesitant but with both you and %s trying you're "
				"able to eventually convince him to donate some supplies.\n\n"
				"He gets one of the helper boys to grab you some extra rations while"
				"you spend some time chatting. You wave goodbye as you head out the door.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernEmpathyCostly( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave town.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Volos greets you warmly, setting down the washing he's currently doing. After some small "
				"talk you get to business.\n\n"
				"You tell him that you need some extra supplies, that you're going on a journey and are "
				"unsure how long it will take. He seems hesitant but you're "
				"able to eventually convince him to donate some supplies.\n\n"
				"He gets one of the helper boys to grab you some extra rations while"
				"you spend some time chatting. You wave goodbye as you head out the door.",
				FG_GREY );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernEmpathyFailure( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_RATION];
		gainSocialWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave town.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Volos greets you warmly, setting down the washing he's currently doing. After some small "
				"talk you get to business.\n\n"
				"You tell him that you need some extra supplies, that you're going on a journey and are "
				"unsure how long it will take. He seems hesitant. After spending hours begging him and any "
				"of the other patrons for help one of them takes pity on you buys a days of rations for you.\n\n"
				"As your leaving you hear people laughing and mocking you.",
				FG_GREY );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "were forced to beg" );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_RATION );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void tavernScene( void )
{
	// have a chance to get some rations
	startScene( );

	Choice* sbChoices = NULL;
	if( isCharacterClass( CC_SCOUT ) ) {
		pushSimpleChoice( &sbChoices, "The old man owes you for last summer. (Scout)", tavernScout );
	} else {
		if( character.gear[G_MONEY] > 0 ) {
			pushSimpleChoice( &sbChoices, "Buy some extra rations. (Money)", tavernMoney );
		}
		pushSkillBasedChoice( &sbChoices, "Try to convince Volos to donate some rations. (Empathy)", SKL_EMPATHY, 12,
			tavernEmpathySuccess, tavernEmpathyCostly, tavernEmpathyFailure );
	}
	/**/if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test beg success.", tavernEmpathySuccess );
		pushSimpleChoice( &sbChoices, "Test beg costly.", tavernEmpathyCostly );
		pushSimpleChoice( &sbChoices, "Test bet fail.", tavernEmpathyFailure );
	}//*/

	pushSimpleChoice( &sbChoices, "Change your mind and leave.", leaveTownLateScene );

	do {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You and %s head over to The Wet Mug. You've never liked that name, too on the nose "
				"for you. But they serve good beer and good food so you're willing to overlook it.\n\n"
				"Heading inside some of the regulars greet both of you before turning back to their "
				"drinks. Waving off the waitress you go right to the bar to talk to Volos, the owner "
				"of this establishment.\n\n"
				"What do you do?",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void libraryScholar( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_MAP];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave the library.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You know this place like the back of your hand. Quickly you get past the various stacks "
				"of dictionaries and old literature to the map section. The local maps are stored in the "
				"back, below the section devoted to maps of the greater Coalition owned territory.\n\n"
				"Without much trouble you find something you think will help and make a quick copy of it.",
				FG_GREY );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MAP );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void librarySearchSuccess( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_MAP];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave the library.", leaveTownScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Nothing seems to be in order in this place, but The Red Marquis is on your side and "
				"you're able to spot where the maps are. You find one that looks like it'll be helpful "
				"and make a copy before leaving.",
				FG_GREY );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MAP );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void librarySearchCostly( void )
{
	if( !fromStatusScreen ) {
		++character.gear[G_MAP];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave the library.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Nothings seems to be in order in this place, but after a long search, and many curses "
				"directed at The Red Marquis, you find the maps. After a thorough search you find something "
				"that looks like it should be helpful. You make a copy and leave as the sun hangs low in the sky.",
				FG_GREY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void librarySearchFailure( void )
{
	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave the library.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"Nothings seems to be in order in this place. You offer a small prayer to The Red "
				"Marquis, hopeful he is in a merciful mood. After spending most of the day you've "
				"decided He is not and leave empty handed.",
				FG_GREY );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MAP );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void researchLibraryScene( void )
{
	// chance to get a map
	startScene( );

	Choice* sbChoices = NULL;
	if( isCharacterClass( CC_SCHOLAR ) ) {
		pushSimpleChoice( &sbChoices, "You know where the good maps are. (Scholar)", libraryScholar );
	} else {
		pushSkillBasedChoice( &sbChoices, "Try to find something that can be used. (Investigate)", SKL_INVESTIGATE, 8,
			librarySearchSuccess, librarySearchCostly, librarySearchFailure );
	}

	/*if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test search success.", librarySearchSuccess );
		pushSimpleChoice( &sbChoices, "Test search costly.", librarySearchCostly );
		pushSimpleChoice( &sbChoices, "Test search fail.", librarySearchFailure );
	}//*/

	pushSimpleChoice( &sbChoices, "Change your mind and leave town.", leaveTownScene );

	do {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You and %s walk for a bit and get to the library connected to the research facility. After showing some "
				"identification you're both let in. It's cool and dry inside, one of the few environmentally controlled buildings "
				"in the settlement.\n\n"
				"%s\n\n"
				"What do you do?",
				FG_GREY, companionsData[CMP_SCHOLAR].name,
				false && isCharacterClass( CC_SCHOLAR ) ?
					"You know this place well and should have no trouble finding what you want." : 
					"You ask your companion if she knows of any good places to start. She shrugs and says she spends most of her time in the tech labs. Only rarely "
					"having reason to come to the library" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksSoldierScene( void )
{
	Gear gearGiven = NUM_GEAR;
	// give the player a gun they don't have, if they have everything then give them another rifle
	if( !fromStatusScreen ) {
		// prefer this order: rifle, pistol, axe, sword
		if( character.gear[G_RIFLE] == 0 ) {
			gearGiven = G_RIFLE;
		} else if( character.gear[G_PISTOL] == 0 ) {
			gearGiven = G_PISTOL;
		} else if( character.gear[G_HANDAXE] == 0 ) {
			gearGiven = G_HANDAXE;
		} else if( character.gear[G_SWORD] == 0 ) {
			gearGiven = G_SWORD;
		} else {
			gearGiven = G_RIFLE;
		}
		++character.gear[gearGiven];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Thank Seija and leave.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to Seija and smile. She rolls her eyes and asks what do you want now. You mention that she still owes quite a bit "
				"of money, and you'd be willing to forgive the debt if you could borrow some equipment for your current mission. After a bit of "
				"banter where she may or may not have implied she wouldn't have to pay it off if you died either she heads into the back. A short "
				"while later she comes out with a %s and hands it to you.",
				FG_GREY, gearData[gearGiven].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gearGiven );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksLieSuccessScene( void )
{
	// the nitwit believes you, get rifle
	Gear gearGiven = G_RIFLE;
	if( !fromStatusScreen ) {
		++character.gear[gearGiven];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Thank the soldier and leave.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You gesture towards %s and lean to whisper to the soldier, saying that the woman there is very close to the "
				"base commander, and since you're both going out together you were wondering if there was anything extra you "
				"could get to help keep her safe. The soldier looks at your companion and mutters about her looking like his "
				"type. She heaves herself up from the desk and heads in back.\n\n"
				"Shortly after she comes back with a %s and hands it to you after having you fill out some paperwork.",
				FG_GREY, companionsData[CMP_SCHOLAR].name, gearData[gearGiven].name );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gearGiven );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksLieCostlySuccessScene( void )
{
	// almost didn't get away with it, get pistol and embarressed
	Gear gearGiven = G_PISTOL;
	if( !fromStatusScreen ) {
		++character.gear[gearGiven];
		gainSocialWound( );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave before anybody sees you.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You lean to whisper to the soldier, saying that you are very close to the "
				"base commander, and since you're heading out on a dangerous mission if there was anything she could give you "
				"to help keep you safe and the commander happy. The soldier looks at you and mutters about you looking like his "
				"type. She heaves herself up from the desk and heads in back.\n\n"
				"Shortly after she comes back with a %s and hands it to you after having you fill out some paperwork.\n\n"
				"As you're leaving the soldier calls after you, asking you to pass on her name next time you and the commander "
				"are having some pillow talk very loudly. %s turns to look at you with confusion. You quickly walk past her while "
				"keeping your head down.",
				FG_GREY, gearData[gearGiven].name, companionsData[CMP_SCHOLAR].name );
			pos = standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gearGiven );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "were embarrassed" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksLieFailureScene( void )
{
	// shit, get a physical and social wound
	if( !fromStatusScreen ) {
		gainSocialWound( );
		gainPhysicalWound( "Ruptured kidney." );
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave quickly.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You say that your on an important mission for the settlement and that you demand better equipment for your task.\n\n"
				"She stares at you for a bit and finally says that anything you were given is all that they could spare at the moment "
				"and if you want more gear you'll have to wait behind all the other vitally important missions going on. You're having "
				"none of this and keep arguing with her. After about 10 minutes of this she gets up and yells something through the door "
				"behind her. Shortly afterwards a tall and large man wearing a guard uniform comes out. The soldier motions towards you "
				"and the beef beast charges you, punching you in the stomach and causing you to double over before you can react.\n\n"
				"You're unceremoniously thrown out the door, landing in front of a group of soldiers that had gathered to see what "
				"was going on. Their laughter rings in your ears. %s hides her face as she picks you up and dusts you off.",
				FG_GREY, companionsData[CMP_SCHOLAR].name );
			pos = standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Physical", "were punched" );
			standardWoundText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, "Social", "were embarrassed" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksBarterSuccessScene( void )
{
	Gear gearGiven = G_RIFLE;
	if( !fromStatusScreen ) {
		++character.gear[gearGiven];
		--character.gear[G_MONEY];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Thank the soldier and leave.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You mention that you're on an important mission and are looking for some good weapons. She almost dismisses you "
				"before you slide a wad of cash onto her desk. She pockets the cash and heads in back. Shortly afterwards she returns "
				"and hands you a %s. She then sits and continues with her paperwork.",
				FG_GREY, gearData[gearGiven].name );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gearGiven );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	checkTooMuchGear( nextScene );
}

void barracksBarterCostlySuccessScene( void )
{
	Gear gearGiven = G_PISTOL;
	if( !fromStatusScreen ) {
		++character.gear[gearGiven];
		--character.gear[G_MONEY];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Thank the soldier and leave.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You mention that you're on an important mission and are looking for some good weapons. She almost dismisses you "
				"before you slide a wad of cash onto her desk. She pockets the cash and heads in back. Shortly afterwards she returns "
				"and hands you a %s. You had expected a bit more than just this. She then sits and continues with her paperwork.",
				FG_GREY, gearData[gearGiven].name );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );
			standardGearGainText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, gearGiven );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );

	checkTooMuchGear( nextScene );
}

void barracksBarterFailureScene( void )
{
	if( !fromStatusScreen ) {
		--character.gear[G_MONEY];
	}

	startScene( );

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Accept your loss and leave.", leaveTownLateScene );
	COORD pos;

	do {
		startPlayDraw( ); {
			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You walk up to the soldier and smile. She looks up from her paperwork and sighs before asking what you want. "
				"You mention that you're on an important mission and are looking for some good weapons. She almost dismisses you "
				"before you slide a wad of cash onto her desk. She pockets the cash and heads in back. Shortly afterwards she returns "
				"with a mountain of a man in guard uniform. They're idly chatting about the punishments for bribing military personal. "
				"You get the hint and turn around.",
				FG_GREY );
			pos = standardGearLossText( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea, G_MONEY );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void barracksScene( void )
{
	// chance to get a weapon
	startScene( );

	Choice* sbChoices = NULL;
	if( isCharacterClass( CC_SOLDIER ) ) {
		pushSimpleChoice( &sbChoices, "Seija owes you money from last weeks poker game. (Soldier)", barracksSoldierScene );
	} else {
		pushSkillBasedChoice( &sbChoices, "Try to convince the soldier to give you some equipment. (Lie)", SKL_LIE, 10,
			barracksLieSuccessScene, barracksLieCostlySuccessScene, barracksLieFailureScene );
		if( character.gear[G_MONEY] > 0 ) {
			pushSkillBasedChoice( &sbChoices, "Offer some money for equipment. (Barter)", SKL_BARTER, 8,
				barracksBarterSuccessScene, barracksBarterCostlySuccessScene, barracksBarterFailureScene );
		}
	}

	/*if( isCharacterClass( CC_DEV_TEST ) ) {
		pushSimpleChoice( &sbChoices, "Test lie success.", barracksLieSuccessScene );
		pushSimpleChoice( &sbChoices, "Test lie costly.", barracksLieCostlySuccessScene );
		pushSimpleChoice( &sbChoices, "Test lie fail.", barracksLieFailureScene );

		pushSimpleChoice( &sbChoices, "Test barter success.", barracksBarterSuccessScene );
		pushSimpleChoice( &sbChoices, "Test barter costly.", barracksBarterCostlySuccessScene );
		pushSimpleChoice( &sbChoices, "Test barter fail.", barracksBarterFailureScene );
	}//*/

	pushSimpleChoice( &sbChoices, "Change your mind and leave town.", leaveTownScene );

	do {
		startPlayDraw( ); {
			drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You and %s walk for a bit and get to the small military base on the outskirts of the settlement. After showing some "
				"identification you're allowed inside. The both of you quickly make your way to the armory. Inside you see %s sitting "
				"behind a desk, filling out some paper work.\n\nWhat do you do?",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name, isCharacterClass( CC_SOLDIER ) ? "Seija" : "a soldier" );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

void meetFredaScene( void )
{
	startScene( );

	if( !fromStatusScreen ) {
		character.hasCompanion[CMP_SCHOLAR] = true;
	}

	Choice* sbChoices = NULL;
	pushSimpleChoice( &sbChoices, "Leave right now.", leaveTownScene );
	pushSimpleChoice( &sbChoices, "See if there's anything useful at the armory.", barracksScene );
	pushSimpleChoice( &sbChoices, "Visit the tavern.", tavernScene );
	pushSimpleChoice( &sbChoices, "Do some last minute research at the library.", researchLibraryScene );

	COORD pos;
	do {
		startPlayDraw( ); {

			pos = drawString( descriptionSafeArea.Left, descriptionSafeArea.Top, descriptionSafeArea,
				"You step out of your small house, all your equipment ready and mentally steeled for journey ahead.\n"
				"Outside you almost bump into a woman waiting in front of you door. She's wearing scholar robes and is carrying a large pack.\n\n"
				"After steadying herself she smiles, introducing herself as %s, the scholar you're "
				"to escort to the factory. As you shake her hand you feel the cold metal contacts embedded into her hands. The sign of being "
				"a tech-initiate and a devotee of The Hidden Emperor. Doesn't appear have any of the other more obvious implants though.\n\n"
				"After introductions are made and plans are gone over she looks at you expectantly, asking if you're leaving right away or "
				"if there's any last minute tasks you need to take care of. You'll only have time to go to one place before you have to leave.",
				FG_GREY,
				companionsData[CMP_SCHOLAR].name );
			drawString( descriptionSafeArea.Left, pos.Y + 2, descriptionSafeArea,
				"You've gained %s as a companion!", FG_GREEN, companionsData[CMP_SCHOLAR].name );

			standardSceneChoiceDraw( sbChoices, sb_count( sbChoices ) );
		} endDraw( );

		standardSceneChoice( sbChoices, sb_count( sbChoices ), true );
	} while( nextScene == NULL );

	sb_free( sbChoices );
	checkTooMuchGear( nextScene );
}

// Character creation scenes
void itemSelectionScene( void )
{
	topTitle = " Choose Your Gear ";

	// based on their barter skill they can get more equipment to select
	// only have a number of equipment slots equal to their Physical die size
	int baseGear = 4;
	int bonusGear = character.ss_barter;
	int totalGear = baseGear + bonusGear;

	int selectedItem = 0;

	int totalAllowed = min( totalGear, carryWeight( ) );

	boolean showError = false;
	boolean done = false;

	WORD normalAttr = FG_DARK_CYAN | BG_BLACK;
	WORD highlightAttr = FG_CYAN | BG_BROWN;
	WORD attr;

	
	SMALL_RECT gearColumn = { 5, 23, 25, 33 };
	SMALL_RECT descriptionArea = { 30, 0, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };
	descriptionArea.Top = gearColumn.Top + 3;

	do {
		startDraw( ); {
			drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea,
				"You spend the week before your departure checking and double checking everything. "
				"The leaders of the settlement are able to give you some supplies, anything beyond that you'll have to gather on your own."
				"%s\n\n"
				"For a base you'll get to choose %i pieces of gear, with a bonus of %i items due to your Bartering skill, "
				"giving you a total of %i pieces you can purchase.\n\n"
				"Due to your Physical attribute and Acrobatics you'll be able to carry %i pieces of gear total.\n\n"
				"Use the up and down arrow keys to select a piece of gear, press right to add one to your inventory and left "
				"to remove one.",
				FG_GREY, 
				character.ss_barter > 0 ? " After calling in some favors and selling off some of your more useless equipment you're able to scrounge together some more." : "",
				baseGear, bonusGear, totalGear, carryWeight( ) );

			drawString( descriptionArea.Left, gearColumn.Top, safeWriteArea, "Gear Allowed Left: %i", FG_GREEN | BG_BLACK, ( totalAllowed - totalGearCount( ) ) );

			if( showError ) {
				drawString( 30, 19, safeWriteArea,
					"Not enough allowed gear left.", FG_RED | BG_BLACK );
			}

			// display all the gear
			SHORT y = gearColumn.Top;
			for( int i = 0; i < NUM_GEAR; ++i ) {
				if( gearData[i].availableAtStore ) {
					attr = ( selectedItem == i ) ? highlightAttr : normalAttr;
					centerStringHoriz( gearColumn, y, "%s: %i", attr, gearData[i].name, character.gear[i] );
					++y;
				}
			}

			drawString( descriptionArea.Left, descriptionArea.Top, descriptionArea, gearData[selectedItem].description, FG_CYAN | BG_BLACK );

			centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to start your adventure",
					FG_YELLOW | BG_BLACK );
		} endDraw( );

		// process input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				showError = false;
				handled = true;
				do {
					--selectedItem;
					if( selectedItem < 0 ) {
						selectedItem = NUM_GEAR - 1;
					}
				} while( !gearData[selectedItem].availableAtStore );
				break;
			case IN_DOWN:
				showError = false;
				handled = true;
				do {
					selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
				} while( !gearData[selectedItem].availableAtStore );
				break;
			case IN_LEFT:
				showError = false;
				handled = true;
				removeGear( selectedItem );
				break;
			case IN_RIGHT:
				handled = true;
				if( ( totalAllowed - totalGearCount( ) ) > 0 ) {
					showError = false;
					addGear( selectedItem );
				} else {
					showError = true;
				}
				break;
			case IN_ENTER:
				handled = true;
				done = true;
				break;
			}
		} while( !handled );

		eatAllInputs( );
	} while( !done );

	currentScene = meetFredaScene;
}

void skillSelectionScene( void )
{
	topTitle = " Choose Your Skills ";
	boolean done = false;
	int selectedSkill = 0;

	WORD normalSkill = FG_DARK_CYAN | BG_BLACK;
	WORD highlightSkill = FG_CYAN | BG_BROWN;
	WORD attr;

	SMALL_RECT physicalColumn = { 9, 18, 29, 22 };
	SMALL_RECT mentalColumn = { 9, 25, 29, 29 };
	SMALL_RECT socialColumn = { 9, 32, 29, 36 };
	SMALL_RECT descriptionArea = { 28, 21, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 };

	boolean showError = false;

	uint8_t bonusPoints[NUM_SKILLS];
	memset( bonusPoints, 0, sizeof( bonusPoints ) );
	switch( character.class ) {
	case CC_SCHOLAR:
		bonusPoints[SKL_KNOWLEDGE] = 2;
		bonusPoints[SKL_INVESTIGATE] = 2;
		break;
	case CC_SCOUT:
		bonusPoints[SKL_SURVIVAL] = 2;
		bonusPoints[SKL_STEALTH] = 2;
		break;
	case CC_SOLDIER:
		bonusPoints[SKL_SHOOT] = 2;
		bonusPoints[SKL_ACROBATICS] = 2;
		break;
	}

	do {
		startDraw( ); {
			drawString( 2, 2, safeWriteArea,
				"Choose what skills you want to have learned. Use the up and down arrow keys to highlight a skill then use the right "
				"arrow key to increase it and the left arrow key to decrease it. "
				"Each level in a skill will cost 1 point up until the skill is at the same value as the skills "
				"matching attribute, after that it will cost 2 points. Bonus skill points from you class are displayed to the right of the skills"
				"value.\n\n"
				"During your adventure you will be presented with a situation and a number of ways to resolve the situation. How successful "
				"you are will be based on a skill check, which is calculated by rolling the die of the attribute associated with the skill "
				"and adding the skill level to it (e.g. an Acrobatics check would roll your Physical die and adding your Acrobatics skill "
				"along with any bonuses from equipment and subtracting penalties from wounds).",
				FG_GREY | BG_BLACK );

			// draw the attributes
			drawString( physicalColumn.Left, physicalColumn.Top, physicalColumn, "Physical: d%i", FG_GREEN | BG_BLACK, character.stat_physicalDie );
			drawString( mentalColumn.Left, mentalColumn.Top, mentalColumn, "Mental: d%i", FG_GREEN | BG_BLACK, character.stat_mentalDie );

			drawString( socialColumn.Left, socialColumn.Top, socialColumn, "Social: d%i", FG_GREEN | BG_BLACK, character.stat_socialDie );

			// draw the skill sheet
			//  physical
			SHORT y = physicalColumn.Top + 1;
			int i = 0;
			for( ; i < 4; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				char bonusAdj = bonusPoints[i] > 0 ? '+' : ' ';
				char bonusAmt = bonusPoints[i] > 0 ? ( '0' + bonusPoints[i] ) : ' ';
				drawString( physicalColumn.Left + 2, y, physicalColumn, "%s: %i%c%c", attr, skillsData[i].name, ( *( skillsData[i].value ) ), bonusAdj, bonusAmt );
				++y;
			}

			//  mental
			y = mentalColumn.Top + 1;
			for( ; i < 8; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				char bonusAdj = bonusPoints[i] > 0 ? '+' : ' ';
				char bonusAmt = bonusPoints[i] > 0 ? ( '0' + bonusPoints[i] ) : ' ';
				drawString( mentalColumn.Left + 2, y, mentalColumn, "%s: %i%c%c", attr, skillsData[i].name, ( *( skillsData[i].value ) ), bonusAdj, bonusAmt );
				++y;
			}

			//  social
			y = socialColumn.Top + 1;
			for( ; i < 12; ++i ) {
				attr = ( selectedSkill == i ) ? highlightSkill : normalSkill;
				char bonusAdj = bonusPoints[i] > 0 ? '+' : ' ';
				char bonusAmt = bonusPoints[i] > 0 ? ( '0' + bonusPoints[i] ) : ' ';
				drawString( socialColumn.Left + 2, y, socialColumn, "%s: %i%c%c", attr, skillsData[i].name, ( *( skillsData[i].value ) ), bonusAdj, bonusAmt );
				++y;
			}

			drawString( 28, 17, safeWriteArea, "Skill points left: %i", FG_GREEN | BG_BLACK, character.skillPointsLeft );

			if( showError ) {
				drawString( 28, 19, safeWriteArea,
					"Not enough points left.", FG_RED | BG_BLACK );
			}

			drawString( descriptionArea.Left, descriptionArea.Top, descriptionArea,
				skillsData[selectedSkill].description, FG_CYAN | BG_BLACK );

			if( character.skillPointsLeft <= 0 ) {
				centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to continue to equipment selection",
					FG_YELLOW | BG_BLACK );
			} else {
				centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
					"Press Enter to randomly distribute the points you have left",
					FG_GREEN | BG_BLACK );
			}

		} endDraw( );

		// handle input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				handled = true;
				--selectedSkill;
				if( selectedSkill < 0 ) {
					selectedSkill = ARRAYSIZE( skillsData ) - 1;
				}
				showError = false;
				break;
			case IN_DOWN:
				handled = true;
				selectedSkill = ( selectedSkill + 1 ) % ARRAYSIZE( skillsData );
				showError = false;
				break;
			case IN_LEFT:
				handled = true;
				if( (*skillsData[selectedSkill].value) > 0 ) {
					--(*skillsData[selectedSkill].value);
					uint8_t refund = ( ( (*skillsData[selectedSkill].value) ) >= (*skillsData[selectedSkill].attrValue ) ) ? 2 : 1;
					character.skillPointsLeft += refund;
					showError = false;
				}
				break;
			case IN_RIGHT: {
					handled = true;
					uint8_t cost = ( (*skillsData[selectedSkill].value) >= (*skillsData[selectedSkill].attrValue ) ) ? 2 : 1;
					if( character.skillPointsLeft < cost ) {
						showError = true;
					} else {
						character.skillPointsLeft -= cost;
						++(*skillsData[selectedSkill].value);
						showError = false;
					}
				} break;
			case IN_ENTER:
				handled = true;
				if( character.skillPointsLeft <= 0 ) {
					done = true;
				} else {
					randomlyDistributeSkills( );
				}
				break;
			}
		} while( !handled );
		eatAllInputs( );
	} while( !done );

	// apply the bonus points
	for( int i = 0; i < NUM_SKILLS; ++i ) {
		(*skillsData[i].value) += bonusPoints[i];
	}

	currentScene = itemSelectionScene;
}

#define NUM_HISTORY_EVENTS 3
#define CHANCE_NON_CLASS_EVENT_DENOM 5 // assuming numerator is 1, so it'll be 1/# chance
typedef enum {
	MT_GAIN_MENTAL,
	MT_LOSE_MENTAL,
	MT_GAIN_PHYS,
	MT_LOSE_PHYS,
	MT_GAIN_SOC,
	MT_LOSE_SOC,
	MT_GAIN_SKILLS,
	MT_GAIN_LARGE_SKILLS
} ModifierType;

char* modifierStrings[] = {
	"( Your Mental stat increases! )",
	"( Your Mental stat decreases... )",
	"( Your Physical stat increases! )",
	"( Your Physical stat decreases... )",
	"( Your Social stat increases! )",
	"( Your Social stat decreases... )",
	"( Your gain some skill points! )",
	"( Your gain a lot of skill points! )",
};

typedef struct {
	char* text;
	ModifierType type;
} HistoryModifier;

typedef struct {
	char* text;
	HistoryModifier* sbMods;
	void (*runOnChosen)(void* evt);
} HistoryEvent;

HistoryEvent* sbScholarHistory = NULL;
HistoryEvent* sbScoutHistory = NULL;
HistoryEvent* sbSoldierHistory = NULL;

HistoryModifier createModifier( char* text, ModifierType type )
{
	HistoryModifier hm;
	hm.text = text;
	hm.type = type;
	return hm;
}

HistoryEvent createEvent( char* text, void (*runOnChosen)(void*), int modCount, ... )
{
	HistoryEvent evt;
	evt.text = text;
	evt.sbMods = NULL;
	evt.runOnChosen = runOnChosen;

	va_list args;
	va_start( args, modCount ); {
		for( int i = 0; i < modCount; ++i ) {
			HistoryModifier modifier = va_arg( args, HistoryModifier );
			sb_push( evt.sbMods, modifier );
		}
	} va_end( args );

	return evt;
}

void scoutEventRoll( void* evt )
{
	HistoryEvent* he = (HistoryEvent*)evt;

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	// add the modifiers based on the roll
	int mainRoll = simpleRoll( character.stat_physicalDie );
	int neutralRoll = simpleRoll( character.stat_mentalDie );

	HistoryModifier hm;
	if( mainRoll > neutralRoll ) {
		hm = createModifier( "You know you can be better and spend the next few months practicing ways to conceal yourself.", MT_GAIN_SKILLS );
	} else {
		hm = createModifier( "You know you can't do better and decide to focus on other things.", MT_LOSE_MENTAL );
	}
	sb_push( he->sbMods, hm );
}

void createScoutEvents( void )
{
	// +phys, -ment
	// 1 - Raise Main by one.
	HistoryEvent evtOne = createEvent( "You run everywhere. Others might find it funny but you don't mind.",
		NULL, 1,
		createModifier( "It keeps you good shape.", MT_GAIN_PHYS ) );
	sb_push( sbScoutHistory, evtOne );

	// 2 - Raise Main by one, gain skill 4 points, lower Negative.
	HistoryEvent evtTwo = createEvent( "One of the lead scouts would regularly take you out on expiditions.",
		NULL, 3,
		createModifier( "The long hikes were great for your physique...", MT_GAIN_PHYS ),
		createModifier( "...and you learned much from watching him...", MT_GAIN_SKILLS ),
		createModifier( "...but he rarely talked, and you learned to be silent as well.", MT_LOSE_SOC ) );
	sb_push( sbScoutHistory, evtTwo );

	// 3 - Raise Neutral by one.
	HistoryEvent evtThree = createEvent( "You'd regularly go out into the woods to find a quite spot to read.",
		NULL, 1,
		createModifier( "Being in nature helped you concentrate.", MT_GAIN_MENTAL ) );
	sb_push( sbScoutHistory, evtThree );

	// 4 - Lower Negative.
	HistoryEvent evtFour = createEvent( "A few people you thought were your friends embarrassed you at a large town assembly.",
		NULL, 1,
		createModifier( "You decided people weren't worth the trouble.", MT_LOSE_SOC ) );
	sb_push( sbScoutHistory, evtFour );

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	HistoryEvent evtFive = createEvent( "As part of your training you and some others spend some time hiding and trying to find each other. "
		"When it's your turn to hide you're always found first.",
		scoutEventRoll, 0 );
	sb_push( sbScoutHistory, evtFive );

	// 6 - Lower Negative, gain 8 skill points.
	HistoryEvent evtSix = createEvent( "While on a hike you found an old library that was well perserved. You would regularly go there and would occasionally find "
		"a book that hadn't completely rotted away.",
		NULL, 2,
		createModifier( "The ancient knowledge was plentiful.", MT_GAIN_LARGE_SKILLS ),
		createModifier( "But the time away from people didn't help their views of you.", MT_LOSE_SOC ) );
	sb_push( sbScoutHistory, evtSix );

	// 7 - Raise Main, lower Neutral and Negative.
	HistoryEvent evtSeven = createEvent( "You spent a few years as a hermit out in the woods, surviving off the land.",
		NULL, 3,
		createModifier( "The constant exercise pushed you physically...", MT_GAIN_PHYS ),
		createModifier( "...but the routine was boring...", MT_LOSE_MENTAL ),
		createModifier( "...and going back to being with other people was hard.", MT_LOSE_SOC ) );
	sb_push( sbScoutHistory, evtSeven );

	// 8 - Raise Main and Neutral.
	HistoryEvent evtEight = createEvent( "You found some old films about martial arts, barbarians, and lone warriors succeeding against all odds. "
		"Studying them led to some improvements in how you carry yourself.",
		NULL, 2,
		createModifier( "They inspired you to work hard on both your body...", MT_GAIN_PHYS ),
		createModifier( "...and your wits.", MT_GAIN_MENTAL ) );
	sb_push( sbScoutHistory, evtEight );
}

void soldierEventRoll( void* evt )
{
	HistoryEvent* he = (HistoryEvent*)evt;

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	// add the modifiers based on the roll
	int mainRoll = simpleRoll( character.stat_physicalDie );
	int neutralRoll = simpleRoll( character.stat_socialDie );

	HistoryModifier hm;
	if( mainRoll > neutralRoll ) {
		hm = createModifier( "You asked the winner for some tips on how to improve your form. They taught you all that they knew.", MT_GAIN_SKILLS );
	} else {
		hm = createModifier( "Unable to take the loss you left while noone was looking and never came back.", MT_LOSE_SOC );
	}
	sb_push( he->sbMods, hm );
}

void createSoldierEvents( void )
{
	// +phys, -ment
	// 1 - Raise Main by one.
	HistoryEvent evtOne = createEvent( "Your parents taught you a calisthenics routine that you do every day.",
		NULL, 1,
		createModifier( "It's kept you healthy and in shape.", MT_GAIN_PHYS ) );
	sb_push( sbSoldierHistory, evtOne );

	// 2 - Raise Main by one, gain skill 4 points, lower Negative.
	HistoryEvent evtTwo = createEvent( "You'd regularly skip out on your lessons to go running and enjoying yourself.",
		NULL, 3,
		createModifier( "This hardened your physique...", MT_GAIN_PHYS ),
		createModifier( "...and you learned many things on your romps.", MT_GAIN_SKILLS ),
		createModifier( "But your studies suffered.", MT_LOSE_MENTAL ) );
	sb_push( sbSoldierHistory, evtTwo );

	// 3 - Raise Neutral by one.
	HistoryEvent evtThree = createEvent( "You knew how to hold your drink and were always the life of a party.",
		NULL, 1,
		createModifier( "Learning to deal with unruly drunks proved to be a useful talent.", MT_GAIN_SOC ) );
	sb_push( sbSoldierHistory, evtThree );

	// 4 - Lower Negative.
	HistoryEvent evtFour = createEvent( "While helping transport some ancient container it broken open, releasing some ancient bioweapon. You became very ill.",
		NULL, 1,
		createModifier( "You recovered, but your mind always feels a bit foggy.", MT_LOSE_MENTAL ) );
	sb_push( sbSoldierHistory, evtFour );

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	HistoryEvent evtFive = createEvent( "One evening you met some people on the beach. After hanging out for a while you decided to have a swimming race. You lost by a lot.",
		soldierEventRoll, 0 );
	sb_push( sbSoldierHistory, evtFive );

	// 6 - Lower Negative, gain 8 skill points.
	HistoryEvent evtSix = createEvent( "One of the commanders of the local garrison took an interest in you and taught you a lot about combat and tactics.",
		NULL, 2,
		createModifier( "His decades of experience rubbed off on you.", MT_GAIN_LARGE_SKILLS ),
		createModifier( "He taught you to memorize things, not to think about them.", MT_LOSE_MENTAL ) );
	sb_push( sbSoldierHistory, evtSix );

	// 7 - Raise Main, lower Neutral and Negative.
	HistoryEvent evtSeven = createEvent( "You got in the habit of getting up early in the morning to workout, fewer distractions made it easier to exercise.",
		NULL, 3,
		createModifier( "You gained quite a bit of muscle doing this...", MT_GAIN_PHYS ),
		createModifier( "...but you got used to being alone...", MT_LOSE_SOC ),
		createModifier( "...and were too tired the rest of the day to concentrate.", MT_LOSE_MENTAL ) );
	sb_push( sbSoldierHistory, evtSeven );

	// 8 - Raise Main and Neutral.
	HistoryEvent evtEight = createEvent( "You joined a local dodgeball league, playing until you were sent away.",
		NULL, 2,
		createModifier( "It kept you in shape...", MT_GAIN_PHYS ),
		createModifier( "...and helped you work as part of team.", MT_GAIN_SOC ) );
	sb_push( sbSoldierHistory, evtEight );
}

void scholarEventRoll( void* evt )
{
	HistoryEvent* he = (HistoryEvent*)evt;

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	// add the modifiers based on the roll
	int mentalRoll = simpleRoll( character.stat_mentalDie );
	int socialRoll = simpleRoll( character.stat_socialDie );

	HistoryModifier hm;
	if( mentalRoll > socialRoll ) {
		hm = createModifier( "Later the teacher spends some time going over some of the basic principles with you.", MT_GAIN_SKILLS );
	} else {
		hm = createModifier( "The other students spend the rest of the semester mocking you over your mistake.", MT_LOSE_SOC );
	}
	sb_push( he->sbMods, hm );
}

void createScholarEvents( void )
{
	// 1 - Raise Main by one.
	HistoryEvent evtOne = createEvent( "One of the teachers you had took some extra time to help you get some books that helped your studies.",
		NULL, 1,
		createModifier( "You find a really good book on higher level mathematics.", MT_GAIN_MENTAL ) );
	sb_push( sbScholarHistory, evtOne );

	// 2 - Raise Main by one, gain skill 4 points, lower Negative.
	HistoryEvent evtTwo = createEvent( "A few of your friends invite you to their study group. You meet together for years.",
		NULL, 3,
		createModifier( "Different perspectives on problems helps you solve some really thorny issues.", MT_GAIN_MENTAL ),
		createModifier( "They all have different talents and you learned a lot from all of them.", MT_GAIN_SKILLS ),
		createModifier( "But this was time you didn't spend doing something more physical.", MT_LOSE_PHYS ) );
	sb_push( sbScholarHistory, evtTwo );
	
	// 3 - Raise Neutral by one.
	HistoryEvent evtThree = createEvent( "Some scholars find an old cache of books that appear to be some sort of game about fighting dragons.",
		NULL, 1,
		createModifier( "You end up playing the game together for a couple years.", MT_GAIN_SOC ) );
	sb_push( sbScholarHistory, evtThree );

	// 4 - Lower Negative.
	HistoryEvent evtFour = createEvent( "While helping one of the high ranking scholars carry some equipment you slip and the item falls onto your leg, breaking it.",
		NULL, 1,
		createModifier( "It heals but never is never as good as it once was.", MT_LOSE_PHYS ) );
	sb_push( sbScholarHistory, evtFour );

	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	HistoryEvent evtFive = createEvent( "During a particulary hard class you're called to the front to solve something on the chalkboard. You're unable to solve the problem.",
		scholarEventRoll, 0 );
	sb_push( sbScholarHistory, evtFive );
	
	// 6 - Lower Negative, gain 8 skill points.
	HistoryEvent evtSix = createEvent( "Your parents were very strict. Forcing you to study late into the night and ignore everything else.",
		NULL, 2,
		createModifier( "You learned a lot from your studying.", MT_GAIN_LARGE_SKILLS ),
		createModifier( "But you lacked any sort of physical exercise.", MT_LOSE_PHYS ) );
	sb_push( sbScholarHistory, evtSix );

	// 7 - Raise Main, lower Neutral and Negative.
	HistoryEvent evtSeven = createEvent( "After you failed an entire semester you were put into solitary studies for a year.",
		NULL, 3,
		createModifier( "Without distractions you were able to focus on improving your mind.", MT_GAIN_MENTAL ),
		createModifier( "But without human contact your social skills withered...", MT_LOSE_SOC ),
		createModifier( "...and your body wasted away as well.", MT_LOSE_PHYS ) );
	sb_push( sbScholarHistory, evtSeven );

	// 8 - Raise Main and Neutral.
	HistoryEvent evtEight = createEvent( "For a while you acted as a tutor for other students.",
		NULL, 2,
		createModifier( "Having to teach others helped you grasp issues easier.", MT_GAIN_MENTAL ),
		createModifier( "It also made dealing with difficult people easier as well.", MT_GAIN_SOC ) );
	sb_push( sbScholarHistory, evtEight );
}

void cleanUpEvents( HistoryEvent* sbEvents )
{
	// strings aren't dynamic, so they never need to be cleaned up, just need to clean up stretchy buffers
	for( size_t i = 0; i < sb_count( sbEvents ); ++i ) {
		sb_free( sbEvents[i].sbMods );
	}
	sb_free( sbEvents );
}

void chooseEvent( HistoryEvent* evt )
{
	// run the actual event, adding and subtracting anything that needs to be done
	if( evt->runOnChosen != NULL ) evt->runOnChosen( (void*)evt );

	for( size_t i = 0; i < sb_count( evt->sbMods ); ++i ) {
		switch( evt->sbMods[i].type ) {
		case MT_GAIN_MENTAL:
			increaseStat( &( character.stat_mentalDie ) );
			break;
		case MT_LOSE_MENTAL:
			decreaseStat( &( character.stat_mentalDie ) );
			break;
		case MT_GAIN_PHYS:
			increaseStat( &( character.stat_physicalDie ) );
			break;
		case MT_LOSE_PHYS:
			decreaseStat( &( character.stat_physicalDie ) );
			break;
		case MT_GAIN_SOC:
			increaseStat( &( character.stat_socialDie ) );
			break;
		case MT_LOSE_SOC:
			decreaseStat( &( character.stat_socialDie ) );
			break;
		case MT_GAIN_SKILLS:
			character.skillPointsLeft += 4;
			break;
		case MT_GAIN_LARGE_SKILLS:
			character.skillPointsLeft += 8;
			break;
		}
	}
}

void displayEvent( HistoryEvent* evt, COORD* pos )
{
	WORD neutral = FG_GREY | BG_BLACK;
	WORD loss = FG_RED | BG_BLACK;
	WORD gain = FG_GREEN | BG_BLACK;

#define NEXTLINE { pos->X = 2; ++(pos->Y); }

	// display main text
	( *pos ) = drawString( pos->X, pos->Y, safeWriteArea, evt->text, neutral );

	for( size_t i = 0; i < sb_count( evt->sbMods ); ++i ) {
		WORD attr = neutral;
		switch( evt->sbMods[i].type ) {
		case MT_GAIN_MENTAL:
		case MT_GAIN_PHYS:
		case MT_GAIN_SOC:
		case MT_GAIN_SKILLS:
		case MT_GAIN_LARGE_SKILLS:
			attr = gain;
			break;
		case MT_LOSE_MENTAL:
		case MT_LOSE_PHYS:
		case MT_LOSE_SOC:
			attr = loss;
			break;
		}

		NEXTLINE;
		// display modifier text
		( *pos ) = drawString( pos->X, pos->Y, safeWriteArea, evt->sbMods[i].text, neutral );

		NEXTLINE;
		// display modifier effect
		(*pos) = drawString( pos->X, pos->Y, safeWriteArea, modifierStrings[evt->sbMods[i].type], attr );
	}
#undef NEXTLINE
}

void historyScene( void )
{
	topTitle = " What Has Shaped You? ";

	// we have eight events per class, choose three of them
	//  for each one you have a 1 in 5 chance of the event to be from a different class
	// the basic format for the skills will be this:
	// 
	// 1 - Raise Main by one.
	// 2 - Raise Main by one, gain skill 4 points, lower Negative.
	// 3 - Raise Neutral by one.
	// 4 - Lower Negative.
	// 5 - Roll Main and Neutral, if Main > Neutral, gain 4 skill points, otherwise lower Neutral.
	// 6 - Lower Negative, gain 8 skill points.
	// 7 - Raise Main, lower Neutral and Negative.
	// 8 - Raise Main and Neutral.
	//
	// Where Main is the classes main attribute (mental for scholars, physical for scouts and soldiers)
	// Negative is the classes weak attribute (physical for scholars, social for scouts, and mental for soldiers)
	// Neutral is the other attribute (social for scholars, mental for scouts, and social for soldiers)

	createScholarEvents( );
	createSoldierEvents( );
	createScoutEvents( );

	// we'll have ten events, choose three of them, will affect stats and how
	//  many skill points you start with
	int indices[][8] = {
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 } };
	SHUFFLE( indices[0], int );
	SHUFFLE( indices[1], int );
	SHUFFLE( indices[2], int );

	// each event will generally pull from their main class, but there's a chance it will pull from one of
	//  the others, 0 is main, 1 and 2 are others
	HistoryEvent* lists[3];

	switch( character.class ) {
	case CC_SOLDIER:
		lists[0] = sbSoldierHistory;
		lists[1] = sbScoutHistory;
		lists[2] = sbScholarHistory;
		break;
	case CC_SCOUT:
		lists[0] = sbScoutHistory;
		lists[1] = sbSoldierHistory;
		lists[2] = sbScholarHistory;
		break;
	case CC_SCHOLAR:
		lists[0] = sbScholarHistory;
		lists[1] = sbScoutHistory;
		lists[2] = sbSoldierHistory;
		break;
	}

	int eventLists[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	for( size_t i = 0; i < ARRAYSIZE( eventLists ); ++i ) {
		if( ( rand( ) % CHANCE_NON_CLASS_EVENT_DENOM ) == 0 ) {
			// use a different class
			eventLists[i] = ( rand( ) % 2 ) + 1;
		}
	}

	int currDisplay = 0;

	while( currDisplay < NUM_HISTORY_EVENTS ) {
		COORD currPos = { 2, 4 };
#define NEXTLINE { currPos.X = 2; ++currPos.Y; }
#define NEXTEVENT { currPos.X = 2; currPos.Y += 2; }
		startDraw( ); {
			currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
					"The following events are the major ones that have shaped your life:",
					FG_GREY | BG_BLACK );

			for( int i = 0; i <= currDisplay; ++i ) {
				int list = eventLists[i];
				HistoryEvent* evt = &( lists[list][indices[list][i]] );
				// process the new event
				if( i == currDisplay ) {
					chooseEvent( evt );
				}

				NEXTEVENT;
				// display the event
				displayEvent( evt, &currPos );
			}

			NEXTEVENT;
			if( currDisplay == ( NUM_HISTORY_EVENTS - 1 ) ) {
				currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
					"Press any key to select your skills",
					FG_YELLOW | BG_BLACK );
			} else {
				currPos = drawString( currPos.X, currPos.Y, safeWriteArea,
					"Press any key to see the next event",
					FG_WHITE | BG_BLACK );
			}

			char buffer[1024];
			sprintf( buffer, "Physical: d%i   Mental: d%i    Social: d%i",
				character.stat_physicalDie, character.stat_mentalDie, character.stat_socialDie );
			drawStringIgnoreSize( 10, 45, safeWriteArea, buffer, FG_CYAN | BG_BLACK );

			sprintf( buffer, "Skill Points: %i", character.skillPointsLeft );
			drawString( 10, 46, safeWriteArea, buffer, FG_CYAN | BG_BLACK );
			
#undef NEXTLINE
#undef NEXTEVENT
		} endDraw( );

		waitForAnyInput( );
		++currDisplay;
	}

	cleanUpEvents( sbScholarHistory );
	sbScholarHistory = NULL;
	cleanUpEvents( sbSoldierHistory );
	sbSoldierHistory = NULL;
	cleanUpEvents( sbScoutHistory );
	sbScoutHistory = NULL;

	currentScene = skillSelectionScene;
}

void classSelectionScene( void )
{
	topTitle = " What is your function in life? ";

	boolean done = false;
	CharacterClass selectedClass = CC_SOLDIER;

	WORD normal = FG_DARK_CYAN | BG_BLACK;
	WORD highlight = FG_CYAN | BG_BROWN;

	char* descriptions[] = {
		"Trained in combat and tasked with protecting the settlement from any dangers. You've done little more than be a glorified security guard for the settlement "
		"since you got here and are itching for some actual action. As a soldier you are in good physical condition, but have had little time for studying. You'll "
		"get some bonus ranks in the Shooting and Acrobatics skills.",

		"Trained in survival and stealth, scouts form the first line of defense for the settlement. You've spent more time out in the field than you have in the "
		"settlement proper, only coming back to report and resupply. As a scout you are in good physical condition, but your lack of contact with others make "
		"your social skills subpar. You'll get some bonus ranks in the Survival and Stealth skills.",

		"Trained in various fields of study, artifact preservation, research methods, and philosophy. You are a generalist scholar, choosing to know a little "
		"of everything instead of choosing only one field of study. As a scholar you will have a strong mind, but your physical ability is lacking. You'll also "
		"get some bonus ranks in the Knowledge and Investigate skills."
	};

	initCharacter( );

	do {
		startDraw( ); {
			drawString( 2, 2, safeWriteArea,
				"What was the job has you been trained for and performed? This will effect the stats, skills, and equipment you will "
				"have later on.\n\n"
				"Use the up and down arrows to highlight which class you want. A short description will appear at the bottom. Press "
				"Enter to choose the highlighted class and continue.",
				FG_GREY | BG_BLACK );

			// draw the class selection
			centerStringHoriz( safeWriteArea, 10, "Soldier", selectedClass == CC_SOLDIER ? highlight : normal );
			centerStringHoriz( safeWriteArea, 11, "Scout", selectedClass == CC_SCOUT ? highlight : normal );
			centerStringHoriz( safeWriteArea, 12, "Scholar", selectedClass == CC_SCHOLAR ? highlight : normal );

			drawString( 2, 20, safeWriteArea, descriptions[selectedClass], FG_WHITE | FG_BLACK );

			centerStringHoriz( safeWriteArea, safeWriteArea.Bottom,
				"Press Enter to continue to your history",
				FG_YELLOW | BG_BLACK );

		} endDraw( );

		// handle input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				handled = true;
				--selectedClass;
				if( selectedClass < 0 ) {
					selectedClass = NUM_CLASSES - 1;
				}
				break;
			case IN_DOWN:
				handled = true;
				++selectedClass;
				if( selectedClass >= NUM_CLASSES ) {
					selectedClass = 0;
				}
				break;
			case IN_ENTER:
				handled = true;
				done = true;
				break;
			}
		} while( !handled );
		eatAllInputs( );
	} while( !done );

	character.class = selectedClass;

	currentScene = historyScene;
}

void introScene( void )
{
	topTitle = " Welcome! ";
	startDraw( ); {
		drawString( 2, 2, safeWriteArea,
			"The Golden Age of Technology ended a few centuries ago.\n\n"
			"Humanity barely survived, but have been slowly regaining knowledge and "
			"expanding.\n\n"
			"You've been working for The Coalition, a group created by the new nations "
			"to rediscover lost knowledge, for your entire life. Last year you were "
			"moved to a new settlement and have been enjoying your work here.\n\n"
			"Recently scouts have been returning with alarming reports. Robots have "
			"begun coming out of the nearby ruined city called Cuglatr. This city "
			"has been a major source of ancient knowledge and technology.\n\n"
			"Most disturbing is the behavior of these robots. When this sort of thing "
			"usually happens the robots are easily subdued. But these ones are coming "
			"out in greater numbers than ever before and they act erratically. They "
			"could prove to be a danger to the entire region surrounding the ruins.\n\n"
			"The source of robots has been found by a scout, but he was not equipped "
			"to deal with the issue. So you and the scholar Freda are being sent to "
			"try and deal with this.\n\n"
			"It will be many days travel through the wilderness. The safety of the "
			"settlement and region resting on your shoulders.\n\n"
			"Before you're sent to you almost certain demise lets figure out who you are.",
			FG_GREY | BG_BLACK );

		centerStringHoriz( safeWriteArea, safeWriteArea.Bottom, "Press any key to start", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	currentScene = classSelectionScene;
}

void titleScene( void )
{
	startDraw( ); {
		centerStringHoriz( renderArea, 11, "Welcome To", FG_GREY | BG_BLACK );
		centerStringHoriz( renderArea, 14, "The Ruins of", FG_GREY | BG_BLACK );
		char blockText[] = {
			201,205,205,205,205,205,187, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32,205,206,205, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32, 32, 32, 32, 32, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32,186, 32, 32, 32,186, 32, 32,201,205,205,205,187, 32, 32,186, 32, 32,201,205,205,205,187, 32, 32,186, 32, 32,201,205,205,205,187, 10,
			186, 32, 32, 32, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32,186, 32, 32, 32,186, 32, 32,186, 32, 32,186, 32, 32, 32, 32, 10,
			200,205,205,205,205,205,188, 32, 32,200,205,205,205,188, 32, 32,200,205,205,205,185, 32, 32,186, 32, 32,200,205,205,205,185, 32, 32,186, 32, 32,186, 32, 32, 32, 32, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,200,205,205,205,188, 10,
			 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,205,203,205,203,205, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32,186, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32,186, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 32,186, 10,
			 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,205,202,205,202,205, 0
		};

		centerStringHoriz( renderArea, 16, blockText, FG_CYAN | BG_BLACK );

		centerStringHoriz( renderArea, 34, "A Game Created For", FG_GREY | BG_BLACK );
		centerStringHoriz( renderArea, 35, "Ludum Dare 49", FG_BROWN | BG_BLACK );

		centerStringHoriz( safeWriteArea, safeWriteArea.Bottom, "Press any key to begin", FG_YELLOW | BG_BLACK );

	} endDraw( );

	waitForAnyInput( );

	currentScene = introScene;
}

void deathScene( void )
{
	startDraw( ); {
		centerStringHoriz( renderArea, 10, "You Have Died...", FG_RED | BG_BLACK );
		char graveOutLine[] =
		{
			201,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,205,187, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 10,
			186, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,186, 0
		};
		centerStringHoriz( renderArea, 16, graveOutLine, FG_GREY | BG_BLACK );


		char graveInside[] =
		{
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 10,
			177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177,177, 0
		};
		centerStringHoriz( renderArea, 17, graveInside, FG_GREY | BG_BLACK );

		char ground[] =
		{
			219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219,219, 0
		};
		centerStringHoriz( renderArea, 33, ground, FG_GREEN | BG_BROWN );

		drawStringIgnoreSize( 34, 32, renderArea, "\\/", FG_GREEN | BG_GREY );

		drawStringIgnoreSize( 42, 32, renderArea, "/", FG_GREEN | BG_GREY );

		SMALL_RECT deathTextArea = { 32, 20, 48, 30 };
		drawString( deathTextArea.Left, deathTextArea.Top, deathTextArea, deathReason, FG_BLACK | BG_GREY );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	nextScene = NULL;
	currentScene = titleScene;
}

void helpScene( void )
{
	topTitle = " Help ";
	nextScene = NULL;
	COORD outPos;
	fromStatusScreen = true;

	startDraw( ); {
		outPos = drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea, "Controls", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"When presented with a choice press up and down to select the choice you want. Then press enter to make that choice.\n"
			"Pressing H will bring up this help screen, pressing C will bring up a more detailed character sheet.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Choices", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"When a choice has a skill name behind it in parenthesis that means that choice will require a skill check. If you "
			"roll too low you will not succeed at the task and you will take a penalty of some kind (e.g. gain a wound or lose an "
			"item). If you roll high enough you succeed. If you roll somewhere in the middle you'll have a costly success. You will "
			"still succeed at the task but take a penalty like you would with a failure.\n"
			"Some choices will also require an item, if you do not have the correct item you will not see that choice available.\n"
			"If you see an arrow at the bottom or top of the choice section of the screen that means there are more choices in that "
			"direction.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Wounds", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Some failures will result in you gaining a Wound. These wounds are associated with a specific Statistic (Physical, "
			"Mental, or Social). They will give you a penalty on any roll involving that Statistic. When the number of Wounds "
			"you have for a Statistic matches the die size of that Statistic you are no longer to use any skills associated "
			"with that Statistic. The only exception is Physical. If you Physical wounds equals your Physical die size you "
			"lose the game.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Gear", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Equipment you find and can use to help in certain Choices. There is a chance you will lose the equipment if you "
			"get a Failure or Costly Success.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Companions", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"Companions are listed with bonuses they give to your Skill checks. Some Choices will only be available if you "
			"have a Companion with you. They take care of themselves so you don't need to worry about their equipment.",
			FG_GREY );

		outPos = drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea, "Goal", FG_WHITE );
		outPos = drawString( safeWriteArea.Left + 1, outPos.Y + 1, safeWriteArea,
			"You've been tasked by The Coalition to escort a tech-initiate through the wilderness, find the source of "
			"the insane robots being created, and stop it. You also have the secondary goal of securing any technology "
			"you can for further study.",
			FG_GREY );

		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	eatAllInputs( );

	topTitle = NULL;
	currentScene = storedScene;
}

void characterDetailsScene( void )
{
	topTitle = " Character Details ";
	nextScene = NULL;
	COORD outPos;
	fromStatusScreen = true;

	SMALL_RECT physicalColumn = { 5, 5, 25, 11 };
	SMALL_RECT mentalColumn = { 30, 5, 50, 11 };
	SMALL_RECT socialColumn = { 55, 5, 75, 11 };
	SMALL_RECT companionArea = { 5, 14, SCREEN_WIDTH - 6, 19 };
	SMALL_RECT inventoryArea = { 5, 22, SCREEN_WIDTH - 6, 45 };
	
	startDraw( ); {

		// class
		drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea, "Class: %s", FG_GREY, classNames[character.class] );

		// physical
		outPos = drawString( physicalColumn.Left, physicalColumn.Top, physicalColumn, "Physical: d%i", FG_GREEN, character.stat_physicalDie );
		int skillIdx = 0;
		for( ; skillIdx < 4; ++skillIdx ) {
			outPos = drawString( physicalColumn.Left + 1, outPos.Y + 1, physicalColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( physicalColumn.Left + 1, outPos.Y + 2, physicalColumn, "Wounds: %i", FG_MAROON, character.wounds_physical );

		// mental
		outPos = drawString( mentalColumn.Left, mentalColumn.Top, mentalColumn, "Mental: d%i", FG_GREEN, character.stat_mentalDie );
		for( ; skillIdx < 8; ++skillIdx ) {
			outPos = drawString( mentalColumn.Left + 1, outPos.Y + 1, mentalColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( mentalColumn.Left + 1, outPos.Y + 2, mentalColumn, "Wounds: %i", FG_MAROON, character.wounds_mental );

		// social
		outPos = drawString( socialColumn.Left, socialColumn.Top, socialColumn, "Social: d%i", FG_GREEN, character.stat_socialDie );
		for( ; skillIdx < 12; ++skillIdx ) {
			outPos = drawString( socialColumn.Left + 1, outPos.Y + 1, socialColumn, "%s: %i", FG_CYAN,
				skillsData[skillIdx].name, (*skillsData[skillIdx].value) );
		}
		outPos = drawString( socialColumn.Left + 1, outPos.Y + 2, socialColumn, "Wounds: %i", FG_MAROON, character.wounds_social );

		// companions
		outPos = drawString( companionArea.Left, companionArea.Top, companionArea, "Companions", FG_GREEN );
		for( int i = 0; i < NUM_COMPANIONS; ++i ) {
			if( character.hasCompanion[i] ) {
				outPos = drawString( companionArea.Left, outPos.Y + 1, companionArea, "%s", FG_CYAN,
					companionsData[i].name );

				outPos = drawString( companionArea.Right - 45, outPos.Y, companionArea, "(P: %i, M: %i, S: %i)", FG_DARK_CYAN,
					companionsData[i].physical, companionsData[i].mental, companionsData[i].social );
			}
		}

		// equipment
		outPos = drawString( inventoryArea.Left, inventoryArea.Top, inventoryArea, "Gear", FG_GREEN );
		SHORT x = inventoryArea.Left + 1;
		for( int i = 0; i < NUM_GEAR; ++i ) {
			if( character.gear[i] > 0 ) {
				outPos = drawString( x, outPos.Y + 1, inventoryArea, "%s x %i", FG_CYAN,
					gearData[i].name, character.gear[i] );

				if( outPos.Y > inventoryArea.Bottom ) {
					x += ( inventoryArea.Right - ( inventoryArea.Left + 1 ) ) / 3;
					outPos.Y = inventoryArea.Top;
				}
			}
		}
		
		centerStringHoriz( renderArea, renderArea.Bottom, "Press any key to continue", FG_YELLOW | BG_BLACK );
	} endDraw( );

	waitForAnyInput( );
	eatAllInputs( );

	currentScene = storedScene;
	topTitle = NULL;
}

void dropGearScene( void )
{
	topTitle = " Over-Encumbered ";
	COORD outPos;
	int selectedItem = 0;
	// make sure the selected item is valid
	while( character.gear[selectedItem] == 0 ) {
		++selectedItem;
	}

	WORD normalAttr = FG_DARK_CYAN | BG_BLACK;
	WORD highlightAttr = FG_CYAN | BG_BROWN;
	WORD attr;

	SMALL_RECT gearColumn = { 5, 7, 25, 45 };

	do {
		startDraw( ); {
			outPos = drawString( safeWriteArea.Left, safeWriteArea.Top, safeWriteArea,
				"You're carrying too much stuff, use the up and down arrow keys to choose an item, and press enter to drop it.", FG_GREY );
			drawString( safeWriteArea.Left, outPos.Y + 2, safeWriteArea,
				"You have to drop %i more %s.", FG_RED, -gearSpaceLeft( ), ( gearSpaceLeft( ) == -1 ) ? "item" : "items" );

			outPos.Y = gearColumn.Top - 1;
			for( int i = 0; i < NUM_GEAR; ++i ) {
				if( character.gear[i] != 0 ) {
					attr = ( i == selectedItem ) ? highlightAttr : normalAttr;
					outPos = drawString( gearColumn.Left, outPos.Y + 1, gearColumn, "%s x %i", attr,
						gearData[i].name, character.gear[i] );
				}
			}

		} endDraw( );

		// process input
		Input input;
		boolean handled = false;
		do {
			input = getNextInput( );
			switch( input ) {
			case IN_UP:
				handled = true;
				do {
					--selectedItem;
					if( selectedItem < 0 ) {
						selectedItem = NUM_GEAR - 1;
					}
				} while( character.gear[selectedItem] == 0 );
				break;
			case IN_DOWN:
				handled = true;
				do {
					selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
				} while( character.gear[selectedItem] == 0 );
				break;
			case IN_ENTER:
				--character.gear[selectedItem];
				if( character.gear[selectedItem] == 0 ) {
					do {
						selectedItem = ( selectedItem + 1 ) % NUM_GEAR;
					} while( character.gear[selectedItem] == 0 );
				}
				handled = true;
				break;
			}
		} while( !handled );

		eatAllInputs( );
	} while( gearSpaceLeft( ) < 0 );

	currentScene = nextScene;
}

/******* END SCENES *********/

int main( int argc, char** argv )
{
	screen.write = GetStdHandle( STD_OUTPUT_HANDLE );
	screen.read = GetStdHandle( STD_INPUT_HANDLE );

	SetConsoleWindowInfo( screen.write, TRUE, &windowSize );
	SetConsoleScreenBufferSize( screen.write, bufferSize );

	SetConsoleTitle( TEXT( "The Ruins of Cuglatr II" ) );

	srand( (unsigned int)time( NULL ) );
	topTitle = NULL;

	testDevCharacter( );

	setupSkillsData( );
	setupGearData( );
	setupCompanionData( );

	deathReason = "UNKNOWN DEATH REASON";

	currentScene = titleScene;

	// approximately 14200 words, holy fuck!
	while( 1 ) {
		currentScene( );
	}

	return 0;
}