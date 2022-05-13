#include "api.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <fstream>
#include <map>

const std::string EPS = "@";
const std::string DEAD_NAME = "?";

enum Equ {
    equ, not_equ, undefined
};

typedef std::map<std::string, std::map<std::string, std::vector<std::string>>> RawTransitionTable;
typedef std::map<std::string, std::map<std::string, std::string>> TransitionTable;
typedef std::map<std::string, std::map<std::string, Equ>> StateEquTable;
typedef std::vector<std::set<std::string>> StateGroups;

template<class T, class E>
bool is_in(const E &element, const T &container) {
    return container.find(element) != container.end();
}

template<class T>
bool is_in(const T &element, const std::vector<T> &container) {
    return std::count(container.begin(), container.end(), element) > 0;
}

template<typename T>
std::set<T> get_union(const std::set<T> &a, const std::set<T> &b) {
    std::set<T> result = a;
    result.insert(b.begin(), b.end());
    return result;
}

TransitionTable get_transition_table(const DFA &dfa) {
    RawTransitionTable table;

    auto states = dfa.get_states();

    for (const auto &state: states) {
        for (char sym: dfa.get_alphabet().to_string()) {
            if (dfa.has_trans(state, sym)) {
                std::string dst_state = dfa.get_trans(state, sym);
                table[state][dst_state].push_back(std::string(1, sym));
            }
        }
    }

    TransitionTable res;
    for (const auto &state_i: states) {
        for (const auto &state_j: states) {
            if (table[state_i][state_j].empty()) {
                res[state_i][state_j] = "";
            } else if (table[state_i][state_j].size() == 1) {
                res[state_i][state_j] = table[state_i][state_j][0];
            } else {
                res[state_i][state_j] = table[state_i][state_j][0];
                for (int i = 1; i < table[state_i][state_j].size(); i++) {
                    res[state_i][state_j] += "|" + table[state_i][state_j][i];
                }
                res[state_i][state_j] = res[state_i][state_j];
            }
        }
    }
    return res;
}

struct MDFA {
public:
    explicit MDFA(const DFA &dfa) : api_dfa(dfa) {
        transition_table = get_transition_table(dfa);
        init_state = dfa.get_initial_state();
        final_states = dfa.get_final_states();
        active_states = dfa.get_states();
    }

    MDFA(const MDFA &mdfa, const std::string &final_state) : api_dfa(mdfa.api_dfa) {
        transition_table = mdfa.transition_table;
        init_state = mdfa.init_state;
        final_states = {final_state};
        active_states = mdfa.active_states;
    }

    //$ dot -Tsvg out.gv > out.svg
    void to_graphviz(const std::string &path) {
        std::ofstream outfile(path);
        outfile << "digraph G {" << std::endl;
        outfile << "{" << std::endl;
        outfile << "node [style=filled fillcolor=yellow]" << std::endl;
        outfile << init_state << " []" << std::endl;
        outfile << "}" << std::endl;
        outfile << "{" << std::endl;
        outfile << "node []" << std::endl;
        for (const auto &state: final_states) {
            outfile << state << " [shape=doublecircle]" << std::endl;
        }
        outfile << "}" << std::endl;

        for (const auto &state_i: active_states) {
            for (const auto &state_j: active_states) {
                if (transition_table[state_i][state_j] != "") {
                    outfile << state_i << "->" << state_j << "[ label=\"" << transition_table[state_i][state_j]
                            << "\"];"
                            << std::endl;
                }
            }
        }
        outfile << "}" << std::endl;
    }

private:
    const DFA api_dfa;
    TransitionTable transition_table;
    std::string init_state;
    std::set<std::string> final_states;
    std::set<std::string> active_states;
};

void print_state_equ_table(StateEquTable &state_equ_table, const DFA &dfa) {
    return;
    std::cout << "   ";
    for (const auto &state_i: dfa.get_states()) {
        std::cout << "    " << state_i;
    }
    std::cout << std::endl;

    for (const auto &state_i: dfa.get_states()) {
        std::cout << "  " << state_i;
        for (const auto &state_j: dfa.get_states()) {
            if (state_equ_table[state_i][state_j] == equ)
                std::cout << "  equ";
            else if (state_equ_table[state_i][state_j] == not_equ)
                std::cout << " nequ";
            else
                std::cout << " unde";
        }
        std::cout << std::endl;
    }
}

int k = 0;


