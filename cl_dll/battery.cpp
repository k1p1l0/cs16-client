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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "draw_util.h"

int CHudBattery::Init( void )
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	m_enArmorType = Vest;

	HOOK_MESSAGE( gHUD.m_Battery, Battery );
	HOOK_MESSAGE( gHUD.m_Battery, ArmorType );
	gHUD.AddHudElem( this );

	return 1;
}

int CHudBattery::VidInit( void )
{
	m_hEmpty[Vest].SetSpriteByName("suit_empty");
	m_hFull[Vest].SetSpriteByName("suit_full");
	m_hEmpty[VestHelm].SetSpriteByName("suithelmet_empty");
	m_hFull[VestHelm].SetSpriteByName("suithelmet_full");

	m_iHeight = m_hFull[Vest].rect.Height();
	m_fFade = 0;

	return 1;
}


int CHudBattery:: MsgFunc_Battery(const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	m_iFlags |= HUD_DRAW;
	int x = reader.ReadShort();

	if( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
		if( m_iBat < 0 )
			m_enArmorType = Vest;
	}

	return 1;
}

int CHudBattery::Draw( float flTime )
{
	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	// CSO HUD style: draw armor bar
	if( gHUD.m_hudstyle && gHUD.m_hudstyle->value >= 1 )
	{
		DrawCSO_ArmorBar( flTime );
		return 1;
	}

	int r, g, b, x, y, a;
	wrect_t rc;

	rc = m_hEmpty[m_enArmorType].rect;

	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1
	rc.top += m_iHeight * ((float)( 100 - ( min( 100, m_iBat ))) * 0.01f );

	DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	// Has health changed? Flash the health #
	if( m_fFade )
	{
		if( m_fFade > FADE_TIME )
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);

		if( m_fFade <= 0 )
		{
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA +  (m_fFade / FADE_TIME) * 128;
	}
	else
	{
		a = MIN_ALPHA;
	}

	DrawUtils::ScaleColors( r, g, b, a );
	
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = ScreenWidth / 5;

	// make sure we have the right sprite handles
	SPR_Set( m_hFull[m_enArmorType].spr, r, g, b );
	SPR_DrawAdditive( 0, x, y, &m_hFull[m_enArmorType].rect );

	if( rc.bottom > rc.top )
	{
		SPR_Set( m_hEmpty[m_enArmorType].spr, r, g, b );
		SPR_DrawAdditive( 0, x, y + (rc.top - m_hEmpty[m_enArmorType].rect.top), &rc );
	}

	x += (m_hEmpty[m_enArmorType].rect.Width());
	x = DrawUtils::DrawHudNumber( x, y, DHN_3DIGITS|DHN_DRAWZERO, m_iBat, r, g, b );

	return 1;
}

int CHudBattery::MsgFunc_ArmorType(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	m_enArmorType = (armortype_t)reader.ReadByte();

	return 1;
}

//
// DrawCSO_ArmorBar - CSO-style armor with blue bar
//
// Layout (bottom-left, right of health area):
//   [Armor Icon] [Number] [===Blue Bar===]
//
void CHudBattery::DrawCSO_ArmorBar( float flTime )
{
	int r = 80, g = 140, b = 255;  // Blue
	int a = 255;

	// Fade logic
	if( m_fFade )
	{
		if( m_fFade > FADE_TIME )
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if( m_fFade <= 0 )
			m_fFade = 0;

		a = MIN_ALPHA + (m_fFade / FADE_TIME) * 128;
	}
	else
	{
		a = MIN_ALPHA;
	}

	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = ScreenWidth / 5;

	// Draw armor icon (white for modern look)
	int iconR = 255, iconG = 255, iconB = 255;
	DrawUtils::ScaleColors( iconR, iconG, iconB, a );

	// Draw filled armor sprite
	SPR_Set( m_hFull[m_enArmorType].spr, iconR, iconG, iconB );
	SPR_DrawAdditive( 0, x, y, &m_hFull[m_enArmorType].rect );

	// Draw empty portion on top
	if( m_iBat < 100 )
	{
		wrect_t rc = m_hEmpty[m_enArmorType].rect;
		int fullHeight = rc.bottom - rc.top;
		rc.top += fullHeight * ((float)(min(100, m_iBat)) * 0.01f);

		if( rc.bottom > rc.top )
		{
			SPR_Set( m_hEmpty[m_enArmorType].spr, iconR, iconG, iconB );
			SPR_DrawAdditive( 0, x, y + (rc.top - m_hEmpty[m_enArmorType].rect.top), &rc );
		}
	}

	// Draw armor number
	int iconWidth = m_hEmpty[m_enArmorType].rect.Width();
	int numR = r, numG = g, numB = b;
	DrawUtils::ScaleColors( numR, numG, numB, a );
	int numX = x + iconWidth;
	int numEndX = DrawUtils::DrawHudNumber( numX, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, numR, numG, numB );

	// Draw armor bar
	int HealthWidth = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).Width();
	int barX = numEndX + HealthWidth / 2;
	int barWidth = CSO_BAR_WIDTH;
	int barHeight = gHUD.m_iFontHeight;
	if( barHeight < 14 ) barHeight = 14;
	int barY = y + ( gHUD.m_iFontHeight - barHeight ) / 2;

	float ratio = 0.0f;
	if( m_iBat > 0 )
		ratio = (float)m_iBat / 100.0f;
	if( ratio > 1.0f ) ratio = 1.0f;

	int fillWidth = (int)( barWidth * ratio );

	// Background
	FillRGBA( barX, barY, barWidth, barHeight, 30, 30, 50, 180 );
	// Fill
	if( fillWidth > 0 )
		FillRGBA( barX, barY, fillWidth, barHeight, r, g, b, a > 200 ? 200 : a );
	// Border
	FillRGBA( barX, barY, barWidth, 1, r/2, g/2, b/2, a/2 );
	FillRGBA( barX, barY + barHeight - 1, barWidth, 1, r/2, g/2, b/2, a/2 );
	FillRGBA( barX, barY, 1, barHeight, r/2, g/2, b/2, a/2 );
	FillRGBA( barX + barWidth - 1, barY, 1, barHeight, r/2, g/2, b/2, a/2 );
}
