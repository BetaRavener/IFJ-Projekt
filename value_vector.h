#ifndef VALUE_VECTOR_H
#define VALUE_VECTOR_H

#include "vector.h"
#include "symbol.h"

typedef Value* ValueVectorIterator;
typedef const Value* ConstValueVectorIterator;

#define VECTOR_STRUCT_ITEM
#define VECTOR_ITEM Value
#define VECTOR_ITERATOR ValueVectorIterator
#include "vector_template.h"
#undef VECTOR_ITERATOR
#undef VECTOR_ITEM
#undef VECTOR_STRUCT_ITEM

static inline void vectorPushIndexValue(Vector *vec, uint32_t index)
{
    if (index >= vec->size) {
        setError(ERR_Range);
        return;
    }

    if (vec->size == vec->capacity) {
        if (vec->capacity == 0)
            vectorReserve(vec, VectorDefaultCapacity);
        else
            vectorReserve(vec, vec->capacity * VectorResizeIncRate);
        if (getError())
            return;
    }

    copyValue((const Value*)(vec->data + index * vec->itemSize), (Value*)vec->end);
    vec->size++;
    vec->end += vec->itemSize;
}

#endif
