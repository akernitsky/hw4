//
//  main.cpp
//  hw4
//
//  Created by Alexander Kernitsky on 17.11.13.
//  Copyright (c) 2013 Alexander Kernitsky. All rights reserved.
//

#include <stdio.h>
#include <vector>
#include <set>
#include <algorithm>
#include <queue>
#include <functional>
#include <iostream>

class Edge
{
public:
	Edge(int _vertex1, int _vertex2) : vertex1(_vertex1), vertex2(_vertex2) {}
    
	int GetVertex1() const { return vertex1; }
	int GetVertex2() const { return vertex2; }
	
private:
	int vertex1;
	int vertex2;
};

// this operator is necessary for set container
bool operator<(const Edge& left, const Edge& right)
{
	// we don't care about weight beacuse we assume that graph has no parallel edges
	return (left.GetVertex1() < right.GetVertex1() ||
            (!(right.GetVertex1() < left.GetVertex1()) && left.GetVertex2() < right.GetVertex2()));
}

enum NodeState
{
    NS_Red, // occupied by red "X"
    NS_Blue, // occupied by blues "O"
    NS_Empty // not occupied
};

class HexNode
{
public:
    HexNode() : state(NS_Empty) {}
    
    bool IsRed() const { return state == NS_Red; }
    bool isBlue() const { return state == NS_Blue; }
    bool IsEmpty() const { return state == NS_Empty; }
    
    NodeState GetState() const { return state; }
    void SetState(NodeState newNodeState) { state = newNodeState; }
    
private:
    NodeState state;
};


class Graph
{
public:
	Graph() {}
	explicit Graph(int _numberOfVertices);
    
	void SetNumberOfVertices(int _numberOfVertices);
    
	// number of vertices
	int V() const { return numberOfVertices; }
	// number of edges
	int E() const;
    
    void Draw() const;
    
	bool Adjacent(int x, int y) const;
	bool AddEdge(int x, int y);
	void RemoveEdge(int x, int y);
	void Neighbors(int x, std::vector<int>& neigborsVertexIndices) const;
    
private:
	int numberOfVertices;
	std::vector<std::set<Edge>> edges; // for each vertex there is set<Edge>
};

Graph::Graph(int _numberOfVertices) :
    numberOfVertices(_numberOfVertices)
{
	SetNumberOfVertices(_numberOfVertices);
}


void Graph::SetNumberOfVertices(int _numberOfVertices)
{
	numberOfVertices = _numberOfVertices;
	edges.clear();
    
	for(int i = 0; i < numberOfVertices; ++i)
	{
		edges.push_back(std::set<Edge>());
	}
}

int Graph::E() const
{
	int edgesCount = 0;
	for(std::vector<std::set<Edge>>::const_iterator it = edges.begin(); it != edges.end(); ++it)
		edgesCount += (*it).size();
	
	return edgesCount / 2;
}

bool Graph::Adjacent(int x, int y) const
{
	Edge edgeToFind(x, y); // we don't care about weight in this case
    
	return edges[x].find(edgeToFind) != edges[x].end();
}

void Graph::Neighbors(int x, std::vector<int>& neigborsVertexIndices) const
{
	neigborsVertexIndices.clear();
	for(std::set<Edge>::iterator it = edges[x].begin(); it != edges[x].end(); ++it)
	{
		neigborsVertexIndices.push_back((*it).GetVertex2());
	}
}

bool Graph::AddEdge(int x, int y)
{
	if(!Adjacent(x, y) && !Adjacent(y, x))
	{
		Edge edgeToAdd1(x, y);
        Edge edgeToAdd2(y, x);
		edges[x].insert(edgeToAdd1);
        edges[y].insert(edgeToAdd2);
		return true;
	}
	return false;
}

void Graph::RemoveEdge(int x, int y)
{
	Edge edgeToRemove1(x, y);
    Edge edgeToRemove2(y, x);
	edges[x].erase(edgeToRemove1);
    edges[y].erase(edgeToRemove2);
}

enum BoardState
{
    BS_RedWins,
    BS_BlueWins,
    BS_GameInProgress,
};

// board for hex game
class HexBoard : public Graph
{
    friend std::ostream& operator<<(std::ostream &out, const HexBoard&);
    
public:
    explicit HexBoard(int _boardExtent);
    
    // returns true if move successfull, returns false if move is invalid
    bool MakeMove(int x, int y, bool isRed);
    
