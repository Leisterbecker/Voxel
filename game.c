/*******************************************************************************************
*
*  Voxel Game
*
*
*  ToDo/Ideas:
*
*    Iterate over all blocks: Check if block-side is empty, mark in integer
*    In renderer: calculate all visible block sides
*  
*    block-int: 10 bits free
*    0000 0000 0000 0000 0000 0000 0000 0000
*                |      |         |    |    |
*                 faces     y       x     z
*
*    faces: 6 bits set 1 for neighbor or 0 if not -> (faces != 0 ? render faces that are 1 : dont render anything, dont put in list)    
*
*    Determine list of visible blocks:   
*
*      + blocklist is list of block-ints
*      + visible blocks is list of pointers to block ints of the blocklist (visible blocks are part of chunk)
*
*          iterate over every block of chunk: check 3d neighborhood and mark blockface if neighbor exists, enums for blockfaces needed
*
*
*
*
*    Raycast visible chunks: 
*
********************************************************************************************/

#include "raylib.h"
#include "rlgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#define MAX_MESH_VBO 7
#define Y_MASK 1044482
#define X_MASK 240
#define Z_MASK 15


const int chunkWidth = 16;
const int chunkHeight = 256;  
const int blockSize =  1.0f;
const int worldSize = 10 * 10;
const int worldLength = 10;

float y_offset = 0.0f;
static float yl = 0.0f;

int wires = 0;


typedef struct {
    int x;
    int z;
    int blocks_array[16 * 16 * 256];
    Model model;
    int loaded;
    int number_blocks;
} Chunk;

Chunk world[10 * 10];

const int renderDistance = 1; //MAXIMUM
//(2*rd+1)^2
Chunk *activeChunks[(2*1+1) * (2*1+1)];

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
void DrawBlock(int color, Chunk c, int block);
void markBlockfaces(int *block, Chunk *chunk);

Chunk createChunkRegion();
int createBlock(int x, int z, int y);
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
    float cam_start_y = 250.0f;
   
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



    puts("Create world");
    createWorld();
    puts("Finished");
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        if(IsKeyPressed(KEY_R)) wires = !wires;
        UpdateCamera(&camera);
        DetermineActiveChunks(&camera);
        GetYOffset();

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();    
            
            if(!wires){
                ClearBackground(BLACK);
            }
            else{
                ClearBackground(BLUE);
            }

            
            DrawText(TextFormat("Chunk x: %i", (int)camera.position.x/32), 10, 30, 8, BLACK);
            DrawText(TextFormat("Chunk z: %i", (int)camera.position.z/32), 130, 30, 8, BLACK);
            DrawText(TextFormat("Cam x: %f, cam z:%f",camera.position.x,camera.position.z), 10, 60, 8, BLACK);


            BeginMode3D(camera);
                DrawActiveChunks(&camera);
                if(wires)    DrawGrid(100,32.0);

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
        y_offset = .08f;
    } 
    else if(IsKeyDown(KEY_E)){
        y_offset = -.08f;
    }
    else{
        y_offset = 0.0f;
    }

}



int createBlock(int x, int z, int y){
    int block = 0;
    block |= ((y << 12) | (x << 4) | (z) | (1 << 31));
    return block;
}

Chunk createChunkRegion(int x, int z){    
    int u = 0;
    char *a = malloc(100 * sizeof(char));

    Chunk chunk;
    chunk.x = x;
    chunk.z = z;

    chunk.loaded = 0;
    float tmp_y = 0;

    
    for(int i = 0; i < chunkWidth; i++){
        for(int j = 0; j < chunkWidth; j++){
            for(int k = 0; k < chunkHeight; k++){
                tmp_y = chunkHeight * perlin2d((i * blockSize)+(chunk.x * chunkWidth* blockSize), (j * blockSize)+(chunk.z * chunkWidth* blockSize), 0.02f, 5);
                if(k <= tmp_y){
                    chunk.blocks_array[u] = createBlock(i,j,k);
                    u++;
                }
            }    
        }
    }
    chunk.number_blocks = u;

    //check for neighbour blocks, mark neighboured faces with 1, blockface bits in order: UPPER LOWER LEFT RIGHT FRONT BACK
    
    for(int i = 0; i < chunk.number_blocks; i++){
        markBlockfaces(&chunk.blocks_array[i], &chunk);
    }
    
    return chunk;
}


