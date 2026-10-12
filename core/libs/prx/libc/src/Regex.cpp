#include "prx/libc/include/general/VabiMacros.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
constexpr int FlagExtended = 0001;
constexpr int FlagIgnoreCase = 0002;
constexpr int FlagNoSub = 0004;
constexpr int FlagNewline = 0010;
constexpr int FlagNoSpec = 0020;
constexpr int FlagEndPointer = 0040;
constexpr int FlagNotBol = 00001;
constexpr int FlagNotEol = 00002;
constexpr int FlagStartEnd = 00004;
constexpr int FlagNameToCode = 255;
constexpr int FlagCodeToName = 0400;

constexpr int ErrNoMatch = 1;
constexpr int ErrBadPattern = 2;
constexpr int ErrCollate = 3;
constexpr int ErrCType = 4;
constexpr int ErrEscape = 5;
constexpr int ErrSubReg = 6;
constexpr int ErrBracket = 7;
constexpr int ErrParen = 8;
constexpr int ErrBrace = 9;
constexpr int ErrBadBrace = 10;
constexpr int ErrRange = 11;
constexpr int ErrSpace = 12;
constexpr int ErrBadRepeat = 13;
constexpr int ErrEmpty = 14;
constexpr int ErrInvalidArgument = 16;
constexpr int ErrIllegalSequence = 17;

constexpr int MagicCompiled = (('r' ^ 0200) << 8) | 'e';
constexpr int MagicGuts = (('R' ^ 0200) << 8) | 'E';
constexpr int DupMax = 255;
constexpr int Infinite = DupMax + 1;
constexpr int Outside = 256;
constexpr std::size_t ElementLimit = std::size_t{1} << 19;
constexpr int MaxNesting = 200;

using Position = std::int64_t;
constexpr Position NoPosition = std::numeric_limits<Position>::min();
constexpr std::size_t WordUsed = std::numeric_limits<std::size_t>::max();

struct GuestRegex {
    int magic;
    std::size_t subexpressions;
    const char* end;
    void* guts;
};
static_assert(sizeof(GuestRegex) == 32);

struct GuestMatch {
    std::int64_t start;
    std::int64_t end;
};
static_assert(sizeof(GuestMatch) == 16);

struct CollatingName {
    const char* name;
    unsigned char code;
};

constexpr CollatingName CollatingNames[] = {
    {"NUL", 0},
    {"SOH", 1},
    {"STX", 2},
    {"ETX", 3},
    {"EOT", 4},
    {"ENQ", 5},
    {"ACK", 6},
    {"BEL", 7},
    {"alert", 7},
    {"BS", 8},
    {"backspace", 8},
    {"HT", 9},
    {"tab", 9},
    {"LF", 10},
    {"newline", 10},
    {"VT", 11},
    {"vertical-tab", 11},
    {"FF", 12},
    {"form-feed", 12},
    {"CR", 13},
    {"carriage-return", 13},
    {"SO", 14},
    {"SI", 15},
    {"DLE", 16},
    {"DC1", 17},
    {"DC2", 18},
    {"DC3", 19},
    {"DC4", 20},
    {"NAK", 21},
    {"SYN", 22},
    {"ETB", 23},
    {"CAN", 24},
    {"EM", 25},
    {"SUB", 26},
    {"ESC", 27},
    {"IS4", 28},
    {"FS", 28},
    {"IS3", 29},
    {"GS", 29},
    {"IS2", 30},
    {"RS", 30},
    {"IS1", 31},
    {"US", 31},
    {"space", 32},
    {"exclamation-mark", 33},
    {"quotation-mark", 34},
    {"number-sign", 35},
    {"dollar-sign", 36},
    {"percent-sign", 37},
    {"ampersand", 38},
    {"apostrophe", 39},
    {"left-parenthesis", 40},
    {"right-parenthesis", 41},
    {"asterisk", 42},
    {"plus-sign", 43},
    {"comma", 44},
    {"hyphen", 45},
    {"hyphen-minus", 45},
    {"period", 46},
    {"full-stop", 46},
    {"slash", 47},
    {"solidus", 47},
    {"zero", 48},
    {"one", 49},
    {"two", 50},
    {"three", 51},
    {"four", 52},
    {"five", 53},
    {"six", 54},
    {"seven", 55},
    {"eight", 56},
    {"nine", 57},
    {"colon", 58},
    {"semicolon", 59},
    {"less-than-sign", 60},
    {"equals-sign", 61},
    {"greater-than-sign", 62},
    {"question-mark", 63},
    {"commercial-at", 64},
    {"left-square-bracket", 91},
    {"backslash", 92},
    {"reverse-solidus", 92},
    {"right-square-bracket", 93},
    {"circumflex", 94},
    {"circumflex-accent", 94},
    {"underscore", 95},
    {"low-line", 95},
    {"grave-accent", 96},
    {"left-brace", 123},
    {"left-curly-bracket", 123},
    {"vertical-line", 124},
    {"right-brace", 125},
    {"right-curly-bracket", 125},
    {"tilde", 126},
    {"DEL", 127},
};

bool IsAlpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool IsDigit(int c) { return c >= '0' && c <= '9'; }
bool IsLower(int c) { return c >= 'a' && c <= 'z'; }
bool IsUpper(int c) { return c >= 'A' && c <= 'Z'; }
bool IsWord(int c) { return IsAlpha(c) || IsDigit(c) || c == '_'; }
int ToLower(int c) { return IsUpper(c) ? c + 32 : c; }
int ToUpper(int c) { return IsLower(c) ? c - 32 : c; }

bool InClass(std::string_view name, int c) {
    const bool space = c == ' ' || (c >= 9 && c <= 13);
    const bool graph = c > 32 && c < 127;
    if (name == "alnum") return IsAlpha(c) || IsDigit(c);
    if (name == "alpha") return IsAlpha(c);
    if (name == "blank") return c == ' ' || c == '\t';
    if (name == "cntrl") return c < 32 || c == 127;
    if (name == "digit") return IsDigit(c);
    if (name == "graph") return graph;
    if (name == "lower") return IsLower(c);
    if (name == "print") return graph || c == ' ';
    if (name == "punct") return graph && !IsAlpha(c) && !IsDigit(c);
    if (name == "space") return space;
    if (name == "upper") return IsUpper(c);
    if (name == "xdigit") return IsDigit(c) || (ToLower(c) >= 'a' && ToLower(c) <= 'f');
    if (name == "number") return IsDigit(c);
    if (name == "rune") return c < 128;
    return false;
}

bool IsClassName(std::string_view name) {
    static constexpr std::string_view Names[] = {"alnum", "alpha", "blank", "cntrl", "digit", "graph",
                                                 "lower", "print", "punct", "space", "upper", "xdigit",
                                                 "ideogram", "special", "phonogram", "number", "rune"};
    return std::find(std::begin(Names), std::end(Names), name) != std::end(Names);
}

enum class Op : std::uint8_t { Char, Set, Bol, Eol, Bow, Eow, Open, Close, Plus, Quest, Alt, Backref };

struct Element;
using Sequence = std::vector<Element>;

struct Element {
    explicit Element(Op value, int argument = 0) : op(value), arg(argument) {}
    Op op;
    int arg;
    int entry = -1;
    int exit = -1;
    Sequence body;
    std::vector<Sequence> branches;
};

