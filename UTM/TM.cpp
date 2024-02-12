#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <set>
#include <list>
#include <algorithm>
#include <regex>

using namespace std;

#define ZERO '0'
#define ONE '1'
#define BLANK 'B'
#define LEFT 'L'
#define RIGHT 'R'

const vector<char> SIGMA = {ZERO, ONE};
const vector<char> GAMMA = {ZERO, ONE, BLANK};

const map<string, string> COLORS = {
    {"red", "\033[31m"},
    {"green", "\033[32m"},
    {"yellow", "\033[33m"},
    {"blue", "\033[34m"},
    {"magenta", "\033[35m"},
    {"cyan", "\033[36m"},
    {"white", "\033[37m"}          };
void err(const string& text, const string& c){ 
    try { cerr << COLORS.at(c) << text << "\033[0m" << endl; }
    catch (const exception& e) {
            cerr << "Color not found: " << c << endl; 
            for (auto& color : COLORS) cerr << color.first << endl;
    }
}

struct State {
                string name;
                State(string _name) : name(_name) {} };

struct Transition {
                State* from; State* to;
                char read, write, dir;
                Transition(State* from, State* to, char read, char write, char dir) 
                                            : from(from), to(to), read(read), write(write), dir(dir) {}
                                                        };

class Tape {
                list<char> tape;
                list<char>::iterator head;
        
        private: bool _Preprocess(string input){
                        for(char c : input)
                            if (c == ZERO || c == ONE) continue;
                            else return false;
                        return true;
                 }

        public:
                Tape(string input){
                    if (this->_Preprocess(input) == false){
                        err("Invalid input [Tape Constructor]", "red");
                        exit(1);           
                    }
                    this->tape = list<char>(input.begin(), input.end());
                    this->tape.push_back(BLANK);
                    this->head = this->tape.begin();
                                  }

                bool move(char dir){
                    try {
                        if(dir == LEFT)
                            if(this->head == this->tape.begin()) { return false; }
                            else { this->head--; }

                        else if(dir == RIGHT)
                            if(this->head != this->tape.end()) { this->head++; }
                            else { this->tape.push_back(BLANK); this->head++; }

                        else { err("Invalid direction", "red"); return false; }
                        return true;
                    }
                    catch (const exception& e) { err(e.what(), "red"); return false; }
                }

                string toString(){
                    string s = "";
                    for (auto it = this->tape.begin(); it != this->tape.end(); it++)
                        if (it == this->head) s += "[" + string(1, *it) + "]";
                        else s += string(1, *it);
                    return s;
                }
                
                friend istream& operator>>(istream& is, Tape& tape){
                                    string input; is >> input;
                                    if (tape._Preprocess(input) == false){
                                        err("Invalid input", "red");
                                        exit(1);           
                                    }
                                    tape.tape = list<char>(input.begin(), input.end());
                                    return is;
                                }

                int headPos(){ return distance(this->tape.begin(), this->head); }

                friend ostream& operator<<(ostream& os, Tape& tape){ os << tape.toString(); return os; }

                char read(){ return *this->head; }

                void write(char c){ *this->head = c; }

                void reset(){ this->head = this->tape.begin(); }
};

class TM {
                set<State*> Q = {};
                State* q0 = nullptr;
                State* qAccept = nullptr, *qReject = nullptr;
                set<Transition*> delta = {};
                Tape* tape = nullptr;

        public:
                TM(set<State*> Q, State* q0, State* qAccept, State* qReject, set<Transition*> delta)
                    : Q(Q), q0(q0), qAccept(qAccept), qReject(qReject), delta(delta) {
                        this->tape = new Tape("");
                    }
                
                TM(set<State*> Q, State* q0, State* qAccept, State* qReject, set<Transition*> delta, string input)
                    : Q(Q), q0(q0), qAccept(qAccept), qReject(qReject), delta(delta), tape(new Tape(input)) {}

