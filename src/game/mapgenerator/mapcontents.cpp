#include "generator.h"

#include <zlib.h>// crc32
#include <math.h> // ceil
#include <stdlib.h> //rand srand

// hash
inline u32 fnv1a32(const void* data, u32 dataSize)
{
	u32 hash = 2166136261;
	for(u32 i = 0; i < dataSize; ++i)
		hash = (hash ^ ((const char*)data)[i]) * 16777619;
	return hash;
}

void CEditorMap2::Init(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole)
{
	m_pGraphics = pGraphics;
	m_pStorage = pStorage;
	m_pConsole = pConsole;

	m_aEnvPoints.hint_size(1024);
	m_aLayers.hint_size(32);
	m_aGroups.hint_size(16);
	m_aEnvelopes.hint_size(32);
}

bool CEditorMap2::Save(const char* pFileName)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "saving to '%s'...", pFileName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", aBuf);
	CDataFileWriter File;
	if(!File.Open(m_pStorage, pFileName))
	{
		str_format(aBuf, sizeof(aBuf), "failed to open file '%s'...", pFileName);
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "editor", aBuf);
		return 0;
	}

	// save version
	{
		CMapItemVersion Item;
		Item.m_Version = CMapItemVersion::CURRENT_VERSION;
		File.AddItem(MAPITEMTYPE_VERSION, 0, sizeof(Item), &Item);
	}

	// save map info
	{
		CMapItemInfo Item;
		Item.m_Version = CMapItemInfo::CURRENT_VERSION;

		// if(m_MapInfo.m_aAuthor[0])
			// Item.m_Author = df.AddData(str_length(m_MapInfo.m_aAuthor)+1, m_MapInfo.m_aAuthor);
		// else
			Item.m_Author = -1;
		// if(m_MapInfo.m_aVersion[0])
			// Item.m_MapVersion = df.AddData(str_length(m_MapInfo.m_aVersion)+1, m_MapInfo.m_aVersion);
		// else
			Item.m_MapVersion = -1;
		// if(m_MapInfo.m_aCredits[0])
			// Item.m_Credits = df.AddData(str_length(m_MapInfo.m_aCredits)+1, m_MapInfo.m_aCredits);
		// else
			Item.m_Credits = -1;
		// if(m_MapInfo.m_aLicense[0])
			// Item.m_License = df.AddData(str_length(m_MapInfo.m_aLicense)+1, m_MapInfo.m_aLicense);
		// else
			Item.m_License = -1;

		File.AddItem(MAPITEMTYPE_INFO, 0, sizeof(Item), &Item);
	}

	// save images
	for(int i = 0; i < m_Assets.m_ImageCount; i++)
	{
		// old code: analyse the image for when saving (should be done when we load the image)
		// pImg->AnalyseTileFlags();

		CImageName ImgName = m_Assets.m_aImageNames[i];
		CImageInfo ImgInfo = m_Assets.m_aTextureInfos[i];
		const bool IsExternal = m_Assets.m_aImageEmbeddedCrc[i] == 0; // external

		CMapItemImage Item;
		Item.m_Version = CMapItemImage::CURRENT_VERSION;

		Item.m_Width = ImgInfo.m_Width;
		Item.m_Height = ImgInfo.m_Height;
		Item.m_External = IsExternal;
		Item.m_ImageName = File.AddData(str_length(ImgName.m_Buff)+1, ImgName.m_Buff);
		if(IsExternal)
			Item.m_ImageData = -1;
		else
		{
			int PixelSize = ImgInfo.m_Format == CImageInfo::FORMAT_RGB ? 3 : 4;
			Item.m_ImageData = File.AddData(Item.m_Width*Item.m_Height*PixelSize, ImgInfo.m_pData);
		}
		Item.m_Format = ImgInfo.m_Format;
		File.AddItem(MAPITEMTYPE_IMAGE, i, sizeof(Item), &Item);
	}

	// save layers
	for(int li = 0, gi = 0; gi < m_aGroups.size(); gi++)
	{
		const CGroup& Group = m_aGroups[gi];
		//ed_dbg("Group#%d NumLayers=%d Offset=(%d, %d)", gi, Group.m_LayerCount, Group.m_OffsetX, Group.m_OffsetY);
		// old feature
		// if(!Group->m_SaveToMap)
		// 	continue;

		CMapItemGroup GItem;
		GItem.m_Version = CMapItemGroup::CURRENT_VERSION;

		GItem.m_ParallaxX = Group.m_ParallaxX;
		GItem.m_ParallaxY = Group.m_ParallaxY;
		GItem.m_OffsetX = Group.m_OffsetX;
		GItem.m_OffsetY = Group.m_OffsetY;
		GItem.m_UseClipping = Group.m_UseClipping;
		GItem.m_ClipX = Group.m_ClipX;
		GItem.m_ClipY = Group.m_ClipY;
		// GItem.m_ClipW = Group.m_ClipW;
		// GItem.m_ClipH = Group.m_ClipH;
		GItem.m_ClipW = 0;
		GItem.m_ClipH = 0;
		GItem.m_StartLayer = li;
		GItem.m_NumLayers = 0;

		// save group name
		StrToInts(GItem.m_aName, sizeof(GItem.m_aName)/sizeof(int), Group.m_aName);


		for(; li < GItem.m_StartLayer + Group.m_LayerCount; li++)
		{

			// old feature
			// if(!Group->m_lLayers[l]->m_SaveToMap)
			// 	continue;

			if(m_aLayers[li].IsTileLayer())
			{
				// Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving tiles layer");
				CLayer& Layer = m_aLayers[li];
				// Layer.PrepareForSave();

				//ed_dbg("  Group#%d Layer=%d (w=%d, h=%d)", gi, li, Layer.m_Width, Layer.m_Height);

				CMapItemLayerTilemap Item;
				Item.m_Version = CMapItemLayerTilemap::CURRENT_VERSION;

				// Item.m_Layer.m_Flags = Layer.m_Flags;
				Item.m_Layer.m_Flags = (int)Layer.m_HighDetail*LAYERFLAG_DETAIL; // LAYERFLAG_DETAIL or 0
				Item.m_Layer.m_Type = Layer.m_Type;

				Item.m_Color.r = Layer.m_Color.r*255;
				Item.m_Color.g = Layer.m_Color.g*255;
				Item.m_Color.b = Layer.m_Color.b*255;
				Item.m_Color.a = Layer.m_Color.a*255;
				Item.m_ColorEnv = Layer.m_ColorEnvelopeID;
				Item.m_ColorEnvOffset = Layer.m_ColorEnvOffset;

				Item.m_Width = Layer.m_Width;
				Item.m_Height = Layer.m_Height;

				if(m_GameGroupID == gi && m_GameLayerID == li)
				{
					Item.m_Flags = TILESLAYERFLAG_GAME;
					//ed_dbg("Game layer reached");
				}
				else
					Item.m_Flags = 0;
				Item.m_Image = Layer.m_ImageID;
				// Item.m_Data = File.AddData(Layer.m_SaveTilesSize, Layer.m_pSaveTiles);
				Item.m_Data = File.AddData(Layer.m_aTiles.size()*sizeof(CTile), Layer.m_aTiles.base_ptr());

				// save layer name
				StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), Layer.m_aName);

				File.AddItem(MAPITEMTYPE_LAYER, li, sizeof(Item), &Item);

				GItem.m_NumLayers++;
			}
			else if(m_aLayers[li].IsQuadLayer())
			{
				// Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving quads layer");
				CLayer& LayerQuad = m_aLayers[li];

				//ed_dbg("  Group#%d Quad=%d (w=%d, h=%d)", gi, li, LayerQuad.m_Width, LayerQuad.m_Height);

				if(LayerQuad.m_aQuads.size())
				{
					CMapItemLayerQuads Item;
					Item.m_Version = CMapItemLayerQuads::CURRENT_VERSION;
					Item.m_Layer.m_Flags = (int)LayerQuad.m_HighDetail*LAYERFLAG_DETAIL; // LAYERFLAG_DETAIL or 0
					Item.m_Layer.m_Type = LayerQuad.m_Type;
					Item.m_Image = LayerQuad.m_ImageID;

					// add the data
					Item.m_NumQuads = LayerQuad.m_aQuads.size();
					Item.m_Data = File.AddDataSwapped(LayerQuad.m_aQuads.size()*sizeof(CQuad), LayerQuad.m_aQuads.base_ptr());

					// save layer name
					StrToInts(Item.m_aName, sizeof(Item.m_aName)/sizeof(int), LayerQuad.m_aName);

					File.AddItem(MAPITEMTYPE_LAYER, li, sizeof(Item), &Item);

					GItem.m_NumLayers++;
				}
			}
		}

		File.AddItem(MAPITEMTYPE_GROUP, gi, sizeof(GItem), &GItem);
	}

	// check for bezier curve envelopes, otherwise use older, smaller envelope points
	int Version = CMapItemEnvelope_v2::CURRENT_VERSION;
	int Size = sizeof(CEnvPoint_v1);
	// for(int e = 0; e < m_aEnvelopes.Count(); e++)
	{
		for(int p = 0; p < m_aEnvPoints.size(); p++)
		{
			if(m_aEnvPoints[p].m_Curvetype == CURVETYPE_BEZIER)
			{
				Version = CMapItemEnvelope::CURRENT_VERSION;
				Size = sizeof(CEnvPoint);
				break;
			}
		}
	}

	// save envelopes
	int PointCount = 0;
	for(int e = 0; e < m_aEnvelopes.size(); e++)
	{
		CMapItemEnvelope Item;
		Item.m_Version = Version;
		Item.m_Channels = m_aEnvelopes[e].m_Channels;
		Item.m_StartPoint = PointCount;
		Item.m_NumPoints = m_aEnvelopes[e].m_NumPoints;
		Item.m_Synchronized = m_aEnvelopes[e].m_Synchronized;
		mem_copy(Item.m_aName, m_aEnvelopes[e].m_aName, sizeof(m_aEnvelopes[e].m_aName)*sizeof(int));

		File.AddItem(MAPITEMTYPE_ENVELOPE, e, sizeof(Item), &Item);
		PointCount += Item.m_NumPoints;
	}

	// save points
	int TotalSize = Size * PointCount;
	unsigned char *pPoints = (unsigned char *)mem_alloc(TotalSize, 1);
	int Offset = 0;
	// for(int e = 0; e < m_aEnvelopes.Count(); e++)
	{
		for(int p = 0; p < m_aEnvPoints.size(); p++)
		{
			mem_copy(pPoints + Offset, &(m_aEnvPoints[p]), Size);
			Offset += Size;
		}
	}

	File.AddItem(MAPITEMTYPE_ENVPOINTS, 0, TotalSize, pPoints);
	mem_free(pPoints);

	// finish the data file
	File.Finish();
	Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "editor", "saving done");

	return true;
}

