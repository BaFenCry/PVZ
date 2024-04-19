/*
* 开发日志：
* 1.创建新项目（空项目模板）vs环境
* 2.导入素材
* 3.实现游戏界面
* 4. 实现游戏顶部工具栏
* 5. 实现工具栏中的植物卡牌
*/

#include <stdio.h>
#include <math.h>
#include <graphics.h>  //EasyX图形库
#include "tools.h"
#include <time.h>
#include "vector2.h"
#include <mmsystem.h>
#include <map>

#pragma comment(lib,"winmm.lib")

#define WIN_WIDTH 900
#define WIN_HEIGHT 600 
#define ZM_MAX 2

enum { WAN_DOU, XIANG_RI_KUI, SHI_REN_HUA, ZHI_WU_COUNT };
std::map<int, int> needSunshine;

enum { GOING, WIN, FAIL }; //
int killCount;  //殺掉僵尸數量
int zmCount;  //已經出現僵尸數量
int gameStatus;
bool isSelected;

IMAGE imgBg;
IMAGE imgBar;
IMAGE imgCards[ZHI_WU_COUNT];
IMAGE* imgZhiwu[ZHI_WU_COUNT][20];
IMAGE imgSun[29];
IMAGE imgChanzi;
IMAGE imgChomperAttack;
IMAGE imgChomperDigest;
int sunshine;

int curX, curY; //当前选中植物在移动过程中的位置
int curZhiwu; // 当前选择的植物，0表示没有选择
struct zhiwu {
	int type;  //植物类型,0表示没有植物
	int frameIndex; // 序列帧序号
	int blood;
	bool catched; //是否被僵尸捕获
	int deadTimer;
	int timer; //阳光洒落计时器
	int x, y;
	int shootTimer;
};
struct zhiwu map[3][9]; //草坪地图

enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT };

struct sunshineBall {
	int x, y; //阳光位置
	int frameIndex; //帧序列号
	int endy; //阳光飘落的目标位置
	bool used; //当前是否在使用该阳光
	int timer;
	float xoff, yoff;
	float t; //贝赛尔曲线时间点。 0....1;
	vector2 p1, p2, p3, p4;
	vector2 pCur; //当前时刻阳光位置
	float speed;
	int status;
};
// 阳光池，用来存放阳光
struct sunshineBall balls[10];

struct zm {
	int x, y, row;
	int frameIndex;
	bool used;
	int speed;
	int blood;
	bool dead;
	int damage;
	bool eating;//是否在吃东西
};
IMAGE imgZM[22];
struct zm zms[15];
IMAGE imgZmDead[20];
IMAGE imgZmEat[21];
IMAGE imgZmStand[11];

struct bullet {
	int x, y, row;
	bool used;
	int speed;
	bool blast; //子弹是否发生爆炸
	int frameIndex;
	int damage;
};
struct bullet bullets[40];
IMAGE imgBulletNormal;
IMAGE imgBulletBlast[6];


bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp == NULL) return false;
	else {
		fclose(fp);
		return true;
	}
}

