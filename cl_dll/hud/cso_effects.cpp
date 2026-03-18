/*
cso_effects.cpp -- CSO HUD effects: hit marker, damage numbers
*/

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "draw_util.h"

//
// ==================== HIT MARKER ====================
//

int CHudHitMarker::Init( void )
{
	// NOTE: Damage message is already hooked by CHudHealth.
	// Hit marker is triggered from CHudHealth::MsgFunc_Damage instead.
	m_flHitTime = 0;
	m_iHitDamage = 0;
	m_iFlags = HUD_DRAW;
	gHUD.AddHudElem( this );
	return 1;
}

int CHudHitMarker::VidInit( void )
{
	return 1;
}

int CHudHitMarker::Draw( float flTime )
{
	if( !gHUD.m_hudstyle || gHUD.m_hudstyle->value < 1 )
		return 1;

	float elapsed = flTime - m_flHitTime;
	if( elapsed > 0.3f || elapsed < 0.0f )
		return 1;

	// Fade out over 0.3 seconds
	float alpha = 1.0f - ( elapsed / 0.3f );
	int a = (int)( 255 * alpha );

	int cx = ScreenWidth / 2;
	int cy = ScreenHeight / 2;
	int size = 12;
	int gap = 5;
	int thick = 3;

	// Draw X-shaped hit marker (4 lines from center)
	// Top-left to center
	FillRGBA( cx - gap - size, cy - gap - size, size, thick, 255, 255, 255, a );
	FillRGBA( cx - gap - thick, cy - gap - size, thick, size, 255, 255, 255, a );
	// Top-right to center
	FillRGBA( cx + gap, cy - gap - size, size, thick, 255, 255, 255, a );
	FillRGBA( cx + gap, cy - gap - size, thick, size, 255, 255, 255, a );
	// Bottom-left
	FillRGBA( cx - gap - size, cy + gap, size, thick, 255, 255, 255, a );
	FillRGBA( cx - gap - thick, cy + gap, thick, size, 255, 255, 255, a );
	// Bottom-right
	FillRGBA( cx + gap, cy + gap, size, thick, 255, 255, 255, a );
	FillRGBA( cx + gap, cy + gap, thick, size, 255, 255, 255, a );

	return 1;
}

void CHudHitMarker::TriggerHit( int damage )
{
	m_flHitTime = gHUD.m_flTime;
	m_iHitDamage = damage;

	// Also add floating damage number
	if( gHUD.m_hudstyle && gHUD.m_hudstyle->value >= 1 )
	{
		float x = ScreenWidth / 2.0f + (float)( ( rand() % 60 ) - 30 );
		float y = ScreenHeight / 2.0f - 30.0f;
		gHUD.m_DamageNumbers.AddDamage( damage, x, y, false );
	}
}

//
// ==================== DAMAGE NUMBERS ====================
//

int CHudDamageNumbers::Init( void )
{
	memset( m_numbers, 0, sizeof( m_numbers ) );
	m_iNext = 0;
	m_iFlags = HUD_DRAW;
	gHUD.AddHudElem( this );
	return 1;
}

int CHudDamageNumbers::VidInit( void )
{
	return 1;
}

void CHudDamageNumbers::AddDamage( int damage, float x, float y, bool headshot )
{
	m_numbers[m_iNext].damage = damage;
	m_numbers[m_iNext].x = x;
	m_numbers[m_iNext].y = y;
	m_numbers[m_iNext].flTime = gHUD.m_flTime;
	m_numbers[m_iNext].headshot = headshot;
	m_iNext = ( m_iNext + 1 ) % MAX_DAMAGE_NUMBERS;
}

int CHudDamageNumbers::Draw( float flTime )
{
	if( !gHUD.m_hudstyle || gHUD.m_hudstyle->value < 1 )
		return 1;

	for( int i = 0; i < MAX_DAMAGE_NUMBERS; i++ )
	{
		if( m_numbers[i].damage <= 0 )
			continue;

		float elapsed = flTime - m_numbers[i].flTime;
		if( elapsed > 1.5f )
		{
			m_numbers[i].damage = 0;
			continue;
		}

		// Float upward and fade out
		float alpha = 1.0f - ( elapsed / 1.5f );
		float yOffset = elapsed * 60.0f;  // rise 60px per second

		int a = (int)( 255 * alpha );
		int x = (int)m_numbers[i].x;
		int y = (int)( m_numbers[i].y - yOffset );

		int r, g, b;
		if( m_numbers[i].headshot )
		{
			r = 255; g = 50; b = 50;  // red for headshot
		}
		else
		{
			r = 255; g = 200; b = 50;  // yellow-orange
		}

		DrawUtils::ScaleColors( r, g, b, a );

		// Draw minus sign as a small bar
		FillRGBA( x - 8, y + gHUD.m_iFontHeight / 2 - 1, 6, 3, r, g, b, a );

		// Draw damage number using HUD number sprites (bigger than text)
		DrawUtils::DrawHudNumber( x, y, DHN_DRAWZERO, m_numbers[i].damage, r, g, b );
	}

	return 1;
}