/*bool CEditorMap2::Load(const char* pFileName)
{
	CDataFileReader File;
	if(!File.Open(m_pStorage, pFileName, IStorage::TYPE_ALL))
		return false;
	// check version
	CMapItemVersion *pItem = (CMapItemVersion *)File.FindItem(MAPITEMTYPE_VERSION, 0);
	if(!pItem || pItem->m_Version != CMapItemVersion::CURRENT_VERSION)
		return false;

	const int FilenameLen = str_length(pFileName);
	mem_copy(m_aPath, pFileName, FilenameLen);
	m_aPath[FilenameLen] = 0;

	Clear();
	AssetsClearEmbeddedFiles();

	int GroupsStart, GroupsNum;
	int LayersStart, LayersNum;
	File.GetType(MAPITEMTYPE_GROUP, &GroupsStart, &GroupsNum);
	File.GetType(MAPITEMTYPE_LAYER, &LayersStart, &LayersNum);

	ed_dbg("GroupsStart=%d GroupsNum=%d LayersStart=%d LayersNum=%d",
			GroupsStart, GroupsNum, LayersStart, LayersNum);

	for(int gi = 0; gi < GroupsNum; gi++)
	{
		CMapItemGroup* pGroup = (CMapItemGroup*)File.GetItem(GroupsStart+gi, 0, 0);
		ed_dbg("Group#%d NumLayers=%d Offset=(%d, %d)", gi, pGroup->m_NumLayers,
				pGroup->m_OffsetX, pGroup->m_OffsetY);
		const int GroupLayerCount = pGroup->m_NumLayers;
		const int GroupLayerStart = pGroup->m_StartLayer;
		CEditorMap2::CGroup Group;
		IntsToStr(pGroup->m_aName, ARR_COUNT(pGroup->m_aName), Group.m_aName);
		Group.m_ParallaxX = pGroup->m_ParallaxX;
		Group.m_ParallaxY = pGroup->m_ParallaxY;
		Group.m_OffsetX = pGroup->m_OffsetX;
		Group.m_OffsetY = pGroup->m_OffsetY;
		Group.m_ClipX = pGroup->m_ClipX;
		Group.m_ClipY = pGroup->m_ClipY;
		Group.m_ClipWidth = pGroup->m_ClipW;
		Group.m_ClipHeight = pGroup->m_ClipH;
		Group.m_UseClipping = pGroup->m_UseClipping;
		Group.m_LayerCount = 0;

		for(int li = 0; li < GroupLayerCount; li++)
		{
			CMapItemLayer* pLayer = (CMapItemLayer*)File.GetItem(LayersStart+GroupLayerStart+li, 0, 0);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				const CMapItemLayerTilemap& Tilemap = *(CMapItemLayerTilemap*)pLayer;
				ed_dbg("Group#%d Layer=%d (w=%d, h=%d)", gi, li,
						Tilemap.m_Width, Tilemap.m_Height);

				m_MapMaxWidth = max(m_MapMaxWidth, Tilemap.m_Width);
				m_MapMaxHeight = max(m_MapMaxHeight, Tilemap.m_Height);

				CLayer LayerTile;
				if(Tilemap.m_Version >= 3)
					IntsToStr(Tilemap.m_aName, ARR_COUNT(Tilemap.m_aName), LayerTile.m_aName);
				else
					LayerTile.m_aName[0] = 0;

				LayerTile.m_Type = LAYERTYPE_TILES;
				LayerTile.m_ImageID = Tilemap.m_Image;
				LayerTile.m_HighDetail = Tilemap.m_Layer.m_Flags & LAYERFLAG_DETAIL;
				LayerTile.m_Width = Tilemap.m_Width;
				LayerTile.m_Height = Tilemap.m_Height;
				LayerTile.m_ColorEnvelopeID = Tilemap.m_ColorEnv;
				LayerTile.m_ColorEnvOffset = Tilemap.m_ColorEnvOffset;
				LayerTile.m_Color = vec4(Tilemap.m_Color.r/255.f, Tilemap.m_Color.g/255.f,
										 Tilemap.m_Color.b/255.f, Tilemap.m_Color.a/255.f);
				LayerTile.m_aTiles.clear();

				CTile *pTiles = (CTile *)File.GetData(Tilemap.m_Data);
				int TileCount = Tilemap.m_Width*Tilemap.m_Height;
				for(int ti = 0; ti < TileCount; ti++)
				{
					CTile Tile = pTiles[ti];
					const int Skips = Tile.m_Skip;
					Tile.m_Skip = 0;

					for(int s = 0; s < Skips; s++)
						LayerTile.m_aTiles.add(Tile);
					LayerTile.m_aTiles.add(Tile);

					TileCount -= Skips;
				}

				const int LayerID = m_aLayers.size();
				m_aLayers.add(LayerTile);

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;

				if(Tilemap.m_Flags&TILESLAYERFLAG_GAME)
				{
					m_GameLayerID = LayerID;
					m_GameGroupID = m_aGroups.size();
				}
			}
			else if(pLayer->m_Type == LAYERTYPE_QUADS)
			{
				const CMapItemLayerQuads& ItemQuadLayer = *(CMapItemLayerQuads*)pLayer;

				CLayer LayerQuad;
				if(ItemQuadLayer.m_Version >= 3)
					IntsToStr(ItemQuadLayer.m_aName, ARR_COUNT(ItemQuadLayer.m_aName), LayerQuad.m_aName);
				else
					LayerQuad.m_aName[0] = 0;

				LayerQuad.m_Type = LAYERTYPE_QUADS;
				LayerQuad.m_ImageID = ItemQuadLayer.m_Image;
				LayerQuad.m_HighDetail = ItemQuadLayer.m_Layer.m_Flags & LAYERFLAG_DETAIL;

				CQuad *pQuads = (CQuad *)File.GetData(ItemQuadLayer.m_Data);
				LayerQuad.m_aQuads.clear();
				LayerQuad.m_aQuads.add(pQuads, ItemQuadLayer.m_NumQuads);

				const int LayerID = m_aLayers.size();
				m_aLayers.add(LayerQuad);

				if(Group.m_LayerCount < MAX_GROUP_LAYERS)
					Group.m_apLayerIDs[Group.m_LayerCount++] = LayerID;
			}
		}

		m_aGroups.add(Group);
	}

	dbg_assert(m_GameLayerID >= 0, "Game layer not found");
	dbg_assert(m_GameGroupID >= 0, "Game group not found");

	// load envelopes
	int EnvelopeStart, EnvelopeCount;
	int EnvPointStart, EnvPointCount;
	File.GetType(MAPITEMTYPE_ENVELOPE, &EnvelopeStart, &EnvelopeCount);
	File.GetType(MAPITEMTYPE_ENVPOINTS, &EnvPointStart, &EnvPointCount);
	// FIXME: EnvPointCount is always 1?

	ed_dbg("EnvelopeStart=%d EnvelopeCount=%d EnvPointStart=%d EnvPointCount=%d",
			EnvelopeStart, EnvelopeCount, EnvPointStart, EnvPointCount);

	CEnvPoint* pEnvPoints = (CEnvPoint*)File.GetItem(EnvPointStart, 0, 0);

	// envelopes
	for(int ei = 0; ei < EnvelopeCount; ei++)
	{
		CMapItemEnvelope *pItem = (CMapItemEnvelope *)File.GetItem(EnvelopeStart+ei, 0, 0);
		const CMapItemEnvelope Env = *pItem;
		m_aEnvelopes.add(Env);

		if(Env.m_Version >= 3)
		{
			m_aEnvPoints.add(&pEnvPoints[Env.m_StartPoint], Env.m_NumPoints);
		}
		else
		{
			// backwards compatibility
			for(int i = 0; i < Env.m_NumPoints; i++)
			{
				// convert CEnvPoint_v1 -> CEnvPoint
				CEnvPoint_v1 *pEnvPoint_v1 = &((CEnvPoint_v1 *)pEnvPoints)[i + Env.m_StartPoint];
				CEnvPoint Point;
				mem_zero(&Point, sizeof(Point));
				mem_copy(&Point, pEnvPoint_v1, sizeof(CEnvPoint_v1));
				m_aEnvPoints.add(Point);
			}
		}
	}

	// load textures
	int ImagesStart, ImagesCount;
	File.GetType(MAPITEMTYPE_IMAGE, &ImagesStart, &ImagesCount);
	CImageName aImageName[MAX_IMAGES];
	CImageInfo aImageInfo[MAX_IMAGES];
	u32 aImageEmbeddedCrc[MAX_IMAGES];

	for(int i = 0; i < ImagesCount && i < MAX_IMAGES; i++)
	{
		mem_zero(&aImageName[i], sizeof(aImageName[i]));
		CMapItemImage *pImg = (CMapItemImage *)File.GetItem(ImagesStart+i, 0, 0);
		const char *pImgName = (char *)File.GetData(pImg->m_ImageName);
		mem_copy(&aImageName[i], pImgName, min((u64)str_length(pImgName), sizeof(aImageName[i].m_Buff)-1));
		aImageInfo[i].m_pData = 0x0;
		aImageInfo[i].m_Width = pImg->m_Width;
		aImageInfo[i].m_Height = pImg->m_Height;

		if(pImg->m_External)
		{
			aImageEmbeddedCrc[i] = 0; // external
			aImageInfo[i].m_Format = CImageInfo::FORMAT_AUTO;
		}
		else
		{
			unsigned long DataSize = 0;
			void *pData = File.GetData(pImg->m_ImageData, &DataSize);

			// save embedded data
			aImageEmbeddedCrc[i] = AssetsAddEmbeddedData(pData, DataSize);
			aImageInfo[i].m_Format = pImg->m_Version == 1 ? CImageInfo::FORMAT_RGBA : pImg->m_Format;
		}
	}

	AssetsClearAndSetImages(aImageName, aImageInfo, aImageEmbeddedCrc, ImagesCount);
	AssetsLoadMissingAutomapFiles();

	return true;
}*/

