
#include <gera.h>
#include "cJSON.h"


typedef struct {
    gbool is_error;
    char data[];
} ParsingResultLayout;

static void free_parsing_result_val(GeraAllocation* allocation) {
    ParsingResultLayout* data = (ParsingResultLayout*) allocation->data;
    GeraObject* value = (GeraObject*) data->data;
    gera___ref_deleted(value->allocation);
}

GeraObject cjson_to_gera_cjson(const cJSON* v);

GeraObject gera_cjson_parse(GeraString input) {
    const char* parse_end = NULL;
    cJSON* parsed = cJSON_ParseWithLengthOpts(
        input.data, input.length_bytes, &parse_end, 0
    );
    if(parsed == NULL) {
        gint position = -1;
        if(parse_end != NULL) {
            position = 0;
            for(const char* b = input.data; b < parse_end; position += 1) {
                b += gera___codepoint_size(*b);
            }
        }
        cJSON_Delete(parsed);
        gera___ref_deleted(input.allocation);
        GeraAllocation* result = gera___alloc(
            sizeof(ParsingResultLayout) + sizeof(gint), NULL
        );
        ParsingResultLayout* data = (ParsingResultLayout*) result->data;
        data->is_error = 1;
        *((gint*) data->data) = position;
        return (GeraObject) { .allocation = result };
    }
    GeraObject value = cjson_to_gera_cjson(parsed);
    cJSON_Delete(parsed);
    gera___ref_deleted(input.allocation);
    GeraAllocation* result = gera___alloc(
        sizeof(ParsingResultLayout) + sizeof(GeraObject),
        &free_parsing_result_val
    );
    ParsingResultLayout* data = (ParsingResultLayout*) result->data;
    data->is_error = 0;
    *((GeraObject*) data->data) = value;
    return (GeraObject) { .allocation = result };
}

gint gera_cjson_res_err_idx(GeraObject result) {
    ParsingResultLayout* data = (ParsingResultLayout*) result.allocation->data;
    gint position = *((gint*) data->data);
    gera___ref_deleted(result.allocation);
    return position;
}

GeraObject gera_cjson_res_val(GeraObject result) {
    ParsingResultLayout* data = (ParsingResultLayout*) result.allocation->data;
    GeraObject value = *((GeraObject*) data->data);
    gera___ref_copied(value.allocation);
    gera___ref_deleted(result.allocation);
    return value;
}


#define GERA_CJSON_NULL 0
#define GERA_CJSON_BOOL 1
#define GERA_CJSON_NUMBER 2
#define GERA_CJSON_STRING 3
#define GERA_CJSON_ARRAY 4
#define GERA_CJSON_OBJECT 5

typedef struct {
    gint val_type;
    char data[];
} GeraCJSONLayout;

typedef struct {
    GeraArray keys;
    GeraArray vals;
} GeraCJSONObjectLayout;

static void free_gera_cjson_string_variant(GeraAllocation* allocation) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) allocation->data;
    GeraString* value = (GeraString*) header->data;
    gera___ref_deleted(value->allocation);
}

static void free_gera_cjson_array_variant(GeraAllocation* allocation) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) allocation->data;
    GeraArray* value = (GeraArray*) header->data;
    gera___ref_deleted(value->allocation);
}

static void free_gera_cjson_object_variant(GeraAllocation* allocation) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) allocation->data;
    GeraCJSONObjectLayout* data = (GeraCJSONObjectLayout*) header->data;
    gera___ref_deleted(data->keys.allocation);
    gera___ref_deleted(data->vals.allocation);
}

static void free_gera_object_array(GeraAllocation* allocation) {
    size_t size = allocation->size / sizeof(GeraObject);
    GeraObject* values = (GeraObject*) allocation->data;
    for(size_t i = 0; i < size; i += 1) {
        gera___ref_deleted(values[i].allocation);
    }
}

static void free_gera_string_array(GeraAllocation* allocation) {
    size_t size = allocation->size / sizeof(GeraString);
    GeraString* values = (GeraString*) allocation->data;
    for(size_t i = 0; i < size; i += 1) {
        gera___ref_deleted(values[i].allocation);
    }
}