enum class Kind : std::uint8_t { Char, Set, Bol, Eol, Bow, Eow, Open, Close, Backref, Split, PlusEnter, PlusLoop, Accept };

struct Node {
    Kind kind;
    int arg = 0;
    int out = -1;
    int loop = -1;
    std::vector<int> outs;
};

using CharacterSet = std::array<bool, 256>;

struct Program {
    Sequence root;
    std::vector<CharacterSet> sets;
    std::size_t groups = 0;
    int flags = 0;
    bool backrefs = false;
    std::size_t lineStarts = 0;
    std::size_t lineEnds = 0;
    std::vector<Node> nodes;
    std::vector<std::vector<int>> predecessors;
    std::vector<int> anchors;
    int start = -1;
    int accept = -1;
    int slots = 0;
};

struct Guts {
    int magic = MagicGuts;
    Program program;
};

struct Failure {
    int code;
};

std::size_t CountElements(const Sequence& sequence) {
    std::size_t total = 0;
    for (const auto& element : sequence) {
        total += 1 + CountElements(element.body);
        for (const auto& branch : element.branches) total += CountElements(branch);
    }
    return total;
}

class Parser {
public:
    Parser(const unsigned char* begin, const unsigned char* end, int flags, Program& program)
        : next_(begin), end_(end), flags_(flags), bre_((flags & FlagExtended) == 0), program_(program) {
        program_.flags = flags;
    }

    void Run() {
        if (flags_ & FlagNoSpec) {
            Require(More(), ErrEmpty);
            Sequence items;
            while (More()) Ordinary(items, *next_++);
            program_.root = std::move(items);
        } else {
            program_.root = ParseRe(Outside, Outside);
        }
    }

    std::size_t Groups() const { return program_.groups; }

private:
    const unsigned char* next_;
    const unsigned char* end_;
    int flags_;
    bool bre_;
    Program& program_;
    std::size_t expanded_ = 0;
    std::array<bool, 10> closed_{};
    int nesting_ = 0;
    int anyIndex_ = -1;
    int newlineFreeIndex_ = -1;

    [[noreturn]] static void Fail(int code) { throw Failure{code}; }
    static void Require(bool condition, int code) {
        if (!condition) Fail(code);
    }
    bool More() const { return next_ < end_; }
    bool More2() const { return next_ + 1 < end_; }
    int Peek() const { return More() ? *next_ : 0; }
    int Peek2() const { return More2() ? next_[1] : 0; }
    bool See(int c) const { return More() && Peek() == c; }
    bool SeeTwo(int a, int b) const { return More() && More2() && Peek() == a && Peek2() == b; }
    bool SeeSpec(int c) const { return bre_ ? SeeTwo('\\', c) : See(c); }
    bool Eat(int c) {
        if (!See(c)) return false;
        ++next_;
        return true;
    }
    bool EatTwo(int a, int b) {
        if (!SeeTwo(a, b)) return false;
        next_ += 2;
        return true;
    }
    bool SeeEnd(int end1, int end2) const { return bre_ ? SeeTwo(end1, end2) : See(end1); }

    int AddSet(const CharacterSet& set) {
        program_.sets.push_back(set);
        return static_cast<int>(program_.sets.size() - 1);
    }

    void EmitCharacter(Sequence& items, int c) {
        Element element{Op::Char};
        element.arg = c;
        items.push_back(std::move(element));
    }

    void EmitSet(Sequence& items, int index) {
        Element element{Op::Set};
        element.arg = index;
        items.push_back(std::move(element));
    }

    void EmitSimple(Sequence& items, Op op, int arg = 0) {
        Element element{op};
        element.arg = arg;
        items.push_back(std::move(element));
    }

    void Ordinary(Sequence& items, int c) {
        if ((flags_ & FlagIgnoreCase) && IsAlpha(c)) {
            CharacterSet set{};
            set[static_cast<std::size_t>(ToLower(c))] = true;
            set[static_cast<std::size_t>(ToUpper(c))] = true;
            EmitSet(items, AddSet(set));
        } else {
            EmitCharacter(items, c);
        }
    }

    void EmitAny(Sequence& items) {
        if (flags_ & FlagNewline) {
            if (newlineFreeIndex_ < 0) {
                CharacterSet set;
                set.fill(true);
                set['\n'] = false;
                newlineFreeIndex_ = AddSet(set);
            }
            EmitSet(items, newlineFreeIndex_);
        } else {
            if (anyIndex_ < 0) {
                CharacterSet set;
                set.fill(true);
                anyIndex_ = AddSet(set);
            }
            EmitSet(items, anyIndex_);
        }
    }

    int ParseCount() {
        int count = 0;
        int digits = 0;
        while (More() && IsDigit(Peek()) && count <= DupMax) {
            count = count * 10 + (*next_++ - '0');
            ++digits;
        }
        Require(digits > 0 && count <= DupMax, ErrBadBrace);
        return count;
    }

    static Sequence Wrap(Op op, Sequence body) {
        Element element{op};
        element.body = std::move(body);
        Sequence result;
        result.push_back(std::move(element));
        return result;
    }

    static Sequence Optional(Sequence body) {
        Element element{Op::Alt};
        element.branches.push_back(std::move(body));
        element.branches.emplace_back();
        Sequence result;
        result.push_back(std::move(element));
        return result;
    }

    static void Append(Sequence& target, Sequence&& source) {
        for (auto& element : source) target.push_back(std::move(element));
    }

    static int MapCount(int n) { return n <= 1 ? n : n == Infinite ? 3 : 2; }

    Sequence Repeat(Sequence operand, int from, int to) {
        const std::size_t size = CountElements(operand);
        const std::size_t copies = static_cast<std::size_t>(to == Infinite ? from + 2 : to + 1);
        Require(size <= ElementLimit / copies, ErrSpace);
        const int shape = MapCount(from) * 8 + MapCount(to);
        switch (shape) {
        case 0 * 8 + 0:
            return {};
        case 0 * 8 + 1:
        case 0 * 8 + 2:
        case 0 * 8 + 3:
            return Optional(Repeat(std::move(operand), 1, to));
        case 1 * 8 + 1:
            return operand;
        case 1 * 8 + 2: {
            Sequence copy = operand;
            Sequence result = Optional(std::move(operand));
            Append(result, Repeat(std::move(copy), 1, to - 1));
            return result;
        }
        case 1 * 8 + 3:
            return Wrap(Op::Plus, std::move(operand));
        case 2 * 8 + 2: {
            Sequence copy = operand;
            Sequence result = std::move(operand);
            Append(result, Repeat(std::move(copy), from - 1, to - 1));
            return result;
        }
        default: {
            Sequence copy = operand;
            Sequence result = std::move(operand);
            Append(result, Repeat(std::move(copy), from - 1, to));
            return result;
        }
        }
    }

    void ApplyRepeat(Sequence& items, std::size_t position, int from, int to) {
        Sequence operand(std::make_move_iterator(items.begin() + static_cast<std::ptrdiff_t>(position)),
                         std::make_move_iterator(items.end()));
        items.erase(items.begin() + static_cast<std::ptrdiff_t>(position), items.end());
        const std::size_t before = CountElements(operand);
        Sequence repeated = Repeat(std::move(operand), from, to);
        const std::size_t after = CountElements(repeated);
        if (after > before) {
            Require(after - before <= ElementLimit - expanded_, ErrSpace);
            expanded_ += after - before;
        }
        Append(items, std::move(repeated));
    }

