#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>

// Отдельное слово или знак и номер строки, где он находится
struct Token {
    std::string text;
    int line;
};

bool wordChar(char c) { // Проверяет, может ли символ входить в слово
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || static_cast<unsigned char>(c) >= 128;
}

// Пропускаем комментов // или #
// Обратная косая черта может продолжать коменты на следующей строке
void skipLine(const std::string& code, size_t& i, int& line) { // Пропускает однострочный комментарий или #
    bool continued = true;

    while (continued && i < code.size()) {
        size_t end = code.find('\n', i);

        if (end == std::string::npos) {
            i = code.size();
            return;
        }
        size_t last = end;

        if (last > i && code[last - 1] == '\r') {
            last--;
        }

        continued = last > i && code[last - 1] == '\\';

        i = end + 1;
        line++;
    }
}

// Комментарии и содержимое строк не добавляем в список
std::vector<Token> tokenize(const std::string& code) { // Выделяет слова и знаки, сохраняя номера строк
    std::vector<Token> tokens;
    size_t i = 0;
    int line = 1;

    while (i < code.size()) {
        char c = code[i];
        
        if (c == '\n') {
            line++;
            i++;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        // Однострочный комментарий или директива препроцессора
        if (c == '#' || (c == '/' && i + 1 < code.size() && code[i + 1] == '/')) {
            skipLine(code, i, line);
            continue;
        }

        // Многострочный комментарий
        if (c == '/' && i + 1 < code.size() && code[i + 1] == '*') {
            i += 2;

            while (i + 1 < code.size() && !(code[i] == '*' && code[i + 1] == '/')) {
                if (code[i] == '\n') {
                    line++;
                } 
                i++;
            }

            if (i + 1 >= code.size()) {
                throw std::runtime_error("Unclosed comment");
            }

            i += 2;
            continue;
        }

        // Необработанные строки: R"tag(...)tag"
        // и варианты с префиксами u8, u, U, L.
        size_t raw = i;

        if (code.compare(i, 4, "u8R\"") == 0) {
            raw = i + 2;
        } 
        else if (code.compare(i, 3, "uR\"") == 0 || code.compare(i, 3, "UR\"") == 0 || code.compare(i, 3, "LR\"") == 0) {
            raw = i + 1;
        }  

        if (code[raw] == 'R' && raw + 1 < code.size() && code[raw + 1] == '"') {
            size_t open = code.find('(', raw + 2);

            if (open == std::string::npos || open - raw - 2 > 16)
                throw std::runtime_error("Invalid raw string");

            std::string delimiter = code.substr(raw + 2, open - raw - 2);

            std::string closing = ")" + delimiter + "\"";
            size_t end = code.find(closing, open + 1);

            if (end == std::string::npos) {
                throw std::runtime_error("Unclosed raw string");
            }
            end += closing.size();

            while (i < end) {
                if (code[i] == '\n') {
                    line++;
                }
                i++;
            }

            continue;
        }

        // Обычные строки и символьные литералы
        if (c == '"' || c == '\'') {
            char quote = c;
            bool closed = false;
            i++;

            while (i < code.size()) {
                // Пропускаем экранированный символ
                if (code[i] == '\\') {
                    i++;
                    if (i < code.size()) {
                        if (code[i] == '\n') {
                            line++;
                        }
                        i++;
                    }
                }
                else if (code[i] == quote) {
                    i++;
                    closed = true;
                    break;
                } else {
                    if (code[i] == '\n') {
                        line++;
                    }
                    i++;
                }
            }

            if (!closed) {
                throw std::runtime_error("Unclosed string");
            }
            continue;
        }

        // Слово или число
        if (wordChar(c)) {
            size_t start = i;
            bool number = std::isdigit(static_cast<unsigned char>(c));

            // Учитываем разделитель цифр: например, 100'000
            while (i < code.size() && (wordChar(code[i]) || (number && code[i] == '\''))) {
                i++;
            }

            tokens.push_back({code.substr(start, i - start), line});
            continue;
        }

        // :: сохраняем как один знак
        if (c == ':' && i + 1 < code.size() && code[i + 1] == ':') {
            tokens.push_back({"::", line});
            i += 2;
            continue;
        }

        tokens.push_back({std::string(1, c), line});
        i++;
    }
    return tokens;
}

// Проверяем, похож ли текст перед { на заголовок функции
bool functionHeader(const std::vector<Token>& header) { // 	Отличает заголовок функции от других конструкций
    int round = 0;
    int square = 0;
    int depth = 0;
    bool operatorDefinition = false;

    for (const Token& token : header) {
        if (token.text == "operator") {
            operatorDefinition = true;
        }     
    }

    // Присваивание вне параметров означает переменную, например: auto action = []() { ... };
    for (const Token& token : header) {
        if (token.text == "(") {depth++;}
        if (token.text == ")") {depth--;}
        if (token.text == "=" && depth == 0 && !operatorDefinition) {
            return false;
        }
    }

    for (size_t i = 0; i < header.size(); i++) {
        const std::string& text = header[i].text;

        if (text == "[") {square++;}
        if (text == "]") {square--;}
        if (text == "(") {
            if (round == 0 && square == 0 && i > 0) {
                const std::string& name = header[i - 1].text;

                // Эти конструкции не являются функциями
                bool blocked = name == "if" || name == "for" || name == "while" || name == "switch" || name == "catch" 
                || name == "sizeof" || name == "alignof" || name == "decltype" || name == "noexcept" || name == "requires" || name == "static_assert";

                bool assignment = false;
                bool hasOperator = false;

                for (size_t j = 0; j < i; j++) {
                    if (header[j].text == "=") {assignment = true;}
                    if (header[j].text == "operator") {hasOperator = true;}
                }

                bool nameOK = !name.empty() && (std::isalpha(static_cast<unsigned char>(name[0])) 
                || name[0] == '_' || static_cast<unsigned char>(name[0]) >= 128);

                // Не принимаем объявление указателя за функцию
                bool pointer = i + 1 < header.size() && (header[i + 1].text == "*" || header[i + 1].text == "&");

                if (!blocked && !pointer && (nameOK || hasOperator) && (!assignment || hasOperator)) {
                    return true;
                }
            }
            round++;
        }
        if (text == ")") {
            round--;
        }
    }
    return false;
}

// В конструкторе member{...} ещё не является телом функции
bool initializerBrace(const std::vector<Token>& header) { // Отличает инициализатор конструктора от тела функции
    int round = 0;
    bool colon = false;

    for (const Token& token : header) {
        if (token.text == "(") {round++;}
        if (token.text == ")") {round--;}
        if (token.text == ":" && round == 0) {
            colon = true;
        }   
    }

    if (!colon || header.empty()) {
        return false;
    }
        
    const std::string& last = header.back().text;

    return wordChar(last[0]) || last == ">" || last == "]";
}

// Находим границы функций и считаем их строки
void findFunctions(const std::vector<Token>& tokens) {
    std::vector<Token> header;

    int round = 0;
    int square = 0;
    int initializerDepth = 0;
    int bodyDepth = 0;
    int firstLine = 0;

    for (const Token& token : tokens) {
        const std::string& text = token.text;

        // находимся внутри функции
        if (bodyDepth > 0) {
            if (text == "{") {bodyDepth++;}
            if (text == "}") {bodyDepth--;}
        
            // Закрылась скобка, с которой началось тело функции
            if (bodyDepth == 0) {
                int count = token.line - firstLine + 1;

                if (count > 40) {
                    std::cout << firstLine << " " << token.line << " " << count << "\n";
                }
            }
            continue;
        }

        // Пропускаем фигурные скобки инициализатора конструктора.
        if (initializerDepth > 0) {
            header.push_back(token);

            if (text == "{") {initializerDepth++;}
            if (text == "}") {initializerDepth--;}
            continue;
        }

        if (text == "(") {round++;}
        if (text == ")") {round--;}
        if (text == "[") {square++;}
        if (text == "]") {square--;}
            

        if (round == 0 && square == 0) {
            // public:, private: и protected: не входят
            // в заголовок следующего метода.
            if (text == ":" && header.size() == 1 && 
            (header[0].text == "public" || header[0].text == "private" || header[0].text == "protected")) {
                header.clear();
                continue;
            }

            if (text == "{") {
                if (functionHeader(header)) {
                    if (initializerBrace(header)) {
                        initializerDepth = 1;
                        header.push_back(token);
                        continue;
                    }

                    firstLine = header.front().line;
                    bodyDepth = 1;
                }
                header.clear();
                continue;
            }
            // Закончилось объявление или закрылась область
            if (text == ";" || text == "}") {
                header.clear();
                continue;
            }
        }
        header.push_back(token);
    }

    if (bodyDepth != 0) {
        throw std::runtime_error("Unclosed function body");
    }  
}

int main() {
    std::string code;
    std::string line;

    // Получаем весь текст документа от TypeScript
    while (std::getline(std::cin, line))
        code += line + "\n";

    // Обработка ошибок и исключений
    try {
        std::vector<Token> tokens = tokenize(code);
        findFunctions(tokens);
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
    
    return 0;
}