#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h> 
#include <math.h>   

typedef enum {
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_LPAREN, 
    TOKEN_RPAREN, 
    TOKEN_END
} TokenType;

typedef struct {
    TokenType type;
    union {
        double number; 
        char operator; 
    } value;
} Token;


#define STACK_SIZE 256

typedef struct {
    Token items[STACK_SIZE];
    int top;
} TokenStack;

void init_stack(TokenStack *s) {
    s->top = -1;
}

bool is_empty(TokenStack *s) {
    return s->top == -1;
}

bool push(TokenStack *s, Token t) {
    if (s->top >= STACK_SIZE - 1) {

        fprintf(stderr, "Error: tokens full\n");
        return false;
    }
    s->items[++(s->top)] = t;
    return true;
}

Token pop(TokenStack *s) {
    if (is_empty(s)) {
        fprintf(stderr, "Error: Empty token list\n");

        Token error_token = {.type = TOKEN_END};

        return error_token;
    }
    return s->items[(s->top)--];
}

Token peek(TokenStack *s) {
     if (is_empty(s)) {

        fprintf(stderr, "Error: Empty token list\n");

        Token error_token = {.type = TOKEN_END};

        return error_token;
    }
    return s->items[s->top];
}

Token *tokenize(const char *input, int *token_count) {
    Token *tokens = malloc(512 * sizeof(Token));

    if (!tokens) {
        perror("Failed to alloc memory to tokens");

        return NULL;
    }
    int capacity = 512;

    int pos = 0;

    const char *current = input;

    while (*current) {
        if (pos >= capacity - 1) { 
            capacity *= 2;

            Token *new_tokens = realloc(tokens, capacity * sizeof(Token));
            
            if (!new_tokens) {
                perror("Failed relocating token memory");
                free(tokens);
                return NULL;
            }

            tokens = new_tokens;
        }

        if (isdigit(*current) || (*current == '.' && isdigit(current[1]))) {
            char *end;

            tokens[pos].type = TOKEN_NUMBER;

            tokens[pos].value.number = strtod(current, &end);

            current = end; 

            pos++;
        } else if (strchr("+-*/", *current)) {
            tokens[pos].type = TOKEN_OPERATOR;

            tokens[pos].value.operator = *current;

            current++;

            pos++;
        } else if (*current == '(') {
            tokens[pos].type = TOKEN_LPAREN;

            tokens[pos].value.operator = '(';

            current++;

            pos++;
        } else if (*current == ')') {
            tokens[pos].type = TOKEN_RPAREN;

            tokens[pos].value.operator = ')';

            current++;

            pos++;
        } else if (isspace(*current)) {
            current++;
        } else {
            fprintf(stderr, "Unknow Char at input: %c\n", *current);

            free(tokens);

            return NULL;
        }
    }

    tokens[pos].type = TOKEN_END;

    *token_count = pos; 

    return tokens;
}

int get_precedence(char op) {
    switch (op) {
        case '+':
        case '-': return 1;
        case '*':
        case '/': return 2;
        default: return 0; 
    }
}

bool infix_to_rpn(Token *infix, int infix_len, Token *rpn_output, int *rpn_len) {
    TokenStack operator_stack;

    init_stack(&operator_stack);

    int rpn_idx = 0;

    for (int i = 0; i < infix_len; ++i) {
        Token current_token = infix[i];

        switch (current_token.type) {
            case TOKEN_NUMBER:
                rpn_output[rpn_idx++] = current_token;
                break;

            case TOKEN_OPERATOR:
                while (!is_empty(&operator_stack) &&
                       peek(&operator_stack).type == TOKEN_OPERATOR &&

                       get_precedence(peek(&operator_stack).value.operator) >= get_precedence(current_token.value.operator))
                {
                    rpn_output[rpn_idx++] = pop(&operator_stack);
                }
                if (!push(&operator_stack, current_token)) return false;
                break;

            case TOKEN_LPAREN:
                if (!push(&operator_stack, current_token)) return false; 
                break;

            case TOKEN_RPAREN:
                while (!is_empty(&operator_stack) && peek(&operator_stack).type != TOKEN_LPAREN) {
                    rpn_output[rpn_idx++] = pop(&operator_stack);
                }
                if (is_empty(&operator_stack) || peek(&operator_stack).type != TOKEN_LPAREN) {
                    fprintf(stderr, "Error: unbalenced parenthesis \n");
                    return false; 
                }
                pop(&operator_stack); 
                break;

            default:
                break;
        }
        if (rpn_idx >= 512 - 1) { 
             fprintf(stderr, "Erro: Output buffer\n");
             return false;
        }
    }

    while (!is_empty(&operator_stack)) {
        Token top_token = peek(&operator_stack);
        if (top_token.type == TOKEN_LPAREN) {
            fprintf(stderr, "Erro: Error: unbalenced parenthesis\n");
            return false; 
        }
        rpn_output[rpn_idx++] = pop(&operator_stack);
         if (rpn_idx >= 512 - 1) {
             fprintf(stderr, "Erro: Output buffer\\n");
             return false;
        }
    }

    rpn_output[rpn_idx].type = TOKEN_END; 
    *rpn_len = rpn_idx;
    return true;
}


