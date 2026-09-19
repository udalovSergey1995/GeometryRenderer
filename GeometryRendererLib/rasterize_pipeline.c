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
		return;

	PathStagesDestroy(&pPipeline->m_sPathStages);
	free(pPipeline);
}

/*
* Проинициализировать объект паплайна набором точек.
* pfPoints - указатель на массив точек {x,y,x,y,x,y, ... }
* iCount - количество пар коордиат
*/
GR_EXPORT
BOOL
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
		return FALSE;

	PSLineItem pLineItem = LineItemAllocate(iCount,
		fIsBeziere ? LineItemType_Bezier : LineItemType_Flatten,
		fIsClose);

	if (!pLineItem)
	{
		return FALSE;
	}
	else
	{
		memcpy(pLineItem->Points, pPoints, sizeof(FLOAT) * iCount * 2);
		memcpy(pLineItem->PointTypes, pTypes, iCount);
	}

	if (!PathStagesAddStage(
		&pPipeline->m_sPathStages,
		PathStageTypeLogicalCurve,
		pLineItem,
		LineItemFreeWithPoints))
	{
		LineItemFreeWithPoints(pLineItem);
		return FALSE;
	}

	return TRUE;
}

GR_EXPORT
BOOL
GR_CALL
PathPipeLineFlattenizeLogicalLine(
	_In_ PSPathPipeLine pPipeline
)
{
	if (!pPipeline || !pPipeline->m_sPathStages.Count)
	{
		return FALSE;
	}

	//Получить тип последнего состояния пайплайна
	PSPathStageEntry pCurrentItem = PathStagesGetLastStage(&pPipeline->m_sPathStages);

	//Проверить не является ли айтем головой списка
	//и узнать совместима ли трансформация с текущим состоянием пайплайна 
	if (!pCurrentItem
		|| pCurrentItem->StageType != PathStageTypeLogicalCurve)
	{
		return FALSE;
	}

	PSLineItem pFlattenLineItem = FlattenItemInit(pCurrentItem->StageData);

	if (!pFlattenLineItem)
	{
		return FALSE;
	}

	if (!PathStagesAddStage(
		&pPipeline->m_sPathStages,
		PathStageTypeApproximated,
		pFlattenLineItem,
		FlattenItemFree))
	{
		FlattenItemFree(pFlattenLineItem);
		return FALSE;
	}

	return TRUE;
}

GR_EXPORT
BOOL
GR_CALL
PathPipeLineApplyDashPattern(
	_In_ PSPathPipeLine pPipeline,
	_In_reads_(iDashCount) PCFLOAT pDashLengths,
	_In_ INT iDashCount,
	_In_ FLOAT fDashOffset
)
{
	PSPathStageEntry pCurrentItem;
	PSDashPatternItem pDashItem;
	PSLineItem pSourceLine;

	if (!pPipeline || !pDashLengths || iDashCount < 2)
		return FALSE;

	if (!pPipeline->m_sPathStages.Count)
		return FALSE;

	pCurrentItem = PathStagesGetLastStage(&pPipeline->m_sPathStages);

	if (!pCurrentItem)
		return FALSE;

	// Можно применять dash только к LogicalCurve или Approximated
	if (pCurrentItem->StageType != PathStageTypeLogicalCurve
		&& pCurrentItem->StageType != PathStageTypeApproximated)
	{
		return FALSE;
	}

	pSourceLine = (PSLineItem)pCurrentItem->StageData;

	if (!pSourceLine)
		return FALSE;

	// Если логический айтем — проверяем отсутствие Bezier
	if (pCurrentItem->StageType == PathStageTypeLogicalCurve)
	{
		UINT i;

		for (i = 0; i < pSourceLine->PointCount; i++)
			if (pSourceLine->PointTypes[i] == LinePointType_BezierControl)
				return FALSE;

		pDashItem = DashPatternItemInit(
			pSourceLine,
			pDashLengths,
			(UINT)iDashCount,
			fDashOffset);
	}
	else
	{
		// Approximated — уже flatten
		pDashItem = DashPatternItemInitFromFlatten(
			pSourceLine,
			pDashLengths,
			(UINT)iDashCount,
			fDashOffset);
	}

	if (!pDashItem)
		return FALSE;

	if (!PathStagesAddStage(
		&pPipeline->m_sPathStages,
		PathStageTypeDashPattern,
		pDashItem,
		DashPatternItemFreeCallback))
	{
		DashPatternItemFree(pDashItem);
		return FALSE;
	}

	return TRUE;
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

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeLineGetLastStage(
	_In_ PSPathPipeLine pPipeline
)
{
	if (!pPipeline)
	{
		return NULL;
	}

	return PathStagesGetLastStage(&pPipeline->m_sPathStages);
}
