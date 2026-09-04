#include <formatter/tokenizer.h>
#include <stdio.h>

typedef struct {
    int index;
    int end;
    TokenLocation location;
    bool recovering;
} tokenizer_recovery_context;

typedef struct {
    string_view sv;
    vector * vec;

    int index;
    TokenLocation location;

    stage_diagnoses * diagnoses;
} parsing_context;

typedef struct {
    int success;
} token_result;

char context_peek(parsing_context * context);
char context_peek_n(parsing_context * context, int n);
char context_fetch(parsing_context * context);
char* context_fetch_n(parsing_context * context, int n);

token_result parse_ident(parsing_context * context);
token_result parse_single_line_comment(parsing_context * context);
token_result parse_multiple_lines_comment(parsing_context * context);
token_result parse_number(parsing_context * context);
token_result parse_decimal_number(parsing_context * context);

static int scan_numeric_segment(parsing_context * context, int begin, int base, unsigned long long * value, int * separator_ok);
static int scan_decimal(parsing_context * context, int begin, int * is_float, int * separator_ok, long double * value);
static int scan_number_suffix(parsing_context * context, int begin, int is_float, enum TokenIValType * itype, int * ok);

#define GEN_TOKEN(NAME,TYPE,LEN) \
    Token NAME = { \
        .type = TYPE, \
        .data = sv_substr(context->sv,context->index, LEN), \
        .location = context->location \
    }

#define ADD_TOKEN(target,token)  *((Token*)vec_push_back((target))) = (token)

