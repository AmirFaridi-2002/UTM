#include <cassert>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

constexpr char ZERO = '0';
constexpr char ONE = '1';
constexpr char BLANK = 'B';
constexpr char LEFT = 'L';
constexpr char RIGHT = 'R';

const std::map<std::string, std::string> COLORS{
    {"red", "\033[31m"},   {"green", "\033[32m"},   {"yellow", "\033[33m"},
    {"blue", "\033[34m"},  {"magenta", "\033[35m"}, {"cyan", "\033[36m"},
    {"white", "\033[37m"}, {"reset", "\033[0m"}};

static void err(const std::string &text, const std::string &c = "red") {
  try {
    std::cerr << COLORS.at(c) << text << COLORS.at("reset") << std::endl;
  } catch (const std::out_of_range &) {
    std::cerr << "Colour not found: " << c << std::endl;
    for (const auto &p : COLORS) {
      if (p.first != "reset")
        std::cerr << p.first << std::endl;
    }
  }
}

struct State {
  explicit State(std::string name) : name(std::move(name)) {}
  std::string name;
};

struct Transition {
  std::shared_ptr<State> from;
  std::shared_ptr<State> to;
  char read;
  char write;
  char dir;
  Transition(std::shared_ptr<State> from, std::shared_ptr<State> to, char read,
             char write, char dir)
      : from(std::move(from)), to(std::move(to)), read(read), write(write),
        dir(dir) {}
};

class Tape {
public:
  explicit Tape(const std::string &input) {
    preprocess(input);
    tape_.assign(input.begin(), input.end());
    tape_.push_back(BLANK);
    head_ = tape_.begin();
  }

  Tape() {
    tape_.push_back(BLANK);
    head_ = tape_.begin();
  }

  void reset() { head_ = tape_.begin(); }

  bool move(char dir) {
    if (dir == LEFT) {
      if (head_ == tape_.begin()) {
        return false;
      }
      --head_;
    } else if (dir == RIGHT) {
      ++head_;
      if (head_ == tape_.end()) {
        tape_.push_back(BLANK);
        head_ = std::prev(tape_.end());
      }
    } else {
      err("Invalid move direction: " + std::string(1, dir), "red");
      return false;
    }
    return true;
  }

  char read() const { return *head_; }

  void write(char c) { *head_ = c; }

  std::string to_string() const {
    std::stringstream ss;
    for (auto it = tape_.begin(); it != tape_.end(); ++it) {
      if (it == head_) {
        ss << '[' << *it << ']';
      } else {
        ss << *it;
      }
    }
    return ss.str();
  }

  int head_pos() const {
    std::list<char>::const_iterator h =
        head_; // convert iterator -> const_iterator
    return static_cast<int>(std::distance(tape_.cbegin(), h));
  }

private:
  void preprocess(const std::string &input) {
    for (char c : input) {
      if (c != ZERO && c != ONE) {
        throw std::invalid_argument(
            "Tape input may only contain '0' and '1' characters.");
      }
    }
  }

  std::list<char> tape_;
  std::list<char>::iterator head_;
};

class TM {
public:
  TM(std::set<std::shared_ptr<State>> Q, std::shared_ptr<State> q0,
     std::shared_ptr<State> qAccept, std::shared_ptr<State> qReject,
     std::set<std::shared_ptr<Transition>> delta, const std::string &input = "")
      : Q_(std::move(Q)), q0_(std::move(q0)), qAccept_(std::move(qAccept)),
        qReject_(std::move(qReject)), delta_(std::move(delta)) {
    if (input.empty())
      tape_ = std::make_unique<Tape>();
    else
      tape_ = std::make_unique<Tape>(input);
  }

