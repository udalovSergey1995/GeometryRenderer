#pragma once

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
   Callback для освобождения данных этапа в PathStages
   ------------------------------------------------------------- */
VOID
GR_CALL
FlattenItemFree(
    _In_ PVOID StageData
);


