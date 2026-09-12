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

_Check_return_
INT
GR_CALL
GetObjectProperty(
	_In_ PVOID pObject,
	_In_ INT eProp,
	_Maybenull_ PVOID pOutData,
	_Out_ PINT pOutLen
);