                bool run(){
                    // Modifications to be made here:
                                /*
                                        After entering the reject state, reject immidiately.
                                        Rejecting is not exclusive to the reject state.

                                        * Make the function cleaner!
                                */
                    State* current = this->q0;
                    this->tape->reset();
                    try {
                        if(this->Q.find(this->q0) == this->Q.end()) { err("q0 not in Q", "red"); return false; }
                        if(this->Q.find(this->qAccept) == this->Q.end()) { err("qAccept not in Q", "red"); return false; }
                        if(this->Q.find(this->qReject) == this->Q.end()) { err("qReject not in Q", "red"); return false; }
                        for (Transition* t : this->delta){
                            if (this->Q.find(t->from) == this->Q.end()) { err("Transition from not in Q", "red"); return false; }
                            if (this->Q.find(t->to) == this->Q.end()) { err("Transition to not in Q", "red"); return false; }
                            if (t->dir != LEFT && t->dir != RIGHT) { err("Invalid direction", "red"); return false; }
                        }
                        while (current != this->qAccept && current != this->qReject){
                            char symbol = this->tape->read();
                            Transition* t = nullptr;
                            for(auto it = this->delta.begin(); it != this->delta.end(); it++){
                                if((*it)->from == current && (*it)->read == symbol){
                                    t = *it;
                                    break;
                                }
                            }
                            if (t == nullptr) { 
                                err("No transition found", "red"); 
                                err(this->config(current), "yellow");
                            }
                            this->tape->write(t->write);
                            if (this->tape->move(t->dir) == false) { 
                                err("Invalid move", "red"); 
                                err(this->config(current), "yellow");
                            }
                            current = t->to;
                        }
                        if (current == this->qAccept) return true;
                        else return false;
                    }
                    catch (const exception& e) { err(e.what(), "red"); return false; }
                }

                friend ostream& operator<<(ostream& os, TM& tm){
                    os << "Q = {";
                    for (State* q : tm.Q) os << q->name << ", ";
                    os << "}\nSigma = {0, 1}\nGamma = {0, 1, B}\nq0 = " << tm.q0->name << "\nqAccept = " << tm.qAccept->name << "\nqReject = " << tm.qReject->name << "\nDelta = {";
                    for (Transition* t : tm.delta) os << "(" << t->from->name << ", " << t->read << ") -> (" << t->to->name << ", " << t->write << ", " << t->dir << "), ";
                    os << "}\n";
                    return os;
                }

                friend istream& operator>>(istream& is, TM& tm){
                    string q0, qAccept, qReject;
                    is >> q0 >> qAccept >> qReject;
                    int N; is >> N;
                    set<State*> Q;
                    for (int i = 0; i < N; i++){
                        string name; is >> name;
                        Q.insert(new State(name));
                    }
                    int M; is >> M;
                    set<Transition*> delta;
                    for (int i = 0; i < M; i++){
                        string from, to; char read, write, dir;
                        is >> from >> read >> to >> write >> dir;
                        Transition* t = new Transition(new State(from), new State(to), read, write, dir);
                        delta.insert(t);
                    }
                    tm = TM(Q, new State(q0), new State(qAccept), new State(qReject), delta);
                    return is;
                }

                string config(State* cur){ return this->tape->toString() + "\n" + cur->name + "\n" + to_string(this->tape->headPos()); }

                string ENCODE() {
                // This function assumes that q0, q1, and q2 are the start, accept, and reject states, respectively. 
                    //In addition, it assumes that the states are named q_i where i is an integer.

                            /*
                                The encoding of a Turing machine M = <Q = {q_i}, SIGMA, GAMMA, delta, q0, q1(accept), q2(reject)> is a string of the form

                            
                            */
                }

};

class UTM {};

int main(int argc, char** argv){

    /*
        Example of a Turing machine that accepts even binary numbers.
        It only checks for the last bit of the input.
    */

    State* q0 = new State("q0");
    State* qAccept = new State("qAccept");
    State* qReject = new State("qReject");

    State* X = new State("X");

    set<State*> Q = {q0, qAccept, qReject, X};

    set<Transition*> delta = {
        new Transition(q0, q0, ZERO, ZERO, RIGHT),
        new Transition(q0, q0, ONE, ONE, RIGHT),
        new Transition(q0, X, BLANK, BLANK, LEFT),
        new Transition(X, qReject, ONE, ONE, RIGHT),
        new Transition(X, qAccept, ZERO, ZERO, RIGHT)
    };

    string inp = "0101001010100011101";

    TM tm(Q, q0, qAccept, qReject, delta, inp);

    cout << tm.run() << endl;

    return 0;
}