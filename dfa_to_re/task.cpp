#include "api.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <fstream>

const std::string EPS = "@";

typedef std::map<std::string, std::map<std::string, std::vector<std::string>>> RawTransitionTable;
typedef std::map<std::string, std::map<std::string, std::string>> TransitionTable;

template<class T, class E>
bool is_in(const E &element, const T &container) {
    return container.find(element) != container.end();
}

template<class T>
bool is_in(const T &element, const std::vector<T> &container) {
    return std::count(container.begin(), container.end(), element) > 0;
}


bool only_eps(const std::vector<std::string> &regs) {
    for (const auto &reg: regs) {
        if (reg != EPS) {
            return false;
        }
    }
    return true;
}

std::string star(const std::string &reg){
    if (reg.length() == 1)
        return reg + "*";
    else
        return "(" + reg + ")*";
}

std::string concat(const std::vector<std::string> &regs) {

    if (std::find(regs.begin(), regs.end(), "") != regs.end()) {
        return "";
    } else if (only_eps(regs)) {
        return EPS;
    } else {
        std::string res = "";
        for (const auto &reg: regs) {
            if (reg != EPS)
                res += reg;
        }
        return res;
    }
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
    
    table["INIT"][dfa.get_initial_state()].push_back(EPS);
    states.insert("INIT");
    
    
    for (const auto &state: states) {
        if (is_in(state, dfa.get_final_states())){
            table[state]["FINAL"].push_back(EPS);
        }
    }
    states.insert("FINAL");
    
    TransitionTable res;
    for (const auto &state_i: states) {
        for (const auto &state_j: states) {
            if (table[state_i][state_j].size() == 0) {
                res[state_i][state_j] = "";
            } else if (table[state_i][state_j].size() == 1) {
                res[state_i][state_j] = table[state_i][state_j][0];
            } else {
                res[state_i][state_j] = table[state_i][state_j][0];
                for (int i = 1; i < table[state_i][state_j].size(); i++) {
                    res[state_i][state_j] += "|" + table[state_i][state_j][i];
                }
                res[state_i][state_j] = "(" + res[state_i][state_j] + ")";
            }
        }
    }
    return res;
}

struct MDFA {
public:
    explicit MDFA(const DFA &dfa) {
        table = get_transition_table(dfa);
//        init_state = dfa.get_initial_state();
        init_state = "INIT";
//        final_states = dfa.get_final_states();
        final_states = std::set<std::string>({"FINAL"});
        active_states = dfa.get_states();
        active_states.insert("INIT");
        active_states.insert("FINAL");
    }

