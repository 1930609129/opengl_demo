#include <windows.h>
#include <gl/gl.h>
#include "../opengl/camera.h"
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../../demo/stb-master/stb_image.h"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);

#include "main.h"

void Recipe_Add(int items[3][3],int itemOut)
{
    recipeCnt++;
    recipe = realloc(recipe,sizeof(TRecipe)*recipeCnt);
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++)
        {
            recipe[recipeCnt-1].items[i][j]=items[i][j];
        }
    }
    recipe[recipeCnt-1].itemOut = itemOut;
}

void Recipe_Check()
{
    for(int k=0;k<recipeCnt;k++)
    {
        BOOL checkOK = TRUE;
        for(int i=0;i<3;i++)
        {
            for(int j=0;j<3;j++)
            {
                if(craft_menu.items[i][j].type!=recipe[k].items[i][j])
                    checkOK = FALSE;
            }
        }
        if(checkOK)
        {
            craft_menu.itemOut.type = recipe[k].itemOut;
            break;
        }
        else
            craft_menu.itemOut.type = 0;
    }
}

BOOL IsPointInSlot(TSlot slot,int x,int y)
{
    return ((x>slot.x)&&(x<slot.x+slot.width)&&(y>slot.y)&&(y<slot.y+slot.height));
}

void Anim_Set(TAnim *anm,TObject *obj)
{
    if(anm->obj!=NULL) return;
    anm->obj = obj;
    anm->cnt = 10;
    anm->dx = (camera.x - obj->x)/(float)anm->cnt;
    anm->dy = (camera.y - obj->y)/(float)anm->cnt;
    anm->dz = ((camera.z - obj->scale -0.2) - obj->z)/(float)anm->cnt;
}

void Anim_Move(TAnim *anm)
{
    if(anm->obj!=NULL)
    {
        anm->obj->x += anm->dx;
        anm->obj->y += anm->dy;
        anm->obj->z += anm->dz;
        anm->cnt--;
        if(anm->cnt<1)
        {
            int i;
            for(i=0;i<bagSize;i++)
            {
                if(bag[i].type<=0)
                {
                    bag[i].type = anm->obj->type;
                    break;
                }
            }
            if(i<bagSize)
            {
                anm->obj->x = rand()%mapW;
                anm->obj->y = rand()%mapH;
            }
            anm->obj->z = Map_GetHeight(anm->obj->x,anm->obj->y);
            anm->obj = NULL;
        }
    }
}

void Tree_Show(TObjGroup obj)
{
    glEnable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(3,GL_FLOAT,0,cube);
        glColor3f(0.7,0.7,0.7);
        glNormal3f(0,0,1);
        glBindTexture(GL_TEXTURE_2D,obj.type);
        for(int i=0;i<obj.itemsCnt;i++)
        {
            if(obj.items[i].type==1) glTexCoordPointer(2,GL_FLOAT,0,cubeUVlog);
            else glTexCoordPointer(2,GL_FLOAT,0,cubeUVleaf);
            glPushMatrix();
                glTranslatef(obj.items[i].x,obj.items[i].y,obj.items[i].z);
                glDrawElements(GL_TRIANGLES,cubeIncCnt,GL_UNSIGNED_INT,cubeInd);
            glPopMatrix();
        }
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Tree_Create(TObjGroup *obj,int type,float x,float y)
{
    obj->type = type;
    float z = Map_GetHeight(x+0.5,y+0.5)-0.5;
    int logs = 6;
    int leafs = 5*5*2 -2 +3*3*2;

    obj->itemsCnt = logs+leafs;
    obj->items = malloc(sizeof(TObject)*obj->itemsCnt);

    for(int i=0;i<logs;i++)
    {
        obj->items[i].type = 1;
        obj->items[i].x = x;
        obj->items[i].y = y;
        obj->items[i].z = z+i;
    }
    int pos = logs;
    for (int k=0;k<2;k++)
    {
        for(int i=x-2;i<=x+2;i++)
        {
            for(int j=y-2;j<=y+2;j++)
            {
                if((i!=x)||(j!=y))
                {
                    obj->items[pos].type = 2;
                    obj->items[pos].x = i;
                    obj->items[pos].y = j;
                    obj->items[pos].z = z+logs - 2+k;
                    pos++;
                }
            }
        }
    }

    for (int k=0;k<2;k++)
    {
        for(int i=x-1;i<=x+1;i++)
        {
            for(int j=y-1;j<=y+1;j++)
            {
                obj->items[pos].type = 2;
                obj->items[pos].x = i;
                obj->items[pos].y = j;
                obj->items[pos].z = z+logs +k;
                pos++;
            }
        }
    }
}

void LoadTexture(char *file_name,int *terget)
{
    int Width,height,cnt;
    unsigned char *data = stbi_load(file_name, &Width, &height, &cnt, 0);
    glGenTextures(1,terget);
    glBindTexture(GL_TEXTURE_2D,*terget);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,Width,height,0,cnt==4?GL_RGBA:GL_RGB,GL_UNSIGNED_BYTE,data);
    glBindTexture(GL_TEXTURE_2D,0);
    stbi_image_free(data);
}

