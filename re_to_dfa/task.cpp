#include "api.hpp"
#include <string>
#include <utility>
#include <vector>
#include "iostream"
#include "map"

const char EPS = '@';

template<typename T>
std::set<T> getUnion(const std::set<T> &a, const std::set<T> &b) {
    std::set<T> result = a;
    result.insert(b.begin(), b.end());
    return result;
}

enum TypeOfOperation {
    concat, repeat, choice, none_type
};

class NameGetter {
public:
    static std::string get_name() {
        num++;
        return "q" + std::to_string(num - 1);
    }

private:
    static int num;
};

int NameGetter::num = 0;

class Converter {
public:

    struct FieldOfPositions {
        explicit FieldOfPositions() : position(0), sym('?') {}

        explicit FieldOfPositions(size_t init_position, char init_sym) : position(init_position), sym(init_sym) {}

        size_t position;
        char sym;
    };

    explicit Converter(const std::string &s, const Alphabet &alphabet) {
        f.assign(s.length(), FieldOfPositions());
        int j = 1;
        for (int i = 0; i < s.length(); i++) {
            if (alphabet.has_char(s[i])) {
                f[i].position = j;
                f[i].sym = s[i];
                j++;
            }
        }
        // #

        f.emplace_back(j, '#');
    }

    size_t convert_to_pos(size_t raw_position) {
        return f[raw_position].position;
    }

    char convert_to_sym(const size_t position) const {
        for (auto i = 0; i < f.max_size(); i++) {
            if (f[i].position == position) {
                return f[i].sym;
            }
        }
        return -1;
    }

    size_t get_max_pos() {
        if (!f.empty())
            return f.back().position;
        else
            return -1;
    }

private:
    std::vector<FieldOfPositions> f;
};

struct Node {

    explicit Node(const char init_sym, size_t position) : sym(init_sym),
                                                          is_leaf(true),
                                                          left(nullptr),
                                                          mid(nullptr),
                                                          right(nullptr),
                                                          type(none_type),
                                                          first_pos({position}),
                                                          last_pos({position}) {
        nullable = (init_sym == EPS);
    }

    explicit Node(TypeOfOperation init_type, Node *left, Node *right) : sym('?'),
                                                                        is_leaf(false),
                                                                        left(left),
                                                                        mid(nullptr),
                                                                        right(right),
                                                                        type(init_type),
                                                                        nullable(false) {}

    explicit Node(TypeOfOperation init_type, Node *mid) : sym('?'), is_leaf(false),
                                                          left(nullptr),
                                                          mid(mid),
                                                          right(nullptr),
                                                          type(init_type),
                                                          nullable(false) {}


    Node *left;
    Node *right;
    Node *mid;

    const bool is_leaf;
    const TypeOfOperation type;
    const char sym;

    bool nullable;
    std::set<size_t> first_pos;
    std::set<size_t> last_pos;
};


struct Parser {

    const std::string s;
    size_t current_position;
    char current_sym;
    const Alphabet alphabet;
    Converter converter;

    explicit Parser(const std::string &init_s, const Alphabet &init_alphabet) :
            s(init_s),
            alphabet(init_alphabet),
            converter(s, alphabet) {
        current_position = s.length();
        // std::cout << "Init s: " << s << std::endl;
    }

    char next_sym() {
        if (current_position == 0) {
            std::cout << "Segfauld" << std::endl;
            return '?';
        }
        current_position--;
        current_sym = s[current_position];
//        std::cout << std::string("Cur sym ") + s[current_position] << std::endl;
        return s[current_position];
    }

    void back_sym() {
        current_position++;
        current_sym = s[current_position];
//        std::cout << std::string("Cur sym (back_sym)") + s[current_position] << std::endl;
    }

    Node *E() {
        // std::cout << "E is called" << std::endl;
        // sleep(1);

        Node *right = T();
        Node *left = nullptr;
        char sym = next_sym();
        if (sym == '|') {
            left = E();
            return new Node(choice, left, right);
        } else if (sym == '#') {
            std::cout << "Successful reading!" << std::endl;
//            left = new Node('#', converter.get_max_pos());
//            return new Node(concat, right, left);
            return right;
        } else {
            back_sym();
            return right;
        }
    }

