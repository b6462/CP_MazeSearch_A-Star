#include"pch.h"
#include<stdio.h>
#include<Windows.h>
#include<time.h>
#include<math.h>

//***************************宏定义段***************************//

#define WALL  0 //墙体
#define ROUTE -1 //通路
#define SRCH -2 //搜索位置
#define START -3 //起点
#define EXIT -4 //出口
#define CUR -5 //当前位置

//*****↓新算法相关↓*****//

#define TURN 3 //转弯增加权值
#define REVERSE 10 //回头增加权值
#define RE_STEP 5 //重复拓展增加权值

//***************************宏定义结束***************************//




//***************************全局变量声明段***************************//

int Regularity = 5;//规整度//每次挖掘最大值
int MapScale = 20;
bool Debug_mode = false; //Debug模式 包含实时挖掘迷宫生成 阶段探索、解谜迷宫过程 以及涉及变量显示

int tgt_x = 0;//目标位置全局变量
int tgt_y = 0;

//0123 上右下左
int mapDir = 1;//下一位置相对于小车在地图上的方向 初始定为向右
int selDir = 1;//小车相对于地图的方向 初始定为向右

int curPos[2] = {0};

//***************************全局变量声明结束***************************//




//***************************结构体声明***************************//

typedef struct TreeNode
{
	TreeNode* prev;//用于List
	int mht;
	int pos[2];
	bool open;
	bool close;
	TreeNode* next;//用于List
}Node, *PNode;

//***************************结构体声明结束***************************//




//***************************函数声明***************************//

PNode NodeOpenInit();//初始化OPEN节点 前后指针置NULL MHT默认取INT_MAX
PNode openSort(PNode openHead);//遍历OPEN表 返回MHT最小的节点位置
int Manhattan(int x1, int y1, int x2, int y2);//计算曼哈顿距离
void TraverseList(PNode TNode);//遍历链表 用于DEBUG
void open_to_close(PNode moveNode, PNode closeHead);//将moveNode从OPEN表移动到CLOSE表
void draw_map(int **maze);//绘制迷宫 刷新迷宫
bool node_to_open(PNode openHead, PNode nextNode);//将nextNode移入OPEN表
void lookAhead(int **maze, PNode curNode, PNode openHead);//从当前位置curNode向前探索 并将可搜索节点添加到OPEN表中
void CreateMaze(int **maze, int x, int y);//迷宫生成函数
void SolveMaze(int **maze, int ent_x, int ent_y);//迷宫探索主函数

//***************************↓新世界↓***************************//

void SolveMaze_newAlg(int **maze, int ent_x, int ent_y);//新算法迷宫探索主函数
void move_to(int map_dir, int **maze);//新算法中用于向目标方向移动位置
int lookAhead_newAlg(int **timeMaze, int **maze, int cur_x, int cur_y);//新算法向前探索函数
int posJudge(int cur_x, int cur_y, int nex_x, int nex_y);//新算法用于确定目标点相对现位置方向的函数
int dir_map_to_sel(int _map_dir);//返回小车相对于地图移动方向
int dir_sel_to_map(int _sel_dir);//返回小车相对于自身移动方向
void draw_map_newAlg(int **maze);//基于新算法数值的新绘制函数

//***************************函数声明结束***************************//

void gotoxy(int x, int y) //防闪屏
{
	COORD pos = {x, y};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);// 获取标准输出设备句柄
	SetConsoleCursorPosition(hOut, pos);//两个参数分别是指定哪个窗体，具体位置
}

void curAbsFacing()
{
	;
}

void solveMem() //清空输入缓存
{
	char c_tmp;
	while ((c_tmp = getchar() != '\n') && c_tmp != EOF);//循环getchar清洗回车缓存
}

bool check()//判断函数 用于判断Y/N输入 返回BOOL
{
	char redo;
	printf("(Y/N)\n");
	scanf_s("%c", &redo);
	if (redo == 'N')
		return FALSE;
	else if (redo == 'Y')
		return TRUE;
	else return check(); //如果输入非Y/N 则重复判断并取判断值返回
}

int Manhattan(int x1, int y1, int x2, int y2)
{
	int result = abs(x2 - x1) + abs(y2 - y1);
	return result;
}

