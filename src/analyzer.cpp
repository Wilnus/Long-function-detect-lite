
#include <iostream>
#include <string>
#include <sstream>
#include <cctype>

using namespace std;

struct Token {
    string text;
    int line;
};

Token nextToken(const string& code, size_t& i, int& line) {
    while (i < code.size()) {
        char c = code[i];

        if (c == '\n') {
            line++;
            i++;
            continue;
        }

        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }

        if (c == '#' || (c == '/' && i + 1 < code.size() && code[i + 1] == '/')) {
            while (i < code.size() && code[i] != '\n') i++;
            continue;
        }

        if (c == '/' && i + 1 < code.size() && code[i + 1] == '*') {
            i += 2;

            while (i < code.size()) {
                if (code[i] == '\n') line++;

                if (code[i] == '*' && i + 1 < code.size() && code[i + 1] == '/') {
                    i += 2;
                    break;
                }
                i++;
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            i++;

            while (i < code.size()) {
                if (code[i] == '\\' && i + 1 < code.size()) {
                    if (code[i + 1] == '\n') line++;
                    i += 2;
                    continue;
                }

                if (code[i] == '\n') line++;
                if (code[i++] == quote) break;
            }
            continue;
        }

        int tokenLine = line;

        if (isalnum((unsigned char)c) || c == '_') {
            size_t start = i++;

            while (i < code.size() &&
                   (isalnum((unsigned char)code[i]) || code[i] == '_')) {
                i++;
            }

            return {code.substr(start, i - start), tokenLine};
        }

        i++;
        return {string(1, c), tokenLine};
    }

    return {"", line};
}


bool isFunction(const string& header) {
    istringstream stream(header);
    string word, previous;


    while (stream >> word) {
        if (word == "(") {


            if (previous == "if" || previous == "for" || previous == "while" || previous == "switch" || previous == "catch") {
                return false;
            }


            if (!previous.empty() &&
                (isalpha((unsigned char)previous[0]) || previous[0] == '_')) {
                return true;
            }
        }

        previous = word;
    }

    return false;
}

int main() {
    string code, lineText;


    while (getline(cin, lineText)) {
        code += lineText + '\n';
    }

    size_t i = 0;
    int line = 1;
    int first = 1;

    string header;
    Token token;


    while (!(token = nextToken(code, i, line)).text.empty()) {


        if (token.text == "{") {

            if (isFunction(header)) {
                int depth = 1;
                Token endToken = token;

                while (depth > 0) {
                    endToken = nextToken(code, i, line);

                    if (endToken.text.empty()) break;

                    if (endToken.text == "{") depth++;
                    if (endToken.text == "}") depth--;
                }

                if (depth == 0) {
                    int last = endToken.line;
                    int length = last - first + 1;

                    if (length > 40) {
                        cout << first << ' ' << last << ' ' << length << '\n';
                    }
                }
            }

            header.clear();
        }

        else if (token.text == ";" || token.text == "}") {
            header.clear();
        }

        else if (token.text == ":" && (header == "public " || header == "private " || header == "protected ")) {
            header.clear();
        }

        else {
            if (header.empty()) first = token.line;
            header += token.text + " ";
        }
    }

    return 0;
}
