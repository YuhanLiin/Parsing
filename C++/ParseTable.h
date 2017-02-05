#ifndef PARSETABLE_H
#define PARSETABLE_H

//Class for 2d array. Used as parse table
class ParseTable{
private:
    int *data;
    int xmax;
    int ymax;

public:
    //Creates table of set size with init values
    ParseTable(int x, int y, int initial);
    //No copying or assigning
    ParseTable(ParseTable&) = delete;
    ParseTable& operator=(ParseTable&) = delete;
    //Query with 2 dimensions
    int& operator()(int x, int y);
    ~ParseTable();
};

#endif