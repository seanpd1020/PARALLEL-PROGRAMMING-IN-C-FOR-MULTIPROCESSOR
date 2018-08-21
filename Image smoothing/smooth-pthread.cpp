#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"
#include <pthread.h>

using namespace std;

//�w�q���ƹB�⪺����
#define NSmooth 1000
#define NB 2

/*********************************************************/
/*�ܼƫŧi�G                                             			*/
/*  bmpHeader    �G BMP�ɪ����Y                          		*/
/*  bmpInfo      �G BMP�ɪ���T                          		*/
/*  **BMPSaveData�G �x�s�n�Q�g�J���������               	*/
/*  **BMPData    �G �Ȯ��x�s�n�Q�g�J���������           	*/
/*********************************************************/
BMPHEADER bmpHeader;                        
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;                                                   
int thread_count;
int counter[NB]={0};
pthread_mutex_t mutex_p ;
/*********************************************************/
/*��ƫŧi�G                                             			 */
/*  readBMP    �G Ū�����ɡA�ç⹳������x�s�bBMPSaveData*/
/*  saveBMP    �G �g�J���ɡA�ç⹳�����BMPSaveData�g�J  */
/*  swap       �G �洫�G�ӫ���                           			 */
/*  **alloc_memory�G �ʺA���t�@��Y * X�x�}               	 */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

void *compute(void *myrank);

int main(int argc,char *argv[])
{
/*********************************************************/
/*�ܼƫŧi�G                                             */
/*  *infileName  �G Ū���ɦW                             */
/*  *outfileName �G �g�J�ɦW                             */
/*  startwtime   �G �O���}�l�ɶ�                         */
/*  endwtime     �G �O�������ɶ�                         */
/*********************************************************/
	char *infileName = "input.bmp";
    char *outfileName = "output4.bmp";
	clock_t startwtime = 0.0, endwtime=0;
	long thread;

	//���t�O���鵹thread_handles�A��argv[1]Ū�Jthread�ƫ�����thread_count
	pthread_t* thread_handles;
	thread_count=atoi(argv[1]);
	thread_handles = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
	
	//Ū���ɮ�
    if ( readBMP( infileName) )
        cout << "Read file successfully!!" << endl;
    else 
        cout << "Read file fails!!" << endl;
	
	//�}�l�p��
	startwtime = clock();
	//���t�O���鵹BMPData	
	BMPData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
	
	//create thread_count��thread�h����compute�禡
	for(thread=0;thread<thread_count;thread++)
	{
		pthread_create(&thread_handles[thread],NULL,compute,(void*)thread);
	}
	//���ݩҦ�thread join����free��thread_handles
	for(thread=0;thread<thread_count;thread++)
	{
		pthread_join(thread_handles[thread],NULL);
	}
	free(thread_handles);
	
	//�����p��
	endwtime = clock();
        
	//��X�ɮ�
	if ( saveBMP( outfileName ) )
        cout << "Save file successfully!!" << endl;
    else
        cout << "Save file fails!!" << endl;
	
	//��X����ɶ��Adestroy mutex_p�A����O����
	cout << "The execution time = "<< (float)(endwtime-startwtime)/CLOCKS_PER_SEC/10 <<endl ;
 	pthread_mutex_destroy(&mutex_p);
	free(BMPData);
 	free(BMPSaveData);

    return 0;
}