void markBlockfaces(int *block, Chunk *chunk){
    int x = (*block & X_MASK) >> 4;
    int z = *block & Z_MASK;
    int y = (*block & Y_MASK) >> 12;

    int index = -1;
    int MSB = 1 << (32 - 1);


    if(y-1 >= 0){
        index = z * chunkWidth * chunkHeight + ((y-1) * chunkWidth) + x; //Upper Side: y-1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 21;
    }

    if(y+1 < chunkHeight){
        index = z * chunkWidth * chunkHeight + ((y+1) * chunkWidth) + x; //Lower Side: y+1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 22;
    }

    if(x-1 >= 0){
        index = z * chunkWidth * chunkHeight + (y * chunkWidth) + (x-1); //Left Side: x-1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 23;
    }
    
    if(x+1 < chunkWidth){
        index = z * chunkWidth * chunkHeight + (y * chunkWidth) + (x+1); //Right Side: x+1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 24;
    }

    if(z-1 >= 0){
        index = (z-1) * chunkWidth * chunkHeight + (y * chunkWidth) + x; //Front Side: z-1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 25;
    }
    
    if(z+1 < chunkWidth){
        index = (z+1) * chunkWidth * chunkHeight + (y * chunkWidth) + x; //Back Side: z+1
        if(chunk->blocks_array[index] & MSB) *block |= 1 << 26;
    }

}


void createWorld(){
    int i;
    for(i = 0; i < worldSize; i++){
        world[i] = createChunkRegion(i%worldLength,i/worldLength); 
    }     
}





void DrawChunkRegion(int color, Chunk *chunk){
    if(!chunk->loaded){
        chunk->model = GetChunkModel(chunk);
        chunk->loaded = 1;
    }
    yl = yl + y_offset;
    if(!wires){
         DrawModelWires(chunk->model, (Vector3){chunk->x * chunkWidth, yl, chunk->z * chunkWidth}, 1.0f, WHITE);
    }
    else{
        DrawModel(chunk->model, (Vector3){chunk->x * chunkWidth, yl, chunk->z * chunkWidth}, 1.0f, WHITE);
    }

}

void DrawActiveChunks(Camera *camera){
    puts("DrawActiveChunks");
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

//raycast the chunks from character view, not behind camera
void DetermineActiveChunks(Camera *camera){
    int i = 0;
    
    Vector3 center = DetermineCurrentChunkRegion(camera);
    int a = (int)center.x/(chunkWidth);
    int c = (int)center.z/(chunkWidth);
    
    int start_x = a - renderDistance;
    int start_z = c - renderDistance;
    
    for(int x = start_x; x < start_x + (2 * renderDistance + 1); x++){
        for(int z = start_z; z < start_z + (2 * renderDistance + 1); z++){
            if(x >= 0  && x < worldLength && z >= 0 && z < worldLength){
                activeChunks[i] = &world[x + worldLength * z];
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


//take another arg: faces -> calculates the specified faces
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
// iterate only over visible blocks, calculate visible block faces and render
    while(u < chunk->number_blocks){
        int b = chunk->blocks_array[u]; 
        
        float *blockVertices = GetCubeVertices((b & X_MASK) >> 4, (b & Y_MASK) >> 12, b & Z_MASK);
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
  
    Model chunkModel = {0};

    if(wires){
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
		

		chunkModel = LoadModelFromMesh(mesh);
		chunkModel.materials[0].maps[MAP_DIFFUSE].texture = LoadTexture("grass.png");

    }
    else{
        mesh.vertices = (float *)RL_MALLOC(verticesCount * sizeof(float));
		memcpy(mesh.vertices, vertices, verticesCount * sizeof(float));


		mesh.normals = (float *)RL_MALLOC(normalsCount * sizeof(float));
		memcpy(mesh.normals, normals, normalsCount * sizeof(float));

		mesh.vertexCount = verticesCount / 3;         // fixme: Why divide by 3 ???
		mesh.triangleCount = (verticesCount / 3) / 2; // fixme: Why divide by 3 and 2 ???

		RL_FREE(vertices);
		RL_FREE(texcoords);
		RL_FREE(normals);

		rlLoadMesh(&mesh, false);
		

		chunkModel = LoadModelFromMesh(mesh);
    }

    return chunkModel;
}