#define sqr(a) (a)*(a)
void CalcNormals(TCell a,TCell b,TCell c,TCell *n)
{
    float wrki;
    TCell v1,v2;

    v1.x = a.x - b.x;
    v1.y = a.y - b.y;
    v1.z = a.z - b.z;
    v2.x = b.x - c.x;
    v2.y = b.y - c.y;
    v2.z = b.z - c.z;

    n->x = (v1.y * v2.z - v1.z * v2.y);
    n->y = (v1.z * v2.x - v1.x * v2.z);
    n->z = (v1.x * v2.y - v1.y * v2.x);
    wrki = sqrt(sqr(n->x)+sqr(n->y)+sqr(n->z));
    n->x /= wrki;
    n->y /= wrki;
    n->z /= wrki;
}

BOOL IsCoordInMap(float x,float y)
{
    return (x>=0) && (x<mapW) && (y>=0) && (y<mapH);
}

void Map_CreateHill(int posX,int posY,int rad,int height)
{
    for (int i = posX-rad;i<=posX+rad;i++)
    {
        for (int j = posY-rad;j<=posY+rad;j++)
        {
            if(IsCoordInMap(i,j))
            {
                float len = sqrt(pow(posX-i,2)+pow(posY-j,2));
                if(len<rad)
                {
                    len = len/rad * M_PI_2;
                    map[i][j].z += cos(len) *height;
                }
            }
        }
    }
}

float Map_GetHeight(float x, float y)
{
    if (!IsCoordInMap(x,y)) return 0;
    int cX = (int)x;
    int cY = (int)y;
    float h1 = ((1-(x-cX))*map[cX][cY].z + (x-cX)*map[cX+1][cY].z);
    float h2 = ((1-(x-cX))*map[cX][cY+1].z + (x-cX)*map[cX+1][cY+1].z);
    return (1-(y-cY))*h1 + (y-cY)*h2;
}

void Game_Create()
{
    memset(&craft_menu,0,sizeof(craft_menu));
    craft_menu.show = FALSE;
}