tokenizer_result parse_tokens(string_view sv, vector * vec){
    tokenizer_result result = {
        .success = 1,
        .diagnoses = vec_new(sizeof(stage_diagnosis), 8)
    };
    
    parsing_context original_context = {
        .sv = sv,
        .vec = vec,
        .index = 0,
        .location = {
            .col = 0,
            .row = 0
        },
        .diagnoses = &(result.diagnoses)
    };
    // 统一dispatched function和主function的调用
    parsing_context * context = &original_context;

    tokenizer_recovery_context recovery = {
        .index = -1,
        .end = 0,
        .location = context->location
    }; 

    while(context->index < sv_length(sv)){
        // printf("parsing... %d/%d\n",context.index,sv_length(sv));
        // 跳过空格
        char peek = context_peek(context);
        
        // 初始化恢复的context
        recovery.recovering = false;
        recovery.end = context->index;

        if(ch_space(peek)){
            context_fetch(context);
        }else if(ch_token_begin(peek)){
            result.success &= parse_ident(context).success;
        }else if(ch_statement_end(peek)){
            GEN_TOKEN(token,T_END_STATEMENT, 1);
            ADD_TOKEN(vec,token);
            
            context_fetch(context);
        }
        /// 括号大家族，这里为了方便依旧使用 XMacro
        #define QUICK_REGISTER(fn, tag) \
            else if(fn(peek)){ \
                GEN_TOKEN(token, tag, 1);\
                ADD_TOKEN(vec,token); \
                context_fetch(context); \
            }
            
        QUICK_REGISTER(ch_parentheses_begin, T_PARENTHESE_BEGIN)
        QUICK_REGISTER(ch_parentheses_end, T_PARENTHESE_END)
        QUICK_REGISTER(ch_brace_begin, T_BRACE_BEGIN)
        QUICK_REGISTER(ch_brace_end, T_BRACE_END)
        QUICK_REGISTER(ch_bracket_begin, T_BRACKET_BEGIN)
        QUICK_REGISTER(ch_bracket_end, T_BRACKET_END)

        #undef QUICK_REGISTER

        /// 数值系统和 .
        else if(ch_digit(peek)){
            result.success &= parse_number(context).success;
        }else if(ch_dot(peek)){
            // 这里看后面是不是数字，如果是就是数字解析，否则走单符号解析
            char peek2 = context_peek_n(context,1);

            if(ch_digit(peek2)){
                result.success &= parse_number(context).success;
            }else{
                GEN_TOKEN(token, T_DOT, 1);
                ADD_TOKEN(vec, token);

                context_fetch(context);
            }
        }
        /// 注释和基础运算符
        else if(ch_operand_div(peek)){ 
            char peek2 = context_peek_n(context,1); // 继续往下看

            if(ch_comment_2_single_line(peek2)){
                // 单行注释，进入专门的解析
                result.success &= parse_single_line_comment(context).success;
            }else if(ch_comment_2_multiple_lines(peek2)){
                // 多行，进入专门的解析
                result.success &= parse_multiple_lines_comment(context).success;
            }else {
                // 作为除法进行识别
                GEN_TOKEN(token, T_OP_DIV, 1);
                ADD_TOKEN(vec,token);

                context_fetch(context);
            }
        }else if(ch_operand_add(peek)){ // 加法，区分 ++ 和 +
            char peek2 = context_peek_n(context,1);

            if(ch_operand_add(peek2)){ // ++
                GEN_TOKEN(token,T_OP_INCREMENT,2);
                ADD_TOKEN(vec,token);

                context_fetch_n(context,2);
            }else{ // 普通加法
                GEN_TOKEN(token, T_OP_ADD, 1);
                ADD_TOKEN(vec, token);

                context_fetch(context);
            }
        }else if(ch_operand_minus(peek)){ // 减法同理
            char peek2 = context_peek_n(context,1);

            if(ch_operand_minus(peek2)){ // --
                GEN_TOKEN(token,T_OP_DECREMENT,2);
                ADD_TOKEN(vec,token);

                context_fetch_n(context,2);
            }else{ // 普通减法
                GEN_TOKEN(token, T_OP_MINUS, 1);
                ADD_TOKEN(vec, token);

                context_fetch(context);
            }
        }else if(ch_operand_mul(peek)){
            // 乘法则不需要进行啥判断（如果这个项目要做指针的话，区分也是到了语法分析才区分）
            GEN_TOKEN(token, T_OP_MUL, 1);
            ADD_TOKEN(vec, token);

            context_fetch(context);
        }
        /// 成对出现的运算符（= ==  !=  <= >= && ||），单/双字符
        #define QUICK_OPS(fn, fn2, SINGLE_TAG, DOUBLE_TAG) \
            else if(fn(peek)){ \
                char peek2 = context_peek_n(context, 1); \
                if(fn2(peek2)){ \
                    GEN_TOKEN(token, DOUBLE_TAG, 2); \
                    ADD_TOKEN(vec, token); \
                    context_fetch_n(context, 2); \
                }else{ \
                    GEN_TOKEN(token, SINGLE_TAG, 1); \
                    ADD_TOKEN(vec, token); \
                    context_fetch(context); \
                } \
            }

        QUICK_OPS(ch_operand_assign, ch_operand_assign, T_OP_ASSIGN, T_OP_EQ)
        QUICK_OPS(ch_operand_not, ch_operand_assign, T_OP_NOT, T_OP_NE)
        QUICK_OPS(ch_operand_lt, ch_operand_assign, T_OP_LT, T_OP_LE)
        QUICK_OPS(ch_operand_gt, ch_operand_assign, T_OP_GT, T_OP_GE)
        QUICK_OPS(ch_operand_and, ch_operand_and, T_OP_BIT_AND, T_OP_LOGICAL_AND)
        QUICK_OPS(ch_operand_or, ch_operand_or, T_OP_BIT_OR, T_OP_LOGICAL_OR)

        #undef QUICK_OPS

        /// 单字符运算符 % ^ ~
        else if(ch_operand_mod(peek)){
            GEN_TOKEN(token, T_OP_MOD, 1);
            ADD_TOKEN(vec, token);
            context_fetch(context);
        }else if(ch_operand_xor(peek)){
            GEN_TOKEN(token, T_OP_BIT_XOR, 1);
            ADD_TOKEN(vec, token);
            context_fetch(context);
        }else if(ch_operand_bnot(peek)){
            GEN_TOKEN(token, T_OP_BIT_NOT, 1);
            ADD_TOKEN(vec, token);
            context_fetch(context);
        }
        /// 报错的恢复措施
        else{
            recovery.recovering = true;
            if(recovery.index < 0){
                recovery.index = context->index;
                recovery.location = context->location;
            }
            context_fetch(context);
        }

        if(!recovery.recovering && recovery.index >= 0){
            string_view sv = sv_substr(context->sv,recovery.index,recovery.end - recovery.index);
            sd_printf(
                &(result.diagnoses),
                "Unrecognized token \"%.*s\" at row %d col %d.",
                sv_length(sv),
                sv_begin(sv),
                recovery.location.row, recovery.location.col
            );
            recovery.index = -1;
            // 失败了
            result.success = 0;
        }
    }

    return result;
}

