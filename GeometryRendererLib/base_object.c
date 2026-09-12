#include "stdafx.h"

_Check_return_
INT
GR_CALL
GetObjectProperty(
	_In_ PVOID pObject,
	_In_ INT eProp,
	_Maybenull_ PVOID pOutData,
	_Out_ PINT pOutLen
)
{
	int result = 0;

	if (!pObject || !pOutLen)
	{
		pOutData = NULL;
		goto ret_pt;
	}

	*pOutLen = 0;

	EObjectType type = ((PSBaseObject)pObject)->type;

	INT iOutLen = 0;

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

