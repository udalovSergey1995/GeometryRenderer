/* path_stages.c — реализация контейнера этапов преобразования пути */

#include "stdafx.h"

/*=============================================================================
 *  Внутренние функции списка
 *===========================================================================*/

static
VOID
GR_CALL
PathStagesInitializeListHead(
    _Out_ PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead;
    ListHead->Blink = ListHead;
}

static
UINT8
GR_CALL
PathStagesIsListEmpty(
    _In_ PLIST_ENTRY ListHead
)
{
    return (UINT8)(ListHead->Flink == ListHead);
}

static
VOID
GR_CALL
PathStagesInsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

static
VOID
GR_CALL
PathStagesRemoveEntryList(
    _Inout_ PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Blink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
}

/*=============================================================================
 *  Инициализация / Создание / Уничтожение
 *===========================================================================*/

VOID
GR_CALL
PathStagesInitialize(
    _Out_ PSPathStages PathStages
)
{
    PathStagesInitializeListHead(&PathStages->Head);
    PathStages->Count = 0;

    PathStages->Base.type = EOT_PathStages;
}

VOID
GR_CALL
PathStagesUninitialize(
    _Inout_ PSPathStages PathStages
)
{
    PathStagesClear(PathStages);
}

_Check_return_
_Success_(return != NULL)
_Ret_maybenull_
PSPathStages
GR_CALL
PathStagesCreate(
    VOID
)
{
    PSPathStages PathStages;

    PathStages = (PSPathStages)malloc(sizeof(SPathStages));
    if (PathStages != NULL)
    {
        PathStagesInitialize(PathStages);
    }

    return PathStages;
}

VOID
GR_CALL
PathStagesDestroy(
    _In_opt_ PSPathStages PathStages
)
{
    if (PathStages != NULL)
    {
        PathStagesUninitialize(PathStages);
        //free(PathStages);
    }
}

/*=============================================================================
 *  Манипуляции с этапами
 *===========================================================================*/

_Check_return_
_Success_(return != 0)
INT32
GR_CALL
PathStagesAddStage(
    _Inout_ PSPathStages PathStages,
    _In_ EPathStageType StageType,
    _In_ PVOID StageData,
    _In_opt_ PFN_PATH_STAGE_FREE FreeCallback
)
{
    PSPathStageEntry Entry;
    PSPathStageEntry LastStage;

    if (PathStages == NULL || StageData == NULL || StageType == PathStageTypeInvalid)
    {
        return 0;
    }

    //
    // Правило: подряд одинаковый этап не может повторяться
    //
    LastStage = PathStagesGetLastStage(PathStages);
    if (LastStage != NULL && LastStage->StageType == StageType)
    {
        return 0;
    }

    Entry = (PSPathStageEntry)malloc(sizeof(SPathStageEntry));
    if (Entry == NULL)
    {
        return 0;
    }

    Entry->StageType = StageType;
    Entry->StageData = StageData;
    Entry->FreeCallback = FreeCallback;
    Entry->Base.type = EOT_PipeStageEntry;

    PathStagesInsertTailList(&PathStages->Head, &Entry->ListEntry);
    PathStages->Count++;

    return 1;
}

VOID
GR_CALL
PathStagesRemoveStage(
    _Inout_ PSPathStages PathStages,
    _In_ PSPathStageEntry StageEntry
)
{
    if (PathStages == NULL || StageEntry == NULL)
    {
        return;
    }

    PathStagesRemoveEntryList(&StageEntry->ListEntry);
    PathStages->Count--;

    if (StageEntry->FreeCallback != NULL)
    {
        StageEntry->FreeCallback(StageEntry->StageData);
    }

    free(StageEntry);
}

VOID
GR_CALL
PathStagesClear(
    _Inout_ PSPathStages PathStages
)
{
    PSPathStageEntry Entry;
    PLIST_ENTRY Current;
    PLIST_ENTRY Next;

    if (PathStages == NULL)
    {
        return;
    }

    Current = PathStages->Head.Flink;
    while (Current != &PathStages->Head)
    {
        Next = Current->Flink;
        Entry = CONTAINING_RECORD(Current, SPathStageEntry, ListEntry);

        if (Entry->FreeCallback != NULL)
        {
            Entry->FreeCallback(Entry->StageData);
        }

        free(Entry);
        Current = Next;
    }

    PathStagesInitializeListHead(&PathStages->Head);
    PathStages->Count = 0;
}

/*=============================================================================
 *  Доступ к этапам
 *===========================================================================*/

_Ret_maybenull_
_Post_writable_byte_size_(sizeof(SPathStageEntry))
PSPathStageEntry
GR_CALL
PathStagesGetLastStage(
    _In_ PSPathStages PathStages
)
{
    if (PathStages == NULL || PathStagesIsListEmpty(&PathStages->Head))
    {
        return NULL;
    }

    return CONTAINING_RECORD(PathStages->Head.Blink, SPathStageEntry, ListEntry);
}

_Ret_maybenull_
_Post_writable_byte_size_(sizeof(SPathStageEntry))
PSPathStageEntry
GR_CALL
PathStagesGetFirstStage(
    _In_ PSPathStages PathStages
)
{
    if (PathStages == NULL || PathStagesIsListEmpty(&PathStages->Head))
    {
        return NULL;
    }

    return CONTAINING_RECORD(PathStages->Head.Flink, SPathStageEntry, ListEntry);
}