typedef struct {
    double items[STACK_SIZE];
    int top;
} DoubleStack;

void init_double_stack(DoubleStack *s) { s->top = -1; }

bool is_double_empty(DoubleStack *s) { return s->top == -1; }

bool push_double(DoubleStack *s, double val) {

    if (s->top >= STACK_SIZE - 1) return false;
    s->items[++(s->top)] = val;
    return true;
}
double pop_double(DoubleStack *s) {
    if (is_double_empty(s)) return NAN;
    return s->items[(s->top)--];
}

bool evaluate_rpn(Token *rpn_tokens, int rpn_len, double *result) {
    DoubleStack operand_stack;

    init_double_stack(&operand_stack);

    for (int i = 0; i < rpn_len; ++i) {
        Token token = rpn_tokens[i];

        if (token.type == TOKEN_NUMBER) {
            if (!push_double(&operand_stack, token.value.number)) {
                fprintf(stderr, "Error: operand pile at evaluation\n");
                return false;
            }
        } else if (token.type == TOKEN_OPERATOR) {
            double right_operand = pop_double(&operand_stack);

            double left_operand = pop_double(&operand_stack);

            if (isnan(right_operand) || isnan(left_operand)) {
                 fprintf(stderr, "Error: missing operands  '%c'\n", token.value.operator);
                 return false;
            }

            double current_result;
            switch (token.value.operator) {
                case '+': current_result = left_operand + right_operand; break;
                case '-': current_result = left_operand - right_operand; break;
                case '*': current_result = left_operand * right_operand; break;
                case '/':
                    if (right_operand == 0.0) {
                        fprintf(stderr, "Error: Division by zero\n");
                        return false;
                    }
                    current_result = left_operand / right_operand;
                    break;
                default:
                    fprintf(stderr, "Unknow Operator: %c\n", token.value.operator);
                    return false;
            }
            if (!push_double(&operand_stack, current_result)) {
                 fprintf(stderr, "Erroe: operand pile at evaluation\n");
                 return false;
            }
        }
    }

    double final_result = pop_double(&operand_stack);
    if (isnan(final_result) || !is_double_empty(&operand_stack)) {
        fprintf(stderr, "Erro: Invalid RPN expression\n");
        return false;
    }

    *result = final_result;
    return true;
}

void generate_assembly_rpn(Token *rpn_tokens, int rpn_len) {
    for (int i = 0; i < rpn_len; ++i) {
        Token token = rpn_tokens[i];

        if (token.type == TOKEN_NUMBER) {
            printf("PUSH %d\n", (int)token.value.number);
        } else if (token.type == TOKEN_OPERATOR) {
            switch (token.value.operator) {
                case '+': printf("ADD\n"); break;
                case '-': printf("SUB\n"); break;
                case '*': printf("MUL\n"); break;
                case '/': printf("DIV\n"); break; 
                default:
                    fprintf(stderr, "Unknow opertor at assembly generation %c\n", token.value.operator);
                    break;
            }
        }
    }
     printf("End of assembly parsing\n");
}

int main() {
    char input[512];

    printf("Enter expression (ex: 3 + 4 * (2 - 1) / 5): ");
    if (!fgets(input, sizeof(input), stdin)) {
        fprintf(stderr, "Error reading input\n");
        return 1;
    }


    input[strcspn(input, "\n")] = '\0';

    if (strlen(input) == 0) {
        printf("Empty input.\n");
        return 0;
    }

    int token_count;
    Token *infix_tokens = tokenize(input, &token_count);
    if (!infix_tokens) {
        fprintf(stderr, "Filed on tokenization.\n");
        return 1;
    }

    Token *rpn_tokens = malloc((token_count + 1) * sizeof(Token)); 
     if (!rpn_tokens) {
        perror("Failed to alloc RPN Memory");
        free(infix_tokens);
        return 1;
    }

    int rpn_len;
    if (infix_to_rpn(infix_tokens, token_count, rpn_tokens, &rpn_len)) {
        printf("Tokenized infix expression: ");
        for(int i=0; i<token_count; ++i) {
            if(infix_tokens[i].type == TOKEN_NUMBER) printf("%.2f ", infix_tokens[i].value.number);
            else printf("%c ", infix_tokens[i].value.operator);
        }
        printf("\n");

        printf("RPN expressions: ");
         for(int i=0; i<rpn_len; ++i) {
            if(rpn_tokens[i].type == TOKEN_NUMBER) printf("%.2f ", rpn_tokens[i].value.number);
            else printf("%c ", rpn_tokens[i].value.operator);
        }
        printf("\n");

        double result;
        if (evaluate_rpn(rpn_tokens, rpn_len, &result)) {
            printf("Result: %f\n", result);
            generate_assembly_rpn(rpn_tokens, rpn_len);
        } else {
             fprintf(stderr, "Error during RPN validation\n");
        }

    } else {
        fprintf(stderr, "Error converting to RPN\n");
    }

    free(infix_tokens);
    free(rpn_tokens);

    return 0;
}