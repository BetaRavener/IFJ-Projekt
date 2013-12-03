/**
 * @file expr.h
 * @brief Declares interface of the bottom-up parser
 **/
#ifndef EXPR_H
#define EXPR_H

#include "parser.h"
#include "vector.h"
#include "token.h"

/**
 * Expression token used in bottom-up parsing
 **/
typedef struct
{
    union
    {
        Token *token;
        int64_t stackOffset;
    };
    uint8_t type;
} ExprToken;

void initExpr();
void deinitExpr();
uint32_t expr(uint32_t resultOffset, uint32_t *maxStackPosUsed);

void initExprToken(ExprToken *token);
void deleteExprToken(ExprToken *token);
void copyExprToken(const ExprToken *src, ExprToken *dest);

typedef ExprToken* ExprTokenVectorIterator;
typedef const ExprToken* ConstExprTokenVectorIterator;

#define VECTOR_STRUCT_ITEM
#define VECTOR_ITEM ExprToken
#define VECTOR_ITERATOR ExprTokenVectorIterator
#include "vector_template.h"
#undef VECTOR_ITERATOR
#undef VECTOR_ITEM
#undef VECTOR_STRUCT_ITEM

#endif