void Map_Init()
{
    for (int i=0;i<bagSize;i++)
        bag[i].type=0;

    LoadTexture("textures/pole.png",&tex_pole);
    LoadTexture("textures/trava.png",&tex_trava);
    LoadTexture("textures/flower.png",&tex_flower);
    LoadTexture("textures/flower2.png",&tex_flower2);
    LoadTexture("textures/grib.png",&tex_grib);
    LoadTexture("textures/tree.png",&tex_tree);
    LoadTexture("textures/tree2.png",&tex_tree2);
    LoadTexture("textures/wood.png",&tex_wood);

    LoadTexture("textures/speed.png",&tex_ico_speed);
    LoadTexture("textures/eye.png",&tex_ico_eye);

    LoadTexture("textures/mortar.png",&tex_ico_mortar);
    LoadTexture("textures/potion_eye.png",&tex_ico_potion_eye);
    LoadTexture("textures/potion_speed.png",&tex_ico_potion_speed);
    LoadTexture("textures/potion_life.png",&tex_ico_potion_life);

    bag[0].type = tex_ico_mortar;
    Recipe_Add((int [3][3]) {tex_flower2,0,tex_flower2
                            ,0,tex_grib,0
                            ,tex_flower2,0,tex_flower2},tex_ico_potion_eye);
    Recipe_Add((int [3][3]) {tex_flower,tex_flower,tex_flower
                            ,0,0,0
                            ,tex_flower,tex_flower,tex_flower},tex_ico_potion_speed);
    Recipe_Add((int [3][3]) {0,tex_grib,0
                            ,tex_grib,tex_grib,tex_grib
                            ,0,tex_grib,0},tex_ico_potion_life);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER,0.99);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0;i<mapW;i++)
    {
        for (int j = 0;j<mapH;j++)
        {

            map[i][j].x = i;
            map[i][j].y = j;
            map[i][j].z = (rand()%10)*0.02;

            mapUV[i][j].u = i;
            mapUV[i][j].v = j;
        }
    }

    for (int i=0;i<mapW-1;i++)
    {
        int pos = i*mapH;
        for (int j=0;j<mapH-1;j++)
        {
            mapInd[i][j][0]=pos;
            mapInd[i][j][1]=pos + 1;
            mapInd[i][j][2]=pos + 1 +mapH;

            mapInd[i][j][3]=pos + 1 + mapH;
            mapInd[i][j][4]=pos + mapH;
            mapInd[i][j][5]=pos;

            pos++;
        }
    }

    for (int i=0;i<10;i++)
    {
        Map_CreateHill(rand()%mapW,rand()%mapH,rand()%50,rand()%10);
    }

    for (int i = 0;i<mapW-1;i++)
    {
        for (int j = 0;j<mapH-1;j++)
        {
            CalcNormals(map[i][j],map[i+1][j],map[i][j+1],&mapNormal[i][j]);
        }
    }

    int travaN = 2000;
    int gribN = 30;
    int treeN = 40;
    plantCnt = travaN + gribN + treeN;
    plantMas = realloc(plantMas,sizeof(*plantMas)*plantCnt);
    for (int i = 0;i<plantCnt;i++)
    {
        if(i<travaN)
        {
            plantMas[i].type = rand()%10!=0? tex_trava:
                               (rand()%2==0? tex_flower:tex_flower2);
            plantMas[i].scale = 0.7+(rand()%5)*0.1;
        }
        else if(i<(travaN+gribN))
        {
            plantMas[i].type = tex_grib;
            plantMas[i].scale = 0.2+(rand()%10)*0.01;
        }
        else
        {
            plantMas[i].type = rand()%2==0 ? tex_tree:tex_tree2;
            plantMas[i].scale = 4+(rand()%14);
        }
        plantMas[i].x = rand()%mapW;
        plantMas[i].y = rand()%mapH;
        plantMas[i].z = Map_GetHeight(plantMas[i].x,plantMas[i].y);
    }

    treeCnt = 50;
    tree = realloc(tree,sizeof(*tree)*treeCnt);
    for(int i=0;i<treeCnt;i++)
    {
        Tree_Create(tree+i,tex_wood,rand()%mapW,rand()%mapH);
    }

}

void Buff_Timer(TBuff *buff)
{
    if(buff->time>0)
    {
        buff->time--;
        if(buff->time<=0)
            buff->timeMax=0;
    }
}

