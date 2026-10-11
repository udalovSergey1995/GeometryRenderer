#include "stdafx.h"

GR_EXPORT
BOOL
GR_CALL
PathPipeLineDrawSlabs(
    _Inout_ PSPathPipeLine pipeline,
    _Inout_updates_bytes_(bufferLength) PBYTE pixels,
    _In_ INT bufferLength,
    _In_ INT width,
    _In_ INT height,
    _In_ INT stride,
    _In_ UINT color
)
{
    PSPathStageEntry entry;
    /* Проверяем ёмкость буфера до изменения пайплайна. uint64_t исключает
       переполнение промежуточных произведений ширины и шага строки. */
    if (!pipeline || !pixels || width <= 0 || height <= 0 || stride <= 0 || bufferLength <= 0 ||
        (uint64_t)width * 4 > (uint64_t)stride ||
        (uint64_t)(height - 1) * stride + (uint64_t)width * 4 > (uint64_t)bufferLength)
    {
        return FALSE;
    }
    entry = PathStagesGetLastStage(&pipeline->m_sPathStages);
    if (!entry)
    {
        return FALSE;
    }
    /* Исходная кривая сначала преобразуется в отрезки. Уже подготовленные
       этапы не пересчитываем: результат растеризации сохраняется в списке. */
    if (entry->StageType == PathStageTypeLogicalCurve &&
        ((PSLineItem)entry->StageData)->Type == LineItemType_Bezier)
    {
        if (!PathPipeLineFlattenizeLogicalLine(pipeline))
        {
            return FALSE;
        }
        entry = PathStagesGetLastStage(&pipeline->m_sPathStages);
    }
    if (entry->StageType != PathStageTypeRasterSlab)
    {
        if (!PathPipeLineRasterizeSlabs(pipeline))
        {
            return FALSE;
        }
        entry = PathStagesGetLastStage(&pipeline->m_sPathStages);
    }
    return RasterSlabItemDraw(
        (PSRasterSlabItem)entry->StageData, pixels, width, height, stride, color);
}

GR_EXPORT
BOOL
GR_CALL
PathPipeLineRasterizeSlabs(
    _Inout_ PSPathPipeLine pipeline
)
{
    PSPathStageEntry entry;
    PSLineItem line;
    PSRasterSlabItem item;
    if (!pipeline || !(entry = PathStagesGetLastStage(&pipeline->m_sPathStages)))
    {
        return FALSE;
    }
    /* Каждый тип хранит геометрию по-своему. Контейнер этапов не знает
       этих деталей; выбор входа выполняется при создании нового айтема. */
    switch (entry->StageType)
    {
    case PathStageTypeLogicalCurve:
    case PathStageTypeApproximated:
        line = (PSLineItem)entry->StageData;
        break;
    case PathStageTypeDashPattern:
        line = ((PSDashPatternItem)entry->StageData)->DashedLine;
        break;
    case PathStageTypeThickLine:
        line = ((PSThickLineItem)entry->StageData)->ThickGeometry;
        break;
    default:
        return FALSE;
    }
    item = RasterSlabItemCreate(line);
    if (!item)
    {
        return FALSE;
    }
    /* Владение результатом переходит контейнеру только после успешного
       добавления. До этого освобождать данные обязан этот метод. */
    if (!PathStagesAddStage(&pipeline->m_sPathStages,
                            PathStageTypeRasterSlab,
                            item,
                            RasterSlabItemFree,
                            RasterSlabItemGetProperty))
    {
        RasterSlabItemFree(item);
        return FALSE;
    }
    return TRUE;
}

/*
 * Проинициализировать объект паплайна
 */
GR_EXPORT
PSPathPipeLine
GR_CALL
PathPipeLineCreate(
    VOID
)
{
    PSPathPipeLine pPipeline = malloc(sizeof(SPathPipeLine));

    if (!pPipeline)
    {
        return NULL;
    }

    memset(pPipeline, 0, sizeof(SPathPipeLine));

    pPipeline->Base.type = EOT_PipeLine;

    PathStagesInitialize(&pPipeline->m_sPathStages);

    return pPipeline;
}

