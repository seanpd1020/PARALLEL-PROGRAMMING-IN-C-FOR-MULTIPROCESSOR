#include <stdio.h>
#include <limits.h>
#include <mpi.h>

int checkCircuit(int, long);	//輸入電路後輸出0或1

int main (int argc,char* argv[]) {

	long i,local_i;	   //local_i :把UINT_MAX平均分配給各個process
	int id,half;	   //id :process的編號，half :在使用tree-structured時用以紀錄proc/2的變數
	int proc,sub;	   //proc :process個數，sub :在使用tree-structured時MPI_Recv用sub來接其他process當時的local_count
	int total_count ,local_count = 0;	//total_count :所有輸出為1的個數，local_count :各個process依據自己分配到的數量計算輸出為1的個數(一開始為0)

	//MPI開始
	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&id);
	MPI_Comm_size(MPI_COMM_WORLD,&proc);

	double startTime = 0.0 , totalTime = 0.0;	//開始時間及總使用時間
	startTime = MPI_Wtime();	//開始計時

	local_i = UINT_MAX / proc;	//把UINT_MAX平均分配給各個process後指派給local_i

	//各個process依據自己分配到的數量計算輸出為1的個數
	for (i = id*local_i; i < (id + 1)*local_i; i++) {
        	local_count += checkCircuit(id, i);
	}

	//tree-structured ０　１　２　３　４　５　６　７
	//		          	　↓　↓　↓　↓
	//				　０　１　２　３
	//				  　　　  ↓  ↓
	//				 	　０　１
	//					      ↓
	//					      ０ 
	for(half = proc/2;half>=1;half=half/2){
        	if(id<2*half){
                	if(id>=half){
                        	MPI_Send(&local_count,1,MPI_INT,id-half,0,MPI_COMM_WORLD);
	                }else{
        	                MPI_Recv(&sub,1,MPI_INT,id+half,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                	        local_count +=sub;
	                }
        	}
	}

	//process 0 印出所有輸出為1的個數
	if(id == 0){
        	 total_count = local_count;
	         printf("\nA total of %d solutions were found.\n", total_count);
	         fflush(stdout);
	}

	totalTime = MPI_Wtime() - startTime;	//結束計時
	//process 0 印出總使用時間
	if(id == 0)printf("Process %d finished in time %f secs.\n\n" , id , totalTime);
	fflush(stdout);

	MPI_Finalize();		//MPI結束


	return 0;
}

//看數字 n 第 i 位元的bit為0或1
#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)

#define SIZE 32

int checkCircuit(int id, long bits) {
	int v[SIZE];	//儲存 bits 的32bit表示
	int i;

	for (i = 0; i < SIZE; i++)
	    v[i] = EXTRACT_BIT(bits,i);

	//輸入電路，若判斷式為真則回傳 1，反之則回傳 0
	if ( ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
	       && (!v[3] || !v[4]) && (v[4] || !v[5])
	       && (v[5] || !v[6]) && (v[5] || v[6])
	       && (v[6] || !v[15]) && (v[7] || !v[8])
	       && (!v[7] || !v[13]) && (v[8] || v[9])
	       && (v[8] || !v[9]) && (!v[9] || !v[10])
	       && (v[9] || v[11]) && (v[10] || v[11])
	       && (v[12] || v[13]) && (v[13] || !v[14])
	       && (v[14] || v[15]) )
	       ||
	    ( (v[16] || v[17]) && (!v[17] || !v[19]) && (v[18] || v[19])
	       && (!v[19] || !v[20]) && (v[20] || !v[21])
	       && (v[21] || !v[22]) && (v[21] || v[22])
	       && (v[22] || !v[31]) && (v[23] || !v[24])
	       && (!v[23] || !v[29]) && (v[24] || v[25])
	       && (v[24] || !v[25]) && (!v[25] || !v[26])
	       && (v[25] || v[27]) && (v[26] || v[27])
	       && (v[28] || v[29]) && (v[29] || !v[30])
	       && (v[30] || v[31]) ) )
	    {
	         printf ("(%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
	              v[31],v[30],v[29],v[28],v[27],v[26],v[25],v[24],v[23],v[22],
	              v[21],v[20],v[19],v[18],v[17],v[16],v[15],v[14],v[13],v[12],
	              v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);
	         fflush (stdout);
	         return 1;
	    } else {
	         return 0;
	    }
}
