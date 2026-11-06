
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

using namespace std;

struct Token {
    string text;
    int line;
};

vector<Token> tokenize(const string& code) {
    vector<Token> result;
    int line = 1;

    for (size_t i = 0; i < code.size();) {
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
                    i += 2;
                    continue;
                }
                if (code[i] == '\n') line++;
                if (code[i++] == quote) break;
            }
            continue;
        }

        if (isalnum((unsigned char)c) || c == '_') {
            size_t start = i++;

            while (i < code.size() && (isalnum((unsigned char)code[i]) || code[i] == '_')) {
                i++;
            }

            result.push_back({code.substr(start, i - start), line});
            continue;
        }

        result.push_back({string(1, c), line});
        i++;
    }

    return result;
}


bool isFunction(const vector<Token>& header) {
    for (size_t i = 1; i < header.size(); i++) {
        if (header[i].text != "(") continue;

        string name = header[i - 1].text;

        if (name == "if" || name == "for" || name == "while" || name == "switch" || name == "catch") {
            return false;
        }

        if (!name.empty() && (isalpha((unsigned char)name[0]) || name[0] == '_')) {
            return true;
        }
    }

    return false;
}


void findFunctions(const vector<Token>& tokens) {
    vector<Token> header;

    for (size_t i = 0; i < tokens.size(); i++) {
        string t = tokens[i].text;

        if (t == "{") {
            if (isFunction(header) && !header.empty()) {
                int first = header[0].line;
                int depth = 1;
                size_t j = i + 1;

                while (j < tokens.size() && depth > 0) {
                    if (tokens[j].text == "{") depth++;
                    if (tokens[j].text == "}") depth--;
                    j++;
                }

                if (depth == 0) {
                    int last = tokens[j - 1].line;
                    int length = last - first + 1;

                    if (length > 40) {
                        cout << first << ' ' << last << ' ' << length << '\n';
                    }
                }

                i = j - 1;
            }

            header.clear();
        }
        else if (t == ";" || t == "}") {
            header.clear();
        }
        else if (t == ":" && header.size() == 1 && (header[0].text == "public" || header[0].text == "private" || header[0].text == "protected")) {
            header.clear();
        }
        else {
            header.push_back(tokens[i]);
        }
    }
}

int main() {
    string code, line;

    while (getline(cin, line)) {
        code += line + '\n';
    }

    findFunctions(tokenize(code));
    return 0;
}
