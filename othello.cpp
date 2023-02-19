#include "othello.hpp"

#include <chrono>
#include <iostream>
#include <random>

bool coordinates::operator==(const coordinates &coords) const
{
    return this->x == coords.x && this->y == coords.y;
}

bool move::operator==(const move &m) const
{
    return this->coords == m.coords;
}

OthelloBoard::OthelloBoard() : scores{0, 0}
{
    this->initStartingBoard();
}

void OthelloBoard::initStartingBoard()
{
    for (int i = 0; i < BOARD_DIM; i++)
    {
        for (int j = 0; j < BOARD_DIM; j++)
        {
            this->board[i][j] = NONE;
        }
    }

    this->board[3][4] = DARK;
    this->board[4][3] = DARK;
    this->scores[DARK] += 2;

    this->board[3][3] = LIGHT;
    this->board[4][4] = LIGHT;
    this->scores[LIGHT] += 2;
}

void OthelloBoard::capture(coordinates coords, direction dir, color c)
{
    color oppC = Othello::flipColor(c);
    for (int i = coords.x + dir.first, j = coords.y + dir.second; 0 <= i && i < BOARD_DIM && 0 <= j && j < BOARD_DIM; i += dir.first, j += dir.second)
    {
        if (this->board[i][j] == oppC)
        {
            this->board[i][j] = c;
            this->scores[c]++;
            this->scores[oppC]--;
        }
        else
            break;
    }
}

color OthelloBoard::getDisk(coordinates coords) const
{
    return this->board[coords.x][coords.y];
}

int OthelloBoard::getScore(color c) const
{
    return this->scores[c];
}

void OthelloBoard::displayBoard() const
{
    std::cout << "  ";
    for (int i = 0; i < BOARD_DIM; i++)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < BOARD_DIM; i++)
    {
        std::cout << i << " ";
        for (int j = 0; j < BOARD_DIM; j++)
        {
            switch (this->board[i][j])
            {
            case DARK:
                std::cout << UNICODE_DARK << " ";
                break;
            case LIGHT:
                std::cout << UNICODE_LIGHT << " ";
                break;
            default:
                std::cout << UNICODE_NONE << " ";
                break;
            }
        }
        std::cout << std::endl;
    }
}

bool OthelloBoard::operator==(const OthelloBoard &b) const
{
    for (int i = 0; i < BOARD_DIM; i++)
    {
        for (int j = 0; j < BOARD_DIM; j++)
        {
            if (this->board[i][j] != b.board[i][j])
                return false;
        }
    }
    return true;
}

Othello::Othello()
{
    this->turn = DARK;
    this->passedLast = false;
}

std::vector<move> Othello::getLegalMoves() const
{
    std::vector<move> moves;
    for (int i = 0; i < BOARD_DIM; i++)
    {
        for (int j = 0; j < BOARD_DIM; j++)
        {
            if (this->board[i][j] == NONE)
            {
                move mv;
                mv.coords = {i, j};
                for (direction dir : DIRECTIONS)
                {
                    bool foundOppC = false;
                    for (int k = i + dir.first, l = j + dir.second; 0 <= k && k < BOARD_DIM && 0 <= l && l < BOARD_DIM; k += dir.first, l += dir.second)
                    {
                        if (this->board[k][l] == this->turn)
                        {
                            if (foundOppC)
                                mv.dirs.push_back(dir);
                            break;
                        }
                        else if (this->board[k][l] == Othello::flipColor(this->turn))
                            foundOppC = true;
                        else
                            break;
                    }
                }
                if (mv.dirs.size() > 0)
                    moves.push_back(mv);
            }
        }
    }

    return moves;
}

void Othello::makeMove(move mv)
{
    if (mv == NULL_MOVE)
    {
        this->passedLast = true;
        this->turn = Othello::flipColor(this->turn);
        return;
    }

    this->board[mv.coords.x][mv.coords.y] = this->turn;
    this->scores[this->turn]++;

    for (direction dir : mv.dirs)
    {
        this->capture(mv.coords, dir, this->turn);
    }

    this->turn = Othello::flipColor(this->turn);
    this->passedLast = false;
}

color Othello::getWinner() const
{
    if (this->scores[DARK] == this->scores[LIGHT])
        return NONE;
    return this->scores[DARK] > this->scores[LIGHT] ? DARK : LIGHT;
}

color Othello::getTurn() const
{
    return this->turn;
}

