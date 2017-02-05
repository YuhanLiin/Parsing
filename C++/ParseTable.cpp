#include "ParseTable.h"
//#include <iostream>

ParseTable::ParseTable(int x, int y, int initial){
    xmax = x;
    ymax = y;
    data = new int[xmax*ymax];
    for (int i=0; i<xmax*ymax; i++){
        data[i] = initial;
    }
}

int &ParseTable::operator()(int x, int y){
    return data[x*ymax + y];
}

ParseTable::~ParseTable(){
    delete[] data;
}

// int main(){
//     ParseTable &table = ParseTable(3, 5, -1);
//     table(0,0) = 4;
//     table(2,4) = 5;
//     for (int i=0; i<3; i++){
//         for (int j=0; j<5; j++){
//             std::cout << table(i,j);
//         }
//     }
// }