    void ApplyStar(Sequence& items, std::size_t position) {
        Sequence operand(std::make_move_iterator(items.begin() + static_cast<std::ptrdiff_t>(position)),
                         std::make_move_iterator(items.end()));
        items.erase(items.begin() + static_cast<std::ptrdiff_t>(position), items.end());
        Append(items, Wrap(Op::Quest, Wrap(Op::Plus, std::move(operand))));
    }

    void ParseBracketTerm(CharacterSet& bitmap, bool ignoreCase);
    void ParseBracket(Sequence& items);
    int ParseBracketSymbol();
    int ParseCollatingElement(int endCharacter);
    void AddCharacter(CharacterSet& bitmap, bool ignoreCase, int c) {
        bitmap[static_cast<std::size_t>(c)] = true;
        if (ignoreCase) {
            bitmap[static_cast<std::size_t>(ToLower(c))] = true;
            bitmap[static_cast<std::size_t>(ToUpper(c))] = true;
        }
    }

    bool ParseEreExpression(Sequence& items);
    bool ParseBreExpression(Sequence& items, int chain);
    Sequence ParseRe(int end1, int end2);
};

int Parser::ParseCollatingElement(int endCharacter) {
    const unsigned char* start = next_;
    while (More() && !SeeTwo(endCharacter, ']')) ++next_;
    Require(More(), ErrBracket);
    const std::string_view name(reinterpret_cast<const char*>(start), static_cast<std::size_t>(next_ - start));
    for (const auto& entry : CollatingNames) {
        if (name == entry.name) return entry.code;
    }
    Require(!name.empty(), ErrIllegalSequence);
    Require(name.size() == 1 && name[0] != '\0', ErrCollate);
    return static_cast<unsigned char>(name[0]);
}

int Parser::ParseBracketSymbol() {
    Require(More(), ErrBracket);
    if (!EatTwo('[', '.')) return *next_++;
    const int value = ParseCollatingElement('.');
    Require(EatTwo('.', ']'), ErrCollate);
    return value;
}

void Parser::ParseBracketTerm(CharacterSet& bitmap, bool ignoreCase) {
    int c = 0;
    switch (More() ? Peek() : 0) {
    case '[':
        c = More2() ? Peek2() : 0;
        break;
    case '-':
        Fail(ErrRange);
    default:
        break;
    }
    if (c == ':') {
        next_ += 2;
        Require(More(), ErrBracket);
        Require(Peek() != '-' && Peek() != ']', ErrCType);
        const unsigned char* start = next_;
        while (More() && IsAlpha(Peek())) ++next_;
        const std::size_t length = static_cast<std::size_t>(next_ - start);
        Require(length < 15, ErrCType);
        const std::string_view name(reinterpret_cast<const char*>(start), length);
        Require(IsClassName(name), ErrCType);
        for (int i = 0; i < 256; ++i) {
            if (InClass(name, i)) AddCharacter(bitmap, ignoreCase, i);
        }
        Require(More(), ErrBracket);
        Require(EatTwo(':', ']'), ErrCType);
    } else if (c == '=') {
        next_ += 2;
        Require(More(), ErrBracket);
        Require(Peek() != '-' && Peek() != ']', ErrCollate);
        const int value = ParseCollatingElement('=');
        AddCharacter(bitmap, ignoreCase, value);
        Require(More(), ErrBracket);
        Require(EatTwo('=', ']'), ErrCollate);
    } else {
        const int start = ParseBracketSymbol();
        int finish = start;
        if (See('-') && More2() && Peek2() != ']') {
            ++next_;
            finish = Eat('-') ? '-' : ParseBracketSymbol();
        }
        if (start == finish) {
            AddCharacter(bitmap, ignoreCase, start);
        } else {
            Require(start <= finish, ErrRange);
            for (int i = start; i <= finish; ++i) AddCharacter(bitmap, ignoreCase, i);
        }
    }
}

void Parser::ParseBracket(Sequence& items) {
    if (next_ + 5 < end_ && std::memcmp(next_, "[:<:]]", 6) == 0) {
        EmitSimple(items, Op::Bow);
        next_ += 6;
        return;
    }
    if (next_ + 5 < end_ && std::memcmp(next_, "[:>:]]", 6) == 0) {
        EmitSimple(items, Op::Eow);
        next_ += 6;
        return;
    }
    CharacterSet bitmap{};
    const bool ignoreCase = (flags_ & FlagIgnoreCase) != 0;
    const bool invert = Eat('^');
    if (Eat(']')) AddCharacter(bitmap, ignoreCase, ']');
    else if (Eat('-')) AddCharacter(bitmap, ignoreCase, '-');
    while (More() && Peek() != ']' && !SeeTwo('-', ']')) ParseBracketTerm(bitmap, ignoreCase);
    if (Eat('-')) AddCharacter(bitmap, ignoreCase, '-');
    Require(More() && *next_++ == ']', ErrBracket);
    if (invert && (flags_ & FlagNewline)) bitmap['\n'] = true;
    CharacterSet result;
    int members = 0;
    int member = 0;
    for (int i = 0; i < 256; ++i) {
        result[static_cast<std::size_t>(i)] = bitmap[static_cast<std::size_t>(i)] != invert;
        if (result[static_cast<std::size_t>(i)]) {
            ++members;
            member = i;
        }
    }
    if (members == 1) EmitCharacter(items, member);
    else EmitSet(items, AddSet(result));
}

bool Parser::ParseEreExpression(Sequence& items) {
    const std::size_t position = items.size();
    bool wasCaret = false;
    const int c = *next_++;
    switch (c) {
    case '(': {
        const int number = static_cast<int>(++program_.groups);
        Require(More(), ErrParen);
        EmitSimple(items, Op::Open, number);
        if (!See(')')) Append(items, ParseRe(')', Outside));
        if (number < 10) closed_[static_cast<std::size_t>(number)] = true;
        EmitSimple(items, Op::Close, number);
        Require(More() && *next_++ == ')', ErrParen);
        break;
    }
    case ')':
        Fail(ErrParen);
    case '^':
        EmitSimple(items, Op::Bol);
        ++program_.lineStarts;
        wasCaret = true;
        break;
    case '$':
        EmitSimple(items, Op::Eol);
        ++program_.lineEnds;
        break;
    case '|':
        Fail(ErrEmpty);
    case '*':
    case '+':
    case '?':
        Fail(ErrBadRepeat);
    case '.':
        EmitAny(items);
        break;
    case '[':
        ParseBracket(items);
        break;
    case '\\': {
        Require(More(), ErrEscape);
        const int escaped = *next_++;
        if (escaped == '<') EmitSimple(items, Op::Bow);
        else if (escaped == '>') EmitSimple(items, Op::Eow);
        else Ordinary(items, escaped);
        break;
    }
    case '{':
        Require(!More() || !IsDigit(Peek()), ErrBadRepeat);
        Ordinary(items, c);
        break;
    default:
        Ordinary(items, c);
        break;
    }

    if (!More()) return false;
    int op = Peek();
    if (!(op == '*' || op == '+' || op == '?' || (op == '{' && More2() && IsDigit(Peek2())))) return false;
    ++next_;
    Require(!wasCaret, ErrBadRepeat);
    switch (op) {
    case '*':
        ApplyStar(items, position);
        break;
    case '+':
        ApplyRepeat(items, position, 1, Infinite);
        break;
    case '?':
        ApplyRepeat(items, position, 0, 1);
        break;
    default: {
        const int count = ParseCount();
        int count2 = count;
        if (Eat(',')) {
            if (IsDigit(Peek())) {
                count2 = ParseCount();
                Require(count <= count2, ErrBadBrace);
            } else {
                count2 = Infinite;
            }
        }
        ApplyRepeat(items, position, count, count2);
        if (!Eat('}')) {
            while (More() && Peek() != '}') ++next_;
            Require(More(), ErrBrace);
            Fail(ErrBadBrace);
        }
        break;
    }
    }

    if (!More()) return false;
    op = Peek();
    if (!(op == '*' || op == '+' || op == '?' || (op == '{' && More2() && IsDigit(Peek2())))) return false;
    Fail(ErrBadRepeat);
}

