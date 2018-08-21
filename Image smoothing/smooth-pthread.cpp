#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"
#include <pthread.h>

using namespace std;

//定義平滑運算的次數
#define NSmooth 1000
#define NB 2

/*********************************************************/
/*變數宣告：                                             			*/
/*  bmpHeader    ： BMP檔的標頭                          		*/
/*  bmpInfo      ： BMP檔的資訊                          		*/
/*  **BMPSaveData： 儲存要被寫入的像素資料               	*/
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           	*/
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;                                                   
int thread_count;
int counter[NB]={0};
pthread_mutex_t mutex_p ;
/*********************************************************/
/*函數宣告：                                             			 */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           			 */
/*  **alloc_memory： 動態分配一個Y * X矩陣               	 */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

void *compute(void *myrank);

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
	char *infileName = "input.bmp";
    char *outfileName = "output4.bmp";
	clock_t startwtime = 0.0, endwtime=0;
	long thread;

	//分配記憶體給thread_handles，用argv[1]讀入thread數指派給thread_count
	pthread_t* thread_handles;
	thread_count=atoi(argv[1]);
	thread_handles = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
	
	//讀取檔案
    if ( readBMP( infileName) )
        cout << "Read file successfully!!" << endl;
    else 
        cout << "Read file fails!!" << endl;
	
	//開始計時
	startwtime = clock();
	//分配記憶體給BMPData	
	BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
	
	//create thread_count個thread去執行compute函式
	for(thread=0;thread<thread_count;thread++)
	{
		pthread_create(&thread_handles[thread],NULL,compute,(void*)thread);
	}
	//等待所有thread join之後free掉thread_handles
	for(thread=0;thread<thread_count;thread++)
	{
		pthread_join(thread_handles[thread],NULL);
	}
	free(thread_handles);
	
	//結束計時
	endwtime = clock();
        
	//輸出檔案
	if ( saveBMP( outfileName ) )
        cout << "Save file successfully!!" << endl;
    else
        cout << "Save file fails!!" << endl;
	
	//輸出執行時間，destroy mutex_p，釋放記憶體
	cout << "The execution time = "<< (float)(endwtime-startwtime)/CLOCKS_PER_SEC/10 <<endl ;
 	pthread_mutex_destroy(&mutex_p);
	free(BMPData);
 	free(BMPSaveData);

    return 0;
}

//讓thread執行的函式
void* compute(void* myrank)
{
	long rank = (long)myrank;
	
	//把bmpInfo.biHeight平均分配給thread_count，剩餘的高度都給最後一個thread	
	int height;
	if(rank == thread_count-1)height = bmpInfo.biHeight/thread_count + bmpInfo.biHeight%thread_count;
	else height = bmpInfo.biHeight/thread_count;
	
	//指派各thread開始進行平滑運算的index
	int start = (bmpInfo.biHeight/thread_count)*rank;
	
	for(int count = 0; count < NSmooth ; count ++)
	{
		//先進入的thread把counter[count%NB]值加1，等待最後一個thread結束後一起離開while迴圈
		//最後一個進入的thread，swap BMPSaveData和BMPData的pointer，並把counter[(counter+1)%NB]歸零
		pthread_mutex_lock(&mutex_p);
		if(counter[count%NB]==thread_count-1)
		{
			swap(BMPSaveData,BMPData);
			counter[(count+1)%NB]=0;
		}
		counter[count%NB]++;
		pthread_mutex_unlock(&mutex_p);
		while(counter[count%NB]<thread_count);
		
		//進行平滑運算
		for(int i = start; i<start + height ; i++)
		{	
			for(int j =0; j<bmpInfo.biWidth ; j++)
			{
			/*********************************************************/
			/*設定上下左右像素的位置                                 		*/
			/*********************************************************/
				int Top = i>0 ? i-1 : bmpInfo.biHeight-1;
				int Down = i<bmpInfo.biHeight-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
			/*********************************************************/
			/*與上下左右像素做平均，並四捨五入                     	 	 */
			/*********************************************************/
				BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/5+0.5;
				BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/5+0.5;
				BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Down][j].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/5+0.5;
			}
		}
	}
	return NULL;	
}
/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//建立輸入檔案物件	
 	ifstream bmpFile( fileName, ios::in | ios::binary );
 
  	//檔案無法開啟
    if ( !bmpFile )
	{
		cout << "It can't open file!!" << endl;
        return 0;
    }
 
	//讀取BMP圖檔的標頭資料
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
    //判決是否為BMP圖檔
    if( bmpHeader.bfType != 0x4d42 )
	{
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }
 
    //讀取BMP的資訊
    bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
    //判斷位元深度是否為24 bits
    if ( bmpInfo.biBitCount != 24 )
	{
        cout << "The file is not 24 bits!!" << endl;
        return 0;
    }

    //修正圖片的寬度為4的倍數
    while( bmpInfo.biWidth % 4 != 0 )
        bmpInfo.biWidth++;

    //動態分配記憶體
    BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
    //讀取像素資料
    //for(int i = 0; i < bmpInfo.biHeight; i++)
    //bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
    //關閉檔案
    bmpFile.close();
 
    return 1;
 
}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
	//判決是否為BMP圖檔
    if( bmpHeader.bfType != 0x4d42 )
	{
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }
        
 	//建立輸出檔案物件
    ofstream newFile( fileName,  ios:: out | ios::binary );
 
    //檔案無法建立
    if ( !newFile )
	{
        cout << "The File can't create!!" << endl;
        return 0;
    }
 	
    //寫入BMP圖檔的標頭資料
    newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//寫入BMP的資訊
    newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

    //寫入像素資料
    //for( int i = 0; i < bmpInfo.biHeight; i++ )
    //newFile.write( ( char* )BMPSaveData[i],bmpInfo.biWidth*sizeof(RGBTRIPLE) );
    newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

    //寫入檔案
    newFile.close();
 
    return 1;
 
}


/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//建立長度為Y的指標陣列
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列 
    for( int i = 0; i < Y; i++)
	{
        temp[ i ] = &temp2[i*X];
    }
 
    return temp;
 
}
/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}