bool Othello::hasPassedLast() const
{
    return this->passedLast;
}

color Othello::flipColor(color c)
{
    switch (c)
    {
    case DARK:
        return LIGHT;
    case LIGHT:
        return DARK;
    default:
        return NONE;
    }
}

void init_zobrist()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis;
    for (int i = 0; i < BOARD_DIM; i++)
    {
        for (int j = 0; j < BOARD_DIM; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                zobristDisks[i][j][k] = dis(gen);
            }
        }
    }
}

unsigned int compute_hash(const OthelloBoard &b)
{
    unsigned int hash = 0;
    for (int i = 0; i < BOARD_DIM; i++)
    {
        for (int j = 0; j < BOARD_DIM; j++)
        {
            color c = b.getDisk({i, j});
            if (c != NONE)
                hash ^= zobristDisks[i][j][c];
        }
    }

    return hash;
}

AI::AI(int depth, bool usePVS)
{
    this->maxDepth = depth;
    this->usePVS = usePVS;

    init_zobrist();
}

move AI::getBestMove(const Othello &o)
{
    auto t1 = std::chrono::high_resolution_clock::now();
    float bestScore;
    move bestMove;
    this->nodes = this->hits = this->prunes = this->researches = this->reductions = this->noPVMove = this->ttUpdates = 0;
    for (int depth = (this->maxDepth % 2 == 0 ? 2 : 1); depth <= this->maxDepth; depth += 2)
    {
        this->curDepth = this->searchDepth = depth;
        auto [curBestScore, curBestMove] = this->negamax(o, -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 0, true);
        bestScore = curBestScore;
        bestMove = curBestMove;
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "t:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;
    std::cout << "eval:" << std::showpos << (o.getTurn() == DARK ? bestScore : -bestScore) << std::noshowpos << std::endl;
    std::cout << "hits/nodes:" << this->hits << "/" << this->nodes << std::endl;
    std::cout << this->nodes / std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count() << " nodes/s" << std::endl;
    std::cout << "prunes:" << this->prunes << std::endl;
    std::cout << "re-searches:" << this->researches << std::endl;
    std::cout << "reductions:" << this->reductions << std::endl;
    // std::pair<Othello, tt_entry> pv[this->maxDepth + 1];
    // Othello pvO = o;
    // for (int i = 0; i < this->maxDepth; i++)
    // {
    //     tt_entry entry = tt.at(pvO);
    //     pv[i] = {pvO, entry};
    //     pvO.makeMove(entry.bestMove);
    // }

    return bestMove;
}

std::pair<float, move> AI::negamax(const Othello &currO, float a, float b, int depth, bool prevPV)
{
    if (this->curDepth == this->maxDepth)
        this->nodes++;
    float eval = evaluate(currO);
    int relDepth = this->curDepth - depth;
    float a0 = a;
    bool hasEntry = false;
    tt_entry *entry;
    if (this->tt.find(currO) != this->tt.end())
    {
        hasEntry = true;
        entry = &this->tt[currO];
        if (entry->relDepth >= relDepth)
        {
            if (this->curDepth == this->maxDepth)
                this->hits++;
            float entryScore = eval + entry->relScore;
            if (entry->flag == EXACT)
                return {entryScore, entry->bestMove};
            else if (entry->flag == LOWER)
                a = std::max(a, entryScore);
            else if (entry->flag == UPPER)
                b = std::min(b, entryScore);

            if (a >= b)
                return {entryScore, entry->bestMove};
        }
    }

    if (depth >= this->searchDepth)
    {
        if (this->curDepth == this->maxDepth)
            this->depthCutoffs[this->searchDepth]++;
        return {eval, NULL_MOVE};
    }

    int startingDepth = this->searchDepth;
    float bestScore;
    move bestMove = NULL_MOVE;
    std::vector<move> moves = currO.getLegalMoves();
    if (moves.empty())
    {
        if (currO.hasPassedLast())
            return {eval == 0 ? 0 : eval > 0 ? WIN_SCORE
                                             : -WIN_SCORE,
                    NULL_MOVE};
        moves.push_back(NULL_MOVE);
    }
    move pvMove = hasEntry ? entry->bestMove : NULL_MOVE;
    if (this->curDepth == this->maxDepth && pvMove == NULL_MOVE)
        this->noPVMove++;
    std::sort(moves.begin(), moves.end(), [this, &currO, &pvMove](move moveA, move moveB)
              { return betterMove(currO, moveA, moveB, pvMove); });
    for (int i = 0; i < moves.size(); i++)
    {
        move move = moves[i];
        bool isPV = move == pvMove;
        int reduction = getDepthReduction(prevPV && isPV, depth, i, moves.size());
        if (this->curDepth == this->maxDepth)
            this->reductions += reduction;
        this->searchDepth = startingDepth - reduction;
        color currTurn = currO.getTurn();
        Othello nextO = currO;
        nextO.makeMove(move);
        float score;
        if (!this->usePVS || isPV)
            score = -this->negamax(nextO, -b, -a, depth + 1, true).first;
        else
        {
            score = -this->negamax(nextO, -a - 1, -a, depth + 1).first;
            if (a < score && score < b)
            {
                if (this->curDepth == this->maxDepth)
                    this->researches++;
                this->searchDepth = startingDepth;
                score = -this->negamax(nextO, -b, -score, depth + 1).first;
                // score = -negamax(nextO, -b, -a, depth + 1).first;
            }
        }
        if (bestMove == NULL_MOVE || score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }
        a = std::max(a, score);
        if (a >= b)
        {
            if (this->curDepth == this->maxDepth)
                this->prunes++;
            break;
        }
    }

    if (!hasEntry || relDepth >= entry->relDepth)
    {
        if (!hasEntry)
        {
            this->tt[currO] = tt_entry{};
            entry = &this->tt[currO];
        }
        if (bestScore <= a0)
        {
            if (this->curDepth == this->maxDepth)
                this->nodeTypes[2]++;
            entry->flag = UPPER;
        }
        else if (bestScore >= b)
        {
            if (this->curDepth == this->maxDepth)
                this->nodeTypes[1]++;
            entry->flag = LOWER;
        }
        else
        {
            if (this->curDepth == this->maxDepth)
                this->nodeTypes[0]++;
            entry->flag = EXACT;
        }
        entry->relScore = bestScore - eval;
        entry->bestMove = bestMove;
        entry->relDepth = relDepth;
        this->ttUpdates++;
    }

    return {bestScore, bestMove};
}

bool AI::betterMove(const Othello &currO, move moveA, move moveB, move pvMove) const
{
    if (moveA == pvMove)
        return true;
    if (moveB == pvMove)
        return false;

    return moveA.dirs.size() > moveB.dirs.size();
}

float AI::evaluate(const Othello &currO)
{
    color turn = currO.getTurn();
    return currO.getScore(turn) - currO.getScore(Othello::flipColor(turn));
}

int AI::getDepthReduction(bool isPV, int depth, int i, int numMoves)
{
    if (isPV || i <= 2 || depth < 6)
        return 0;
    return 1;
}

int main()
{
    color AI_COLOR = LIGHT;

    Othello o;
    int depth;
    std::cout << "Depth:";
    std::cin >> depth;
    AI ai(depth);

    auto t0 = std::chrono::high_resolution_clock::now();

    while (true)
    {
        o.displayBoard();
        std::cout << "It is " << (o.getTurn() == DARK ? "dark" : "light") << "'s turn." << std::endl;
        std::vector<move> moves = o.getLegalMoves();
        if (moves.empty())
        {
            std::cout << "No legal moves. Passing turn." << std::endl;
            if (o.hasPassedLast())
                break;
            o.makeMove(NULL_MOVE);
        }
        else
        {
            move m;
            if (o.getTurn() == AI_COLOR)
            {
                m = ai.getBestMove(o);
                std::cout << "AI move: (" << m.coords.x << ", " << m.coords.y << ")" << std::endl;
            }
            else
            {

                std::cout << "Legal moves: " << std::endl;
                for (int i = 0; i < moves.size(); i++)
                {
                    std::cout << i << ": (" << moves[i].coords.x << ", " << moves[i].coords.y << ")" << std::endl;
                }
                int choice;
                std::cin >> choice;
                m = moves[choice];
            }

            o.makeMove(m);
        }
    }

    o.displayBoard();

    if (o.getWinner() == NONE)
        std::cout << "Draw!" << std::endl;
    else
        std::cout << (o.getWinner() == DARK ? "Dark" : "Light") << " wins!" << std::endl;
    std::cout << "Final score: Dark: " << o.getScore(DARK) << ", Light: " << o.getScore(LIGHT) << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << "ms" << std::endl;
}