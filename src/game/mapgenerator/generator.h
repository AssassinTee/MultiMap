#ifndef MAPGENERATOR_GENERATOR_H
#define MAPGENERATOR_GENERATOR_H

#include <stdint.h>
#include <string.h> // memset
#include <base/system.h>
#include <base/tl/array.h>

#include <engine/editor.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

#include <game/mapitems.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <generated/client_data.h>
#include <game/client/components/mapimages.h>
#include "auto_map2.h"

class IGraphics;
class IConsole;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

template<typename T>
struct array2: public array<T>
{
	typedef array<T> ParentT;

	inline T& add(const T& Elt)
	{
		return this->base_ptr()[ParentT::add(Elt)];
	}

	T& add(const T* aElements, int EltCount)
	{
		for(int i = 0; i < EltCount; i++)
			ParentT::add(aElements[i]);
		return *(this->base_ptr()+this->size()-EltCount);
	}

	T& add_empty(int EltCount)
	{
		dbg_assert(EltCount > 0, "Add 0 or more");
		for(int i = 0; i < EltCount; i++)
			ParentT::add(T());
		return *(this->base_ptr()+this->size()-EltCount);
	}

	void clear()
	{
		// don't free buffer on clear
		this->num_elements = 0;
	}

	inline void remove_index_fast(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index_fast(Index);
	}

	// keeps order, way slower
	inline void remove_index(int Index)
	{
		dbg_assert(Index >= 0 && Index < this->size(), "Index out of bounds");
		ParentT::remove_index(Index);
	}

	inline T& operator[] (int Index)
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}

	inline const T& operator[] (int Index) const
	{
		dbg_assert(Index >= 0 && Index < ParentT::size(), "Index out of bounds");
		return ParentT::operator[](Index);
	}
};

struct CEditorMap2
{
	enum
	{
		MAX_IMAGES=128,
		MAX_GROUP_LAYERS=64,
		MAX_IMAGE_NAME_LEN=64,
		MAX_EMBEDDED_FILES=64,
	};

	struct CLayer
	{
		char m_aName[12];
		int m_Type = 0;
		int m_ImageID = 0;
		bool m_HighDetail; // TODO: turn into m_Flags ?
		vec4 m_Color;

		// NOTE: we have to split the union because gcc doesn't like non-POD anonymous structs...
		// TODO: enable c++11, please?

		//union
		//{
			array2<CTile> m_aTiles;
			array2<CQuad> m_aQuads;
		//};

		union
		{
			// tile
			struct {
				int m_Width;
				int m_Height;
				int m_ColorEnvelopeID;
				int m_ColorEnvOffset;
			};

			// quad
			struct {

			};
		};

		inline bool IsTileLayer() const { return m_Type == LAYERTYPE_TILES; }
		inline bool IsQuadLayer() const { return m_Type == LAYERTYPE_QUADS; }
	};

	struct CGroup
	{
		char m_aName[12] = {0};
		int m_apLayerIDs[MAX_GROUP_LAYERS];
		int m_LayerCount = 0;
		int m_ParallaxX = 0;
		int m_ParallaxY = 0;
		int m_OffsetX = 0;
		int m_OffsetY = 0;
		int m_ClipX = 0;
		int m_ClipY = 0;
		int m_ClipWidth = 0;
		int m_ClipHeight = 0;
		bool m_UseClipping = false;
	};

	struct CEnvelope
	{
		int m_Version;
		int m_Channels;
		CEnvPoint* m_aPoints;
		int m_NumPoints;
		//int m_aName[8];
		bool m_Synchronized;
	};

	struct CImageName
	{
		char m_Buff[MAX_IMAGE_NAME_LEN];
	};

	struct CEmbeddedFile
	{
		u32 m_Crc;
		int m_Type; // unused for now (only images)
		void* m_pData;
	};

	struct CAssets
	{
		// TODO: pack some of these together
		CImageName m_aImageNames[MAX_IMAGES];
		u32 m_aImageNameHash[MAX_IMAGES];
		u32 m_aImageEmbeddedCrc[MAX_IMAGES];
		IGraphics::CTextureHandle m_aTextureHandle[MAX_IMAGES];
		CImageInfo m_aTextureInfos[MAX_IMAGES];
		int m_ImageCount = 0;

		CEmbeddedFile m_aEmbeddedFile[MAX_EMBEDDED_FILES];
		int m_EmbeddedFileCount;

		array<u32> m_aAutomapTileHashID;
		array<CTilesetMapper2> m_aAutomapTile;
	};

	int m_MapMaxWidth = 0;
	int m_MapMaxHeight = 0;
	int m_GameLayerID = -1;
	int m_GameGroupID = -1;

	char m_aPath[256];

	array2<CEnvPoint> m_aEnvPoints;
	array2<CLayer> m_aLayers;
	array2<CGroup> m_aGroups;
	array2<CMapItemEnvelope> m_aEnvelopes;

	CAssets m_Assets;

	IGraphics* m_pGraphics;
	IConsole *m_pConsole;
	IStorage *m_pStorage;

	inline IGraphics* Graphics() { return m_pGraphics; };
	inline IConsole *Console() { return m_pConsole; };
	inline IStorage *Storage() { return m_pStorage; };

	void Init(IStorage *pStorage, IGraphics* pGraphics, IConsole* pConsole);
	bool Save(const char *pFileName);
	//bool Load(const char *pFileName);
	void LoadDefault();
	void Clear();

	// loads not-loaded images, clears the rest
	void AssetsClearAndSetImages(CImageName* aName, CImageInfo* aInfo, u32* aImageEmbeddedCrc, int ImageCount);
	u32 AssetsAddEmbeddedData(void* pData, u64 DataSize);
	void AssetsClearEmbeddedFiles();
	bool AssetsAddAndLoadImage(const char* pFilename); // also loads automap rules if there are any
	void AssetsDeleteImage(int ImgID);
	void AssetsLoadAutomapFileForImage(int ImgID);
	void AssetsLoadMissingAutomapFiles();
	CTilesetMapper2* AssetsFindTilesetMapper(int ImgID);

	CLayer& NewTileLayer(int Width, int Height);
	CLayer& NewQuadLayer();
};

class CMapGenerator
{
public:
	CMapGenerator();
	bool GenerateMap(IStorage* pStorage, IGraphics* pGraphics, IConsole* pConsole, const char* pFilename);
private:
	IEditor* m_pEditor;
};

#endif
