#include<stdio.h>
#include<stdlib.h>
#include<windows.h>
#include<conio.h>
#include<time.h>
#include<math.h>
#define LINE 20 //游戏区行数
#define COLUMN 30 //游戏区列数
/*定义按键*/
#define UP 72 //方向键：上
#define DOWN 80 //方向键：下
#define LEFT 75 //方向键：左
#define RIGHT 77 //方向键：右
#define SPACE 32 //暂停
#define ESC 27 //退出
/*自定义一些状态*/
#define ER_GAMEOVER 0xff//游戏正常结束
#define ER_INIT_IN 0x21 //在init函数发生异常退出
#define ER_SHOW_BEFORE 0x30//在writesnake出错
#define ER_SHOW_IN 0x31 //在show函数发生异常退出
#define ER_SHOW_ODD 0x32//在show函数发现未知标记
#define ER_CHANGE_IN 0x33//在showchange函数发现未知标记
#define ER_MOVE_IN 0x41 //在move函数发生异常退出
#define ER_MOVE_ODD 0x42//在move函数发现未知标记
#define ER_MOVE_BODYHALF 0x43//在move函数发生未知按键
#define ER_RANDFOOD_IN 0x51//在randfood发生异常
#define ER_RANDFOOD_FULL 0x52//在randfood发现屏幕已满
#define ER_ACTIVE_IN 0x61//在在active函数发生异常退出
#define ER_GAMEOVER_WALL 0x91//撞墙死亡
#define ER_GAMEOVER_BODY 0x92//自锁死亡
#define ER_GWX_STUP 0xF //发生其他GWX没预料到的情况
/*定义输出状态*/
const char KONG = ' '; //标记空
const char WALL = '#'; //标记墙
const char FOOD = '*'; //标记食物
const char HEAD = '@'; //标记蛇头
const char BODY = 'o'; //标记蛇身
/*一些全局变量，后期可封装到class中*/
char ScreenTmp[COLUMN + 1][LINE + 1]; //游戏区为(1,1)~(LINE,C)
char ScreenTmp2[COLUMN * 2 + 2][LINE + 1];
int len = 2;     //蛇身长
int speed = 350; //速度
int circle = 0;  //时间帧率
char ch = 0;     //按键信息
int err = 0;     //游戏报错信息

/*一些函数*/
int DataInit();         //数据初始化
int ScreenInit();       //图形显示初始化
int MoveAndJudge(char); //移动与判断
int ShowChange();       //快速打印图形
int ActiveMode(int*);   //动态图形显示
int GameOver(int);      //游戏结束界面

struct position {       //二维位置变量
	int y;
	int x;
} food, snake[COLUMN * LINE] = {};

struct UseFile {        //文件读写
	int S_time;
	int S_len=0;
	int FileRead()
	{
		FILE * fp = fopen("GwxSnake.dat", "r");
		fscanf(fp, "%d%d", &S_time, &S_len);
		fclose(fp);
		return S_len;
	}
	void FileWrite()
	{
		FILE * fp = fopen("GwxSnake.dat", "w");
		fprintf(fp, "%d %d", S_time, S_len);
		fclose(fp);
	}
}Usefile;

struct WIN32APIs { //封装后的WIn32API,导致程序不能跨平台的罪魁祸首
	
	void CoordHide(void)
	{
		CONSOLE_CURSOR_INFO curInfo; //定义光标信息的结构体变量
		curInfo.dwSize = 1; //如果没赋值的话，光标隐藏无效
		curInfo.bVisible = FALSE; //将光标设置为不可见
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE); //获取控制台句柄
		SetConsoleCursorInfo(handle, &curInfo); //设置光标信息
	}
	void CoordJmp(int i, int j)
	{
		COORD pos; //定义光标位置的结构体变量
		pos.X = i;
		pos.Y = j;
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE); //获取控制台句柄
		SetConsoleCursorPosition(handle, pos); //设置光标位置
	}
	void DrawChar(int i, int j, char c)
	{
		CoordJmp(i, j);
		printf("%c", c);
	}
	void DrawInt(int i, int j, int d)
	{
		CoordJmp(i, j);
		printf("%d", d);
	}
	void DrawTxt(int i, int j, const char* s)
	{
		CoordJmp(i, j);
		printf("%s", s);
	}
	bool MusicYes=0;
	void MusicBeep(unsigned int a,unsigned int b)
	{
		Beep(a,b);
	}
} Win32api;

int main()
{
	srand((unsigned int)time(NULL));//初始化随机数种子
	if(!Usefile.FileRead())
	{
		printf("未发现原有游戏记录，按任意键进行初始化\n");
		_getch();
	}
	if (!err)err = DataInit();
	else return 0;
	if (!err)err = ScreenInit();
	else return 0;
	if (!err)err = ActiveMode(&speed);
	else return 0;
	GameOver(err);
	getchar();
}