void TraverseList(PNode TNode)
{
	PNode temp  = NodeOpenInit();
	temp = TNode;
	int cal = 0;
	while (temp)
	{
		cal++;
		printf("curMht:%d\nCurPos:%d,%d\nopen:%d\nclose:%d\n\n", temp->mht, temp->pos[0], temp->pos[1], temp->open, temp->close);
		temp = temp->next;
		if (cal >= 20)
			system("pause");//防止死循环卡死 用于debug
	}
}

PNode NodeOpenInit()//新建节点并返回 之后设置给open
{
	PNode newNode = (PNode)malloc(sizeof(Node));
	newNode->close = false;
	newNode->open = true;
	newNode->next = NULL;
	newNode->prev = NULL;
	newNode->mht = INT_MAX;
	return newNode;
}

void open_to_close(PNode moveNode, PNode closeHead)//获得close表头 将需要移动节点移动到close表表头之下
{
	if (moveNode->prev)
	{
		moveNode->prev->next = moveNode->next;
	}
	if (moveNode->next)
	{
		moveNode->next->prev = moveNode->prev;
	}

	if (closeHead->next)
	{
		moveNode->prev = closeHead;
		moveNode->next = closeHead->next;
		closeHead->next->prev = moveNode;
		closeHead->next = moveNode;
	}
	else
	{
		closeHead->next = moveNode;
		moveNode->prev = closeHead;
		moveNode->next = NULL;
	}
	moveNode->close = true;
	moveNode->open = false;
}

bool node_to_open(PNode openHead, PNode closeHead, PNode nextNode)
{
	bool checker = true;
	PNode temp = closeHead->next;
	while (temp)
	{
		if (temp->pos[0] == nextNode->pos[0] && temp->pos[1] == nextNode->pos[1])
		{
			if (Debug_mode)
			{
				printf("位于 %d,%d 的节点被证实冗余 不列入OPEN表\n", temp->pos[0], temp->pos[1]);
				system("pause");
			}
			checker = false;
			free(nextNode);
			break;
		}
		temp = temp->next;
	}
	temp = openHead->next;
	while (temp)
	{
		if (temp->pos[0] == nextNode->pos[0] && temp->pos[1] == nextNode->pos[1])
		{
			if (Debug_mode)
			{
				printf("位于 %d,%d 的节点被证实冗余 不列入OPEN表\n", temp->pos[0], temp->pos[1]);
				system("pause");
			}
			checker = false;
			free(nextNode);
			break;
		}
		temp = temp->next;
	}
	if (checker)
	{
		nextNode->prev = openHead;
		nextNode->next = openHead->next;
		openHead->next->prev = nextNode;
		openHead->next = nextNode;
	}
	return checker;
}

void lookAhead(int **maze,  PNode curNode, PNode openHead, PNode closeHead)//从当前节点(curNode)分解子节点生成加入open表(openHead)
{
	int cur_x = curNode->pos[0];
	int cur_y = curNode->pos[1];
	printf("目前坐标位置:%d,%d\n", cur_x, cur_y);
	for (int i = cur_x - 1; i < cur_x + 2; i++)
	{
		for (int j = cur_y - 1; j < cur_y + 2; j++)
		{
			if (Debug_mode)
			{
				printf("目前搜索位置:%d,%d\n", i, j);
				system("pause");
			}
			if (openHead->next)
			{
				if ((i == cur_x || j == cur_y) && !(i == cur_x && j == cur_y) && maze[i][j] == ROUTE)
				{
					PNode nextNode = NodeOpenInit();
					nextNode->pos[0] = i;
					nextNode->pos[1] = j;
					nextNode->mht = Manhattan(i, j, tgt_x, tgt_y);
					bool check = node_to_open(openHead, closeHead, nextNode);
					if (check)
					{
						maze[i][j] = SRCH;
						if (Debug_mode)
						{
							system("cls");
							draw_map(maze);
							printf("探索发现空点\n");
							printf("目前坐标位置:%d,%d\n", cur_x, cur_y);
							printf("目前搜索位置:%d,%d\n", i, j);
							printf("目前OPEN表遍历结果:\n");
							TraverseList(openHead);
							system("pause");
						}
					}
				}
				else if ((i == cur_x || j == cur_y) && !(i == cur_x && j == cur_y) && maze[i][j] == EXIT)
				{
					system("cls");
					PNode nextNode = NodeOpenInit();
					nextNode->pos[0] = i;
					nextNode->pos[1] = j;
					nextNode->mht = Manhattan(i, j, tgt_x, tgt_y);
					maze[i][j] = CUR;
					draw_map(maze);
					printf("SOLVED\n");
					system("pause");
				}
			}
		}
	}
}

