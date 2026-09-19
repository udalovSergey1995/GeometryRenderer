#pragma once

typedef struct _PathPipeLine
{
	SBaseObject Base;
	SPathStages m_sPathStages;
} SPathPipeLine, * PSPathPipeLine;

/*
* Проинициализировать объект паплайна
*/
GR_EXPORT
PSPathPipeLine
GR_CALL
PathPipeLineCreate();

GR_EXPORT
VOID
GR_CALL
PathPipeLineDestroy(
	_In_ PSPathPipeLine pPipeline
);

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
);

GR_EXPORT
BOOL
GR_CALL
PathPipeLineFlattenizeLogicalLine(
	_In_ PSPathPipeLine pPipeline
);

/*
* Применить dash pattern к последнему этапу (Logical или Approximated).
* pDashLengths - массив длин [dash, gap, dash, gap, ...]
* iDashCount   - количество элементов в массиве (должно быть >= 2)
* fDashOffset  - смещение начала dash pattern
*/
GR_EXPORT
BOOL
GR_CALL
PathPipeLineApplyDashPattern(
	_In_ PSPathPipeLine pPipeline,
	_In_reads_(iDashCount) PCFLOAT pDashLengths,
	_In_ INT iDashCount,
	_In_ FLOAT fDashOffset
);

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeEnumPathstages(
	_In_ PSPathPipeLine pPipeline,
	_In_ PSPathStageEntry pCurrentItem
);

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeLineGetLastStage(
	_In_ PSPathPipeLine pPipeline
);
