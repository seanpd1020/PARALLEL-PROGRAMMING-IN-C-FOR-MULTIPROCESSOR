#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*********************************************************/
BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;


/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  swap       ： 交換二個指標                           */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*  proc         ：process個數                           */
/*  id           ：process id                            */
/*  i            ：iteration                             */
/*  rem          ：bmpInfo.biHeight除以proc後的餘數      */
/*  new_h        ：各process經Scatterv後得到新的高度     */
/*  width        ：bmpInfo.biWidth                       */
/*  *sendcount   ：Scatterv,Gatherv要使用的每個process   */
/*		   收到或寄出的data數                    */
/*  *displs      ：Scatterv,Gatherv要使用的每個process   */
/*                 收到或寄出的data數的累計              */
/*  sum          ：紀錄上述累計值的變數                  */
/*********************************************************/
        char *infileName = "input.bmp";
        char *outfileName = "output3.bmp";
        double startwtime = 0.0, endwtime=0;
        int proc, id, i;
        int rem, new_h, width;
        int *sendcount, *displs, sum = 0;

	//MPI開始
        MPI_Init(&argc,&argv);
        MPI_Comm_size(MPI_COMM_WORLD,&proc);
        MPI_Comm_rank(MPI_COMM_WORLD,&id);

        //process 0 讀取檔案及計算各變數值
	//處理餘數的方法為：若餘數不等於0則把bmpInfo.biHeight除以proc後+1，然後除了最後一個process
	//外，指派給new_h；最後一個process的new_h值則為bmpInfo.Height-(proc-1)*其他process的new_h值
        if(id == 0){
                if ( readBMP( infileName) )
                        cout << "Read file successfully!!" << endl;
                else {
                        cout << "Read file fails!!" << endl;
                        MPI_Finalize();
                        return 0;
                }
                rem = bmpInfo.biHeight % proc;
                if(rem != 0)new_h = (bmpInfo.biHeight / proc )+1;
                else new_h = bmpInfo.biHeight / proc;
 		width = bmpInfo.biWidth;
        }

	//把在process 0 初始化的變數 Bcast 到各process
        MPI_Bcast(&bmpInfo.biHeight,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&rem,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&new_h,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Bcast(&width,1,MPI_INT,0,MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);

        if(id == (proc-1))new_h = bmpInfo.biHeight - (proc-1)*new_h;

        //開始計時
	startwtime = MPI_Wtime();

	//分配記憶體後，計算sendcount值及displs值
        sendcount = (int*)malloc(sizeof(RGBTRIPLE)*proc*width*new_h);
        displs = (int*)malloc(sizeof(RGBTRIPLE)*proc*width*new_h);
        for(i=0;i<proc;i++){
                if(i == (proc-1) && rem != 0)sendcount[i]=width * (bmpInfo.biHeight - (bmpInfo.biHeight/proc+1) * (proc - 1))*sizeof(RGBTRIPLE);
                else sendcount[i] = width*new_h*sizeof(RGBTRIPLE);
                displs[i] = sum;
                sum += sendcount[i];
        }
        
	// **BMPData : 暫時儲存要被寫入的像素資料
	// **BMPRecv : 各process用來儲存自己分配到的像素資料
	// **BMPNew  : 各process上下擴增各一列，用來收發像素資料
        RGBTRIPLE **BMPData = NULL;
        RGBTRIPLE **BMPRecv = NULL;
        RGBTRIPLE **BMPNew = NULL;
        BMPData = alloc_memory( new_h, width);
        BMPRecv = alloc_memory( new_h, width);
        BMPNew = alloc_memory(2,width);

	//把BMPSaveData分配給各process的BMPRecv
        if (id == 0) MPI_Scatterv((char*)BMPSaveData[0],sendcount,displs,MPI_CHAR,(char*)BMPRecv[0],(new_h * width * sizeof(RGBTRIPLE)),MPI_CHAR,0,MPI_COMM_WORLD);
        else MPI_Scatterv(NULL,sendcount,displs,MPI_CHAR,(char*)BMPRecv[0],(new_h * width * sizeof(RGBTRIPLE)),MPI_CHAR,0,MPI_COMM_WORLD);
	
	//進行多次的平滑運算
	for(int count = 0; count < NSmooth ; count ++){
		
		//把狀況分成 BMPRecv[new_h-1] 與 BMPNew[0] 之間的收發
		//以及       BMPRecv[0]       與 BMPNew[1] 之間的收發
		//再分別分成奇數與偶數process輪流做send與recv
                int check_id_1 = (id == 0) ? proc-1:id-1;
                int check_id_2 = (id == (proc-1)) ? 0:id+1;
                if(id % 2 == 0){
                        MPI_Recv((char*)BMPNew[0],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                        MPI_Send((char*)BMPRecv[new_h-1],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_2,0,MPI_COMM_WORLD);
                }else{
                        MPI_Send((char*)BMPRecv[new_h-1],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_2,0,MPI_COMM_WORLD);
                        MPI_Recv((char*)BMPNew[0],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_1,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                }
                if(id % 2 == 0){
                        MPI_Recv((char*)BMPNew[1],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_2,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                        MPI_Send((char*)BMPRecv[0],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_1,0,MPI_COMM_WORLD);
                }else{
                        MPI_Send((char*)BMPRecv[0],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_1,0,MPI_COMM_WORLD);
                        MPI_Recv((char*)BMPNew[1],width*sizeof(RGBTRIPLE),MPI_CHAR,check_id_2,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                }
                MPI_Barrier(MPI_COMM_WORLD);
		
		//把像素資料與暫存指標做交換
                swap(BMPRecv,BMPData);

		//進行平滑運算
                for(int k = 0; k<new_h ; k++)
                        for(int j = 0; j<width ; j++){
                                /*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
                                int Top = k>0 ? k-1 : new_h-1;
                                int Down = k<new_h-1 ? k+1 : 0;
                                int Left = j>0 ? j-1 : width-1;
                                int Right = j<width-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*第一及最後一列的上下像素需使用上下process的BMPNew值    */
				/*其他列則直接使用上下左右像素做運算                     */
				/*********************************************************/
				if(k != 0 && k != (new_h-1)){
                                        BMPRecv[k][j].rgbBlue =  (double) ((double)BMPData[k][j].rgbBlue+(double)BMPData[Top][j].rgbBlue+(double)BMPData[Down][j].rgbBlue+(double)BMPData[k][Left].rgbBlue+(double)BMPData[k][Right].rgbBlue)/5+0.5;
                                        BMPRecv[k][j].rgbGreen =  (double) ((double)BMPData[k][j].rgbGreen+(double)BMPData[Top][j].rgbGreen+(double)BMPData[Down][j].rgbGreen+(double)BMPData[k][Left].rgbGreen+(double)BMPData[k][Right].rgbGreen)/5+0.5;
                                        BMPRecv[k][j].rgbRed =  (double) ((double)BMPData[k][j].rgbRed+(double)BMPData[Top][j].rgbRed+(double)BMPData[Down][j].rgbRed+(double)BMPData[k][Left].rgbRed+(double)BMPData[k][Right].rgbRed)/5+0.5;
                                }
                                else if(k == 0){
                                        BMPRecv[k][j].rgbBlue =  (double) ((double)BMPData[k][j].rgbBlue+(double)BMPNew[0][j].rgbBlue+(double)BMPData[Down][j].rgbBlue+(double)BMPData[k][Left].rgbBlue+(double)BMPData[k][Right].rgbBlue)/5+0.5;
                                        BMPRecv[k][j].rgbGreen =  (double) ((double)BMPData[k][j].rgbGreen+(double)BMPNew[0][j].rgbGreen+(double)BMPData[Down][j].rgbGreen+(double)BMPData[k][Left].rgbGreen+(double)BMPData[k][Right].rgbGreen)/5+0.5;
                                        BMPRecv[k][j].rgbRed =  (double) ((double)BMPData[k][j].rgbRed+(double)BMPNew[0][j].rgbRed+(double)BMPData[Down][j].rgbRed+(double)BMPData[k][Left].rgbRed+(double)BMPData[k][Right].rgbRed)/5+0.5;
                                }
                                else if(k == (new_h-1)){
                                        BMPRecv[k][j].rgbBlue =  (double) ((double)BMPData[k][j].rgbBlue+(double)BMPNew[1][j].rgbBlue+(double)BMPData[Top][j].rgbBlue+(double)BMPData[k][Left].rgbBlue+(double)BMPData[k][Right].rgbBlue)/5+0.5;
                                        BMPRecv[k][j].rgbGreen =  (double) ((double)BMPData[k][j].rgbGreen+(double)BMPNew[1][j].rgbGreen+(double)BMPData[Top][j].rgbGreen+(double)BMPData[k][Left].rgbGreen+(double)BMPData[k][Right].rgbGreen)/5+0.5;
                                        BMPRecv[k][j].rgbRed =  (double) ((double)BMPData[k][j].rgbRed+(double)BMPNew[1][j].rgbRed+(double)BMPData[Top][j].rgbRed+(double)BMPData[k][Left].rgbRed+(double)BMPData[k][Right].rgbRed)/5+0.5;
                                }
                        }

        }

	//從各process收集各個BMPRecv後存到BMPSaveData
        if(id == 0){
                MPI_Gatherv((char*)BMPRecv[0],(new_h * width * sizeof(RGBTRIPLE)),MPI_CHAR,(char*)BMPSaveData[0],sendcount,displs,MPI_CHAR,0,MPI_COMM_WORLD);
        }
        else{
                MPI_Gatherv((char*)BMPRecv[0],(new_h*width*sizeof(RGBTRIPLE)),MPI_CHAR,NULL,sendcount,displs,MPI_CHAR,0,MPI_COMM_WORLD);
	}
        MPI_Barrier(MPI_COMM_WORLD);

	//印出使用時間，及寫入檔案	
        if(id == 0){
                endwtime = MPI_Wtime();
                if ( saveBMP( outfileName ) )
                        cout << "Save file successfully!!" << endl;
                else
                        cout << "Save file fails!!" << endl;

                cout << "The execution time = "<< endwtime-startwtime <<endl ;
        }

	//釋放記憶體
        free(BMPData);
        free(BMPRecv);
        free(BMPSaveData);

        MPI_Finalize();

        return 0;
}

/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
        //建立輸入檔案物件
        ifstream bmpFile( fileName, ios::in | ios::binary );

        //檔案無法開啟
        if ( !bmpFile ){
                cout << "It can't open file!!" << endl;
                return 0;
        }
	//讀取BMP圖檔的標頭資料
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

        //判決是否為BMP圖檔
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

        //讀取BMP的資訊
        bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

        //判斷位元深度是否為24 bits
        if ( bmpInfo.biBitCount != 24 ){
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
        //      bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
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
        if( bmpHeader.bfType != 0x4d42 ){
                cout << "This file is not .BMP!!" << endl ;
                return 0;
        }

        //建立輸出檔案物件
        ofstream newFile( fileName,  ios:: out | ios::binary );

        //檔案無法建立
        if ( !newFile ){
                cout << "The File can't create!!" << endl;
                return 0;
        }

        //寫入BMP圖檔的標頭資料
        newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

        //寫入BMP的資訊
        newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

        //寫入像素資料
        //for( int i = 0; i < bmpInfo.biHeight; i++ )
        //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
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
        for( int i = 0; i < Y; i++){
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
