//-----------------------------------------------------------------------------
// scene.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgScene.h"
#include "AtgResourceDatabase.h"

namespace ATG
{

CONST StringID Scene::TypeID( L"Scene" );

//-----------------------------------------------------------------------------
// Name: Scene::Scene
//-----------------------------------------------------------------------------
Scene::Scene()
{
    m_pResourceDatabase = NULL;
    FXLCreateEffectPool( &m_pGlobalParameterPool );
    m_pResourceDatabase = new ResourceDatabase();
}

Scene::~Scene()
{
    m_pGlobalParameterPool->Release();
    m_pGlobalParameterPool = NULL;
    delete m_pResourceDatabase;
    m_pResourceDatabase = NULL;
}

} // namespace ATG