void CEditorMap2::LoadDefault()
{
	Clear();

	//Add Background
	CGroup BgGroup;
	BgGroup.m_OffsetX = 0;
	BgGroup.m_OffsetY = 0;
	BgGroup.m_ParallaxX = 0;
	BgGroup.m_ParallaxY = 0;

	CLayer& BgQuadLayer = NewQuadLayer();

	CQuad SkyQuad;
	SkyQuad.m_ColorEnv = -1;
	SkyQuad.m_PosEnv = -1;
	const int Width = i2fx(800);
	const int Height = i2fx(600);
	SkyQuad.m_aPoints[0].x = SkyQuad.m_aPoints[2].x = -Width;
	SkyQuad.m_aPoints[1].x = SkyQuad.m_aPoints[3].x = Width;
	SkyQuad.m_aPoints[0].y = SkyQuad.m_aPoints[1].y = -Height;
	SkyQuad.m_aPoints[2].y = SkyQuad.m_aPoints[3].y = Height;
	SkyQuad.m_aPoints[4].x = SkyQuad.m_aPoints[4].y = 0;
	SkyQuad.m_aColors[0].r = SkyQuad.m_aColors[1].r = 94;
	SkyQuad.m_aColors[0].g = SkyQuad.m_aColors[1].g = 132;
	SkyQuad.m_aColors[0].b = SkyQuad.m_aColors[1].b = 174;
	SkyQuad.m_aColors[2].r = SkyQuad.m_aColors[3].r = 204;
	SkyQuad.m_aColors[2].g = SkyQuad.m_aColors[3].g = 232;
	SkyQuad.m_aColors[2].b = SkyQuad.m_aColors[3].b = 255;
	SkyQuad.m_aColors[0].a = SkyQuad.m_aColors[1].a = 255;
	SkyQuad.m_aColors[2].a = SkyQuad.m_aColors[3].a = 255;
	BgQuadLayer.m_aQuads.add(SkyQuad);

	BgGroup.m_apLayerIDs[BgGroup.m_LayerCount++] = m_aLayers.size()-1;
	m_aGroups.add(BgGroup);


	//Add Middle Ground
	CGroup MgGroup;
	MgGroup.m_OffsetX = 0;
	MgGroup.m_OffsetY = 0;
	MgGroup.m_ParallaxX = 90;
	MgGroup.m_ParallaxY = 90;

	/*CLayer& Gamelayer =*/ NewTileLayer(512, 512);

	MgGroup.m_apLayerIDs[MgGroup.m_LayerCount++] = m_aLayers.size()-1;
	m_aGroups.add(MgGroup);

	//Add Doodads Ground
	CGroup DoGroup;
	DoGroup.m_OffsetX = 0;
	DoGroup.m_OffsetY = 0;
	DoGroup.m_ParallaxX = 100;
	DoGroup.m_ParallaxY = 100;

	/*CLayer& Gamelayer =*/ NewTileLayer(512, 512);

	DoGroup.m_apLayerIDs[DoGroup.m_LayerCount++] = m_aLayers.size()-1;

	/*CLayer& Gamelayer =*/ NewTileLayer(512, 512);

	DoGroup.m_apLayerIDs[DoGroup.m_LayerCount++] = m_aLayers.size()-1;
	m_aGroups.add(DoGroup);

	//Add gamelayer
	CGroup GameGroup;
	GameGroup.m_OffsetX = 0;
	GameGroup.m_OffsetY = 0;
	GameGroup.m_ParallaxX = 100;
	GameGroup.m_ParallaxY = 100;

	/*CLayer& Gamelayer =*/ NewTileLayer(512, 512);

	GameGroup.m_apLayerIDs[GameGroup.m_LayerCount++] = m_aLayers.size()-1;
	m_aGroups.add(GameGroup);

	m_GameGroupID = m_aGroups.size()-1;
	m_GameLayerID = GameGroup.m_apLayerIDs[0];

	//Add foreground
	CGroup ForeGroundGroup;
	ForeGroundGroup.m_OffsetX = 0;
	ForeGroundGroup.m_OffsetY = 0;
	ForeGroundGroup.m_ParallaxX = 100;
	ForeGroundGroup.m_ParallaxY = 100;

	/*CLayer& Gamelayer =*/ NewTileLayer(512, 512);

	ForeGroundGroup.m_apLayerIDs[ForeGroundGroup.m_LayerCount++] = m_aLayers.size()-1;

	m_aGroups.add(ForeGroundGroup);
}

