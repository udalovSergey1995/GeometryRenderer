#pragma once

#define PROP_NAMESPACE(_items){ _items }

typedef enum _EStageProps
{
	ESP_BadObject,

	ESP_TYPE,

	ESP_LOGICAL_ITEM_START,

		ESP_IS_CLOSED,

		ESP_IS_BESIERE,

		ESP_POINTS_COUNT,

	ESP_LOGICAL_ITEM_END
} EStageProps;

#define IS_LOGICAL_ITEM_PROPS(__prop) \
	(__prop > ESP_LOGICAL_ITEM_START && __prop < ESP_LOGICAL_ITEM_END)

