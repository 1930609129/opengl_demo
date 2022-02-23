#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

typedef struct{
    float x,y,z;
}TCell;

typedef struct{
    float r,g,b;
}TColor;

typedef struct{
    float u,v;
}TUV;

typedef struct{
    float x,y,z;
    float type;
    float scale;
}TObject;

typedef struct{
    TObject *items;
    int itemsCnt;
    int type;
}TObjGroup;

#define mapW 100
#define mapH 100
TCell map[mapW][mapH];
TCell mapNormal[mapW][mapH];
TUV mapUV[mapW][mapH];

GLuint mapInd[mapW-1][mapH-1][6];
int mapIndCnt = sizeof(mapInd)/sizeof(GLuint);

float plant[] = {-0.5,0,0, 0.5,0,0, 0.5,0,1, -0.5,0,1,
                 0,-0.5,0, 0,0.5,0, 0,0.5,1, 0,-0.5,1};
float plantUV[] = {0,1, 1,1, 1,0, 0,0, 0,1, 1,1, 1,0, 0,0};
GLuint plantInd[] = {0,1,2, 2,3,0, 4,5,6, 6,7,4};
int plantIndCnt = sizeof(plantInd)/sizeof(GLuint);

int tex_pole,tex_trava,tex_flower,tex_flower2,tex_grib,tex_tree,tex_tree2;

TObject *plantMas = NULL;
int plantCnt = 0;

int tex_wood;

float cube[] = {0,0,0, 1,0,0, 1,1,0, 0,1,0,
                0,0,1, 1,0,1, 1,1,1, 0,1,1,
                0,0,0, 1,0,0, 1,1,0, 0,1,0,
                0,0,1, 1,0,1, 1,1,1, 0,1,1};

float cubeUVlog[] = {0.5,0.5, 1,0.5, 1,0, 0.5,0,
                     0.5,0.5, 1,0.5, 1,0, 0.5,0,
                     0,0.5, 0.5,0.5, 0,0.5, 0.5,0.5,
                     0,0, 0.5,0, 0,0, 0.5,0};

float cubeUVleaf[] = {0,1, 0.5,1, 0.5,0.5, 0,0.5,
                      0,1, 0.5,1, 0.5,0.5, 0,0.5,
                      0,0.5, 0.5,0.5, 0,0.5, 0.5,0.5,
                      0,1, 0.5,1, 0,1, 0.5,1};

GLuint cubeInd[] = {0,1,2, 2,3,0, 4,5,6, 6,7,4, 8,9,13, 13,12,8,
                    9,10,14, 14,13,9, 10,11,15, 15,14,10, 11,8,12, 12,15,11};

int cubeIncCnt = sizeof(cubeInd)/sizeof(GLuint);

TObjGroup *tree = NULL;
int treeCnt = 0;

float sun[] = {-1,-1,0, 1,-1,0, 1,1,0, -1,1,0};

BOOL selectMode = FALSE;

#define ObjListCnt 255
typedef struct{
    int plantMas_Index;
    int colorIndex;
}TSelectObj;
TSelectObj selectMas[ObjListCnt];
int selectMasCnt = 0;

typedef struct{
    TObject *obj;
    float dx,dy,dz;
    int cnt;
}TAnim;

TAnim animation = {0,0,0,0,0};

POINT scrSize;
float scrKoef;

typedef struct{
    int type;
    int x,y;
    int width,height;
}TSlot;
#define bagSize 16
TSlot bag[bagSize];

float bagRect[] = {0,0, 1,0, 1,1, 0,1};
float bagRectUV[] = {0,0, 1,0, 1,1, 0,1};

int health = 15;
int healthMax = 20;

float heart[] = {0.5,0.25, 0.25,0, 0,0.25, 0.5,1, 1,0.25, 0.75,0};

BOOL mouseBind = TRUE;

typedef struct{
    int time;
    int timeMax;
}TBuff;

struct{
    TBuff speed;
    TBuff eye;
}buffs={0,0,0,0};

int tex_ico_speed,tex_ico_eye;
int tex_ico_mortar,tex_ico_potion_eye,tex_ico_potion_speed,tex_ico_potion_life;

int handItemType = 0;

POINT mousePos;

struct{
    int x,y;
    int width,height;
    TSlot items[3][3];
    TSlot itemOut;
    BOOL show;
}craft_menu;

typedef struct{
    int items[3][3];
    int itemOut;
}TRecipe;
TRecipe *recipe;
int recipeCnt = 0;

float Map_GetHeight(float x, float y);

#endif // MAIN_H_INCLUDED