void Map_Proc()
{
    static hunger = 0;
    hunger++;
    if(hunger>200)
    {
        hunger = 0;
        health--;
        if(health<1)
        {
            PostQuitMessage(0);
        }
    }
    Buff_Timer(&buffs.speed);
    Buff_Timer(&buffs.eye);
}

void Map_Show()
{
    float sz = 0.1;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-scrKoef*sz,scrKoef*sz, -sz,sz, sz*2,1000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_DEPTH_TEST);



    static float alfa = 0;
    alfa += 0.03;
    if(alfa > 180) alfa -= 360;

    #define abs(a) ((a)>0?(a):-(a))
    float kcc = 1 - (abs(alfa)/180);

    #define sakat 40.0
    float k = 90 - abs(alfa);
    k = (sakat - abs(k));
    k = k < 0 ? 0:k/sakat;

    if (selectMode) glClearColor(0,0,0,0);
    else glClearColor(0.6f*kcc, 0.8f*kcc, 1.0f*kcc, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(selectMode)
    {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
    }

    else
    {
        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }

    Anim_Move(&animation);

    glPushMatrix();
        if(!selectMode){
            glPushMatrix();
                glRotatef(-camera.Xrot,1,0,0);
                glRotatef(-camera.Zrot,0,0,1);
                glRotatef(alfa,0,1,0);
                glTranslatef(0,0,20);
                glDisable(GL_DEPTH_TEST);

                    glDisable(GL_TEXTURE_2D);
                    glColor3f(1,1-k*0.8,1-k);
                    glEnableClientState(GL_VERTEX_ARRAY);
                        glVertexPointer(3,GL_FLOAT,0,sun);
                        glDrawArrays(GL_TRIANGLE_FAN,0,4);
                    glDisableClientState(GL_VERTEX_ARRAY);
                    glEnable(GL_TEXTURE_2D);

                glEnable(GL_DEPTH_TEST);
            glPopMatrix();
        }

        Camera_Apply();

        glPushMatrix();
            glRotatef(alfa,0,1,0);
            GLfloat position[]={0,0,1,0};
            glLightfv(GL_LIGHT0,GL_POSITION,position);
            float mas[] = {1+k*2,1,1,0};
            glLightfv(GL_LIGHT0,GL_DIFFUSE,mas);

            float clr = kcc*0.15+0.05;
            float mas0[]={clr,clr,clr,0};
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT,mas0);
        glPopMatrix();

        if(!selectMode)
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
                glVertexPointer(3,GL_FLOAT,0,map);
                glTexCoordPointer(2 ,GL_FLOAT,0,mapUV);
                glColor3f(0.7,0.7,0.7);
                glNormalPointer(GL_FLOAT,0,mapNormal);
                glBindTexture(GL_TEXTURE_2D,tex_pole);
                glDrawElements(GL_TRIANGLES,mapIndCnt,GL_UNSIGNED_INT,mapInd);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_NORMAL_ARRAY);
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glVertexPointer(3,GL_FLOAT,0,plant);
            glTexCoordPointer(2 ,GL_FLOAT,0,plantUV);
            glColor3f(0.7,0.7,0.7);
            glNormal3f(0,0,1);
            selectMasCnt = 0;
            int selectColor = 1;
            for(int i=0;i<plantCnt;i++)
            {
                if(selectMode)
                {
                    if((plantMas[i].type==tex_tree)||(plantMas[i].type==tex_tree2))
                        continue;
                    static int radius = 3;
                    if((plantMas[i].x>camera.x-radius)
                       &&(plantMas[i].x<camera.x+radius)
                       &&(plantMas[i].y>camera.y-radius)
                       &&(plantMas[i].y<camera.y+radius))
                    {
                        glColor3ub(selectColor,0,0);
                        selectMas[selectMasCnt].colorIndex = selectColor;
                        selectMas[selectMasCnt].plantMas_Index = i;
                        selectMasCnt++;
                        selectColor++;
                        if (selectColor>=255)
                            break;
                    }
                    else
                        continue;
                }
                else
                {
                    if((plantMas[i].type==tex_grib)&&(buffs.eye.time>0))
                        glDisable(GL_LIGHTING);
                }
                glBindTexture(GL_TEXTURE_2D,plantMas[i].type);
                glPushMatrix();
                    glTranslatef(plantMas[i].x,plantMas[i].y, plantMas[i].z);
                    glScalef(plantMas[i].scale,plantMas[i].scale,plantMas[i].scale);
                    glDrawElements(GL_TRIANGLES,plantIndCnt,GL_UNSIGNED_INT,plantInd);
                glPopMatrix();

                if(!selectMode)
                {
                    if((plantMas[i].type==tex_grib)&&(buffs.eye.time>0))
                        glEnable(GL_LIGHTING);
                }
            }
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        if(!selectMode)
        {
            for(int i=0;i<treeCnt;i++)
            {
            Tree_Show(tree[i]);
            }
        }

    glPopMatrix();
}

