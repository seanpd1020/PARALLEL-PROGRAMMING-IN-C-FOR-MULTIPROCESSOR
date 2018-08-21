#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

#define RAND_TOP 100 //rand 數字的最大值

//比較的函式
int compare(const void* ap, const void* bp){
        int a = *((const int*)ap);
        int b = *((const int*)bp);

        if(a < b)return -1;
        else if (a > b)return 1;
        else return 0;
}

//在data[]中找出數字最大值的index
int max_index(int* data, int n){
        int i, max = data[0], maxi = 0;

        for(i = 1; i < n; i++){
                if(data[i] > max){
                        max = data[i];
                        maxi = i;
                }
        }
        return maxi;
}

//在data[]中找出數字最小值的index
int min_index(int* data, int n){
        int i, min = data[0], mini = 0;

        for(i = 1; i < n; i++){
                if(data[i] < min){
                        min = data[i];
                        mini = i;
                }
        }
        return mini;
}

//parallel odd/even sort
void parallel_sort(int* data, int id, int size, int n){
        int i;
        int buffer[n];//用來讀取partner data[]的陣列

        for(i = 0; i < size; i++){
                int partner;

                //even phase
                if(i % 2 == 0){
                        //even process
                        if(id % 2 == 0){
                                partner = id + 1;
                        }
                        //odd process
                        else{
                                partner = id - 1;
                        }
                }
                //odd phase
                else{
                        //even process
                        if(id % 2 == 0){
                                partner = id - 1;
                        }
                        //odd process
                        else{
                                partner = id + 1;
                        }
                }

                //如果partener < 0 or > size-1 代表超出範圍，則跳過此輪迴圈
                if(partner < 0 || partner >= size)continue;

                //even process 先 send 後 recv，odd process 先 recv 後 send
                if(id % 2 == 0){
                        MPI_Send(data, n, MPI_INT, partner, 0, MPI_COMM_WORLD);
                        MPI_Recv(buffer, n, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
                else{
                        MPI_Recv(buffer, n, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        MPI_Send(data, n, MPI_INT, partner, 0, MPI_COMM_WORLD);
                }

                if(id < partner){
                        //如果id < partner，若id的最大值大於partner的最小值則交換，直到id的最大值小於partner的最小值
                        while(1){
                                //找出buffer最小值的index
                                int mini = min_index(buffer, n);
                                //找出data最大值的index
                                int maxi = max_index(data, n);

                                if(buffer[mini] < data[maxi]){
                                        int temp = buffer[mini];
                                        buffer[mini] = data[maxi];
                                        data[maxi] = temp;
                                }
                                else{
                                        break;
                                }
                        }
                }
                else{
                        //如果id > partner，若id的最小值小於partner的最大值則交換，直到id的最小值大於partner的最大值
                        while(1){
                                //找出buffer最大值的index
                                int maxi = max_index(buffer, n);
                                //找出data最小值的index
                                int mini = min_index(data, n);

                                if(buffer[maxi] > data[mini]){
                                        int temp = buffer[maxi];
                                        buffer[maxi] = data[mini];
                                        data[mini] = temp;
                                }
                                else{
                                        break;
                                }
                        }
                }
        }
        //sort 各個 local list
        qsort(data, n, sizeof(int), &compare);
}
int main(int argc, char* argv[]){

        int id,proc;
        int n,remain;//n = 要sort的數字個數，remain = n除以proc後的餘數

        //initial MPI
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &id);
        MPI_Comm_size(MPI_COMM_WORLD, &proc);

        //timeing
        double starttime = MPI_Wtime();

        //process 0 輸入 n 後，計算remain，把n分配給proc後再+1
        if(id == 0){
                printf("Enter the number of data that you want to sort :");
                scanf("%d",&n);
                remain = n % proc;
                n = n / proc;
                n++;
        }

        //Bcast n and remain to all process
        MPI_Bcast(&n,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&remain,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        //儲存local list的陣列
        int data[n];
        //初始化data值
        //若id < remain則local list最後一個數字正常取rand
        //若id >= remain則local list最後一個數字取比RAND_TOP還要大的數
        int i = 0,j = 0;
        srand(time(NULL)+id);
        for(i = 0;i < n-1;i++)data[i] = rand()% RAND_TOP;
        if(id < remain)data[n-1] = rand()% RAND_TOP;
        if(id >= remain)data[n-1] = RAND_TOP+1;

        //sort local list
        qsort(data, n, sizeof(int), &compare);

        //gather local list to process 0
        int* rec_buf;
        if(id == 0)rec_buf = (int*)malloc((proc * n) * sizeof(int));
        MPI_Gather(data, n, MPI_INT, rec_buf, n, MPI_INT, 0, MPI_COMM_WORLD);

        //process 0 print local list
        if(id == 0){
                for(i = 0;i < proc;i++){
                        printf("local_list[%d] :",i);
                        for(j = i*n;j < (i+1)*n;j++){
                                //若id >= remain，則跳過最後一格數字不print
                                if(i >= remain)if(j == (i+1)*n-1)continue;
                                printf("%d ",rec_buf[j]);
                        }
                        printf("\n");
                }
        }

        //do the parallel odd even sort
        parallel_sort(data, id, proc, n);
        MPI_Barrier(MPI_COMM_WORLD);

        //gather sorted local list to process 0
        MPI_Gather(data,n ,MPI_INT, rec_buf, n, MPI_INT, 0, MPI_COMM_WORLD);

        //process 0 print sorted global list and totaltime
        if(id == 0){
                for(i = 0;i < proc * (n-1) + remain;i++){
                        printf("%d ",rec_buf[i]);
                        if((i + 1) % (n-1) == 0)printf("\n");
                }
                double totaltime = MPI_Wtime() - starttime;
                printf("Time cost :%lf s\n",totaltime);
        }

        //quit MPI
        MPI_Finalize( );
        return 0;
}
