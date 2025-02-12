#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<cstring>
#include<string>
#include<map>
#include<iostream>
#include<vector>
#include<queue>
#include<cmath>
#include<sstream>
#include<fstream>

#define inf (int)((1ll<<31)-1)

using namespace std;

typedef pair<int, int> Pair;

struct vertex{
    int id;
    map<int, int> dist;
    map<int, int> prev;
};

int num_vertex;

// ofstream output("output.txt");
ifstream changeFile;

void BF_dist(vertex* u, vector<Pair> edge[]){
    (*u).dist.clear();
    (*u).prev.clear();
    for(int i=1; i<=num_vertex;i++){
        (*u).dist[i]=inf;
        (*u).prev[i]=-1;
    }
    (*u).dist[(*u).id]=0;
    for(int i=0; i< num_vertex-1; i++){
        for(int j=1; j<= num_vertex; j++){
            for(int k=0; k<edge[j].size(); k++){
                int v = edge[j][k].first;
                int w = edge[j][k].second;
                if((*u).dist[j]!=inf && (*u).dist[j]+w < (*u).dist[v]){
                    (*u).dist[v]=(*u).dist[j]+w;
                    (*u).prev[v]=j;
                }else if((*u).dist[j]!=inf && (*u).dist[j]+w == (*u).dist[v] && j<(*u).prev[v]){
                    (*u).dist[v]=(*u).dist[j]+w;
                    (*u).prev[v]=j;
                }
            }
        }
    }return;
}

vector<int> ShowPath(int src, int dest, vertex* vertices){
    int now = dest;
    vector<int> path;
    vertex s = vertices[src];
    // if(src == dest){
    //     // path.push_back(src);
    //     return path;
    // }
    while(now!=src && now!=-1){
        if(now!=dest) path.push_back(now);
        now = s.prev[now];
    }
    return path;
}

void get_num_vertex(char** argv){
    string src, dest, w;
    int u, v;
    ifstream numFile;
    numFile.open(argv[1]);

    while(numFile >> src >> dest >> w){
        u = stoi(src);
        v = stoi(dest);
        if(u > num_vertex) num_vertex = u;
        if(v > num_vertex) num_vertex = v;
    }
    numFile.close();
    return;
}

void readgraph(char** argv, vector<Pair> edge[]){
    string u, v, w;
    int u_num, v_num, w_num;
    ifstream inFile;
    inFile.open(argv[1]);
    while(inFile>>u>>v>>w){
        u_num = stoi(u);
        v_num = stoi(v);
        w_num = stoi(w);
        if(w_num != -999){
            edge[u_num].push_back(make_pair(v_num,w_num));
            edge[v_num].push_back(make_pair(u_num,w_num));
        }
    }
    inFile.close();
    return;
}

void updating_tables(vertex* vertices, vector<Pair> edge[], FILE* fpOut){
    for(int i=1; i<=num_vertex; i++){
        BF_dist(&vertices[i], edge);
    }
    for(int i=1; i<=num_vertex; i++){
        for(int j=1; j<=num_vertex; j++){
            if(vertices[i].dist[j]!=inf){
                vector<int> path=ShowPath(i,j,vertices);
                fprintf(fpOut, "%d %d %d\n", j, path.empty()?j:path.back(), vertices[i].dist[j]);
            }
        }
    }return;
}

void send_msg(FILE* fpOut, char** argv, vertex* vertices){
    ifstream inFile;
    inFile.open(argv[2]);
    string words;
    int src,dest;
    
    while(getline(inFile, words)){
        if(words == "")continue;
        stringstream ss(words);
        string msg;
        ss>>src>>dest;

        string word;
        while(ss>>word) msg = msg + word + " ";
        msg.pop_back();

        if(vertices[src].dist[dest] != inf){
            vector<int> path = ShowPath(src, dest, vertices);
            // fprintf(fpOut, "from %d to %d cost %d hops %d ", src, dest, vertices[src].dist[dest], src);
            path.push_back(src);
            fprintf(fpOut, "from %d to %d cost %d hops ", src, dest, vertices[src].dist[dest]);
            for(int i = path.size()-1; i>=0; i--){
                fprintf(fpOut, "%d ",path[i]);
            }
            fprintf(fpOut, "message %s\n", msg.c_str());
        }else{
            fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n", src, dest, msg.c_str());
        }
    }
    inFile.close();return;
}

int graphchange(vector<Pair>edge[]){
    string src, dest, dis;
    int u,v,w;
    if(changeFile.is_open()==false)return 0;
    if(changeFile>>src>>dest>>dis){
        u = stoi(src);
        v = stoi(dest);
        w = stoi(dis);
        for(int i=0; i<edge[u].size(); i++){
            if(edge[u][i].first == v){
                edge[u].erase(edge[u].begin()+i);
                break;
            }
        }
        for(int i=0; i<edge[v].size(); i++){
            if(edge[v][i].first == u){
                edge[v].erase(edge[v].begin()+i);
                break;
            }
        }
        if(w == -999)return 1;
        edge[u].push_back(make_pair(v,w));
        edge[v].push_back(make_pair(u,w));
        return 1;
    }return 0;
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    num_vertex = 0;
    get_num_vertex(argv);
    vector<Pair> edge[num_vertex+1];

    readgraph(argv, edge);

    vertex* vertices = new vertex[num_vertex+1];
    for(int i=1; i<=num_vertex; i++){
        vertices[i].id = i;
        for(int j=1; j<=num_vertex; j++){
            vertices[i].dist[j] = inf;
            vertices[i].prev[j] = -1;
        }
    }

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    changeFile.open(argv[3]);
    while(1){
        updating_tables(vertices, edge, fpOut);
        send_msg(fpOut, argv, vertices);
        if(graphchange(edge))continue;
        else break;
    }

    changeFile.close();
    fclose(fpOut);
    return 0;
}

