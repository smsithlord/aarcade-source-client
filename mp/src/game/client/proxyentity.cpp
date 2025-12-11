//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "proxyentity.h"
#include "iclientrenderable.h"
#include "toolframework_client.h"

#include "aarcade/client/c_anarchymanager.h"	// Added for Anarchy Arcade

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Cleanup
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::Release( void )
{ 
	delete this; 
}

//-----------------------------------------------------------------------------
// Helper class to deal with floating point inputs
//-----------------------------------------------------------------------------
void CEntityMaterialProxy::OnBind( void *pRenderable )
{
	if (g_pAnarchyManager->IsPaused() || g_pAnarchyManager->IsInSourceGame())	// Added for Anarchy Arcade
		return;

	if( !pRenderable )
		return;

	IClientRenderable *pRend = ( IClientRenderable* )pRenderable;
	C_BaseEntity *pEnt = pRend->GetIClientUnknown()->GetBaseEntity();
	if ( pEnt )
	{
		OnBind( pEnt );
		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
}
