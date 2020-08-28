/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples in Notepad++, provided with default raylib installer package, 
*   just press F6 and run [raylib_compile_execute] script, it will compile and execute.
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on [C:\raylib\raylib\examples] directory and
*   raylib official webpage: [www.raylib.com]
*
*   Enjoy using raylib. :)
*
*   This example has been created using raylib 1.0 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2013-2020 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "rlgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MAX_MESH_VBO 7


const int chunkWidth = 16;
const int chunkHeight = 16;  
const int blockSize = 2.0f;

const int worldSize = 32;

float y_offset = 0.0f;
static float y = 0.0f;


typedef struct {
    int x;
    int z; 
    int y;
    int solid;
    
} Block;


typedef struct {
    int x;
    int z;
    Block blocks_array[16 * 16 * 16];
    Model model;
    int loaded;
    int number_blocks;
} Chunk;

Chunk world[32][32];

const int renderDistance = 6; //MAXIMUM
//(2*rd+1)^2
Chunk *activeChunks[(2*6+1) * (2*6+1)];

static float *GetCubeVertices(float x, float y, float z);
Model GetChunkModel(Chunk *chunk);

int noise2(int x, int y);
float lin_inter(float x, float y, float s);
float smooth_inter(float x, float y, float s);
float noise2d(float x, float y);
float perlin2d(float x, float y, float freq, int depth);

void DrawChunkRegion(int color, Chunk *chunk);
Vector3 DetermineCurrentChunkRegion(Camera *cam);
void DetermineActiveChunks(Camera *camera);
void DrawActiveChunks(Camera *cam);
void DrawBlock(int color, Chunk c, Block block);

Chunk createChunkRegion();
Block createBlock(int x, int z, int y);
void createWorld();
void freeWorld();

void GetYOffset();
char *createKey(int x, int z, int y);





int main(void)
{
    puts("main");
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    
    
    float cam_start_x = 100.0f;
    float cam_start_z = 100.0f;
    float cam_start_y = 45.0f;
   
    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");
    
    Camera camera = {0};
    camera.position = (Vector3){cam_start_x,cam_start_y,cam_start_z};
    camera.target = (Vector3){1.0f,1.0f,1.0f};
    camera.up = (Vector3){0.0f,1.0f,0.0f};
    camera.fovy = 45.0f;
    camera.type = CAMERA_PERSPECTIVE;
    
    
 

    SetCameraMode(camera, CAMERA_FIRST_PERSON);
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    createWorld();
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);
        DetermineActiveChunks(&camera);
        
        GetYOffset();

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();    
            
            ClearBackground(BLUE);
            
            DrawText(TextFormat("Chunk x: %i", (int)camera.position.x/32), 10, 30, 8, BLACK);
            DrawText(TextFormat("Chunk z: %i", (int)camera.position.z/32), 130, 30, 8, BLACK);
            DrawText(TextFormat("Cam x: %f, cam z:%f",camera.position.x,camera.position.z), 10, 60, 8, BLACK);


            BeginMode3D(camera);
                DrawActiveChunks(&camera);
                DrawGrid(100,32.0);

            EndMode3D(); 
            DrawFPS(10, 10); 

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void GetYOffset(){
    if(IsKeyDown(KEY_Q)){
        y_offset = .002f;
    } 
    else if(IsKeyDown(KEY_E)){
        y_offset = -.002f;
    }
    else{
        y_offset = 0.0f;
    }

}



Block createBlock(int x, int z, int y){
    Block block;
    block.x = x;
    block.z = z;
    block.y = y;  
    return block;
}

Chunk createChunkRegion(int x, int z){    
    int u = 0;
    char *a = malloc(100 * sizeof(char));

    Chunk chunk;
    chunk.x = x;
    chunk.z = z;
    chunk.model;
    chunk.loaded = 0;
    int tmp_y = 0;
    //map_init(&chunk.blocks);
    
    for(int i = 0; i < chunkWidth; i++){
        for(int j = 0; j < chunkWidth; j++){
            for(int k = 0; k < chunkHeight; k++){
                tmp_y = round(chunkHeight * perlin2d((i * blockSize)+(chunk.x * chunkWidth* blockSize), (j * blockSize)+(chunk.z * chunkWidth* blockSize), 0.04f, chunkHeight));
                if(k < tmp_y){
                    chunk.blocks_array[u] = createBlock(i,j,tmp_y);
                    u++;
                }
            }    
        }
    }
    chunk.number_blocks = u;
    return chunk;
}