  bool run() {
    if (!valid_configuration())
      return false;
    auto current = q0_;
    tape_->reset();

    while (true) {
      if (current == qAccept_)
        return true;
      if (current == qReject_)
        return false;

      char symbol = tape_->read();
      std::shared_ptr<Transition> selected = nullptr;
      for (const auto &t : delta_) {
        if (t->from == current && t->read == symbol) {
          selected = t;
          break;
        }
      }
      if (!selected) {
        err("No transition found from state '" + current->name +
                "' reading symbol '" + std::string(1, symbol) + "'",
            "red");
        err(config(current), "yellow");
        return false;
      }
      tape_->write(selected->write);
      if (!tape_->move(selected->dir)) {
        err("Invalid move detected", "red");
        err(config(current), "yellow");
        return false;
      }
      current = selected->to;
    }
  }

  std::string description() const {
    std::stringstream ss;
    ss << "Q = {";
    bool first = true;
    for (const auto &q : Q_) {
      if (!first)
        ss << ", ";
      ss << q->name;
      first = false;
    }
    ss << "}\nSigma = {0, 1}\nGamma = {0, 1, B}\nq0 = " << q0_->name
       << "\nqAccept = " << qAccept_->name << "\nqReject = " << qReject_->name
       << "\nDelta = {";
    first = true;
    for (const auto &t : delta_) {
      if (!first)
        ss << ", ";
      ss << "(" << t->from->name << ", " << t->read << ") -> (" << t->to->name
         << ", " << t->write << ", " << t->dir << ")";
      first = false;
    }
    ss << "}";
    return ss.str();
  }

  std::string encode() const {
    std::stringstream ss;
    bool first = true;
    for (const auto &t : delta_) {
      if (!first)
        ss << ';';
      // Extract numeric part of state names by stripping leading non‑digit
      // characters.  If parsing fails, we treat the number as 0.
      auto extract_index = [](const std::string &s) -> int {
        std::smatch m;
        if (std::regex_search(s, m, std::regex("\\d+"))) {
          return std::stoi(m.str());
        }
        return 0;
      };
      int from_idx = extract_index(t->from->name);
      int to_idx = extract_index(t->to->name);
      ss << from_idx << '#';
      ss << t->read << '#';
      ss << to_idx << '#';
      ss << t->write << '#';
      // Encode direction: L = 0, R = 1
      ss << (t->dir == LEFT ? '0' : '1');
      first = false;
    }
    return ss.str();
  }

  std::string config(const std::shared_ptr<State> &cur) const {
    std::stringstream ss;
    ss << tape_->to_string() << "\n" << cur->name << "\n" << tape_->head_pos();
    return ss.str();
  }

private:
  bool valid_configuration() const {
    if (!Q_.count(q0_)) {
      err("Initial state q0 not in Q", "red");
      return false;
    }
    if (!Q_.count(qAccept_)) {
      err("Accepting state not in Q", "red");
      return false;
    }
    if (!Q_.count(qReject_)) {
      err("Rejecting state not in Q", "red");
      return false;
    }
    for (const auto &t : delta_) {
      if (!Q_.count(t->from)) {
        err("Transition from state not in Q: " + t->from->name, "red");
        return false;
      }
      if (!Q_.count(t->to)) {
        err("Transition to state not in Q: " + t->to->name, "red");
        return false;
      }
      if (t->dir != LEFT && t->dir != RIGHT) {
        err("Invalid direction in transition: " + std::string(1, t->dir),
            "red");
        return false;
      }
    }
    return true;
  }

  std::set<std::shared_ptr<State>> Q_;
  std::shared_ptr<State> q0_;
  std::shared_ptr<State> qAccept_;
  std::shared_ptr<State> qReject_;
  std::set<std::shared_ptr<Transition>> delta_;
  std::unique_ptr<Tape> tape_;
};

static std::pair<TM, std::string>
build_even_number_tm(const std::string &input) {
  auto q0 = std::make_shared<State>("q0");
  auto qAccept = std::make_shared<State>("qAccept");
  auto qReject = std::make_shared<State>("qReject");
  auto X = std::make_shared<State>("X");
  std::set<std::shared_ptr<State>> Q{q0, qAccept, qReject, X};
  std::set<std::shared_ptr<Transition>> delta{
      std::make_shared<Transition>(q0, q0, ZERO, ZERO, RIGHT),
      std::make_shared<Transition>(q0, q0, ONE, ONE, RIGHT),
      std::make_shared<Transition>(q0, X, BLANK, BLANK, LEFT),
      std::make_shared<Transition>(X, qReject, ONE, ONE, RIGHT),
      std::make_shared<Transition>(X, qAccept, ZERO, ZERO, RIGHT)};
  TM tm(Q, q0, qAccept, qReject, delta, input);
  return {std::move(tm), input};
}