//��thread���檺�禡
void* compute(void* myrank)
{
	long rank = (long)myrank;
	
	//��bmpInfo.biHeight�������t��thread_count�A�Ѿl�����׳����̫�@��thread	
	int height;
	if(rank == thread_count-1)height = bmpInfo.biHeight/thread_count + bmpInfo.biHeight%thread_count;
	else height = bmpInfo.biHeight/thread_count;
	
	//�����Uthread�}�l�i�業�ƹB�⪺index
	int start = (bmpInfo.biHeight/thread_count)*rank;
	
	for(int count = 0; count < NSmooth ; count ++)
	{
		//���i�J��thread��counter[count%NB]�ȥ[1�A���ݳ̫�@��thread������@�_���}while�j��
		//�̫�@�Ӷi�J��thread�Aswap BMPSaveData�MBMPData��pointer�A�ç�counter[(counter+1)%NB]�k�s
		pthread_mutex_lock(&mutex_p);
		if(counter[count%NB]==thread_count-1)
		{
			swap(BMPSaveData,BMPData);
			counter[(count+1)%NB]=0;
		}
		counter[count%NB]++;
		pthread_mutex_unlock(&mutex_p);
		while(counter[count%NB]<thread_count);
		
		//�i�業�ƹB��
		for(int i = start; i<start + height ; i++)
		{	
			for(int j =0; j<bmpInfo.biWidth ; j++)
			{
			/*********************************************************/
			/*�]�w�W�U���k��������m                                 		*/
			/*********************************************************/
				int Top = i>0 ? i-1 : bmpInfo.biHeight-1;
				int Down = i<bmpInfo.biHeight-1 ? i+1 : 0;
				int Left = j>0 ? j-1 : bmpInfo.biWidth-1;
				int Right = j<bmpInfo.biWidth-1 ? j+1 : 0;
			/*********************************************************/
			/*�P�W�U���k�����������A�å|�ˤ��J                     	 	 */
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
/* Ū������                                              */
/*********************************************************/
int readBMP(char *fileName)
{
	//�إ߿�J�ɮת���	
 	ifstream bmpFile( fileName, ios::in | ios::binary );
 
  	//�ɮ׵L�k�}��
    if ( !bmpFile )
	{
		cout << "It can't open file!!" << endl;
        return 0;
    }
 
	//Ū��BMP���ɪ����Y���
	bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );
 
    //�P�M�O�_��BMP����
    if( bmpHeader.bfType != 0x4d42 )
	{
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }
 
    //Ū��BMP����T
    bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );
        
    //�P�_�줸�`�׬O�_��24 bits
    if ( bmpInfo.biBitCount != 24 )
	{
        cout << "The file is not 24 bits!!" << endl;
        return 0;
    }

    //�ץ��Ϥ����e�׬�4������
    while( bmpInfo.biWidth % 4 != 0 )
        bmpInfo.biWidth++;

    //�ʺA���t�O����
    BMPSaveData = alloc_memory( bmpInfo.biHeight, bmpInfo.biWidth);
        
    //Ū���������
    //for(int i = 0; i < bmpInfo.biHeight; i++)
    //bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
	bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);
	
    //�����ɮ�
    bmpFile.close();
 
    return 1;
 
}
/*********************************************************/
/* �x�s����                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
	//�P�M�O�_��BMP����
    if( bmpHeader.bfType != 0x4d42 )
	{
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }
        
 	//�إ߿�X�ɮת���
    ofstream newFile( fileName,  ios:: out | ios::binary );
 
    //�ɮ׵L�k�إ�
    if ( !newFile )
	{
        cout << "The File can't create!!" << endl;
        return 0;
    }
 	
    //�g�JBMP���ɪ����Y���
    newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

	//�g�JBMP����T
    newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

    //�g�J�������
    //for( int i = 0; i < bmpInfo.biHeight; i++ )
    //newFile.write( ( char* )BMPSaveData[i],bmpInfo.biWidth*sizeof(RGBTRIPLE) );
    newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

    //�g�J�ɮ�
    newFile.close();
 
    return 1;
 
}


/*********************************************************/
/* ���t�O����G�^�Ǭ�Y*X���x�}                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{        
	//�إߪ��׬�Y�����а}�C
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
	RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//��C�ӫ��а}�C�̪����Ыŧi�@�Ӫ��׬�X���}�C 
    for( int i = 0; i < Y; i++)
	{
        temp[ i ] = &temp2[i*X];
    }
 
    return temp;
 
}
/*********************************************************/
/* �洫�G�ӫ���                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}