PNode openSort(int **maze, PNode openHead, PNode closeHead) //遍历open表返回mht最小节点 并且删除属于close的节点
{
	int mhtTemp = INT_MAX;
	PNode tempNodeS = NodeOpenInit();
	tempNodeS = openHead->next;
	PNode temp = NodeOpenInit();
	int cal = 0;
	while (tempNodeS)
	{
		cal++;
		if (cal > 50)//debug用
		{
			printf("卡死\n");
			printf("CurMht:%d\nCurPos:%d,%d\nopen:%d\nclose:%d\n\n", tempNodeS->mht, tempNodeS->pos[0], tempNodeS->pos[1], tempNodeS->open, tempNodeS->close);
			system("pause");
		}
		if (Debug_mode)
		{
			system("cls");
			printf("目前排序遍历节点:\n");
			printf("CurMht:%d\nCurPos:%d,%d\nopen:%d\nclose:%d\n\n", tempNodeS->mht, tempNodeS->pos[0], tempNodeS->pos[1], tempNodeS->open, tempNodeS->close);
		}
		if(tempNodeS->close == 1)//需要添加 目前有些close节点close为1但是没有被删除 2019年10月16日23点31分
		{
			if (tempNodeS->next)
			{
				PNode temp_c = NodeOpenInit();
				tempNodeS->next->prev = temp_c;
				temp_c->next = tempNodeS->next;
				open_to_close(tempNodeS, closeHead);
				tempNodeS = temp_c;
				tempNodeS = tempNodeS->next;
			}
			else
			{
				open_to_close(tempNodeS, closeHead);
				tempNodeS = NULL;
			}
		}
		else if (tempNodeS->mht < mhtTemp)
		{
			mhtTemp = tempNodeS->mht;
			temp = tempNodeS;
			tempNodeS = tempNodeS->next;
		}
		else
		{
			tempNodeS = tempNodeS->next;
		}
		
	}
	if (Debug_mode)
	{
		system("cls");
		draw_map(maze);
		printf("排序结束 目前openHead遍历结果:\n");
		TraverseList(openHead);
		printf("排序所得最佳OPEN节点:\n");
		printf("CurMht:%d\nCurPos:%d,%d\nopen:%d\nclose:%d\n\n", temp->mht, temp->pos[0], temp->pos[1], temp->open, temp->close);
		system("pause");
	}
	return temp;
}

void draw_map(int **maze)
{
	for (int i = 0; i < MapScale; i++) 
	{
		for (int j = 0; j < MapScale; j++) 
		{
			if (maze[i][j] == ROUTE)
			{
				printf("  ");
			}
			else if (maze[i][j] == WALL)
			{
				printf("■");
			}
			else if (maze[i][j] == START)
				printf("入");
			else if (maze[i][j] == SRCH)
				printf("□");
			else if (maze[i][j] == CUR)
				printf("⊙");
			else
				printf("出");
		}
		printf("\n");
	}
}

void draw_map_newAlg(int **maze)
{
	for (int i = 0; i < MapScale; i++)
	{
		for (int j = 0; j < MapScale; j++)
		{
			if (maze[i][j] == ROUTE)
			{
				printf("  ");
			}
			else if (maze[i][j] == WALL)
			{
				printf("■");
			}
			else if (maze[i][j] == START)
				printf("入");
			else if (maze[i][j] > 0)//说明已经赋值 为被探索过的节点
				printf("□");
			else if (maze[i][j] == CUR)
				printf("⊙");
			else
				printf("出");
		}
		printf("\n");
	}
}

