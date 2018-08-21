#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<omp.h>
//設欲排序的陣列最大值為100000
#define array_size 100000

int main(int argc,char* argv[])
{
	double starttime,endtime,serial_time,parallel_time;
	int count,i,j,thread_count,size = array_size;
	
	thread_count = atoi(argv[1]);//由command line輸入thread數
	
	//利用random給array陣列隨機填值
	int array[size];
	srand(time(NULL));
	for(i=0;i<size;i++)
	{
		array[i] = (rand()%size) + 1;
	}
	
	starttime = omp_get_wtime();//開始計時

	//進行serial counting sort
	int *temp = malloc(size * sizeof(int));
	for(i=0;i<size;i++)
	{
		count = 0;
		for(j=0;j<size;j++)
		{
			if(array[j]<array[i])
				count++;
			else if(array[j]==array[i] && j<i)
				count++;
		}
		temp[count]=array[i];
	}

	endtime = omp_get_wtime();//結束計時
	
	serial_time = endtime - starttime;//計算serial運算時間

	//將排序完成的temp陣列整個複製給array陣列後free掉temp記憶體
	memcpy(array,temp,sizeof(int)*size);
	free(temp);

	//輸出排序後的結果
	for(i=0;i<size;i++)
		printf("%d ",array[i]);

	//再random產生一個新的array陣列
	for(i=0;i<size;i++)
	{
		array[i] = (rand()%size) + 1;
	}

	starttime = omp_get_wtime();//開始計時

	//進行parallel counting sort
	temp = malloc(sizeof(int)*size);
#pragma omp parallel for num_threads(thread_count) default(none) shared(array,temp,size) private(i,j,count)
	for(i=0;i<size;i++)
	{
		count = 0;
		for(j=0;j<size;j++)
		{
			if(array[j]<array[i])
				count++;
			else if(array[j]==array[i] && j<i)
				count++;
		}
		temp[count] = array[i];
	}
	
	endtime = omp_get_wtime();//結束計時
	
	parallel_time = endtime - starttime;//計算parallel計算時間

	//將排序完成的temp陣列整個複製給array陣列後free掉temp記憶體
	memcpy(array,temp,sizeof(int)*size);
	free(temp);

	//輸出排序後的結果
	for(i=0;i<size;i++)
		printf("%d ",array[i]);

	//輸出serial及parallel總計算時間
	printf("\nSerial time is %lf\nParallel time is %lf\n",serial_time,parallel_time);
	
	return 0;
}