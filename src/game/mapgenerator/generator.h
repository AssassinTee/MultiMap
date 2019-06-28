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
	bool GenerateMap(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole, const char* pFilename);
private:
	enum
	{
		GRASS_MAIN=0,
	};

	void DoBackground();
	void DoForeground();
	void DoGameLayer();

	void AddMapres();
	void FillLayer(CEditorMap2::CLayer* layer, CTile Tile);
	CTile GenerateTile(int Index, int Flags=0, int Skip=0, int Padding=0);

	//Puncher
	void PunchHoles(CEditorMap2::CLayer* layer, float radius, int num);
	void PunchHole(CEditorMap2::CLayer* layer, float radius);

	void PunchRectangles(CEditorMap2::CLayer* layer, int width, int height, int num);
	void PunchRectangle(CEditorMap2::CLayer* layer, int width, int height);
	void PunchRectangleAt(CEditorMap2::CLayer* layer, int x, int y, int length, bool RectHorizontal);

	void PunchLine(CEditorMap2::CLayer* layer, const vec2& p1, const vec2& p2);
	void PunchBresenham(CEditorMap2::CLayer* layer, int x1, int y1, int x2, int y2, bool RectHorizontal);

	//PunchTile?
	void SetAir(CEditorMap2::CLayer* layer, int x, int y);

	//Connector
	void ConnectMinimalSpanningTree(CEditorMap2::CLayer* layer);

	//AutoMapper
	void LoadAutomapperJson(const char* pFilename);
	void LoadAutomapperFiles();


	CEditorMap2* m_pEditor;
	IConsole* m_pConsole;
	IAutoMapper* m_pAutoMapperTiles;
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
	std::list<vec2> m_vHorizontalEdges;
	std::list<vec2> m_vVerticalEdges;

	CTile AIR;
	CTile SOLID;
};

#endif