void CreateMaze(int **maze, int x, int y) 
{
	maze[x][y] = ROUTE;
	int dig_dir[4][2] = { { 1,0 },{ -1,0 },{ 0,1 },{ 0,-1 } };//挖掘方向数组{x,y}
	for (int i = 0; i < 4; i++) //为每次递归随机化方向数组
	{
		int r = rand() % 4;//rand(0->3)
		int temp = dig_dir[0][0];
		dig_dir[0][0] = dig_dir[r][0];
		dig_dir[r][0] = temp;

		temp = dig_dir[0][1];
		dig_dir[0][1] = dig_dir[r][1];
		dig_dir[r][1] = temp;
	}

	for (int i = 0; i < 4; i++) 
	{
		int dx = x;
		int dy = y;
		int range = 1 + (rand() % Regularity);//由规整度决定每次挖掘距离
		while (range > 0) 
		{
			dx += dig_dir[i][0];
			dy += dig_dir[i][1];
			
			if (maze[dx][dy] == ROUTE) //如果已挖掘则跳出
			{
				break;
			}

			int break_check = 0;//遍历挖掘目标周围 如果会挖穿则跳出
			for (int j = dx - 1; j < dx + 2; j++) 
			{
				for (int k = dy - 1; k < dy + 2; k++) 
				{
					if (abs(j - dx) + abs(k - dy) == 1 && maze[j][k] == ROUTE)//只需判断四项 斜向不属于挖穿 
					{
						break_check++;//如果大于2 则说明除挖掘起点外存在挖穿点 跳出
					}
				}
			}
			if (break_check > 1) 
			{
				break;
			}
			range--;
			maze[dx][dy] = ROUTE;
			/*if (Debug_mode)
			{
				system("cls");//显示挖掘点坐标 挖掘方向 挖掘距离
				printf("dig pos: %d,%d\n", dx, dy);
				printf("dig dir:%d,%d\n", dig_dir[i][0], dig_dir[i][1]);
				printf("range:%d\n", range);
				draw_map(maze);
			}*/
		}

		if (range <= 0) //说明上循环自然退出 即目前点未挖穿 继续递归挖掘
		{
			CreateMaze(maze, dx, dy);
		}
	}
}

void SolveMaze(int **maze, int ent_x, int ent_y)
{
	PNode RootNode = NodeOpenInit();//初始化根节点
	RootNode->pos[0] = ent_x;
	RootNode->pos[1] = ent_y;
	RootNode->mht = Manhattan(ent_x, ent_y, tgt_x, tgt_y);
	PNode OpenHead = NodeOpenInit();
	PNode CloseHead = NodeOpenInit();
	CloseHead->open = false;
	CloseHead->close = true;
	OpenHead->mht = 1000;//用于断点观察
	OpenHead->next = RootNode;//头节点进入open表
	RootNode->prev = OpenHead;
	while (1)
	{
		PNode tempNode = NodeOpenInit();
		tempNode = openSort(maze, OpenHead, CloseHead);
		if (Debug_mode)
			system("cls");
		else
			gotoxy(0, 0);
		draw_map(maze);
		lookAhead(maze, tempNode, OpenHead, CloseHead);
		//tempNode处理 执行move指令 压栈存储路径
		if (Debug_mode)
		{
			printf("探索结束 目前所处节点MHT值:\n%d\n", tempNode->mht);
			system("pause");
		}
		maze[tempNode->pos[0]][tempNode->pos[1]] = CUR;
		open_to_close(tempNode, CloseHead);
	}
}

int dir_sel_to_map(int _sel_dir)//小车相对于自身移动方向
{
	int _map_dir = mapDir;//小车在地图上的方向
	int sel_dir = _sel_dir;
	int result_map_dir = (_map_dir + sel_dir) % 4;
	return result_map_dir;//小车相对于地图移动方向
}

int dir_map_to_sel(int _map_dir)//小车相对于地图移动方向
{
	int sel_map_dir = selDir;//小车在地图上的方向
	int map_dir = _map_dir;
	int result_sel_dir = (4 + map_dir - sel_map_dir) % 4;
	return result_sel_dir;//小车相对于自身移动方向
}

int posJudge(int cur_x, int cur_y, int nex_x, int nex_y)
{
	if (nex_x - cur_x == 1)
		return 1;
	else if (nex_x - cur_x == -1)
		return 3;
	else if (nex_y - cur_y == 1)
		return 2;//反写 因为数组y轴倒置
	else
		return 0;
}

