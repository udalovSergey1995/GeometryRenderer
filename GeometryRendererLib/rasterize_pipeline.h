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

/*
 * Применить генерацию толстой линии к последнему этапу
 * (LogicalCurve или Approximated / DashPattern).
 *
 * Thickness  - толщина линии
 * JoinStyle  - стиль соединения (Miter/Round/Bevel)
 * CapStyle   - стиль окончания (Flat/Round/Square)
 * MiterLimit - ограничение miter (имеет смысл только для Miter)
 */
GR_EXPORT
BOOL
GR_CALL
PathPipeLineApplyThickLine(
	_In_ PSPathPipeLine     pPipeline,
	_In_ FLOAT              Thickness,
	_In_ THICK_LINE_JOIN    JoinStyle,
	_In_ THICK_LINE_CAP     CapStyle,
	_In_ FLOAT              MiterLimit
);
