#ifndef CHUNK_H
#define CHUNK_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include "Block.h"

#define CHUNK_X 2
#define CHUNK_Y 2
#define CHUNK_Z 2

#define V_SIZE 3
#define V_TRIANGLE 9
#define V_FACE 18
#define V_QUAD 36
#define V_SIZE_FLOATS (V_SIZE * V_QUAD)

enum Face {
    NORTH,
    SOUTH,
    WEST,
    EAST,
    TOP,
    BOTTOM
} face;

float northVertices[] = {
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f
};

float southVertices[] = {
    -0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f
};

float westVertices[] = {
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
};

float eastVertices[] = {
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f
};

float topVertices[] = {
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f
};

float bottomVertices[] = {
    -0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
};

float vertices[] = {
    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,

    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,

    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,

    -0.5f, -0.5f, -0.5f,
    0.5f, -0.5f, -0.5f,
    0.5f, -0.5f,  0.5f,
    0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,

    -0.5f,  0.5f, -0.5f,
    0.5f,  0.5f, -0.5f,
    0.5f,  0.5f,  0.5f,
    0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f
}; 


class Chunk {
    public:
    Block blocks[CHUNK_X][CHUNK_Y][CHUNK_Z];
    float vertex[V_SIZE_FLOATS * CHUNK_X * CHUNK_Y * CHUNK_Z];
    unsigned int vbo;
    unsigned int vao;
    unsigned int count;
    bool changed;

    Chunk() {
        memset(blocks, NULL, sizeof(blocks));
        count = 0;
        changed = true;

        int i = 0;
        for(int x = 0; x < CHUNK_X; x++){
            for(int y = 0; y < CHUNK_Y; y++){
                for(int z = 0; z < CHUNK_Z; z++){
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, northVertices, x, y, z);
                    }
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, southVertices, x, y, z);
                    }
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, westVertices, x, y, z);
                    }
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, eastVertices, x, y, z);
                    }
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, topVertices, x, y, z);
                    }
                    if(!hasNeighbour(face, x,y,z)){
                        createFaceVertices(&i , vertex, bottomVertices, x, y, z);
                    }
                }
            }
        }
        

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex), vertex, GL_STATIC_DRAW);
        
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0); 
        count = CHUNK_X * CHUNK_Y * CHUNK_Z;
    }

    ~Chunk() {
        //glDeleteBuffers(1, &vbo);
        //glDeleteVertexArrays(1, &vao); segfault?
    }

    void createFaceVertices(int *index, float *targetPtr, float *facePtr, int x, int y, int z){
        for(int i = 0; i < V_FACE; i+=3){
            targetPtr[(*index)++] = facePtr[i] + x ;
            targetPtr[(*index)++] = facePtr[i+1] + y;
            targetPtr[(*index)++] = facePtr[i+2] + z;
        }
    }

    bool hasNeighbour(Face face, int x, int y, int z){
        float neighbour;
        switch(face){
            case NORTH: neighbour = get(x,y+1,z); break;
            default: break;
        }
        return false;
    }

    Block get(unsigned int x, unsigned int y, unsigned int z){
        return blocks[x][y][z];
    }

    void set(unsigned x, unsigned y, unsigned z, int block_value){
        blocks[x][y][z] = Block(block_value);
        changed = true;
    }

    void update(){
    }

    void render(){
        if(changed){
            update();
        }

        // If this chunk is empty, we don't need to draw anything.
        if(!count){
            return;
        }
        glEnable(GL_CULL_FACE);  



        glBindVertexArray(vao);
        //glBindBuffer(GL_ARRAY_BUFFER, vbo);
        //glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 3 * sizeof(GLfloat), 0);
        glDrawArrays(GL_TRIANGLES, 0, count * V_QUAD);
    }
};

#endif