void createWorld(){
    int i;
    int j;
    for(i = 0; i < worldSize; i++){
        for(j = 0; j < worldSize; j++){
            world[i][j] = createChunkRegion(i,j); 
        }
    }     
}





void DrawChunkRegion(int color, Chunk *chunk){
    if(!chunk->loaded){
        chunk->model = GetChunkModel(chunk);
        chunk->loaded = 1;
    }
    y = y + y_offset;
    DrawModel(chunk->model, (Vector3){chunk->x * 16, y, chunk->z * 16}, 1.0f, WHITE);
}

void DrawActiveChunks(Camera *camera){
    int i = 0;
    while(i < ((2*renderDistance+1)*(2*renderDistance+1))){
        DrawChunkRegion(i,(activeChunks[i]));
        i++;
    }
}


Vector3 DetermineCurrentChunkRegion(Camera *camera){
    Vector3 currentChunk = camera->position;
    if(camera->position.x > currentChunk.x ){
        currentChunk.x = currentChunk.x + (chunkWidth * blockSize);
    }
    else if(camera->position.x < currentChunk.x ){
        currentChunk.x = currentChunk.x - (chunkWidth * blockSize);
    }
    else if(camera->position.z > currentChunk.z ){
        currentChunk.z = currentChunk.z + (chunkWidth * blockSize);
    }
    else if(camera->position.z < currentChunk.z){
        currentChunk.z = currentChunk.z - (chunkWidth * blockSize);
    }
    else{
        return currentChunk;
    }
   
    return currentChunk;
}

void DetermineActiveChunks(Camera *camera){
    int i = 0;
    
    Vector3 center = DetermineCurrentChunkRegion(camera);
    int a = (int)center.x/(chunkWidth);
    int c = (int)center.z/(chunkWidth);
    
    int start_x = a - renderDistance;
    int start_z = c - renderDistance;
    
    for(int x = start_x; x < start_x + (2 * renderDistance + 1); x++){
        for(int y = start_z; y < start_z + (2 * renderDistance + 1); y++){
            if(x >= 0  && x < worldSize && y >= 0 && y < worldSize){
                activeChunks[i] = &world[x][y];
                i++;
            }
        }       
    }
}



static int SEED = 1337;

static int hash[] = {208, 34, 231, 213, 32, 248, 233, 56, 161, 78, 24, 140, 71, 48, 140, 254, 245, 255, 247, 247, 40,
                     185, 248, 251, 245, 28, 124, 204, 204, 76, 36, 1, 107, 28, 234, 163, 202, 224, 245, 128, 167, 204,
                     9, 92, 217, 54, 239, 174, 173, 102, 193, 189, 190, 121, 100, 108, 167, 44, 43, 77, 180, 204, 8, 81,
                     70, 223, 11, 38, 24, 254, 210, 210, 177, 32, 81, 195, 243, 125, 8, 169, 112, 32, 97, 53, 195, 13,
                     203, 9, 47, 104, 125, 117, 114, 124, 165, 203, 181, 235, 193, 206, 70, 180, 174, 0, 167, 181, 41,
                     164, 30, 116, 127, 198, 245, 146, 87, 224, 149, 206, 57, 4, 192, 210, 65, 210, 129, 240, 178, 105,
                     228, 108, 245, 148, 140, 40, 35, 195, 38, 58, 65, 207, 215, 253, 65, 85, 208, 76, 62, 3, 237, 55, 89,
                     232, 50, 217, 64, 244, 157, 199, 121, 252, 90, 17, 212, 203, 149, 152, 140, 187, 234, 177, 73, 174,
                     193, 100, 192, 143, 97, 53, 145, 135, 19, 103, 13, 90, 135, 151, 199, 91, 239, 247, 33, 39, 145,
                     101, 120, 99, 3, 186, 86, 99, 41, 237, 203, 111, 79, 220, 135, 158, 42, 30, 154, 120, 67, 87, 167,
                     135, 176, 183, 191, 253, 115, 184, 21, 233, 58, 129, 233, 142, 39, 128, 211, 118, 137, 139, 255,
                     114, 20, 218, 113, 154, 27, 127, 246, 250, 1, 8, 198, 250, 209, 92, 222, 173, 21, 88, 102, 219};