GR_EXPORT
VOID
GR_CALL
PathPipeLineDestroy(
    _Inout_ PSPathPipeLine pPipeline
)
{
    if (!pPipeline)
    {
        return;
    }

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
    _Inout_ PSPathPipeLine pPipeline,
    _In_ PFLOAT pPoints,
    _In_ PBYTE pTypes,
    _In_ INT iCount,
    _In_ BOOL fIsBeziere,
    _In_ BOOL fIsClose
)
{
    if (!pPipeline || !pTypes || !pPoints || !iCount)
    {
        return FALSE;
    }

    PSLineItem pLineItem =
        LineItemAllocate(iCount, fIsBeziere ? LineItemType_Bezier : LineItemType_Flatten, fIsClose);

    if (!pLineItem)
    {
        return FALSE;
    }
    else
    {
        memcpy(pLineItem->Points, pPoints, sizeof(FLOAT) * iCount * 2);
        memcpy(pLineItem->PointTypes, pTypes, iCount);
    }

    if (!PathStagesAddStage(&pPipeline->m_sPathStages,
                            PathStageTypeLogicalCurve,
                            pLineItem,
                            LineItemFreeWithPoints,
                            LineItemGetProperty))
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
    _Inout_ PSPathPipeLine pPipeline
)
{
    if (!pPipeline || !pPipeline->m_sPathStages.Count)
    {
        return FALSE;
    }

    // Получить тип последнего состояния пайплайна
    PSPathStageEntry pCurrentItem = PathStagesGetLastStage(&pPipeline->m_sPathStages);

    // Проверить не является ли айтем головой списка
    // и узнать совместима ли трансформация с текущим состоянием пайплайна
    if (!pCurrentItem || pCurrentItem->StageType != PathStageTypeLogicalCurve)
    {
        return FALSE;
    }

    PSLineItem pFlattenLineItem = FlattenItemInit(pCurrentItem->StageData);

    if (!pFlattenLineItem)
    {
        return FALSE;
    }

    if (!PathStagesAddStage(&pPipeline->m_sPathStages,
                            PathStageTypeApproximated,
                            pFlattenLineItem,
                            FlattenItemFree,
                            FlattenItemGetProperty))
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
    _Inout_ PSPathPipeLine pPipeline,
    _In_reads_(iDashCount) PCFLOAT pDashLengths,
    _In_ INT iDashCount,
    _In_ FLOAT fDashOffset
)
{
    PSPathStageEntry pCurrentItem;
    PSDashPatternItem pDashItem;
    PSLineItem pSourceLine;

    if (!pPipeline || !pDashLengths || iDashCount < 2)
    {
        return FALSE;
    }

    if (!pPipeline->m_sPathStages.Count)
    {
        return FALSE;
    }

    pCurrentItem = PathStagesGetLastStage(&pPipeline->m_sPathStages);

    if (!pCurrentItem)
    {
        return FALSE;
    }

    // Можно применять dash только к LogicalCurve или Approximated
    if (pCurrentItem->StageType != PathStageTypeLogicalCurve &&
        pCurrentItem->StageType != PathStageTypeApproximated)
    {
        return FALSE;
    }

    pSourceLine = (PSLineItem)pCurrentItem->StageData;

    if (!pSourceLine)
    {
        return FALSE;
    }

    // Если логический айтем — проверяем отсутствие Bezier
    if (pCurrentItem->StageType == PathStageTypeLogicalCurve)
    {
        UINT i;

        for (i = 0; i < pSourceLine->PointCount; i++)
        {
            if (pSourceLine->PointTypes[i] == LinePointType_BezierControl)
            {
                return FALSE;
            }
        }

        pDashItem = DashPatternItemInit(pSourceLine, pDashLengths, (UINT)iDashCount, fDashOffset);
    }
    else
    {
        // Approximated — уже flatten
        pDashItem = DashPatternItemInitFromFlatten(
            pSourceLine, pDashLengths, (UINT)iDashCount, fDashOffset);
    }

    if (!pDashItem)
    {
        return FALSE;
    }

    if (!PathStagesAddStage(&pPipeline->m_sPathStages,
                            PathStageTypeDashPattern,
                            pDashItem,
                            DashPatternItemFreeCallback,
                            DashPatternItemGetProperty))
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
    _Inout_ PSPathPipeLine pPipeline,
    _In_ PSPathStageEntry pCurrentItem
)
{

    PSPathStageEntry entry = NULL;

    if (!pPipeline || pPipeline->m_sPathStages.Head.Flink == &pPipeline->m_sPathStages.Head)
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
    {
        entry = NULL;
    }

ret_pt:

    return entry;
}