token_result parse_single_line_comment(parsing_context * context){
    {
        GEN_TOKEN(token,T_COMMENT_SINGLE_LINE,2);
        ADD_TOKEN(context->vec, token);
    }
    context_fetch_n(context,2);
    
    token_result ret = {
        .success = 0
    };
    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    char prev = '\0';
    while(true){
        char peek = context_peek(context);
        if(ch_line_break(peek) && !ch_comment_single_line_renew(prev) || ch_eof(peek)) break;
        context_fetch(context);

        if(peek != '\r')prev = peek;
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    token.type = T_COMMENT_BODY;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    *((Token*)vec_push_back(context->vec)) = token;

    return ret;
}

token_result parse_multiple_lines_comment(parsing_context * context){
    {
        GEN_TOKEN(token,T_BEGIN_COMMENT_MULTIPLE_LINES, 2);
        ADD_TOKEN(context->vec, token);
    }

    token_result ret = {
        .success = 0
    };
    TokenLocation begin = context->location;
    context_fetch_n(context,2);

    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    char prev = '\0';
    bool ended = false;
    while(true){
        char peek = context_peek(context);
        if(ch_comment_2_multiple_lines(prev) && ch_operand_div(peek)){
            ended = true;
            context_fetch(context); // 读取完毕目前的operand
            break;
        }else if(ch_eof(peek)) break;

        context_fetch(context);

        if(peek != '\r')prev = peek;
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    token.type = T_COMMENT_BODY;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin - ended * 2);\
    // 注释内容
    *((Token*)vec_push_back(context->vec)) = token;

    if(ended){
        ret.success = 1;
        token.type = T_END_COMMENT_MULTIPLE_LINES;
        token.data = sv_substr(context->sv,context->index - 2,2);
        token.location = context->location;
        token.location.col -= 2; // */ 一定是在一行里面的，因此就可以直接读取
        // 注释结尾
        *((Token*)vec_push_back(context->vec)) = token;
    }else{
        sd_printf(
            context->diagnoses,
            "Unclosed multiple-lines comment at row %d col %d!",
            begin.row, begin.col
        );
    }

    return ret;
}

const char * token_string(enum TokenType type){
    return token_strings[type];
}

char context_peek(parsing_context * context){
    if(context->index >= sv_length(context->sv)) return '\0'; 
    return sv_at(context->sv,context->index);
}

char context_peek_n(parsing_context * context,int n){
    if(context->index >= sv_length(context->sv)) return '\0'; 
    return sv_at(context->sv,context->index + n);
}


char context_fetch(parsing_context * context){
    char ch = sv_at(context->sv,context->index);
    /// Very important?
    if(ch_eof(ch))return ch;
    
    if(ch_line_break(ch)){
        ++(context->location.row);
        context->location.col = 0;
    }else ++(context->location.col);

    ++(context->index);
    return ch;
}


char* context_fetch_n(parsing_context * context, int n){
    char * begin = sv_begin(context->sv) + context->index;
    for(int i = 0;i < n;++i)context_fetch(context);
    return begin;
}

token_result parse_ident(parsing_context * context){
    token_result ret = {
        .success = 0
    };
    int pos_begin = context->index;
    Token token = token_null(context->sv);
    token.location = context->location;

    while(true){
        char peek = context_peek(context);
        if(!ch_token_middle(peek) || ch_eof(peek)) break;
        context_fetch(context);
        // printf("fetched %c\n",context_fetch(context));
    }
    
    // printf("%d %d %d\n",pos_begin, context->index , context->sv.length);

    ret.success = 1;
    token.type = T_IDENTIFIER;
    token.data = sv_substr(context->sv,pos_begin,context->index - pos_begin);

    ADD_TOKEN(context->vec, token);
}

/// 扫描一段某进制的连续数字，允许分隔符(')穿插
/// 返回结束位置(相对context->index)；value 累积数值；separator_ok 表示分隔符是否夹在两个合法数字之间
static int scan_numeric_segment(parsing_context * context, int begin, int base, unsigned long long * value, int * separator_ok){
    int index = begin;
    *value = 0;
    *separator_ok = 1;

    while(true){
        char ch = context_peek_n(context, index);

        if(ch_eof(ch)) break;

        if(ch_number_sep(ch)){
            if(
                ch_number_value(context_peek_n(context, index - 1), base) < 0 ||
                ch_number_value(context_peek_n(context, index + 1), base) < 0
            ){
                *separator_ok = 0;
            }
            ++index;
            continue;
        }

        int digit = ch_number_value(ch, base);
        if(digit < 0) break;
        *value = *value * base + digit;
        ++index;
    }

    return index;
}

/// 扫描十进制字面量（整数或浮点，可带小数点/指数），
/// 返回结束位置；is_float 记录是否出现小数点或指数；value 计算出的数值
static int scan_decimal(parsing_context * context, int begin, int * is_float, int * separator_ok, long double * value){
    int index = begin;
    *is_float = 0;
    *separator_ok = 1;
    *value = 0.0;

    // 整数部分
    while(true){
        char ch = context_peek_n(context, index);

        if(ch_eof(ch)) break;

        if(ch_number_sep(ch)){
            if(
                ch_number_value(context_peek_n(context, index - 1), 10) < 0 ||
                ch_number_value(context_peek_n(context, index + 1), 10) < 0
            ){
                *separator_ok = 0;
            }
            ++index;
            continue;
        }

        int digit = ch_number_value(ch, 10);
        if(digit < 0) break;
        *value = *value * 10.0 + digit;
        ++index;
    }

    // 小数部分
    if(ch_dot(context_peek_n(context, index))){
        *is_float = 1;
        ++index;

        long double scale = 0.1;
        while(true){
            char ch = context_peek_n(context, index);

            if(ch_eof(ch)) break;

            if(ch_number_sep(ch)){
                if(
                    ch_number_value(context_peek_n(context, index - 1), 10) < 0 ||
                    ch_number_value(context_peek_n(context, index + 1), 10) < 0
                ){
                    *separator_ok = 0;
                }
                ++index;
                continue;
            }

            int digit = ch_number_value(ch, 10);
            if(digit < 0) break;
            *value += digit * scale;
            scale *= 0.1;
            ++index;
        }
    }

    // 指数部分
    char exponent_ch = context_peek_n(context, index);
    if(exponent_ch == 'e' || exponent_ch == 'E'){
        *is_float = 1;
        ++index;

        bool exponent_negative = false;
        char sign_char = context_peek_n(context, index);
        if(sign_char == '+') ++index;
        else if(sign_char == '-'){ exponent_negative = true; ++index; }

        int exponent = 0;
        while(true){
            char ch = context_peek_n(context, index);

            if(ch_eof(ch)) break;

            if(ch_number_sep(ch)){
                if(
                    ch_number_value(context_peek_n(context, index - 1), 10) < 0 ||
                    ch_number_value(context_peek_n(context, index + 1), 10) < 0
                ){
                    *separator_ok = 0;
                }
                ++index;
                continue;
            }

            int digit = ch_number_value(ch, 10);
            if(digit < 0) break;
            exponent = exponent * 10 + digit;
            ++index;
        }

        long double power = 1.0;
        for(int i = 0; i < exponent; ++i) power *= 10.0;
        *value = exponent_negative ? *value / power : *value * power;
    }

    return index;
}

/// 在数字体外(begin处)检查后缀并确定其类型。
/// 返回 消费到的结束位置；itype 输出类型；ok 表示后缀是否合法
static int scan_number_suffix(parsing_context * context, int begin, int is_float, enum TokenIValType * itype, int * ok){
    int index = begin;
    *ok = 1;

    // 浮点后缀: 默认 double，f/F 为 float，l/L 为 long double
    if(is_float){
        char ch = context_peek_n(context, index);
        if(ch_number_suffix_float(ch)){
            *itype = I_FLOAT;
            return index + 1;
        }
        if(ch_number_suffix_long(ch)){
            *itype = I_LONG_DOUBLE;
            return index + 1;
        }
        *itype = I_DOUBLE;
        return index;
    }

    // 整数后缀: u/U 无符号，l/L 长（可重复一次成为 long long）
    int unsig = 0;
    int longs = 0;
    while(true){
        char ch = context_peek_n(context, index);
        if(ch_number_suffix_unsigned(ch)) unsig = 1;
        else if(ch_number_suffix_long(ch)) ++longs;
        else break;
        ++index;
    }

    if(longs > 2){
        *ok = 0;
        *itype = I_VOID;
        return index;
    }

    if(longs == 0){
        *itype = unsig ? I_UINT : I_INT;
    }else if(longs == 1){
        *itype = unsig ? I_ULONG : I_LONG;
    }else{
        *itype = unsig ? I_ULONG_LONG : I_LONG_LONG;
    }
    return index;
}

/// 处理十进制字面量（整数或浮点）。既服务于以 0 开头的浮点(0.)，也服务于普通十进制
token_result parse_decimal_number(parsing_context * context){
    token_result ret = {
        .success = 0
    };

    int is_float = 0;
    int separator_ok = 1;
    long double value = 0.0;
    int end = scan_decimal(context, 0, &is_float, &separator_ok, &value);

    if(!separator_ok){
        sd_printf(
            context->diagnoses,
            "Number separators are used wrongly at row %d col %d.",
            context->location.row, context->location.col
        );
        context_fetch_n(context, end);
        return ret;
    }

    enum TokenIValType itype = I_VOID;
    int suffix_ok = 1;
    end = scan_number_suffix(context, end, is_float, &itype, &suffix_ok);

    if(!suffix_ok){
        sd_printf(
            context->diagnoses,
            "Invalid number suffix at row %d col %d.",
            context->location.row, context->location.col
        );
        context_fetch_n(context, end);
        return ret;
    }

    GEN_TOKEN(token, is_float ? T_NUMBER_FLOAT : T_NUMBER, end);
    token.itype = itype;
    if(is_float){
        token.ival.ld = value;
    }else{
        token.ival.ull = (unsigned long long)value;
    }
    ADD_TOKEN(context->vec, token);
    context_fetch_n(context, end);

    ret.success = 1;
    return ret;
}

token_result parse_number(parsing_context * context){
    token_result ret = {
        .success = 0
    };
    char peek = context_peek(context);

    // 检测0
    if(peek == '0'){
        char peek2 = context_peek_n(context, 1);

        if(ch_dot(peek2)){ // 浮点 0.xxx
            return parse_decimal_number(context);
        }else if(ch_number_base_16_clue(peek2)){ // 十六进制 0x / 0X
            unsigned long long value = 0;
            int separator_ok = 1;
            int end = scan_numeric_segment(context, 2, 16, &value, &separator_ok);

            if(end == 2){
                sd_printf(
                    context->diagnoses,
                    "Hex digits are expected after \"0x\" at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, 2);
                return ret;
            }

            if(!separator_ok){
                sd_printf(
                    context->diagnoses,
                    "Number separators are used wrongly at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, end);
                return ret;
            }

            enum TokenIValType itype = I_VOID;
            int suffix_ok = 1;
            end = scan_number_suffix(context, end, 0, &itype, &suffix_ok);

            if(!suffix_ok){
                sd_printf(
                    context->diagnoses,
                    "Invalid number suffix at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, end);
                return ret;
            }

            GEN_TOKEN(token, T_NUMBER_HEX, end);
            token.itype = itype;
            token.ival.ull = value;
            ADD_TOKEN(context->vec, token);
            context_fetch_n(context, end);

            ret.success = 1;
            return ret;
        }else if(ch_number_base_2_clue(peek2)){ // 二进制 0b / 0B
            unsigned long long value = 0;
            int separator_ok = 1;
            int end = scan_numeric_segment(context, 2, 2, &value, &separator_ok);

            if(end == 2){
                sd_printf(
                    context->diagnoses,
                    "Binary digits are expected after \"0b\" at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, 2);
                return ret;
            }

            if(!separator_ok){
                sd_printf(
                    context->diagnoses,
                    "Number separators are used wrongly at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, end);
                return ret;
            }

            enum TokenIValType itype = I_VOID;
            int suffix_ok = 1;
            end = scan_number_suffix(context, end, 0, &itype, &suffix_ok);

            if(!suffix_ok){
                sd_printf(
                    context->diagnoses,
                    "Invalid number suffix at row %d col %d.",
                    context->location.row, context->location.col
                );
                context_fetch_n(context, end);
                return ret;
            }

            GEN_TOKEN(token, T_NUMBER_BIN, end);
            token.itype = itype;
            token.ival.ull = value;
            ADD_TOKEN(context->vec, token);
            context_fetch_n(context, end);

            ret.success = 1;
            return ret;
        }

        // 八进制（0 开头），或单独的 0
        unsigned long long value = 0;
        int separator_ok = 1;
        int body_end = scan_numeric_segment(context, 1, 8, &value, &separator_ok);

        if(!separator_ok){
            sd_printf(
                context->diagnoses,
                "Number separators are used wrongly at row %d col %d.",
                context->location.row, context->location.col
            );
            context_fetch_n(context, body_end);
            return ret;
        }

        // 八进制数字（或单独的0）后面紧跟 8/9 属于非法
        if(
            ch_digit(context_peek_n(context, body_end)) &&
            ch_number_value(context_peek_n(context, body_end), 8) < 0
        ){
            sd_printf(
                context->diagnoses,
                "Invalid octal digit at row %d col %d.",
                context->location.row, context->location.col + body_end
            );
            int consumed = body_end;
            while(ch_digit(context_peek_n(context, consumed))) ++consumed;
            context_fetch_n(context, consumed);
            return ret;
        }

        enum TokenIValType itype = I_VOID;
        int suffix_ok = 1;
        int end = scan_number_suffix(context, body_end, 0, &itype, &suffix_ok);

        if(!suffix_ok){
            sd_printf(
                context->diagnoses,
                "Invalid number suffix at row %d col %d.",
                context->location.row, context->location.col
            );
            context_fetch_n(context, end);
            return ret;
        }

        if(body_end == 1){ // 单独的 0（可能带后缀，如 0u）
            GEN_TOKEN(token, T_NUMBER, end);
            token.itype = itype;
            token.ival.ull = value;
            ADD_TOKEN(context->vec, token);
            context_fetch_n(context, end);

            ret.success = 1;
            return ret;
        }

        GEN_TOKEN(token, T_NUMBER_OCT, end);
        token.itype = itype;
        token.ival.ull = value;
        ADD_TOKEN(context->vec, token);
        context_fetch_n(context, end);

        ret.success = 1;
        return ret;
    }

    // 普通的十进制整数或浮点（含以 . 开头的 .5）
    return parse_decimal_number(context);
}

#undef GEN_TOKEN