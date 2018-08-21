#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <fstream>
#include <omp.h>
#include <string.h>

#define MAX_CITIES 50
#define MAX_DIST 1000
#define MAX_TOUR (MAX_CITIES * MAX_DIST)
#define MAX_ANTS 500
#define ALPHA 1.0
#define BETA 5.0 //增加pheromone
#define RHO 0.5 //pheromone 蒸發率
#define QVAL 100
#define MAX_TOURS 10000
#define MAX_ITERATION MAX_TOURS*MAX_CITIES
#define INIT_PHER 1.0/MAX_CITIES

using namespace std;

struct ant{
	int curCity, nextCity, pathIndex;
	int tabu[MAX_CITIES];
	int path[MAX_CITIES];
	double tourLength;
};

int cities[MAX_CITIES];
ant ants[MAX_ANTS];
double dist[MAX_CITIES][MAX_CITIES];
double phero[MAX_CITIES][MAX_CITIES];
double best=(double)MAX_TOUR;
int bestIndex;
int max_cities = 17;
char matrix[20] ;

//初始化dist[],phero[],以及ant的相關屬性
void initial()
{
	int from,to,ant_num;
    	ifstream fi;
    	fi.open(matrix);
	
	//creating cities
	for(from = 0; from < max_cities; from++)
	{
		for(to=0;to<max_cities;to++)
		{
			dist[from][to] = 0.0;
			phero[from][to] = INIT_PHER;
		}
	}
	
	//computing distance
	for(from = 0; from < max_cities; from++)
	{
		for( to =0; to < max_cities; to++)
		{
			fi>>dist[from][to];
			dist[to][from] = dist[from][to];
		}
	}
	
	//initializing the ANTs
	to = 0;
	for( ant_num = 0; ant_num < MAX_ANTS; ant_num++)
	{
		if(to == max_cities)
			to=0;
		
		ants[ant_num].curCity = to++;
		
		for(from = 0; from < max_cities; from++)
		{
			ants[ant_num].tabu[from] = 0;
			ants[ant_num].path[from] = -1;
		}
		
		ants[ant_num].pathIndex = 1;
		ants[ant_num].path[0] = ants[ant_num].curCity;
		ants[ant_num].nextCity = -1;
		ants[ant_num].tourLength = 0;
		//loading first city into tabu list
		ants[ant_num].tabu[ants[ant_num].curCity] =1;
		
	}
}

//重新初始化ant的各個屬性
void restart()
{
	int ant_num,i,to = 0;
#pragma omp for private(to)
	for(ant_num = 0; ant_num < MAX_ANTS; ant_num++)
	{
		if(ants[ant_num].tourLength < best)
		{
			best = ants[ant_num].tourLength;
			bestIndex = ant_num;
		}
		
		ants[ant_num].nextCity = -1;
		ants[ant_num].tourLength = 0.0;
		
		for(i=0;i<max_cities;i++)
		{
			ants[ant_num].tabu[i] = 0;
			ants[ant_num].path[i] = -1;
		}
		
		if(to == max_cities)
			to=0;
			
		ants[ant_num].curCity = to++;
		ants[ant_num].pathIndex = 1;
		ants[ant_num].path[0] = ants[ant_num].curCity;
		ants[ant_num].tabu[ants[ant_num].curCity] = 1;
	}
}

double compute(int from, int to)
{
	return(( pow( phero[from][to], ALPHA) * pow( (1.0/ dist[from][to]), BETA)));
}

//選擇下一個要走的城市
int select_city(int ant_num)
{
	int from, to;
	long double denom = 0.0;

	from=ants[ant_num].curCity;
	
	for(to=0; to < max_cities; to++)
	{
		if(ants[ant_num].tabu[to] == 0)
		{
			denom = denom + compute(from,to);
		}
	}
	
	do
	{
		double p;
		to++;		
		if(to >= max_cities)
			to=0;
		if(ants[ant_num].tabu[to] == 0)
		{
			if(denom != 0.0)
				p = compute(from,to)/denom;
			double x = ((double)rand()/RAND_MAX); 
			if(x < p)
				break;
		}
	}while(1);
	
	return to;
}

int simulate()
{
	int k;
	int move = 0;
#pragma omp for
	for(k=0; k<MAX_ANTS; k++)
	{
		//checking if there are any more cities to visit
		if( ants[k].pathIndex < max_cities )
		{
			ants[k].nextCity = select_city(k);
			ants[k].tabu[ants[k].nextCity] = 1;
			ants[k].path[ants[k].pathIndex++] = ants[k].nextCity;
			ants[k].tourLength = ants[k].tourLength + dist[ants[k].curCity][ants[k].nextCity];
			
			//last city to first city
			if(ants[k].pathIndex == max_cities)
			{
				ants[k].tourLength = ants[k].tourLength + dist[ants[k].path[max_cities -1]][ants[k].path[0]];
			}
			ants[k].curCity = ants[k].nextCity;
			move++;
		}
	}
	return move;
}

//更新phero[]
void update()
{
	int from,to,i,ant_num;
	//Pheromone Evaporation
#pragma omp for	
	for(from=0; from<max_cities;from++)
	{
		for(to=0;to<max_cities;to++)
		{
			if(from!=to)
			{
				phero[from][to] = phero[from][to] * (1.0 - RHO);
				if(phero[from][to]<0.0)
				{
					phero[from][to] = INIT_PHER;
				}
			}
		}
	}
	
	//Add new pheromone to the trails
#pragma omp for
	for(ant_num = 0; ant_num < MAX_ANTS; ant_num++)
	{
		for(i=0;i<max_cities;i++)
		{	
			if( i < max_cities-1 )
			{
				from = ants[ant_num].path[i];
				to = ants[ant_num].path[i+1];
			}
			else
			{
				from = ants[ant_num].path[i];
				to = ants[ant_num].path[0];
			}
			phero[from][to] = phero[from][to] + (QVAL/ ants[ant_num].tourLength);
			phero[to][from] = phero[from][to];
			
		}
	}
#pragma omp for
	for (from=0; from < max_cities;from++)
	{
		for( to=0; to < max_cities; to++)
		{
			phero[from][to] = phero[from][to] * RHO;
		}
	}
	
}

int main(int argc,char* argv[])
{
	max_cities = atoi(argv[1]);
	int min_tour ;
	if(max_cities == 17)
	{
		min_tour = 2085;
		strcpy(matrix,"gr17_d.txt");
	}
	else if(max_cities == 26)
	{
		min_tour = 937;
		strcpy(matrix,"fri26_d.txt");
	}
	else if(max_cities == 42)
	{
		min_tour = 699;
		strcpy(matrix,"dantzig42_d.txt");
	}
	else if(max_cities == 48)
	{
		min_tour = 10628;
		strcpy(matrix,"att48_d.txt");
	}
	int id,proc;
	int iteration = 0;
    	double starttime=0.0,endtime=0.0;
	
	srand(time(NULL));
	
	initial();
	starttime = omp_get_wtime();
#pragma omp parallel  num_threads(50)
	{
		for(iteration = 0; iteration < MAX_ITERATION; iteration++)
		{
			if(simulate() == 0)
			{
				update();
				restart();
#pragma omp barrier
			}
			if(best == min_tour)break;
		}
		
	}
	
	endtime = omp_get_wtime();
	cout<<endl<<"My minimal tour has length "<<best<<endl;
	cout<<"Time : "<<endtime-starttime<<" s"<<endl<<endl;
		
	return 0;
}
