#include<iostream>
#include<chrono>

#include<termios.h>
#include<unistd.h>
#include<fcntl.h>

using namespace std;

const int Height = 27;
const int Wide = 60;

const int dir[4][2] = {
    {-1,0}, //上
    {0,1},  //右
    {1,0},  //下
    {0,-1}  //左
};

// 设置终端为非阻塞模式
void setNonBlockingMode() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~(ICANON | ECHO);  // 禁用规范模式和回显
    ttystate.c_cc[VMIN] = 0;               // 最小字符数
    ttystate.c_cc[VTIME] = 0;              // 超时时间
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    
    // 设置非阻塞
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

// 恢复终端设置
void restoreTerminalMode() {
    struct termios ttystate;
    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

//获取按键
char getKey() {
    char ch =0;
    if(read(STDIN_FILENO,&ch,1) > 0) {
        return ch;
    }
    return 0; //没有按键
}


void hideCursor() {
    cout<<"\033[?25l";
}

enum BlockType {            //用枚举类型来明确的表示每个格子的状态
    EMPTY = 0,
    FOOD = 1
};

struct Map {
    BlockType data[Height][Wide];
    bool hasFood;
};

struct Pos {
    int x;
    int y;
};

struct Snake {                      //把蛇表示在一个一位数据里面
    Pos snake[Height * Wide];
    int snakeDir;
    int snakeLength;
    int lastMoveTime;
    int moveFrequency;
};

void initSnake (Snake* snk) {
    snk ->snakeLength = 1;
    snk ->snakeDir = 1;
    snk ->snake[0] = {Wide/2,Height/2};         //注意Wight和x对应，Height和y对应
    snk ->lastMoveTime = 0;
    snk ->moveFrequency = 200;

}
/*          以下是Windows中的写法：
void drawUnit(Pos p,const char unit[]) {
    COORD coord;
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    coord.X = p.x + 1;
    coord.Y = p.y + 1;
    SetConsoleCursorPosition(hOutput,coord);
    cout<<unit;
}
    */
   
void drawUnit(Pos p,const char unit[]) {
    cout<<"\033["<<(p.y + 1) <<";"<<(p.x +1) << "H";        //用ANSI把光标调过去
    cout<<unit;                     
    cout<<flush;            //这个可选，刷新一下缓冲区

}

void drawSnake(Snake* snk) {
    for(int i=0;i<snk->snakeLength;i++) {
        drawUnit(snk ->snake[i],"\u25A0");
    }
}

bool checkOutOfBound(Pos p ) {              //出界了就返回true
    if(p.x == 0 || p.x == Wide +1) {
        return true;
    }
    if(p.y == 0 || p.y == Height +1) {
        return true;
    }
    return false;
}

void drawMap(Map* map) {                //用来把data中存储的地图信息打印出来
    system("clear");        //Windows中用cls，MacOS中用clear
    cout<<"\u250C";
    for(int i=0;i<Wide;i++) {
        cout<<"\u2500";
    }
    cout<<"\u2510"<<endl;
    //
    for(int i=0;i<Height;i++) {
        cout<<"\u2502";
        for(int j=0;j<Wide;j++) {
            if(map -> data[i][j] == 0) {
                cout<<" ";
            }

        }
        cout<<"\u2502"<<endl;
    }
    //
    cout<<"\u2514";
    for(int i=0;i<Wide;i++) {
        cout<<"\u2500";
    }
    cout<<"\u2518"<<endl;
}

void initMap(Map* map) {                //用来初始化一个全空的地图
    for(int y=0;y<Height;y++) {
        for(int x=0;x<Wide;x++) {
            map ->data[y][x] = BlockType::EMPTY;        //其实等价于0
        }
    }
    map -> hasFood = false;
}

void moveSnake(Snake* snk) {
    for(int i = snk->snakeLength -1;i>=1;i--) {
        snk ->snake[i] = snk -> snake[i - 1];       //在数组的头插入一个元素，把所有的后面的元素都往后移动一个元素
    }
    snk ->snake[0].y += dir[snk->snakeDir][0];
    snk ->snake[0].x += dir[snk->snakeDir][1];
}
bool doMove(Snake* snk,Map* map) {
    Pos tail = snk ->snake[snk->snakeLength - 1];       //蛇尾的位置
    drawUnit(tail," ");             //把蛇尾擦掉
    moveSnake(snk);
    if(checkOutOfBound(snk->snake[0])){ //确保不要超出边界
        return false;
    }
    drawUnit(snk->snake[0],"\u25A0");//画出蛇头
    return true;
}
bool checkSnakeMove(Snake* snk,Map* map) {
    auto now = std::chrono::steady_clock::now();    //这里用了Cpp11的获取时间的方法，是跨平台的，比较友好
    auto duration = now.time_since_epoch();
    auto curTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    if (curTime - snk->lastMoveTime > snk->moveFrequency) {
        if(false == doMove(snk,map)){
            return false;
        }
        snk->lastMoveTime = curTime;
    }
    return true;
}
void checkChangeDir(Snake* snk) {
    char key = getKey();
    if(key != 0) {
        switch(key) {
            case 'w':
                if(snk ->snakeDir != 2)
                snk ->snakeDir = 0;
                break;
            case 'd':
                if(snk ->snakeDir != 3)            
                snk ->snakeDir = 1;
                break;
            case 's':
                if(snk ->snakeDir != 0)
                snk ->snakeDir = 2;
                break;
            case 'a':
                if(snk ->snakeDir != 1)
                snk ->snakeDir = 3;
                break;
            default:
                break;
        }
    }
}
//检测食物生成
void checkFoodGenerate(Snake* snk,Map* map) {
    if(false == map->hasFood) {
        while(1) {
            int x = rand() % Wide-1;
            x++;
            int y = rand() % Height-1;
            y++;
            int i=0;
            while(i < snk->snakeLength ) {
                if(x == snk->snake[i].x && y == snk->snake[i].y) {
                    break;
                }
                i++;
            }
            if(i == snk->snakeLength) {
                map ->data[y][x] = BlockType::FOOD;
                map ->hasFood = true;
                drawUnit({x,y},"\u25CF");
                return;
            }
        }
    }
}
//检测是否吃
void checkEatFood(Snake* snk,Pos tail,Map* map) {
    Pos head = snk->snake[0];
    if(map->data[head.y][head.x] == BlockType::FOOD) {
        snk->snake[snk->snakeLength++] =tail;
        map->data[head.y][head.x] =BlockType::EMPTY;
        map->hasFood =false;
        drawUnit(tail,"\u25A0");
    }
}
int main () {
    Map map;
    Snake snk;
    setNonBlockingMode();  //设置非阻塞模式
    initSnake(&snk);
    hideCursor();
    initMap(&map);
    drawMap(&map);
    drawSnake(&snk);
    while(1){
        checkChangeDir(&snk);
        if(false == checkSnakeMove(&snk,&map)){
            break;
        }
        checkFoodGenerate(&snk,&map);
        checkEatFood(&snk,snk.snake[snk.snakeLength-1],&map);
    }
    drawUnit({Wide/2-4,Height /2},"游戏结束！");
    restoreTerminalMode();  //恢复终端设置
    while(1){}
    return 0;
}