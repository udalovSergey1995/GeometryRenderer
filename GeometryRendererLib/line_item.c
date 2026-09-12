#include "stdafx.h"

/* -------------------------------------------------------------
   LineItemAllocate
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
LineItemAllocate(
    _In_ UINT           PointCount,
    _In_ LINE_ITEM_TYPE Type,
    _In_ BOOL           ClosePath
    )
{
    PSLineItem      item;
    PFLOAT          points;
    PLINE_POINT_TYPE pointTypes;
    size_t          pointsSize;
    size_t          typesSize;

    item = (PSLineItem)malloc(sizeof(SLineItem));
    if (item == NULL)
    {
        return NULL;
    }

    if (PointCount > 0)
    {
        pointsSize = (size_t)PointCount * 2 * sizeof(FLOAT);
        points = (PFLOAT)malloc(pointsSize);
        if (points == NULL)
        {
            free(item);
            return NULL;
        }

        typesSize = (size_t)PointCount * sizeof(LINE_POINT_TYPE);
        pointTypes = (PLINE_POINT_TYPE)malloc(typesSize);
        if (pointTypes == NULL)
        {
            free(points);
            free(item);
            return NULL;
        }

        memset(points, 0, pointsSize);
        memset(pointTypes, 0, typesSize);
    }
    else
    {
        points     = NULL;
        pointTypes = NULL;
    }

    item->Points     = points;
    item->PointTypes = pointTypes;
    item->PointCount = PointCount;
    item->Type       = Type;
    item->ClosePath  = ClosePath;

    return item;
}

/* -------------------------------------------------------------
   LineItemInitWithPoints
   ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
BOOL
GR_CALL
LineItemInitWithPoints(
    _Inout_ PSLineItem Item,
    _In_reads_(PointCount * 2) PCFLOAT Points,
    _In_reads_(PointCount) PCLINE_POINT_TYPE PointTypes,
    _In_ UINT               PointCount,
    _In_ LINE_ITEM_TYPE     Type,
    _In_ BOOL               ClosePath
    )
{
    if (Item == NULL)
    {
        return FALSE;
    }

    Item->Points     = (PFLOAT)Points;
    Item->PointTypes = (PLINE_POINT_TYPE)PointTypes;
    Item->PointCount = PointCount;
    Item->Type       = Type;
    Item->ClosePath  = ClosePath;

    return TRUE;
}

/* -------------------------------------------------------------
   LineItemFree
   ------------------------------------------------------------- */
VOID
GR_CALL
LineItemFree(
    _In_ PSLineItem Item
    )
{
    if (Item != NULL)
    {
        free(Item);
    }
}

/* -------------------------------------------------------------
   LineItemFreeWithPoints
   ------------------------------------------------------------- */
VOID
GR_CALL
LineItemFreeWithPoints(
    _In_ PSLineItem Item
    )
{
    if (Item != NULL)
    {
        if (Item->Points != NULL)
        {
            free(Item->Points);
            Item->Points = NULL;
        }

        if (Item->PointTypes != NULL)
        {
            free(Item->PointTypes);
            Item->PointTypes = NULL;
        }

        Item->PointCount = 0;
        free(Item);
    }
}