bool Parser::ParseBreExpression(Sequence& items, int chain) {
    const std::size_t position = items.size();
    int c = *next_++;
    bool escaped = false;
    if (c == '\\') {
        Require(More(), ErrEscape);
        c = *next_++;
        escaped = true;
    }
    bool dollar = false;
    if (!escaped) {
        switch (c) {
        case '.':
            EmitAny(items);
            break;
        case '[':
            ParseBracket(items);
            break;
        case '*':
            Require(chain == 0, ErrBadRepeat);
            Ordinary(items, c);
            break;
        case '$':
            dollar = true;
            Ordinary(items, c);
            break;
        default:
            Ordinary(items, c);
            break;
        }
    } else {
        switch (c) {
        case '<':
            EmitSimple(items, Op::Bow);
            break;
        case '>':
            EmitSimple(items, Op::Eow);
            break;
        case '{':
            Fail(ErrBadRepeat);
        case '(': {
            const int number = static_cast<int>(++program_.groups);
            EmitSimple(items, Op::Open, number);
            if (More() && !SeeTwo('\\', ')')) Append(items, ParseRe('\\', ')'));
            if (number < 10) closed_[static_cast<std::size_t>(number)] = true;
            EmitSimple(items, Op::Close, number);
            Require(EatTwo('\\', ')'), ErrParen);
            break;
        }
        case ')':
        case '}':
            Fail(ErrParen);
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            const int number = c - '0';
            Require(closed_[static_cast<std::size_t>(number)], ErrSubReg);
            EmitSimple(items, Op::Backref, number);
            program_.backrefs = true;
            break;
        }
        default:
            Ordinary(items, c);
            break;
        }
    }

    if (Eat('*')) {
        ApplyStar(items, position);
    } else if (EatTwo('\\', '{')) {
        const int count = ParseCount();
        int count2 = count;
        if (Eat(',')) {
            if (More() && IsDigit(Peek())) {
                count2 = ParseCount();
                Require(count <= count2, ErrBadBrace);
            } else {
                count2 = Infinite;
            }
        }
        ApplyRepeat(items, position, count, count2);
        if (!EatTwo('\\', '}')) {
            while (More() && !SeeTwo('\\', '}')) ++next_;
            Require(More(), ErrBrace);
            Fail(ErrBadBrace);
        }
    } else if (dollar) {
        return true;
    }
    return false;
}

Sequence Parser::ParseRe(int end1, int end2) {
    struct NestingGuard {
        int& depth;
        explicit NestingGuard(int& value) : depth(value) {
            Require(++depth <= MaxNesting, ErrSpace);
        }
        ~NestingGuard() { --depth; }
    } guard(nesting_);
    const bool outer = end1 == Outside && end2 == Outside;
    std::vector<Sequence> branches;
    for (;;) {
        Sequence items;
        int chain = 0;
        bool terminate = false;
        if (bre_ && Eat('^')) {
            EmitSimple(items, Op::Bol);
            ++program_.lineStarts;
        }
        while (More() && (bre_ || !SeeSpec('|')) && !SeeEnd(end1, end2)) {
            terminate = bre_ ? ParseBreExpression(items, chain) : ParseEreExpression(items);
            ++chain;
        }
        if (bre_ && terminate) {
            items.back() = Element{Op::Eol};
            ++program_.lineEnds;
        }
        Require(!items.empty(), ErrEmpty);
        branches.push_back(std::move(items));
        if (bre_) break;
        int ate = 0;
        while (Eat('|')) ++ate;
        if (ate == 0) break;
        Require(!(ate > 1 || (outer && !More())), ErrEmpty);
    }
    if (branches.size() == 1) return std::move(branches.front());
    Element alternation{Op::Alt};
    alternation.branches = std::move(branches);
    Sequence result;
    result.push_back(std::move(alternation));
    return result;
}

class GraphBuilder {
public:
    explicit GraphBuilder(Program& program) : program_(program) {}

    void Run() {
        program_.accept = Add(Kind::Accept, 0, -1);
        program_.start = BuildSequence(program_.root, program_.accept);
        program_.predecessors.assign(program_.nodes.size(), {});
        for (std::size_t i = 0; i < program_.nodes.size(); ++i) {
            const Node& node = program_.nodes[i];
            const int index = static_cast<int>(i);
            if (node.out >= 0) program_.predecessors[static_cast<std::size_t>(node.out)].push_back(index);
            if (node.loop >= 0) program_.predecessors[static_cast<std::size_t>(node.loop)].push_back(index);
            for (const int target : node.outs) program_.predecessors[static_cast<std::size_t>(target)].push_back(index);
            if (node.kind == Kind::Bol || node.kind == Kind::Eol || node.kind == Kind::Bow || node.kind == Kind::Eow) {
                program_.anchors.push_back(index);
            }
        }
    }

private:
    Program& program_;

    int Add(Kind kind, int arg, int out) {
        program_.nodes.push_back(Node{kind, arg, out, -1, {}});
        return static_cast<int>(program_.nodes.size() - 1);
    }

    int AddSplit(std::vector<int> outs) {
        const int index = Add(Kind::Split, 0, -1);
        program_.nodes[static_cast<std::size_t>(index)].outs = std::move(outs);
        return index;
    }

    int BuildSequence(Sequence& sequence, int next) {
        for (std::size_t i = sequence.size(); i-- > 0;) next = BuildElement(sequence[i], next);
        return next;
    }

    int BuildElement(Element& element, int next) {
        element.exit = next;
        int index = -1;
        switch (element.op) {
        case Op::Char: index = Add(Kind::Char, element.arg, next); break;
        case Op::Set: index = Add(Kind::Set, element.arg, next); break;
        case Op::Bol: index = Add(Kind::Bol, 0, next); break;
        case Op::Eol: index = Add(Kind::Eol, 0, next); break;
        case Op::Bow: index = Add(Kind::Bow, 0, next); break;
        case Op::Eow: index = Add(Kind::Eow, 0, next); break;
        case Op::Open: index = Add(Kind::Open, element.arg, next); break;
        case Op::Close: index = Add(Kind::Close, element.arg, next); break;
        case Op::Backref: index = Add(Kind::Backref, element.arg, next); break;
        case Op::Quest: {
            const int body = BuildSequence(element.body, next);
            index = AddSplit({body, next});
            break;
        }
        case Op::Plus: {
            const int slot = program_.slots++;
            const int loop = Add(Kind::PlusLoop, slot, next);
            const int body = BuildSequence(element.body, loop);
            index = Add(Kind::PlusEnter, slot, body);
            program_.nodes[static_cast<std::size_t>(loop)].loop = index;
            break;
        }
        case Op::Alt: {
            std::vector<int> outs;
            for (auto& branch : element.branches) outs.push_back(BuildSequence(branch, next));
            index = AddSplit(std::move(outs));
            break;
        }
        }
        element.entry = index;
        return index;
    }
};