void CEditorMap2::Clear()
{
	m_GameLayerID = -1;
	m_GameGroupID = -1;
	m_MapMaxWidth = 0;
	m_MapMaxHeight = 0;

	// release tiles and quads
	const int LayerCount = m_aLayers.size();
	for(int li = 0; li < LayerCount; li++)
	{
		if(m_aLayers[li].IsTileLayer())
			m_aLayers[li].m_aTiles.clear();
		else
			m_aLayers[li].m_aQuads.clear();
	}

	m_aLayers.clear();
	m_aGroups.clear();
	m_aEnvelopes.clear();
	m_aEnvPoints.clear();
}

void CEditorMap2::AssetsClearAndSetImages(CEditorMap2::CImageName* aName, CImageInfo* aInfo,
	u32* aImageEmbeddedCrc, int ImageCount)
{
	dbg_assert(ImageCount <= MAX_IMAGES, "ImageCount > MAX_IMAGES");

	IGraphics::CTextureHandle aKeepTexHandle[MAX_IMAGES];
	for(int ni = 0; ni < ImageCount; ni++)
		aKeepTexHandle[ni].Invalidate();

	for(int i = 0; i < m_Assets.m_ImageCount; i++)
	{
		bool Found = false;
		for(int ni = 0; ni < ImageCount; ni++)
		{
			if(m_Assets.m_aImageEmbeddedCrc[i] == aImageEmbeddedCrc[ni] &&
			   mem_comp(m_Assets.m_aImageNames[i].m_Buff, aName[ni].m_Buff, sizeof(aName[ni].m_Buff)) == 0)
			{
				aKeepTexHandle[ni] = m_Assets.m_aTextureHandle[i];
				Found = true;
				break;
			}
		}

		if(!Found)
			m_pGraphics->UnloadTexture(&m_Assets.m_aTextureHandle[i]);
	}

	m_Assets.m_ImageCount = ImageCount;

	memmove(m_Assets.m_aImageNames, aName, sizeof(aName[0]) * ImageCount);
	memmove(m_Assets.m_aTextureInfos, aInfo, sizeof(aInfo[0]) * ImageCount);
	memmove(m_Assets.m_aImageEmbeddedCrc, aImageEmbeddedCrc, sizeof(aImageEmbeddedCrc[0]) * ImageCount);

	for(int i = 0; i < ImageCount && i < MAX_IMAGES; i++)
	{
		const int TextureFlags = IGraphics::TEXLOAD_MULTI_DIMENSION;

		if(aKeepTexHandle[i].IsValid())
		{
			//ed_dbg("%s kept", aName[i].m_Buff);
			m_Assets.m_aTextureHandle[i] = aKeepTexHandle[i];
			continue;
		}

		if(m_Assets.m_aImageEmbeddedCrc[i] == 0) // external
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "mapres/%s.png", aName[i].m_Buff);
			m_Assets.m_aTextureHandle[i] = m_pGraphics->LoadTexture(aBuf, IStorage::TYPE_ALL,
				CImageInfo::FORMAT_AUTO, TextureFlags);
			m_Assets.m_aTextureInfos[i].m_Format = CImageInfo::FORMAT_AUTO;
			//ed_dbg("%s loaded", aBuf);
		}
		else
		{
			const u32 Crc = aImageEmbeddedCrc[i];
			const int EmbeddedFileCount = m_Assets.m_EmbeddedFileCount;
			CEmbeddedFile* pFile = 0x0;
			for(int f = 0; f < EmbeddedFileCount; f++)
			{
				if(m_Assets.m_aEmbeddedFile[f].m_Crc == Crc)
				{
					pFile = &m_Assets.m_aEmbeddedFile[f];
					break;
				}
			}

			// embedded file should be found
			dbg_assert(pFile != 0x0, "Embedded file not found");

			m_Assets.m_aTextureHandle[i] = m_pGraphics->LoadTextureRaw(aInfo[i].m_Width, aInfo[i].m_Height,
				aInfo[i].m_Format, pFile->m_pData, CImageInfo::FORMAT_RGBA, TextureFlags);
		}

		m_Assets.m_aImageNameHash[i] = fnv1a32(&m_Assets.m_aImageNames[i],
			sizeof(m_Assets.m_aImageNames[i]));

		dbg_assert(m_Assets.m_aTextureHandle[i].IsValid(), "Invalid texture");
	}
}

