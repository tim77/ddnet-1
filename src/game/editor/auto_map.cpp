#include <stdio.h>	// sscanf

#include <engine/console.h>
#include <engine/storage.h>
#include <engine/shared/linereader.h>

#include "auto_map.h"
#include "editor.h"

CAutoMapper::CAutoMapper(CEditor *pEditor)
{
	m_pEditor = pEditor;
	m_FileLoaded = false;
}

void CAutoMapper::Load(const char* pTileName)
{
	char aPath[256];
	str_format(aPath, sizeof(aPath), "editor/%s.rules", pTileName);
	IOHANDLE RulesFile = m_pEditor->Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!RulesFile)
		return;

	CLineReader LineReader;
	LineReader.Init(RulesFile);

	CConfiguration *pCurrentConf = 0;
	CRun *pCurrentRun = 0;
	CIndexRule *pCurrentIndex = 0;

	char aBuf[256];

	// read each line
	while(char *pLine = LineReader.Get())
	{
		// skip blank/empty lines as well as comments
		if(str_length(pLine) > 0 && pLine[0] != '#' && pLine[0] != '\n' && pLine[0] != '\r'
			&& pLine[0] != '\t' && pLine[0] != '\v' && pLine[0] != ' ')
		{
			if(pLine[0]== '[')
			{
				// new configuration, get the name
				pLine++;
				CConfiguration NewConf;
				int ConfigurationID = m_lConfigs.add(NewConf);
				pCurrentConf = &m_lConfigs[ConfigurationID];
				str_copy(pCurrentConf->m_aName, pLine, str_length(pLine));

				// add start run
				CRun NewRun;
				NewRun.m_AutomapCopy = true;
				int RunID = pCurrentConf->m_aRuns.add(NewRun);
				pCurrentRun = &pCurrentConf->m_aRuns[RunID];
			}
			else if(!str_comp_num(pLine, "NewRun", 6))
			{
				// add new run
				CRun NewRun;
				NewRun.m_AutomapCopy = true;
				int RunID = pCurrentConf->m_aRuns.add(NewRun);
				pCurrentRun = &pCurrentConf->m_aRuns[RunID];
			}
			else if(!str_comp_num(pLine, "Index", 5))
			{
				// new index
				int ID = 0;
				char aOrientation1[128] = "";
				char aOrientation2[128] = "";
				char aOrientation3[128] = "";

				sscanf(pLine, "Index %d %127s %127s %127s", &ID, aOrientation1, aOrientation2, aOrientation3);

				CIndexRule NewIndexRule;
				NewIndexRule.m_ID = ID;
				NewIndexRule.m_Flag = 0;
				NewIndexRule.m_RandomProbability = 1.0;
				NewIndexRule.m_DefaultRule = true;

				if(str_length(aOrientation1) > 0)
				{
					if(!str_comp(aOrientation1, "XFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_VFLIP;
					else if(!str_comp(aOrientation1, "YFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_HFLIP;
					else if(!str_comp(aOrientation1, "ROTATE"))
						NewIndexRule.m_Flag |= TILEFLAG_ROTATE;
				}

				if(str_length(aOrientation2) > 0)
				{
					if(!str_comp(aOrientation2, "XFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_VFLIP;
					else if(!str_comp(aOrientation2, "YFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_HFLIP;
					else if(!str_comp(aOrientation2, "ROTATE"))
						NewIndexRule.m_Flag |= TILEFLAG_ROTATE;
				}

				if(str_length(aOrientation3) > 0)
				{
					if(!str_comp(aOrientation3, "XFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_VFLIP;
					else if(!str_comp(aOrientation3, "YFLIP"))
						NewIndexRule.m_Flag |= TILEFLAG_HFLIP;
					else if(!str_comp(aOrientation3, "ROTATE"))
						NewIndexRule.m_Flag |= TILEFLAG_ROTATE;
				}

				// add the index rule object and make it current
				int IndexRuleID = pCurrentRun->m_aIndexRules.add(NewIndexRule);
				pCurrentIndex = &pCurrentRun->m_aIndexRules[IndexRuleID];
			}
			else if(!str_comp_num(pLine, "Pos", 3) && pCurrentIndex)
			{
				int x = 0, y = 0;
				char aValue[128];
				int Value = CPosRule::NORULE;
				array<CIndexInfo> NewIndexList;

				sscanf(pLine, "Pos %d %d %127s", &x, &y, aValue);

				if(!str_comp(aValue, "EMPTY"))
				{
					Value = CPosRule::INDEX;
					CIndexInfo NewIndexInfo = {0, 0};
					NewIndexList.add(NewIndexInfo);
				}
				else if(!str_comp(aValue, "FULL"))
				{
					Value = CPosRule::NOTINDEX;
					CIndexInfo NewIndexInfo1 = {0, 0};
					//CIndexInfo NewIndexInfo2 = {-1, 0};
					NewIndexList.add(NewIndexInfo1);
					//NewIndexList.add(NewIndexInfo2);
				}
				else if(!str_comp(aValue, "INDEX") || !str_comp(aValue, "NOTINDEX"))
				{
					if(!str_comp(aValue, "INDEX"))
						Value = CPosRule::INDEX;
					else
						Value = CPosRule::NOTINDEX;

					int pWord = 4;
					while(true) {
						int ID = 0;
						char aOrientation1[128] = "";
						char aOrientation2[128] = "";
						char aOrientation3[128] = "";
						char aOrientation4[128] = "";
						sscanf(str_trim_words(pLine, pWord), "%d %127s %127s %127s %127s", &ID, aOrientation1, aOrientation2, aOrientation3, aOrientation4);

						CIndexInfo NewIndexInfo;
						NewIndexInfo.m_ID = ID;
						NewIndexInfo.m_Flag = -1;

						if(!str_comp(aOrientation1, "OR")) {
							NewIndexList.add(NewIndexInfo);
							pWord += 2;
							continue;
						} else if(str_length(aOrientation1) > 0) {
							if(!str_comp(aOrientation1, "XFLIP"))
								NewIndexInfo.m_Flag = TILEFLAG_VFLIP;
							else if(!str_comp(aOrientation1, "YFLIP"))
								NewIndexInfo.m_Flag = TILEFLAG_HFLIP;
							else if(!str_comp(aOrientation1, "ROTATE"))
								NewIndexInfo.m_Flag = TILEFLAG_ROTATE;
							else if(!str_comp(aOrientation1, "NONE"))
								NewIndexInfo.m_Flag = 0;
						} else {
							NewIndexList.add(NewIndexInfo);
							break;
						}

						if(!str_comp(aOrientation2, "OR")) {
							NewIndexList.add(NewIndexInfo);
							pWord += 3;
							continue;
						} else if(str_length(aOrientation2) > 0 && NewIndexInfo.m_Flag != 0) {
							if(!str_comp(aOrientation2, "XFLIP"))
								NewIndexInfo.m_Flag |= TILEFLAG_VFLIP;
							else if(!str_comp(aOrientation2, "YFLIP"))
								NewIndexInfo.m_Flag |= TILEFLAG_HFLIP;
							else if(!str_comp(aOrientation2, "ROTATE"))
								NewIndexInfo.m_Flag |= TILEFLAG_ROTATE;
						} else {
							NewIndexList.add(NewIndexInfo);
							break;
						}

						if(!str_comp(aOrientation3, "OR")) {
							NewIndexList.add(NewIndexInfo);
							pWord += 4;
							continue;
						} else if(str_length(aOrientation3) > 0 && NewIndexInfo.m_Flag != 0) {
							if(!str_comp(aOrientation3, "XFLIP"))
								NewIndexInfo.m_Flag |= TILEFLAG_VFLIP;
							else if(!str_comp(aOrientation3, "YFLIP"))
								NewIndexInfo.m_Flag |= TILEFLAG_HFLIP;
							else if(!str_comp(aOrientation3, "ROTATE"))
								NewIndexInfo.m_Flag |= TILEFLAG_ROTATE;
						} else {
							NewIndexList.add(NewIndexInfo);
							break;
						}

						if(!str_comp(aOrientation4, "OR")) {
							NewIndexList.add(NewIndexInfo);
							pWord += 5;
							continue;
						} else {
							NewIndexList.add(NewIndexInfo);
							break;
						}
					}
				}

				if(Value != CPosRule::NORULE) {
					CPosRule NewPosRule = {x, y, Value, NewIndexList};
					pCurrentIndex->m_aRules.add(NewPosRule);
				}
			}
			else if(!str_comp_num(pLine, "Random", 6) && pCurrentIndex)
			{
				float Value;
				char Specifier = ' ';
				sscanf(pLine, "Random %f%c", &Value, &Specifier);
				if(Specifier == '%')
				{
					pCurrentIndex->m_RandomProbability = Value / 100.0;
				}
				else
				{
					pCurrentIndex->m_RandomProbability = 1.0 / Value;
				}
			}
			else if(!str_comp_num(pLine, "NoDefaultRule", 13) && pCurrentIndex)
			{
				pCurrentIndex->m_DefaultRule = false;
			}
			else if(!str_comp_num(pLine, "NoLayerCopy", 11) && pCurrentRun)
			{
				pCurrentRun->m_AutomapCopy = false;
			}
		}
	}

	// add default rule for Pos 0 0 if there is none
	for (int g = 0; g < m_lConfigs.size(); ++g)
	{
		for (int h = 0; h < m_lConfigs[g].m_aRuns.size(); ++h)
		{
			for(int i = 0; i < m_lConfigs[g].m_aRuns[h].m_aIndexRules.size(); ++i)
			{
				bool Found = false;
				for(int j = 0; j < m_lConfigs[g].m_aRuns[h].m_aIndexRules[i].m_aRules.size(); ++j)
				{
					CPosRule *pRule = &m_lConfigs[g].m_aRuns[h].m_aIndexRules[i].m_aRules[j];
					if(pRule && pRule->m_X == 0 && pRule->m_Y == 0)
					{
						Found = true;
						break;
					}
				}
				if(!Found && m_lConfigs[g].m_aRuns[h].m_aIndexRules[i].m_DefaultRule)
				{
					array<CIndexInfo> NewIndexList;
					CIndexInfo NewIndexInfo = {0, 0};
					NewIndexList.add(NewIndexInfo);
					CPosRule NewPosRule = {0, 0, CPosRule::NOTINDEX, NewIndexList};
					m_lConfigs[g].m_aRuns[h].m_aIndexRules[i].m_aRules.add(NewPosRule);
				}
			}
		}
	}

	io_close(RulesFile);

	str_format(aBuf, sizeof(aBuf),"loaded %s", aPath);
	m_pEditor->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", aBuf);

	m_FileLoaded = true;
}

const char* CAutoMapper::GetConfigName(int Index)
{
	if(Index < 0 || Index >= m_lConfigs.size())
		return "";

	return m_lConfigs[Index].m_aName;
}

void CAutoMapper::Proceed(CLayerTiles *pLayer, int ConfigID)
{
	if(!m_FileLoaded || pLayer->m_Readonly || ConfigID < 0 || ConfigID >= m_lConfigs.size())
		return;

	CConfiguration *pConf = &m_lConfigs[ConfigID];

	// for every run: copy tiles, automap, overwrite tiles
	for(int h = 0; h < pConf->m_aRuns.size(); ++h) {
		CRun *pRun = &pConf->m_aRuns[h];

		// don't make copy if it's requested
		CLayerTiles *pReadLayer;
		if(pRun->m_AutomapCopy) 
		{
			pReadLayer = new CLayerTiles(pLayer->m_Width, pLayer->m_Height);
		} 
		else 
		{
			pReadLayer = pLayer;
		}

		// copy tiles
		if(pRun->m_AutomapCopy) 
		{
			for(int y = 0; y < pLayer->m_Height; y++) {
				for(int x = 0; x < pLayer->m_Width; x++)
				{
					CTile *in = &pLayer->m_pTiles[y*pLayer->m_Width+x];
					CTile *out = &pReadLayer->m_pTiles[y*pLayer->m_Width+x];
					out->m_Index = in->m_Index;
					out->m_Flags = in->m_Flags;
				}
			}
		}

		// auto map
		for(int y = 0; y < pLayer->m_Height; y++) {
			for(int x = 0; x < pLayer->m_Width; x++)
			{
				CTile *pTile = &(pLayer->m_pTiles[y*pLayer->m_Width+x]);
				m_pEditor->m_Map.m_Modified = true;

				for(int i = 0; i < pRun->m_aIndexRules.size(); ++i)
				{
					bool RespectRules = true;
					for(int j = 0; j < pRun->m_aIndexRules[i].m_aRules.size() && RespectRules; ++j)
					{
						CPosRule *pRule = &pRun->m_aIndexRules[i].m_aRules[j];

						int CheckIndex, CheckFlags;
						int CheckX = x + pRule->m_X;
						int CheckY = y + pRule->m_Y;
						if(CheckX >= 0 && CheckX < pLayer->m_Width && CheckY >= 0 && CheckY < pLayer->m_Height) {
							int CheckTile = CheckY * pLayer->m_Width + CheckX;
							CheckIndex = pReadLayer->m_pTiles[CheckTile].m_Index;
							CheckFlags = pReadLayer->m_pTiles[CheckTile].m_Flags;
						} else {
							CheckIndex = -1;
							CheckFlags = 0;
						}

	 					if(pRule->m_Value == CPosRule::INDEX)
						{
							bool PosRuleTest = false;
							for(int i = 0; i < pRule->m_aIndexList.size() && !PosRuleTest; ++i) {
								if(CheckIndex == pRule->m_aIndexList[i].m_ID && (pRule->m_aIndexList[i].m_Flag == -1 || CheckFlags == pRule->m_aIndexList[i].m_Flag))
									PosRuleTest = true;
							}
							if(!PosRuleTest)
								RespectRules = false;
						}
	 					else if(pRule->m_Value == CPosRule::NOTINDEX)
						{
							bool PosRuleTest = true;
							for(int i = 0; i < pRule->m_aIndexList.size() && PosRuleTest; ++i) {
								if(CheckIndex == pRule->m_aIndexList[i].m_ID && (pRule->m_aIndexList[i].m_Flag == -1 || CheckFlags == pRule->m_aIndexList[i].m_Flag))
									PosRuleTest = false;
							}
							if(!PosRuleTest)
								RespectRules = false;
						}
					}

					if(RespectRules &&
						(pRun->m_aIndexRules[i].m_RandomProbability >= 1.0 || (float)rand() / ((float)RAND_MAX + 1) < pRun->m_aIndexRules[i].m_RandomProbability))
					{
						pTile->m_Index = pRun->m_aIndexRules[i].m_ID;
						pTile->m_Flags = pRun->m_aIndexRules[i].m_Flag;
					}
				}
			}
		}

		// clean-up
		if(pRun->m_AutomapCopy)
			delete pReadLayer;
	}
}