GeraObject cjson_to_gera_cjson(const cJSON* v) {
    switch(v->type) {
        case cJSON_NULL: {
            GeraAllocation* a = gera___alloc(sizeof(GeraCJSONLayout), NULL);
            GeraCJSONLayout* header = (GeraCJSONLayout*) a->data;
            header->val_type = GERA_CJSON_NULL;
            return (GeraObject) { .allocation = a };
        }
        case cJSON_False:
        case cJSON_True: {
            GeraAllocation* a = gera___alloc(
                sizeof(GeraCJSONLayout) + sizeof(gbool), NULL
            );
            GeraCJSONLayout* header = (GeraCJSONLayout*) a->data;
            header->val_type = GERA_CJSON_BOOL;
            *((gbool*) header->data) = v->type == cJSON_True;
            return (GeraObject) { .allocation = a };
        }
        case cJSON_Number: {
            GeraAllocation* a = gera___alloc(
                sizeof(GeraCJSONLayout) + sizeof(gfloat), NULL
            );
            GeraCJSONLayout* header = (GeraCJSONLayout*) a->data;
            header->val_type = GERA_CJSON_NUMBER;
            *((gfloat*) header->data) = v->valuedouble;
            return (GeraObject) { .allocation = a };
        }
        case cJSON_String: {
            GeraAllocation* a = gera___alloc(
                sizeof(GeraCJSONLayout) + sizeof(GeraString),
                &free_gera_cjson_string_variant
            );
            GeraCJSONLayout* header = (GeraCJSONLayout*) a->data;
            header->val_type = GERA_CJSON_STRING;
            *((GeraString*) header->data) = gera___alloc_string(v->valuestring);
            return (GeraObject) { .allocation = a };
        }
        case cJSON_Array: {
            size_t length = cJSON_GetArraySize(v);
            GeraAllocation* vals_a = gera___alloc(
                sizeof(GeraObject) * length,
                &free_gera_object_array
            );
            GeraObject* vals = (GeraObject*) vals_a->data;
            size_t i = 0;
            const cJSON* item;
            cJSON_ArrayForEach(item, v) {
                vals[i] = cjson_to_gera_cjson(item);
                i += 1;
            }
            GeraAllocation* arr_a = gera___alloc(
                sizeof(GeraCJSONLayout) + sizeof(GeraArray),
                &free_gera_cjson_array_variant
            );
            GeraCJSONLayout* header = (GeraCJSONLayout*) arr_a->data;
            header->val_type = GERA_CJSON_ARRAY;
            *((GeraArray*) header->data) = (GeraArray) { 
                .allocation = vals_a, .length = length
            };
            return (GeraObject) { .allocation = arr_a };
        }
        case cJSON_Object: {
            size_t length = cJSON_GetArraySize(v);
            GeraAllocation* keys_a = gera___alloc(
                sizeof(GeraString) * length,
                &free_gera_string_array
            );
            GeraString* keys = (GeraString*) keys_a->data;
            GeraAllocation* vals_a = gera___alloc(
                sizeof(GeraObject) * length,
                &free_gera_object_array
            );
            GeraObject* vals = (GeraObject*) vals_a->data;
            size_t i = 0;
            const cJSON* item;
            cJSON_ArrayForEach(item, v) {
                keys[i] = gera___alloc_string(item->string);
                vals[i] = cjson_to_gera_cjson(item);
                i += 1;
            }
            GeraAllocation* obj_a = gera___alloc(
                sizeof(GeraCJSONLayout) + sizeof(GeraCJSONObjectLayout),
                &free_gera_cjson_object_variant
            );
            GeraCJSONLayout* header = (GeraCJSONLayout*) obj_a->data;
            header->val_type = GERA_CJSON_OBJECT;
            GeraCJSONObjectLayout* data = (GeraCJSONObjectLayout*) header->data;
            data->keys = (GeraArray) { .allocation = keys_a, .length = length };
            data->vals = (GeraArray) { .allocation = vals_a, .length = length };
            return (GeraObject) { .allocation = obj_a };
        }
        default:
            gera___panic("Unable to convert cJSON value to Gera object!");
    }
}


gbool gera_cjson_val_bool(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    gbool value = *((gbool*) header->data);
    gera___ref_deleted(v.allocation);
    return value;
}

gfloat gera_cjson_val_number(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    gfloat value = *((gfloat*) header->data);
    gera___ref_deleted(v.allocation);
    return value;
}

GeraString gera_cjson_val_string(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    GeraString value = *((GeraString*) header->data);
    gera___ref_copied(value.allocation);
    gera___ref_deleted(v.allocation);
    return value;
}

GeraArray gera_cjson_val_array(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    GeraArray value = *((GeraArray*) header->data);
    gera___ref_copied(value.allocation);
    gera___ref_deleted(v.allocation);
    return value;
}

GeraArray gera_cjson_val_obj_keys(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    GeraCJSONObjectLayout* data = (GeraCJSONObjectLayout*) header->data;
    GeraArray keys = data->keys;
    gera___ref_copied(keys.allocation);
    gera___ref_deleted(v.allocation);
    return keys;
}

GeraArray gera_cjson_val_obj_vals(GeraObject v) {
    GeraCJSONLayout* header = (GeraCJSONLayout*) v.allocation->data;
    GeraCJSONObjectLayout* data = (GeraCJSONObjectLayout*) header->data;
    GeraArray vals = data->vals;
    gera___ref_copied(vals.allocation);
    gera___ref_deleted(v.allocation);
    return vals;
}