u32 CEditorMap2::AssetsAddEmbeddedData(void* pData, u64 DataSize)
{
	// 10 Mb limit
	dbg_assert(DataSize < (u64)(1024*1024*10), "DataSize is invalid");
	void* pDataCopy = mem_alloc(DataSize, 0);
	memmove(pDataCopy, pData, DataSize);

	CEmbeddedFile EmbeddedFile;
	u32 Crc = crc32(0L, 0x0, 0);
	Crc = crc32(Crc, (u8*)pData, DataSize);
	EmbeddedFile.m_Crc = Crc;
	EmbeddedFile.m_Type = 0;
	EmbeddedFile.m_pData = pDataCopy;
	m_Assets.m_aEmbeddedFile[m_Assets.m_EmbeddedFileCount++] = EmbeddedFile;
	return Crc;
}

void CEditorMap2::AssetsClearEmbeddedFiles()
{
	for(int i = 0; i < m_Assets.m_EmbeddedFileCount; i++)
	{
		if(m_Assets.m_aEmbeddedFile[i].m_pData)
		{
			free(m_Assets.m_aEmbeddedFile[i].m_pData);
			m_Assets.m_aEmbeddedFile[i].m_pData = 0x0;
		}
	}
	m_Assets.m_EmbeddedFileCount = 0;
}

