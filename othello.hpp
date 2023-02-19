#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using direction = std::pair<int, int>;

const size_t BOARD_DIM = 8;
const std::string UNICODE_NONE = " ",
                  UNICODE_DARK = "●",
                  UNICODE_LIGHT = "○";
const float WIN_SCORE = std::numeric_limits<float>::infinity();
const direction DIRECTIONS[] = {
    {0, 1},  // right
    {0, -1}, // left
    {1, 0},  // down
    {-1, 0}, // up
    {1, 1},  // down-right
    {1, -1}, // down-left
    {-1, 1}, // up-right
    {-1, -1} // up-left
};

enum color
{
    DARK,
    LIGHT,
    NONE
};

enum tt_flag
{
    EXACT,
    LOWER,
    UPPER
};

struct coordinates
{
    int x, y;

    bool operator==(const coordinates &coords) const;
};

struct move
{
    coordinates coords;
    std::vector<direction> dirs;

    bool operator==(const move &m) const;
};

const move NULL_MOVE = {{-1, -1}, {}};

struct tt_entry
{
    int relDepth;
    tt_flag flag;
    float relScore;
    move bestMove;
};

class OthelloBoard
{
private:
    void initStartingBoard();

protected:
    color board[BOARD_DIM][BOARD_DIM];
    int scores[2];

    void capture(coordinates coords, direction dir, color c);

public:
    OthelloBoard();

    color getDisk(coordinates coords) const;
    int getScore(color c) const;
    void displayBoard() const;

    bool operator==(const OthelloBoard &b) const;
};

class Othello : public OthelloBoard
{
private:
    color turn;
    bool passedLast;

public:
    Othello();

    std::vector<move> getLegalMoves() const;
    void makeMove(move mv);
    color getWinner() const;
    color getTurn() const;
    bool hasPassedLast() const;

    static color flipColor(color c);
};

unsigned int zobristDisks[BOARD_DIM][BOARD_DIM][2];

void init_zobrist();
unsigned int compute_hash(const OthelloBoard &b);

template <>
struct std::hash<OthelloBoard>
{
    size_t operator()(const OthelloBoard &b) const
    {
        return compute_hash(b);
    }
};

class AI
{
private:
    int maxDepth, curDepth, searchDepth;
    bool usePVS;
    int nodes, hits, prunes, researches, reductions, noPVMove, ttUpdates;
    int depthCutoffs[100] = {}, nodeTypes[3] = {};
    std::unordered_map<OthelloBoard, tt_entry> tt;

    std::pair<float, move> negamax(const Othello &currO, float a, float b, int depth, bool prevPV = false);
    bool betterMove(const Othello &currO, move i, move j, move pvMove) const;

    static float evaluate(const Othello &currO);
    static int getDepthReduction(bool isPV, int depth, int i, int numMoves);

public:
    AI(int depth, bool usePVS = true);

    move getBestMove(const Othello &m);
};