int lookAhead_newAlg(int **timeMaze, int **maze, int cur_x, int cur_y)
{
	int temp = INT_MAX;
	int next_map_dir = -1;
	for(int i = cur_x-1; i < cur_x + 2; i++)
		for (int j = cur_y - 1; j < cur_y + 2; j++)
		{
			if (maze[i][j] == EXIT)
			{
				system("cls");
				draw_map_newAlg(maze);
				printf("SLOVED\n");
				Sleep(2000);
				system("pause");
			}
			if (((i == cur_x && j != cur_y) || (i != cur_x && j == cur_y)) && (maze[i][j] == ROUTE || maze[i][j] > 0))
			{
				
				int map_dir = posJudge(cur_y, cur_x, j, i);//反写 因为数组横纵相反
				int sel_dir = dir_map_to_sel(map_dir);
				maze[i][j] = Manhattan(i, j, tgt_x, tgt_y);
				if (Debug_mode)
				{
					printf("\n搜索节点:(%d,%d)\n", i, j);
					printf("搜索节点相对于地图方向:%d\n", map_dir);
					printf("搜索节点对于自身方向:%d\n", sel_dir);
					printf("自身相对地图方向:%d\n", selDir);
					printf("搜索节点曼哈顿权值:%d\n", maze[i][j]);
				}
				if (sel_dir == 1 || sel_dir == 3)
				{
					//if(timeMaze[i][j]<4)//重复次数过多则提倡转弯
						maze[i][j] += TURN;
					if(Debug_mode)
						printf("转弯 权值增加\n");
				}
				else if (sel_dir == 2)
				{
					maze[i][j] += REVERSE;
					if(Debug_mode)
						printf("回头 权值增加\n");
				}
				maze[i][j] += timeMaze[i][j];//修改在temp判断之前 不然不会被计入最低权值计算 2019年10月20日23点08分 
				if (maze[i][j] < temp)
				{
					temp = maze[i][j];
					next_map_dir = map_dir;
				}
				if (Debug_mode)
				{
					printf("增加重复拓展权值，最终权值:%d\n", maze[i][j]);
					printf("目前最低启发函数:%d\n\n", temp);
					system("pause");
				}
			}
		}
	int print_sel_dir = dir_map_to_sel(next_map_dir);

	printf("\n当前相对地图朝向:");
	switch (selDir)
	{
	case 0:
		printf("↑\n");
		break;
	case 1:
		printf("→\n");
		break;
	case 2:
		printf("↓\n");
		break;
	case 3:
		printf("←\n");
		break;
	}

	switch (print_sel_dir)
	{
	case 0:
		printf("直行\n");
		break;
	case 1:
		printf("右转\n");
		break;
	case 2:
		printf("回头\n");
		break;
	case 3:
		printf("左转\n");
		break;
	}
	selDir = next_map_dir;
	return next_map_dir;
}

void move_to(int map_dir, int **maze)
{
	if(Debug_mode)
		printf("before move_to (%d,%d)", curPos[0], curPos[1]);
	switch (map_dir)//反写 因为数组坐标横纵相反
	{
	case 0:
		curPos[0]--;
		break;
	case 1:
		curPos[1]++;
		break;
	case 2:
		curPos[0]++;
		break;
	case 3:
		curPos[1]--;
		break;
	}
	if (Debug_mode)
	{
		printf("move_to (%d,%d)", curPos[0], curPos[1]);
		system("pause");
	}
		
}

void SolveMaze_newAlg(int **maze, int ent_x, int ent_y)
{
	int **MazeTimeVal = (int**)malloc(MapScale * sizeof(int *));//用于记载该节点被经过次数
	for (int i = 0; i < MapScale; i++)
	{
		MazeTimeVal[i] = (int*)calloc(MapScale, sizeof(int));
	}
	for(int i = 0; i<MapScale;i++)//初始化
		for (int j = 0; j < MapScale; j++)
		{
			MazeTimeVal[i][j] = 0;
		}
	curPos[0] = ent_x;
	curPos[1] = ent_y;
	maze[ent_x][ent_y] = Manhattan(ent_x, ent_y, tgt_x, tgt_y);
	while (1)
	{
		if (Debug_mode)
			system("cls");
		else
			gotoxy(0, 0);
		draw_map_newAlg(maze);
		printf("\n节点当前坐标: (%d,%d)", curPos[0], curPos[1]);
		int next_map_dir = lookAhead_newAlg(MazeTimeVal, maze, curPos[0], curPos[1]);//返回接下来移动方向相对于地图值
		/*
		gotoxy(curPos[1], curPos[0]);
		switch (selDir)
		{
		case 0:
			printf("↑");
			break;
		case 1:
			printf("→");
			break;
		case 2:
			printf("↓");
			break;
		case 3:
			printf("←");
			break;
		}
		gotoxy(MapScale + 1, 1);
		*/
		printf("\n目前节点相对地图移动方向:%d", selDir);
		move_to(next_map_dir, maze);//改变curPos到下一节点
		printf("\n移动后坐标: (%d,%d)", curPos[0], curPos[1]);
		MazeTimeVal[curPos[0]][curPos[1]] += RE_STEP;
		if (Debug_mode)//如果不想连续解则可以注释掉
			system("pause");
	}
}

