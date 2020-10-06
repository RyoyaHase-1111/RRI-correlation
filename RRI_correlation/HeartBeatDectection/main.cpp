#define _CRT_SECURE_NO_WARNINGS
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

struct tm global_time;

int ReadFile_ID = 6;
double *tdata; //計測された離散時間のデータ
int *sdata[2];	//センサデータ
int tstep_cnt;  //時系列データのステップ数

//------------------------------------------------------
double rnd() //0以上1未満
{
	return((double)(rand() % RAND_MAX) / RAND_MAX);
}

//------------------------------------------------------
double rnd2() //0以上1以下
{
	return((double)((rand() % (RAND_MAX + 1)) / RAND_MAX));
}

//------------------------------------------------------
double rndn()
{
	return (rnd() + rnd() + rnd() + rnd() + rnd() + rnd() +
		rnd() + rnd() + rnd() + rnd() + rnd() + rnd() - 6.0);
}


//------------------------------------------------------
void release_alloc()
{
	int i;

	for (i = 0; i < 2; i++) {
		free(sdata[i]);
	}
	free(tdata);
}

//------------------------------------------------------
void init() 
{
	int i, j;
	FILE *fp;
	const int buf_size = 100;
	char buf[buf_size];
	char rfilename[30] = { 0 };
	int row = 0, col = 3; //行と列の数

	sprintf(rfilename, "data%d.csv", ReadFile_ID);

	// データ数のカウント
	fp = fopen(rfilename, "r");
	if (fp) {
		while (fgets(buf, buf_size, fp) != NULL) {
			row++;
			memset(buf, 0, sizeof(buf));
		}
		printf("Row= %d , Column = %d \n", row, col);
	}
	tstep_cnt = row;
	
	// 動的メモリの確保
	tdata = (double*)malloc(sizeof(double)*tstep_cnt);
	for(i = 0; i < 2; i++)
		sdata[i] = (int*)malloc(sizeof(int)*tstep_cnt);

	// 動的メモリで確保した変数にデータ入力
	fp = fopen(rfilename, "r");
	if (fp) {
		for (i = 0; i < tstep_cnt; i++) {
			for (j = 0; j < col - 1; j++) {
				if(j == 0) fscanf(fp, "%lf,", &tdata[i]);
				else fscanf(fp, "%d\n", &sdata[0][i]); //ECG
			}
			//fscanf(fp, "%d\n", &sdata[1][i]); //BCG

			//printf("%.2lf, %d, %d\n", tdata[i], sdata[0][i], sdata[1][i]);
		}

		fclose(fp);
	}
}

//--------------------------------------------------
void ECG_HearBeatDect() 
{
	int i, j, k, t = 0, s = 1;
	int ini = 50, max_steps = 100, max_data = 0; // ini: 相関の確認の開始ステップ，max_steps: 領域の幅
	double ar = 0, ave = 0,max_ar;
	double hb_time[2] = {0}; // 0: 現在, 1: 1ステップ前 
	double alfa = 1.0;
	int hb_time_memory;
	int year, month, date, hour, min, sec; 

	FILE *fp;
	char wfilename[50] = { 0 };

	//--------------書き出しファイルを時間でネーミングするため----------------
	time_t tt = time(NULL);
	localtime_s(&global_time, &tt);
	year = global_time.tm_year + 1900;
	month = global_time.tm_mon + 1;
	date = global_time.tm_yday + 1;
	hour = global_time.tm_hour;
	min = global_time.tm_min;
	sec = global_time.tm_sec;
	//-----------------------------------------------------------------------

	// RRIのファイル書き出し
	sprintf(wfilename, "RRI_data%d_%d%d%d%d%d-%d%d%d%d%d%d.csv", ReadFile_ID, 
		year, (int)(month / 10), (int)(month % 10), (int)(date / 10), (int)(date % 10),
		(int)(hour / 10), (int)(hour % 10), (int)(min / 10), (int)(min % 10), (int)(sec / 10), (int)(sec % 10));
	
	//----------------------------------------------------------------------

	fp = fopen(wfilename, "w");

	while (s) {
		for (i = ini; i < ini + max_steps; i++) {
			if ((t + i * 2) >= tstep_cnt) { s = 0; break; }
			ave = 0;
			ar = 0;
			for (j = t; j < i * 2 + t; j++) ave += (1.0 / (double)(i * 2)) * (double)sdata[0][j]; //平均値
			for (j = t; j < i + t; j++) ar += (1.0 / (double)i) * (((double)sdata[0][j] - ave) * ((double)sdata[0][j + i] - ave));

			if (i == ini) {
				k = i;
				max_ar = ar;
			}
			else {
				if (max_ar < ar) {
					k = i;
					max_ar = ar; //相関最大
				}
			}
		}

		alfa = 1.0;
		if (t > 0) alfa = 1.25; //探索領域を推定される周期の1.25倍に拡張して探索（初回の探索は除く）
		max_data = -100; j = t; //自己相関から相関最大となる周期がわかるので，波形のその範囲で最大値を探索し，R波とする．
		for (i = t; i < k * alfa + t; i++) {
			if (max_data < sdata[0][i]) {
				double p = 1.0;
				if(t > 1) p = exp(-(((i - t) - hb_time_memory) * ((i - t) - hb_time_memory)) / pow((double)k * 0.5, 2));
				if (p > 0.5) {
					max_data = sdata[0][i];
					j = i;
				}
			}
		}
		hb_time_memory = j - t; //一時的にR波の位置を記憶
		hb_time[1] = hb_time[0];
		hb_time[0] = tdata[j];
		//printf("%d, %d, %d, %.2lf, %.3lf\n", j, hb_time_memory, k, max_ar, hb_time[0] - hb_time[1]);
		printf("%d, %.3lf\n", j, hb_time[0] - hb_time[1]);
		if (fp) fprintf(fp, "%lf, %lf\n", hb_time[0], hb_time[0] - hb_time[1]);
		
		if (s == 0) { //時系列データ終盤の探索
			max_data = -100; j = t + k; //自己相関から相関最大となる周期がわかるので，波形のその範囲で最大値を探索し，R波とする．
			for (i = t + k; i < 2 * k + t; i++) {
				if (max_data < sdata[0][i]) {
					double p;
					p = exp(-(((i - t - k) - hb_time_memory) * ((i - t - k) - hb_time_memory)) / pow((double)k * 0.5, 2));
					if (p > 0.5) {
						max_data = sdata[0][i];
						j = i;
					}
				}
			}

			hb_time[1] = hb_time[0];
			hb_time[0] = tdata[j];
			printf("%d, %.3lf\n", j, hb_time[0] - hb_time[1]);
			if (fp) { fprintf(fp, "%lf, %lf\n", hb_time[0], hb_time[0] - hb_time[1]); fclose(fp); }
		}

		max_steps = k;
		ini = (double)k * 0.5;
		t += k;
	}
}

//--------------------------------------------------
void main()
{
	init();
	ECG_HearBeatDect();
	release_alloc();
}