bool CEditorMap2::AssetsAddAndLoadImage(const char* pFilename)
{
	// TODO: return error ID
	if(m_Assets.m_ImageCount > MAX_IMAGES-1)
		return false;

	const int StrLen = str_length(pFilename);
	const bool EndsWithPng = StrLen > 4 && str_comp_nocase_num(pFilename+StrLen-4, ".png", 4) == 0;
	if(!EndsWithPng)
	{
		// TODO: can we load anything other than png files?
		//ed_dbg("ERROR: '%s' image file not supported", pFilename);
		return false;
	}


	CImageName ImgName;
	mem_zero(&ImgName, sizeof(ImgName));
	const int NameLen = min(StrLen-4, (int)sizeof(ImgName.m_Buff)-1);
	memmove(ImgName.m_Buff, pFilename, NameLen);
	ImgName.m_Buff[NameLen] = 0;
	const u32 NameHash = fnv1a32(&ImgName, sizeof(ImgName));

	// find image by name
	const int ImageCount = m_Assets.m_ImageCount;
	for(int i = 0; i < ImageCount; i++)
	{
		if(m_Assets.m_aImageNameHash[i] == NameHash)
		{
			//ed_log("'%s' image file already loaded", pFilename);
			return false;
		}
	}

	const int ImgID = m_Assets.m_ImageCount++;
	m_Assets.m_aImageNames[ImgID] = ImgName;
	m_Assets.m_aImageNameHash[ImgID] = NameHash;
	m_Assets.m_aImageEmbeddedCrc[ImgID] = 0;

	char aFilePath[256];
	str_format(aFilePath, sizeof(aFilePath), "mapres/%s", pFilename);

	CImageInfo ImgInfo;
	IGraphics::CTextureHandle TexHnd;
	if(Graphics()->LoadPNG(&ImgInfo, aFilePath, IStorage::TYPE_ALL))
	{
		const int TextureFlags = IGraphics::TEXLOAD_MULTI_DIMENSION;
		ImgInfo.m_Format = CImageInfo::FORMAT_AUTO;
		TexHnd = Graphics()->LoadTextureRaw(ImgInfo.m_Width, ImgInfo.m_Height, ImgInfo.m_Format,
			ImgInfo.m_pData, ImgInfo.m_Format, TextureFlags);
		mem_free(ImgInfo.m_pData); // TODO: keep this?
		ImgInfo.m_pData = NULL;

		if(!TexHnd.IsValid())
		{
			//ed_dbg("LoadTextureRaw ERROR: could not load '%s' image file", aFilePath);
			m_Assets.m_ImageCount--;
			return false;
		}
	}
	else
	{
		//ed_dbg("LoadPNG ERROR: could not load '%s' image file", aFilePath);
		m_Assets.m_ImageCount--;
		return false;
	}

	m_Assets.m_aTextureHandle[ImgID] = TexHnd;
	m_Assets.m_aTextureInfos[ImgID] = ImgInfo;

	//ed_dbg("Image '%s' loaded", aFilePath);

	AssetsLoadAutomapFileForImage(ImgID);

	return true;
}

