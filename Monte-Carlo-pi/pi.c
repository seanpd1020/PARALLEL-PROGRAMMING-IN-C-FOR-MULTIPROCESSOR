#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
void srandom (unsigned seed);
long long int count (long long int);

int main (int argc, char *argv[])
{
        double  pi,start_time=0.0,end_time;	//pi :要計算的pi值，開始及結束的時間
        int id , num_of_proc, i, half;		//id :process的編號，num_of_proc :process個數，half :在使用tree-structured時用以紀錄num_of_proc/2的變數
        long long int number_in_circle, total_circle=0;		//number_in_circle :各process射在圓圈內的個數，total_circle :所有process射在圓圈內個數的總和
	long long int number_of_tosses, total_tosses; 		//number_of_tosses :各process射鏢的次數，total_tosses :所有process射鏢的次數總和
        long long int sub;					//sub :在使用tree-structured時MPI_Recv用sub來接其他process當時的number_in_circle
	
	//MPI開始
        MPI_Init(&argc,&argv);
        MPI_Comm_rank(MPI_COMM_WORLD,&id);
        MPI_Comm_size(MPI_COMM_WORLD,&num_of_proc);

        srandom (time(NULL));	//給random函數設置seed
	
	//使用 process 0 輸入總射鏢數
        if(id == 0){
                printf("Enter the number you want to toss: ");
                scanf("%lld",&total_tosses);
        }
        MPI_Bcast(&total_tosses,1,MPI_LONG_LONG_INT,0,MPI_COMM_WORLD);		//利用Bcast傳遞total_tosses值給各個process
        MPI_Barrier(MPI_COMM_WORLD);						//讓各個process都run到這一行後再開始繼續run

        start_time = MPI_Wtime();	//開始計時

        number_of_tosses=total_tosses/num_of_proc;	//分配總射鏢數給各個process
        number_in_circle=count(number_of_tosses);	//利用count函式計算各process射在圓圈內的次數
	
	//tree-structured ０　１　２　３　４　５　６　７
	//		          	　↓　↓　↓　↓
	//				　０　１　２　３
	//				  　　　  ↓  ↓
	//				 	　０　１
	//					      ↓
	//					      ０
	for(half = num_of_proc/2;half>=1;half=half/2){
		if(id<2*half){
                        if(id>=half){
                       	        MPI_Send(&number_in_circle,1,MPI_LONG_LONG_INT,id-half,0,MPI_COMM_WORLD);
                 	}else{
                       	        MPI_Recv(&sub,1,MPI_LONG_LONG_INT,id+half,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    	        		number_in_circle +=sub;
                       	}
                }
        }

	//process 0 利用公式算出估計pi值，結束時間，並輸出估計pi值，花費時間，誤差值
	if (id == 0) {
                total_circle = number_in_circle;
                pi = 4*(double)total_circle/total_tosses;
                end_time = MPI_Wtime();
                printf("After %lld throws, average value of pi = %3.13f\n",total_tosses,pi);
                printf("Time: %lf s\n",(end_time-start_time));
                printf ("Error of the real value of PI is : %3.13f \n",fabs(3.1415926535897-pi));
        }
        fflush(stdout);

        MPI_Finalize();		//MPI結束

        return 0;
}

//計算射標在圓圈內的次數並回傳
long long int count(long long int tosses)
{
        long random(void);
        double x, y, r;
        long long int num_in_circle = 0, n;
        unsigned int rand_max;
        rand_max = 2 << (31-1);
	for (n = 1; n <= tosses; n++)  {
		r = (double)random()/rand_max;
                x = (2.0 * r) - 1.0;
                r = (double)random()/rand_max;
                y = (2.0 * r) - 1.0;
                if ((x*x+ y*y) <= 1.0)num_in_circle++;
        }
        return(num_in_circle);
}
