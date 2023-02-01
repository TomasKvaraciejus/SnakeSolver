#include <iostream>
#include <stdlib.h>
#include <windows.h>
#include <queue>
#include <string>
#include <stack>
#include <unordered_map>

struct coord
{
	int x = 0, y = 0;

	coord(int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	coord()
	{
		x = 0; 
		y = 0;
	}

	coord absDistance (coord bCoord)
	{
		return coord(abs(x - bCoord.x), abs(y - bCoord.y));
	}

	friend bool operator< (const coord & aCoord, const coord & bCoord)
	{
		return (aCoord.x + aCoord.y < bCoord.x + bCoord.y);
	}

	friend bool operator== (const coord &aCoord, const coord &bCoord)
	{
		return (aCoord.x == bCoord.x && aCoord.y == bCoord.y);
	}

	friend bool operator!= (const coord& aCoord, const coord& bCoord)
	{
		return !(aCoord == bCoord);
	}
};

template <>
struct std::hash<coord>
{
	std::size_t operator()(const coord& c) const
	{
		using std::size_t;
		using std::hash;

		return ((hash<int>()(c.x)
			^ (hash<int>()(c.y) << 1)) >> 1);
	}
};

std::ostream& operator << (std::ostream& os, const coord& c)
{
	return (os << c.x << ' ' << c.y);
}

class node
{
public:

	coord c;
	int g, h;
	node* parent;

	node(coord _c, int _g, int _h)
	{
		c = _c;
		g = _g;
		h = _h;
	}

	int f()
	{
		return g + h;
	}
};

class Compare
{
	public:

	bool operator()(const node* lhs, const node* rhs) const { return (lhs->c < rhs->c); }