    Node *T() {
        // std::cout << "T is called" << std::endl;
        // sleep(1);
        Node *left;
        Node *right;

        right = F();
        char sym = next_sym();
        back_sym();

        if (sym != '|' and sym != '#' and sym != '(') {
            left = T();
            return new Node(concat, left, right);
        }
        return right;
    }

    Node *F() {
        // std::cout << "F is called" << std::endl;
        // sleep(1);
        Node *mid;

        char sym = next_sym();
        if (sym == '*') {
            sym = next_sym();
            if (sym == ')') {
                mid = E();
                next_sym(); // (
                return new Node(repeat, mid);
            } else {
                back_sym();
                mid = C();
                return new Node(repeat, mid);
            }

        } else if (sym == ')') {
            mid = E();
            next_sym(); // (
            return mid;
        } else {
            back_sym();
            mid = C();
            return mid;
        }
    }

    Node *C() {
        // std::cout << "C is called" << std::endl;
        // sleep(1);

        char sym = next_sym();
        if (!alphabet.has_char(sym)) {
            // std::cout << "Read eps" << std::endl;
            back_sym();
            return new Node(EPS, converter.convert_to_pos(current_position));
        } else {
            // std::cout << "Read " << sym << std::endl;
            return new Node(sym, converter.convert_to_pos(current_position));
        }
    }
    // (a|)*
};

bool fill_nullable(Node *tree) {
    bool left;
    bool right;
    switch (tree->type) {
        case concat:
            left = fill_nullable(tree->left);
            right = fill_nullable(tree->right);
            tree->nullable = left and right;
            return left and right;
        case repeat:
            fill_nullable(tree->mid);
            tree->nullable = true;
            return true;
        case choice:
            left = fill_nullable(tree->left);
            right = fill_nullable(tree->right);
            tree->nullable = left or right;
            return left or right;
        case none_type:
            return tree->nullable;
    }
}

std::set<size_t> fill_firstpos(Node *tree) {
    std::set<size_t> left;
    std::set<size_t> right;
    std::set<size_t> mid;

    switch (tree->type) {
        case concat:
            if (tree->left->nullable) {
                left = fill_firstpos(tree->left);
                right = fill_firstpos(tree->right);
                tree->first_pos = getUnion(left, right);
                return tree->first_pos;
            } else {
                left = fill_firstpos(tree->left);
                fill_firstpos(tree->right);

                tree->first_pos = left;
                return tree->first_pos;
            }
        case repeat:
            mid = fill_firstpos(tree->mid);
            tree->first_pos = mid;
            return tree->first_pos;
        case choice:
            left = fill_firstpos(tree->left);
            right = fill_firstpos(tree->right);
            tree->first_pos = getUnion(left, right);
            return tree->first_pos;
        case none_type:
            return tree->first_pos;
    }
}

std::set<size_t> fill_lastpos(Node *tree) {
    std::set<size_t> left;
    std::set<size_t> right;
    std::set<size_t> mid;

    switch (tree->type) {
        case concat:
            if (tree->right->nullable) {
                left = fill_lastpos(tree->left);
                right = fill_lastpos(tree->right);
                tree->last_pos = getUnion(left, right);
                return tree->last_pos;
            } else {
                right = fill_lastpos(tree->right);
                fill_lastpos(tree->left);

                tree->last_pos = right;
                return tree->last_pos;
            }
        case repeat:
            mid = fill_lastpos(tree->mid);
            tree->last_pos = mid;
            return tree->last_pos;
        case choice:
            left = fill_lastpos(tree->left);
            right = fill_lastpos(tree->right);
            tree->last_pos = getUnion(left, right);
            return tree->last_pos;
        case none_type:
            return tree->last_pos;
    }
}