void Player_Take(HWND hwnd)
{
    selectMode = TRUE;
    Map_Show();
    selectMode = FALSE;

    RECT rct;
    GLubyte clr[3];
    GetClientRect(hwnd,&rct);
    glReadPixels(rct.right/2.0,rct.bottom/2.0,1,1,GL_RGB,
                 GL_UNSIGNED_BYTE,clr);
    if(clr[0]>0)
    {
        for(int i=0;i<selectMasCnt;i++)
        {
            if(selectMas[i].colorIndex==clr[0])
            {
                Anim_Set(&animation,plantMas+selectMas[i].plantMas_Index);
            }
        }
    }
}

void Player_Move()
{
    Camera_MoveDirection(GetKeyState('W')<0 ? 1:(GetKeyState('S')<0 ? -1 :0),
                         GetKeyState('D')<0 ? 1:(GetKeyState('A')<0 ? -1 :0),0.1+(buffs.speed.time>0?0.2:0));
    if(mouseBind)
        Camera_AutoMoveByMouse(400,400,0.2);
    camera.z = Map_GetHeight(camera.x,camera.y)+1.7;
}

void CraftMenu_Resize(int scale)
{
    craft_menu.width = scale * 6;
    craft_menu.height = scale * 4;
    craft_menu.x = (scrSize.x - craft_menu.width)*0.5;
    craft_menu.y = (scrSize.y - craft_menu.height)*0.5;
    int scale05 = scale * 0.5;
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++)
        {
            craft_menu.items[i][j].x = craft_menu.x+scale05 + i*scale;
            craft_menu.items[i][j].y = craft_menu.y+scale05 + j*scale;
            craft_menu.items[i][j].width = scale;
            craft_menu.items[i][j].height = scale;
        }
    }
    craft_menu.itemOut.x = craft_menu.x+scale05 + 4*scale;
    craft_menu.itemOut.y = craft_menu.y+scale05 + 1*scale;
    craft_menu.itemOut.width = scale;
    craft_menu.itemOut.height = scale;
}

void WndResize(int x,int y)
{
    glViewport(0,0,x,y);
    scrSize.x = x;
    scrSize.y = y;
    scrKoef = x / (float) y;

    CraftMenu_Resize(50);
}

void Cell_Show(int x,int y,int scaleX,int scaleY,int type)
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,bagRect);
    glTexCoordPointer(2,GL_FLOAT,0,bagRectUV);
    glPushMatrix();
        glTranslatef(x,y,0);
        glScalef(scaleX,scaleY,1);
        glColor3ub(110,95,73);
        glDisable(GL_TEXTURE_2D);
        glDrawArrays(GL_TRIANGLE_FAN,0,4);

        if(type>0)
        {
            glColor3f(1,1,1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D,type);
            glDrawArrays(GL_TRIANGLE_FAN,0,4);
        }

        glColor3ub(160,146,116);
        glLineWidth(3);
        glDisable(GL_TEXTURE_2D);
        glDrawArrays(GL_LINE_LOOP,0,4);
    glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void Bag_Show(int x,int y,int scale)
{
    for (int i=0;i<bagSize;i++)
    {
        Cell_Show(x+i*scale,y,scale,scale,bag[i].type);
    }
}