    MDFA(const MDFA &mdfa, const std::string &final_state) {
        table = mdfa.table;
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
                if (table[state_i][state_j] != "") {
                    outfile << state_i << "->" << state_j << "[ label=\"" << table[state_i][state_j] << "\"];"
                            << std::endl;
                }
            }
        }
        outfile << "}" << std::endl;
    }

    std::string min_in_out() {
        std::map<std::string, int> count_in_out;
        for (const auto &state: active_states) {
            count_in_out[state] = 0;
        }
        int max = 0;
        for (const auto &state_i: active_states) {
            for (const auto &state_j: active_states) {
                if (table[state_i][state_j] != "") {
                    count_in_out[state_i] += 1;
                    count_in_out[state_j] += 1;
                    if (max < count_in_out[state_i])
                        max = count_in_out[state_i];
                    if (max < count_in_out[state_j])
                        max = count_in_out[state_j];
                }
            }
        }

        int min = max + 1;
        std::string min_state;

        for (auto &state: active_states) {
            if ((count_in_out[state] <= min) and
                !(final_states.find(state) != final_states.end()) and
                (init_state != state)) {
                min = count_in_out[state];
                min_state = state;
            }
        }
        return min_state;
    }

    void delete_state(const std::string &to_rm_state) {
        std::vector<std::string> in_states;
        std::vector<std::string> out_states;
        for (const auto &state: active_states) {
            if (state == to_rm_state)
                continue;
            if (table[state][to_rm_state] != "") {
                in_states.push_back(state);
            }
            if (table[to_rm_state][state] != "") {
                out_states.push_back(state);
            }
        }

        std::vector<std::string> q;
        std::vector<std::string> p;
        std::vector<std::vector<std::string>> R;
        std::string S;

        q.assign(in_states.size(), "");
        p.assign(out_states.size(), "");
        R.assign(in_states.size(), p);
        if (table[to_rm_state][to_rm_state] != "")
            S = star(table[to_rm_state][to_rm_state]);
        else
            S = EPS;

        table[to_rm_state][to_rm_state] = "";

        //fill q, p, R
        int i = 0;
        for (const auto &in_state: in_states) {
            q[i] = table[in_state][to_rm_state];
            table[in_state][to_rm_state] = "";
            i++;
        }
        i = 0;
        for (const auto &out_state: out_states) {
            p[i] = table[to_rm_state][out_state];
            table[to_rm_state][out_state] = "";
            i++;
        }
        i = 0;
        for (const auto &in_state: in_states) {
            int j = 0;
            for (const auto &out_state: out_states) {
                R[i][j] = table[in_state][out_state];
                j++;
            }
            i++;
        }
        i = 0;
        for (const auto &in_state: in_states) {
            int j = 0;
            for (const auto &out_state: out_states) {
                if (R[i][j] != "")
                    table[in_state][out_state] = "(" + R[i][j] + "|" + concat({q[i], S, p[j]}) + ")";
                else
                    table[in_state][out_state] = concat({q[i], S, p[j]});
                j++;
            }
            i++;
        }
        active_states.erase(to_rm_state);
    }

    void delete_intermediate_states() {
        std::string state_to_rm;
        while ((state_to_rm = min_in_out()) != "") {
            delete_state(state_to_rm);
        }
    }

    friend std::string delete_finals(const MDFA &dfa);

private:
    TransitionTable table;
    std::string init_state;
    std::set<std::string> final_states;
    std::set<std::string> active_states;
};

std::string delete_finals(const MDFA &dfa) {
    std::set<std::string> X;

    for (const auto &final_state: dfa.final_states) {
        MDFA one_final_dfa(dfa, final_state);
        one_final_dfa.delete_intermediate_states();

        auto init_state = one_final_dfa.init_state;

        if (one_final_dfa.init_state == final_state) {
            std::string R = one_final_dfa.table[init_state][final_state];
            if (R == "") {
                X.insert(EPS);
            } else {
                X.insert(star(R));
            }
        } else {
            std::string R, S, U, T;
            R = one_final_dfa.table[init_state][init_state];
            S = one_final_dfa.table[init_state][final_state];
            U = one_final_dfa.table[final_state][final_state];
            if (U != "") {
                U = star(U);
            } else {
                U = EPS;
            }
            T = one_final_dfa.table[final_state][init_state];

            std::string res;
            if (R == "" and concat({S, U, T}) == "")
                res = concat({S, U});
            else if (R == "")
                res = star(concat({S, U, T})) + concat({S, U});
            else if (concat({S, U, T}) == "")
                
                res = star(R) + concat({S, U});

            X.insert(res);
        }

    }

    
    std::string res;

    bool has_eps = std::find(X.begin(), X.end(), EPS) != X.end();
//    has_eps = false;
    for (const auto &reg : X) {
        if (reg != EPS)
            res += "|" + reg;
    }

    if (!has_eps) {
        res.erase(0, 1);
    }
    
    res.erase(std::remove(res.begin(), res.end(), '@'), res.end());
    
//    if (res.find("()*") != std::string::npos)
//        return "";
    return res;
    
}


std::string dfa2re(DFA &d) {
    
    MDFA my_dfa(d);
    my_dfa.to_graphviz("../out.gv");
    my_dfa.delete_intermediate_states();
    my_dfa.to_graphviz("../out1.gv");
    const auto res = delete_finals(my_dfa);
    std::cout << "Ans: " << res << std::endl;
    return res;
}
