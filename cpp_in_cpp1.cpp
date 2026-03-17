#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <array>

// Create an alias for std::string
using string = std::string;
using ifstream = std::ifstream;

const string DEFAULT_FILE = "input1.txt";

const std::string CPP_KEYWORDS[] = {
    "alignas","alignof","and","and_eq","asm","atomic_cancel","atomic_commit","atomic_noexcept",
    "auto","bitand","bitor","bool","break","case","catch","char","char8_t","char16_t","char32_t",
    "class","compl","concept","const","consteval","constexpr","constinit","const_cast","continue",
    "contract_assert","co_await","co_return","co_yield","decltype","default","delete","do","double",
    "dynamic_cast","else","enum","explicit","export","extern","false","float","for","friend","goto",
    "if","inline","int","long","mutable","namespace","new","noexcept","not","not_eq","nullptr",
    "operator","or","or_eq","private","protected","public","reflexpr","register","reinterpret_cast",
    "requires","return","short","signed","sizeof","static","static_assert","static_cast","struct",
    "switch","synchronized","template","this","thread_local","throw","true","try","typedef","typeid",
    "typename","union","unsigned","using","virtual","void","volatile","wchar_t","while","xor","xor_eq"
};
const std::string CPP_OPERATORS[] = {
    ">>=", "<<=", "++", "--", "->*", "->", "&&", "||", "<=", ">=", "==", "!=",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<", ">>",
    "::", ":", "?", "~", "!", "+", "-", "*", "/", "%", "<", ">", "&", "|", "^", "="
};

const char* CPP_SEPARATORS = "{}()[];,.";
const char* CPP_OPERATORS_CHARS = "<>-=+*/%&|^:?~!*";
const char* CPP_WHITESPACE = " \t\n";

bool isInOps(const std::string word) {
    for (const string &kw : CPP_OPERATORS) {
        if (word == kw) {
            return true;
        }
    }
    return false;
}

bool isKeyword(const std::string word) {
    for (const string& kw : CPP_KEYWORDS) {
        if (word == kw) {
            return true;
        }
    }
    return false;
}
inline bool isSep(const char word){
    return word != '\0' && strchr(CPP_SEPARATORS, word);
}
inline bool isOp(const char word){
    return word != '\0' && strchr(CPP_OPERATORS_CHARS, word);
}
inline bool isWsp(const char word){
    return word != '\0' && strchr(CPP_WHITESPACE, word);
}
inline bool isStop(const char word){
    return isWsp(word) || isOp(word) || isSep(word) || word == '\"';
}

inline bool isLetter(const char& c){
    return c!= '\0' && ((c >='a' && c <= 'z') || (c >='A' && c <='Z') || c == '_');
}
inline bool isLetterOrNr(const char& c){
    return isLetter(c) || (c <= '9' && c >='0');
}


enum TokenType{
    OPERATOR,
    VALUE,
    KEY_WORD,
    SEPARATOR,
    IDENTIFIER,
    COMMENT,
    ERROR,
    FILE_END,
    INCLUDE  // considering libraries this
};

struct LexInfo{
     TokenType type;
     size_t len,line_nr,next_line_nr;
     const char* token_start;
     string error_message = "";

     void increase_lines(){
        line_nr++;
        next_line_nr++;
     }
     string getWord() const {
        string lexedWord = "";
        for(size_t i=0;i<len;i++)
            lexedWord += *(token_start+i);

        return lexedWord;
     }

     friend std::ostream& operator<<(std::ostream& os, const LexInfo& token) {
        if(token.type == FILE_END){
            return os << "Reached the end of file!";
        }

        string token_name = "";
        if(token.type == ERROR){
            os << "\n *** ERROR: " << token.error_message << " ***\n";
            token_name = "error";
        }

        if(token.type == SEPARATOR){
            token_name = "separator";
        }
        if(token.type == OPERATOR){
            token_name = "operator";
         }
        if(token.type == KEY_WORD){
            token_name = "key-word";
        }
        if(token.type == IDENTIFIER){
            token_name = "identifier";
        }
        if(token.type == COMMENT){
            token_name = "comment";
        }
        if(token.type == INCLUDE)
            token_name = "include";

        if(token.type == VALUE)
            token_name = "value";

        os << "\"" << token.getWord();
        os << "\", on line: " << token.line_nr << ", type: " << token_name <<", len: " << token.len << "\n\n";
        return os;
    }
};

bool isComment(const LexInfo& result){
    if(result.token_start[0] == '\0' || result.token_start[0] != '/') return false;
    if(result.token_start[1] == '\0') return false; // in case we have a lone / at the end of a program

    if(result.token_start[0] == '/' && result.token_start[1] == '*') /// multi line comment
        return true;
    if(result.token_start[0] == '/' && result.token_start[1] == '/') /// single line comment
        return true;
    return false;
}

