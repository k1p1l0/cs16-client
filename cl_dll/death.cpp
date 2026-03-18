/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// death notice
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include "draw_util.h"

float color[3];

struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH*2];
	char szVictim[MAX_PLAYER_NAME_LENGTH*2];
	int iId;	// the index number of the associated sprite
	bool bSuicide;
	bool bTeamKill;
	bool bNonPlayerKill;
	float flDisplayTime;
	float *KillerColor;
	float *VictimColor;
	int iHeadShotId;
	int iDrawBg;  // CSO: 0=none, 1=green(you killed), 2=red(you died)
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

int CHudDeathNotice :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( gHUD.m_DeathNotice, DeathMsg );

	hud_deathnotice_time = CVAR_CREATE( "hud_deathnotice_time", "6", FCVAR_ARCHIVE );
	m_iFlags = 0;
	m_iKillStreak = 0;
	m_flLastKillTime = 0;
	m_flStreakDisplayTime = 0;
	m_szStreakText[0] = 0;

	return 1;
}


void CHudDeathNotice :: InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}


int CHudDeathNotice :: VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );
	m_HUD_d_headshot = gHUD.GetSpriteIndex("d_headshot");

	return 1;
}

int CHudDeathNotice :: Draw( float flTime )
{
	int x, y, r, g, b, i;

	for( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if ( rgDeathNoticeList[i].flDisplayTime < flTime )
		{ // display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = min( rgDeathNoticeList[i].flDisplayTime, flTime + DEATHNOTICE_DISPLAY_TIME );

		// Hide when scoreboard drawing. It will break triapi
		//if ( gViewPort && gViewPort->AllowedToPrintText() )
		//if ( !gHUD.m_iNoConsolePrint )
		{
			// Draw the death notice
			if( !g_iUser1 )
			{
				y = YRES(DEATHNOTICE_TOP) + 2 + (20 * i);  //!!!
			}
			else
			{
				y = ScreenHeight / 5 + 2 + (20 * i);
			}

			int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;

			// CSO HUD: Draw background rectangle behind kill feed entry
			if( gHUD.m_hudstyle && gHUD.m_hudstyle->value >= 1 )
			{
				int bgPadX = 6;
				int bgPadY = 2;
				int killerLen = rgDeathNoticeList[i].bSuicide ? 0 : DrawUtils::ConsoleStringLen( rgDeathNoticeList[i].szKiller );
				int victimLen = DrawUtils::ConsoleStringLen( rgDeathNoticeList[i].szVictim );
				int spriteWidth = gHUD.GetSpriteRect(id).Width();
				if( rgDeathNoticeList[i].iHeadShotId )
					spriteWidth += gHUD.GetSpriteRect(m_HUD_d_headshot).Width();
				int totalWidth = killerLen + spriteWidth + victimLen + bgPadX * 3;

				int bgX = ScreenWidth - totalWidth - bgPadX;
				int bgY = y - bgPadY - 2;
				int bgH = 24;

				int bgR, bgG, bgB, bgA;
				switch( rgDeathNoticeList[i].iDrawBg )
				{
				case 1:  bgR = 0; bgG = 80; bgB = 0; bgA = 120; break;   // green
				case 2:  bgR = 80; bgG = 0; bgB = 0; bgA = 120; break;   // red
				default: bgR = 20; bgG = 20; bgB = 20; bgA = 100; break;  // neutral
				}

				FillRGBA( bgX, bgY, totalWidth + bgPadX, bgH, bgR, bgG, bgB, bgA );
			}
			x = ScreenWidth - DrawUtils::ConsoleStringLen(rgDeathNoticeList[i].szVictim) - (gHUD.GetSpriteRect(id).Width());
			if( rgDeathNoticeList[i].iHeadShotId )
				x -= gHUD.GetSpriteRect(m_HUD_d_headshot).Width();

			if ( !rgDeathNoticeList[i].bSuicide )
			{
				x -= (5 + DrawUtils::ConsoleStringLen( rgDeathNoticeList[i].szKiller ) );

				// Draw killers name
				if ( rgDeathNoticeList[i].KillerColor )
					DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2] );
				x = 5 + DrawUtils::DrawConsoleString( x, y, rgDeathNoticeList[i].szKiller );
			}

			r = 255;  g = 80;	b = 0;
			if ( rgDeathNoticeList[i].bTeamKill )
			{
				r = 10;	g = 240; b = 10;  // display it in sickly green
			}

			// Draw death weapon
			SPR_Set( gHUD.GetSprite(id), r, g, b );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(id) );

			x += (gHUD.GetSpriteRect(id).Width());

			if( rgDeathNoticeList[i].iHeadShotId)
			{
				SPR_Set( gHUD.GetSprite(m_HUD_d_headshot), r, g, b );
				SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(m_HUD_d_headshot));
				x += (gHUD.GetSpriteRect(m_HUD_d_headshot).Width());
			}

			// Draw victims name (if it was a player that was killed)
			if (!rgDeathNoticeList[i].bNonPlayerKill)
			{
				if ( rgDeathNoticeList[i].VictimColor )
					DrawUtils::SetConsoleTextColor( rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2] );
				x = DrawUtils::DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
			}
		}
	}

	if( i == 0 )
		m_iFlags &= ~HUD_DRAW; // disable hud item

	// CSO HUD: Draw kill streak notification (center screen)
	if( gHUD.m_hudstyle && gHUD.m_hudstyle->value >= 1 && m_szStreakText[0] )
	{
		float elapsed = flTime - m_flStreakDisplayTime;
		if( elapsed < 2.5f )
		{
			float alpha = 1.0f;
			if( elapsed > 2.0f )
				alpha = 1.0f - ( ( elapsed - 2.0f ) / 0.5f );

			// Scale effect: starts big, settles to normal
			float scaleEffect = 1.0f;
			if( elapsed < 0.15f )
				scaleEffect = 1.3f - ( elapsed / 0.15f ) * 0.3f;

			int sa = (int)( 255 * alpha );
			int sr = 255, sg = 220, sb = 50;
			DrawUtils::ScaleColors( sr, sg, sb, sa );

			int textLen = DrawUtils::HudStringLen( m_szStreakText );
			int tx = ( ScreenWidth - textLen ) / 2;
			int ty = ScreenHeight / 4;

			// Background panel
			FillRGBA( tx - 20, ty - 8, textLen + 40, 30, 0, 0, 0, (int)( 160 * alpha ) );
			// Gold accent lines
			FillRGBA( tx - 20, ty - 8, textLen + 40, 2, 255, 200, 50, (int)( 200 * alpha ) );
			FillRGBA( tx - 20, ty + 22, textLen + 40, 2, 255, 200, 50, (int)( 200 * alpha ) );
			DrawUtils::DrawHudString( tx, ty, ScreenWidth, m_szStreakText, sr, sg, sb );
		}
		else
		{
			m_szStreakText[0] = 0;
		}
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_DRAW;

	BufferReader reader( pszName, pbuf, iSize );

	int killer = reader.ReadByte();
	int victim = reader.ReadByte();
	int headshot = reader.ReadByte();

	char killedwith[32];
	strncpy( killedwith, "d_", sizeof(killedwith) );
	strncat( killedwith, reader.ReadString(), sizeof( killedwith ) - 2 );

	//if (gViewPort)
	//	gViewPort->DeathMsg( killer, victim );
	gHUD.m_Scoreboard.DeathMsg( killer, victim );

	gHUD.m_Spectator.DeathMessage(victim);
	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	//if (gViewPort)
		//gViewPort->GetAllPlayersInfo();
	gHUD.m_Scoreboard.GetAllPlayersInfo();

	// Get the Killer's name
	const char *killer_name = g_PlayerInfoList[ killer ].name;
	if ( !killer_name )
	{
		killer_name = "";
		rgDeathNoticeList[i].szKiller[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
	}

	// Get the Victim's name
	const char *victim_name = NULL;
	// If victim is -1, the killer killed a specific, non-player object (like a sentrygun)
	if ( ((char)victim) != -1 )
		victim_name = g_PlayerInfoList[ victim ].name;
	if ( !victim_name )
	{
		victim_name = "";
		rgDeathNoticeList[i].szVictim[0] = 0;
	}
	else
	{
		rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
	}

	// Is it a non-player object kill?
	if ( ((char)victim) == -1 )
	{
		rgDeathNoticeList[i].bNonPlayerKill = true;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strncpy( rgDeathNoticeList[i].szVictim, killedwith+2, sizeof(killedwith) );
	}
	else
	{
		if ( killer == victim || killer == 0 )
			rgDeathNoticeList[i].bSuicide = true;

		if ( !strncmp( killedwith, "d_teammate", sizeof(killedwith)  ) )
			rgDeathNoticeList[i].bTeamKill = true;
	}

	rgDeathNoticeList[i].iHeadShotId = headshot;

	// CSO HUD: Determine background type based on local player
	rgDeathNoticeList[i].iDrawBg = 0;
	if( gHUD.m_hudstyle && gHUD.m_hudstyle->value >= 1 )
	{
		int localIdx = gEngfuncs.GetLocalPlayer()->index;

		if( killer == localIdx && victim != localIdx )
		{
			rgDeathNoticeList[i].iDrawBg = 1;  // green — you killed

			// Kill streak tracking
			if( gHUD.m_flTime - m_flLastKillTime < 4.0f )
				m_iKillStreak++;
			else
				m_iKillStreak = 1;

			m_flLastKillTime = gHUD.m_flTime;

			// Set streak text
			switch( m_iKillStreak )
			{
			case 2: strncpy( m_szStreakText, "DOUBLE KILL!", sizeof(m_szStreakText) ); m_flStreakDisplayTime = gHUD.m_flTime; break;
			case 3: strncpy( m_szStreakText, "TRIPLE KILL!", sizeof(m_szStreakText) ); m_flStreakDisplayTime = gHUD.m_flTime; break;
			case 4: strncpy( m_szStreakText, "ULTRA KILL!", sizeof(m_szStreakText) ); m_flStreakDisplayTime = gHUD.m_flTime; break;
			case 5: strncpy( m_szStreakText, "RAMPAGE!", sizeof(m_szStreakText) ); m_flStreakDisplayTime = gHUD.m_flTime; break;
			default:
				if( m_iKillStreak > 5 )
				{
					snprintf( m_szStreakText, sizeof(m_szStreakText), "%d KILL STREAK!", m_iKillStreak );
					m_flStreakDisplayTime = gHUD.m_flTime;
				}
				break;
			}

			// Headshot notification
			if( headshot && m_iKillStreak < 2 )
			{
				strncpy( m_szStreakText, "HEADSHOT!", sizeof(m_szStreakText) );
				m_flStreakDisplayTime = gHUD.m_flTime;
			}
		}
		else if( victim == localIdx )
		{
			rgDeathNoticeList[i].iDrawBg = 2;  // red — you died
			m_iKillStreak = 0;  // reset streak on death
		}
	}

	// Find the sprite in the list
	int spr = gHUD.GetSpriteIndex( killedwith );

	rgDeathNoticeList[i].iId = spr;

	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + hud_deathnotice_time->value;


	if (rgDeathNoticeList[i].bNonPlayerKill)
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if ( rgDeathNoticeList[i].bSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if ( !strncmp( killedwith, "d_world", sizeof(killedwith)  ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " killed self" );
			}
		}
		else if ( rgDeathNoticeList[i].bTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			if( headshot )
				ConsolePrint( "*** ");
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if ( *killedwith && (*killedwith > 13 ) && strncmp( killedwith, "d_world", sizeof(killedwith) ) && !rgDeathNoticeList[i].bTeamKill )
		{
			if ( headshot )
				ConsolePrint(" with a headshot from ");
			else
				ConsolePrint(" with ");

			ConsolePrint( killedwith+2 ); // skip over the "d_" part
		}

		if( headshot ) ConsolePrint( " ***");
		ConsolePrint( "\n" );
	}

	return 1;
}