int noise2(int x, int y)
{
    int tmp = hash[(y + SEED) % 256];
    return hash[(tmp + x) % 256];
}

float lin_inter(float x, float y, float s)
{
    return x + s * (y - x);
}

float smooth_inter(float x, float y, float s)
{
    return lin_inter(x, y, s * s * (3 - 2 * s));
}

float noise2d(float x, float y)
{
    int x_int = x;
    int y_int = y;
    float x_frac = x - x_int;
    float y_frac = y - y_int;
    int s = noise2(x_int, y_int);
    int t = noise2(x_int + 1, y_int);
    int u = noise2(x_int, y_int + 1);
    int v = noise2(x_int + 1, y_int + 1);
    float low = smooth_inter(s, t, x_frac);
    float high = smooth_inter(u, v, x_frac);
    return smooth_inter(low, high, y_frac);
}

float perlin2d(float x, float y, float freq, int depth)
{
    float xa = x * freq;
    float ya = y * freq;
    float amp = 1.0;
    float fin = 0;
    float div = 0.0;

    int i;
    for (i = 0; i < depth; i++)
    {
        div += 256 * amp;
        fin += noise2d(xa, ya) * amp;
        amp /= 2;
        xa *= 2;
        ya *= 2;
    }

    return fin / div;
}


static float texcoordsRef[] = {
    // face 1
    0.5f, 1.0f,
    0.25f, 1.0f,
    0.25f, 0.0f,

    0.25f, 0.0f,
    0.5f, 0.0f,
    0.5f, 1.0f,


    // face 2
    0.25f, 1.0f,
    0.25f, 0.0f,
    0.5f, 0.0f,

    0.5f, 0.0f,
    0.5f, 1.0f,
    0.25f, 1.0f,

    // face 3 (top)
    0.0f, 0.0f,
    0.25f, 0.0f,
    0.25f, 1.0f,

    0.25f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    // face 4 (bottom)
    0.0f, 0.0f,
    0.25f, 0.0f,
    0.25f, 1.0f,

    0.25f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    // face 5
    0.25f, 1.0f,
    0.25f, 0.0f,
    0.5f, 0.0f,

    0.5f, 0.0f,
    0.5f, 1.0f,
    0.25f, 1.0f,

    // face 6
    0.5f, 1.0f,
    0.25f, 1.0f,
    0.25f, 0.0f,

    0.25f, 0.0f,
    0.5f, 0.0f,
    0.5f, 1.0f,
};

static float normalsRef[] = {
    // face 1
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,

    // face 2
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,

    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,
    0.0f, 0.0f, -1.0f,

    // face 3
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f,

    // face 4
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,

    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,

    // face 5
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,

    // face 6
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,

    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f,
    -1.0f, 0.0f, 0.0f};

// note: Yes, the implementation of the world model is dirty.
// todo: reimplement as chunks