class Matcher {
public:
    Matcher(const Program& program, const unsigned char* text, Position begin, Position end, int flags)
        : program_(program), nodes_(program.nodes), text_(text), begin_(begin), end_(end), flags_(flags),
          backrefs_(program.backrefs), visited_(program.nodes.size(), 0), rank_(program.nodes.size(), 0),
          consumed_(program.nodes.size(), 0), marked_(program.nodes.size(), 0), layer_(program.nodes.size(), 0) {}

    bool Search(std::size_t wanted, GuestMatch* out) {
        if (backrefs_) return SearchWithBackrefs(wanted, out);
        Position start = NoPosition;
        Position last = NoPosition;
        if (!Leftmost(start, last)) return false;
        Fill(start, last, wanted, out);
        if (wanted > 1) {
            groups_.assign(program_.groups + 1, GuestMatch{-1, -1});
            Dissect(program_.root, 0, start, last);
            CopyGroups(wanted, out);
        }
        return true;
    }

private:
    struct Thread {
        int node;
        Position start;
    };

    struct Pending {
        int node;
        std::size_t rank;
    };

    struct Frame {
        int node;
        Position position;
        int reg;
        Position old;
    };

    struct Layer {
        std::vector<std::uint32_t> stamp;
        std::vector<Position> value;
        std::vector<int> marked;
        std::uint32_t here = 0;
    };

    const Program& program_;
    const std::vector<Node>& nodes_;
    const unsigned char* text_;
    Position begin_;
    Position end_;
    int flags_;
    bool backrefs_;
    std::vector<std::uint32_t> visited_;
    std::vector<std::size_t> rank_;
    std::vector<std::uint32_t> consumed_;
    std::vector<std::uint32_t> marked_;
    std::vector<std::size_t> layer_;
    std::uint32_t generation_ = 0;
    std::vector<Pending> pending_;
    std::vector<int> work_;
    std::vector<GuestMatch> groups_;
    std::vector<Position> slots_;
    std::vector<Frame> frames_;
    std::vector<std::pair<int, Position>> seeds_;
    Layer top_;
    Layer upper_;
    Layer lower_;

    static void Fill(Position start, Position end, std::size_t wanted, GuestMatch* out) {
        if (wanted == 0) return;
        out[0].start = start;
        out[0].end = end;
    }

    void CopyGroups(std::size_t wanted, GuestMatch* out) const {
        for (std::size_t i = 1; i < wanted; ++i) {
            if (i < groups_.size()) out[i] = groups_[i];
            else out[i] = GuestMatch{-1, -1};
        }
    }

    std::uint32_t NextGeneration() {
        if (++generation_ == 0) {
            std::fill(visited_.begin(), visited_.end(), 0);
            std::fill(consumed_.begin(), consumed_.end(), 0);
            std::fill(marked_.begin(), marked_.end(), 0);
            generation_ = 1;
        }
        return generation_;
    }

    bool PreviousIsReal(Position p) const {
        return !((p == 0 && begin_ >= 0) || (p == begin_ && !(flags_ & FlagNotBol)));
    }
    bool AtEol(Position p) const {
        return (p == end_ && !(flags_ & FlagNotEol)) ||
               (p < end_ && text_[p] == '\n' && (program_.flags & FlagNewline));
    }
    bool AtBol(Position p) const {
        if (backrefs_) {
            return (p == begin_ && !(flags_ & FlagNotBol)) ||
                   (p > 0 && p < end_ && text_[p - 1] == '\n' && (program_.flags & FlagNewline));
        }
        if (!PreviousIsReal(p)) return !(flags_ & FlagNotBol);
        return text_[p - 1] == '\n' && (program_.flags & FlagNewline);
    }
    bool AtBow(Position p) const {
        if (p >= end_ || !IsWord(text_[p])) return false;
        if (backrefs_) return (p == begin_ && !(flags_ & FlagNotBol)) || (p > 0 && !IsWord(text_[p - 1]));
        return AtBol(p) || (PreviousIsReal(p) && !IsWord(text_[p - 1]));
    }
    bool AtEow(Position p) const {
        if (backrefs_) return (AtEol(p) || (p < end_ && !IsWord(text_[p]))) && p > begin_ && IsWord(text_[p - 1]);
        return PreviousIsReal(p) && IsWord(text_[p - 1]) && (AtEol(p) || (p < end_ && !IsWord(text_[p])));
    }

    bool Condition(Kind kind, Position p) const {
        switch (kind) {
        case Kind::Bol: return AtBol(p);
        case Kind::Eol: return AtEol(p);
        case Kind::Bow: return AtBow(p);
        default: return AtEow(p);
        }
    }

    bool Consumes(const Node& node, Position p) const {
        if (node.kind == Kind::Char) return text_[p] == node.arg;
        return node.kind == Kind::Set && program_.sets[static_cast<std::size_t>(node.arg)][text_[p]];
    }

    std::size_t Passes(Position p) const {
        return (AtBol(p) ? program_.lineStarts : 0) + (AtEol(p) ? program_.lineEnds : 0);
    }

    void Consume(int target, Position start, std::uint32_t here, std::vector<Thread>& next) {
        const auto index = static_cast<std::size_t>(target);
        if (consumed_[index] == here) return;
        consumed_[index] = here;
        next.push_back({target, start});
    }

    template <typename Accepted>
    void Step(const std::vector<Thread>& current, Position p, Position limit, int accept, std::vector<Thread>& next,
              Accepted&& accepted) {
        const std::uint32_t here = NextGeneration();
        const std::size_t passes = backrefs_ ? 0 : Passes(p);
        bool reached = false;
        for (const Thread& thread : current) {
            pending_.clear();
            pending_.push_back({thread.node, 0});
            while (!pending_.empty()) {
                const Pending item = pending_.back();
                pending_.pop_back();
                const auto index = static_cast<std::size_t>(item.node);
                if (visited_[index] == here && rank_[index] <= item.rank) continue;
                visited_[index] = here;
                rank_[index] = item.rank;
                if (item.node == accept) {
                    if (!reached) {
                        reached = true;
                        accepted(thread.start);
                    }
                    continue;
                }
                const Node& node = nodes_[index];
                switch (node.kind) {
                case Kind::Char:
                case Kind::Set:
                    if (p < limit && Consumes(node, p)) Consume(node.out, thread.start, here, next);
                    break;
                case Kind::Backref:
                    pending_.push_back({node.out, item.rank});
                    if (p < limit) Consume(item.node, thread.start, here, next);
                    break;
                case Kind::Bol:
                case Kind::Eol:
                    if (!Condition(node.kind, p)) break;
                    if (backrefs_) pending_.push_back({node.out, item.rank});
                    else if (item.rank != WordUsed && item.rank < passes) pending_.push_back({node.out, item.rank + 1});
                    break;
                case Kind::Bow:
                case Kind::Eow:
                    if (!Condition(node.kind, p)) break;
                    if (backrefs_) pending_.push_back({node.out, item.rank});
                    else if (item.rank != WordUsed) pending_.push_back({node.out, WordUsed});
                    break;
                case Kind::Split:
                    for (const int target : node.outs) pending_.push_back({target, item.rank});
                    break;
                case Kind::PlusLoop:
                    pending_.push_back({node.loop, item.rank});
                    pending_.push_back({node.out, item.rank});
                    break;
                case Kind::Accept:
                    break;
                case Kind::Open:
                case Kind::Close:
                case Kind::PlusEnter:
                    pending_.push_back({node.out, item.rank});
                    break;
                }
            }
        }
    }

