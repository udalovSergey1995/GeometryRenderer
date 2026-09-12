#pragma once
#ifndef _FLATTEN_ITEM_H_
#define _FLATTEN_ITEM_H_

/* -------------------------------------------------------------
   Параметры аппроксимации по умолчанию
   ------------------------------------------------------------- */
#define FLATTEN_DEFAULT_TOLERANCE 0.25f

   /* -------------------------------------------------------------
      Создание аппроксимированного (flattened) айтема из логического
      ------------------------------------------------------------- */
_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
FlattenItemInit(
    _In_ PSLineItem LineItem
);

_Check_return_
_Ret_maybenull_
PSLineItem
GR_CALL
FlattenItemInitEx(
    _In_ PSLineItem LineItem,
    _In_ FLOAT      Flatness
);

/* -------------------------------------------------------------
   Освобождение flatten айтема
   ------------------------------------------------------------- */
VOID
GR_CALL
FlattenItemFree(
    _Inout_ PSLineItem FlattenItem
);

/* -------------------------------------------------------------
   Callback для освобождения данных этапа в PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
FlattenItemFree(
    _In_ PVOID StageData
);

#endif /* _FLATTEN_ITEM_H_ */