    BoardState GetBoardState() const { return boardState; }
    
private:
    const int boardExtent;
    std::vector<HexNode> nodes;
    BoardState boardState;
    
    int getTopVertexIndex() const { return V() - 4; }
    int getBottomVertexIndex() const { return V() - 3; }
    int getLeftVertexIndex() const { return V() - 2; }
    int getRightVertexIndex() const { return V() - 1; }
    void makeConnections(int x, int y);
    void drawLine(std::ostream &out, int lineIndex) const; // draw horz line
    void drawNode(std::ostream& out, int nodeIndex) const; // draw one node
    
    void checkBoardState();
    bool isConnected(int vertex1, int vertex2) const;
    bool dfs(int vertex1, int vertex2, std::vector<bool>& visitedVertices) const;
    
    int getNodeIndex(int x, int y) const { return y * boardExtent + x; }
};
    
HexBoard::HexBoard(int _boardExtent) : Graph(_boardExtent * _boardExtent + 4),
    boardExtent(_boardExtent),
    boardState(BS_GameInProgress)
{
    HexNode hexNode;
    
    for(int i = 0; i < V(); ++i)
        nodes.push_back(hexNode);
    
    nodes[getLeftVertexIndex()].SetState(NS_Blue); // special node for connecting all blue pieces on left side
    nodes[getRightVertexIndex()].SetState(NS_Blue); // special node for connecting all blue pieces on right side
    nodes[getTopVertexIndex()].SetState(NS_Red);  // special node for connecting all red pieces on top side
    nodes[getBottomVertexIndex()].SetState(NS_Red);  // special node for connecting all red pieces on bottom side
}

bool HexBoard::MakeMove(int x, int y, bool isRed)
{
    if(x >= 0 && x < boardExtent && y >= 0 && y < boardExtent)
    {
        const int nodeIndex = getNodeIndex(x, y);
        if(nodes[nodeIndex].IsEmpty())
        {
            nodes[nodeIndex].SetState(isRed ? NS_Red : NS_Blue);
            makeConnections(x, y);
            checkBoardState();
            return true;
        }
        
    }
    return false;
}

void HexBoard::makeConnections(int x, int y)
{
    const int nodeIndex = getNodeIndex(x, y);
    
    std::vector<int> possibleNeighbors;
    
    if(x == 0)
        possibleNeighbors.push_back(getLeftVertexIndex()); // connected to special left node
    else
    {
        const int leftEdgePosition = getNodeIndex(x - 1, y);
        possibleNeighbors.push_back(leftEdgePosition);
        
        if(y < boardExtent - 1)
        {
            const int leftBottomEdgePosition = getNodeIndex(x - 1, y + 1);
            possibleNeighbors.push_back(leftBottomEdgePosition);
        }
    }
    
    if(x == boardExtent - 1)
        possibleNeighbors.push_back(getRightVertexIndex());  // connected to special right node
    else
    {
        const int rightEdgePosition = getNodeIndex(x + 1, y);
        possibleNeighbors.push_back(rightEdgePosition);
        
        if(y > 0)
        {
            const int rightTopEdgePosition = getNodeIndex(x + 1, y - 1);
            possibleNeighbors.push_back(rightTopEdgePosition);
        }
    }
    
    if(y == 0)
        possibleNeighbors.push_back(getTopVertexIndex()); // connected to special top node
    else
    {
        const int topEdgePosition = getNodeIndex(x, y - 1);
        possibleNeighbors.push_back(topEdgePosition);
    }
    
    if(y == boardExtent - 1)
        possibleNeighbors.push_back(getBottomVertexIndex()); // connected to special bottom node
    else
    {
        const int bottomEdgePosition = getNodeIndex(x, y + 1);
        possibleNeighbors.push_back(bottomEdgePosition);
    }
    
    for (std::vector<int>::const_iterator it = possibleNeighbors.begin(); it != possibleNeighbors.end(); ++it)
    {
        if(!nodes[nodeIndex].IsEmpty() && nodes[*it].GetState() == nodes[nodeIndex].GetState())
        {
            AddEdge(nodeIndex, *it);
        }
    }
    
}

void HexBoard::drawNode(std::ostream& out, int nodeIndex) const
{
    switch (nodes[nodeIndex].GetState()) {
        case NS_Red:
            out << "X";
            break;
        case NS_Blue:
            out << "O";
            break;
        case NS_Empty:
            out << ".";
            break;
    }
}