void gameInit() {
	//加载游戏背景
	//把字符集改成“多字节字符集”
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar5.png");
	//memset(imgZhiwu, 0, sizeof(imgZhiwu));
	memset(map, 0, sizeof(map));

	curZhiwu = 0;
	sunshine = 150;
	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;
	isSelected = false;

	//初始化植物所需阳光数目
	needSunshine[WAN_DOU] = 100;
	needSunshine[XIANG_RI_KUI] = 50;
	needSunshine[SHI_REN_HUA] = 150;


	//初始化植物卡牌
	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; ++i) {
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);

		for (int j = 0; j < 20; ++j) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//判断文件是否存在
			if (fileExist(name)) {
				imgZhiwu[i][j] = new IMAGE;
				loadimage(imgZhiwu[i][j], name);
			}
			else {
				break;
			}
		}
	}
	memset(balls, 0, sizeof(balls)); //初始化阳光
	for (int i = 0; i < 29; ++i) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSun[i], name);
	}
	memset(zms, 0, sizeof(zms)); //僵尸初始化
	for (int i = 0; i < 22; ++i) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i + 1);
		loadimage(&imgZM[i], name);
	}

	memset(bullets, 0, sizeof(bullets));
	sprintf_s(name, sizeof(name), "res/bullets/bullet_normal.png");
	loadimage(&imgBulletNormal, name);
	loadimage(&imgBulletBlast[5], "res/bullets/bullet_blast.png");
	int w = imgBulletBlast[5].getwidth(), h = imgBulletBlast[5].getheight();
	for (int i = 0; i < 5; ++i) {
		float k = (i + 1) * 0.17;
		loadimage(&imgBulletBlast[i], "res/bullets/bullet_blast.png", w * k, h * k, true);
	}
	for (int i = 0; i < 20; ++i) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZmDead[i], name);
	}
	for (int i = 0; i < 21; ++i) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZmEat[i], name);
	}
	for (int i = 0; i < 11; ++i) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}
	loadimage(&imgChanzi, "res/chanzi.png");
	loadimage(&imgChomperAttack, "res/Plants/Chomper/ChomperAttack.gif");
	loadimage(&imgChomperDigest, "res/Plants/Chomper/ChomperDigest.gif");
	//预加载音频文件
	//mciSendString("open res/audio/plant2.mp3 alias plant", NULL, 0, 0);
	mciSendString("open res/audio/Look_up_at_the_Sky.mp3 alias mySound", NULL, 0, 0);
	mciSendString("open res/audio/Faster.mp3 alias Faster", NULL, 0, 0);
	//配置随机种子
	srand(time(NULL));
	//创建游戏窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT, 1);
	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black");
	f.lfQuality = ANTIALIASED_QUALITY;
	settextstyle(&f);
	setbkmode(TRANSPARENT);  //透明
	setcolor(BLACK); //颜色
}

void drawSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; ++i) {
		//if (balls[i].used || balls[i].xoff) {
		if (balls[i].used) {
			IMAGE* img = &imgSun[balls[i].frameIndex];
			//putimagePNG(balls[i].x, balls[i].y, img);
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}
}

void drawZm() {
	int zmcount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmcount; ++i) {
		if (zms[i].used) {
			IMAGE* img = NULL;
			if (zms[i].eating) {
				img = &imgZmEat[zms[i].frameIndex];
			}
			else if (zms[i].dead) {
				img = &imgZmDead[zms[i].frameIndex];
			}
			else {
				img = &imgZM[zms[i].frameIndex];
			}
			putimagePNG(zms[i].x, zms[i].y, img);
		}
	}
}

void drawBullet() {
	int count = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < count; ++i) {
		IMAGE* img;
		if (bullets[i].used) {
			if (bullets[i].blast) img = &imgBulletBlast[bullets[i].frameIndex];
			else img = &imgBulletNormal;
			putimagePNG(bullets[i].x, bullets[i].y, img);
		}
	}
}

void drawCards() {
	for (int i = 0; i < ZHI_WU_COUNT; ++i) {
		int x = 338 + i * 64;
		putimagePNG(x, 6, &imgCards[i]);
	}
}