void CEditorMap2::AssetsDeleteImage(int ImgID)
{
	Graphics()->UnloadTexture(&m_Assets.m_aTextureHandle[ImgID]);
	const int SwappedID = m_Assets.m_ImageCount-1;
	tl_swap(m_Assets.m_aImageNames[ImgID], m_Assets.m_aImageNames[SwappedID]);
	tl_swap(m_Assets.m_aImageEmbeddedCrc[ImgID], m_Assets.m_aImageEmbeddedCrc[SwappedID]);
	tl_swap(m_Assets.m_aTextureHandle[ImgID], m_Assets.m_aTextureHandle[SwappedID]);
	tl_swap(m_Assets.m_aTextureInfos[ImgID], m_Assets.m_aTextureInfos[SwappedID]);
	m_Assets.m_ImageCount--;

	const int LayerCount = m_aLayers.size();
	for(int li = 0; li < LayerCount; li++)
	{
		if(m_aLayers[li].m_ImageID == ImgID)
			m_aLayers[li].m_ImageID = -1;
		if(m_aLayers[li].m_ImageID == SwappedID)
			m_aLayers[li].m_ImageID = ImgID;
	}
}

void CEditorMap2::AssetsLoadAutomapFileForImage(int ImgID)
{
	u32 ImgNameHash = m_Assets.m_aImageNameHash[ImgID];
	dbg_assert(ImgNameHash != 0x0, "Image hash is invalid");

	bool FoundTileMapper = false;
	const u32* aAutomapTileHashID = m_Assets.m_aAutomapTileHashID.base_ptr();
	const int aAutomapTileHashIDCount = m_Assets.m_aAutomapTileHashID.size();
	for(int i = 0; i < aAutomapTileHashIDCount; i++)
	{
		if(aAutomapTileHashID[i] == ImgNameHash)
		{
			FoundTileMapper = true;
			break;
		}
	}

	if(FoundTileMapper)
		return;

	char aAutomapFilePath[256];
	str_format(aAutomapFilePath, sizeof(aAutomapFilePath), "editor/automap/%s.json", m_Assets.m_aImageNames[ImgID].m_Buff);
	IOHANDLE File = Storage()->OpenFile(aAutomapFilePath, IOFLAG_READ, IStorage::TYPE_ALL);
	if(File)
	{
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
			//ed_log("Error parsing json: '%s' (%s)", aError, aAutomapFilePath);
			return;
		}

		// generate configurations
		const json_value &rTileset = (*pJsonData)[(const char *)AutoMap::GetTypeName(AutoMap::TYPE_TILESET)];
		if(rTileset.type == json_array)
		{
			// tile automapper
			m_Assets.m_aAutomapTileHashID.add(ImgNameHash);
			int TileMapperArrayID = m_Assets.m_aAutomapTile.add(CTilesetMapper2());
			CTilesetMapper2& TileMapper = m_Assets.m_aAutomapTile[TileMapperArrayID];
			TileMapper.LoadJsonRuleSets(rTileset);
			//ed_dbg("Automap file '%s' loaded (Tileset mapper)", aAutomapFilePath);
		}
		else
		{
			const json_value &rDoodads = (*pJsonData)[(const char *)AutoMap::GetTypeName(AutoMap::TYPE_DOODADS)];
			if(rDoodads.type == json_array)
			{
				// doodad automapper
				// TODO: implement this
				//ed_dbg("Automap file '%s' loaded (Doodads mapper)", aAutomapFilePath);
			}
		}

		// clean up
		json_value_free(pJsonData);
	}
}

