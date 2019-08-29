#ifndef MAPGENERATOR_GENERATOR_H
#define MAPGENERATOR_GENERATOR_H

#include <stdint.h>
#include <string.h> // memset
#include <vector>
#include <list>
#include <base/system.h>
#include <base/tl/array.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/editor.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

#include <game/mapitems.h>
#include "auto_map.h"
#include "mapcontents.h"

class IGraphics;
class IConsole;

class CMapGenerator
{
public:
	CMapGenerator();
	bool GenerateMap(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole, const char* pFilename, const char* pType);
	static CTile GenerateTile(int Index, int Flags=0, int Skip=0, int Padding=0);

private:
	enum Tile
	{
		AIR=0,
		SOLID,
		DEATH,
		UNHOOK,
		SPAWN_DEFAULT=192,
		SPAWN_RED,
		SPAWN_BLUE,
	};

	enum Group
	{
		BG_SKY=0,
		BG_TILE,
		BG_TILE_2,
		MG_TILE,
		MG_DOODADS,
		GAME_LAYER,
		FG_TILE
	};

	void DoBackground(const char* pType);
	void DoMidground(const char* pType);
	void DoForeground();
	void DoGameLayer();
	void DoDoodadsLayer(const char* pType);
	void DoBorderCorrection();


	void AddMapres(const char* pType);
	void FillLayer(CEditorMap2::CLayer* layer, CTile Tile);

	//Puncher
	void PunchHoles(CEditorMap2::CLayer* layer, float radius, int num, bool UsePOI = false);
	void PunchHole(CEditorMap2::CLayer* layer, float radius, bool UsePOI = false);

	void PunchRectangles(CEditorMap2::CLayer* layer, int width, int height, int num, bool UsePOI = false);
	void PunchRectangle(CEditorMap2::CLayer* layer, int width, int height, bool UsePOI = false);
	void PunchRectangleAt(CEditorMap2::CLayer* layer, int x, int y, int length, bool RectHorizontal);

	void PunchLine(CEditorMap2::CLayer* layer, const vec2& p1, const vec2& p2);
	void PunchBresenham(CEditorMap2::CLayer* layer, int x1, int y1, int x2, int y2, bool RectHorizontal);

	//PunchTile?
	void SetTile(CEditorMap2::CLayer* layer, int x, int y, int Index=0, bool OnlyAir=false);

	//Connector
	void ConnectMinimalSpanningTree(CEditorMap2::CLayer* layer);

	//AutoMapper
	void LoadAutomapperJson(const char* pFilename, IAutoMapper* pAutomapper);
	void LoadAutomapperFiles(const char* pType=0);

	//GameLayer
	void PlaceGameItems();
	void PlaceSpawns();

	//Corrections
	void DoGrassDoodads();
	void DoJungleDoodads();
	void DoDesertDoodads();
	void DoWinterDoodads();

	//More corrections
	void CleanLayer(CEditorMap2::CLayer* layer, int Solid);

	CEditorMap2* m_pEditor;
	IConsole* m_pConsole;
	IAutoMapper* m_pAutoMapperTiles;
	IAutoMapper* m_pAutoMapperTiles2;
	IAutoMapper* m_pAutoMapperDoodads;

	struct AUTOMAP_RULES
	{
		enum GRASS_MAIN {
			GRASS=0,
			CAVE,
		};
	};

	int m_Width;
	int m_Height;

	std::list<vec2> m_vPOI;//more performance
	std::list<vec2> m_vCorners;
	std::list<vec2> m_vHorizontalTopEdges;
	std::list<vec2> m_vHorizontalBottomEdges;
	std::list<vec2> m_vVerticalEdges;
};

#endif
