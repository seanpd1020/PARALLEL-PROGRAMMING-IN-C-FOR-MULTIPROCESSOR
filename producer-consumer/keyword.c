#include<stdio.h>
#include<stdlib.h>
#include<omp.h>
#include<string.h>
#include<dirent.h>
//設最大讀取檔案數為1000
#define MAX_FILES 1000
//設一行最多1000個char字元
#define MAX_CHAR 1000
//設最大讀取keyword數為1000
#define MAX_KEY_COUNT 1000
//設producer為2個thread
#define num_of_producer 2
//設consumer為2個thread
#define num_of_consumer 2

struct node{
	char* data;
	struct node* next;
};

int key_count = 0;//計算keyword數
int num_of_key[MAX_KEY_COUNT]={0};//初始化計數keyword出現次數的陣列為0
char keyword[MAX_KEY_COUNT][MAX_CHAR];//用來儲存讀取到的keyword的陣列
void handle(int prod_count,int cons_count,FILE* files[],int file_count);//producers 和 consumers 做處理
void readfile(FILE*,struct node**,struct node**,int);//讀取檔案
void enqueue(char*,struct node**,struct node**);//插入 node 到 single shared queue
struct node* dequeue(struct node**,struct node**,int);//取出 node 從 single shared queue
void tokenize(char*,int);//從由queue中取出的一行data找出欲尋找的keyword並使其對應的num_of_key陣列+1

int main(int argc,char* argv[])
{
	int prod_count = num_of_producer,cons_count = num_of_consumer;
	FILE* files[MAX_FILES];
	int file_count = 0;
	double starttime,endtime;
	
	//開始計時
	starttime = omp_get_wtime();
	
	//從儲存文章的路徑中找出所有檔案來讀，並以files[]陣列指向各個檔案
	DIR *dir;
	{
		char dirname[100];
		sprintf(dirname,"/home/C14046036/article");
		dir = opendir(dirname);
	}
	if(dir)
	{
		struct dirent *entry;
		while((entry = readdir(dir))!=NULL)
		{
			if(entry->d_name[0] == '.')continue;
			char art[]="article/";
			strcat(art,entry->d_name);
			files[file_count] = fopen(art,"r");
			if(files[file_count++] == NULL)
			{
				perror("Error opening file");
				return(-1);
			}
		}
	}
	closedir(dir);

	//從key.txt中讀取keyword，經由strtok分割後，儲存到keyword[]陣列裡
	printf("\nKeyword : ");
	FILE* fp;char line[100],*token;
	fp = fopen("key.txt","r");
	while(fgets(line,100,fp)!=NULL)
	{
		token = strtok(line," ,.-\n\r");
		while(token != NULL)
		{
			strcpy(keyword[key_count++],token);
			printf("%s ",keyword[key_count-1]);
			token = strtok(NULL," ,.-\n\r");
		}
	}
	printf("\n\n");
	fclose(fp);

	//利用producers從所有文章中逐行讀取並插入single shared queue中，consumers會從此queue中取出data並進行tokenize，計數其中所包含的各keyword數
	handle(prod_count,cons_count,files,file_count);
	
	//停止計時
	endtime = omp_get_wtime();

#pragma omp barrier
	//輸出所有keyword各出現的總數，以及使用時間
	int i;
	printf("-----------Keyword Total------------\n");
	for(i=0;i<key_count;i++)
	{
		printf("%s:%d\n",keyword[i],num_of_key[i]);
	}
	printf("-----------Keyword Total------------\n");
	printf("Total time = %lf\n",endtime-starttime);

	return 0;
}

void handle(int prod_count,int cons_count,FILE* files[],int file_count)
{
	int thread_count = prod_count + cons_count;//producers數與consumers數的和即為thread數
	struct node* queue_head = NULL;
	struct node* queue_tail = NULL;
	int prod_done = 0;
#pragma omp parallel num_threads(thread_count) default(none) shared(file_count,queue_head,queue_tail,files,prod_count,cons_count,prod_done)
	{
		int my_rank = omp_get_thread_num(),i;
		if(my_rank<prod_count)
		{//producers
			for(i=my_rank;i<file_count;i=i+prod_count)
			{
				readfile(files[i],&queue_head,&queue_tail,my_rank);
			}
#pragma omp critical
			prod_done++;
		}
		else
		{//consumers
			for(i=0;i<4000000;i++);//此迴圈delay一段時間，是為了讓producers能先enqueue一些nodes，consumers才有nodes可以dequeue
			struct node* tmp_node;
			while(prod_done<prod_count)
			{
				tmp_node = dequeue(&queue_head,&queue_tail,my_rank);
				
				if(tmp_node!=NULL)
				{
#pragma omp critical
					tokenize(tmp_node->data,my_rank);
					free(tmp_node);
				}
			}
			while(queue_head!=NULL)
			{
				tmp_node = dequeue(&queue_head,&queue_tail,my_rank);
				if(tmp_node!=NULL)
				{
#pragma omp critical
					tokenize(tmp_node->data,my_rank);
					free(tmp_node);
				}
			}
		}
	}
}

void readfile(FILE* file,struct node** queue_head,struct node** queue_tail,int my_rank)
{
	char* line = malloc(MAX_CHAR*sizeof(char));
	while(fgets(line,MAX_CHAR,file)!=NULL)
	{
		printf("Thread %d => %s",my_rank,line);
		enqueue(line,queue_head,queue_tail);
		line = malloc(MAX_CHAR*sizeof(char));
	}
	fclose(file);
}

void enqueue(char* line,struct node** queue_head,struct node** queue_tail)
{
	struct node* tmp_node = NULL;
	tmp_node = malloc(sizeof(struct node));
	tmp_node->data = line;
	tmp_node->next = NULL;
	//將欲插入的node插到single shared queue尾端
#pragma omp critical
	if(*queue_tail == NULL)
	{
		*queue_head = tmp_node;
		*queue_tail = tmp_node;
	}
	else
	{
		(*queue_tail)->next = tmp_node;
		*queue_tail = tmp_node;
	}
}

struct node* dequeue(struct node** queue_head,struct node** queue_tail,int my_rank)
{
	struct node* tmp_node = NULL;
	//將欲取出的node由single shared queue頭端取出，並return此node指標
	if(*queue_head == NULL)
		return NULL;
#pragma omp critical
	{
		if(*queue_head == *queue_tail)
			*queue_tail = (*queue_tail)->next;
		tmp_node = *queue_head;
		*queue_head = (*queue_head)->next;
	}
	return tmp_node;
}

void tokenize(char* line,int my_rank)
{
	int i;char *word;
	word = strtok(line," ,.-\n\r");
	while(word != NULL)
	{
		for(i=0;i<key_count;i++)
		{
			//因不論大小寫故以strcasecmp判斷之
			if(strcasecmp(word,keyword[i])==0)
			{
#pragma omp atomic
				num_of_key[i]++;
			}
		}
		word = strtok(NULL," ,.-\n\r");
	}
}