void drawPlant() {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 9; ++j) {
			if (map[i][j].type > 0) {
				//int x = 250 + j * 82.5;
				//int y = 175 + i * 105+10;
				int zhiWuType = map[i][j].type;
				int index_frame = map[i][j].frameIndex;
				//putimagePNG(x, y, imgZhiwu[zhiWuType - 1][index_frame]);
				putimagePNG(map[i][j].x, map[i][j].y, imgZhiwu[zhiWuType - 1][index_frame]);
			}
		}
	}
	//渲染拖动植物
	if (curZhiwu) {
		IMAGE* img = imgZhiwu[curZhiwu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}

void drawScore() {
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(278, 67, scoreText);
}

void drawChanzi() {
	if (isSelected) {
		IMAGE* img = &imgChanzi;
		putimagePNG(curX - img->getheight() / 2, curY - img->getheight() / 2, img);
	}
}

void updateWindow() {
	BeginBatchDraw();//开始缓冲
	putimagePNG(-112, 0, &imgBg);
	putimagePNG(250, 0, &imgBar);
	putimagePNG(150, 12, &imgChanzi);
	drawCards();
	drawPlant();
	drawZm();
	drawSunshine();
	drawBullet();
	drawScore();
	drawChanzi();
	EndBatchDraw(); //结束双缓冲
}

void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int width = imgSun[0].getwidth(), height = imgSun[0].getheight();
	for (int i = 0; i < count; ++i) {
		if (balls[i].used) {
			//int x = balls[i].x, y = balls[i].y;
			int x = balls[i].pCur.x, y = balls[i].pCur.y;
			if (msg->x > x && msg->x<x + width && msg->y>y && msg->y < y + height) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//mciSendString("play res/sunshine.mp3",0,0,0);
				PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);
				//设置阳光偏移量
				//float destY = 0, destX = 262;
				//float angle = atan((y - destY) / (x - destX));
				//balls[i].xoff = 4 * cos(angle);
				//balls[i].yoff = 4 * sin(angle);
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 8;
				balls[i].speed = 1.0 / (distance / off);
				break;
			}
		}
	}
}

void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {  //监听消息
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 338 && msg.x < 338 + 64 * ZHI_WU_COUNT && msg.y >= 6 && msg.y <= 96) {
				int index = (msg.x - 338) / 64;
				/*printf("%d\n", index);*/
				status = 1;
				if (sunshine >= needSunshine[index]) curZhiwu = index + 1;
				if (isSelected) isSelected = false;
			}
			else if (msg.x > 150 && msg.x < 230 && msg.y>12 && msg.y < 92) {
				isSelected = !isSelected;
			}
			else {
				if (isSelected) {
					if (msg.x >= 250 - 112 && msg.x <= 990 - 112 && msg.y >= 175 && msg.y <= 490) {
						int row = (msg.y - 175) / 105;
						int col = (msg.x - 250 + 112) / 82.5;
						printf("%d  %d\n", row, col);
						if (map[row][col].type != 0) {
							PlaySound("res/plant.wav", NULL, SND_FILENAME | SND_ASYNC);
							sunshine += needSunshine[map[row][col].type - 1] / 2;
							map[row][col].type = 0;
							printf("%d %d\n", needSunshine[map[row][col].type - 1] / 2, sunshine);
							map[row][col].frameIndex = 0;
							map[row][col].blood = 0;
							map[row][col].x = 0;
							map[row][col].y = 0;
							map[row][col].shootTimer = 0;
						}
					}
				}
				else {
					collectSunshine(&msg);
				}
			}
			//printf("%d %d\n",msg.x,msg.y);
		}
		else if ((msg.message == WM_MOUSEMOVE && status == 1) || isSelected) {
			curX = msg.x;
			curY = msg.y;
			//printf("%d %d\n", msg.x, msg.y);

		}
		else if (msg.message == WM_LBUTTONUP) {
			if (msg.x >= 250 - 112 && msg.x <= 990 - 112 && msg.y >= 175 && msg.y <= 490) {
				int row = (msg.y - 175) / 105;
				int col = (msg.x - 250 + 112) / 82.5;
				//printf("%d  %d\n", row, col);
				if (map[row][col].type == 0) {
					if (curZhiwu) PlaySound("res/plant.wav", NULL, SND_FILENAME | SND_ASYNC);
					map[row][col].type = curZhiwu;
					sunshine -= needSunshine[curZhiwu - 1];
					map[row][col].frameIndex = 0;
					map[row][col].blood = 200;
					//map[row][col].catched = false;
					//int x = 250 + j * 82.5;
					//int y = 175 + i * 105+10;
					map[row][col].x = 250 - 112 + col * 82.5;
					map[row][col].y = 175 + row * 105 + 10;
					map[row][col].shootTimer = 40;
				}
			}
			status = 0;
			curZhiwu = 0;
		}
	}
}


void createSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	//printf("test\n");
	static int count = 0;
	static int fre = 400;
	++count;
	if (count >= fre) {
		fre = 200 + rand() % 200;
		count = 0;
		//从阳光池中选择一个可以使用的
		int i;
		for (i = 0; i < ballMax && balls[i].used; ++i);
		if (i >= ballMax) return;
		//printf("ballMax: %d\n i: %d\n", ballMax,i);
		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x=300 + rand() % (900 - 300);//阳光范围300-900
		//balls[i].y = rand()%60;
		//balls[i].endy =175+(rand() % 4)*90;
		balls[i].timer = 0;
		//balls[i].xoff = 0;
		//balls[i].yoff = 0;
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(300 + rand() % (900 - 300), rand() % 60);
		balls[i].p4 = vector2(balls[i].p1.x, 175 + (rand() % 4) * 90);
		int off = 2;
		int distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
		//printf("创建阳光！\n");
	}
	//向日葵生产阳光
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 9; ++j) {
			if (map[i][j].type == XIANG_RI_KUI + 1) {
				map[i][j].timer++;
				if (map[i][j].timer > 300) {
					map[i][j].timer = 0;
					int k;
					for (k = 0; k < ballMax && balls[k].used; ++k);
					if (k >= ballMax) return;

					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (100 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w, map[i][j].y + imgZhiwu[XIANG_RI_KUI][0]->getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.5, balls[k].p1.y - 50);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 50);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.02;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; ++i) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer >= 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t > 1) {
					sun->used = false;
					sunshine += 25;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t > 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
				}
			}
		}
		//else if (balls[i].xoff>0.000001) {
		//	//设置阳光偏移量
		//	float destY = 0, destX = 262;
		//	float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
		//	balls[i].xoff = 6 * cos(angle);
		//	balls[i].yoff = 6 * sin(angle);
		//	balls[i].x -= balls[i].xoff;
		//	balls[i].y -= balls[i].yoff;
		//	if (balls[i].y < 0 || balls[i].x < 262) {
		//		balls[i].xoff = 0;
		//		balls[i].yoff = 0;
		//		sunshine += 25;
		//	}
		//}
	}
}

void createzm() {
	if (zmCount >= ZM_MAX) return;
	static int zmFre = 200;
	static int zmTimer = 0;
	++zmTimer;
	if (zmTimer > zmFre) {
		zmTimer = 0;
		zmFre = 200 + rand() % 200;
		int count = sizeof(zms) / sizeof(zms[0]);
		int i;
		for (i = 0; i < count && zms[i].used; ++i);
		if (i < count) {
			int temp = rand() % 3;
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].y = 125 + temp * 100;
			zms[i].speed = 1;
			zms[i].row = temp;
			zms[i].blood = 100;
			zms[i].dead = false;
			zms[i].eating = false;
			zms[i].damage = 1;
			zmCount++;
		}
		else {
			printf("创建僵尸失败！");
		}
	}
}

void updatezm() {
	int zmMax = sizeof(zms) / sizeof(zms[0]);
	static int zmCount = 0;
	//更新僵尸位置
	zmCount++;
	if (zmCount > 4) {
		zmCount = 0;
		for (int i = 0; i < zmMax; ++i) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < 30) {
					gameStatus = FAIL;
				}
			}
		}
	}
	static int zmCount2 = 0;
	++zmCount2;
	if (zmCount2 > 4) {
		zmCount2 = 0;
		for (int i = 0; i < zmMax; ++i) {
			if (zms[i].used) {
				if (zms[i].dead) {
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) {
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX) {
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating) {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
				}
			}
		}
	}
}
void updatePlant() {
	static int plant_count = 0;
	plant_count++;
	if (plant_count > 3) {
		plant_count = 0;
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 9; ++j) {
				if (map[i][j].type > 0) {
					map[i][j].frameIndex++;
					int zhiWutype = map[i][j].type;
					int frame_index = map[i][j].frameIndex;
					if (imgZhiwu[zhiWutype - 1][frame_index] == NULL) {
						map[i][j].frameIndex = 0;
					}
				}
			}
		}
	}
}