int RandFood()//二级函数-随机生成食物
{
	int status = ER_RANDFOOD_IN;
	if (len > (COLUMN - 2) * (LINE - 2))return ER_RANDFOOD_FULL;
	int i, j;
	do {
		i = rand() % (LINE - 1) + 1;
		j = rand() % (COLUMN - 1) + 1;
	} while (ScreenTmp[i][j] != KONG); //确保生成食物的位置为空
	
	food.x = i;
	food.y = j;
	ScreenTmp[food.y][food.x] = FOOD;
	
	status = 0;
	return status;
}

int MoveAndJudge(char ch)
{
	int status = ER_MOVE_IN;
	/*设置移动方向*/
	int x = 0, y = 0;
	switch (ch) {
	case UP:
		x = -1;
		break;
	case DOWN:
		x = 1;
		break;
	case LEFT:
		y = -1;
		break;
	case RIGHT:
		y = 1;
		break;
	case SPACE:
		break;
		default:
			return ER_MOVE_ODD;
			break;
	}
	
	if (x + y) { //判断是否移动
		
		if (speed) speed = 340 - 4.0 * sqrt(2 * circle) - 100 * len / ((LINE - 2) * (COLUMN - 2)); //调节速度
		
		if (ScreenTmp[snake[0].y + y][snake[0].x + x] == KONG || ((snake[0].y + y) == snake[len].y && (snake[0].x + x) == snake[len].x)) { //前方为空或正好蛇尾可移的情况
			//变化的部分图形
			ScreenTmp[snake[len].y][snake[len].x] = KONG;
			ScreenTmp[snake[0].y][snake[0].x] = BODY;
			//变更蛇身
			for (int i = len; i > 0; i--)
				snake[i] = snake[i - 1];
			//变更蛇头
			snake[0].x += x;
			snake[0].y += y;
			ScreenTmp[snake[0].y][snake[0].x] = HEAD;
		} else if (ScreenTmp[snake[0].y + y][snake[0].x + x] == FOOD) { //吃到食物的情况
			//蛇长增加
			len++;
			//变更蛇身
			for (int i = len; i > 0; i--)
				snake[i] = snake[i - 1];
			//变更蛇头
			ScreenTmp[snake[0].y][snake[0].x] = BODY;
			snake[0].x += x;
			snake[0].y += y;
			ScreenTmp[snake[0].y][snake[0].x] = HEAD;
			//刷新食物
			if(RandFood())return ER_RANDFOOD_FULL;
			//MusicYes
			Win32api.MusicYes=true;
		} else if (ScreenTmp[snake[0].y + y][snake[0].x + x] == WALL) { //撞墙
			return ER_GAMEOVER_WALL;
		} else if (ScreenTmp[snake[0].y + y][snake[0].x + x] == BODY) { //自锁
			return ER_GAMEOVER_BODY;
		} else return ER_GWX_STUP;
	}
	
	else return 0;
	status = 0;
	return status;
}

int ShowChange()
{
	int status= ER_CHANGE_IN;
	for (int j = 0; j <= LINE; j++) {
		for (int i = 0; i <= COLUMN; i++) {
			if (ScreenTmp2[i * 2][j] != ScreenTmp[i][j]) {
				ScreenTmp2[i * 2][j] = ScreenTmp[i][j];
				Win32api.DrawChar(i * 2, j, ScreenTmp[i][j]);
			}
		}
	}
	if(Win32api.MusicYes)//发出声音
	{
		Win32api.MusicBeep(800,200);
		Win32api.MusicYes=false;
	}
	Win32api.DrawInt(9, LINE + 1, circle);
	Win32api.DrawInt(9, LINE + 2, len);
	Win32api.DrawInt(9, LINE + 3, 400 - speed);
	status=0;
	return status;
}

void GetCh()//动态模式中用于获取键盘输入
{
	char tmp = _getch();
	switch (tmp) { //防止按下相反按键导致的结束
	case UP:
		if (ch != DOWN)ch = tmp;
		Win32api.DrawTxt(9, LINE + 4, "向上");
		break;
	case DOWN:
		if (ch != UP)ch = tmp;
		Win32api.DrawTxt(9, LINE + 4, "向下");
		break;
	case LEFT:
		if (ch != RIGHT)ch = tmp;
		Win32api.DrawTxt(9, LINE + 4, "向左");
		break;
	case RIGHT:
		if (ch != LEFT)ch = tmp;
		Win32api.DrawTxt(9, LINE + 4, "向右");
		break;
	case SPACE:
		ch = SPACE;
		Win32api.DrawTxt(9, LINE + 4, "暂停");
		break;
		default://解决方向键码导致的BUG-0x42
		break;
	}
	return;
}

