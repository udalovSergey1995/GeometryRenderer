#include "stdafx.h"

BOOL
GR_CALL
CopyObjectProperty(
    _Maybenull_ PCVOID pData,
    _In_ INT iDataLen,
    _Maybenull_ PVOID pOutData,
    _Out_ PINT pOutLen
)
{
    if (!pOutLen)
    {
        return FALSE;
    }

    *pOutLen = 0;
    if (iDataLen < 0 || (iDataLen > 0 && !pData))
    {
        return FALSE;
    }

    *pOutLen = iDataLen;
    if (pOutData && iDataLen > 0)
    {
        memcpy(pOutData, pData, (size_t)iDataLen);
    }

    return TRUE;
}

/* pOutLen: ёмкость буфера на входе, необходимый размер на выходе.
   Callback сохраняет прежнюю сигнатуру: перед копированием запрашиваем размер. */
_Check_return_
GR_EXPORT
INT
GR_CALL
GetObjectProperty(
    _In_ PVOID pObject,
    _In_ INT eProp,
    _Maybenull_ PVOID pOutData,
    _Inout_ PINT pOutLen
)
{
    INT capacity;
    INT required = 0;
    if (!pOutLen) return FALSE;
    capacity = *pOutLen;
    *pOutLen = 0;
    if (!pObject) return FALSE;
    if (!PathStagesGetProperty(pObject, eProp, NULL, &required)) return FALSE;
    *pOutLen = required;
    if (!pOutData) return TRUE;
    if (capacity < required || capacity < 0) return FALSE;
    return PathStagesGetProperty(pObject, eProp, pOutData, pOutLen);
}
