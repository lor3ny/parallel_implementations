#include <cstddef>
#include <iostream>
#include <map>
#include <mpi.h>
#include <vector>
#include <random>
#include <list>
#include<cstdlib>

#define N 20
#define entryNode 0
#define endNode (N)-1

using namespace std;


// NOTES
/*
- it is possible to parallelize the cicles with OpenMP? (if not, using CUDA)
- nodeQueue insert at the start and remove from the end (FIFO)
*/


void initializeMatrix(int* data){
    srand((unsigned) time(NULL));
    for(int i = 0; i<N; i++){
        for(int j = 0; j<N; j++){
            int random = 0 + (rand() % 10);
            if(random < 1){
                data[i*N + j] = 1;
            } else {
                data[i*N + j] = 0;
            }
            cout << data[i*N + j];
        }
        cout << endl;
    }
}



int main(int argc, char *argv[]){
    

    MPI_Init(&argc, &argv);

    int procsCount;
    int procId;
    MPI_Comm_size(MPI_COMM_WORLD, &procsCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &procId);

    int graph[N*N];

    int startNode;
    bool entryCheck = false;
    bool endCheck = false;
    int sigtermCheck = 0;

    if(procId == 0){

        //Maybe its better to choose in a more sparse way
        int starters[procsCount];
        for(int i = 0; i<procsCount; i++){
            if(i == 0){
                starters[i] = entryNode;
                continue;
            }
            
            if(i == procsCount-1){
                starters[i] = endNode;
                continue;
            }

            starters[i] = i;
        }
        
        //Send Entry Nodes
        MPI_Scatter(starters, 1, MPI_INT, &startNode, 1, MPI_INT, 0, MPI_COMM_WORLD);

        initializeMatrix(graph);
        MPI_Bcast(graph, N*N, MPI_INT, 0, MPI_COMM_WORLD);


        entryCheck = true;

        // BFS COMPUTATION
        vector<int> path;
        vector<bool> visited;
        for (int i = 0; i < N; i++)
            visited.push_back(false);

        list<int> nodeQueue;

        visited[startNode] = true;
        nodeQueue.push_back(startNode);

        list<int>::iterator i;

        MPI_Request request;
        MPI_Irecv(&sigtermCheck, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &request); 
        
        while (!endCheck) {

            if(sigtermCheck){
                break;
            }  

            if(nodeQueue.empty()){
                break;
            }

            int currVertex = nodeQueue.front();
            nodeQueue.pop_front();

            path.push_back(currVertex);

            for (int i = 0; i< N; ++i) {

                int value = graph[currVertex*N + i];
                if(i == endNode && value == 1){
                    endCheck = true;
                    currVertex = i;
                    path.push_back(currVertex);
                    break;
                }

                if (value == 1 && !visited[i]) {
                    visited[i] = true;
                    nodeQueue.push_back(i);
                }
            }
        }

        //Send completition
        if(endCheck){
            sigtermCheck = 1;
            MPI_Bcast(&sigtermCheck, 1, MPI_INT, 0, MPI_COMM_WORLD);
        }

        cout << procId << " start node: " << startNode << " endcheck: " << endCheck << " startcheck: "<< entryCheck << " visited:";
        for (int i = 0; i < path.size(); i++) {
            cout << path[i] << " - ";
        }   
        cout << endl;

    } else {

        // INITIALIZATION

        // Recv entry point
        MPI_Scatter(nullptr, 0, MPI_INT, &startNode, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if(startNode == endNode){
            endCheck = true;
        }

        // Recv the matrix
        MPI_Bcast(graph, N*N, MPI_INT, 0, MPI_COMM_WORLD);

        // BFS Computation
        vector<int> path;
        vector<bool> visited;
        for (int i = 0; i < N; i++)
            visited.push_back(false);

        list<int> nodeQueue;

        visited[startNode] = true;
        nodeQueue.push_back(startNode);

        list<int>::iterator i;

        MPI_Request request;
        MPI_Ibcast(&sigtermCheck, 1, MPI_INT, 0, MPI_COMM_WORLD, &request); 

        int checkPass = -1; 
        
        while (!endCheck || !entryCheck) {

            if(sigtermCheck){
                break;
            }  

            if(nodeQueue.empty()){
                break;
            }


            int currVertex;
            if(checkPass != -1){
                currVertex = checkPass;
                checkPass = -1;
            } else {
                currVertex = nodeQueue.front();
                nodeQueue.pop_front();
            }


            // Verifica solo quando lo seleziona, ha senso verificarlo quando lo trova?

            path.push_back(currVertex);

            for (int i = 0; i< N; ++i) {

                int value = graph[currVertex*N + i];


                if(i == endNode && value == 1){
                    if(entryCheck){
                        endCheck = true;
                        currVertex = i;
                        path.push_back(i);
                        break;
                    } else {
                        endCheck = true;
                        visited[i] = true;
                        checkPass = i;
                        continue;
                    }
                }

                if(i == entryNode && value == 1){
                    if(endCheck){
                        entryCheck = true;
                        currVertex = i;
                        path.push_back(i);
                        break;
                    } else {
                        entryCheck = true;
                        visited[i] = true;
                        checkPass = i;
                        continue;
                    }
                }

                if (value == 1 && !visited[i]) {
                    visited[i] = true;
                    nodeQueue.push_back(i);
                }
            }
        }

        // Send completition
        if(sigtermCheck == 0 && (endCheck && entryCheck)){
            sigtermCheck = 1;
            MPI_Send(&sigtermCheck, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        
        cout << procId << " start node: " << startNode << " endcheck: " << endCheck << " startcheck: "<< entryCheck << " sigterm: "<<sigtermCheck <<  " visited:";
        for (int i = 0; i < path.size(); i++) {
            cout << path[i] << " - ";
        }   
        cout << endl;

    }   
    


    MPI_Finalize();

    
}