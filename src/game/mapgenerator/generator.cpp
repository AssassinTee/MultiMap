#include "generator.h"

#include <math.h> // ceil
#include <stdlib.h> //rand srand


CMapGenerator::CMapGenerator()
{
	m_Width = 512;
	m_Height = 512;
	AIR = GenerateTile(0);
	SOLID = GenerateTile(1);
}

bool CMapGenerator::GenerateMap(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole, const char* pFilename)
{
	char aBuf[128];
	m_pConsole = pConsole;

	m_pEditor = new CEditorMap2;
	m_pEditor->Init(pStorage, pGraphics, pConsole);
	m_pEditor->LoadDefault();

	m_pAutoMapperTiles = new CTilesetMapper(m_pEditor);
	m_pAutoMapperDoodads = new CDoodadsMapper(m_pEditor);
	LoadAutomapperFiles();

	AddMapres();

	str_format(aBuf, sizeof(aBuf), "Starting background");
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "genmap", aBuf);

	DoBackground();

	str_format(aBuf, sizeof(aBuf), "Starting foreground");
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "genmap", aBuf);

	DoForeground();

	str_format(aBuf, sizeof(aBuf), "Starting gamelayer");
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "genmap", aBuf);

	DoGameLayer();


	str_format(aBuf, sizeof(aBuf), "maps/%s", pFilename);
	bool ret = m_pEditor->Save(aBuf);

	//clean up
	delete m_pEditor;
	delete m_pAutoMapperTiles;
	delete m_pAutoMapperDoodads;

	m_pEditor = 0;
	m_pAutoMapperTiles = 0;
	m_pAutoMapperDoodads = 0;
	return ret;
}

void CMapGenerator::DoBackground()
{
	CEditorMap2::CGroup* Group = &(m_pEditor->m_aGroups[1]);
	str_copy(Group->m_aName, "BackTiles", sizeof(Group->m_aName));

	CEditorMap2::CLayer* grassLayer = &(m_pEditor->m_aLayers[Group->m_apLayerIDs[GRASS_MAIN]]);
	grassLayer->m_ImageID = GRASS_MAIN;

	//Fill All with cavetiles
	FillLayer(grassLayer, GenerateTile(13));

	//Punch some holes in it
	for(int r = 15; r >= 7; --r)
	{
		PunchHoles(grassLayer, r, 10);
	}

	//Automap the F out of it
	RECTi Area;
	Area.h = grassLayer->m_Height;
	Area.w = grassLayer->m_Width;
	Area.x = 0;
	Area.y = 0;
	m_pAutoMapperTiles->Proceed(grassLayer, AUTOMAP_RULES::GRASS_MAIN::CAVE, Area);
}

void CMapGenerator::DoForeground()
{
	m_vPOI.clear();
	CEditorMap2::CGroup* Group = &m_pEditor->m_aGroups[3];
	str_copy(Group->m_aName, "MainTiles", sizeof(Group->m_aName));

	CEditorMap2::CLayer* grassLayer = &(m_pEditor->m_aLayers[Group->m_apLayerIDs[GRASS_MAIN]]);
	grassLayer->m_ImageID = GRASS_MAIN;

	//Fill all with grasstiles
	FillLayer(grassLayer, GenerateTile(1));

	//Do Some Points of interest
	for(int r = 15; r >= 7; --r)
	{
		PunchHoles(grassLayer, r, 10);
	}

	for(int w = 20; w >= 10; --w)
	{
		PunchRectangles(grassLayer, w, 20-w, 10);
	}

	//Add some pregenerated buildings like shops exits, whatever

	//Make Minimal spanning tree between POIs
	ConnectMinimalSpanningTree(grassLayer);

	//Automap the rest
	RECTi Area;
	Area.h = grassLayer->m_Height;
	Area.w = grassLayer->m_Width;
	Area.x = 0;
	Area.y = 0;
	m_pAutoMapperTiles->Proceed(grassLayer, AUTOMAP_RULES::GRASS_MAIN::GRASS, Area);
}

void CMapGenerator::DoGameLayer()
{
	//CEditorMap2::CGroup* GameGroup = &m_pEditor->m_aGroups[2];
	CEditorMap2::CGroup* ForeGroundGroup = &m_pEditor->m_aGroups[3];
	CEditorMap2::CLayer* grassLayer = &(m_pEditor->m_aLayers[ForeGroundGroup->m_apLayerIDs[GRASS_MAIN]]);

	CEditorMap2::CLayer* gamelayer = &(m_pEditor->m_aLayers[m_pEditor->m_GameLayerID]);

	//Make Solid tiles where MainTiles says so
	for(int i = 0; i < m_Width*m_Height; ++i)
	{
		if(grassLayer->m_aTiles[i].m_Index != AIR.m_Index)//Not air means it should be solid!
			gamelayer->m_aTiles[i] = SOLID;
	}

	//Place some junk in POIs

	//Place Spawn points
}

