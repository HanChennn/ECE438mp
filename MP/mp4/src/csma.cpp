#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
#include<cstring>
#include<fstream>
#include<map>
#include<string.h>
#include<iostream>
#include<algorithm>
#include<vector>

using namespace std;

typedef struct{
    int id, R, back;
} node_t;

typedef struct{
    int N, L, M, trans_state;
    unsigned long T, cur, success_trans;
    vector<int> R;
    vector<node_t> nodes;
} csma_t;

static csma_t csma;

void read_file(string file){
    csma.cur = 0;
    csma.trans_state = 0;
    csma.success_trans = 0;
	ifstream infile;
    string line;
	infile.open(file);
    while (getline(infile, line)) {
        // char* str = const_cast<char *>(line.c_str());
        char* str = new char[line.size() + 1];
        strcpy(str, line.c_str());
        long val = 0 ;
        switch(*str){
            case 'N':
                sscanf(str + 1, "%d", &csma.N); break;
            case 'L':
                sscanf(str + 1, "%d", &csma.L); break;
            case 'M':
                sscanf(str + 1, "%d", &csma.M); break;
            case 'T':
                sscanf(str + 1, "%lu", &csma.T); break;
            case 'R':
                val = strtol(str + 1, &str, 10);
                while (val) {
                    csma.R.push_back(val);
                    val = strtol(str, &str, 10);
                }
                break;
            default:
                break;
        }
    }
    for (int i = 0; i < csma.N; i++) {
        node_t node = {i, 0, i % csma.R[0]};
        csma.nodes.push_back(node);
    }
    infile.close();
}

int work(){
    vector<int> line;
    for (int i = 0; i < csma.N; i++){
        if (csma.nodes[i].back == 0) line.push_back(i);
    }
    switch(line.size()){
        case 1:
            csma.nodes[line[0]].R = 0;
            csma.nodes[line[0]].back = (csma.nodes[line[0]].id + csma.cur + csma.L) % csma.R[csma.nodes[line[0]].R];
            if (csma.cur + csma.L < csma.T) {
                csma.cur += csma.L - 1;
                csma.success_trans += csma.L;
            }
            else {
                csma.success_trans += csma.T - csma.cur; 
                csma.cur = csma.T - 1;
            }
            break;
        case 0:
            for (int i = 0; i < csma.N; i++) csma.nodes[i].back--;
            break;
        default:
            if (line.size() > 1) {
            for (int i = 0; i < line.size(); i++) {
                    if (++csma.nodes[line[i]].R == csma.M) csma.nodes[line[i]].R = 0;
                    csma.nodes[line[i]].back = (csma.nodes[line[i]].id + csma.cur + 1) % csma.R[csma.nodes[line[i]].R];
                }
            }
    }
    return csma.cur++;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: ./csma input.txt\n");
        return -1;
    }
    FILE* outfile = fopen("output.txt", "w");
    read_file(argv[1]);
    while(work() != csma.T - 1);
    fprintf(outfile, "%.2lf\n", (double) csma.success_trans / (double) csma.T);
    fclose(outfile);
    return 0;
}