Equ check_equ(const std::string &state1, const std::string &state2, DFA &dfa, StateEquTable &state_equ_table, std::map<std::string, std::map<std::string, bool>> marked) {
//    std::cout << state1 <<"  "<< state2 << std::endl;
    print_state_equ_table(state_equ_table, dfa);
    
    if (marked[state1][state2] == true or marked[state2][state1] == true) {
        state_equ_table[state1][state2] = equ;
        state_equ_table[state2][state1] = equ;
        return equ;
    }
    marked[state1][state2] = true;
    marked[state2][state1] = true;
    
    if (state1 == state2) {
        state_equ_table[state1][state2] = equ;
        state_equ_table[state2][state1] = equ;
        return equ;
    }
    if (state_equ_table[state1][state2] == equ or state_equ_table[state2][state1] == equ) {
        state_equ_table[state1][state2] = equ;
        state_equ_table[state2][state1] = equ;
        return equ;
    }
    if (state_equ_table[state1][state2] == not_equ or state_equ_table[state2][state1] == not_equ) {
        state_equ_table[state1][state2] = not_equ;
        state_equ_table[state2][state1] = not_equ;
        return not_equ;
    }
    const auto final_states = dfa.get_final_states();
    
    
    if ((is_in(state1, final_states) and !is_in(state2, final_states)) or
        (!is_in(state1, final_states) and is_in(state2, final_states))) {
        state_equ_table[state1][state2] = not_equ;
        state_equ_table[state2][state1] = not_equ;
        return not_equ;
    }
    
    
    k++;
    
    for (char alph_sym: dfa.get_alphabet().to_string()) {
        auto trans1 = dfa.get_trans(state1, alph_sym);
        auto trans2 = dfa.get_trans(state2, alph_sym);
        Equ transes_is_equal = check_equ(trans1, trans2, dfa, state_equ_table, marked);
        if (transes_is_equal == not_equ) {
            state_equ_table[state1][state2] = not_equ;
            state_equ_table[state2][state1] = not_equ;
            return not_equ;
        }
        if (transes_is_equal == undefined) {
            exit(1);
        }
    }
    //

    state_equ_table[state1][state2] = equ;
    state_equ_table[state2][state1] = equ;
    return equ;
}

StateGroups get_state_equ_pairs(DFA &dfa) {
    // add dead_station
    dfa.create_state(DEAD_NAME);
    auto states = dfa.get_states();
    for (const auto &state: dfa.get_states()) {
        for (char alph_sym: dfa.get_alphabet().to_string()) {
            if (!dfa.has_trans(state, alph_sym)) {
                dfa.set_trans(state, alph_sym, DEAD_NAME);
            }
        }
    }

    StateEquTable state_equ_table;
//    print_state_equ_table(state_equ_table, dfa);

    for (const auto &state_i: dfa.get_states()) {
        for (const auto &state_j: dfa.get_states()) {
            state_equ_table[state_i][state_j] = undefined;
        }
    }
//    print_state_equ_table(state_equ_table, dfa);



    // check equ
    StateGroups pairs;
    for (const auto &state_i: dfa.get_states()) {
        for (const auto &state_j: dfa.get_states()) {
//            std::cout << state_i << "  " << state_j << std::endl;
            std::map<std::string, std::map<std::string, bool>> marked;
//            for (const auto &state_i: dfa.get_states()) {
//                for (const auto &state_j: dfa.get_states()) {
//                    marked[state_i][state_j] = false;
//                }
//            }
            if (state_i != state_j and check_equ(state_i, state_j, dfa, state_equ_table, marked)==equ){
                if (not is_in({state_i, state_j}, pairs))
                    pairs.push_back({state_i, state_j});
            }
//            print_state_equ_table(state_equ_table, dfa);
        }
    }
    print_state_equ_table(state_equ_table, dfa);
    return pairs;
}

template<class T>
bool has_disjoint(const T &conainer1, const T &conainer2){
    for (const auto &elem: conainer1)
        if (is_in(elem, conainer2))
            return true;
    return false;
}

StateGroups merge_into_groups(const StateGroups &pairs, const DFA &dfa) {
    StateGroups groups = pairs;
    
    bool has_any_disjoint = true;
    while (has_any_disjoint) {
//        std::cout << "count of gruops: " << groups.size() << std::endl;
        has_any_disjoint = false;
        StateGroups new_groups;
        std::set<int> to_remove;
        
        for (int i = 1; i < groups.size(); i++){
            for (int j = 0; j < i; j++){
                if (has_disjoint(groups[i], groups[j])){
                    has_any_disjoint = true;
                    new_groups.push_back(get_union(groups[i], groups[j]));
                    to_remove.insert(i);
                    to_remove.insert(j);
                    // sorry about that :)
                    goto fin;
                }
            }
        }
    fin:
        
        for (int i = 0; i < groups.size(); i++){
            if (not is_in(i, to_remove)){
                new_groups.push_back(groups[i]);
            }
        }
        if (has_any_disjoint)
            groups = new_groups;
        
    }
    
    return groups;
}