int ActiveMode(int* act)
{
	int status = ER_ACTIVE_IN;
	if (*act) //激活动态模式
		while (1) {
		
		if (!_kbhit()) { //_kbhit()其实在conio.h中
			
			err = MoveAndJudge(ch);
			if (err == ER_GAMEOVER_WALL) return ER_GAMEOVER_WALL;
			if (err == ER_GAMEOVER_BODY) return ER_GAMEOVER_BODY;
			if (err == ER_RANDFOOD_FULL) return ER_RANDFOOD_FULL;
			else err = ShowChange();
			Sleep(*act);//防止CPU占用过高，看详细解释
			if (ch != SPACE)circle++;
			
		} else {
			GetCh();
			//circles-=circles%10;
			/*
			  这些代码被注释掉为是防止按键瞬间蛇移动过快
			  err=Move(ch);
			  if(err==ER_GAMEOVER)return err;
			  else err=Show();
			 */
			
		}
	} else //静态模式
		while (1) {
		ch = _getch();
		err = MoveAndJudge(ch);
		if (err == ER_GAMEOVER)return err;
		else err = ShowChange();
		circle++;
	}
	status = 0;
	return status;
}

int GameOver(int err)
{
	system("cls");
	printf("\n\n\n\n");
	printf("  |    GAME OVER         Status %#x    |\n\n", err);
	if (err == ER_GAMEOVER_BODY)      printf("    * 哎呀，饥不择食地把自己吃掉了呢 *\n");
	else if (err == ER_GAMEOVER_WALL) printf("    * 醒醒吧，你还没有练就铁头功吶 *\n");
	else if (err == ER_RANDFOOD_FULL) printf("    * 不可能吧，你这么厉害的嘛！！ *\n");
	else                              printf("    * 你是如何导致游戏结束的呢？？ *\n");
	printf("\n  >  您的此次长度为 %3d , 时间帧率 %-5d\n", len,circle);
	printf("\n  >      历史最长为 %3d , 时间帧率 %-5d\n",Usefile.S_len,Usefile.S_time);
	if(len>Usefile.S_len)
	{
		Usefile.S_len=len;
		Usefile.S_time=circle;
		Usefile.FileWrite();
	}
	return 0;
}


int DataInit()//数据初始化
{
	int status = ER_INIT_IN;
	
	/*部分-空*/
	for (int i = 0; i <= LINE; i++) //将两个ScreenTmp初始化为空
		for (int j = 0; j <= COLUMN; j++) {
		ScreenTmp2[j * 2][i] = KONG;
		ScreenTmp2[j * 2 + 1][i] = KONG;
		ScreenTmp[j][i] = KONG;
	}
	/*部分-墙*/
	for (int i = 0; i <= LINE; i++) {
		ScreenTmp[0][i] = WALL;
		ScreenTmp[COLUMN][i] = WALL;
	}
	for (int i = 1; i < COLUMN; i++) {
		ScreenTmp[i][0] = WALL;
		ScreenTmp[i][LINE] = WALL;
	}
	/*部分-蛇*/
	snake[0].x = rand() % (LINE - 2) + 1; //头向右，保证尾巴不越界
	snake[0].y = rand() % (COLUMN - len - 1) + len + 1;
	ScreenTmp[snake[0].y][snake[0].x] = HEAD;
	for (int i = 1; i <= len; i++) { //身体
		snake[i].x = snake[i - 1].x;
		snake[i].y = snake[i - 1].y - 1;
		ScreenTmp[snake[i].y][snake[i].x] = BODY;
	}
	/*部分-食物*/
	do {
		food.x = rand() % (LINE - 1) + 1;
		food.y = rand() % (COLUMN - 1) + 1;
	} while (ScreenTmp[food.y][food.x] != KONG); //确保生成食物的位置为空
	ScreenTmp[food.y][food.x] = FOOD;
	/*完成*/
	status = 0;
	return status;//执行出错为非0
}

int ScreenInit()//图形显示初始化
{
	int status = ER_SHOW_IN;
	/*窗口部分*/
	system("title 贪吃蛇-计算机导论作业用"); //设置窗口名字
	system("mode con cols=62 lines=26"); //设置窗口的大小
	system("color 0a");
	system("cls");
	/*隐藏光标*/
	Win32api.CoordHide();
	/*初始化显示框架*/
	for (int j = 0; j <= LINE; j++) {
		for (int i = 0; i <= COLUMN; i++)
			printf("%c ", ScreenTmp[i][j]);
		printf("\n");
	}
	printf("时间帧率 0\n");// circle);
	printf("蛇身长度 0\n"); //len);
	printf("相对速度 0\n"); //speed);
	printf("移动方向 0\n");
	status = 0;
	return status;//执行出错为非0
}