void CMapGenerator::AddMapres()
{
	m_pEditor->AssetsAddAndLoadImage("grass_main.png");//GRASS_MAIN = 0
}

void CMapGenerator::FillLayer(CEditorMap2::CLayer* layer, CTile Tile)
{
	for(int i = 0; i < m_Height*m_Width; ++i)
	{
		layer->m_aTiles[i] = Tile;
	}
}

void CMapGenerator::PunchHoles(CEditorMap2::CLayer* layer, float radius, int num)
{
	for(int i = 0; i < num; ++i)
		PunchHole(layer, radius);
}

void CMapGenerator::PunchHole(CEditorMap2::CLayer* layer, float radius)
{
	int rad = (int)std::ceil(radius);
	int x = rand()%(512-2*rad);
	int y = rand()%(512-2*rad);
	for(int dx = -rad; dx < rad; ++dx)
	{
		for(int dy = -rad; dy < rad; ++dy)
		{
			if(std::sqrt(dx*dx+dy*dy) < radius)
				layer->m_aTiles[(y+rad+dy)*512+x+dx+rad] = AIR;
		}
	}

	//Save Interesting points for gameing
	//Middle
	m_vPOI.emplace_back(vec2(x+rad, y+rad));

	//Circle 'Edges
	m_vHorizontalEdges.emplace_back(vec2(x + rad ,	y));
	m_vHorizontalEdges.emplace_back(vec2(x + rad ,	y + 2*rad-1));
	m_vVerticalEdges.emplace_back(vec2(x, 			y + rad));
	m_vVerticalEdges.emplace_back(vec2(x + 2*rad-1, y + rad));
}

void CMapGenerator::PunchRectangles(CEditorMap2::CLayer* layer, int width, int height, int num)
{
	//int offset = m_vPOI.size();
	//m_vPOI.resize(offset+num);
	for(int i = 0; i < num; ++i)
		PunchRectangle(layer, width, height);
}

void CMapGenerator::PunchRectangle(CEditorMap2::CLayer* layer, int width, int height)
{
	int x = rand()%(512-width-2);
	int y = rand()%(512-height-2);
	for(int dx = -width/2; dx < width/2; ++dx)
	{
		for(int dy = -height/2; dy < height/2; ++dy)
		{
			layer->m_aTiles[(y+height/2+1+dy)*512+x+dx+width/2+1] = AIR;
		}
	}
	//Save interesting points for gaming
	//Middle
	m_vPOI.emplace_back(vec2(x+width/2, y+height/2));

	//Edges
	m_vHorizontalEdges.emplace_back(vec2(x+width/2+1,y+1));
	m_vHorizontalEdges.emplace_back(vec2(x+width/2+1,y+height));
	m_vVerticalEdges.emplace_back(vec2(x+1,y+height/2+1));
	m_vVerticalEdges.emplace_back(vec2(x+width,y+height/2+1));

	//Corners
	m_vCorners.emplace_back(vec2(x+1,y+1));
	m_vCorners.emplace_back(vec2(x+width,y+height));
	m_vCorners.emplace_back(vec2(x+width,y+1));
	m_vCorners.emplace_back(vec2(x+1,y+height));
}

/* badum tss */
void CMapGenerator::PunchLine(CEditorMap2::CLayer* layer, const vec2& p1, const vec2& p2)
{
    int dx = abs(p2.x - p1.x);
    int dy = abs(p2.y - p1.y);

	PunchBresenham(layer, p1.x, p1.y, p2.x, p2.y, dy > dx);
}

void CMapGenerator::PunchBresenham(CEditorMap2::CLayer* layer, int xstart, int ystart, int xend, int yend, bool RectHorizontal)
{
    int x, y, t, dx, dy, incx, incy, pdx, pdy, ddx, ddy, deltaslowdirection, deltafastdirection, err;
    int startwidth = 5;

/* Entfernung in beiden Dimensionen berechnen */
   dx = xend - xstart;
   dy = yend - ystart;

/* Vorzeichen des Inkrements bestimmen */
   incx = sign(dx);
   incy = sign(dy);
   if(dx<0) dx = -dx;
   if(dy<0) dy = -dy;

/* feststellen, welche Entfernung größer ist */
   if (dx>dy)
   {
      /* x ist schnelle Richtung */
      pdx=incx; pdy=0;    /* pd. ist Parallelschritt */
      ddx=incx; ddy=incy; /* dd. ist Diagonalschritt */
      deltaslowdirection =dy;   deltafastdirection =dx;   /* Delta in langsamer Richtung, Delta in schneller Richtung */
   } else
   {
      /* y ist schnelle Richtung */
      pdx=0;    pdy=incy; /* pd. ist Parallelschritt */
      ddx=incx; ddy=incy; /* dd. ist Diagonalschritt */
      deltaslowdirection =dx;   deltafastdirection =dy;   /* Delta in langsamer Richtung, Delta in schneller Richtung */
   }

/* Initialisierungen vor Schleifenbeginn */
   x = xstart;
   y = ystart;
   err = deltafastdirection/2;
   PunchRectangleAt(layer, x,y, startwidth, RectHorizontal);

/* Pixel berechnen */
   for(t=0; t<deltafastdirection; ++t) /* t zaehlt die Pixel, deltafastdirection ist Anzahl der Schritte */
   {
      /* Aktualisierung Fehlerterm */
      err -= deltaslowdirection;
      if(err<0)
      {
          /* Fehlerterm wieder positiv (>=0) machen */
          err += deltafastdirection;
          /* Schritt in langsame Richtung, Diagonalschritt */
          x += ddx;
          y += ddy;
      } else
      {
          /* Schritt in schnelle Richtung, Parallelschritt */
          x += pdx;
          y += pdy;
      }
      int delta = (rand()%3)-1;//-1, 0, 1
      startwidth = max(5, min(7, startwidth+delta));
      PunchRectangleAt(layer, x,y, startwidth, RectHorizontal);
   }
}