static float *GetCubeVertices(float x, float y, float z)
{
    // not using indices
    float width = blockSize;
    float height = blockSize;
    float length = blockSize;
    float *cubeVertices = malloc(36 * 3 * sizeof(float));
    float ref[] = {
        // face 1
        -width / 2 + x, -height / 2 + y, length / 2 + z,
        width / 2 + x, -height / 2 + y, length / 2 + z,
        width / 2 + x, height / 2 + y, length / 2 + z,

        width / 2 + x, height / 2 + y, length / 2 + z,
        -width / 2 + x, height / 2 + y, length / 2 + z,
        -width / 2 + x, -height / 2 + y, length / 2 + z,

        // face 2
        -width / 2 + x, -height / 2 + y, -length / 2 + z,
        -width / 2 + x, height / 2 + y, -length / 2 + z,
        width / 2 + x, height / 2 + y, -length / 2 + z,

        width / 2 + x, height / 2 + y, -length / 2 + z,
        width / 2 + x, -height / 2 + y, -length / 2 + z,
        -width / 2 + x, -height / 2 + y, -length / 2 + z,

        // face 3
        -width / 2 + x, height / 2 + y, -length / 2 + z,
        -width / 2 + x, height / 2 + y, length / 2 + z,
        width / 2 + x, height / 2 + y, length / 2 + z,

        width / 2 + x, height / 2 + y, length / 2 + z,
        width / 2 + x, height / 2 + y, -length / 2 + z,
        -width / 2 + x, height / 2 + y, -length / 2 + z,

        // face 4
        -width / 2 + x, -height / 2 + y, -length / 2 + z,
        width / 2 + x, -height / 2 + y, -length / 2 + z,
        width / 2 + x, -height / 2 + y, length / 2 + z,

        width / 2 + x, -height / 2 + y, length / 2 + z,
        -width / 2 + x, -height / 2 + y, length / 2 + z,
        -width / 2 + x, -height / 2 + y, -length / 2 + z,

        // face 5
        width / 2 + x, -height / 2 + y, -length / 2 + z,
        width / 2 + x, height / 2 + y, -length / 2 + z,
        width / 2 + x, height / 2 + y, length / 2 + z,

        width / 2 + x, height / 2 + y, length / 2 + z,
        width / 2 + x, -height / 2 + y, length / 2 + z,
        width / 2 + x, -height / 2 + y, -length / 2 + z,

        // face 6
        -width / 2 + x, -height / 2 + y, -length / 2 + z,
        -width / 2 + x, -height / 2 + y, length / 2 + z,
        -width / 2 + x, height / 2 + y, length / 2 + z,

        -width / 2 + x, height / 2 + y, length / 2 + z,
        -width / 2 + x, height / 2 + y, -length / 2 + z,
        -width / 2 + x, -height / 2 + y, -length / 2 + z};

    for (int i = 0; i < 36 * 3; i++)
    {
        cubeVertices[i] = ref[i];
    }
    
    return cubeVertices;
}


Model GetChunkModel(Chunk *chunk)
{   
    Mesh mesh = {0};
    mesh.vboId = (unsigned int *)RL_CALLOC(MAX_MESH_VBO, sizeof(unsigned int));

    float *vertices = RL_MALLOC(36 * 3 * chunkWidth * chunkHeight * chunkWidth * sizeof(float));
    float *texcoords = RL_MALLOC(36 * 2 * chunkWidth * chunkHeight * chunkWidth * sizeof(float));
    float *normals = RL_MALLOC(36 * 3 * chunkWidth * chunkHeight * chunkWidth * sizeof(float));

    int verticesCount = 0;
    int texcoordsCount = 0;
    int normalsCount = 0;


    int u = 0;
    while(u < chunk->number_blocks){
        Block b = chunk->blocks_array[u]; 
        float *blockVertices = GetCubeVertices(b.x, b.y, b.z);
        for (int v = 0; v < 36 * 3; v++){
            vertices[verticesCount + v] = blockVertices[v];
        }
        for (int t = 0; t < 36 * 2; t++){
            texcoords[texcoordsCount + t] = texcoordsRef[t];
        }
        for (int n = 0; n < 36 * 3; n++){
            normals[normalsCount + n] = normalsRef[n];
        }
        verticesCount += 36 * 3;
        texcoordsCount += 36 * 2;
        normalsCount += 36 * 3;
  
        u++;

    }
   


    mesh.vertices = (float *)RL_MALLOC(verticesCount * sizeof(float));
    memcpy(mesh.vertices, vertices, verticesCount * sizeof(float));

    mesh.texcoords = (float *)RL_MALLOC(texcoordsCount * sizeof(float));
    memcpy(mesh.texcoords, texcoords, texcoordsCount * sizeof(float));

    mesh.normals = (float *)RL_MALLOC(normalsCount * sizeof(float));
    memcpy(mesh.normals, normals, normalsCount * sizeof(float));

    mesh.vertexCount = verticesCount / 3;         // fixme: Why divide by 3 ???
    mesh.triangleCount = (verticesCount / 3) / 2; // fixme: Why divide by 3 and 2 ???

    RL_FREE(vertices);
    RL_FREE(texcoords);
    RL_FREE(normals);

    rlLoadMesh(&mesh, false);

    Model chunkModel = LoadModelFromMesh(mesh);

    chunkModel.materials[0].maps[MAP_DIFFUSE].texture = LoadTexture("grass.png");

    return chunkModel;
}



