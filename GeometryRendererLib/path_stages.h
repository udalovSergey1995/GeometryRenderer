/* path_stages.h — контейнер этапов преобразования пути (Win32-стиль, SAL2, чистый C)  */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/*=============================================================================
 *  Двусвязный список (аналог LIST_ENTRY)
 *===========================================================================*/

typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

/*=============================================================================
 *  Типы этапов преобразования
 *===========================================================================*/

typedef enum _PathStageType
{
    PathStageTypeInvalid = 0,
    PathStageTypeLogicalCurve,
    PathStageTypeApproximated,
    PathStageTypeDashPattern,
    PathStageTypeThickLine,
    PathStageTypeRasterSlab,
    PathStageTypeCount
} EPathStageType;

/*=============================================================================
 *  Callback для освобождения данных этапа
 *===========================================================================*/

typedef
VOID
(GR_CALL *PFN_PATH_STAGE_FREE)(
    _In_ PVOID StageData
);

/*=============================================================================
 *  Элемент списка этапов
 *===========================================================================*/
struct _PathStageEntry;

/* Единая сигнатура обработчиков свойств всех path stage айтемов.
   pOutData == NULL: запрос размера. pOutLen: размер результата в байтах.
   TRUE: свойство получено, FALSE: неизвестное свойство/ошибка. */
typedef
BOOL
(GR_CALL *GET_PATH_STAGE_PROPERTY)(
    _In_ struct _PathStageEntry *pEntry,
    _In_ EStageProps eProp,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
);

typedef struct _PathStageEntry
{
    SBaseObject Base;

    LIST_ENTRY ListEntry;

    EPathStageType StageType;

    PVOID StageData;

    PFN_PATH_STAGE_FREE FreeCallback;

    GET_PATH_STAGE_PROPERTY GetPropertyCallback;

} SPathStageEntry, *PSPathStageEntry;

/*=============================================================================
 *  Контейнер этапов
 *===========================================================================*/
typedef struct _PathStages
{
    SBaseObject Base;

    LIST_ENTRY Head;

    UINT32 Count;
} SPathStages, *PSPathStages;

/*=============================================================================
 *  Макрос итерации по этапам
 *===========================================================================*/

#define PATH_STAGES_FOREACH(PathStagesVar, EntryVar)                                               \
    for ((EntryVar) = PathStagesGetFirstStage(PathStagesVar); (EntryVar) != NULL;                  \
         (EntryVar) =                                                                              \
             (((EntryVar)->ListEntry.Flink == &(PathStagesVar)->Head)                              \
                  ? NULL                                                                           \
                  : CONTAINING_RECORD((EntryVar)->ListEntry.Flink, SPathStageEntry, ListEntry)))

/*=============================================================================
 *  Прототипы функций
 *===========================================================================*/

VOID
GR_CALL
PathStagesInitialize(
    _Out_ PSPathStages PathStages
);

VOID
GR_CALL
PathStagesUninitialize(
    _Inout_ PSPathStages PathStages
);

_Check_return_ _Success_(return != NULL) _Ret_maybenull_ PSPathStages GR_CALL
    PathStagesCreate(VOID);

VOID
GR_CALL
PathStagesDestroy(
    _In_opt_ PSPathStages PathStages
);

_Check_return_ _Success_(return != 0) INT32 GR_CALL
    PathStagesAddStage(_Inout_ PSPathStages PathStages,
                       _In_ EPathStageType StageType,
                       _In_ PVOID StageData,
                       _In_opt_ PFN_PATH_STAGE_FREE FreeCallback,
                       _In_ GET_PATH_STAGE_PROPERTY GetPropertyCallback);

VOID
GR_CALL
PathStagesRemoveStage(
    _Inout_ PSPathStages PathStages,
    _In_ PSPathStageEntry StageEntry
);

VOID
GR_CALL
PathStagesClear(
    _Inout_ PSPathStages PathStages
);

/* Откат append-only операции. NULL означает исходно пустой список.
   Checkpoint должен принадлежать этому контейнеру и оставаться живым. */
_Check_return_
BOOL
GR_CALL
PathStagesRollbackTo(
    _Inout_ PSPathStages PathStages,
    _In_opt_ PSPathStageEntry Checkpoint
);

_Ret_maybenull_ _Post_writable_byte_size_(sizeof(SPathStageEntry)) PSPathStageEntry GR_CALL
    PathStagesGetLastStage(_In_ PSPathStages PathStages);

_Ret_maybenull_ _Post_writable_byte_size_(sizeof(SPathStageEntry)) PSPathStageEntry GR_CALL
    PathStagesGetFirstStage(_In_ PSPathStages PathStages);

UINT32
GR_CALL
PathStagesGetCount(
    _In_ PSPathStages PathStages
);

typedef VOID
GR_CALL(GR_CALL *PFN_PATH_STAGE_ENUM)(_In_ PSPathStageEntry StageEntry, _In_opt_ PVOID Context);

VOID
GR_CALL
PathStagesEnum(
    _In_ PSPathStages PathStages,
    _In_ PFN_PATH_STAGE_ENUM Callback,
    _In_opt_ PVOID Context
);

BOOL
GR_CALL
PathStagesEntryGetProperty(
    _In_ PSPathStageEntry pEntry,
    _In_ EStageProps eProp,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
);

BOOL
GR_CALL
PathStagesGetProperty(
    _In_ PVOID pObject,
    _In_ INT eProp,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
);

#ifdef __cplusplus
}
#endif