	bool operator() (node *a, node *b)
	{
		return ((a->f()) > (b->f()));
	}
};

const int gridSize = 16;
const int minSnakeLength = 3;
char grid[gridSize][gridSize]; // game grid, values are: O - empty space, S - snake, A - apple, H - snake head 
std::queue<coord> snake;
std::stack<coord> path;
std::string endMsg = "???";

coord userInput;
coord prevUserInput;

bool gameInProgress = true;
bool playerInControl = false;
std::vector<coord> applePos;

void updateScreen();
void initializeGrid();
void moveSnake();
int clampValue(int val);
void generateApple(int amount);
void getInput();
void drawPath(std::stack<coord> path);
std::stack<coord> snakeSolver();
std::stack<coord> parsePath(node *q);

int main(int argc, char *argv[])
{
	srand(time(NULL));

	initializeGrid();
	path = snakeSolver();
	updateScreen();

	while (userInput.x == 0 && userInput.y == 0) { getInput(); } // wait for user input

	while (gameInProgress)
	{
		getInput();
		moveSnake();
		updateScreen();
		Sleep(100);
	}

	std::cout << "snork " << endMsg << "\nscore: " << snake.size();
}

// GAME STATE

void initializeGrid()
{
	memset(grid, ' ', sizeof(grid));

	snake.push(coord(gridSize / 2, gridSize / 2));
	grid[snake.back().x][snake.back().y] = 'H';

	generateApple(3);
}

void moveSnake()
{
	int nextX;
	int nextY;

	if (playerInControl)
	{
		nextX = clampValue(snake.back().x + userInput.x);
		nextY = clampValue(snake.back().y + userInput.y);
	}
	else
	{
		nextX = path.top().x;
		nextY = path.top().y;
		path.pop();
	}

	grid[snake.back().x][snake.back().y] = 'S';
	snake.push(coord(nextX, nextY));

	if (grid[nextX][nextY] == 'A')
	{
		applePos.erase(std::find(applePos.begin(), applePos.end(), coord(nextX, nextY)));
		generateApple(1);
		if (!playerInControl)
		{
			path = snakeSolver();
			if (path.empty())
			{
				gameInProgress = false;
				endMsg = "lost";
			}
		}
	}
	else if (grid[nextX][nextY] == 'S')
	{
		gameInProgress = false;
		endMsg = "died";
	}
	else if(snake.size() > minSnakeLength)
	{
		grid[snake.front().x][snake.front().y] = ' ';
		snake.pop();
	}

	grid[nextX][nextY] = 'H';

	prevUserInput = userInput;
}

int clampValue(int val)
{
	if (val >= gridSize)
		return 0;
	else if (val < 0)
		return gridSize - 1;
	else
		return val;
}

void generateApple(int amount)
{
	for (int i = 0; i < amount; ++i)
	{
		int x, y;
		do
		{
			x = rand() % gridSize;
			y = rand() % gridSize;
		} while (grid[x][y] == 'S' || grid[x][y] == 'H' || grid[x][y] == 'A');
		applePos.push_back(coord(x, y));
		grid[x][y] = 'A';
	}

	std::sort(applePos.begin(), applePos.end(), ([](coord a, coord b) { return (a.absDistance(snake.back()) < b.absDistance(snake.back())); }));
}
 
// UI

void updateScreen()
{
	std::string output = "";
	for (int i = 0; i < 10; ++i)
	{
		output += "\n";
	}
	//system("cls");

	for (int y = 0; y < gridSize; ++y)
	{
		for (int x = 0; x < gridSize; ++x)
		{
			output += std::string(1, grid[x][gridSize - y - 1]) + " ";
		}
		output += '\n';
	}

	std::cout << output;
}

void getInput()
{
	if (GetAsyncKeyState(0x57) && prevUserInput.y != -1) // up key
	{
		userInput.y = 1;
		userInput.x = 0;
	}
	if (GetAsyncKeyState(0x53) && prevUserInput.y != 1) // down key
	{
		userInput.y = -1;
		userInput.x = 0;
	}
	if (GetAsyncKeyState(0x41) && prevUserInput.x != 1) // left key
	{
		userInput.x = -1;
		userInput.y = 0;
	}
	if (GetAsyncKeyState(0x44) && prevUserInput.x != -1) // right key
	{
		userInput.x = 1;
		userInput.y = 0;
	}
}

// SNAKE SOLVER

void drawPath(std::stack<coord> path)
{
	path.pop();
	while (path.size() != 1)
	{
		coord p = path.top();
		path.pop();
			grid[p.x][p.y] = '.';
	}
}

std::stack<coord> snakeSolver()
{
	std::unordered_map<coord, int> gameStates;
	std::queue<coord> _snake = snake;
	for (int i = 0; i < snake.size(); ++i)
	{
		gameStates.emplace(_snake.front(), i);
		_snake.pop();
	}

	coord start = snake.back(), end;
	int h = 0;

	for (int appleIndex = 0; appleIndex < applePos.size(); ++appleIndex)
	{
		std::priority_queue<node*, std::vector<node*>, Compare> open;
		node* visited[gridSize][gridSize];
		bool inOpen[gridSize][gridSize];
		memset(inOpen, false, sizeof(inOpen));
		memset(visited, NULL, sizeof(visited));
		node* q = new node(start, 0, abs(start.x - end.x) + abs(start.y - end.y));

		open.emplace(q);

		end = applePos[appleIndex];
		while (!open.empty())
		{
			q = open.top();

			if (q->c == end)
			{
				return parsePath(q);
			}

			open.pop();
			inOpen[q->c.x][q->c.y] = false;

			for (int _x = -1; _x < 2; ++_x)
			{
				for (int _y = -1; _y < 2; ++_y)
				{
					if (abs(_x + _y) == 1)
					{
						int x = clampValue(q->c.x + _x),
							y = clampValue(q->c.y + _y);

						if (grid[x][y] != 'S') //|| (gameStates.find(coord(x, y)) != gameStates.end() && (gameStates.find(coord(x, y))->second < q->g)))
						{
							node* nextNode = new node(coord(x, y),
								q->g + 1,
								abs(x - end.x) + abs(y - end.y));

							nextNode->parent = q;

							if (visited[x][y] == NULL || visited[x][y]->g > nextNode->g)
							{
								visited[x][y] = nextNode;

								if (!inOpen[x][y])
								{
									open.emplace(nextNode);
									inOpen[x][y] = true;
								}
							}
						}
					}
				}
			}
		}
	}

	return parsePath(nullptr);
}

std::stack<coord> parsePath(node* q)
{
	std::stack<coord> p;
	while (q != nullptr)
	{
		p.push(q->c);
		q = q->parent;
	}
	if (!p.empty())
		p.pop();
	return p;
}