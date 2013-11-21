#include "expr.h"
#include "nierr.h"
#include "token_vector.h"
#include "context.h"
#include "ial.h"

// definitions from parser which need to be present also in expression parser
extern ConstTokenVectorIterator tokensIt;
extern SymbolTable *globalSymbolTable;
extern Context *currentContext;
extern Vector *constantsTable;

static Vector *exprVector = NULL;
static ExprToken endToken;

typedef enum
{
    Low,
    High,
    Equal,
    Error
} TokenPrecedence;

static uint8_t precedenceTable[STT_Semicolon][STT_Semicolon] =
{
//      +       -       *       /       .       <       >       <=      >=      ===     !==     (       )       func    ,       $       var
    {   High,   High,   Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // +
    {   High,   High,   Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // -
    {   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // *
    {   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // /
    {   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // .
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // <
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // >
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // <=
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // >=
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // ===
    {   Low,    Low,    Low,    Low,    Low,    High,   High,   High,   High,   High,   High,   Low,    High,   Low,    High,   High,   Low   },  // !==
    {   Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Equal,  Low,    Equal,  Error,  Low   },  // (
    {   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   Error,  High,   Error,  High,   High,   Error },  // )
    {   Error,  Error,  Error,  Error,  Error,  Error,  Error,  Error,  Error,  Error,  Error,  Equal,  Error,  Error,  Error,  Error,  Error },  // func
    {   Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Equal,  Low,    Equal,  High,   Low   },  // ,
    {   Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Low,    Error,  Low,    Error,  Error,  Low   },  // $
    {   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   High,   Error,  High,   Error,  High,   High,   Error }   // var
};

void initExpr()
{
    exprVector = newExprTokenVector();
    endToken.token = newToken();
    endToken.token->type = STT_EOF;
    endToken.type = Terminal;
}

void deinitExpr()
{
    free(endToken.token);
    vectorClearExprToken(exprVector);
    freeExprTokenVector(&exprVector);
}

ExprToken* findTopmostTerminal()
{
    ExprTokenVectorIterator itr = vectorEndExprToken(exprVector);

    while (itr != vectorBeginExprToken(exprVector)) {
        itr--;
        if (itr->type == Terminal)
            return itr;
    }

    return NULL;
}

static inline uint8_t tokenTypeToExprType(uint8_t tokenType)
{
    if (tokenType >= STT_Semicolon)
        return STT_EOF;

    if (tokenType >= STT_Variable && tokenType <= STT_String)
        return STT_Variable;

    return tokenType;
}

uint8_t reduceMultiparamFunc(uint32_t stackPos)
{
    if (stackPos == 0)
        return 0;

    ExprTokenVectorIterator first = vectorAt(exprVector, stackPos);
    ExprTokenVectorIterator second = vectorAt(exprVector, stackPos - 1);
    if (first->type == NonTerminal) { // got E,E..) expect , or (
        if (second->type == Terminal && second->token->type == STT_Comma) { // we got ,E,E..), enter recursion
            if (!reduceMultiparamFunc(stackPos - 2))
                return 0;
printf(",E");
        }
        else if (second->type == Terminal && second->token->type == STT_LeftBracket) { // got (E,..,E), expect id
            if (stackPos < 2)
                return 0;

            ExprTokenVectorIterator id = vectorAt(exprVector, stackPos - 2);
            if (id->type != Terminal)
                return 0;

            if (id->token->type != STT_Identifier)
                return 0;

            // TODO generate instructions
printf("func(E");
            id->type = NonTerminal;
            vectorPopExprToken(exprVector);
            vectorPopExprToken(exprVector);
            return 1;
        }
    }

    // TODO generate instructions
    vectorPopExprToken(exprVector);
    vectorPopExprToken(exprVector);
    return 1;
}

uint8_t reduce(ExprToken *topTerm)
{
    switch (topTerm->token->type) {
        case STT_Variable:
            // todo: generate instruction
puts("Rule: E -> var");
            topTerm->type = NonTerminal;
            break;
        case STT_Number:
puts("Rule: E -> int");
            topTerm->type = NonTerminal;
            break;
        case STT_Double:
puts("Rule: E -> double");
            topTerm->type = NonTerminal;
            break;
        case STT_Equal:
        case STT_NotEqual:
        case STT_Less:
        case STT_Greater:
        case STT_LessEqual:
        case STT_GreaterEqual:
        case STT_Minus:
        case STT_Multiply:
        case STT_Divide:
        case STT_Dot:
        case STT_Plus: {
if (topTerm->token->type == STT_Plus)
    puts("Rule: E -> E + E");
else if (topTerm->token->type == STT_Dot)
    puts("Rule: E -> E . E");
else if (topTerm->token->type == STT_Divide)
    puts("Rule: E -> E / E");
else if (topTerm->token->type == STT_Multiply)
    puts("Rule: E -> E * E");
else if (topTerm->token->type == STT_Minus)
    puts("Rule: E -> E - E");
else if (topTerm->token->type == STT_Equal)
    puts("Rule: E -> E === E");
else if (topTerm->token->type == STT_NotEqual)
    puts("Rule: E -> E !== E");
else if (topTerm->token->type == STT_Less)
    puts("Rule: E -> E < E");
else if (topTerm->token->type == STT_Greater)
    puts("Rule: E -> E > E");
else if (topTerm->token->type == STT_LessEqual)
    puts("Rule: E -> E <= E");
else if (topTerm->token->type == STT_GreaterEqual)
    puts("Rule: E -> E >= E");

            uint32_t stackSize = vectorSize(exprVector);
            if (stackSize < 3) {
                setError(ERR_Syntax);
                return 0;
            }

            ExprTokenVectorIterator operand1 = vectorAt(exprVector, stackSize - 1);
            if (operand1->type != NonTerminal) {
                setError(ERR_Syntax);
                return 0;
            }

            ExprTokenVectorIterator operator = vectorAt(exprVector, stackSize - 2);
            if (operator != topTerm) {
                setError(ERR_Syntax);
                return 0;
            }

            ExprTokenVectorIterator operand2 = vectorAt(exprVector, stackSize - 3);
            if (operand2->type != NonTerminal) {
                setError(ERR_Syntax);
                return 0;
            }

            vectorPopExprToken(exprVector);
            vectorPopExprToken(exprVector);
            break;
        }
        case STT_RightBracket: {
            uint32_t stackSize = vectorSize(exprVector);
            if (stackSize < 3) {
                setError(ERR_Syntax);
                return 0;
            }

            ExprTokenVectorIterator nextTerm = vectorAt(exprVector, stackSize - 1);
            if (nextTerm == topTerm) { // at top must be )
                nextTerm = vectorAt(exprVector, stackSize - 2);

                if (nextTerm->type == Terminal && nextTerm->token->type == STT_LeftBracket) { // it's function if there is ()
puts("Rule: E -> func()");
                    nextTerm = vectorAt(exprVector, stackSize - 3);

                    if (nextTerm->type == Terminal && nextTerm->token->type == STT_Identifier) {
                        vectorPopExprToken(exprVector);
                        vectorPopExprToken(exprVector);
                        nextTerm->type = NonTerminal;
                    }
                }
                else if (nextTerm->type == NonTerminal) { // we have E) and are not sure what it is
                    nextTerm = vectorAt(exprVector, stackSize - 3);

                    if (nextTerm->type == Terminal && nextTerm->token->type == STT_Comma) { // we have ,E) and it's function
                        uint32_t stackPos = stackSize - 2;

                        if (stackPos < 4) { // not enough space for best case func(E,E)
                            setError(ERR_Syntax);
                            return 0;
                        }

printf("Rule: E -> ");
                        if (!reduceMultiparamFunc(stackPos)) {
                            setError(ERR_Syntax);
                            return 0;
                        }

                        vectorPopExprToken(exprVector);
puts(")");
                    }
                    else if (nextTerm->type == Terminal && nextTerm->token->type == STT_LeftBracket) { // it's (E) and we don't know if it is single param func or just (E)
                        if (stackSize >= 4) {
                            ExprTokenVectorIterator backupTerm = nextTerm; // we need to backup current token for (E) case
                            nextTerm = vectorAt(exprVector, stackSize - 4);

                            if (nextTerm->type == Terminal && nextTerm->token->type == STT_Identifier) { // we have id(E) and it's function
puts("Rule: E -> func(E)");
                                vectorPopExprToken(exprVector);
                                vectorPopExprToken(exprVector);
                                vectorPopExprToken(exprVector);
                                nextTerm->type = NonTerminal;
                            }
                            else { // it's just (E)
puts("Rule: E -> (E)");
                                vectorPopExprToken(exprVector);
                                vectorPopExprToken(exprVector);
                                backupTerm->type = NonTerminal;
                            }
                        }
                        else { // it can only be (E) if we have just 3 items on the stack
puts("Rule: E -> (E)");
                            vectorPopExprToken(exprVector);
                            vectorPopExprToken(exprVector);
                            nextTerm->type = NonTerminal;
                        }
                    }
                }
            }
            break;
        }
        default:
            return 0;
    }

    return 1;
}

void initExprToken(ExprToken *token)
{
    memset(token, 0, sizeof(ExprToken));
}

void deleteExprToken(ExprToken *token)
{
    (void)token;
}

void copyExprToken(ExprToken *src, ExprToken *dest)
{
    dest->type = src->type;
    dest->token = src->token;
}

int32_t expr()
{
    // insert bottom of the stack (special kind of token) to the stack
    uint8_t endOfInput = 0;
    const Token *currentToken = NULL;
    vectorClearExprToken(exprVector);

    while (1) {
        if (!endOfInput) {
            if (tokensIt->type < STT_Semicolon) // invalid token received
                currentToken = tokensIt;
            else {
                currentToken = endToken.token;
                endOfInput = 1;
            }
        }

        ExprToken *topToken = findTopmostTerminal();

        // no terminal found
        if (!topToken) {
            // there is just one non-terminal on the stack, we succeeded
            if (endOfInput && vectorSize(exprVector) == 1 && ((ExprToken*)vectorBack(exprVector))->type == NonTerminal)
                return 1;

            // if there is no topmost terminal and input is right bracket, end successfuly
            // top-down parser will take care of bad input, since he will assume I loaded first token after expression
            if (currentToken->type == STT_RightBracket)
                return 1;

            topToken = &endToken;
        }

        switch (precedenceTable[tokenTypeToExprType(topToken->token->type)][tokenTypeToExprType(currentToken->type)]) {
            case Low:
            case Equal: {
                vectorPushDefaultExprToken(exprVector);
                ExprToken *exprToken = vectorBack(exprVector);
                exprToken->token = (Token*)currentToken;
                exprToken->type = Terminal;
                break;
            }
            case High:
                if (!reduce(topToken))
                    return 0;

                continue;
            case Error:
                setError(ERR_Syntax);
                // @todo Cleanup
                return -1;
        }

        if (!endOfInput)
            tokensIt++;
    }
}