static bool run_test(const std::string &name, TM &tm, bool expected) {
  bool result = tm.run();
  std::cout << "Test " << name << ": expected "
            << (expected ? "accept" : "reject") << ", got "
            << (result ? "accept" : "reject") << ".\n";
  if (result != expected) {
    std::cout << "  FAILED" << std::endl;
  } else {
    std::cout << "  PASSED" << std::endl;
  }
  return result == expected;
}

int main() {
  bool all_ok = true;

  // Test 1: Even number TM should accept strings ending with 0 and reject those
  // ending with 1.  We use two inputs to test both cases.
  {
    auto pair0 = build_even_number_tm("0101001010100011100");
    all_ok &= run_test("EvenNumber_Accept", pair0.first, true);
    auto pair1 = build_even_number_tm("0101001010100011101");
    all_ok &= run_test("EvenNumber_Reject", pair1.first, false);
  }

  // Test 2: TM with missing transition should immediately reject.  We
  // construct a machine that has no transitions at all and run it on
  // non‑empty input.  It should reject because there is no transition
  // defined for the initial state reading '0'.
  {
    auto q0 = std::make_shared<State>("q0");
    auto qAccept = std::make_shared<State>("qAccept");
    auto qReject = std::make_shared<State>("qReject");
    std::set<std::shared_ptr<State>> Q{q0, qAccept, qReject};
    std::set<std::shared_ptr<Transition>> empty_delta;
    TM tm(Q, q0, qAccept, qReject, empty_delta, "0");
    all_ok &= run_test("MissingTransition", tm, false);
  }

  // Test 3: TM rejects immediately when trying to move off the left end of
  // the tape.  We create a machine that attempts to move left from the
  // starting position without checking the cell, causing a reject.
  {
    auto q0 = std::make_shared<State>("q0");
    auto qAccept = std::make_shared<State>("qAccept");
    auto qReject = std::make_shared<State>("qReject");
    std::set<std::shared_ptr<State>> Q{q0, qAccept, qReject};
    std::set<std::shared_ptr<Transition>> delta{
        std::make_shared<Transition>(q0, qAccept, ZERO, ZERO, LEFT)};
    TM tm(Q, q0, qAccept, qReject, delta, "0");
    all_ok &= run_test("MoveOffLeft", tm, false);
  }

  // Test 4: Encoding function.  We build a very small machine and check
  // that encode() produces the expected string.  The machine has a
  // single transition (q0, 0) -> (q1, 1, R).  According to the encoding
  // scheme, this should become "0#0#1#1#1".
  {
    auto q0 = std::make_shared<State>("q0");
    auto q1 = std::make_shared<State>("q1");
    auto qAccept = q1;
    auto qReject = std::make_shared<State>("qReject");
    std::set<std::shared_ptr<State>> Q{q0, q1, qReject};
    std::set<std::shared_ptr<Transition>> delta{
        std::make_shared<Transition>(q0, q1, ZERO, ONE, RIGHT)};
    TM tm(Q, q0, qAccept, qReject, delta, "0");
    std::string enc = tm.encode();
    bool ok = (enc == "0#0#1#1#1");
    std::cout << "Test Encoding: expected 0#0#1#1#1, got " << enc << "\n";
    if (!ok)
      std::cout << "  FAILED" << std::endl;
    else
      std::cout << "  PASSED" << std::endl;
    all_ok &= ok;
  }

  if (!all_ok) {
    std::cerr << "\nSome tests failed." << std::endl;
    return 1;
  }
  std::cout << "\nAll tests passed." << std::endl;
  return 0;
}