int main(void) {
	while (1)
	{
		printf("Input regularity:\n");
		scanf_s("%d", &Regularity);
		printf("Input maze scale:\n");
		scanf_s("%d", &MapScale);
		printf("Input entrance y coordinate:(bigger than 2)\n");
		int ent_y = 2;
		scanf_s("%d", &ent_y);
		solveMem();
		printf("Debug Mode on?\n");
		if (check())
			Debug_mode = true;
		solveMem();
		system("cls");
		srand((unsigned)time(NULL));//srand设置rand()种子 由time提供
		int **Maze = (int**)malloc(MapScale * sizeof(int *));
		for (int i = 0; i < MapScale; i++) 
		{
			Maze[i] = (int*)calloc(MapScale, sizeof(int));
		}

		for (int i = 0; i < MapScale; i++) //绘制外延空白
		{
			Maze[i][0] = ROUTE;
			Maze[0][i] = ROUTE;
			Maze[i][MapScale - 1] = ROUTE;
			Maze[MapScale - 1][i] = ROUTE;
		}

		CreateMaze(Maze, ent_y, 2);//绘制起点 非实际起点
		system("cls");

		Maze[ent_y][1] = START;

		//在最右侧寻找出口
		int exit_y = 0;
		for (exit_y = MapScale - 3; exit_y >= 0; exit_y--) 
		{
			if (Maze[exit_y][MapScale - 3] == ROUTE) 
			{
				Maze[exit_y][MapScale - 2] = EXIT;
				tgt_x = exit_y;
				tgt_y = MapScale - 2;
				break;
			}
		}

		//画迷宫
		draw_map(Maze);
		printf("Solve with A* ?\n");
		if (check())
		{
			system("cls");
			SolveMaze(Maze, ent_y, 2);
		}
		else
		{
			solveMem();
			printf("Solve with newAlg ?\n");
			if (check())
			{
				system("cls");
				SolveMaze_newAlg(Maze, ent_y, 2);
			}
		}
		for (int i = 0; i < MapScale; i++) free(Maze[i]);
		free(Maze);//释放
		system("cls");
	}
	return 0;
}

//*******************************搭建日志*******************************//
//搭建迷宫生成函数↓
//迷宫生成完成 加入了规整度调整与规模调整 入口纵坐标可设置
//加入debug模式判断
//2019年10月14日22点23分
//
//完善搜索迷宫框架↓
//指针有问题 OPEN表删除错误 Sort后只剩下一处临时节点
//重写Sort函数 去除了其中搜索close=1清除的指令 用open_to_close()函数解决
//LookAhead会留下close=1节点 导致在旧节点死循环
//恢复了先前删除的指令 在Sort中进行搜索删除 解决
//2019年10月16日23点31分
//多处指针操作改错
//规模10~15大多可得解 规模20有空指针问题
//2019年10月17日00点32分
//
//分析全部节点操作 空指针现象属列表中添加节点时的拼接问题 解决
//已证实规模30迷宫得解 无空指针问题
//搜索框架搭建完毕
//修正了未扩展节点和已扩展节点的显示字符
//2019年10月17日08点00分
//
//创建小车绝对方向判断与转向操作相关函数↓
//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//考虑到压栈 新结构体 轨迹退回以及其所产生的多余移动步数和低效率 放弃当前A*
//
//编写newAlg 优化A*
//编写相对、绝对方向转换函数
//编写新探索函数lookAhead_newAlg()
//编写新Solve_newAlg()函数
//只能搜索两格进入死循环 待调试
//2019年10月18日11点56分
//
//修改了相对绝对方向表达方式 修改了其函数错误
//经常陷入两点权值循环 修改了转弯与后退增加权值
//编写大量debug文本
//修改权重赋值方式
//无改善 依旧死循环
//发现重复扩展发生于长距离重复循环 建立新数组用于储存重复拓展次数用于增加权值
//规模20迷宫得解
//规模30迷宫死循环
//事实证明规模20成功几率只有一半 还是会卡死
//规模10没问题（
//啊规模10也会卡死
//完蛋
//2019年10月18日16点28分
//
//发现原因
//重复拓展次数没有被纳入临时最低权值判断循环
//修改完成 规模20迷宫得解
//规模30迷宫得解
//初步判断算法正确
//至此 A*算法以及优化A*算法设计完成 
//迷宫构建 迷宫搜索得解 功能实现
//2019年10月20日23点11分
//增加了每一步相对方向与绝对方向显示
//最后修改完成
//2019年10月21日02点57分