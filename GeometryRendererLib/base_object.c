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

_Check_return_ GR_EXPORT INT GR_CALL
GetObjectProperty(_In_ PVOID pObject,
                  _In_ INT eProp,
                  _Maybenull_ PVOID pOutData,
                  _Out_ PINT pOutLen)
{
    int result = 0;

    if (pOutLen)
    {
        *pOutLen = 0;
    }

    if (!pObject || !pOutLen)
    {
        pOutData = NULL;
        goto ret_pt;
    }

    *pOutLen = 0;

    EObjectType type = ((PSBaseObject)pObject)->type;

    if (type >= EOT_TypesCount)
    {
        pOutData = NULL;
        goto ret_pt;
    }

    switch (type)
    {
    case EOT_BadObject:
        break;

    case EOT_PipeLine:
    {
        break;
    }

    case EOT_PathStages:
    case EOT_PipeStageEntry:
    {
        result = PathStagesGetProperty(pObject, eProp, pOutData, pOutLen);
        break;
    }
    default:
        break;
    }

ret_pt:

    return result;
}
