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
int
GR_CALL
PathPipeLineDestroy(
	_In_ PSPathPipeLine pPipeline
);

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
);

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeEnumPathstages(
	_In_ PSPathPipeLine pPipeline,
	_In_ PSPathStageEntry pCurrentItem
);


