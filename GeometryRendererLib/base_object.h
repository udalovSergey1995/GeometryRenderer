#pragma once

typedef enum _EObjectType
{
    EOT_BadObject,

    EOT_PipeLine,
    EOT_PathStages,
    EOT_PipeStageEntry,

    EOT_TypesCount,
} EObjectType;

typedef struct _BaseObject
{
    EObjectType type;
} SBaseObject, *PSBaseObject;

/* Общая передача значения свойства без временного выделения памяти.
   pOutLen — выходной размер в байтах; вызывающий выделяет достаточный буфер. */
BOOL
GR_CALL
CopyObjectProperty(
    _Maybenull_ PCVOID pData,
    _In_ INT iDataLen,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
);

_Check_return_ GR_EXPORT INT GR_CALL
GetObjectProperty(_In_ PVOID pObject,
                  _In_ INT eProp,
                  _Maybenull_ PVOID pOutData,
                  _Out_ PINT pOutLen);