    bool Leftmost(Position& start, Position& last) {
        std::vector<Thread> current;
        std::vector<Thread> next;
        Position best = NoPosition;
        Position bestEnd = NoPosition;
        for (Position p = begin_;; ++p) {
            if (best == NoPosition) current.push_back({program_.start, p});
            next.clear();
            Position label = NoPosition;
            Step(current, p, end_, program_.accept, next, [&](Position found) { label = found; });
            if (label != NoPosition && (best == NoPosition || label <= best)) {
                best = label;
                bestEnd = p;
            }
            if (p >= end_) break;
            if (best != NoPosition) {
                std::erase_if(next, [&](const Thread& thread) { return thread.start > best; });
                if (next.empty()) break;
            }
            current.swap(next);
        }
        if (best == NoPosition) return false;
        start = best;
        last = bestEnd;
        return true;
    }

    template <typename Visit>
    void Walk(int entry, int accept, Position start, Position limit, Visit&& visit) {
        std::vector<Thread> current{{entry, start}};
        std::vector<Thread> next;
        for (Position p = start;; ++p) {
            next.clear();
            Step(current, p, limit, accept, next, [&](Position) { visit(p); });
            if (p >= limit || next.empty()) break;
            current.swap(next);
        }
    }

    Position Longest(const Sequence& sequence, Position start, Position limit) {
        if (sequence.empty()) return start;
        Position result = NoPosition;
        Walk(sequence.front().entry, sequence.back().exit, start, limit, [&](Position p) { result = p; });
        return result;
    }

    void Spread(int accept, Position p, std::size_t layer, std::uint32_t here, std::vector<int>& marked) {
        while (!work_.empty()) {
            const int node = work_.back();
            work_.pop_back();
            for (const int predecessor : program_.predecessors[static_cast<std::size_t>(node)]) {
                if (predecessor == accept) continue;
                const Node& source = nodes_[static_cast<std::size_t>(predecessor)];
                switch (source.kind) {
                case Kind::Char:
                case Kind::Set:
                case Kind::Accept:
                    break;
                case Kind::Bol:
                case Kind::Eol:
                case Kind::Bow:
                case Kind::Eow:
                    if (backrefs_ && Condition(source.kind, p)) Mark(predecessor, layer, here, marked);
                    break;
                case Kind::Open:
                case Kind::Close:
                case Kind::Backref:
                case Kind::Split:
                case Kind::PlusEnter:
                case Kind::PlusLoop:
                    Mark(predecessor, layer, here, marked);
                    break;
                }
            }
        }
    }

    void Mark(int node, std::size_t layer, std::uint32_t here, std::vector<int>& marked) {
        const auto index = static_cast<std::size_t>(node);
        if (marked_[index] == here) return;
        marked_[index] = here;
        layer_[index] = layer;
        marked.push_back(node);
        work_.push_back(node);
    }

    std::vector<char> Reaches(int entry, int accept, Position from, Position to, bool exact) {
        std::vector<char> result(static_cast<std::size_t>(to - from + 1), 0);
        std::vector<int> good;
        std::vector<int> marked;
        for (Position p = to;; --p) {
            const std::uint32_t here = NextGeneration();
            marked.clear();
            work_.clear();
            if (!exact || p == to) Mark(accept, WordUsed, here, marked);
            if (p < to) {
                for (const int node : good) {
                    if (node != accept && nodes_[static_cast<std::size_t>(node)].kind == Kind::Backref) {
                        Mark(node, WordUsed, here, marked);
                    }
                    for (const int predecessor : program_.predecessors[static_cast<std::size_t>(node)]) {
                        if (predecessor != accept && Consumes(nodes_[static_cast<std::size_t>(predecessor)], p)) {
                            Mark(predecessor, WordUsed, here, marked);
                        }
                    }
                }
            }
            Spread(accept, p, WordUsed, here, marked);
            if (!backrefs_) {
                const std::size_t passes = Passes(p);
                for (std::size_t layer = passes + 1; layer-- > 0;) {
                    for (const int anchor : program_.anchors) {
                        if (anchor == accept) continue;
                        const Node& node = nodes_[static_cast<std::size_t>(anchor)];
                        const auto target = static_cast<std::size_t>(node.out);
                        if (marked_[target] != here || !Condition(node.kind, p)) continue;
                        if (node.kind == Kind::Bow || node.kind == Kind::Eow) {
                            if (layer_[target] == WordUsed) Mark(anchor, layer, here, marked);
                        } else if (layer < passes && layer_[target] >= layer + 1) {
                            Mark(anchor, layer, here, marked);
                        }
                    }
                    Spread(accept, p, layer, here, marked);
                }
            }
            if (marked_[static_cast<std::size_t>(entry)] == here) result[static_cast<std::size_t>(p - from)] = 1;
            good.swap(marked);
            if (p == from) break;
        }
        return result;
    }

    bool Epsilon(const Node& node, Position p) const {
        switch (node.kind) {
        case Kind::Open:
        case Kind::Close:
        case Kind::Backref:
        case Kind::Split:
        case Kind::PlusEnter:
        case Kind::PlusLoop:
            return true;
        case Kind::Bol:
        case Kind::Eol:
        case Kind::Bow:
        case Kind::Eow:
            return backrefs_ && Condition(node.kind, p);
        default:
            return false;
        }
    }