UINT32
GR_CALL
PathStagesGetCount(
    _In_ PSPathStages PathStages
)
{
    if (PathStages == NULL)
    {
        return 0;
    }

    return PathStages->Count;
}

/*=============================================================================
 *  Итерация
 *===========================================================================*/

VOID
GR_CALL
PathStagesEnum(
    _In_ PSPathStages PathStages,
    _In_ PFN_PATH_STAGE_ENUM Callback,
    _In_opt_ PVOID Context
)
{
    PSPathStageEntry Entry;

    if (PathStages == NULL || Callback == NULL)
    {
        return;
    }

    PATH_STAGES_FOREACH(PathStages, Entry)
    {
        Callback(Entry, Context);
    }
}

BOOL
GR_CALL
PathStagesEntryGetProperty(
    _In_ PSPathStageEntry pEntry,
    _In_ EStageProps eProp,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
)
{
    BOOL fResult = FALSE;

    BOOL fIsFree = FALSE;
    PBYTE pTmpOutData = NULL;
    INT iOutLen = 0;

    if (IS_LOGICAL_ITEM_PROPS(eProp) 
        && 
        (pEntry->StageType == PathStageTypeLogicalCurve 
            || pEntry->StageType == PathStageTypeApproximated
            || pEntry->StageType == PathStageTypeDashPattern))
    {
        PSLineItem pLine = pEntry->StageData;

        switch (eProp)
        {
            case ESP_IS_CLOSED:
            {
                iOutLen = sizeof(pLine->ClosePath);
                pTmpOutData = (PBYTE) & pLine->ClosePath;

                fResult = TRUE;
                break;
            }
            case ESP_IS_BESIERE:
            {
                iOutLen = sizeof(BOOL);
                pTmpOutData = malloc(iOutLen);

                if (!pTmpOutData)
                {
                    pTmpOutData = NULL;
                    fResult = FALSE;
                    fIsFree = FALSE;
                    break;
                }

                BOOL* val = (BOOL *)pTmpOutData;

                (*val) = (pLine->Type == LineItemType_Bezier);

                fIsFree = TRUE;
                fResult = TRUE;
                break;
            }
            case ESP_POINTS_COUNT:
            {
                iOutLen = sizeof(pLine->PointCount);
                pTmpOutData = malloc(sizeof(INT));

                if (!pTmpOutData)
                {
                    pTmpOutData = NULL;
                    fResult = FALSE;
                    fIsFree = FALSE;
                    break;
                }

                PINT val = (PINT)pTmpOutData;

                (*val) = pLine->PointCount;

                fIsFree = TRUE;
                fResult = TRUE;
                break;
            }
            default:
                fResult = FALSE;
                break;
        }
    }
    else if (IS_DASH_PATTERN_PROPS(eProp)
        && pEntry->StageType == PathStageTypeDashPattern)
    {
        PSDashPatternItem pDashItem = (PSDashPatternItem)pEntry->StageData;

        __debugbreak();

		if (!pDashItem)
			return FALSE;

		switch (eProp)
		{
			case ESP_DASH_COUNT:
			{
				iOutLen = sizeof(INT);
				pTmpOutData = malloc(iOutLen);

				if (!pTmpOutData)
				{
					pTmpOutData = NULL;
					fResult = FALSE;
					fIsFree = FALSE;
					break;
				}

				INT* val = pTmpOutData;
				(*val) = (INT)pDashItem->DashCount;

				fIsFree = TRUE;
				fResult = TRUE;
				break;
			}

			case ESP_DASH_OFFSET:
			{
				iOutLen = sizeof(FLOAT);
				pTmpOutData = malloc(iOutLen);

				if (!pTmpOutData)
				{
					pTmpOutData = NULL;
					fResult = FALSE;
					fIsFree = FALSE;
					break;
				}

				FLOAT* val = pTmpOutData;
				(*val) = pDashItem->DashOffset;

				fIsFree = TRUE;
				fResult = TRUE;
				break;
			}

			default:
				fResult = FALSE;
				break;
		}
    }
    else
    {
        switch (eProp)
        {
            case ESP_TYPE:
            {
                iOutLen = sizeof(pEntry->StageType);
                pTmpOutData = (PBYTE) & pEntry->StageType;

                fResult = TRUE;
                break;
            }
            default:
                fResult = FALSE;
                break;
        }
    }

    *pOutLen = iOutLen;
    if (pTmpOutData && pOutData)
    {
        memcpy(pOutData, pTmpOutData, iOutLen);
    }

    if (fIsFree)
    {
        free(pTmpOutData);
    }

    return fResult;
}

BOOL
GR_CALL
PathStagesGetProperty(
	_In_ PVOID pObject,
	_In_ INT eProp,
	_Maybenull_ PVOID pOutData,
	_Out_ PINT pOutLen
)
{
    if (!pOutLen)
    {
        return FALSE;
    }

    EObjectType type = ((PSBaseObject)pObject)->type;

    *pOutLen = 0;

    switch (type)
    {           
        case EOT_PathStages:
        {
            PSPathStages pStages = pObject;

            __debugbreak();

            return FALSE;
        }
        case EOT_PipeStageEntry:
		{
			PSPathStageEntry pEntry = pObject;

            return PathStagesEntryGetProperty(pEntry, eProp, pOutData, pOutLen);
		}
    default:
        break;
    }

    return FALSE;
}


