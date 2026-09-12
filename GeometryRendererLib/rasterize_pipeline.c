#include "stdafx.h"

/*
* Проинициализировать объект паплайна
*/
GR_EXPORT
PSPathPipeLine
GR_CALL
PathPipeLineCreate()
{
	PSPathPipeLine pPipeline = malloc(sizeof(SPathPipeLine));

	if (!pPipeline)
		return NULL;

	memset(pPipeline, 0, sizeof(SPathPipeLine));

	pPipeline->Base.type = EOT_PipeLine;

	PathStagesInitialize(&pPipeline->m_sPathStages);

	return pPipeline;
}

GR_EXPORT
VOID
GR_CALL
PathPipeLineDestroy(
	_In_ PSPathPipeLine pPipeline
)
{
	if (!pPipeline)
		return 0;

	PathStagesDestroy(&pPipeline->m_sPathStages);
	free(pPipeline);
}

/*
* Проинициализировать объект паплайна набором точек.
* pfPoints - указатель на массив точек {x,y,x,y,x,y, ... }
* iCount - количество пар коордиат
*/
GR_EXPORT
INT
GR_CALL
PathPipeLineAddLogicalLine(
	_In_ PSPathPipeLine pPipeline,
	_In_ PFLOAT pPoints,
	_In_ PBYTE pTypes,
	_In_ INT iCount,
	_In_ BOOL fIsBeziere,
	_In_ BOOL fIsClose
)
{
	if (!pPipeline || !pTypes|| !pPoints || !iCount)
		return 0;

	PSLineItem pLineItem = LineItemAllocate(iCount,
		fIsBeziere ? LineItemType_Bezier : LineItemType_Flatten,
		fIsClose);

	if (!pLineItem)
	{
		return 0;
	}
	else
	{
		memcpy(pLineItem->Points, pPoints, sizeof(FLOAT) * iCount);
		memcpy(pLineItem->PointTypes, pTypes, iCount);
	}

	if (!PathStagesAddStage(
		&pPipeline->m_sPathStages,
		PathStageTypeLogicalCurve,
		pLineItem,
		LineItemFreeWithPoints))
	{
		LineItemFree(pLineItem);
		return 0;
	}

	return 1;
}

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeEnumPathstages(
	_In_ PSPathPipeLine pPipeline,
	_In_ PSPathStageEntry pCurrentItem
)
{

	PSPathStageEntry entry = NULL;

	if (!pPipeline 
		|| pPipeline->m_sPathStages.Head.Flink == &pPipeline->m_sPathStages.Head)
	{
		goto ret_pt;
	}

	if (!pCurrentItem)
	{
		entry = CONTAINING_RECORD(pPipeline->m_sPathStages.Head.Flink, SPathStageEntry, ListEntry);
	}
	else
	{
		entry = CONTAINING_RECORD(pCurrentItem->ListEntry.Flink, SPathStageEntry, ListEntry);
	}

	if (&entry->ListEntry == &(pPipeline->m_sPathStages.Head))
		entry = NULL;

ret_pt:

	return entry;
}