void fill_follow_pos(Node *tree, std::vector<std::set<size_t>> &table_follow_pos) {
    if (!tree->is_leaf) {
        if (tree->left != nullptr) fill_follow_pos(tree->left, table_follow_pos);
        if (tree->right != nullptr) fill_follow_pos(tree->right, table_follow_pos);
        if (tree->mid != nullptr) fill_follow_pos(tree->mid, table_follow_pos);
    }
    if (tree->type == concat) {
        for (size_t pos: tree->left->last_pos) {
            table_follow_pos[pos] = getUnion(table_follow_pos[pos], tree->right->first_pos);
        }
    } else if (tree->type == repeat) {
        for (size_t pos: tree->mid->last_pos) {
            table_follow_pos[pos] = getUnion(table_follow_pos[pos], tree->mid->first_pos);
        }
    }
}

class DFAHelper {
public:
    std::map<const std::string, std::set<size_t>> name_to_pos = {};
    std::map<const std::string, bool> marked = {};

    std::map<std::set<size_t>, const std::string> pos_to_name = {};

    void create_state(const std::string &name, const std::set<size_t> set) {
        name_to_pos[name] = set;
        marked[name] = false;
    }

    std::string get_name(const std::set<size_t> &by) {
        for (const auto &name: name_to_pos) {
            if (name.second == by) {
                return name.first;
            }
        }
        return "?";
    }

    void set_as_marked(const std::string &s) {
        marked[s] = true;
    }

    void set_as_marked(const std::set<size_t> set) {
        marked[get_name(set)] = true;
    }

};

void create_DFA(DFA &dfa, std::set<size_t> &current_set, std::vector<std::set<size_t>> &table_follow_pos,
                const Alphabet &alphabet, const std::string &s,
                const Parser &parser, DFAHelper &helper) {
    // std::cout << dfa.to_string() << std::endl;

    helper.set_as_marked(current_set);
    for (char sym: alphabet.to_string()) {
        std::set<size_t> positions = {};
        for (size_t position_in_string: current_set) {
            if (sym == parser.converter.convert_to_sym(position_in_string)) {
                positions.insert(position_in_string);
            }
        }
        std::set<size_t> S{};
        for (size_t pos: positions) {
            S = getUnion(S, table_follow_pos[pos]);
        }
        if (S.empty()) {
            continue;
        }
        if (!dfa.has_state(helper.get_name(S))) {
            std::string name = NameGetter::get_name();
            auto end_pos = table_follow_pos.size() - 1;
            const bool is_final = S.find(end_pos) != S.end();
            dfa.create_state(name, is_final);
            helper.create_state(name, S);
            // std::cout << helper.get_name(current_set) << std::endl;
            // std::cout << helper.get_name(S) << std::endl;
            dfa.set_trans(helper.get_name(current_set), sym, helper.get_name(S));

            create_DFA(dfa, S, table_follow_pos, alphabet, s, parser, helper);
        } else {
            dfa.set_trans(helper.get_name(current_set), sym, helper.get_name(S));
        }
    }
}

DFA re2dfa(const std::string &s) {

    // std::cout << s << std::endl;
    Parser parser('#' + s, Alphabet(s));
    Node *right = parser.E();
    Node *left = new Node('#', parser.converter.get_max_pos());
    Node *tree = new Node(concat, right, left);
    fill_nullable(tree);
    fill_firstpos(tree);
    fill_lastpos(tree);

    std::vector<std::set<size_t>> table_follow_pos;
    table_follow_pos.assign(parser.converter.get_max_pos() + 1, {});

    fill_follow_pos(tree, table_follow_pos);

    DFAHelper helper;
    DFA dfa = DFA(Alphabet(s));
    std::string name_of_state = NameGetter::get_name();
    std::set<size_t> first_pos_root = tree->first_pos;

    auto end_pos = table_follow_pos.size() - 1;
    const bool is_final = first_pos_root.find(end_pos) != first_pos_root.end();

    dfa.create_state(name_of_state, is_final);
    helper.create_state(name_of_state, first_pos_root);
    dfa.set_initial(name_of_state);


    create_DFA(dfa, first_pos_root, table_follow_pos, Alphabet(s), s, parser, helper);

    return dfa;
}