    void FillLayer(Layer& layer, int accept, Position p) {
        std::stable_sort(seeds_.begin(), seeds_.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        layer.here = NextGeneration();
        layer.marked.clear();
        for (const auto& [seed, value] : seeds_) {
            if (layer.stamp[static_cast<std::size_t>(seed)] == layer.here) continue;
            layer.stamp[static_cast<std::size_t>(seed)] = layer.here;
            layer.value[static_cast<std::size_t>(seed)] = value;
            layer.marked.push_back(seed);
            work_.clear();
            work_.push_back(seed);
            while (!work_.empty()) {
                const int node = work_.back();
                work_.pop_back();
                for (const int predecessor : program_.predecessors[static_cast<std::size_t>(node)]) {
                    const auto index = static_cast<std::size_t>(predecessor);
                    if (predecessor == accept || layer.stamp[index] == layer.here || !Epsilon(nodes_[index], p)) continue;
                    layer.stamp[index] = layer.here;
                    layer.value[index] = value;
                    layer.marked.push_back(predecessor);
                    work_.push_back(predecessor);
                }
            }
        }
    }

    std::vector<Position> LongestFrom(int entry, int accept, Position from, Position to) {
        for (Layer* layer : {&top_, &upper_, &lower_}) {
            layer->stamp.assign(nodes_.size(), 0);
            layer->value.assign(nodes_.size(), NoPosition);
        }
        std::vector<Position> result(static_cast<std::size_t>(to - from + 1), NoPosition);
        std::vector<std::pair<int, Position>> base;
        std::vector<std::pair<int, Position>> previous;
        for (Position p = to;; --p) {
            base.clear();
            base.push_back({accept, p});
            if (p < to) {
                for (const auto& [node, value] : previous) {
                    if (node != accept && nodes_[static_cast<std::size_t>(node)].kind == Kind::Backref) base.push_back({node, value});
                    for (const int predecessor : program_.predecessors[static_cast<std::size_t>(node)]) {
                        if (predecessor != accept && Consumes(nodes_[static_cast<std::size_t>(predecessor)], p)) {
                            base.push_back({predecessor, value});
                        }
                    }
                }
            }
            seeds_ = base;
            FillLayer(top_, accept, p);
            Layer* final = &top_;
            if (!backrefs_) {
                const std::size_t passes = Passes(p);
                for (std::size_t layer = passes;; --layer) {
                    seeds_ = base;
                    for (const int anchor : program_.anchors) {
                        if (anchor == accept) continue;
                        const Node& node = nodes_[static_cast<std::size_t>(anchor)];
                        const auto target = static_cast<std::size_t>(node.out);
                        if (!Condition(node.kind, p)) continue;
                        if (node.kind == Kind::Bow || node.kind == Kind::Eow) {
                            if (top_.stamp[target] == top_.here) seeds_.push_back({anchor, top_.value[target]});
                        } else if (layer < passes && upper_.stamp[target] == upper_.here) {
                            seeds_.push_back({anchor, upper_.value[target]});
                        }
                    }
                    FillLayer(lower_, accept, p);
                    std::swap(upper_, lower_);
                    if (layer == 0) break;
                }
                final = &upper_;
            }
            previous.clear();
            for (const int node : final->marked) previous.push_back({node, final->value[static_cast<std::size_t>(node)]});
            if (final->stamp[static_cast<std::size_t>(entry)] == final->here) {
                result[static_cast<std::size_t>(p - from)] = final->value[static_cast<std::size_t>(entry)];
            }
            if (p == from) break;
        }
        return result;
    }

    Position Rest(const Sequence& sequence, std::size_t index, Position start, Position stop) {
        const Element& element = sequence[index];
        std::vector<Position> ends;
        Walk(element.entry, element.exit, start, stop, [&](Position p) { ends.push_back(p); });
        if (ends.empty()) return start;
        const bool last = index + 1 == sequence.size();
        std::vector<char> tail;
        if (!last) tail = Reaches(sequence[index + 1].entry, sequence.back().exit, start, stop, true);
        for (auto it = ends.rbegin(); it != ends.rend(); ++it) {
            const Position rest = *it;
            if (rest == start || (last ? rest == stop : tail[static_cast<std::size_t>(rest - start)] != 0)) return rest;
        }
        return start;
    }

    void Dissect(const Sequence& sequence, std::size_t index, Position start, Position stop) {
        Position position = start;
        for (; index < sequence.size(); ++index) {
            const Element& element = sequence[index];
            switch (element.op) {
            case Op::Char:
            case Op::Set:
                ++position;
                break;
            case Op::Open:
                groups_[static_cast<std::size_t>(element.arg)].start = position;
                break;
            case Op::Close:
                groups_[static_cast<std::size_t>(element.arg)].end = position;
                break;
            case Op::Quest: {
                const Position rest = Rest(sequence, index, position, stop);
                if (Longest(element.body, position, rest) != NoPosition) Dissect(element.body, 0, position, rest);
                position = rest;
                break;
            }
            case Op::Plus: {
                const Position rest = Rest(sequence, index, position, stop);
                Position innerStart = position;
                Position previous = innerStart;
                Position innerEnd = NoPosition;
                const std::vector<Position> longest =
                    LongestFrom(element.body.front().entry, element.body.back().exit, position, rest);
                for (;;) {
                    innerEnd = longest[static_cast<std::size_t>(innerStart - position)];
                    if (innerEnd == NoPosition || innerEnd == innerStart) break;
                    previous = innerStart;
                    innerStart = innerEnd;
                }
                if (innerEnd == NoPosition) {
                    innerEnd = innerStart;
                    innerStart = previous;
                }
                Dissect(element.body, 0, innerStart, innerEnd);
                position = rest;
                break;
            }
            case Op::Alt: {
                const Position rest = Rest(sequence, index, position, stop);
                for (const auto& branch : element.branches) {
                    if (Longest(branch, position, rest) == rest) {
                        Dissect(branch, 0, position, rest);
                        break;
                    }
                }
                position = rest;
                break;
            }
            default:
                break;
            }
        }
    }

    void Restore(const Frame& frame) {
        const auto reg = static_cast<std::size_t>(frame.reg);
        if (frame.node == -1) groups_[reg].start = frame.old;
        else if (frame.node == -2) groups_[reg].end = frame.old;
        else slots_[reg] = frame.old;
    }

    bool Backtrack(Position start, Position& bestEnd, std::vector<GuestMatch>& best) {
        frames_.clear();
        frames_.push_back({program_.start, start, 0, 0});
        while (!frames_.empty()) {
            const Frame frame = frames_.back();
            frames_.pop_back();
            if (frame.node < 0) {
                Restore(frame);
                continue;
            }
            int index = frame.node;
            Position position = frame.position;
            for (bool alive = true; alive;) {
                const Node& node = nodes_[static_cast<std::size_t>(index)];
                switch (node.kind) {
                case Kind::Char:
                case Kind::Set:
                    if (position >= end_ || !Consumes(node, position)) {
                        alive = false;
                        break;
                    }
                    ++position;
                    index = node.out;
                    break;
                case Kind::Bol:
                case Kind::Eol:
                case Kind::Bow:
                case Kind::Eow:
                    if (!Condition(node.kind, position)) alive = false;
                    else index = node.out;
                    break;
                case Kind::Open: {
                    auto& value = groups_[static_cast<std::size_t>(node.arg)].start;
                    frames_.push_back({-1, 0, node.arg, value});
                    value = position;
                    index = node.out;
                    break;
                }
                case Kind::Close: {
                    auto& value = groups_[static_cast<std::size_t>(node.arg)].end;
                    frames_.push_back({-2, 0, node.arg, value});
                    value = position;
                    index = node.out;
                    break;
                }
                case Kind::PlusEnter: {
                    auto& value = slots_[static_cast<std::size_t>(node.arg)];
                    frames_.push_back({-3, 0, node.arg, value});
                    value = position;
                    index = node.out;
                    break;
                }
                case Kind::Backref: {
                    const GuestMatch& group = groups_[static_cast<std::size_t>(node.arg)];
                    if (group.end == -1 || group.end < group.start) {
                        alive = false;
                        break;
                    }
                    const Position length = group.end - group.start;
                    if (length > end_ - position ||
                        std::memcmp(text_ + position, text_ + group.start, static_cast<std::size_t>(length)) != 0) {
                        alive = false;
                        break;
                    }
                    position += length;
                    index = node.out;
                    break;
                }
                case Kind::Split:
                    for (std::size_t i = node.outs.size(); i-- > 1;) frames_.push_back({node.outs[i], position, 0, 0});
                    index = node.outs[0];
                    break;
                case Kind::PlusLoop:
                    if (position > slots_[static_cast<std::size_t>(node.arg)]) {
                        frames_.push_back({node.out, position, 0, 0});
                        index = node.loop;
                    } else {
                        index = node.out;
                    }
                    break;
                case Kind::Accept:
                    if (bestEnd == NoPosition || position > bestEnd) {
                        bestEnd = position;
                        best = groups_;
                    }
                    if (position == end_) return true;
                    alive = false;
                    break;
                }
            }
        }
        return bestEnd != NoPosition;
    }

    bool SearchWithBackrefs(std::size_t wanted, GuestMatch* out) {
        const std::vector<char> viable = Reaches(program_.start, program_.accept, begin_, end_, false);
        std::vector<GuestMatch> best;
        for (Position start = begin_; start <= end_; ++start) {
            if (viable[static_cast<std::size_t>(start - begin_)] == 0) continue;
            groups_.assign(program_.groups + 1, GuestMatch{-1, -1});
            slots_.assign(static_cast<std::size_t>(program_.slots), NoPosition);
            Position bestEnd = NoPosition;
            if (!Backtrack(start, bestEnd, best)) continue;
            groups_ = std::move(best);
            Fill(start, bestEnd, wanted, out);
            CopyGroups(wanted, out);
            return true;
        }
        return false;
    }
};

int Compile(GuestRegex* regex, const char* pattern, int flags) {
    flags &= ~0200;
    if ((flags & FlagExtended) && (flags & FlagNoSpec)) return ErrInvalidArgument;
    std::size_t length;
    if (flags & FlagEndPointer) {
        if (regex->end < pattern) return ErrInvalidArgument;
        length = static_cast<std::size_t>(regex->end - pattern);
    } else {
        length = std::strlen(pattern);
    }
    auto* guts = new (std::nothrow) Guts;
    if (guts == nullptr) return ErrSpace;
    const auto* begin = reinterpret_cast<const unsigned char*>(pattern);
    Parser parser(begin, begin + length, flags, guts->program);
    int error = 0;
    try {
        parser.Run();
        GraphBuilder(guts->program).Run();
    } catch (const Failure& failure) {
        error = failure.code;
    } catch (const std::bad_alloc&) {
        error = ErrSpace;
    }
    regex->subexpressions = parser.Groups();
    if (error != 0) {
        delete guts;
        regex->guts = nullptr;
        regex->magic = 0;
        return error;
    }
    regex->guts = guts;
    regex->magic = MagicCompiled;
    return 0;
}

struct ErrorEntry {
    int code;
    const char* name;
    const char* text;
};

constexpr ErrorEntry ErrorEntries[] = {
    {1, "REG_NOMATCH", "regexec() failed to match"},
    {2, "REG_BADPAT", "invalid regular expression"},
    {3, "REG_ECOLLATE", "invalid collating element"},
    {4, "REG_ECTYPE", "invalid character class"},
    {5, "REG_EESCAPE", "trailing backslash (\\)"},
    {6, "REG_ESUBREG", "invalid backreference number"},
    {7, "REG_EBRACK", "brackets ([ ]) not balanced"},
    {8, "REG_EPAREN", "parentheses not balanced"},
    {9, "REG_EBRACE", "braces not balanced"},
    {10, "REG_BADBR", "invalid repetition count(s)"},
    {11, "REG_ERANGE", "invalid character range"},
    {12, "REG_ESPACE", "out of memory"},
    {13, "REG_BADRPT", "repetition-operator operand invalid"},
    {14, "REG_EMPTY", "empty (sub)expression"},
    {15, "REG_ASSERT", "\"can't happen\" -- you found a bug"},
    {16, "REG_INVARG", "invalid argument to regex routine"},
    {17, "REG_ILLSEQ", "illegal byte sequence"},
};
}

