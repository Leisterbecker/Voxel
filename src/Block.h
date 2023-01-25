#ifndef CHUNK_H
#define CHUNK_H

class Block {
    public:
    int type;

    Block(){
        type = 0;
    }

    Block(int _type){
        type = _type;
    }

    ~Block(){
        
    }
#endif