void HandItem_Show()
{
    if((handItemType>0)&&(!mouseBind))
    {
        Cell_Show(mousePos.x,mousePos.y,50,50,handItemType);
    }
}

int Bag_GetCnt(int type)
{
    int cnt=0;
    for(int i=0;i<bagSize;i++)
    {
        if(bag[i].type==type)
            cnt++;
    }
    return cnt;
}

void Bag_DelCut(int type,int cnt)
{
    for(int i=0;i<bagSize;i++)
    {
        if(bag[i].type==type)
        {
            bag[i].type=-1;
            cnt--;
            if(cnt<=0) return;
        }
    }
}

void Bag_Click(int x,int y,int scale,int mx, int my,int button)
{
    if((my<y)||(my>y+scale)) return;
    for(int i=0;i<bagSize;i++)
    {
        if((mx>x+i*scale)&&(mx<x+(i+1)*scale))
        {
            if(button==WM_LBUTTONDOWN)
            {
                int type = handItemType;
                handItemType = bag[i].type;
                bag[i].type = type;
            }
            else if(bag[i].type==tex_ico_mortar)
            {
                craft_menu.show=!craft_menu.show;
            }
            else if(bag[i].type==tex_grib)
            {
                health++;
                if(health>healthMax) health = healthMax;
                bag[i].type=-1;
            }
            else if(bag[i].type==tex_ico_potion_life)
            {
                health+=15;
                if(health>healthMax) health = healthMax;
                bag[i].type=-1;
            }
            else if(bag[i].type==tex_ico_potion_speed)
            {
                buffs.speed.time = 3600;
                buffs.speed.timeMax = 3600;
                bag[i].type=-1;
            }
            else if(bag[i].type==tex_ico_potion_eye)
            {
                buffs.eye.time = 3600;
                buffs.eye.timeMax = 3600;
                bag[i].type=-1;
            }
            else
                bag[i].type = -1;
        }
    }
}

void Health_Show(int x,int y,int scale)
{
    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,heart);
        for(int i=0;i<healthMax;i++)
        {
            glPushMatrix();
                glTranslatef(x+i*scale,y,0);
                glScalef(scale,scale,1);
                if (i<health) glColor3f(1,0,0);
                else glColor3f(0,0,0);
                glDrawArrays(GL_TRIANGLE_FAN,0,6);
            glPopMatrix();
        }
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Cross_Show()
{
    static float cross[]={0,-1, 0,1, -1,0, 1,0};
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2,GL_FLOAT,0,cross);
        glPushMatrix();
            glColor3f(1,1,1);
            glTranslatef(scrSize.x*0.5,scrSize.y*0.5,0);
            glScalef(15,15,1);
            glLineWidth(3);
            glDrawArrays(GL_LINES,0,4);
        glPopMatrix();
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Buff_Show(int x,int y,int scale,TBuff buff,int texID)
{
    if(buff.time>0)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(2,GL_FLOAT,0,bagRect);
        glTexCoordPointer(2,GL_FLOAT,0,bagRectUV);
            glPushMatrix();
                glTranslatef(x,y,0);
                glScalef(scale,scale,1);
                glColor3f(1,1,1);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D,texID);
                glDrawArrays(GL_TRIANGLE_FAN,0,4);

                glScalef(1,1-(buff.time/(float)buff.timeMax),1);
                glColor4f(1,1,1, 0.5);
                glDisable(GL_ALPHA_TEST);
                    glDisable(GL_TEXTURE_2D);
                    glDrawArrays(GL_TRIANGLE_FAN,0,4);
                glEnable(GL_ALPHA_TEST);
            glPopMatrix();
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }
}

