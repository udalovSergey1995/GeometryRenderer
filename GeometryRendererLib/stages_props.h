#pragma once

#define PROP_NAMESPACE(_items){ _items }

typedef enum _EStageProps
{
	ESP_BadObject,

	ESP_TYPE,

	// ===== Логическая линия =====
	ESP_LOGICAL_ITEM_START,

		ESP_IS_CLOSED,

		ESP_IS_BESIERE,

		ESP_PATH_LEN,

		ESP_PATH_DATA,

		ESP_PATH_FLAGS,

	ESP_LOGICAL_ITEM_END,

	// ===== GПрерывистые линии =====
	ESP_DASH_PATTERN_START,

		ESP_DASH_COUNT,
		
		ESP_DASH_OFFSET,

	ESP_DASH_PATTERN_END,

	// ===== Толстые линии =====
    ESP_THICK_LINE_START,
        ESP_THICKNESS,

        ESP_JOIN_STYLE,

        ESP_CAP_STYLE,

        ESP_MITER_LIMIT,

    ESP_THICK_LINE_END
} EStageProps;

#define IS_LOGICAL_ITEM_PROPS(__prop) \
	(__prop > ESP_LOGICAL_ITEM_START && __prop < ESP_LOGICAL_ITEM_END)		 

#define IS_DASH_PATTERN_PROPS(__prop) \
	(__prop > ESP_DASH_PATTERN_START && __prop < ESP_DASH_PATTERN_END)

#define IS_THICK_LINE_PROPS(__prop) \
    (__prop > ESP_THICK_LINE_START && __prop < ESP_THICK_LINE_END)