void CEditorMap2::AssetsLoadMissingAutomapFiles()
{
	// TODO: this could be much better
	const int ImgCount = m_Assets.m_ImageCount;
	for(int i = 0; i < ImgCount; i++)
	{
		AssetsLoadAutomapFileForImage(i);
	}
}

CTilesetMapper2* CEditorMap2::AssetsFindTilesetMapper(int ImgID)
{
	if(ImgID < 0)
		return 0;
	dbg_assert(ImgID < m_Assets.m_ImageCount, "ImgID out of bounds");

	CTilesetMapper2* pMapper = 0;

	const u32 ImgNameHash = m_Assets.m_aImageNameHash[ImgID];
	const u32* aAutomapTileHashID = m_Assets.m_aAutomapTileHashID.base_ptr();
	const int aAutomapTileHashIDCount = m_Assets.m_aAutomapTileHashID.size();
	for(int i = 0; i < aAutomapTileHashIDCount; i++)
	{
		if(aAutomapTileHashID[i] == ImgNameHash)
		{
			pMapper = &m_Assets.m_aAutomapTile[i];
			break;
		}
	}

	return pMapper;
}

CEditorMap2::CLayer& CEditorMap2::NewTileLayer(int Width, int Height)
{
	CLayer TileLayer;
	mem_zero(TileLayer.m_aName, sizeof(TileLayer.m_aName));
	TileLayer.m_Type = LAYERTYPE_TILES;
	TileLayer.m_ImageID = -1;
	TileLayer.m_Color = vec4(1, 1, 1, 1);
	TileLayer.m_Width = Width;
	TileLayer.m_Height = Height;
	TileLayer.m_ColorEnvelopeID = -1;
	TileLayer.m_aTiles.clear();
	TileLayer.m_aTiles.add_empty(TileLayer.m_Width * TileLayer.m_Height);
	return m_aLayers.add(TileLayer);
}

CEditorMap2::CLayer&CEditorMap2::NewQuadLayer()
{
	CLayer QuadLayer;
	mem_zero(QuadLayer.m_aName, sizeof(QuadLayer.m_aName));
	QuadLayer.m_Type = LAYERTYPE_QUADS;
	QuadLayer.m_ImageID = -1;
	QuadLayer.m_Color = vec4(1, 1, 1, 1);
	QuadLayer.m_aQuads.clear();
	return m_aLayers.add(QuadLayer);
}

void CEditorMap2::CLayer::Clamp(RECTi *pRect)
{
	if(pRect->x < 0)
	{
		pRect->w += pRect->x;
		pRect->x = 0;
	}

	if(pRect->y < 0)
	{
		pRect->h += pRect->y;
		pRect->y = 0;
	}

	if(pRect->x+pRect->w > m_Width)
		pRect->w = m_Width - pRect->x;

	if(pRect->y+pRect->h > m_Height)
		pRect->h = m_Height - pRect->y;

	if(pRect->h < 0)
		pRect->h = 0;
	if(pRect->w < 0)
		pRect->w = 0;
}