void CraftMenu_Click(int mx,int my,int button)
{
    if((!craft_menu.show)||(button!=WM_LBUTTONDOWN)) return;
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++)
        {
            if(IsPointInSlot(craft_menu.items[i][j],mx,my))
            {
                int type = handItemType;
                handItemType = craft_menu.items[i][j].type;
                craft_menu.items[i][j].type = type;
                if(craft_menu.items[i][j].type<=0)
                    craft_menu.items[i][j].type=0;
            }
        }
    }
    if(IsPointInSlot(craft_menu.itemOut,mx,my)&&(handItemType<=0)&&(craft_menu.itemOut.type>0))
    {
        handItemType = craft_menu.itemOut.type;
        craft_menu.itemOut.type = 0;
        for(int i=0;i<3;i++)
        {
            for(int j=0;j<3;j++)
            {
                craft_menu.items[i][j].type=0;
            }
        }
    }
    Recipe_Check();
}

void CraftMenu_Show()
{
    if((!craft_menu.show)||(mouseBind)) return;
    Cell_Show(craft_menu.x,craft_menu.y,craft_menu.width,craft_menu.height,0);
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++)
        {
            Cell_Show(craft_menu.items[i][j].x
                     ,craft_menu.items[i][j].y
                     ,craft_menu.items[i][j].width
                     ,craft_menu.items[i][j].height
                     ,craft_menu.items[i][j].type);
        }
    }
    Cell_Show(craft_menu.itemOut.x
             ,craft_menu.itemOut.y
             ,craft_menu.itemOut.width
             ,craft_menu.itemOut.height
             ,craft_menu.itemOut.type);
}

void Menu_Show()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,scrSize.x, scrSize.y,0, -1,1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    Bag_Show(10,10,50);
    Health_Show(10,70,30);
    Cross_Show();
    CraftMenu_Show(50);

    Buff_Show(10,110,50,buffs.speed,tex_ico_speed);
    Buff_Show(60,110,50,buffs.eye,tex_ico_eye);
    HandItem_Show();
}

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;
    float theta = 0.0f;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "OpenGL Sample",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          1000,
                          600,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    SetCursor(wcex.hCursor);

    /* enable OpenGL for the window */
    EnableOpenGL(hwnd, &hDC, &hRC);

    RECT rct;
    GetClientRect(hwnd,&rct);
    WndResize(rct.right,rct.bottom);
    Map_Init();
    glEnable(GL_DEPTH_TEST);

    /* program main loop */
    while (!bQuit)
    {
        /* check for messages */
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            /* handle or dispatch messages */
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            /* OpenGL animation code goes here */

            GetCursorPos(&mousePos);
            ScreenToClient(hwnd,&mousePos);

            if (GetForegroundWindow()==hwnd)
                Player_Move();
            Map_Proc();
            Map_Show();
            Menu_Show();

            SwapBuffers(hDC);

            theta += 1.0f;
            Sleep (1);
        }
    }

    /* shutdown OpenGL */
    DisableOpenGL(hwnd, hDC, hRC);

    /* destroy the window explicitly */
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_CREATE:
            Game_Create();
        break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if(mouseBind)
                Player_Take(hwnd);
            else
            {
                Bag_Click(10,10,50,LOWORD(lParam),HIWORD(lParam),uMsg);
                CraftMenu_Click(LOWORD(lParam),HIWORD(lParam),uMsg);
            }
        break;

        case WM_SIZE:
            WndResize(LOWORD(lParam),HIWORD(lParam));
        break;

        case WM_SETCURSOR:
            ShowCursor(!mouseBind);
        break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE:
                    PostQuitMessage(0);
                break;
                case 'E':
                    mouseBind=!mouseBind;
                    SetCursorPos(400,400);
                    if(mouseBind)
                        while(ShowCursor(FALSE)>=0);
                    else
                        while(ShowCursor(TRUE)<=0);
                break;
            }
        }
        break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