void HexBoard::drawLine(std::ostream &out, int lineIndex) const
{
    const int firstNodeInThisLineIndex = lineIndex * boardExtent;
  
    out << lineIndex << "  ";
    drawNode(out, firstNodeInThisLineIndex);
    
    for( int i = firstNodeInThisLineIndex + 1; i < firstNodeInThisLineIndex + boardExtent; ++i)
    {
        out << " - ";
        drawNode(out, i);
    }
    
    out << std::endl;
}

bool HexBoard::isConnected(int vertex1, int vertex2) const
{
    // visited vectors
    std::vector<bool> visitedVertices(V(), false);
    // using depth first search to determine if vertex1 connected to vertex2
    return dfs(vertex1, vertex2, visitedVertices);
}

bool HexBoard::dfs(int vertex1, int vertex2, std::vector<bool>& visitedVertices) const
{
    std::vector<int> neigbors;
    Neighbors(vertex1, neigbors);
    visitedVertices[vertex1] = true;
    
    if(vertex1 == vertex2)
        return true;
    
    for(std::vector<int>::const_iterator it = neigbors.begin(); it != neigbors.end(); ++it)
    {
        if(!visitedVertices[*it])
        {
            const bool found = dfs(*it, vertex2, visitedVertices);
            if(found)
                return true;
        }
    }
    return false;
}

void HexBoard::checkBoardState()
{
    if(isConnected(getLeftVertexIndex(), getRightVertexIndex()))
        boardState = BS_BlueWins;
    else if(isConnected(getTopVertexIndex(), getBottomVertexIndex()))
        boardState = BS_RedWins;
}

static void drawHorNumbers(std::ostream &out, int boardExtent)
{
    out << "  " << 0;

    for(int i = 1; i < boardExtent; ++i)
        out << "   " << i;
    
    out << std::endl;
}

static void drawEdges(std::ostream &out, int boardExtent)
{
    out << "   ";
    
    for(int i = 0; i < 2 * boardExtent - 1; ++i)
        out << ((i % 2 == 0) ? "\\ " : "/ ");
    
    out << std::endl;
}

// draw offset
static void drawOffset(std::ostream& out, int offset)
{
    for(int i = 0; i < offset; ++i)
        out << " ";
}

std::ostream& operator<<(std::ostream &out, const HexBoard& hex)
{
    drawHorNumbers(out, hex.boardExtent);
    hex.drawLine(out, 0);
    
    for(int i = 1; i < hex.boardExtent; ++i)
    {
        drawOffset(out, 2 * i - 1);
        drawEdges(out, hex.boardExtent);
        drawOffset(out, 2 * i);
        hex.drawLine(out, i);
    }
    return out;
}

static void playerMove(HexBoard& board, bool isRed)
{
    const std::string playerName = isRed ? "Second" : "First";
    std::cout << playerName << " Player enter you move (x y)";
    int x1 = -1;
    int y1 = -1;
    std::cin >> x1 >> y1;
    std::cout << std::endl;
    
    while(!board.MakeMove(x1, y1, isRed))
    {
        std::cout << "illegal move try again" << std::endl;
        std::cin >> x1 >> y1;
        std::cout << std::endl;
    }
}

static void player1Move(HexBoard& board)
{
    playerMove(board, false);
}

static void player2Move(HexBoard& board)
{
    playerMove(board, true);
}

int main()
{
    std::cout << "Enter board size (1...1000)";
    int boardSize = 0;
    std::cin >> boardSize;
    std::cout << std::endl;
    
    while(boardSize < 1 || boardSize > 1000) {
        std::cout << "Enter board size (1...1000)";
        std::cin >> boardSize;
        std::cout << std::endl;
    }
    
    HexBoard hexBoard(boardSize);
    
    std::cout << hexBoard << std::endl;
    
    while(hexBoard.GetBoardState() == BS_GameInProgress)
    {
        player1Move(hexBoard);
        std::cout << hexBoard << std::endl;
        
        if(hexBoard.GetBoardState() == BS_GameInProgress)
        {
            player2Move(hexBoard);
            std::cout << hexBoard << std::endl;
        }
	}
    if(hexBoard.GetBoardState() == BS_BlueWins)
    {
        std::cout << "First Player wins" << std::endl;
    }
    else if(hexBoard.GetBoardState() == BS_RedWins)
    {
        std::cout << "Second Player wins" << std::endl;
    }
    else
    {
        std::cout << "Unexpected error" << std::endl;
    }
    
    return 0;
}