void shoot() {
	bool lines[3] = { false,false,false };
	int zmcount = sizeof(zms) / sizeof(zms[0]);
	int dangerX = WIN_WIDTH - imgZM[0].getwidth() + 50;
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < zmcount; ++i) {
		if (zms[i].used && zms[i].x < dangerX) {
			lines[zms[i].row] = true;
		}
	}
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 9; ++j) {
			if (map[i][j].type == WAN_DOU + 1 && lines[i]) {
				//static int count = 0;
				//++count;
				map[i][j].shootTimer++;
				if (map[i][j].shootTimer > 120) {
					map[i][j].shootTimer = 0;
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; ++k);
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 6;
						int zwX = 250 - 112 + j * 82.5;
						int zwY = 175 + i * 105 + 10;
						bullets[k].x = zwX + imgZhiwu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						bullets[k].damage = 10;
					}
				}
			}
		}
	}
}



void updateBullet() {
	static int count = 0;
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	++count;
	if (count > 1) {
		count = 0;
		for (int i = 0; i < bulletMax; ++i) {
			if (bullets[i].used) {
				bullets[i].x += bullets[i].speed;
				if (bullets[i].x > WIN_WIDTH) {
					bullets[i].used = false;
				}
				if (bullets[i].blast) {
					bullets[i].frameIndex++;
					if (bullets[i].frameIndex >= 6) {
						bullets[i].used = false;
					}
				}
			}
		}
	}
}

void checkBulletToZm(int zmCount) {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int w = imgBulletNormal.getwidth();
	for (int i = 0; i < bCount; ++i) {
		if (!bullets[i].used || bullets[i].blast) continue;
		for (int j = 0; j < zmCount; ++j) {
			if (!zms[j].used) continue;
			if (bullets[i].row != zms[j].row) continue;
			if (!zms[j].dead && bullets[i].x + w - zms[j].x >= 90 && bullets[i].x + w - zms[j].x <= 120) {
				bullets[i].blast = true;
				zms[j].blood -= bullets[i].damage;
				if (zms[j].blood <= 0) {
					zms[j].dead = true;
					zms[j].frameIndex = 0;
					zms[j].speed = 0;
					zms[j].eating = false;
				}
				break;
			}
		}
	}
}
void checkZmToZhiwu(int zmCount) {
	for (int i = 0; i < zmCount; ++i) {
		if (zms[i].dead) continue;
		int row = zms[i].row;
		for (int j = 0; j < 9; ++j) {
			int ZhiwuL = 250 - 112 + j * 81;
			int x1 = ZhiwuL + 10, x2 = ZhiwuL + 60;
			int x3 = zms[i].x + 80;
			if (map[row][j].type == 0) {
				if (x3 > x1 && x3 < x2 && zms[i].eating) {
					zms[i].eating = false;
					zms[i].speed = 1;
				}
				continue;
			}
			if (x3 > x1 && x3 < x2) {
				if (zms[i].eating) {  //原版中如果植物被catch后，则其他僵尸无法捕获该植物，也就是同时只有一个僵尸可以吃该植物
					mciSendString("play res/chomp.wav", NULL, 0, 0);
					map[row][j].blood -= zms[i].damage;
					if (map[row][j].blood <= 0) {
						map[row][j].catched = false;
						map[row][j].type = 0;
						//如果植物血量小于0时，需要停止僵尸的eating状态，但是原版代码只能释放一个僵尸的eating代码，
						//因此会导致多个僵尸同时吃一个植物，植物被消灭后只有一个僵尸恢复行走状态，而其他僵尸一直保持eating状态
						for (int c = 0; c < zmCount; ++c) {
							if (zms[c].row != row) continue;
							int temp_x = zms[c].x + 80;
							if (temp_x > x1 && temp_x < x2) {
								zms[c].eating = false;
								zms[c].frameIndex = 0;
								zms[c].speed = 1;
							}
						}
					}
				}
				else {
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}


void collisionCheck() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	checkBulletToZm(zmCount);
	checkZmToZhiwu(zmCount);
}

void updateGame() {
	updatePlant();//植物更新
	createSunshine(); //创建阳光
	updateSunshine(); //更新阳光
	createzm(); //僵尸创建
	updatezm();//僵尸更新
	shoot(); //发射子弹
	updateBullet();//更新子弹

	collisionCheck(); //碰撞检测
}

void startUI() {
	IMAGE imgMenu, imgMenu1, imgMenu2;
	loadimage(&imgMenu, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");

	int flag = 0;
	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &imgMenu);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.x >= 475 && msg.x <= 475 + imgMenu1.getwidth()
				&& msg.y >= 75 && msg.y <= 75 + imgMenu1.getheight()
				&& msg.message == WM_LBUTTONDOWN) {
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP && flag == 1) {
				EndBatchDraw();
				break;
			}
		}
		EndBatchDraw();
	}
}