std::map<std::string, std::vector<std::string>> get_new_names(const StateGroups &groups, const DFA &dfa) {
    std::map<std::string, std::vector<std::string>> result;

    // adding ungrouped states
    for (const auto &state: dfa.get_states()) {
        // if does not exist in anyone group
        bool exist = false;
        for (const auto &group: groups) {
            if (is_in(state, group)) {
                exist = true;
                break;
            }
        }
        if (not exist) {
            result[state] = {state};
        }
    }

    for (const auto &group: groups) {
        bool has_dead = false;
        if (is_in(DEAD_NAME, group))
            has_dead = true;
        
        if (has_dead) {
            continue;
        }
        std::string new_name;
        for (const auto &state: group) {
            new_name += state + "_";
        }
        result[new_name] = {};
        std::copy(group.begin(), group.end(), std::back_inserter(result[new_name]));
    }

    return result;
}

std::string in_new_names(const std::map<std::string, std::vector<std::string>> &new_names, const std::string &name) {
    for (const auto &new_name: new_names) {
        if (is_in(name, new_name.second)) {
            return new_name.first;
        }
    }
    return "Error";
}

bool is_final(const std::vector<std::string> &group, const DFA &dfa){
    if (is_in(group[0], dfa.get_final_states())){
        return true;
    } else {
        return false;
    }
}

std::string get_initial(const std::map<std::string, std::vector<std::string>> &groups, const DFA &dfa){
    auto init_state = dfa.get_initial_state();
    for (const auto &group: groups) {
        if (is_in(init_state, group.second)){
            return group.first;
        }
    }
    return "Error";
}
    
    
DFA build_dfa(const StateGroups &groups, const DFA &dfa) {
    DFA minimised_dfa(dfa.get_alphabet());
    auto new_names = get_new_names(groups, dfa);
    for (const auto &new_name: new_names) {
        minimised_dfa.create_state(new_name.first, is_final(new_name.second, dfa));
    }

    for (const auto &new_name: new_names) {
        for (const char sym: dfa.get_alphabet().to_string()) {
            if (dfa.has_trans(new_name.second[0], sym)) {
                const auto trans_to = in_new_names(new_names, dfa.get_trans(new_name.second[0], sym));
                bool x = minimised_dfa.set_trans(new_name.first, sym, trans_to);
                x = not x;

            }
        }
    }
    auto init = get_initial(new_names, dfa);
    
    minimised_dfa.set_initial(init);
    
    return minimised_dfa;
}

int c = 0;

void walk(const std::string &from_state, std::map<std::string, bool> &marked, const DFA& dfa){
    marked[from_state] = true;
    
    for (const char sym:dfa.get_alphabet().to_string()) {
        if (dfa.has_trans(from_state, sym)){
            const auto trans = dfa.get_trans(from_state, sym);
            if (not marked[trans]){
                walk(trans, marked, dfa);
            }
        }
    }
}

void delete_unattainable(DFA& dfa){
    auto init_state = dfa.get_initial_state();
    
    std::map<std::string, bool> marked;
    for (const auto& state:dfa.get_states()) {
        marked[state] = false;
    }
    
    walk(init_state, marked, dfa);
    
    for (const auto& state:dfa.get_states()) {
        if (not marked[state]){
            dfa.delete_state(state);
        }
    }
}

void delete_non_generative(DFA& dfa){
    bool has_any_non_generative = true;
    while (has_any_non_generative) {
        has_any_non_generative = false;
        for (const auto& state:dfa.get_states()) {
            if (not is_in(state, dfa.get_final_states())) {
                bool is_non_generative = true;
                for (const char sym: dfa.get_alphabet().to_string()) {
                    if (dfa.has_trans(state, sym) and dfa.get_trans(state, sym) != state) {
                        is_non_generative = false;
                        break;
                    }
                }
                if (is_non_generative) {
                    has_any_non_generative = true;
                    dfa.delete_state(state);
                }
            }
        }
    }
}


DFA dfa_minim(DFA &d) {
    auto pairs = get_state_equ_pairs(d);
    std::cout << "get_state_equ_pairs" << std::endl;
    auto groups = merge_into_groups(pairs, d);
    std::cout << "merge_into_groups" << std::endl;
    auto minim_dfa = build_dfa(groups, d);
    std::cout << "build_dfa" << std::endl;

    minim_dfa.delete_state(DEAD_NAME);
    return minim_dfa;
}