const char* skipWhitespaces(LexInfo& result){
    const char *p = result.token_start;
    /// skip all whitespaces, tabs, newlines
    for(;*p != '\0';p++){
        char c = *p;

        if( isWsp(c) ){
            if( c == '\n')
                result.increase_lines();
            continue;
        }
        break; /// end loop on first non-whtiespace character
    }
    return p;
}
bool handleInclude(LexInfo& result, const string& word){
    /// expected form: #include <NAME>

    if(word != "#include")
        return false;

    result.type = INCLUDE;

    const char*p = result.token_start;

    bool has_smaller = false, has_bigger = false;

    while( *p != '\0' && *p != '\n'){

        if(*p == '<'){
            if(has_smaller)
                result.type = ERROR;

            has_smaller = true;
        }

        if(*p == '>'){
            if(!has_smaller)
                result.type = ERROR;

            if(has_bigger)
                result.type = ERROR;

            has_bigger = true;
        }
        p++;
    }
    if( !(has_bigger && has_smaller) )
        result.type = ERROR;

    if(result.type == ERROR){
        result.error_message = "Incorrect <, > placement!";
    }
    result.len = (p-result.token_start);

    return true;
}
bool handleOperator(LexInfo& result){
    /// expected form: +, * , /, %, <, <=, >, <<, >>, !, =, ==, !=, etc.

    const char*p = result.token_start;
    if(!isOp(*p))
        return false;

    while(isOp(*p)){
    p++;
    result.len = p - result.token_start;
    string word = result.getWord();
    if(!isInOps(word)){
        result.len--;
        break;
    }
    }

    string word = result.getWord();
    if(isInOps(word)){
        result.type = OPERATOR;
        return true;
    }

    result.type = ERROR;
    result.error_message = "Invalid operator!";

    return true;
}

bool handleIdentifier(LexInfo& result, const string& word){
    /// expected form: (a-z | A-Z | _)(a-z | A-Z | _ | 0-9)*

    if(!isLetter(word[0]))
        return false;

    for(const char l : word){
        if(!isLetterOrNr(l)){
            result.error_message = "Invalid identifier!";
            result.type = ERROR;
            return true;
        }
    }
    result.type = IDENTIFIER;

    return true;
}

bool handleNumber(LexInfo& result, const string& word){
/// expected form: \d+.\d*
    if( !(word[0] >= '0' && word[0] <= '9')){
        return false;
    }

    result.type = VALUE;
    for (const char& l : word){
        if( !(l >='0' && l<='9')){
            result.type = ERROR;
            result.error_message = "Invalid number!";
            return true;
        }
    }
        const char* p = result.token_start;

    if( p[result.len] == '.'){
        p = p + result.len + 1;
        while(!isStop(*p)){
            if( !(*p >='0' || *p<='9')){
                result.type = ERROR;
                result.error_message = "Invalid number!";
            }
            p++;
        }
        result.len = p - result.token_start;
    }

    return true;

}

bool handleValue(LexInfo& result, const string& word){
    if(word[0] >= '0' && word[0] <= '9')
        return handleNumber(result,word);

    if(word[0] == '\"' || word[0] == '\''){
        char initialQuote = word[0];
        const char *p = result.token_start + 1;

        while(*p != '\0' && (*p!= '\n' || *(p-1) == '\\')){
            if( *p == initialQuote && p[-1] != '\\' ){ /// ignore \" as escape
                result.type = VALUE;
                result.len = p - result.token_start + 1;
                return true;
            }
            p++;
        }

        result.type = ERROR;
        result.len = p - result.token_start;
        result.error_message = "Unfinished char/string! ";
        return true;

    }

    return false;
}

bool handleComment(LexInfo& result){
    /// expected form: //(anything)*\n | /\* (anything)* \*/

    if(!isComment(result))
        return false;

    result.type = COMMENT;

    if(result.token_start[1] == '/'){ /// single line comment

        const char*p = result.token_start;
        while(!strchr("\n\0",*p)){
            p++;
        }
        result.len = (p - result.token_start);

        return true;
    }
    /// assume result.token_start[1] == '*' so multiline comment:

    const char*p = result.token_start + 2; /// ignore the first /*

    while( *p != '\0' && *(p+1) != '\0' && (*(p+1) != '/' || *p != '*')){
        if(*p == '\n')
            result.next_line_nr++;
        p++;
    }

    if(*p == '\0' || *(p+1) == '\0'){
        result.type = ERROR;
        result.error_message = "Unfinished comment!";
    }

    result.len = (p + 2 - result.token_start);

    return true;
}

LexInfo lex(const char *start, size_t skipLen = 0, size_t current_line = 0 ){
    LexInfo result;
    result.token_start = start + skipLen;

    result.line_nr = current_line;
    result.next_line_nr = current_line;

    const char * p = skipWhitespaces(result);

    if(*p == '\0'){
        result.type = FILE_END;
        return result;
    }

    result.token_start = p;

    for(;*p != '\0';p++){
        char c = *p;

        if( isStop(c)){
            break;
        }
    }
    result.len = std::max(p - result.token_start,(long int)1);

    if(p - result.token_start == 0){
        if( isSep(*p)){
            result.type = SEPARATOR;
            return result;
        }
    }

    string lexedWord = result.getWord();

    if(isKeyword(lexedWord)){
        result.type = KEY_WORD;
        return result;
    }

    if(handleComment(result))
        return result;

    if(handleInclude(result, lexedWord))
        return result;

    if(handleValue(result,lexedWord))
        return result;

    if(handleOperator(result))
        return result;

    if(handleIdentifier(result,lexedWord))
        return result;

    result.type = ERROR;
    result.error_message = "Unknown type!";
    return result;
}

ifstream getInputStream(){
    string filename;
    std::cout << "enter filename (or 1 for default): ";
    std::cin >> filename;
    if(filename == "1"){
        filename = DEFAULT_FILE;
    }
    ifstream fin(filename);

    if (!fin.is_open()){
        std::cerr << "Failed to open file to be lexed! Double check: " << filename << std::endl;
        exit(1);
    }
    return fin;
}

int main(){
    ifstream fin = getInputStream();

    string input = "";
    string line;
    while (getline(fin, line)) {
        input += line + "\n";
    }

    LexInfo result = lex(&input[0], 0, 0);
    while(result.type != TokenType::FILE_END){
        std::cout << result;

        result = lex(result.token_start, result.len, result.next_line_nr);
    }
    std::cout << result;


}