void CMapGenerator::PunchRectangleAt(CEditorMap2::CLayer* layer, int x, int y, int length, bool RectHorizontal)
{
	for(int offset = -length/2; offset < length/2; ++offset)
	{
		if(RectHorizontal)
			SetAir(layer, x+offset, y);
		else
			SetAir(layer, x, y+offset);
	}
}

void CMapGenerator::SetAir(CEditorMap2::CLayer* layer, int x, int y)
{
	if(x < 1 || x >= m_Width-1 || y < 1 || y >= m_Height-1)
		return;
	layer->m_aTiles[y*m_Height+x] = AIR;
}

void CMapGenerator::ConnectMinimalSpanningTree(CEditorMap2::CLayer* layer)
{
	std::list<vec2> POI(m_vPOI);
	std::vector<vec2> graph;
	graph.push_back(POI.front());
	POI.pop_front();

	while(POI.size())
	{
		int IndexGraph = -1;
		auto IndexPOI = POI.begin();
		int curmax = m_Width*m_Height;
		for(auto it = POI.begin(); it != POI.end(); ++it)
		{
			for(int j = 0; j < (int)graph.size(); ++j)
			{
				double dist = distance(graph[j], *it);
				if(dist < curmax)
				{
					curmax = dist;
					IndexGraph = j;
					IndexPOI = it;
				}
			}
		}
		if(IndexGraph == -1)//Should never happen
			break;

		/*char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Connecting P(%f, %f) and P(%f, %f)", graph[IndexGraph].x, graph[IndexGraph].y, IndexPOI->x, IndexPOI->y);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "genmap", aBuf);*/

		PunchLine(layer, graph[IndexGraph], *IndexPOI);
		graph.push_back(*IndexPOI);
		m_vPOI.erase(IndexPOI);
	}
}

CTile CMapGenerator::GenerateTile(int Index, int Flags, int Skip, int Padding)
{
	CTile tile;
	tile.m_Index = (unsigned char)Index;
	tile.m_Flags = (unsigned char)Flags;
	tile.m_Skip = (unsigned char)Skip;
	tile.m_Reserved = (unsigned char)Padding;
	return tile;
}

void CMapGenerator::LoadAutomapperFiles()
{
	LoadAutomapperJson("grass_main");
	LoadAutomapperJson("grass_doodads");
}

void CMapGenerator::LoadAutomapperJson(const char* pFilename)
{
	// read file data into buffer
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "editor/automap/%s.json", pFilename);
	IOHANDLE File = m_pEditor->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
		return;

	int FileSize = (int)io_length(File);
	char *pFileData = (char *)mem_alloc(FileSize, 1);
	io_read(File, pFileData, FileSize);
	io_close(File);

	// parse json data
	json_settings JsonSettings;
	mem_zero(&JsonSettings, sizeof(JsonSettings));
	char aError[256];
	json_value *pJsonData = json_parse_ex(&JsonSettings, pFileData, FileSize, aError);
	mem_free(pFileData);

	if(pJsonData == 0)
	{
		m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, aBuf, aError);
		return;
	}

	// generate configurations
	const json_value &rTileset = (*pJsonData)[(const char *)IAutoMapper::GetTypeName(IAutoMapper::TYPE_TILESET)];
	if(rTileset.type == json_array)
	{
		m_pAutoMapperTiles->Load(rTileset);
	}
	else
	{
		const json_value &rDoodads = (*pJsonData)[(const char *)IAutoMapper::GetTypeName(IAutoMapper::TYPE_DOODADS)];
		if(rDoodads.type == json_array)
		{
			m_pAutoMapperDoodads->Load(rDoodads);
		}
	}

	// clean up
	json_value_free(pJsonData);
}
