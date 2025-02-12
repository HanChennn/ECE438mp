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
#include<fstream>
#include<stack>
#include<set>
#include<list>
#include<cstdio>
#include<cmath>

#define inf 2147483647
//(1ll<<31)-1
#define Max_distance 1e6

using namespace std;

FILE *fpOut;
struct Message{
    int src;
    int dest;
    string msg;
    Message(int u, int v, const string& m):src(u),dest(v),msg(m){}
};
set< int> nodes;
map< int, map<int,int> > edge;
map< int, map<int,pair<int,int> > > dis_table;
list<Message> msg_list;


class cmp{
	public:
		bool operator()(const pair<int, int> &o1,const pair<int, int> &o2)const{
			if(o1.first==o2.first) return o1.second>o2.second;
			return o1.first>o2.first;
		}
};

int read_graph(string fname){
    FILE* fp = fopen(fname.c_str(),"r");
    if(fp==NULL)return -1;
    int u,v,w;
    while(fscanf(fp,"%d %d %d",&u,&v,&w)==3){
        edge[u][v] = w;
        edge[v][u] = w;
        if(nodes.find(u)==nodes.end())edge[u][u]=0,nodes.insert(u);
        if(nodes.find(v)==nodes.end())edge[v][v]=0,nodes.insert(v);
    }
    // for (auto iter : nodes) {
    //     edge[iter][iter] = 0;
    // }
    fclose(fp);
    return 0;
}

void print_topo(){
    int nxt;
    for(auto src:nodes){
        for(auto dest:nodes){
            nxt = dest;
            if(dis_table[src][dest].first==inf)continue;
            while(dis_table[src][nxt].second!=src){
                nxt = dis_table[src][nxt].second;
            }
            fprintf(fpOut, "%d %d %d\n", dest, nxt, dis_table[src][dest].first);
        }
    }return;
}

void dij(){
    map<int,bool> vis;
    priority_queue< pair<int,int>, vector< pair<int,int> >, cmp > pq;
    for(auto src:nodes){
        for(auto u:nodes){
            dis_table[src][u] = make_pair(inf,-1);
            dis_table[u][u] = make_pair(0,u);
            vis[u] = false;
        }
        dis_table[src][src] = make_pair(0,src);
        while(!pq.empty())pq.pop();
        pq.push(make_pair(0,src));
        while(!pq.empty()){
            int u = pq.top().second; pq.pop();
            if(vis[u])continue;
            vis[u]=true;
            // for(auto p:edge[u]){
            for(map<int,int> ::iterator p=edge[u].begin();p!=edge[u].end();p++){
                int v = (*p).first, w = (*p).second;
                if(vis[v])continue;
                if(dis_table[src][v].first>dis_table[src][u].first+w){
                    dis_table[src][v] = make_pair(dis_table[src][u].first+w,u);
                    pq.push(make_pair(dis_table[src][v].first,v));
                }
                else if(dis_table[src][v].first == dis_table[src][u].first+w && dis_table[src][v].second>u){
                    dis_table[src][v] = make_pair(dis_table[src][u].first+w,u);
                    pq.push(make_pair(dis_table[src][v].first,v));
                }
            }
        }
    }
    print_topo();
    return;
}

int read_msgfile(string fname){
    ifstream msgfile(fname);
    if(msgfile.is_open()==false)return -1;
    int src,dest;
    string info;
    while(msgfile>>src>>dest){
        getline(msgfile,info);
        if(!info.empty() && info[0]==' ') info = info.substr(1);
        msg_list.push_back(Message(src,dest,info));
    }return 0;
}

void send_msg(){
    int nxt, cost;
    for(list<Message>::iterator p = msg_list.begin(); p!=msg_list.end(); p++){
        stack<int> s;
        nxt = (*p).dest;
        cost = dis_table[(*p).src][(*p).dest].first;
        fprintf(fpOut, "from %d to %d cost ", (*p).src, (*p).dest);
        if(cost == inf) fprintf(fpOut,"infinite hops unreachable ");
        else{
            fprintf(fpOut, "%d hops ", cost);
            while(nxt != (*p).src && dis_table[(*p).src][nxt].second!=-1){
                s.push(nxt);
                nxt = dis_table[(*p).src][nxt].second;
            }
            s.push(nxt);
            while(s.size()>1){
                fprintf(fpOut, "%d ",s.top());
                s.pop();
            }
        }
        fprintf(fpOut, "message %s\n", (*p).msg.c_str());
    }return;
}

int read_changefile(string fname){
    ifstream changefile(fname);
    if(changefile.is_open()==false)return -1;
    int u,v,w;
    while(changefile>>u>>v>>w){
        edge[u][v] = w;
        edge[v][u] = w;
        if(nodes.find(u)==nodes.end())edge[u][u]=0,nodes.insert(u);
        if(nodes.find(v)==nodes.end())edge[v][v]=0,nodes.insert(v);
        for (auto iter : nodes) {
            edge[iter][iter] = 0;
        }
        dij();
        send_msg();
    }
    return 0;
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }
    int re=0;
    fpOut = fopen("output.txt", "w");
    if((re = read_graph(argv[1])) == -1)exit(1);
    dij();
    if((re = read_msgfile(argv[2])) == -1) exit(1);
    send_msg();
    if((re = read_changefile(argv[3])) == -1) exit(1);
    fclose(fpOut);
    return 0;
}