void viewScene() {
	mciSendString("play mySound", NULL, 0, 0);
	//PlaySound("res/Look_up_at_the_Sky.wav", NULL, SND_FILENAME | SND_ASYNC);
	int xMin = WIN_WIDTH - imgBg.getwidth();
	vector2 points[9] = { {550,80},{530,160},{630,170},{530,200},{525,270},
		{565,370},{605,340},{705,280},{690,340} };
	int index[9];
	for (int i = 0; i < 9; ++i) {
		index[i] = rand() % 11;
	}
	int count = 0;
	for (int x = 0; x >= xMin; x -= 1) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);
		++count;
		for (int k = 0; k < 9; ++k) {
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 4) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 4) count = 0;
		EndBatchDraw();
		Sleep(10);
	}
	for (int x = 0; x < 100; x++) {
		BeginBatchDraw();
		putimage(xMin, 0, &imgBg);
		++count;
		for (int k = 0; k < 9; ++k) {
			putimagePNG(points[k].x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 4) index[k] = (index[k] + 1) % 11;
		}
		if (count >= 4) count = 0;
		EndBatchDraw();
		Sleep(10);
	}
	for (int x = xMin; x <= -112; x += 1) {
		BeginBatchDraw();
		putimage(x, 0, &imgBg);
		++count;
		for (int k = 0; k < 9; ++k) {
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 4) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 4) count = 0;
		EndBatchDraw();
		Sleep(10);
	}
}

void CardsDown() {
	int h = imgBar.getheight();
	for (int y = -h; y <= 0; ++y) {
		BeginBatchDraw();
		putimage(-112, 0, &imgBg);
		putimagePNG(250, y, &imgBar);
		putimagePNG(150, y + 12, &imgChanzi);
		for (int i = 0; i < ZHI_WU_COUNT; ++i) {
			int x = 338 + i * 64;
			putimagePNG(x, y + 6, &imgCards[i]);
		}
		EndBatchDraw();
		Sleep(3);
	}
	// 当需要停止播放时
	Sleep(1000);
	mciSendString("stop mySound", NULL, 0, 0);
	mciSendString("play Faster", NULL, 0, 0);
}

bool checkOver() {
	bool res = false;
	if (gameStatus == WIN) {
		Sleep(2000);
		loadimage(&imgBg, "res/win2.png");
		mciSendString("play res/win.mp3", NULL, 0, 0);
		mciSendString("stop Faster", NULL, 0, 0);
		res = true;
	}
	else if (gameStatus == FAIL) {
		Sleep(2000);
		loadimage(&imgBg, "res/fail2.png");
		mciSendString("play res/lose.mp3", NULL, 0, 0);
		mciSendString("stop Faster", NULL, 0, 0);
		res = true;
	}
	return res;
}

int main() {
	gameInit();
	putimagePNG(300, 200, &imgChomperAttack);
	startUI();
	viewScene();
	CardsDown();
	int timer = 0;
	bool flag = true;

	while (1) {
		//用户点击
		userClick();
		timer += getDelay();
		if (timer > 5) {
			flag = true;
			timer = 0;
		}
		if (flag) {
			flag = false;
			updateWindow();
			updateGame();
			if (checkOver()) {
				break;
			}
		}
	}
	putimage(0, 0, &imgBg);
	system("pause");
	return 0;
}