extern "C" {

int APS5_VABI regcomp_nid_postfix(GuestRegex* regex, const char* pattern, int flags) {
    return Compile(regex, pattern, flags);
}

int APS5_VABI regexec_nid_postfix(const GuestRegex* regex, const char* string, std::size_t nmatch, GuestMatch* match, int flags) {
    if (regex->magic != MagicCompiled) return ErrBadPattern;
    const auto* guts = static_cast<const Guts*>(regex->guts);
    if (guts == nullptr || guts->magic != MagicGuts) return ErrBadPattern;
    flags &= FlagNotBol | FlagNotEol | FlagStartEnd;
    if (guts->program.flags & FlagNoSub) nmatch = 0;
    std::int64_t start;
    std::int64_t stop;
    if (flags & FlagStartEnd) {
        start = match[0].start;
        stop = match[0].end;
    } else {
        start = 0;
        stop = static_cast<std::int64_t>(std::strlen(string));
    }
    if (stop < start) return ErrInvalidArgument;
    const std::size_t wanted = std::min(nmatch, guts->program.groups + 1);
    std::vector<GuestMatch> found;
    try {
        found.resize(wanted == 0 ? 1 : wanted);
        Matcher matcher(guts->program, reinterpret_cast<const unsigned char*>(string), start, stop, flags);
        if (!matcher.Search(wanted, found.data())) return ErrNoMatch;
    } catch (const std::bad_alloc&) {
        return ErrSpace;
    }
    for (std::size_t i = 0; i < nmatch; ++i) match[i] = i < wanted ? found[i] : GuestMatch{-1, -1};
    return 0;
}

std::size_t APS5_VABI regerror_nid_postfix(int code, const GuestRegex* regex, char* buffer, std::size_t size) {
    char converted[50];
    const int target = code & ~FlagCodeToName;
    const ErrorEntry* entry = nullptr;
    const char* text;
    if (code == FlagNameToCode) {
        text = "0";
        for (const auto& candidate : ErrorEntries) {
            if (std::strcmp(candidate.name, regex->end) == 0) {
                std::snprintf(converted, sizeof(converted), "%d", candidate.code);
                text = converted;
                break;
            }
        }
    } else {
        for (const auto& candidate : ErrorEntries) {
            if (candidate.code == target) {
                entry = &candidate;
                break;
            }
        }
        if (code & FlagCodeToName) {
            if (entry != nullptr) std::snprintf(converted, sizeof(converted), "%s", entry->name);
            else std::snprintf(converted, sizeof(converted), "REG_0x%x", static_cast<unsigned>(target));
            text = converted;
        } else {
            text = entry != nullptr ? entry->text : "*** unknown regexp error code ***";
        }
    }
    const std::size_t length = std::strlen(text) + 1;
    if (size > 0) {
        if (size > length) {
            std::strcpy(buffer, text);
        } else {
            std::strncpy(buffer, text, size - 1);
            buffer[size - 1] = '\0';
        }
    }
    return length;
}

void APS5_VABI regfree_nid_postfix(GuestRegex* regex) {
    if (regex->magic != MagicCompiled) return;
    auto* guts = static_cast<Guts*>(regex->guts);
    if (guts == nullptr || guts->magic != MagicGuts) return;
    regex->magic = 0;
    guts->magic = 0;
    delete guts;
}

}