GR_EXPORT
PSPathStageEntry
GR_CALL
PathPipeLineGetLastStage(
    _Inout_ PSPathPipeLine pPipeline
)
{
    if (!pPipeline)
    {
        return NULL;
    }

    return PathStagesGetLastStage(&pPipeline->m_sPathStages);
}

GR_EXPORT
BOOL
GR_CALL
PathPipeLineApplyThickLine(
    _Inout_ PSPathPipeLine pPipeline,
    _In_ FLOAT Thickness,
    _In_ THICK_LINE_JOIN JoinStyle,
    _In_ THICK_LINE_CAP CapStyle,
    _In_ FLOAT MiterLimit
)
{
    return PathPipeLineApplyThickLineEx(
        pPipeline, Thickness, JoinStyle, CapStyle, MiterLimit, THICK_LINE_DEFAULT_ARC_TOLERANCE);
}

GR_EXPORT
BOOL
GR_CALL
PathPipeLineApplyThickLineEx(
    _Inout_ PSPathPipeLine pPipeline,
    _In_ FLOAT Thickness,
    _In_ THICK_LINE_JOIN JoinStyle,
    _In_ THICK_LINE_CAP CapStyle,
    _In_ FLOAT MiterLimit,
    _In_ FLOAT ArcTolerance
)
{
    PSPathStageEntry pCurrentItem;
    PSThickLineItem pThickItem;
    PSLineItem pSourceLine;

    if (!pPipeline || Thickness <= 0.0f)
    {
        return FALSE;
    }

    if (!pPipeline->m_sPathStages.Count)
    {
        return FALSE;
    }

    pCurrentItem = PathStagesGetLastStage(&pPipeline->m_sPathStages);
    if (!pCurrentItem)
    {
        return FALSE;
    }

    // Можно применять к LogicalCurve, Approximated или DashPattern
    if (pCurrentItem->StageType != PathStageTypeLogicalCurve &&
        pCurrentItem->StageType != PathStageTypeApproximated &&
        pCurrentItem->StageType != PathStageTypeDashPattern)
    {
        return FALSE;
    }

    // Получаем исходную геометрию
    if (pCurrentItem->StageType == PathStageTypeDashPattern)
    {
        PSDashPatternItem pDash = (PSDashPatternItem)pCurrentItem->StageData;
        if (!pDash || !pDash->DashedLine)
        {
            return FALSE;
        }

        pSourceLine = pDash->DashedLine;
    }
    else
    {
        pSourceLine = (PSLineItem)pCurrentItem->StageData;
    }

    if (!pSourceLine)
    {
        return FALSE;
    }

    // Создаём thick-айтем
    if (pCurrentItem->StageType == PathStageTypeLogicalCurve)
    {
        // На всякий случай можно проверить отсутствие Безье
        pThickItem = ThickLineItemInitEx(
            pSourceLine, Thickness, JoinStyle, CapStyle, MiterLimit, ArcTolerance);
    }
    else
    {
        pThickItem = ThickLineItemInitFromFlattenEx(
            pSourceLine, Thickness, JoinStyle, CapStyle, MiterLimit, ArcTolerance);
    }

    if (!pThickItem)
    {
        return FALSE;
    }

    if (!PathStagesAddStage(&pPipeline->m_sPathStages,
                            PathStageTypeThickLine,
                            pThickItem,
                            ThickLineItemFreeCallback,
                            ThickLineItemGetProperty))
    {
        ThickLineItemFree(pThickItem);
        return FALSE;
    }

    return TRUE;
}
